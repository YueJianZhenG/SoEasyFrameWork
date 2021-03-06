#pragma once
#include<memory>
#include<Manager/Manager.h>
#include<Protocol/Common.pb.h>
using namespace std;
using namespace PB;
namespace SoEasy
{
	class NetWorkWaitCorAction;
	class ServiceBase : public Object
	{
	public:
		ServiceBase();
		virtual ~ServiceBase() { }
	public:
		virtual bool OnInit();
		virtual void OnInitComplete() { };
		virtual void OnSystemUpdate();
		virtual void OnConnectDone(SharedTcpSession tcpSession) { }
	public:	
		void PushHandleMessage(SharedPacket argv);
		void PushHandleMessage(const std::string & address, SharedPacket argv);
	public:
		bool IsInit() { return this->mIsInit; }
		const int GetServiceId() { return this->mServiceId; }
		virtual bool HasMethod(const std::string & method) = 0;
		const std::string & GetServiceName() { return this->mServiceName; };
 	protected:
		bool ReplyMessage(const long long cb, shared_ptr<NetWorkPacket> msg);
		bool ReplyMessage(const std::string & address, shared_ptr<NetWorkPacket> msg);
		virtual bool InvokeMethod(const std::string & method, shared_ptr<NetWorkPacket>) = 0;
		virtual bool InvokeMethod(const std::string & address, const std::string & method, SharedPacket packet) = 0;
	public:
		void Sleep(long long ms);
		void Start(const std::string & name, std::function<void()> && func);
	public:
		void InitService(const std::string & name, int serviceId);
	public:
		XCode Call(const std::string method, Message & returnData);
		XCode Call(const std::string method, shared_ptr<Message> message = nullptr);	
		XCode Call(const std::string method, shared_ptr<Message> message, Message & returnData);
	public:
		XCode Notice(const std::string method, shared_ptr<Message> message = nullptr);
	private:
		shared_ptr<NetWorkPacket> CreatePacket(const std::string & func, shared_ptr<Message> message, shared_ptr<NetWorkWaitCorAction> callBack);
	private:
		bool mIsInit;
		int mServiceId;
		std::string mServiceName;
		class NetWorkManager * mNetManager;
		class ActionManager * mActionManager;
		class CoroutineManager * mCorManager;
		DoubleBufferQueue<SharedPacket> mLocalHandleMsgQueue;
		DoubleBufferQueue<SharedNetPacket> mProxyHandleMsgQueue;
	};
}