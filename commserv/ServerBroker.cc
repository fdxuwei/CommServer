#include <sstream>
#include <boost/bind.hpp>
#include <muduo/base/Logging.h>
#include "ServerBroker.h"
#include "ServerId.h"
#include "Spliter.h"

using namespace std;

const std::string ServerBroker::rootPath_ = "/commserver";

ServerBroker::ServerBroker()
	: cs_ (NULL)
{

};

bool ServerBroker::init(CommServer *cs, const std::string& zkString)
{
	cs_ = cs;
	ZkRet zr = zk_.init(zkString);
	if(!zr)
	{
		LOG_ERROR << "initialize zookeeper failed.";
	}
	return (bool)zr;
}

bool ServerBroker::regist(const std::string &servName, const std::string &servIp, unsigned servPort)
{
	std::stringstream ssPath;
	ssPath << rootPath_ << "/" << servName << "/" << servIp << ":" << servPort;
	ZkRet zr = zk_.createEphemeralNode(ssPath.str(), "");
	if(!zr)
	{
		LOG_ERROR << "zookeeper create ephemeral node failed.";
	}
	return (bool)zr;
}

bool ServerBroker::care(const std::string &servName)
{
	std::stringstream ssPath;
	ssPath << rootPath_ << "/" << servName;
	ZkRet zr = zk_.watchChildren(ssPath.str(), boost::bind(&ServerBroker::servCallback, this, _1, _2));
	if(!zr)
	{
		LOG_ERROR << "zookeeper watch children failed.";
	}
	return (bool)zr;

}

void ServerBroker::servCallback(const std::string &path, const std::vector<std::string> &value)
{
	std::string servName = zk_.getNodeName(path);
	NetAddrMap::iterator it = servsMap_.find(servName);
	
	if(servsMap_.end() == it)
	{
		// first initialize
		for(int i = 0; i < value.size(); ++i)
		{
			Spliter sp(":", value[i].c_str());
			cs_->connect(servName, sp[0], atoi(sp[1].c_str()));
			//
			NetAddrSet &nas = servsMap_[servName];
			nas.insert(value[i]);
		}
		return;
	}
	//
	NetAddrSet &oldAddrSet = it->second;
	NetAddrSet newAddrSet;
	for(int i = 0; i < value.size(); ++i)
	{
		newAddrSet.insert(value[i]);
	}
	
	// find and remove address
	for(NetAddrSet::iterator ito = oldAddrSet.begin(); ito != oldAddrSet.end(); ++ito)
	{
		if(newAddrSet.find(*ito) == newAddrSet.end())
		{
			Spliter sp(":", ito->c_str());
			cs_->removeServer(servName, sp[0], atoi(sp[1].c_str()));
			oldAddrSet.erase(*ito);
		}
	}

	// find and add address
	for(NetAddrSet::iterator itn = newAddrSet.begin(); itn != newAddrSet.end(); ++itn)
	{
		if(oldAddrSet.find(*itn) == oldAddrSet.end())
		{
			Spliter sp(":", itn->c_str());
			cs_->connect(servName, sp[0], atoi(sp[1].c_str()));
			oldAddrSet.insert(*itn);
		}
	}

}
