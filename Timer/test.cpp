#include "timer.hpp"
#include <iostream>
#include <memory>
#include <sys/epoll.h>

int main() {
    int epfd = epoll_create(1); //创建epoll

    std::unique_ptr<Timer> timer = std::make_unique<Timer>();
    int i = 0;

    //测试添加
    timer->AddTimer(1000, [&i](const TimerNode& node) {
        std::cout << "now time:" << Timer::GetTick() << "\n\t";
        std::cout << "node id=" << node.id << " revoked times:" << ++i << std::endl;
        });
    timer->AddTimer(1000, [&i](const TimerNode& node) {
        std::cout << "now time:" << Timer::GetTick() << "\n\t";
        std::cout << "node id=" << node.id << " revoked times:" << ++i << std::endl;
        });
    timer->AddTimer(3000, [&i](const TimerNode& node) {
        std::cout << "now time:" << Timer::GetTick() << "\n\t";
        std::cout << "node id=" << node.id << " revoked times:" << ++i << std::endl;
        });

    //测试删除
    auto node = timer->AddTimer(2000, [&i](const TimerNode& node) {
        std::cout << "now time:" << Timer::GetTick() << "\n\t";
        std::cout << "node id=" << node.id << " revoked times:" << ++i << std::endl;
        });
    timer->DelTimer(node);

    std::cout << "now time:" << Timer::GetTick() << std::endl;

    //开始从网络中去取，就绪的事件
    epoll_event ev[64] = { 0 }; //准备一个静态的数组：用来接收网络中就绪的事件
    while (true) { //通过一个死循环不断地从网络中获取事件
        //阻塞式接收（-1：一直阻塞；0：非阻塞；>0：阻塞等待的毫秒级时间）
        // timeout 最近触发的定时任务离当前的时间
        int n = epoll_wait(epfd, ev, 64, timer->TimeToSleep());
        for (int i = 0; i < n; i++) {
            /*TODO: handle net event*/
        }
        /*TODO: handle timer event*/
        while (timer->CheckTimer()) continue;
    }
}