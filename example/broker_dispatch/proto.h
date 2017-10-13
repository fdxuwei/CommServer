#ifndef _PROTO_H_
#define _PROTO_H_

#include <string>
#include <commserv/ProtoObjs.h>

enum DispProtoId
{
	DPI_TASK_ASSIGN = USR_PROTO_BASE+1,
	DPI_TASK_DONE = USR_PROTO_BASE+2
};
//
class TaskAssign: public JsonObjs
{
public:
	virtual void pack()
	{
		set("id", taskid);
		set("content", taskContent);
	}
	virtual void parse()
	{
		get("id", taskid);
		get("content", taskContent);
	}
	//
	int taskid;
	std::string taskContent;
};
//
class TaskDone: public JsonObjs
{
public:
	virtual void pack()
	{
		set("id", taskid);
	}
	virtual void parse()
	{
		get("id", taskid);
	}
	//
	int taskid;
};


#endif