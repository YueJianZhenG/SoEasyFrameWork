#include"SessionManager.h"
#include"NetWorkManager.h"
#include"LocalActionManager.h"
#include<Core/Applocation.h>
#include<Util/StringHelper.h>
#include<NetWork/NetLuaAction.h>
#include<NetWork/NetWorkRetAction.h>
#include<Coroutine/CoroutineManager.h>
namespace SoEasy
{
	shared_ptr<TcpClientSession> SessionManager::CreateTcpSession(SharedTcpSocket socket)
	{
		shared_ptr<TcpClientSession> tcpSession = std::make_shared<TcpClientSession>(this, socket);
		if (tcpSession != nullptr)
		{
			this->mNewSessionQueue.AddItem(tcpSession);
			return tcpSession;
		}
		return nullptr;
	}

	shared_ptr<TcpClientSession> SessionManager::CreateTcpSession(std::string name, std::string address)
	{
		std::string connectIp;
		unsigned short connectPort;
		if (!StringHelper::ParseIpAddress(address, connectIp, connectPort))
		{
			SayNoDebugError("parse " << address << " fail")
			return false;
		}
		return this->CreateTcpSession(name, connectIp, connectPort);
	}

	shared_ptr<TcpClientSession> SessionManager::CreateTcpSession(std::string name, std::string ip, unsigned short port)
	{
		shared_ptr<TcpClientSession> tcpSession = std::make_shared<TcpClientSession>(this, name, ip, port);
		if (tcpSession != nullptr)
		{
			tcpSession->StartConnect();
		}
		return tcpSession;
	}

	bool SessionManager::AddNewSession(shared_ptr<TcpClientSession> tcpSession)
	{
		if (!tcpSession)
		{
			return false;
		}
		if (!tcpSession->IsActive())
		{
			SayNoDebugError(tcpSession->GetAddress() << " is error add fail");
			return false;
		}
		this->mNewSessionQueue.AddItem(tcpSession);
		return true;
	}

	bool SessionManager::AddErrorSession(shared_ptr<TcpClientSession> tcpSession)
	{
		if (!tcpSession)
		{
			return false;
		}
		if (tcpSession->IsActive())
		{
			tcpSession->CloseSocket();
			return false;
		}
		this->mErrorSessionQueue.AddItem(tcpSession);
		return true;
	}

	void SessionManager::AddRecvMessage(shared_ptr<TcpClientSession> session, const char * message, size_t size)
	{
		if (session != nullptr)
		{
			const std::string & address = session->GetAddress();
			SharedNetPacket sharedPacket = std::make_shared<NetMessageBuffer>(address, message, size);
			mRecvMessageQueue.AddItem(sharedPacket);
		}
	}

	bool SessionManager::OnInit()
	{
		this->mActionManager = this->GetManager<LocalActionManager>();
		this->mNetWorkManager = this->GetManager<NetWorkManager>();
		this->mCoroutineSheduler = this->GetManager<CoroutineManager>();
		SayNoAssertRetFalse_F(this->mActionManager);
		SayNoAssertRetFalse_F(this->mNetWorkManager);
		SayNoAssertRetFalse_F(this->mCoroutineSheduler);
		return true;
	}

