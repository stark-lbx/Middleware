#ifndef __MYTIMER__H_
#define __MYTIMER__H_

#include <chrono>
#include <functional>
#include <set>

struct TimerNodeBase {
    time_t expire; //过期时间
    int64_t id;
};
// C++14新特性：find()传入的参数可以不是<T>中的T，可以是T中有效比较字段
struct TimerNode : public TimerNodeBase {
    using Callback = std::function<void(const TimerNode& node)>;
    Callback func; //回调函数
};

class Timer {
public:
    static int64_t GenID();

    static time_t GetTick();

    //多久时间过期、执行什么方法
    TimerNodeBase AddTimer(time_t msec, TimerNode::Callback func);
    
    bool DelTimer(TimerNodeBase& node);
    bool CheckTimer();

    //最近触发的事件
    time_t TimeToSleep();
private:
    static int64_t gid;
    std::set<TimerNode, std::less<>> timermap;
};
#endif //__MYTIMER__H_
