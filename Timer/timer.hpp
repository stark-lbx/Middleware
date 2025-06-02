#ifndef __MYTIMER__H_
#define __MYTIMER__H_

#include <chrono>
#include <functional>
#include <set>

struct TimerNodeBase {
    time_t expire; //����ʱ��
    int64_t id;
};
// C++14�����ԣ�find()����Ĳ������Բ���<T>�е�T��������T����Ч�Ƚ��ֶ�
struct TimerNode : public TimerNodeBase {
    using Callback = std::function<void(const TimerNode& node)>;
    Callback func; //�ص�����
};
bool operator<(const TimerNodeBase& lhd, const TimerNodeBase& rhd) {
    if (lhd.expire == rhd.expire)
        return lhd.id < rhd.id;
    return lhd.expire < rhd.expire;
}

class Timer {
public:
    static int64_t GenID() { return ++gid; }

    static time_t GetTick();

    //���ʱ����ڡ�ִ��ʲô����
    TimerNodeBase AddTimer(time_t msec, TimerNode::Callback func);
    
    bool DelTimer(TimerNodeBase& node);
    bool CheckTimer();

    //����������¼�
    time_t TimeToSleep();
private:
    static int64_t gid;
    std::set<TimerNode, std::less<>> timermap;
};
#endif //__MYTIMER__H_