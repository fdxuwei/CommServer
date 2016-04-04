#ifndef MSG_HANDLER_H
#define MSG_HANDLER_H

#include <assert.h>
#include <boost/function.hpp>
#include <muduo/net/TcpConnection.h>
//#include "CommServer.h"

class CommServer;
class PacketBuffer;
typedef boost::function<void (CommServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)> MsgFunc;

class MsgHandler
{
public:
	MsgHandler();
	MsgHandler(const MsgFunc& func);
	MsgHandler(const MsgFunc& func, int threadPoolId);
	int threadPoolId() const{ return threadPoolId_; }
	void doIt(CommServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data) 
	{ assert(func_); func_(commServer, conn, data);}
private:
	MsgFunc func_;
	int threadPoolId_;
};

#endif