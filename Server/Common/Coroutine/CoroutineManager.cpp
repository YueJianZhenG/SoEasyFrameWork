#include"CoroutineManager.h"
#include<chrono>
#include<memory.h>
#include"Coroutine.h"
#include"CoroutineEvent.h"
#include<Core/Applocation.h>
#include<Util/NumberHelper.h>
#include<Timer/CorSleepTimer.h>
#include<Manager/TimerManager.h>
#include<Manager/NetWorkManager.h>
#include<Manager/ActionManager.h>
#include<NetWork/NetWorkRetAction.h>
using namespace std::chrono;

namespace SoEasy
{
#ifdef _WIN32
#include<Windows.h>
	void __stdcall WinEntry(LPVOID manager)
#else
	void LinuxEntry(void * manager)
#endif
	{
		CoroutineManager * pCoroutineMgr = (CoroutineManager*)manager;
		if (pCoroutineMgr != nullptr)
		{
			Coroutine * pCoroutine = pCoroutineMgr->GetCoroutine();
			if (pCoroutine != nullptr)
			{
				pCoroutine->mBindFunc();
				pCoroutineMgr->Destory(pCoroutine->mCoroutineId);
			}
		}
	}

	CoroutineManager::CoroutineManager()
	{
#ifdef _WIN32
		this->mMainCoroutineStack = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
#endif
		this->mNetWorkManager = nullptr;
		this->mFunctionManager = nullptr;
	}

	bool CoroutineManager::OnInit()
	{		
		this->mTimerManager = this->GetManager<TimerManager>(); 
		this->mNetWorkManager = this->GetManager<NetWorkManager>();
		this->mFunctionManager = this->GetManager<ActionManager>();
		SayNoAssertRetFalse_F(this->mTimerManager);
		SayNoAssertRetFalse_F(this->mNetWorkManager);
		SayNoAssertRetFalse_F(this->mNetWorkManager);
		return true;
	}

	void CoroutineManager::OnInitComplete()
	{	
		
	}

	long long CoroutineManager::Start(CoroutineAction func)
	{
		long long id = this->Create(func);
		this->Resume(id);
		return id;
	}

	long long CoroutineManager::Create(CoroutineAction func)
	{
		long long id = NumberHelper::Create();
		Coroutine * pCoroutine = new Coroutine(id, func);
		this->mCoroutineMap.insert(std::make_pair(id, pCoroutine));
		return pCoroutine->mCoroutineId;
	}

#ifdef _WIN32
	void CoroutineManager::LoopCheckDelCoroutine()
	{
		while (!this->mDestoryCoroutine.empty())
		{
			Coroutine * pCoroutine = this->mDestoryCoroutine.front();
			this->mDestoryCoroutine.pop();
			delete pCoroutine;
		}
	}
#endif
	void CoroutineManager::LoopCoroutineEvent()
	{
		if (!this->mCorEventList.empty())
		{
			for (auto iter = this->mCorEventList.begin(); iter != this->mCorEventList.end();)
			{
				CoroutineEvent * corEvent = (*iter);
				long long id = corEvent->GetBindCoroutineID();
				if (this->GetCoroutine(id) != nullptr)
				{
					if (corEvent->Invoke() == false)
					{
						iter++;
						continue;
					}
				}
				this->mCorEventList.erase(iter++);
				this->Resume(corEvent->GetBindCoroutineID());
				delete corEvent;
			}
		}
		while (!this->mResumeCoroutine.empty())
		{
			long long id = this->mResumeCoroutine.front();
			this->mResumeCoroutine.pop();
			this->WakeUpCoroutine(id);
		}
	}


	void CoroutineManager::Sleep(long long ms)
	{
		Coroutine * pCoroutine = this->GetCoroutine();
		if (pCoroutine != nullptr)
		{
			const long long id = pCoroutine->mCoroutineId;
			this->mTimerManager->CreateTimer<CorSleepTimer>(this, id, ms);
			this->YieldReturn();
		}
	}

	void CoroutineManager::Resume(long long id)
	{
		Coroutine * pCoroutine = this->GetCoroutine(id);
		if (pCoroutine != nullptr)
		{
			this->mResumeCoroutine.push(id);
		}
	}

	void CoroutineManager::WakeUpCoroutine(long long id)
	{
		Coroutine * pCoroutine = this->GetCoroutine(id);
		if (pCoroutine != nullptr)
		{
			if (pCoroutine->mState == CorState::Ready)
			{
				this->mCurrentCorId = id;
#ifdef _WIN32
				pCoroutine->mContextStack = CreateFiber(STACK_SIZE, WinEntry, this);
				SwitchToFiber(pCoroutine->mContextStack);
#else
				getcontext(&pCoroutine->mCorContext);
				pCoroutine->mState = CorState::Running;
				pCoroutine->mCorContext.uc_stack.ss_size = STACK_SIZE;
				pCoroutine->mCorContext.uc_link = &this->mMainContext;
				pCoroutine->mCorContext.uc_stack.ss_sp = this->mSharedStack;
				makecontext(&pCoroutine->mCorContext, (void(*)(void))LinuxEntry, 1, this);
				swapcontext(&this->mMainContext, &pCoroutine->mCorContext);
#endif

			}
			else if (pCoroutine->mState == CorState::Suspend)
			{
				this->mCurrentCorId = id;
				pCoroutine->mState = CorState::Running;
#ifdef _WIN32
				SwitchToFiber(pCoroutine->mContextStack);
#else
				void * start = this->mSharedStack + STACK_SIZE - pCoroutine->mStackSize;
				memcpy(start, pCoroutine->mContextStack, pCoroutine->mStackSize);
				swapcontext(&this->mMainContext, &pCoroutine->mCorContext);
#endif
			}
		}
	}

