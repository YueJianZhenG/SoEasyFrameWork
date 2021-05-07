#include<Core/Applocation.h>
#include<Core/ManagerFactory.h>
#include<Manager/ScriptManager.h>
#include<Manager/TimerManager.h>
#include<Manager/ActionManager.h>
#include<Manager/AddressManager.h>
#include<Manager/NetWorkManager.h>
#include<Manager/CommandManager.h>
#include<Manager/DownLoadManager.h>
#include<Manager/ListenerManager.h>
#include<Manager/MysqlManager.h>
#include<Manager/RedisManager.h>
#include<Coroutine/CoroutineManager.h>

using namespace SoEasy;
using namespace DataBase;

TYPE_REFLECTION(SoEasy::TimerManager, "TimerManager");
TYPE_REFLECTION(SoEasy::ScriptManager, "ScriptManager");
TYPE_REFLECTION(SoEasy::ActionManager, "ActionManager");
TYPE_REFLECTION(SoEasy::AddressManager, "AddressManager");
TYPE_REFLECTION(SoEasy::NetWorkManager, "NetWorkManager");
TYPE_REFLECTION(SoEasy::CommandManager, "CommandManager");
TYPE_REFLECTION(SoEasy::DownLoadManager, "DownLoadManager");
TYPE_REFLECTION(SoEasy::ListenerManager, "ListenerManager");
TYPE_REFLECTION(SoEasy::CoroutineManager, "CoroutineManager");

TYPE_REFLECTION(DataBase::RedisManager, "RedisManager");
TYPE_REFLECTION(DataBase::MysqlManager, "MysqlManager");


#ifdef _WIN32
#pragma comment(lib,"lua53.lib")
#pragma comment(lib,"Common.lib")
#pragma comment(lib,"hiredis.lib")
#pragma comment(lib,"libmysql.lib")
#pragma comment(lib,"DataBase.lib")
#pragma comment(lib,"ServerData.lib")
#pragma comment(lib,"libprotobufd.lib")
#endif
int main(int argc, char ** argv)
{
	ManagerFactory factory;
	factory.RegisterManager<RedisManager>();
	factory.RegisterManager<MysqlManager>();
	factory.RegisterManager<TimerManager>();
	factory.RegisterManager<ScriptManager>();
	factory.RegisterManager<ActionManager>();
	factory.RegisterManager<AddressManager>();
	factory.RegisterManager<NetWorkManager>();
	factory.RegisterManager<CommandManager>();
	factory.RegisterManager<DownLoadManager>();
	factory.RegisterManager<ListenerManager>();
	factory.RegisterManager<CoroutineManager>();


	Applocation app("Server", factory, "./Config/SrvConfig.json");

	return app.Run();
}