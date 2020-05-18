#ifndef _TIMTER_MANAGER_H_
#define _TIMTER_MANAGER_H_

#include <chrono>
#include <ctime>
#include <functional>
#include <map>
#include <assert.h>
#include "ObjArray.hpp"

/*
高精度定时器
精确单位:毫秒
*/
class TimerManager
{
	struct timer_data;
	typedef std::function<size_t()> callback_t;
	typedef long long clock_run_t;
	typedef ObjArray<timer_data> timer_manager_t;
	typedef std::multimap<clock_run_t, size_t> sort_timer_t;

	struct timer_data {
		enum { init = 0x0, running = 0x1, closing = 0x2 };	//state
		size_t		id;			//定时器id
		clock_run_t	clock_run;	//执行时间
		callback_t	cb;			//回调函数
		uint8_t		state;
	};

	sort_timer_t	sort_timers_;
	timer_manager_t	timers_;

public:
	//时间点
	struct Point {
		int year;
		int month;
		int day;
		int	hour;	//小时,0-23
		int min;	//分,0-59
		int sec;	//秒,0-59
		int mssec;	//毫秒,0-999
	};

public:
	static clock_run_t Now()
	{

		return std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();

	}

	static clock_run_t ToSystemTime(const Point& point)
	{
		std::tm t;
		t.tm_year = point.year - 1900;
		t.tm_mon = point.month - 1;
		t.tm_mday = point.day;
		t.tm_hour = point.hour;
		t.tm_min = point.min;
		t.tm_sec = point.sec;
		t.tm_isdst = 0;
		t.tm_wday = 0;
		t.tm_yday = 0;
		std::time_t time_sec = std::mktime(&t);
		clock_run_t time_msec = time_sec * 1000 + point.mssec;
		return time_msec;
	}

    static TimerManager& Instance()
	{
        static TimerManager t;
		return t;
	}

	/*
	注册定时器
	clock_run	:	系统时间(毫秒)
	callback	:	回调函数
	*/
	size_t Regist(clock_run_t clock_run, const callback_t& callback)
	{

		size_t id;
		auto t = timers_.NewVal(id);
		if (t)
		{

			assert(id != timer_manager_t::INVALID_INDEX);
			t->cb = callback;
			t->id = id;
			t->clock_run = clock_run;
			t->state = timer_data::init;

			if (!sort_timers_.insert(std::make_pair(t->clock_run, id))->first)
			{
				timers_.DelVal(id);
				return timer_manager_t::INVALID_INDEX;
			}

			return id;

		}

		return timer_manager_t::INVALID_INDEX;

	}

	/*
	延迟执行
	eclapse	:	超时时间(毫秒)
	callback:	回调函数
	*/
	template<typename T>
	size_t DelayRun(size_t eclapse, const T& callback)
	{
		auto clock_run = eclapse + Now();
		return Regist(clock_run, [=]() {callback(); return 0; });
	}

	/*
	在某个时间点执行
	point	: 时间点
	callback: 回调函数
	*/
	template<typename T>
	size_t RunAt(const Point& point, const T& callback)
	{
		auto clock_run = ToSystemTime(point);
		return Regist(clock_run, [=]() {callback(); return 0; });
	}

	/*
	重复执行
	eclapse	: 重复周期(毫秒)
	callback: 回调函数
	*/
	template<typename T>
	size_t RepeatRun(size_t eclapse, const T& callback)
	{
		auto clock_run = eclapse + Now();
		return Regist(clock_run, [=]() {callback(); return eclapse; });
	}

	template<typename T>
	size_t RunEveryDay(const Point& point, const T& callback)
	{
		
		clock_run_t cycle = 24 * 3600 * 1000;
		auto timestamp = ToSystemTime(point);
		auto now = Now();
		auto eclapse = (timestamp - now) % cycle;
		if (eclapse < 0) eclapse += cycle;
		auto clock_run = now + eclapse;
		return Regist(clock_run, [=]() {callback(); return cycle; });
	}

	template<typename T>
	size_t RunEveryWeek(const Point& point, const T& callback)
	{

		clock_run_t cycle = 7 * 24 * 3600 * 1000;
		auto timestamp = ToSystemTime(point);
		auto now = Now();
		auto eclapse = (timestamp - now) % cycle;
		if (eclapse < 0) eclapse += cycle;
		auto clock_run = now + eclapse;
		return Regist(clock_run, [=]() {callback(); return cycle; });
	}

	//注销定时器
	void UnRegist(size_t timerid)
	{

		auto t = timers_.GetVal(timerid);
		if (t) {

			auto it = sort_timers_.find(t->clock_run);
			bool find = false;

			for (; it != sort_timers_.end(); ++it) {

				if (it->second == timerid) {
					find = true;
					break;
				}

			}

			assert(find);
			if (find) {

				if (t->state & timer_data::running)
					t->state |= timer_data::closing;
				else {
					sort_timers_.erase(it);
					timers_.DelVal(timerid);
				}

			}

		}

	}

	//更新定时器
    void Update(clock_run_t now = TimerManager::Now())
	{

		for (auto it = sort_timers_.begin();
			it != sort_timers_.end();) {

			if (it->first > now) break;	//时间未到

			auto id = it->second;
			auto t = timers_.GetVal(id);

			if (!t) {

				assert(false);
				it = sort_timers_.erase(it);
				continue;

			}

			t->state |= timer_data::running;
			size_t next_eclapse = t->cb();
			t->state &= ~timer_data::running;
			if ((t->state & timer_data::closing) || (0 == next_eclapse)) {

				it = sort_timers_.erase(it);
				timers_.DelVal(id);
				continue;

			}

			it = sort_timers_.erase(it);
			t->clock_run = now + next_eclapse;
			sort_timers_.insert(std::make_pair(t->clock_run, id));

		}

	}

};

#endif
