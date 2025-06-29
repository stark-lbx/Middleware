#ifndef __STARK_THREADPOOL_BLOCKQUEUE__H__
#define __STARK_THREADPOOL_BLOCKQUEUE__H__


#include "Task.h"
#include <mutex>
#include <condition_variable>

template<class T>
class BlockQueue {
public:
  void offer(const T& one);//插入一个
  T take();//拿出一个

  void clear();//清空队列
  void close();//关闭队列--允许取出无法插入
  void set_cancel(int num);//如果调用方不想阻塞那么多，那么取消阻塞中的num个

  bool empty() const;
  bool full() const;
  int size() const;
  int get_capacity() const;

  BlockQueue(int qsize);
  ~BlockQueue();
private:
  T* queue;//循环队列
  int head;//队头
  int rear;//队尾
  const int capacity;//总长
  bool closed;//是否关闭的标志
  int cancelsize;

  mutable std::mutex mtx;//访问循环队列时需要将mtx锁上，一些静态方法可能需要mtx进行加锁，用mutable修饰
  std::condition_variable customer;//队列的访问是个生产者、消费者模型
  std::condition_variable producer;//队列的访问是个生产者、消费者模型
};



//#include "BlockQueue.hpp"

template<class T>
BlockQueue<T>::BlockQueue(int qsize)
  :queue(new T[qsize]), head(0), rear(0),
  capacity(qsize), closed(false), cancelsize(0) {
  //init coding
}

template<class T>
BlockQueue<T>::~BlockQueue() {
  //mtx.~mutex();
  //cv.~condition_variable();
  if (!closed) close();
  if (queue)delete[] queue;
}

template<class T>
void BlockQueue<T>::offer(const T& one) {
  std::unique_lock<std::mutex> lock(mtx);

  while ((rear + 1) % capacity == head && !closed) {
    //队列为满--阻塞
    producer.wait(lock);
  }
  if (closed) return;

  queue[rear] = one;
  rear = (rear + 1) % capacity;
  customer.notify_one();//生产了一个,通知
}

template<class T>
T BlockQueue<T>::take() {
  std::unique_lock<std::mutex> lock(mtx);
  while (rear == head && !closed) {
    //队列为空 --阻塞
    customer.wait(lock);
    if (cancelsize > 0) {
      --cancelsize;
      return T();
    }
  }
  if (closed) return T();
  //customer.wait(lock, []()->bool {return (rear == head) && !closed; });

  T ret = queue[head];
  head = (head + 1) % capacity;
  producer.notify_one();//消费了一个,通知
  return ret;
}

template<class T>
void BlockQueue<T>::clear() {
  std::unique_lock<std::mutex> lock(mtx);

  head = rear = 0;
  customer.notify_all();
  producer.notify_all();
}

template<class T>
void BlockQueue<T>::close() {
  std::unique_lock<std::mutex> lock(mtx);

  closed = true;
  customer.notify_all();
  producer.notify_all();
}

template<class T>
void BlockQueue<T>::set_cancel(int num) {
  std::unique_lock<std::mutex> lock(mtx);
  cancelsize = num;
  for (int i = 0; i < cancelsize; i++) {
    customer.notify_one();
  }
}

template<class T>
bool BlockQueue<T>::empty() const {
  std::unique_lock<std::mutex> lock(mtx);
  return (rear == head);
}

template<class T>
bool BlockQueue<T>::full() const {
  std::unique_lock<std::mutex> lock(mtx);
  return (rear + 1) % capacity == head;
}

template<class T>
int BlockQueue<T>::size() const {
  std::unique_lock<std::mutex> lock(mtx);
  return (rear + capacity - head) % capacity;
}

template<class T>
int BlockQueue<T>::get_capacity() const {
  return capacity;
}


#endif // !__STARK_THREADPOOL_BLOCKQUEUE__H__