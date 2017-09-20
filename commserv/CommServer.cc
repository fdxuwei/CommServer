#include <assert.h>
#include <stdint.h>
#include <boost/bind.hpp>
#include <boost/any.hpp>
#include <muduo/base/Logging.h>
#include <muduo/net/Endian.h>
#include <muduo/net/InetAddress.h>
#include "CommServer.h"
#include "PacketBuffer.h"
#include "ProtoObjs.h"
#include "ServerId.h"

using namespace std;
using namespace muduo;
using namespace muduo::net;

#define MAX_PACKET_SIZE 65536
#define BASE_PROTO_HEAD_LEN 8
#define INVALID_THREADPOOL_ID -1

CommServer::CommServer(muduo::net::EventLoop *loop, int threadNum /* =0 */)
	: loop_ (loop)
	, threadNum_ (threadNum)
	, heartbeatTime_ (60)
	, tcpClients_ (loop)
	, autoThreadPoolId_ (INVALID_THREADPOOL_ID)
{
	//
	registerHandler(PI_CONNECT_REQ, boost::bind(&CommServer::handleConnectReq, this, _1, _2 ,_3));
	registerHandler(PI_CONNECT_ACK, boost::bind(&CommServer::handleConnectAck, this, _1, _2 ,_3));
	//
	tcpClients_.setConnectionCallback(boost::bind(&CommServer::onClientConnection, this, _1, _2));
	tcpClients_.setMessageCallback(boost::bind(&CommServer::onMessage, this, _1, _2, _3));
	tcpClients_.setThreadNum(threadNum);
	tcpClients_.start();
}

void CommServer::setHeartbeatTime(int seconds)
{
	heartbeatTime_ = seconds;
}

void CommServer::setServreInfo(const std::string &servName, const std::string &servIp, unsigned servPort)
{
	servName_ = servName;
	servIp_ = servIp;
	servPort_ = servPort;
	LOG_INFO << "Server name=" << servName_ << ", server ip=" << servIp_ << ", server port=" << servPort;
}

void CommServer::listen()
{
	assert(!tcpServer_);
	LOG_INFO << "Listen on " << servIp_ << ":" << servPort_;
	tcpServer_.reset(new TcpServer(loop_, InetAddress(servIp_, servPort_), ""));
	tcpServer_->setConnectionCallback(boost::bind(&CommServer::onServerConnection, this, _1));
	tcpServer_->setMessageCallback(boost::bind(&CommServer::onMessage, this, _1, _2, _3));
	tcpServer_->setThreadNum(threadNum_);
	tcpServer_->start();
}

void CommServer::connect(const std::string &servName, const std::string &ip, int port)
{
	LOG_INFO << "Connect to " << servName << ":" << ip << ":" << port;
	tcpClients_.connect(servName, ip, port);
}

void CommServer::onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp time)
{
	int readableBytes;
	int packetLen;
	while(((readableBytes = buf->readableBytes()) >= BASE_PROTO_HEAD_LEN)
		&& ((packetLen = buf->peekInt32()) <= readableBytes))
	{
		if(packetLen < BASE_PROTO_HEAD_LEN || packetLen > MAX_PACKET_SIZE)
		{
			LOG_ERROR << "invalid packet length: " << packetLen;
			conn->forceClose();
			return;
		}
		// handle packet
		int dataSize = buf->readInt32()-BASE_PROTO_HEAD_LEN;
		int proto = buf->readInt32();
		//
		MsgHandlerMap::iterator itr = handlers_.find(proto);
		if(handlers_.end() == itr)
		{
			LOG_ERROR << "Can not find handler of protocol " << proto;
			return;
		}
		int threadPoolId = itr->second.threadPoolId();
		if(INVALID_THREADPOOL_ID == threadPoolId)
		{
			PacketBuffer pb(buf->peek(), dataSize);
			itr->second.doIt(this, conn, pb);
		}
		else
		{
			// all threadPoolIds in MsgHandlerMap is valid
			assert(threadPools_.size() > threadPoolId && 0 >= threadPoolId);
			boost::shared_array<char> sa(new char[dataSize]);
			memcpy(sa.get(), buf->peek(), dataSize);
			PacketBuffer pb(sa, dataSize);
			threadPools_[threadPoolId]->run(boost::bind(&MsgHandler::doIt, &itr->second, this, conn, pb));
		}
		//
		buf->retrieve(dataSize);
	}
}

void CommServer::onServerConnection(const muduo::net::TcpConnectionPtr &conn)
{
	//
	if(conn->connected())
	{
		// init context
		ConnContext *ctx = boost::any_cast<ConnContext>(conn->getMutableContext());
		if(NULL == ctx)
		{
			conn->setContext(ConnContext());
			ctx = boost::any_cast<ConnContext>(conn->getMutableContext());
			ctx->ctype = CT_SERVER;
		}
		//
	}
	else
	{
		const ConnContext &ctx = boost::any_cast<ConnContext>(conn->getContext());
		LOG_WARN << "connection down, conn=" << ctx.servId;
	}
}

