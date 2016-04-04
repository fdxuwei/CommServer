#ifndef _SIMPLE_JSON_H_
#define _SIMPLE_JSON_H_

#include <stdint.h>
#include <string.h>
#include <string>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

class SimpleJson
{
public:
	typedef rapidjson::Value::StringRefType StringRefType;
	SimpleJson();
	// parse
	void parse(const char *json, int size);
	// StringRefType is faster than const char *
	template<class T>
	void get(StringRefType key, T &data);
	// pack
	void set(StringRefType key, const char *data); 
	template<class T>
	void set(StringRefType key, const T &data); 

	//
	void write(); // must be called before the following 2 functions
	const char *jsonStr() const { return sb_.GetString(); }
	int jsonSize() const{ return static_cast<int>(sb_.GetSize()); }
	//
private:
	rapidjson::Document doc_;
	rapidjson::StringBuffer sb_;
};

inline SimpleJson::SimpleJson()
	: doc_(rapidjson::kObjectType)
{
};

inline void SimpleJson::parse(const char *json, int size)
{
//	std::string str(json, size);
	doc_.Parse(json, size);
	if(doc_.HasParseError())
	{
		std::stringstream ssErr;
		ssErr << "errcode=" << doc_.GetParseError() << ", json=" << std::string(json, size);
		throw ssErr.str();
	}
}

template<>
inline void SimpleJson::get<int>(StringRefType key, int &data)
{
	rapidjson::Value::MemberIterator itrDt = doc_.FindMember(key);
	if(doc_.MemberEnd() == itrDt)
	{
		std::stringstream ssErr;
		ssErr << "json need " << key;
		throw ssErr.str();
	}
	if(!itrDt->value.IsInt())
	{
		std::stringstream ssErr;
		ssErr << "not an int type, key=" << key;
		throw ssErr.str();
	}
	data = itrDt->value.GetInt();
}

template<>
inline void SimpleJson::get<unsigned>(StringRefType key, unsigned &data)
{
	rapidjson::Value::MemberIterator itrDt = doc_.FindMember(key);
	if(doc_.MemberEnd() == itrDt)
	{
		std::stringstream ssErr;
		ssErr << "json need " << key;
		throw ssErr.str();
	}
	if(!itrDt->value.IsUint())
	{
		std::stringstream ssErr;
		ssErr << "not an unsigned type, key=" << key;
		throw ssErr.str();
	}
	data = itrDt->value.GetUint();
}

template<>
inline void SimpleJson::get<int64_t>(StringRefType key, int64_t &data)
{
	rapidjson::Value::MemberIterator itrDt = doc_.FindMember(key);
	if(doc_.MemberEnd() == itrDt)
	{
		std::stringstream ssErr;
		ssErr << "json need " << key;
		throw ssErr.str();
	}
	if(!itrDt->value.IsInt64())
	{
		std::stringstream ssErr;
		ssErr << "not an int64 type, key=" << key;
		throw ssErr.str();
	}
	data = itrDt->value.GetInt64();
}

template<>
inline void SimpleJson::get<uint64_t>(StringRefType key, uint64_t &data)
{
	rapidjson::Value::MemberIterator itrDt = doc_.FindMember(key);
	if(doc_.MemberEnd() == itrDt)
	{
		std::stringstream ssErr;
		ssErr << "json need " << key;
		throw ssErr.str();
	}
	if(!itrDt->value.IsUint64())
	{
		std::stringstream ssErr;
		ssErr << "not an uint64 type, key=" << key;
		throw ssErr.str();
	}
	data = itrDt->value.GetUint64();
}

template<>
inline void SimpleJson::get<std::string>(StringRefType key, std::string &data)
{
	rapidjson::Value::MemberIterator itrDt = doc_.FindMember(key);
	if(doc_.MemberEnd() == itrDt)
	{
		std::stringstream ssErr;
		ssErr << "json need " << key;
		throw ssErr.str();
	}
	if(!itrDt->value.IsString())
	{
		std::stringstream ssErr;
		ssErr << "not an string type, key=" << key;
		throw ssErr.str();
	}
	data = itrDt->value.GetString();
}

template<>
inline void SimpleJson::get<double>(StringRefType key, double &data)
{
	rapidjson::Value::MemberIterator itrDt = doc_.FindMember(key);
	if(doc_.MemberEnd() == itrDt)
	{
		std::stringstream ssErr;
		ssErr << "json need " << key;
		throw ssErr.str();
	}
	if(!itrDt->value.IsDouble())
	{
		std::stringstream ssErr;
		ssErr << "not an double type, key=" << key;
		throw ssErr.str();
	}
	data = itrDt->value.GetDouble();
}

template<>
inline void SimpleJson::get<short>(StringRefType key, short &data)
{
	rapidjson::Value::MemberIterator itrDt = doc_.FindMember(key);
	if(doc_.MemberEnd() == itrDt)
	{
		std::stringstream ssErr;
		ssErr << "json need " << key;
		throw ssErr.str();
	}
	if(!itrDt->value.IsInt())
	{
		std::stringstream ssErr;
		ssErr << "not an int type, key=" << key;
		throw ssErr.str();
	}
	data = static_cast<short>(itrDt->value.GetInt());
}

template<>
inline void SimpleJson::get<unsigned short>(StringRefType key, unsigned short &data)
{
	rapidjson::Value::MemberIterator itrDt = doc_.FindMember(key);
	if(doc_.MemberEnd() == itrDt)
	{
		std::stringstream ssErr;
		ssErr << "json need " << key;
		throw ssErr.str();
	}
	if(!itrDt->value.IsUint())
	{
		std::stringstream ssErr;
		ssErr << "not an uint type, key=" << key;
		throw ssErr.str();
	}
	data = static_cast<unsigned short>(itrDt->value.GetUint());
}

template<>
inline void SimpleJson::get<bool>(StringRefType key, bool &data)
{
	rapidjson::Value::MemberIterator itrDt = doc_.FindMember(key);
	if(doc_.MemberEnd() == itrDt)
	{
		std::stringstream ssErr;
		ssErr << "json need " << key;
		throw ssErr.str();
	}
	if(itrDt->value.IsInt())
	{
		data = static_cast<bool>(itrDt->value.GetInt());
	}
	else if(itrDt->value.IsBool())
	{
		data = itrDt->value.GetBool();
	}
	else
	{
		std::stringstream ssErr;
		ssErr << "not an int or bool type, key=" << key;
		throw ssErr.str();
	}
}
	
// pack

inline void SimpleJson::write()
{
	rapidjson::Writer<rapidjson::StringBuffer> writer(sb_);
    if(!doc_.Accept(writer))
		sb_.Clear();
}

//
inline void SimpleJson::set(StringRefType key, const char *data)
{
	rapidjson::Value vald(data, strlen(data));
	doc_.AddMember(key, vald, doc_.GetAllocator());
	assert(vald.IsNull());
}

template<class T>
void SimpleJson::set(StringRefType key, const T &data)
{
	rapidjson::Value vald(data);
	doc_.AddMember(key, vald, doc_.GetAllocator());
	assert(vald.IsNull());
}

template<>
inline void SimpleJson::set<std::string>(StringRefType key, const std::string &data)
{
	rapidjson::Value vald(data.c_str(), data.size());
	doc_.AddMember(key, vald, doc_.GetAllocator());
	assert(vald.IsNull());
}



#endif