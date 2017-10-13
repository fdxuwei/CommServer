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
	std::string remoteServName;
};

bool parseArgs(int ac, char *av[], Config &cfg)
{
	if(ac < 3)
	{
		string usage = "Usage: main -l name:ip:port] [-r name]";
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
		case 'r':
			{
				cfg.remoteServName = optarg;
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
class Master
{
public:
	Master(EventLoop *loop)
		: loop_ (loop)
		, server_ (loop, 4) // 4 threads
		, taskNum_ (0)
	{
	}
	bool init(const ServInfo &localInfo, const std::string &remoteServName)
	{
		consumerName_ = remoteServName;
		//
		server_.registerHandler(DPI_TASK_DONE, boost::bind(&Master::handleTaskDone, this, _1, _2, _3));
		//
		server_.setServreInfo(localInfo.name, localInfo.ip, localInfo.port);
		//
		broker_.init(&server_, "127.0.0.1:2181");
		//
		if(!broker_.care(remoteServName))
		{
			cout << "care server failed." << endl;
			return false;
		}
		return true;
	}
	//
	void dispatchTask()
	{
		loop_->runAfter(5.0, boost::bind(&Master::dispatchTaskInLoop, this));
	}
	//
	void handleTaskDone(CommServer *s, const TcpConnectionPtr &conn, const PacketBuffer& pb)
	{
		TaskDone td;
		td.fromJson(pb.data(), pb.size());
		//
		{
			muduo::MutexLockGuard lg(mutex_);
			doneTasks_.insert(td.taskid);
			if(doneTasks_.size() == taskNum_)
			{
				LOG_INFO << "All tasks is done!";
			}
		}
	}
private:
	EventLoop *loop_;
	CommServer server_;
	ServerBroker broker_;
	int taskNum_;
	muduo::MutexLock mutex_;
	std::string consumerName_;
	set<int> doneTasks_;
	//
	//
	void dispatchTaskInLoop()
	{
		taskNum_ = 100000;
		LOG_INFO << "dispatch " << taskNum_ << " task";
		for(int i = 0; i < taskNum_; ++i)
		{
			TaskAssign ta;
			ta.taskid = i;
			ta.taskContent.assign(400, 'x');// packet of about 400 bytes
			ta.toJson();
			server_.sendPacketRandom(consumerName_, DPI_TASK_ASSIGN, ta.jsonStr(), ta.jsonSize());
		}
	}

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
	Master server(&loop);
	//
	if(!server.init(cfg.localInfo, cfg.remoteServName))
	{
		return -1;
	}
	//
	server.dispatchTask();
	loop.loop();
	return 0;
}