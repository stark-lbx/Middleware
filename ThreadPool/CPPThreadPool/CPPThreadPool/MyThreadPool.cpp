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

  //工作者线程执行while-loop一直工作
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

  //管理者线程动态分配线程
  manager = std::move(std::thread([this]() {
    //添加线程时采用固定增加个数
    const int INCREMENT = 2;
    const int DECREMENT = 2;
    while (!closed.load()) {
      std::this_thread::sleep_for(std::chrono::seconds(3));//周期
      std::lock_guard<std::mutex> lock(mutex);

      //添加线程
      //任务个数 大于 存活线程个数 并且 存活的线程 小于 最大的线程限制
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

      //销毁线程
      //忙的线程*2 < 存活的线程 and 存活的线程数大于最小的线程数
      if (busyNum * 2 < aliveNum && aliveNum > maxsize) {
        tasks.set_cancel(DECREMENT);
      }
    }
  }));
  manager.detach();//将管理者线程挂到后台
}

void MyThreadPool::stop() {
  closed.store(true);
  //关闭入队操作
  tasks.close();
}

MyThreadPool::MyThreadPool(int qsize, int minsize, int maxsize)
  :tasks(qsize), minsize(minsize), maxsize(maxsize),
  closed(false), busyNum(0) {
  fixed_workers = std::vector<std::thread>(minsize);
}

MyThreadPool::~MyThreadPool() {
  this->stop();
  // clear会通知所有生产者和消费者解锁，
  // 解锁后 如果队列是关闭的 会直接从所在函数返回
  tasks.clear();

  if (manager.joinable())manager.join();
  //std::cerr << "manager join over\n";

  //保证固定的核心工作线程都回收资源
  for (auto& worker : fixed_workers) {
    if (worker.joinable())  worker.join();
  }
  //std::cerr << "fixed_worker join over\n";

  //让动态分配的工作线程结束
  for (auto& worker : dynamic_workers) {
    tasks.set_cancel(1);
    if (worker.joinable())worker.join();
  }
  //std::cerr << "dynamic_worker join over\n";
}


