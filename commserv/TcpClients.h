#ifndef TCP_CLIENTS_H
#define TCP_CLIENTS_H

#include <string>
#include <map>
#include <assert.h>
#include <boost/function.hpp>
#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/Callbacks.h>

typedef boost::shared_ptr<muduo::net::TcpClient> TcpClientPtr;
typedef boost::function<void (const muduo::net::TcpConnectionPtr &conn, const std::string &clientName)> ClientConnectionCallback;

class TcpClients
{
public:
	TcpClients(muduo::net::EventLoop *loop);
	void start();
	void connect(const std::string &ip, int port);
	void setThreadNum(int num)
	{ assert(!threadPool_.started());
	  threadPool_.setThreadNum(num);	}
	void setConnectionCallback(const ClientConnectionCallback& cb)
	{ connectionCallback_ = cb; }
	void setMessageCallback(const muduo::net::MessageCallback& cb)
	{ messageCallback_ = cb; }
	void removeTcpClient(const std::string &ip, int port);
	void removeTcpClientIfNotRetry(const std::string &ip, int port);
private:
	//
	typedef std::map<std::string, TcpClientPtr> TcpClientMap;
	//
	std::string makeClientName(const std::string &ip, int port);
	void connectInLoop(const std::string &ip, int port);
	void removeTcpClientInLoop(const std::string &ip, int port);
	void removeTcpClientIfNotRetryInLoop(const std::string &ip, int port);
	void removeTcpClient(const TcpClientPtr &c);
	//
	ClientConnectionCallback connectionCallback_;
	muduo::net::MessageCallback messageCallback_;
	// 
	int threadNum_;
	//
	muduo::net::EventLoop *loop_;
	muduo::net::EventLoopThreadPool threadPool_;
	TcpClientMap tcpClients_;
};

#endif