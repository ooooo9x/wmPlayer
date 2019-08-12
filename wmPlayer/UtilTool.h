#pragma once
#include "common.h"

struct ArgStruct
{
	std::string mode;
	std::string parentId;
	std::string sendUrl;
	std::string playUrl;
};


class UtilTool
{
public:
	~UtilTool();
	int64_t getSystemTime();
	char* wchar_t2char(wchar_t* wstr);
	wchar_t * string2wchar_t(const std::string& pKey);
	std::string base64Decoder(std::string &inStr);
	bool jsonParser(std::string &jsonStr, ArgStruct &argStruct);
	int jsonResParser(std::string &jsonStr);
	int sendHttp(std::string &mystr, std::string &sendUrl);
	void convertSecToTime(int sec, char *time);
	void convertSecToWTime(int sec, wchar_t *time, int size);
	int16_t uint8_to_int16(uint8_t high, uint8_t low);

	//单例相关
	static UtilTool  *getInstance();
	static void delInstance();

private:
	//logger
	static Logger& logger;

	//单例相关
	UtilTool();
	UtilTool & operator = (UtilTool &t);
	UtilTool(const UtilTool &);
	static UtilTool * instance;
	static std::mutex m_mt;
};