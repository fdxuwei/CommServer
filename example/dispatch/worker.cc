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


struct Config
{
	int appid;
	int servtype;
	int servno;
	boost::shared_ptr<InetAddress> localAddr;
	vector<InetAddress> remoteAddr;
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
	if(ac < 4)
	{
		string usage = "Usage: main -a appid -t servtype -n servno [-l ip:port] [-r ip:port,..]";
		cout << usage << endl;
		return false;
	}
	//
	int oc;
	while((oc = getopt(ac, av, ":a:t:n:l:r:")) != -1)
	{
		switch(oc)
		{
		case 'a':
			cfg.appid = atoi(optarg);
			break;
		case 't':
			cfg.servtype = atoi(optarg);
			break;
		case 'n':
			cfg.servno = atoi(optarg);
			break;
		case 'l':
			{
				Spliter sp(":", optarg);
				cfg.localAddr.reset(new InetAddress(sp[0], atoi(sp[1].c_str())));
			}
			break;
		case 'r':
			{
				Spliter sp(":,", optarg);
				for(int i = 0; i < sp.size(); i+=2)
				{
					cfg.remoteAddr.push_back(InetAddress(sp[i], atoi(sp[i+1].c_str())));
				}
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
	void init(int appid, int servtype, int servno, InetAddress &addr)
	{
		server_.registerHandler(DPI_TASK_ASSIGN, boost::bind(&Worker::handleTaskAssign, this, _1, _2, _3));
		//
		appid_ = appid;
		servtype_ = servtype;
		servno_ = servno;
		server_.setServreInfo(appid, servtype, servno);
		//
		server_.listen(addr.toIp(), addr.toPort());

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
	int appid_;
	int servtype_;
	int servno_;
	EventLoop *loop_;
	CommServer server_;
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
	server.init(cfg.appid, cfg.servtype, cfg.servno, *cfg.localAddr.get());
	loop.loop();
	return 0;
}