#include"Coroutine.h"
#include<memory.h>
#include"CoroutineManager.h"
#ifdef _WIN32
#include<Windows.h>
#else
#include<ucontext.h>
#endif
namespace SoEasy
{
	Coroutine::~Coroutine()
	{
		if (this->mContextStack)
		{
#ifdef _WIN32
			DeleteFiber(this->mContextStack);
#else
			free(this->mContextStack);
#endif
			this->mContextStack = nullptr;
		}
	}



}