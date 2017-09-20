#ifndef SERVER_BROKER_H
#define SERVER_BROKER_H

#include <string>
#include <boost/function.hpp>

class ServerBroker
{
public:
	void regist(const std::string &servName, const std::string &servIp, unsigned servPort);
	void care(const std::string &servName);
};

#endif