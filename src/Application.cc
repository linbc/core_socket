#include "Application.h"
#include <thread>

namespace core_socket
{

	namespace time
	{
		uint32_t now()
		{
#ifdef WIN32
			return GetTickCount();
#else
			static struct timeval __process__start_time ;	//程序启动时间

			uint32_t inteval ;
			struct timeval tmnow;	

			if(0==__process__start_time.tv_sec) {
				gettimeofday(&__process__start_time, NULL) ;		
			}
			gettimeofday(&tmnow, NULL) ;

			inteval = (tmnow.tv_sec - __process__start_time.tv_sec) * 1000 +  
				(tmnow.tv_usec - __process__start_time.tv_usec) / 1000 ;
			return inteval ;
#endif // WIN32
		}

		uint32_t diff(uint32_t prev,uint32_t curr)
		{
			// getMSTime() have limited data range and this is case when it overflow in this tick
			if (prev > curr)
				return (0xFFFFFFFF - prev) + curr;
			else
				return curr - prev;
		}

		void sleep(uint32_t ms)
		{
			std::chrono::milliseconds dura(ms);
			std::this_thread::sleep_for(dura);
		}
	}

//////////////////////////////////////////////////////////////////////////
//for config
//Config::Config()
//{	
//	port = 0;
//}

//////////////////////////////////////////////////////////////////////////
//for Application
Application::Application(Config &conf):conf_(conf)
{

}

Application::~Application()
{

}

bool Application::Open()
{
	if(conf_.host.empty())
		return false;
	if(conf_.port == 0)
		return false;
	
	//主动
	return connMgr_.Connection(conf_.host,conf_.port,new_connection_);
}

void Application::Close()
{
	connMgr_.Close();
}

void Application::Update(uint32_t diff)
{
	//网络跳跳
	connMgr_.Select(0);
}

void Application::RunLoop()
{
	//主心跳开始了
	uint32_t realCurrTime = 0;
	uint32_t realPrevTime = time::now();

	uint32_t diff = 0;
	uint32_t prevSleepTime = 0;                               // used for balanced full tick time length near WORLD_SLEEP_CONST

	while(running_){
		realCurrTime = time::now();
		diff = time::diff(realPrevTime, realCurrTime);

		++tick_counter_;
		Update(diff);

		realPrevTime = realCurrTime;

		// diff (D0) include time of previous sleep (d0) + tick time (t0)
		// we want that next d1 + t1 == WORLD_SLEEP_CONST
		// we can't know next t1 and then can use (t0 + d1) == WORLD_SLEEP_CONST requirement
		// d1 = WORLD_SLEEP_CONST - t0 = WORLD_SLEEP_CONST - (D0 - d0) = WORLD_SLEEP_CONST + d0 - D0
		if (diff <= WORLD_SLEEP_CONST + prevSleepTime)
		{
			prevSleepTime = WORLD_SLEEP_CONST + prevSleepTime - diff;
			time::sleep(prevSleepTime);					
		}
		else
			prevSleepTime = 0;
	}
}

void Application::Log(const string& s)
{
	std::cout << s << std::endl;
}

}
