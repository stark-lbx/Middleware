#include "timer.hpp"
#include <iostream>
#include <memory>
#include <sys/epoll.h>

int main() {
    int epfd = epoll_create(1); //����epoll

    std::unique_ptr<Timer> timer = std::make_unique<Timer>();
    int i = 0;

    //�������
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

    //����ɾ��
    auto node = timer->AddTimer(2000, [&i](const TimerNode& node) {
        std::cout << "now time:" << Timer::GetTick() << "\n\t";
        std::cout << "node id=" << node.id << " revoked times:" << ++i << std::endl;
        });
    timer->DelTimer(node);

    std::cout << "now time:" << Timer::GetTick() << std::endl;

    //��ʼ��������ȥȡ���������¼�
    epoll_event ev[64] = { 0 }; //׼��һ����̬�����飺�������������о������¼�
    while (true) { //ͨ��һ����ѭ�����ϵش������л�ȡ�¼�
        //����ʽ���գ�-1��һֱ������0����������>0�������ȴ��ĺ��뼶ʱ�䣩
        // timeout ��������Ķ�ʱ�����뵱ǰ��ʱ��
        int n = epoll_wait(epfd, ev, 64, timer->TimeToSleep());
        for (int i = 0; i < n; i++) {
            /*TODO: handle net event*/
        }
        /*TODO: handle timer event*/
        while (timer->CheckTimer()) continue;
    }
}