#include <stdio.h>
#include <assert.h>
#include <boost/any.hpp>
#include <muduo/base/Logging.h>
#include "ServerPool.h"
#include "SafeRand.h"
#include "ServerId.h"

using namespace std;
using namespace muduo::net;

bool ServerPool::send(const std::string &servName, const std::string &servIp, unsigned servPort, const char *data, int size)
{
	TcpConnectionPtr conn = getConn(servName, servIp, servPort);
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

bool ServerPool::sendRandom(const std::string &servName, const char *data, int size)
{
	TcpConnectionPtr conn = getConnRandom(servName);
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

muduo::net::TcpConnectionPtr ServerPool::getConn(const std::string &servName, const std::string &servIp, unsigned servPort)
{
	std::string servId = makeServId(servName, servIp, servPort);
	//
	{
		ReadLock rl(mutex_);
		ConnMapType::iterator itr = connMap_.find(servId);
		if(connMap_.end() == itr)
		{
			LOG_WARN << "can not find connection of server " << servId;
			return muduo::net::TcpConnectionPtr();
		}
		return itr->second;
	}
}

muduo::net::TcpConnectionPtr ServerPool::getConnRandom(const std::string &servName)
{
	//
	{
		ReadLock rl(mutex_);
		ConnVecMapType::iterator itm = connVecMap_.find(servName);
		if(connVecMap_.end() == itm || itm->second.empty())
		{
			LOG_WARN << "can not find any connection of server " << servName;
			return muduo::net::TcpConnectionPtr();
		}
		int randIndex = safeRand()%itm->second.size();
		return itm->second[randIndex];
	}
}

void ServerPool::addServer(const std::string &servName, const std::string &servIp, unsigned servPort, const muduo::net::TcpConnectionPtr &conn)
{
	std::string servId = makeServId(servName, servIp, servPort);
	//
	LOG_WARN << "add to server pool, serverId=" << servId;
	{
		WriteLock wl(mutex_);
		pair<ConnMapType::iterator, bool> org;
		org = connMap_.insert(std::make_pair(servId, conn));
		if(org.second)
		{
			// newly inserted
			connVecMap_[servName].push_back(conn);
		}
		else
		{
			// already exist, replace it;
			LOG_WARN << "ServerPool replace connection, original is " << org.first->second->name() << ", new is" << conn->name();
			assert(removeFromConnVec(servName, org.first->second));
			org.first->second = conn;
			connVecMap_[servName].push_back(conn);
		}
	}
}

void ServerPool::removeServer(const std::string &servName, const std::string &servIp, unsigned servPort, const muduo::net::TcpConnectionPtr &conn)
{
	std::string servId = makeServId(servName, servIp, servPort);
	//
	LOG_WARN << "add to server pool, serverId=" << servId;
	{
		WriteLock wl(mutex_);
		ConnMapType::iterator itr = connMap_.find(servId);
		if(connMap_.end() == itr)
		{
			LOG_WARN << "remove, connection " << servId << " dosn't exist.";
			return;
		}
		if(conn->name() != itr->second->name())
		{
			// we should not remove a reconnected connection for the same server
			LOG_WARN << "remove, connection name not match, target is " << conn->name() << ", found is " << itr->second->name();
			return;
		}
		assert(removeFromConnVec(servName, itr->second));
		connMap_.erase(itr);	
	}
}


bool ServerPool::removeFromConnVec(const std::string &servName, const muduo::net::TcpConnectionPtr &conn)
{
	ConnVecMapType::iterator itm = connVecMap_.find(servName);
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
