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
	vector<ServInfo> remoteInfos;
};

class Spliter
{
public:
	// split by any char in delim
	Spliter(const char *delim, const char *str)
	{
		bool delims[256] = {false};
		int i = 0;
		while(*delim != '\0')
		{
			delims[*delim++] = true;
		}
		//
		const char *tmp = str;
		while(*tmp != '\0')
		{
			while(*tmp != '\0' && !delims[*tmp])
				++tmp;
			svec_.push_back(string(str, tmp-str));
			if(*tmp == '\0')
				break;
			str = ++tmp;
		}
	}
	const string &operator[](int indx) const
	{
		if(indx < svec_.size())
			return svec_[indx];
		else
			return empty_;
	}
	int size() const {return svec_.size(); }
private:
	string empty_;
	vector<string> svec_;
};

bool parseArgs(int ac, char *av[], Config &cfg)
{
	if(ac < 3)
	{
		string usage = "Usage: main -l name:ip:port] [-r name:ip:port,..]";
		cout << usage << endl;
		return false;
	}
	//
	int oc;
	while((oc = getopt(ac, av, "l:r:")) != -1)
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
	void init(const ServInfo &localInfo)
	{
		server_.registerHandler(DPI_TASK_ASSIGN, boost::bind(&Worker::handleTaskAssign, this, _1, _2, _3));
		//
		server_.setServreInfo(localInfo.name, localInfo.ip, localInfo.port);
		//
		server_.listen();

	}
	//
	void handleTaskAssign(CommServer *s, const TcpConnectionPtr &conn, const PacketBuffer& pb)
	{
		TaskAssign ta;
		ta.fromJson(pb.data(), pb.size());
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
	server.init(cfg.localInfo);
	loop.loop();
	return 0;
}