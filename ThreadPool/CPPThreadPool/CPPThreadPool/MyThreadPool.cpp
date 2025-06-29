#include "MyThreadPool.h"
//#include <iostream>

void MyThreadPool::submit(Task& task) {
  if (closed.load()) {
    throw std::runtime_error("submit to closed ThreadPool");
  }
  tasks.offer(task);
}

void MyThreadPool::submit(void(*callback)(void*), void* arg) {
  if (closed.load()) {
    throw std::runtime_error("submit to closed ThreadPool");
  }
  tasks.offer(Task{ callback,arg });
}

int MyThreadPool::get_total_size() const {
  if (closed.load()) {
    throw std::runtime_error("get total size of threads from closed ThreadPool");
  }
  std::lock_guard<std::mutex> lock(this->mutex);
  return totalthreadsize();
}

int MyThreadPool::get_alive_size() const {
  if (closed.load()) {
    throw std::runtime_error("get alive size of threads from closed ThreadPool");
  }
  std::lock_guard<std::mutex> lock(this->mutex);
  return alivethreadsize();
}

void MyThreadPool::start() {
  std::lock_guard<std::mutex> lock(this->mutex);

  //�������߳�ִ��while-loopһֱ����
  for (auto& fix_worker : fixed_workers) {
    fix_worker = std::move(std::thread([this]() {
      while (!closed.load()) {
        auto task = this->tasks.take();
        if (closed.load() || task.func == nullptr)break;
        busyNum++;
        task.func(task.args);
        busyNum--;
      }
    }));
  }

  //�������̶߳�̬�����߳�
  manager = std::move(std::thread([this]() {
    //����߳�ʱ���ù̶����Ӹ���
    const int INCREMENT = 2;
    const int DECREMENT = 2;
    while (!closed.load()) {
      std::this_thread::sleep_for(std::chrono::seconds(3));//����
      std::lock_guard<std::mutex> lock(mutex);

      //����߳�
      //������� ���� ����̸߳��� ���� �����߳� С�� �����߳�����
      int aliveNum = alivethreadsize();
      if (tasks.size() > aliveNum && aliveNum < maxsize) {
        for (int i = 0; i < INCREMENT; i++) {
          dynamic_workers.emplace_back([this]() {
            while (!closed.load()) {
              auto task = this->tasks.take();
              busyNum++;
              task.func(task.args);
              busyNum--;
            }
          });
        }
      }

      //�����߳�
      //æ���߳�*2 < �����߳� and �����߳���������С���߳���
      if (busyNum * 2 < aliveNum && aliveNum > maxsize) {
        tasks.set_cancel(DECREMENT);
      }
    }
  }));
  manager.detach();//���������̹߳ҵ���̨
}

void MyThreadPool::stop() {
  closed.store(true);
  //�ر���Ӳ���
  tasks.close();
}

MyThreadPool::MyThreadPool(int qsize, int minsize, int maxsize)
  :tasks(qsize), minsize(minsize), maxsize(maxsize),
  closed(false), busyNum(0) {
  fixed_workers = std::vector<std::thread>(minsize);
}

MyThreadPool::~MyThreadPool() {
  this->stop();
  // clear��֪ͨ���������ߺ������߽�����
  // ������ ��������ǹرյ� ��ֱ�Ӵ����ں�������
  tasks.clear();

  if (manager.joinable())manager.join();
  //std::cerr << "manager join over\n";

  //��֤�̶��ĺ��Ĺ����̶߳�������Դ
  for (auto& worker : fixed_workers) {
    if (worker.joinable())  worker.join();
  }
  //std::cerr << "fixed_worker join over\n";

  //�ö�̬����Ĺ����߳̽���
  for (auto& worker : dynamic_workers) {
    tasks.set_cancel(1);
    if (worker.joinable())worker.join();
  }
  //std::cerr << "dynamic_worker join over\n";
}


