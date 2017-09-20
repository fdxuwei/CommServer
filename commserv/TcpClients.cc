#include <stdio.h>
#include <boost/bind.hpp>
#include <muduo/base/Logging.h>
#include <muduo/net/InetAddress.h>
#include "TcpClients.h"
#include "ServerId.h"

using namespace std;
using namespace muduo;
using namespace muduo::net;

TcpClients::TcpClients(EventLoop *loop)
	: loop_ (loop)
	, threadNum_ (0)
	, threadPool_ (loop_, "")
{

}

void TcpClients::start()
{
	threadPool_.start();
}

void TcpClients::connect(const std::string &servName, const std::string &ip, int port)
{
	loop_->runInLoop(boost::bind(&TcpClients::connectInLoop, this, servName, ip, port));
}

void TcpClients::removeTcpClient(const std::string &servName, const std::string &ip, int port)
{
	loop_->runInLoop(boost::bind(&TcpClients::removeTcpClientInLoop, this, servName, ip, port));
}

void TcpClients::removeTcpClientIfNotRetry(const std::string &servName, const std::string &ip, int port)
{
	loop_->runInLoop(boost::bind(&TcpClients::removeTcpClientIfNotRetryInLoop, this, servName, ip, port));
}

void TcpClients::connectInLoop(const std::string &servName, const std::string &ip, int port)
{
	// only run in main loop
	loop_->assertInLoopThread();
	//
	std::string servId = makeServId(servName, ip, port);
	if(tcpClients_.find(servId) != tcpClients_.end())
	{
		LOG_WARN << "There is already a same client exists: " << servId;
		return;
	}
	//
	TcpClientPtr tcp(new TcpClient(threadPool_.getNextLoop(), InetAddress(ip, port), servId));
	tcp->setConnectionCallback(boost::bind(connectionCallback_, _1, servId));
	tcp->setMessageCallback(messageCallback_);
	tcp->enableRetry();
	tcp->connect();
	// add to map
	tcpClients_[servId] = tcp;
}

void TcpClients::removeTcpClient(const TcpClientPtr &c)
{
	// do nothing
}

void TcpClients::removeTcpClientInLoop(const std::string &servName, const std::string &ip, int port)
{
	// only run in main loop
	loop_->assertInLoopThread();
	//
	//
	string servId = makeServId(servName, ip, port);
	TcpClientMap::iterator itr = tcpClients_.find(servId);
	if(tcpClients_.end() != itr)
	{
		TcpClientPtr cp= itr->second;
		cp->getLoop()->runInLoop(boost::bind(&TcpClient::stop, cp.get()));
		// cp->stop();
		if(cp->connection())
		{
			cp->connection()->forceClose();
		}
		//
		cp->getLoop()->runInLoop(boost::bind(&TcpClients::removeTcpClient, this, cp));
		tcpClients_.erase(itr);
	}
}

void TcpClients::removeTcpClientIfNotRetryInLoop(const std::string &servName, const std::string &ip, int port)
{
	// only run in main loop
	loop_->assertInLoopThread();
	//
	string servId = makeServId(servName, ip, port);
	TcpClientMap::iterator it = tcpClients_.find(servId);
	if(tcpClients_.end() == it)
		return;
	//
	if(!it->second->retry())
	{
		LOG_WARN << "Erase tcp client, name=" << servId;
		tcpClients_.erase(it);
	}
}
