#ifndef _COMM_SERVER_H_
#define _COMM_SERVER_H_

#include <string>
#include <vector>
#include <map>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <muduo/base/Timestamp.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/EventLoopThreadPool.h>
#include "MsgHandler.h"
#include "TcpClients.h"
#include "ServerPool.h"

//
#define SYS_PROTO_BASE 0
#define CTL_PROTO_BASE 50
#define USR_PROTO_BASE 100
enum PROTO_ID
{
	PI_HEART_REQ = SYS_PROTO_BASE,
	PI_HEART_ACK = SYS_PROTO_BASE+1,
	PI_CONNECT_REQ = SYS_PROTO_BASE+2,
	PI_CONNECT_ACK = SYS_PROTO_BASE+3,
};

//

enum CONN_TYPE
{
	CT_SERVER,
	CT_CLIENT
};

struct ConnContext
{
	friend class CommServer;
public:
	
protected:
	std::string clientName;
	CONN_TYPE ctype;
};

typedef boost::function<void ()> ThreadPoolCallback;

class PacketBuffer;
class CommServer: public boost::enable_shared_from_this<CommServer>
{
	friend class CommHandler;
public:
	CommServer(muduo::net::EventLoop *loop, int threadNum = 0);
	void setServreInfo(int appid, int servtype, int servno);
	void listen(const std::string &ip, int port);
	void connect(const std::string &ip, int port);
	void sendPacket(const muduo::net::TcpConnectionPtr &conn, int proto, const char *data, int size);
	void sendPacket(int stype, int no, int proto, const char *data, int size);
	void sendPacket(int stype, int no, int appid, int proto, const char *data, int size);
	void sendPacketRandom(int stype, int proto, const char *data, int size);
	//
	void setHeartbeatTime(int seconds);
	//
	void registerHandler(int proto, const MsgFunc& handler);
	void registerHandler(int proto, const MsgFunc& handler, int threadPoolId);
	//
	int createThreadPool(int threadNum);
	void runInThreadPool(int threadPoolId, const ThreadPoolCallback &cb);
private:
	//
	void onServerConnection(const muduo::net::TcpConnectionPtr &conn);
	void onClientConnection(const muduo::net::TcpConnectionPtr &conn, const std::string &clientName);
	void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp time);
	//
	void makePacket(int proto, const char *in, int inSize, char *out, int outBufsize, int &outSize);
	//
	void handleHeartReq(CommServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);
	void handleHeartAck(CommServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);
	void handleConnectReq(CommServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);
	void handleConnectAck(CommServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);
	//
	typedef std::map<int, MsgHandler> MsgHandlerMap;
	MsgHandlerMap handlers_;
	//
	int appid_;
	int servtype_;
	int servno_;
	//
	muduo::net::EventLoop *loop_;
	int threadNum_;
	//
	int heartbeatTime_; // seconds
	//
	typedef boost::shared_ptr<muduo::net::TcpServer> TcpServerPtr;
	TcpServerPtr tcpServer_;
	//
	TcpClients tcpClients_;
	//
	typedef boost::shared_ptr<muduo::ThreadPool> ThreadPoolPtr;
	typedef std::vector<ThreadPoolPtr> ThreadPoolVec;
	ThreadPoolVec threadPools_;
	int autoThreadPoolId_;
	//
	ServerPool serverPool_;

};

#endif