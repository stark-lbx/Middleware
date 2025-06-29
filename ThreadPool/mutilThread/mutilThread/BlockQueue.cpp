//#include "BlockQueue.hpp"
//
//template<class T>
//BlockQueue<T>::BlockQueue(int qsize)
//  :queue(new T[qsize]), head(0), rear(0),
//  capacity(qsize), closed(false), cancelsize(0) {
//  //init coding
//}
//
//template<class T>
//BlockQueue<T>::~BlockQueue() {
//  //mtx.~mutex();
//  //cv.~condition_variable();
//  close();
//  if (queue)delete[] queue;
//}
//
//template<class T>
//void BlockQueue<T>::offer(const T& one) {
//  std::unique_lock<std::mutex> lock(mtx);
//
//  while ((rear + 1) % capacity == head && !closed) {
//    //队列为满--阻塞
//    producer.wait(lock);
//  }
//  if (closed) return;
//
//  queue[rear] = one;
//  rear = (rear + 1) % capacity;
//  customer.notify_one();//生产了一个,通知
//}
//
//template<class T>
//T BlockQueue<T>::take() {
//  std::unique_lock<std::mutex> lock(mtx);
//  while (rear == head) {
//    //队列为空 --阻塞
//    customer.wait(lock);
//    if (cancelsize > 0) {
//      --cancelsize;
//      return T();
//    }
//
//  }
//  if (closed) return T();
//  //customer.wait(lock, []()->bool {return (rear == head) && !closed; });
//
//  T ret = queue[head];
//  head = (head + 1) % capacity;
//  producer.notify_one();//消费了一个,通知
//  return ret;
//}
//
//template<class T>
//void BlockQueue<T>::clear() {
//  std::unique_lock<std::mutex> lock(mtx);
//
//  head = rear = 0;
//  customer.notify_all();
//  producer.notify_all();
//}
//
//template<class T>
//void BlockQueue<T>::close() {
//  std::unique_lock<std::mutex> lock(mtx);
//
//  closed = true;
//  customer.notify_all();
//  producer.notify_all();
//}
//
//template<class T>
//void BlockQueue<T>::set_cancel(int num) {
//  std::unique_lock<std::mutex> lock(mtx);
//  cancelsize = num;
//  for (int i = 0; i < cancelsize; i++) {
//    customer.notify_one();
//  }
//}
//
//template<class T>
//bool BlockQueue<T>::empty() const {
//  std::unique_lock<std::mutex> lock(mtx);
//  return (rear == head);
//}
//
//template<class T>
//bool BlockQueue<T>::full() const {
//  std::unique_lock<std::mutex> lock(mtx);
//  return (rear + 1) % capacity == head;
//}
//
//template<class T>
//int BlockQueue<T>::size() const {
//  std::unique_lock<std::mutex> lock(mtx);
//  return (rear + capacity - head) % capacity;
//}
//
//template<class T>
//int BlockQueue<T>::get_capacity() const {
//  return capacity;
//}
