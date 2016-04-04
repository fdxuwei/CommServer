#include <stdio.h>
#include <boost/bind.hpp>
#include <muduo/base/Logging.h>
#include <muduo/net/InetAddress.h>
#include "TcpClients.h"

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

void TcpClients::connect(const std::string &ip, int port)
{
	loop_->runInLoop(boost::bind(&TcpClients::connectInLoop, this, ip, port));
}

void TcpClients::removeTcpClient(const std::string &name)
{
	loop_->runInLoop(boost::bind(&TcpClients::removeTcpClientInLoop, this, name));
}

void TcpClients::removeTcpClientIfNotRetry(const std::string &name)
{
	loop_->runInLoop(boost::bind(&TcpClients::removeTcpClientIfNotRetryInLoop, this, name));
}

void TcpClients::connectInLoop(const std::string &ip, int port)
{
	// only run in main loop
	loop_->assertInLoopThread();
	//
	std::string clientName = makeClientName(ip, port);
	if(tcpClients_.find(clientName) != tcpClients_.end())
	{
		LOG_WARN << "There is already a same client exists: " << clientName;
		return;
	}
	//
	TcpClientPtr tcp(new TcpClient(threadPool_.getNextLoop(), InetAddress(ip, port), clientName));
	tcp->setConnectionCallback(boost::bind(connectionCallback_, _1, clientName));
	tcp->setMessageCallback(messageCallback_);
	tcp->enableRetry();
	tcp->connect();
	// add to map
	tcpClients_[clientName] = tcp;
}


void TcpClients::removeTcpClientInLoop(const std::string &name)
{
	// only run in main loop
	loop_->assertInLoopThread();
	//
	tcpClients_.erase(name);
}

void TcpClients::removeTcpClientIfNotRetryInLoop(const std::string &name)
{
	// only run in main loop
	loop_->assertInLoopThread();
	//
	TcpClientMap::iterator it = tcpClients_.find(name);
	if(tcpClients_.end() == it)
		return;
	//
	if(!it->second->retry())
	{
		LOG_WARN << "Erase tcp client, name=" << name;
		tcpClients_.erase(it);
	}
}

std::string TcpClients::makeClientName(const std::string &ip, int port)
{
	char buf[128] = {0};
	snprintf(buf, sizeof(buf), "%s:%d", ip.c_str(), port);
	return buf;
}