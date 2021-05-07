#pragma once
#include"Manager.h"
#include<Other/AssestsFileInfo.h>
#include<Protocol/ServerCommon.pb.h>
using namespace PB;
namespace SoEasy
{
	class DownLoadManager : public Manager
	{
	public:
		DownLoadManager() { }
		~DownLoadManager() { }
	protected:
		bool OnInit() override;
	private:
		bool AsyncDownLoadAssest(shared_ptr<TcpClientSession> tcpSession, const std::string & assestName);
	};
}