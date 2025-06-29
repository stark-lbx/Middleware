#ifndef __STARK_THREADPOOL_MYTP__H__
#define __STARK_THREADPOOL_MYTP__H__

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "BlockQueue.hpp"
class MyThreadPool {
public:
  void submit(Task& task);//�ύһ������
  void submit(void(*callback)(void*), void* arg);//�ύһ������

  int get_total_size() const;//��ȡ�ܵ��̸߳���
  int get_alive_size() const;//��ȡ�����̸߳���

  void start();
  void stop();

  /// <summary>
  /// qszie:    �̳߳ص�������д洢����
  /// minsize:  �̳߳ص���С�߳�����
  /// maxsize:  �̳߳ص�����߳�����
  /// </summary>
  MyThreadPool(int qsize, int minsize, int maxsize);
  ~MyThreadPool(); // ��������

private:
  int totalthreadsize()const {
    int append_size = dynamic_workers.size();
    return minsize + append_size;
  }
  int alivethreadsize()const {
    int total_size = minsize + dynamic_workers.size();
    return total_size - busyNum;
  }
  // ��ֹ�������̳߳ز��ɸ��ƣ�
  MyThreadPool(const MyThreadPool&) = delete;
  MyThreadPool& operator=(const MyThreadPool&) = delete;
private:
  BlockQueue<Task> tasks;//������� --�����̴߳�������л�ȡ��������ִ��

  const int minsize;
  const int maxsize;
  std::atomic<bool> closed;//�̳߳��Ƿ�ر�
  std::atomic<int> busyNum;

  //�̹߳���
  mutable std::mutex mutex;//�����̳߳ص�ͬ��
  std::thread manager;//�������߳�
  std::vector<std::thread> fixed_workers;//�̶��������߳�
  std::vector<std::thread> dynamic_workers;//��̬�������߳�
};

// manager�߳���Ҫ��ѭ��������
// worker�߳���Ҫִ�ж����е�����

#endif // !__STARK_THREADPOOL_MYTP__H__