#include "timer.hpp"

bool operator<(const TimerNodeBase& lhd, const TimerNodeBase& rhd) {
    if (lhd.expire == rhd.expire)
        return lhd.id < rhd.id;
    return lhd.expire < rhd.expire;
}

int64_t Timer::gid = 0;

int64_t Timer::GenID() { return ++gid; }

time_t Timer::GetTick() {
	using namespace std::chrono;
	/*
	  获取当前时间：steady_clock::now()
	  将时间点转换为毫秒时间：time_point_cast<milliseconds>();
	*/
	auto sc = time_point_cast<milliseconds>(steady_clock::now());
	/*
	  求取时间间隔：sc.time_since_epoch();
	  将间隔的时间表示为毫秒：duration_cast<milliseconds>();
	*/
	auto tmp = duration_cast<milliseconds>(sc.time_since_epoch());

	return tmp.count();
}

TimerNodeBase Timer::AddTimer(time_t msec, TimerNode::Callback func) {
	TimerNode tnode;
	tnode.expire = GetTick() + msec;
	tnode.func = func;
	tnode.id = GenID();
	//加某个数据结构中
	timermap.insert(tnode);
	return static_cast<TimerNodeBase>(tnode);
}

bool Timer::DelTimer(TimerNodeBase& node) {
	//从某个数据结构中删除
	auto iter = timermap.find(node);
	if (iter != timermap.end()) {
		timermap.erase(iter);
		return true;
	}
	return false;
}

bool Timer::CheckTimer() {
	//先找到某个数据结构最小的节点
	//跟当前时间进行比较
	auto iter = timermap.begin();
	if (iter != timermap.end() && iter->expire <= GetTick()) {
		//没有任何节点
		iter->func(*iter);
		timermap.erase(iter);
		return true;
	}
	return false;
}

time_t Timer::TimeToSleep() {
	auto iter = timermap.begin();
	if (iter == timermap.end()) {
		//没有任何节点
		return -1;
	}
	time_t diss = iter->expire - GetTick();
	return diss > 0 ? diss : 0;
}
