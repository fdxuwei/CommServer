#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <muduo/base/Logging.h>
#include <muduo/net/InetAddress.h>
#include <commserv/CommServer.h>

#ifndef CONN_TEST
#define CONN_TEST 0
#endif

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

void ipChanged(CommServer *cs, const vector<ServInfo> &addList, const vector<ServInfo> &delList)
{
	//
	cout << "ip changed" << endl;
	//
	for(int i = 0; i < addList.size(); ++i)
	{
		cs->connect(addList[i].name, addList[i].ip, addList[i].port);
	}
	//
	for(int i = 0; i < delList.size(); ++i)
	{
		cs->removeServer(delList[i].name, delList[i].ip, delList[i].port);
	}
}

void mock(CommServer *cs)
{
	vector<ServInfo> addList;
	vector<ServInfo> delList;
	static int enrty = 0;
	
	if(0 == enrty)
	{
		addList.push_back(ServInfo("provider", "192.168.1.107", 8000));
		addList.push_back(ServInfo("provider", "192.168.1.107", 8001));
		addList.push_back(ServInfo("provider", "192.168.1.107", 8002));
		delList.push_back(ServInfo("provider", "192.168.1.107", 8003));
		delList.push_back(ServInfo("provider", "192.168.1.107", 8004));
	}
	else if(1 == enrty)
	{
		addList.push_back(ServInfo("provider", "192.168.1.107", 8003));
		addList.push_back(ServInfo("provider", "192.168.1.107", 8004));
		delList.push_back(ServInfo("provider", "192.168.1.107", 8001));
		delList.push_back(ServInfo("provider", "192.168.1.107", 8002));
		delList.push_back(ServInfo("provider", "192.168.1.107", 8000));
	}
	else if(2 == enrty)
	{
		addList.push_back(ServInfo("provider", "192.168.1.107", 8005));
		delList.push_back(ServInfo("provider", "192.168.1.107", 8005));
	}
	enrty = (++enrty)%3;

	ipChanged(cs, addList, delList);
}

void handleBlock(CommServer *cs, const TcpConnectionPtr &conn, const PacketBuffer &pb)
{
	sleep(50);
}

int main(int ac, char *av[])
{
	Config cfg;
	//
	if(!parseArgs(ac, av, cfg))
	{
		return -1;
	}
	//
	muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
	//
	EventLoop loop;
	CommServer server(&loop, 4);
	//
	server.setServreInfo(cfg.localInfo.name, cfg.localInfo.ip, cfg.localInfo.port);
	// listen
	server.listen();
	// connect
	for(int i = 0; i < cfg.remoteInfos.size(); ++i)
	{
		server.connect(cfg.remoteInfos[i].name, cfg.remoteInfos[i].ip, cfg.remoteInfos[i].port);
	}
	int taskThreadId = server.createThreadPool(1);
	server.registerHandler(8, handleBlock, taskThreadId);
#if CONN_TEST
	loop.runEvery(0.01, boost::bind(mock, &server));
#endif
	loop.loop();
	return 0;
}