	void CoroutineManager::YieldReturn()
	{
		Coroutine * pCoroutine = this->GetCoroutine();
		if (pCoroutine != nullptr)
		{
			this->mCurrentCorId = 0;
			pCoroutine->mState = CorState::Suspend;
#ifdef _WIN32
			SwitchToFiber(this->mMainCoroutineStack);
#else
			this->SaveStack(pCoroutine, this->mSharedStack + STACK_SIZE);
			swapcontext(&pCoroutine->mCorContext, &this->mMainContext);
#endif
		}
	}

	Coroutine * CoroutineManager::GetCoroutine()
	{
		auto iter = this->mCoroutineMap.find(this->mCurrentCorId);
		return iter != this->mCoroutineMap.end() ? iter->second : nullptr;
	}

	Coroutine * CoroutineManager::GetCoroutine(long long id)
	{
		auto iter = this->mCoroutineMap.find(id);
		return iter != this->mCoroutineMap.end() ? iter->second : nullptr;
	}

	void CoroutineManager::Destory(long long id)
	{
		auto iter = this->mCoroutineMap.find(id);
		if (iter != this->mCoroutineMap.end())
		{
			this->mCurrentCorId = 0;
			Coroutine * cor = iter->second;
			this->mCoroutineMap.erase(iter);
#ifdef _WIN32
			mDestoryCoroutine.push(cor);
			SwitchToFiber(this->mMainCoroutineStack);
#else
			delete cor;
			setcontext(&mMainContext);
#endif
		}
	}

	long long CoroutineManager::GetNowTime()
	{
		auto timeNow = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
		return timeNow.count();
	}

	void CoroutineManager::SaveStack(Coroutine * cor, char * top)
	{
		char dummy = 0;
		size_t size = top - &dummy;
		if (cor->mStackSize < size)
		{
			free(cor->mContextStack);
			cor->mContextStack = malloc(size);
		}
		cor->mStackSize = size;
		memcpy(cor->mContextStack, &dummy, cor->mStackSize);
	}

	XCode CoroutineManager::Call(shared_ptr<TcpClientSession> session, const std::string func, Message & returnData)
	{		
		NetWorkWaitCorAction * callBack = new NetWorkWaitCorAction(func, this);
		XCode code = this->SendCallMessage(session, func, nullptr, callBack);
		if (code == XCode::Successful)
		{
			this->YieldReturn();
			if (!returnData.ParseFromString(callBack->GetMsgData()))
			{
				return XCode::ParseMessageError;
			}
			return callBack->GetCode();
		}
		return XCode::Failure;
	}

	XCode CoroutineManager::Call(shared_ptr<TcpClientSession> session, const std::string func, const Message * message)
	{
		NetWorkWaitCorAction * callBack = new NetWorkWaitCorAction(func, this);
		XCode code = this->SendCallMessage(session, func, message, callBack);
		if (code == XCode::Successful)
		{
			this->YieldReturn();
			return callBack->GetCode();
		}
		return XCode::Failure;
	}

	XCode CoroutineManager::Call(shared_ptr<TcpClientSession> session, const std::string func, const Message * message, Message & returnData)
	{
		NetWorkWaitCorAction * callBack = new NetWorkWaitCorAction(func, this);
		XCode code = this->SendCallMessage(session, func, message, callBack);
		if (code == XCode::Successful)
		{
			this->YieldReturn();
			if (!returnData.ParseFromString(callBack->GetMsgData()))
			{
				return XCode::ParseMessageError;
			}
			return callBack->GetCode();
		}
		return XCode::Failure;
	}

	XCode CoroutineManager::SendCallMessage(shared_ptr<TcpClientSession> session, const std::string & func, const Message * message, NetWorkRetActionBox * callBack)
	{
		if (session == nullptr)
		{
			return XCode::SessionIsNull;
		}

		shared_ptr<NetWorkPacket> callData = make_shared<NetWorkPacket>();

		if (message != nullptr)
		{
			if (!message->SerializePartialToString(&mMessageBuffer))
			{
				return XCode::SerializationFailure;
			}
			callData->set_message_data(mMessageBuffer);
			callData->set_protoc_name(message->GetTypeName());
		}
		long long id = this->mFunctionManager->AddCallback(callBack);
		callData->set_func_name(func);
		callData->set_callback_id(id);

		const std::string & address = session->GetAddress();
		if (!this->mNetWorkManager->SendMessageByAdress(address, callData))
		{
			return XCode::SendMessageFail;
		}
		return XCode::Successful;
	}

	void CoroutineManager::OnFrameUpdate(float t)
	{
#ifdef _WIN32
		this->LoopCheckDelCoroutine();
#endif
		this->LoopCoroutineEvent();
	}
}