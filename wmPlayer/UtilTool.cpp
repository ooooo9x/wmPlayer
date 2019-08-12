#include "stdafx.h"
#include "UtilTool.h"

UtilTool::UtilTool()
{

}

UtilTool::~UtilTool()
{

}

UtilTool *UtilTool::getInstance()
{
	if (instance == nullptr)
	{
		std::lock_guard<std::mutex> lgd(m_mt);
		if (instance == nullptr)
		{
			instance = new UtilTool;
		}
	}

	return instance;
}

void UtilTool::delInstance()
{
	std::lock_guard<std::mutex> lgd(m_mt);
	if (instance)
	{
		delete instance;
		instance = nullptr;
	}
}

//Logger& UtilTool::logger = Logger::get("UtilTool");
Logger& UtilTool::logger = Logger::get(Logger::ROOT);

UtilTool * UtilTool::instance = nullptr;
std::mutex UtilTool::m_mt;

int64_t UtilTool::getSystemTime()
{
	struct timeb t;
	ftime(&t);
	return 1000 * t.time + t.millitm;
}

char* UtilTool::wchar_t2char(wchar_t* wstr) {
	int len = WideCharToMultiByte(CP_ACP, 0, wstr, wcslen(wstr), NULL, 0, NULL, NULL);
	char* cstr = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, wstr, wcslen(wstr), cstr, len, NULL, NULL);
	cstr[len] = '\0';
	return cstr;
	//delete cstr;
}

//��Ҫ������ʹ����wchar_t*��delete[]�ͷ��ڴ� 
wchar_t * UtilTool::string2wchar_t(const std::string& pKey)
{
	char* pCStrKey = (char*)pKey.c_str();
	//��һ�ε��÷���ת������ַ������ȣ�����ȷ��Ϊwchar_t*���ٶ����ڴ�ռ� 
	int pSize = MultiByteToWideChar(CP_ACP, 0, pCStrKey, strlen(pCStrKey) + 1, NULL, 0);
	wchar_t *pWCStrKey = new wchar_t[pSize];
	//�ڶ��ε��ý����ֽ��ַ���ת����˫�ֽ��ַ��� 
	MultiByteToWideChar(CP_ACP, 0, pCStrKey, strlen(pCStrKey) + 1, pWCStrKey, pSize);
	return pWCStrKey;
	//delete pWCStrKey;
}

std::string UtilTool::base64Decoder(std::string &inStr)
{
	std::istringstream istr(inStr);
	Poco::Base64Decoder b64in(istr);
	std::string outStr;
	//b64in >> s;
	int c = b64in.get();
	while (c != -1)
	{
		outStr += char(c);
		c = b64in.get();
	}

	return outStr;
}

bool UtilTool::jsonParser(std::string &jsonStr, ArgStruct &argStruct)
{
	//jsonStr = "{ \"name\" : \"yuhaiyang\" }";
	Poco::JSON::Parser parser;
	Poco::Dynamic::Var result;
	parser.reset();
	try
	{
		//��ʼ��argStruct
		argStruct.mode = "0";
		argStruct.parentId = "";
		argStruct.playUrl = "";
		argStruct.sendUrl = "";

		//����arg��
		result = parser.parse(jsonStr);

		Poco::JSON::Object::Ptr pObj = result.extract<Poco::JSON::Object::Ptr>();
		Poco::Dynamic::Var ret = pObj->get("mode");
		if (!ret.isEmpty())
		{
			argStruct.mode = ret.convert<std::string>();
		}
		Poco::Dynamic::Var ret0 = pObj->get("parentId");
		if (!ret0.isEmpty())
		{
			argStruct.parentId = ret0.convert<std::string>();
		}
		Poco::Dynamic::Var ret1 = pObj->get("playUrl"); 
		if (!ret1.isEmpty())
		{
			std::string tem = ret1.convert<std::string>();
			//std::string tem = ret2.toString();
			argStruct.playUrl = tem;
		}
		Poco::Dynamic::Var ret2 = pObj->get("sendUrl");
		if (!ret2.isEmpty())
		{
			std::string tem = ret2.convert<std::string>();
			//std::string tem = ret2.toString();
			argStruct.sendUrl = tem;
		}

		return true;
	}
	catch (const Poco::JSON::JSONException& ex)
	{
		std::string mes = ex.message();
		logger.error("jsonParser JSONException -->" + mes);
		return false;
	}
	catch (std::exception &ex)
	{
		std::string mes = ex.what();
		logger.error("jsonParser std::exception -->" + mes);
		return false;
	}
	
	return false;
}

