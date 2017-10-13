#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <unistd.h>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <commserv/CommServer.h>
#include <commserv/PacketBuffer.h>
#include <commserv/Spliter.h>
#include <commserv/ServerBroker.h>
#include "proto.h"

using namespace std;
using namespace muduo;
using namespace muduo::net;


struct ServInfo
{
	ServInfo(){}
	ServInfo(const std::string&n, const std::string &i, unsigned p)
		: name (n)
		, ip (i)
		, port (p)
	{
	}
	std::string name;
	std::string ip;
	unsigned port;
};

struct Config
{
	ServInfo localInfo;
};

bool parseArgs(int ac, char *av[], Config &cfg)
{
	if(ac < 2)
	{
		string usage = "Usage: main -l name:ip:port]";
		cout << usage << endl;
		return false;
	}
	//
	int oc;
	while((oc = getopt(ac, av, "l:")) != -1)
	{
		switch(oc)
		{
		case 'l':
			{
				Spliter sp(":", optarg);
				cfg.localInfo.name = sp[0];
				cfg.localInfo.ip = sp[1];
				cfg.localInfo.port = atoi(sp[2].c_str());
			}
			break;
		case '?':
			cout << "invalid option: " << static_cast<char>(optopt) << endl;
			return false;
		case ':':
			cout << "lack argument of option: " << static_cast<char>(optopt) << endl;
			return false;

		default:
			return false;
		}
	}
	return true;
}

class Worker
{
public:
	Worker(EventLoop *loop)
		: loop_ (loop)
		, server_ (loop, 4) // 4 threads
	{
	}
	bool init(const ServInfo &localInfo)
	{
		server_.registerHandler(DPI_TASK_ASSIGN, boost::bind(&Worker::handleTaskAssign, this, _1, _2, _3));
		//
		server_.setServreInfo(localInfo.name, localInfo.ip, localInfo.port);
		//
		server_.listen();
		// register address
		broker_.init(&server_, "127.0.0.1:2181");
		if(!broker_.regist(localInfo.name, localInfo.ip, localInfo.port))
		{
			cout << "register server failed!" << endl;
			return false;
		}
		return true;
	}
	//
	void handleTaskAssign(CommServer *s, const TcpConnectionPtr &conn, const PacketBuffer& pb)
	{
		TaskAssign ta;
		ta.fromJson(pb.data(), pb.size());
		//
		cout << "received task: " << ta.taskid << endl;
		//
		TaskDone td;
		td.taskid = ta.taskid;
		td.toJson();
		//
		server_.sendPacket(conn, DPI_TASK_DONE, td.jsonStr(), td.jsonSize());
	}
private:
	EventLoop *loop_;
	CommServer server_;
	ServerBroker broker_;
	std::string consumerName_;
	//
};


int main(int ac, char *av[])
{
	Config cfg;
	//
	if(!parseArgs(ac, av, cfg))
	{
		return -1;
	}
	//
	muduo::Logger::setLogLevel(muduo::Logger::INFO);
	//
	EventLoop loop;
	Worker server(&loop);
	//
	if(!server.init(cfg.localInfo))
	{
		return -1;
	}
	//
	loop.loop();
	return 0;
}