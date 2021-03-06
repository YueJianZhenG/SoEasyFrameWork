#pragma once
#include<string>
#include<XCode/XCode.h>
#include<Object/Object.h>
#include<Manager/ConsoleManager.h>
#include<NetWork/TelnetClientSession.h>
namespace SoEasy
{
	class CommandBase
	{
	public:
		CommandBase(ConsoleManager * commandMgr);
		virtual void Invoke(SharedTelnetSession session, const std::string paramate = "") = 0;
	protected:
		ConsoleManager * mCommandManager;
	};

	class StateCommand : public CommandBase
	{
	public:
		using CommandBase::CommandBase;
		void Invoke(SharedTelnetSession session, const std::string paramate) override;
	};

	class ServiceListCommand : public CommandBase
	{
	public:
		using CommandBase::CommandBase;
		void Invoke(SharedTelnetSession session, const std::string paramate) override;
	};

	class ServiceCallCommand :public CommandBase
	{
	public:
		using CommandBase::CommandBase;
		void Invoke(SharedTelnetSession session, const std::string paramate) override;
	};
}