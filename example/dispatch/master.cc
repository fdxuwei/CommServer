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
		case 'r':
			{
				Spliter sp(":,", optarg);
				for(int i = 0; i < sp.size(); i+=3)
				{
					ServInfo si;
					si.name = sp[i];
					si.ip = sp[i+1];
					si.port = atoi(sp[i+2].c_str());
					cfg.remoteInfos.push_back(si);
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
class Master
{
public:
	Master(EventLoop *loop)
		: loop_ (loop)
		, server_ (loop, 4) // 4 threads
		, taskNum_ (0)
	{
	}
	void init(const ServInfo &localInfo, const vector<ServInfo> &remoteInfoList)
	{
		server_.registerHandler(DPI_TASK_DONE, boost::bind(&Master::handleTaskDone, this, _1, _2, _3));
		//
		server_.setServreInfo(localInfo.name, localInfo.ip, localInfo.port);
		//
		if(remoteInfoList.size() > 0)
		{
			consumerName_ = remoteInfoList[0].name;
		}
		//
		// connect to worker
		for(int i = 0; i < remoteInfoList.size(); ++i)
		{
			server_.connect(remoteInfoList[i].name, remoteInfoList[i].ip, remoteInfoList[i].port);
		}

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
	server.init(cfg.localInfo, cfg.remoteInfos);
	server.dispatchTask();
	loop.loop();
	return 0;
}