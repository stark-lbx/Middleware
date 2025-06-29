#ifndef __STARK_THREADPOOL_BLOCKQUEUE__H__
#define __STARK_THREADPOOL_BLOCKQUEUE__H__


#include "Task.h"
#include <mutex>
#include <condition_variable>

template<class T>
class BlockQueue {
public:
  void offer(const T& one);//����һ��
  T take();//�ó�һ��

  void clear();//��ն���
  void close();//�رն���--����ȡ���޷�����
  void set_cancel(int num);//������÷�����������ô�࣬��ôȡ�������е�num��

  bool empty() const;
  bool full() const;
  int size() const;
  int get_capacity() const;

  BlockQueue(int qsize);
  ~BlockQueue();
private:
  T* queue;//ѭ������
  int head;//��ͷ
  int rear;//��β
  const int capacity;//�ܳ�
  bool closed;//�Ƿ�رյı�־
  int cancelsize;

  mutable std::mutex mtx;//����ѭ������ʱ��Ҫ��mtx���ϣ�һЩ��̬����������Ҫmtx���м�������mutable����
  std::condition_variable customer;//���еķ����Ǹ������ߡ�������ģ��
  std::condition_variable producer;//���еķ����Ǹ������ߡ�������ģ��
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
    //����Ϊ��--����
    producer.wait(lock);
  }
  if (closed) return;

  queue[rear] = one;
  rear = (rear + 1) % capacity;
  customer.notify_one();//������һ��,֪ͨ
}

template<class T>
T BlockQueue<T>::take() {
  std::unique_lock<std::mutex> lock(mtx);
  while (rear == head && !closed) {
    //����Ϊ�� --����
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
  producer.notify_one();//������һ��,֪ͨ
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