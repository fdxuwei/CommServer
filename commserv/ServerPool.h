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
	bool send(int appid, int servtype, int servno, const char *data, int size); // thread safe
	bool sendRandom(int appid, int servtype, const char *data, int size); // thread safe
	void addServer(int appid, int servtype, int servno, const muduo::net::TcpConnectionPtr &conn); // thread safe
	void removeServer(int appid, int servtype, int servno, const muduo::net::TcpConnectionPtr &conn); // thread safe
//	void getAllConnections(std::vector<const muduo::net::TcpConnectionPtr conn> &connVec); // thread safe
private:
	void makeServerName(int appid, int servtype, int servno, std::string &name);
	void makeServerVecName(int appid, int servtype, std::string &name);
	muduo::net::TcpConnectionPtr getConn(int appid, int servtype, int servno);
	muduo::net::TcpConnectionPtr getConnRandom(int appid, int servtype);
	bool removeFromConnVec(const std::string &vecName, const muduo::net::TcpConnectionPtr &conn);
	//
	typedef std::map<std::string, muduo::net::TcpConnectionPtr> ConnMapType;
	typedef std::vector<muduo::net::TcpConnectionPtr> ConnVecType;
	typedef std::map<std::string, ConnVecType> ConnVecMapType;
	ConnMapType connMap_;
	ConnVecMapType connVecMap_;
	//
	typedef boost::shared_mutex MutexType;
	typedef boost::shared_lock_guard<MutexType> ReadLock;
	typedef boost::lock_guard<MutexType> WriteLock;
	MutexType mutex_;
};

#endif