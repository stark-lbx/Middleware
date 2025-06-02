#include "timer.hpp"

int64_t Timer::gid = 0;

int64_t Timer::GenID() { return ++gid; }

time_t Timer::GetTick() {
	using namespace std::chrono;
	/*
	  ��ȡ��ǰʱ�䣺steady_clock::now()
	  ��ʱ���ת��Ϊ����ʱ�䣺time_point_cast<milliseconds>();
	*/
	auto sc = time_point_cast<milliseconds>(steady_clock::now());
	/*
	  ��ȡʱ������sc.time_since_epoch();
	  �������ʱ���ʾΪ���룺duration_cast<milliseconds>();
	*/
	auto tmp = duration_cast<milliseconds>(sc.time_since_epoch());

	return tmp.count();
}

TimerNodeBase Timer::AddTimer(time_t msec, TimerNode::Callback func) {
	TimerNode tnode;
	tnode.expire = GetTick() + msec;
	tnode.func = func;
	tnode.id = GenID();
	//��ĳ�����ݽṹ��
	timermap.insert(tnode);
	return static_cast<TimerNodeBase>(tnode);
}

bool Timer::DelTimer(TimerNodeBase& node) {
	//��ĳ�����ݽṹ��ɾ��
	auto iter = timermap.find(node);
	if (iter != timermap.end()) {
		timermap.erase(iter);
		return true;
	}
	return false;
}

bool Timer::CheckTimer() {
	//���ҵ�ĳ�����ݽṹ��С�Ľڵ�
	//����ǰʱ����бȽ�
	auto iter = timermap.begin();
	if (iter != timermap.end() && iter->expire <= GetTick()) {
		//û���κνڵ�
		iter->func(*iter);
		timermap.erase(iter);
		return true;
	}
	return false;
}

time_t Timer::TimeToSleep() {
	auto iter = timermap.begin();
	if (iter == timermap.end()) {
		//û���κνڵ�
		return -1;
	}
	time_t diss = iter->expire - GetTick();
	return diss > 0 ? diss : 0;
}