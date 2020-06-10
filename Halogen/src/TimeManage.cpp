#include "TimeManage.h"

std::atomic<bool> KeepSearching;

Timer::Timer() : Begin(clock()), End(clock())
{
	ElapsedTime = 0;
}

Timer::~Timer()
{
}

void Timer::Start()
{
	Begin = clock();
	End = clock();
}

void Timer::Restart()
{
	Begin = clock();
	End = clock();
	ElapsedTime = 0;
}

int Timer::ElapsedMs()
{
	End = clock();
	ElapsedTime = (double(End) - double(Begin)) / CLOCKS_PER_SEC * 1000;

	//std::cout << ElapsedTime << std::endl;
	return ElapsedTime;
}

SearchTimeManage::SearchTimeManage() : timer(Timer())
{
	AllowedSearchTimeMS = 0;
}

SearchTimeManage::~SearchTimeManage()
{
}

bool SearchTimeManage::ContinueSearch()
{
	return (timer.ElapsedMs() < AllowedSearchTimeMS / 2);
}

bool SearchTimeManage::AbortSearch(uint64_t nodes)
{
	if ((nodes & 0x3FF) == 0 || nodes <= 0x3FFF)	//should hit once every 1024 times. & is quicker than (nodes % 1024). EDIT: will hit once every 1024, but every time its called initially to help with very fast time controls
		CacheShouldStop = timer.ElapsedMs() > AllowedSearchTimeMS;

	return (!KeepSearching || CacheShouldStop);
}

void SearchTimeManage::StartSearch(int ms)
{
	timer.Restart();
	AllowedSearchTimeMS = ms;
	timer.Start();
}