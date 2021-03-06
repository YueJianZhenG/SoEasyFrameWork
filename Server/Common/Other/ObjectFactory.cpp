#include"ObjectFactory.h"
#include<Define/CommonDef.h>

namespace SoEasy
{
	ObjectFactory * ObjectFactory::Get()
	{
		static ObjectFactory factory(10);
		return &factory;
	}

	Message * ObjectFactory::CreateMessage(const std::string & name)
	{
		if (name.empty())
		{
			return nullptr;
		}
		auto iter = mMessageMap.find(name);
		if (iter != mMessageMap.end())
		{
			Message * pMessage = iter->second;
			pMessage->Clear();
			return pMessage;
		}
		const DescriptorPool * pDescriptorPool = DescriptorPool::generated_pool();
		const Descriptor * pDescriptor = pDescriptorPool->FindMessageTypeByName(name);
		if (pDescriptor != nullptr)
		{
			MessageFactory * factory = MessageFactory::generated_factory();
			Message * pMessage = const_cast<Message *>(factory->GetPrototype(pDescriptor));
			mMessageMap.insert(std::make_pair(name, pMessage));
			return pMessage;
		}
		return nullptr;
	}
}
