#ifndef SERVER_ID_H
#define SERVER_ID_H

#include <string>
#include <stdio.h>

inline std::string makeServId(const std::string &servName, const std::string &ip, int port)
{
	char buf[128] = {0};
	snprintf(buf, sizeof(buf), "%s:%s:%d", servName.c_str(), ip.c_str(), port);
	return buf;
}

#endif