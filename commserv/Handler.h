#ifndef _HANDLER_H_
#define _HANDLER_H_

#include <boost/shared_ptr.hpp>
#include <muduo/net/TcpConnection.h>

class Handler
{
public:
	virtual bool handle(const muduo::net::TcpConnectionPtr &conn, int proto, const char* data, int size) = 0;
};

typedef boost::shared_ptr<Handler> HandlerPtr;

#endif 