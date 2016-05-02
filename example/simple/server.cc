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

void ipChanged(CommServer *cs, const vector<InetAddress> &addList, const vector<InetAddress> &delList)
{
	//
	cout << "ip changed" << endl;
	//
	for(int i = 0; i < addList.size(); ++i)
	{
		cs->connect(addList[i].toIp(), addList[i].toPort());
	}
	//
	for(int i = 0; i < delList.size(); ++i)
	{
		cs->removeClient(delList[i].toIp(), delList[i].toPort());
	}
}

void mock(CommServer *cs)
{
	vector<InetAddress> addList;
	vector<InetAddress> delList;
	static int enrty = 0;
	
	if(0 == enrty)
	{
		addList.push_back(InetAddress("192.168.1.107", 8000));
		addList.push_back(InetAddress("192.168.1.107", 8001));
		addList.push_back(InetAddress("192.168.1.107", 8002));
		delList.push_back(InetAddress("192.168.1.107", 8003));
		delList.push_back(InetAddress("192.168.1.107", 8004));
	}
	else if(1 == enrty)
	{
		addList.push_back(InetAddress("192.168.1.107", 8003));
		addList.push_back(InetAddress("192.168.1.107", 8004));
		delList.push_back(InetAddress("192.168.1.107", 8001));
		delList.push_back(InetAddress("192.168.1.107", 8002));
		delList.push_back(InetAddress("192.168.1.107", 8000));
	}
	else if(2 == enrty)
	{
		addList.push_back(InetAddress("192.168.1.107", 8005));
		delList.push_back(InetAddress("192.168.1.107", 8005));
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
	server.setServreInfo(cfg.appid, cfg.servtype, cfg.servno);
	// listen
	if(cfg.localAddr)
	{
		server.listen(cfg.localAddr->toIp(), cfg.localAddr->toPort());
	}
	// connect
	for(int i = 0; i < cfg.remoteAddr.size(); ++i)
	{
		server.connect(cfg.remoteAddr[i].toIp(), cfg.remoteAddr[i].toPort());
	}
	int taskThreadId = server.createThreadPool(1);
	server.registerHandler(8, handleBlock, taskThreadId);
#if CONN_TEST
	loop.runEvery(0.01, boost::bind(mock, &server));
#endif
	loop.loop();
	return 0;
}