void CommServer::onClientConnection(const muduo::net::TcpConnectionPtr &conn, const std::string &servId)
{
	//
	if(conn->connected())
	{
		// init context
		ConnContext *ctx = boost::any_cast<ConnContext>(conn->getMutableContext());
		if(NULL == ctx)
		{
			conn->setContext(ConnContext());
			ctx = boost::any_cast<ConnContext>(conn->getMutableContext());
			ctx->ctype = CT_CLIENT;
			ctx->servId = servId;
		}
		// send self info 
		ServerInfo si;
		si.servName = servName_;
		si.servIp = servIp_;
		si.servPort = servPort_;
		si.toJson();
		//
		sendPacket(conn, PI_CONNECT_REQ, si.jsonStr(), si.jsonSize());
	}
	else
	{
		const ConnContext &ctx = boost::any_cast<ConnContext>(conn->getContext());
		LOG_WARN << "connection down, conn=" << ctx.servId;
	//	tcpClients_.removeTcpClientIfNotRetry(clientName);
	}
}

void CommServer::makePacket(int proto, const char *in, int inSize, char *out, int outBufsize, int &outSize)
{
	using namespace muduo::net::sockets;
	//
	outSize = BASE_PROTO_HEAD_LEN + inSize;
	assert(outSize <= outBufsize);
	char *tmp = out;
	*(uint32_t*)tmp = hostToNetwork32(outSize);
	tmp += sizeof(uint32_t);
	*(uint32_t*)tmp = hostToNetwork32(proto);
	tmp += sizeof(uint32_t);
	memcpy(tmp, in, inSize);
}

void CommServer::sendPacket(const muduo::net::TcpConnectionPtr &conn, int proto, const char *data, int size)
{
	//
	char buf[MAX_PACKET_SIZE];
	int packetLen;
	makePacket(proto, data, size, buf, sizeof(buf), packetLen);
	//
	conn->send(buf, packetLen);

}

void CommServer::sendPacket(const std::string &servName, const std::string &servIp, unsigned servPort, int proto, const char *data, int size)
{
	char buf[MAX_PACKET_SIZE];
	int packetLen;
	makePacket(proto, data, size, buf, sizeof(buf), packetLen);
	//
	serverPool_.send(servName, servIp, servPort, buf, packetLen);
}

void CommServer::sendPacketRandom(const std::string &servName, int proto, const char *data, int size)
{
	char buf[MAX_PACKET_SIZE];
	int packetLen;
	makePacket(proto, data, size, buf, sizeof(buf), packetLen);
	//
	serverPool_.sendRandom(servName, buf, packetLen);
}

void CommServer::registerHandler(int proto, const MsgFunc& handler)
{
	loop_->assertInLoopThread();
	//
	assert(handlers_.end() == handlers_.find(proto));
	handlers_[proto] = MsgHandler(handler, INVALID_THREADPOOL_ID);
}

void CommServer::registerHandler(int proto, const MsgFunc& handler, int threadPoolId)
{
	loop_->assertInLoopThread();
	//
	assert(handlers_.end() == handlers_.find(proto));
	handlers_[proto] = MsgHandler(handler, threadPoolId);
}

int CommServer::createThreadPool(int threadNum)
{
	loop_->assertInLoopThread();
	//
	++autoThreadPoolId_;
	assert(autoThreadPoolId_ == threadPools_.size());
	//
	ThreadPoolPtr tpp(new muduo::ThreadPool);
	tpp->start(threadNum);
	threadPools_.push_back(tpp);
	//
	return autoThreadPoolId_;
}

void CommServer::runInThreadPool(int threadPoolId, const ThreadPoolCallback &cb)
{
	assert(threadPoolId < threadPools_.size() && threadPoolId >= 0);
	//
	threadPools_[threadPoolId]->run(cb);
}
//

void CommServer::handleConnectReq(CommServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
	ServerInfo si;
	if(!si.fromJson(data.data(), data.size()))
	{
		LOG_ERROR << "Error packet: handleConnectReq";
		conn->forceClose();
		return ;
	}
	//
	ConnContext *ctx = boost::any_cast<ConnContext>(conn->getMutableContext());
	assert(ctx);
	ctx->servId = makeServId(si.servName, si.servIp, si.servPort);
	//
	serverPool_.addServer(si.servName, si.servIp, si.servPort, conn);
	// response
	ServerInfo thisInfo;
	thisInfo.servName = servName_;
	thisInfo.servIp = servIp_;
	thisInfo.servPort = servPort_;
	thisInfo.toJson();
	sendPacket(conn, PI_CONNECT_ACK, thisInfo.jsonStr(), thisInfo.jsonSize());
}
//
void CommServer::handleConnectAck(CommServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
	ServerInfo si;
	if(!si.fromJson(data.data(), data.size()))
	{
		LOG_ERROR << "Error packet: handleConnectAck";
		conn->forceClose();
		return ;
	}
	//
	const ConnContext &ctx = boost::any_cast<ConnContext>(conn->getContext());
	std::string siId = makeServId(si.servName, si.servIp, si.servPort);
	if(ctx.servId != siId)
	{
		LOG_ERROR << "server info not match: local=" << ctx.servId << ", remote=" << siId;
		conn->forceClose();
		return;
	}
	//
	serverPool_.addServer(si.servName, si.servIp, si.servPort, conn);
}

void CommServer::removeServer(const std::string &servName, const std::string &servIp, unsigned servPort)
{
	tcpClients_.removeTcpClient(servName, servIp, servPort);
}