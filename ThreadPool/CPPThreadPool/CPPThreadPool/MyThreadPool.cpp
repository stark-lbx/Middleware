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
      while (!closed.load()) {//�̳߳�δ�ر���һֱloop
        auto task = this->tasks.take(); //����ʽtake����

        // ����̳߳عر��ˣ���ô�Ϳ����˳���
        if (closed.load())break;
        // ����ȡ�����������ǿյģ�ɱ���߳���
        if (task.func == nullptr) {
          tasks.cancel(1);
          continue;
        }

        busyNum++;//æ�߳�+1

        task.func(task.args);

        busyNum--;//æ�߳�-1
      }
    }));
  }

  //�������̶߳�̬�����߳�
  manager = std::move(std::thread([this]() {
    //����߳�ʱ���ù̶����Ӹ���
    const int INCREMENT = 2;
    const int DECREMENT = 2;
    while (!closed.load()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));//����or��������
      std::lock_guard<std::mutex> lock(mutex);

      //����߳�
      //������� ���� ����̸߳��� ���� �����߳� С�� �����߳�����
      int aliveNum = alivethreadsize();
      if (tasks.size() > aliveNum && aliveNum < maxsize) {
        // �����̶��������߳�
        for (int i = 0; i < INCREMENT; i++) {
          dynamic_workers.emplace_back([this]() {
            auto tid = std::this_thread::get_id();

            //��Ӧ���뿪���̼߳�����û���ҵ��������̳߳�δ�رգ���Ӧ��ִ��
            while (!shouldExit.count(tid) && !closed.load()) {
              auto task = this->tasks.take();
              //����̳߳عر��ˣ�����ȡ�����������ǿյģ���ô�Ϳ����˳���
              if (closed.load() || task.func == nullptr)break;

              busyNum++;
              task.func(task.args);
              busyNum--;
            }

            shouldExit.insert(tid);
          });
        }
      }

      //�����߳�
      //æ���߳�*2 < �����߳� and �����߳���������С���߳���
      if (busyNum * 2 < aliveNum && aliveNum > maxsize) {
        // �Ӷ�����ȡ���̶��������߳�ȡ����
        tasks.cancel(DECREMENT);//�Ӷ�̬�̶߳����� �����߳�
      }
      //������Ӧ��������߳�
      dynamic_workers.erase(
        std::remove_if(dynamic_workers.begin(), dynamic_workers.end(),
          [this](const std::thread& t) {
        return !t.joinable() || shouldExit.count(t.get_id());
      }), dynamic_workers.end());
    }
  }));
  manager.detach();//���������̹߳ҵ���̨
}

void MyThreadPool::stop() {
  closed.store(true);
  // �رն���
  tasks.close();

  // �ȴ����ж�̬�߳����
  for (auto& worker : dynamic_workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
  dynamic_workers.clear();

  // �ȴ����й̶��߳����
  for (auto& worker : fixed_workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  // �ȴ������߳����
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


