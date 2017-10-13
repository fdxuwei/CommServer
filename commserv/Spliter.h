#ifndef SPLITER_H
#define SPLITER_H

#include <string>
#include <vector>

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
			svec_.push_back(std::string(str, tmp-str));
			if(*tmp == '\0')
				break;
			str = ++tmp;
		}
	}
	const std::string &operator[](int indx) const
	{
		if(indx < svec_.size())
			return svec_[indx];
		else
			return empty_;
	}
	int size() const {return svec_.size(); }
private:
	std::string empty_;
	std::vector<std::string> svec_;
};

#endif