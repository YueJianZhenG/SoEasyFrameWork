#pragma once
#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#endif
#include<vector>
#include<sstream>
#include"mysql.h"
#include<memory>
#include<unordered_map>
#include<Thread/ThreadTaskAction.h>
using namespace std;
using namespace SoEasy;
namespace DataBase
{
	class MysqlManager;
	typedef  MYSQL_RES MysqlQueryResult;
	typedef MYSQL SayNoMysqlSocket;
}

namespace DataBase
{
	enum XMysqlCode
	{
		MysqlSuccessful,		//�ɹ�
		MysqlFailure,			//ʧ��
		MysqlNotInCoroutine,	//����Э����
		MysqlStartTaskFail,		//����ʧ��
		MysqlSocketIsNull,		//socket��
		MysqlSelectDbFailure,	//ѡ��dbʧ��
		MysqlInvokeFailure,		//ִ��sql���ʧ��
	};
}

namespace DataBase
{
	class MysqlQueryLine
	{
	public:
		bool AddMember(const std::string & key, const std::string & value);
	public:
		const int GetInt32Field(const char * key);
		const float GetFloat32Field(const char * key);
		const double GetFloat64Field(const char * key);
		const long long GetInt64Field(const char * key);
		const std::string & GetStringField(const char * key);
	private:
		std::unordered_map<std::string, std::string> mContent;
	};
}

namespace DataBase
{
	class MysqlQueryData
	{
	public:
		
		MysqlQueryData() { }
		~MysqlQueryData() { }
	public:
		friend class MysqlTaskAction;
		size_t GetRowCount() { return mFiledMap.size(); }	//��ȡ�ж��ٸ��ֶ�
		size_t GetColumnCount() { return mContentVector.size(); }	//��ȡ�ж�����
		XMysqlCode GetErrorCode() { return this->mErrorCode; }
		std::shared_ptr<MysqlQueryLine> GetLineData(size_t col = 0);	//��ȡһ������
		const std::string & GetErrorMessage() { return this->mErrorMessage; }	//��ȡ������Ϣ
	public:
		bool AddFieldName(const char * name, size_t len, int pos);
		void AddFieldContent(int row, const char * data, size_t len);
		void SetErrorCode(XMysqlCode code) { this->mErrorCode = code; }
		void SetErrorMessage(const char * msg) { this->mErrorMessage = msg; }
	private:
		XMysqlCode mErrorCode;
		std::string mErrorMessage;
		std::unordered_map<std::string, int> mFiledMap;
		std::vector<std::vector<std::string>> mContentVector;
	};
}