	void SessionManager::OnRecvNewMessageAfter(const std::string & address, const char * msg, size_t size)
	{
		this->mCurrentSession = this->mNetWorkManager->GetTcpSession(address);
		SayNoAssertRet(this->mCurrentSession, "not find session : " << address << " size = " << size);

		shared_ptr<NetWorkPacket> nNetMsgPackage = make_shared<NetWorkPacket>();
		SayNoAssertRet_F(nNetMsgPackage->ParseFromArray(msg, size));

		if (this->mRecvMsgCallback != nullptr)
		{
			this->mRecvMsgCallback(this->mCurrentSession, nNetMsgPackage);
			return;
		}
	
		if (!nNetMsgPackage->func_name().empty())
		{
			const std::string & name = nNetMsgPackage->func_name();
	
			shared_ptr<NetLuaAction> luaAction = this->mActionManager->GetLuaAction(name);
			if (luaAction != nullptr)	//lua 函数自己返回
			{			
				XCode code = luaAction->Invoke(address, nNetMsgPackage);
				if (code != XCode::LuaCoroutineReturn)
				{
					shared_ptr<NetWorkPacket> returnPacket = make_shared<NetWorkPacket>();
					returnPacket->set_error_code(code);
					returnPacket->set_operator_id(nNetMsgPackage->operator_id());
					returnPacket->set_callback_id(nNetMsgPackage->callback_id());
					this->mNetWorkManager->SendMessageByAdress(address, returnPacket);
				}
			}
			else  //在协程中执行
			{
				this->mCoroutineSheduler->Start([this, nNetMsgPackage, address]()
				{
					shared_ptr<NetWorkPacket> returnPacket = make_shared<NetWorkPacket>();
					XCode code = this->InvokeAction(this->mCurrentSession, nNetMsgPackage, returnPacket);
					if (nNetMsgPackage->callback_id() != 0)
					{
						returnPacket->set_error_code(code);
						returnPacket->set_operator_id(nNetMsgPackage->operator_id());
						returnPacket->set_callback_id(nNetMsgPackage->callback_id());
						this->mNetWorkManager->SendMessageByAdress(address, returnPacket);
					}
				});
			}
		}
		else if (nNetMsgPackage->callback_id() != 0)
		{
			const long long id = nNetMsgPackage->callback_id();
			auto callback = this->mActionManager->GetCallback(id);
			if (callback == nullptr)
			{
				SayNoDebugError("not find call back " << id);
				return;
			}
			callback->Invoke(this->mCurrentSession, nNetMsgPackage);
		}
		this->mCurrentSession = nullptr;
	}

	XCode SessionManager::InvokeAction(shared_ptr<TcpClientSession> tcpSession, shared_ptr<NetWorkPacket> callInfo, shared_ptr<NetWorkPacket> returnData)
	{
		const std::string & name = callInfo->func_name();
		shared_ptr<LocalActionProxy> localActionProxy = this->mActionManager->GetAction(name);
		if (localActionProxy == nullptr)
		{
			SayNoDebugError(name << " does not exist");
			return XCode::CallFunctionNotExist;
		}	
		return localActionProxy->Invoke(callInfo, returnData);
	}

	long long SessionManager::GetIdByAddress(const std::string & address)
	{
		return 0;
	}

	bool SessionManager::ParseAddress(const std::string & address, string & ip, unsigned short & port)
	{
		size_t pos = address.find(":");
		if (pos == std::string::npos)
		{
			return false;
		}
		ip = address.substr(0, pos);
		port = (unsigned short)std::stoul(address.substr(pos + 1));
		return true;
	}

	void SessionManager::OnSystemUpdate()
	{
		shared_ptr<TcpClientSession> pTcpSession = nullptr;
		this->mNewSessionQueue.SwapQueueData();
		this->mErrorSessionQueue.SwapQueueData();

		while (this->mErrorSessionQueue.PopItem(pTcpSession))
		{
			if (!this->mNetWorkManager->RemoveTcpSession(pTcpSession))
			{
				SayNoDebugError("remove tcp session error : " << pTcpSession->GetAddress());
				continue;
			}
			this->OnSessionErrorAfter(pTcpSession);
			pTcpSession = nullptr;
		}

		while (this->mNewSessionQueue.PopItem(pTcpSession))
		{
			if (!this->mNetWorkManager->AddTcpSession(pTcpSession))
			{
				SayNoDebugError("add tcp session error : " << pTcpSession->GetAddress());
				continue;
			}
			this->OnSessionConnectAfter(pTcpSession);
			if (pTcpSession->IsContent())
			{
				pTcpSession->InvokeConnectCallback();
			}
			pTcpSession = nullptr;
		}

		SharedNetPacket pMessagePacket;
		this->mRecvMessageQueue.SwapQueueData();
		while (this->mRecvMessageQueue.PopItem(pMessagePacket))
		{
			const std::string & address = pMessagePacket->mAddress;
			const std::string & message = pMessagePacket->mCommandMsg;
			this->OnRecvNewMessageAfter(address, message.c_str(), message.size());
		}
	}
}