int UtilTool::jsonResParser(std::string &jsonStr)
{
	//jsonStr = "{ \"name\" : \"yuhaiyang\" }";
	Poco::JSON::Parser parser;
	Poco::Dynamic::Var result;
	parser.reset();
	int code = -1;
	try
	{
		result = parser.parse(jsonStr);

		Poco::JSON::Object::Ptr pObj = result.extract<Poco::JSON::Object::Ptr>();
		Poco::Dynamic::Var ret = pObj->get("code");

		if (!ret.isEmpty())
		{
			code = ret.convert<int>();
		}
	}
	catch (const Poco::JSON::JSONException& ex)
	{
		std::string mes = ex.message();
		logger.error("jsonResParser JSONException -->" + mes);
	}
	catch (std::exception &ex)
	{
		std::string mes = ex.what();
		logger.error("jsonResParser std::exception -->" + mes);
	}
	
	return code;
}

int UtilTool::sendHttp(std::string &mystr,std::string &sendUrl)
{
	//Logger& logger = Logger::get("sendHttp");
	logger.information("start sendHttp url-->" + sendUrl);
	logger.information("start sendHttp str-->" + mystr);

	try
	{
		Poco::URI url(sendUrl);
		//ʹ�ø����ķ�����ip �Ͷ˿�ʵ����һ��http�ͻ����࣬����û��������.
		Poco::Net::HTTPClientSession session(url.getHost(), url.getPort());
		logger.information("����session ok");
		//����һ�������࣬��Ҳû���������ӣ�path�������·�����������http://192.168.4.19:8000/radapi10/userauth.htm��ô��ʱstring path=/radapi10/userauth.htm
		Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, url.getPathAndQuery(), Poco::Net::HTTPMessage::HTTP_1_1);

		logger.information("����request ok");
		request.setChunkedTransferEncoding(false);
		//�������ݱ��룬���ı���ʽ��Ҳ��������ΪJSON,text/xml��
		request.setContentType("application/json");

		request.setContentLength((int)mystr.length());

		//ȡ�÷�����io�������ͳ�ȥ
		std::ostream& send = session.sendRequest(request);
		logger.information("sendRequest ok");
		//���ͳ�ȥ
		send << mystr;
		logger.information("send ok-->" + mystr);

		//ȡ�÷��������ص�������������������
		Poco::Net::HTTPResponse response;
		//receiveResponse(...)����һ�������������ã��÷����¾�
		std::istream& res = session.receiveResponse(response);

		//bufferΪ���ݻ�����
		//res.read(buffer, 1024);
		std::stringstream responseStream;
		Poco::StreamCopier::copyStream(res, responseStream);
		std::string responseContent;
		responseContent = responseStream.str();

		std::cout << responseContent;
		logger.information("responseContent-->" + responseContent);
		return jsonResParser(responseContent);
	}
	catch (Poco::Exception &ex)
	{
		std::string mes = ex.message();
		logger.error("Poco::Exception -->" + mes);
	}
	catch (std::exception &ex)
	{
		std::string mes = ex.what();
		logger.error("std::exception -->" + mes);
		exit(1);
	}
	return 999;
}

void UtilTool::convertSecToTime(int sec,char *time)
{
	sprintf(time, "%.2d:%.2d:%.2d", sec / 3600, (sec - (sec / 3600 * 3600)) / 60, sec - (sec / 3600 * 3600) - (sec - (sec / 3600 * 3600)) / 60 * 60);
}

void UtilTool::convertSecToWTime(int sec, wchar_t *time,int size)
{
	//swprintf(wcs, 200, L"%.2d:%.2d:%.2d", media_instance->dot_start_time / 3600, (media_instance->dot_start_time - (tem_time / 3600 * 3600)) / 60, tem_time - (tem_time / 3600 * 3600) - (tem_time - (tem_time / 3600 * 3600)) / 60 * 60);
	swprintf(time, size, L"%.2d:%.2d:%.2d", sec / 3600, (sec - (sec / 3600 * 3600)) / 60, sec - (sec / 3600 * 3600) - (sec - (sec / 3600 * 3600)) / 60 * 60);
}

int16_t UtilTool::uint8_to_int16(uint8_t high, uint8_t low)
{
	/*uint8_t hi = 0xff;
	uint8_t lo = 0xff;*/
	int16_t  i = 0;
	i |= high << 8;
	i |= low;
	
	return i;
}