#include <stdio.h>
#include <assert.h>
#include <boost/any.hpp>
#include <muduo/base/Logging.h>
#include "ServerPool.h"
#include "SafeRand.h"

using namespace std;
using namespace muduo::net;

bool ServerPool::send(int appid, int servtype, int servno, const char *data, int size)
{
	TcpConnectionPtr conn = getConn(appid, servtype, servno);
	//
	if(conn)
	{
		conn->send(data, size);
		return true;
	}
	else
	{
		LOG_WARN << "no active connections";
		return false;
	}
}

bool ServerPool::sendRandom(int appid, int servtype, const char *data, int size)
{
	TcpConnectionPtr conn = getConnRandom(appid, servtype);
	//
	if(conn)
	{
		conn->send(data, size);
		return true;
	}
	else
	{
		LOG_WARN << "no active connections";
		return false;
	}
}

muduo::net::TcpConnectionPtr ServerPool::getConn(int appid, int servtype, int servno)
{
	std::string serverName;
	makeServerName(appid, servtype, servno, serverName);
	//
	{
		ReadLock rl(mutex_);
		ConnMapType::iterator itr = connMap_.find(serverName);
		if(connMap_.end() == itr)
		{
			LOG_WARN << "can not find connection of server " << serverName;
			return muduo::net::TcpConnectionPtr();
		}
		return itr->second;
	}
}

muduo::net::TcpConnectionPtr ServerPool::getConnRandom(int appid, int servtype)
{
	std::string serverVecName;
	makeServerVecName(appid, servtype, serverVecName);
	//
	{
		ReadLock rl(mutex_);
		ConnVecMapType::iterator itm = connVecMap_.find(serverVecName);
		if(connVecMap_.end() == itm || itm->second.empty())
		{
			LOG_WARN << "can not find any connection of server type " << serverVecName;
			return muduo::net::TcpConnectionPtr();
		}
		int randIndex = safeRand()%itm->second.size();
		return itm->second[randIndex];
	}
}

void ServerPool::addServer(int appid, int servtype, int servno, const muduo::net::TcpConnectionPtr &conn)
{
	std::string serverName, serverVecName;
	makeServerName(appid, servtype, servno, serverName);
	makeServerVecName(appid, servtype, serverVecName);
	//
	LOG_WARN << "add to server pool, serverName=" << serverName;
	{
		WriteLock wl(mutex_);
		pair<ConnMapType::iterator, bool> org;
		org = connMap_.insert(std::make_pair(serverName, conn));
		if(org.second)
		{
			// newly inserted
			connVecMap_[serverVecName].push_back(conn);
		}
		else
		{
			// already exist, replace it;
			LOG_WARN << "ServerPool replace connection, original is " << org.first->second->name() << ", new is" << conn->name();
			assert(removeFromConnVec(serverVecName, org.first->second));
			org.first->second = conn;
			connVecMap_[serverVecName].push_back(conn);
		}
	}
}

void ServerPool::removeServer(int appid, int servtype, int servno, const muduo::net::TcpConnectionPtr &conn)
{
	std::string serverName, serverVecName;
	makeServerName(appid, servtype, servno, serverName);
	makeServerVecName(appid, servtype, serverVecName);
	//
	LOG_WARN << "remove from server pool, serverName=" << serverName;
	{
		WriteLock wl(mutex_);
		ConnMapType::iterator itr = connMap_.find(serverName);
		if(connMap_.end() == itr)
		{
			LOG_WARN << "remove, connection " << serverName << " dosn't exist.";
			return;
		}
		if(conn->name() != itr->second->name())
		{
			// we should not remove a reconnected connection for the same server
			LOG_WARN << "remove, connection name not match, target is " << conn->name() << ", found is " << itr->second->name();
			return;
		}
		assert(removeFromConnVec(serverVecName, itr->second));
		connMap_.erase(itr);	
	}
}


bool ServerPool::removeFromConnVec(const std::string &vecName, const muduo::net::TcpConnectionPtr &conn)
{
	ConnVecMapType::iterator itm = connVecMap_.find(vecName);
	if(connVecMap_.end() == itm)
	{
		return false;
	}
	for(ConnVecType::iterator itv = itm->second.begin(); itv != itm->second.end(); ++itv)
	{
		if((*itv)->name() == conn->name())
		{
			itm->second.erase(itv);
			return true;
		}
	}
	return false;
}

ServerPool::~ServerPool()
{
	LOG_TRACE << "ServerPool::~ServerPool";
}

void ServerPool::makeServerName(int appid, int servtype, int servno, std::string &name)
{
	char buf[128];
	int len = snprintf(buf, sizeof(buf), "%d_%d_%d", appid, servtype, servno);
	name.assign(buf, len);
}

void ServerPool::makeServerVecName(int appid, int servtype, std::string &name)
{
	char buf[128];
	int len = snprintf(buf, sizeof(buf), "%d_%d", appid, servtype);
	name.assign(buf, len);
}