#ifndef _PROTO_OBJS_H_
#define _PROTO_OBJS_H_

#include <string>
#include <muduo/base/Logging.h>
#include "SimpleJson.h"
//
class JsonObjs
{
	typedef SimpleJson::StringRefType StringRefType;
public:
	//
	const char * toJson()
	{
		pack();
		sj_.write();
		return sj_.jsonStr(); 
	}
	const char * jsonStr()
	{
		return sj_.jsonStr(); 
	}

	int jsonSize() const
	{
		return sj_.jsonSize();
	}
	//
	bool fromJson(const char *json, int size)
	{
		try
		{
			sj_.parse(json, size);
			parse();
		}
		catch(const std::string &key)
		{
			LOG_ERROR << "Json need " << key;
			return false;
		}
		return true; 
	}
	//
	template<class T>
	void set(StringRefType key, const T &val)
	{
		sj_.set(key, val);
	}
	//
	template<class T>
	void get(StringRefType key, T &val)
	{
		sj_.get(key, val);
	}
	// do not copy SimpleJson
	JsonObjs()
	{}
	JsonObjs(const JsonObjs& jo)
	{}
	JsonObjs& operator=(const JsonObjs& jo)
	{}

protected:
	virtual void pack() = 0;
	virtual void parse() = 0;
private:
	SimpleJson sj_;
};
// 
class ServerInfo: public JsonObjs
{
public:
	virtual void pack()
	{
		set("appid", appid);
		set("servtype", servtype);
		set("servno", servno);
	}
	virtual void parse()
	{
		get("appid", appid);
		get("servtype", servtype);
		get("servno", servno);
	}
	//
	int appid;
	int servtype;
	int servno;
};
//
class AddrInfo: public JsonObjs
{
public:
	//
	virtual void pack()
	{
		set("ip", ip);
		set("port", port);
	}
	virtual void parse()
	{
		get("ip", ip);
		get("port", port);
	}
	//
	std::string ip;
	int port;

};


#endif