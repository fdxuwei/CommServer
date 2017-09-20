#ifndef _SERVER_POOL_H_
#define _SERVER_POOL_H_

#include <vector>
#include <map>
#include <string>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/thread/lock_guard.hpp>
#include <muduo/net/TcpConnection.h>

// server pool to hold connection of servers
class ServerPool
{
public:
	~ServerPool();
	bool send(const std::string &servName, const std::string &servIp, unsigned servPort, const char *data, int size); // thread safe
	bool sendRandom(const std::string &servName, const char *data, int size); // thread safe
	void addServer(const std::string &servName, const std::string &servIp, unsigned servPort, const muduo::net::TcpConnectionPtr &conn); // thread safe
	void removeServer(const std::string &servName, const std::string &servIp, unsigned servPort, const muduo::net::TcpConnectionPtr &conn); // thread safe
private:
	muduo::net::TcpConnectionPtr getConn(const std::string &servName, const std::string &servIp, unsigned servPort);
	muduo::net::TcpConnectionPtr getConnRandom(const std::string &servName);
	bool removeFromConnVec(const std::string &servId, const muduo::net::TcpConnectionPtr &conn);
	//
	typedef std::map<std::string, muduo::net::TcpConnectionPtr> ConnMapType;
	typedef std::vector<muduo::net::TcpConnectionPtr> ConnVecType;
	typedef std::map<std::string, ConnVecType> ConnVecMapType;
	ConnMapType connMap_; // servId->conn
	ConnVecMapType connVecMap_; // servName->conns
	//
	typedef boost::shared_mutex MutexType;
	typedef boost::shared_lock_guard<MutexType> ReadLock;
	typedef boost::lock_guard<MutexType> WriteLock;
	MutexType mutex_;
};

#endif