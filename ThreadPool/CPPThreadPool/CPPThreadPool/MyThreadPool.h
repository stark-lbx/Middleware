#ifndef __STARK_THREADPOOL_MYTP__H__
#define __STARK_THREADPOOL_MYTP__H__

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "BlockQueue.hpp"
class MyThreadPool {
public:
  void submit(Task& task);//提交一个任务
  void submit(void(*callback)(void*), void* arg);//提交一个任务

  int get_total_size() const;//获取总的线程个数
  int get_alive_size() const;//获取空闲线程个数

  void start();
  void stop();

  /// <summary>
  /// qszie:    线程池的任务队列存储容量
  /// minsize:  线程池的最小线程限制
  /// maxsize:  线程池的最大线程限制
  /// </summary>
  MyThreadPool(int qsize, int minsize, int maxsize);
  ~MyThreadPool(); // 析构函数

private:
  int totalthreadsize()const {
    int append_size = dynamic_workers.size();
    return minsize + append_size;
  }
  int alivethreadsize()const {
    int total_size = minsize + dynamic_workers.size();
    return total_size - busyNum;
  }
  // 禁止拷贝（线程池不可复制）
  MyThreadPool(const MyThreadPool&) = delete;
  MyThreadPool& operator=(const MyThreadPool&) = delete;
private:
  BlockQueue<Task> tasks;//任务队列 --工作线程从任务队列获取队首任务执行

  const int minsize;
  const int maxsize;
  std::atomic<bool> closed;//线程池是否关闭
  std::atomic<int> busyNum;

  //线程管理
  mutable std::mutex mutex;//保护线程池的同步
  std::thread manager;//管理者线程
  std::vector<std::thread> fixed_workers;//固定工作者线程
  std::vector<std::thread> dynamic_workers;//动态工作者线程
};

// manager线程主要做循环检测操作
// worker线程主要执行队列中的任务

#endif // !__STARK_THREADPOOL_MYTP__H__