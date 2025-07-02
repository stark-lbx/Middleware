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
      while (!closed.load()) {//线程池未关闭则一直loop
        auto task = this->tasks.take(); //阻塞式take任务

        // 如果线程池关闭了，那么就可以退出了
        if (closed.load())break;
        // 或者取出来的任务是空的，杀错线程了
        if (task.func == nullptr) {
          tasks.cancel(1);
          continue;
        }

        busyNum++;//忙线程+1

        task.func(task.args);

        busyNum--;//忙线程-1
      }
    }));
  }

  //管理者线程动态分配线程
  manager = std::move(std::thread([this]() {
    //添加线程时采用固定增加个数
    const int INCREMENT = 2;
    const int DECREMENT = 2;
    while (!closed.load()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));//扩容or缩容周期
      std::lock_guard<std::mutex> lock(mutex);

      //添加线程
      //任务个数 大于 存活线程个数 并且 存活的线程 小于 最大的线程限制
      int aliveNum = alivethreadsize();
      if (tasks.size() > aliveNum && aliveNum < maxsize) {
        // 新增固定数量的线程
        for (int i = 0; i < INCREMENT; i++) {
          dynamic_workers.emplace_back([this]() {
            auto tid = std::this_thread::get_id();

            //在应该离开的线程集合中没有找到，并且线程池未关闭，才应该执行
            while (!shouldExit.count(tid) && !closed.load()) {
              auto task = this->tasks.take();
              //如果线程池关闭了，或者取出来的任务是空的，那么就可以退出了
              if (closed.load() || task.func == nullptr)break;

              busyNum++;
              task.func(task.args);
              busyNum--;
            }

            shouldExit.insert(tid);
          });
        }
      }

      //销毁线程
      //忙的线程*2 < 存活的线程 and 存活的线程数大于最小的线程数
      if (busyNum * 2 < aliveNum && aliveNum > maxsize) {
        // 从队列中取消固定数量的线程取任务
        tasks.cancel(DECREMENT);//从动态线程队列中 撤销线程
      }
      //检测清除应该清除的线程
      dynamic_workers.erase(
        std::remove_if(dynamic_workers.begin(), dynamic_workers.end(),
          [this](const std::thread& t) {
        return !t.joinable() || shouldExit.count(t.get_id());
      }), dynamic_workers.end());
    }
  }));
  manager.detach();//将管理者线程挂到后台
}

void MyThreadPool::stop() {
  closed.store(true);
  // 关闭队列
  tasks.close();

  // 等待所有动态线程完成
  for (auto& worker : dynamic_workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
  dynamic_workers.clear();

  // 等待所有固定线程完成
  for (auto& worker : fixed_workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  // 等待管理线程完成
  if (manager.joinable()) {
    manager.join();
  }
}

MyThreadPool::MyThreadPool(int qsize, int minsize, int maxsize)
  :tasks(qsize), minsize(minsize), maxsize(maxsize),
  closed(false), busyNum(0) {
  fixed_workers = std::vector<std::thread>(minsize);
}

MyThreadPool::~MyThreadPool() {
  if (!closed.load())this->stop();
}


