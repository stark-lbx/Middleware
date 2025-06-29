#ifndef __STARK_CTHREAD_POOL__H__
#define __STARK_CTHREAD_POOL__H__

#include <pthread.h>

//任务结构体
typedef struct Task {
  void(*function)(void*);
  void* arg;
}Task;

//线程池结构体
typedef struct CThreadPool {
  //任务队列
  Task* taskQ;
  int queueCapacity;//容量
  int queueSize;//当前任务个数
  int queueFront;//对头-取数据
  int queueRear;//队尾-放数据

  //线程池配置
  pthread_t managerID;//管理者线程ID
  pthread_t* threadIDs;//工作线程ID
  int minNum;//最小线程数
  int maxNum;//最大线程数
  int busyNum;//正在忙的线程数
  int aliveNum;//存活的线程数
  int exitNum;//要销毁的线程数

  //多线程、同步机制
  pthread_mutex_t mutexPool;
  pthread_mutex_t mutexBusy;
  pthread_cond_t notFull;
  pthread_cond_t notEmpty;

  int shutdown;//是不是要销毁线程池了
}CThreadPool;

//创建线程池
CThreadPool* createThreadPool(int min_, int max_, int qsize_);
//线程池销毁
void destroyThreadPool(CThreadPool* pool);
//给线程池添加任务
void addtoThreadPool(CThreadPool* pool, void(*callback)(void*), void*);
//获取线程池中工作的线程个数
int getWorkThreadsCount(CThreadPool* pool_);
//获取线程池中活着的线程个数
int getAliveThreadsCount(CThreadPool* pool_);


#endif // !__STARK_CTHREAD_POOL__H__