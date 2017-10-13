#ifndef SERVER_BROKER_H
#define SERVER_BROKER_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <boost/function.hpp>
#include <ZooKeeper.h>
#include "CommServer.h"

class ServerBroker
{
public:
	ServerBroker();
	bool init(CommServer *cs, const std::string& zkString);
	bool regist(const std::string &servName, const std::string &servIp, unsigned servPort);
	bool care(const std::string &servName);

private:
	void servCallback(const std::string &path, const std::vector<std::string> &value);
	const static std::string rootPath_;
	CommServer *cs_;
	ZooKeeper zk_;
	typedef std::set<std::string> NetAddrSet;
	typedef std::map<std::string, NetAddrSet> NetAddrMap; 
	NetAddrMap servsMap_;
};

#endif