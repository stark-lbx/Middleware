#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "C_ThreadPool.h"
#define FIX_MODIFY 2

void threadExit(CThreadPool* pool) {
  pthread_t tid = pthread_self();
  //pthread_mutex_lock(&pool->mutexPool);
  for (int i = 0; i < pool->maxNum; i++) {
    if (pool->threadIDs[i] == tid) {
      pool->threadIDs[i] = 0;
      break;
    }
  }
  pthread_exit(NULL);
}

void* worker(void* arg) {
  CThreadPool* pool = (CThreadPool*)arg;//传入的参数应该是一个线程池

  //循环执行：
  //  加锁- 避免线程池被竞争
  //    检查作业队列中是否有任务 and 线程池是否还没关
  //    如果是，那么就通过条件变量 等待锁的资源, 避免虚假唤醒, 需要使用while检查
  //    然后再次检查，如果线程池是关闭的，那么应该将锁释放掉，然后线程退出。
  //    否则从任务队列taskQ中取出一个任务，移动头节点模拟移除任务（taskQ设置为循环数组结构）
  //  解锁- 避免一直占用锁资源导致后面形成死锁
  //  
  //  执行函数任务，执行前将busyNum++；执行后将busyNum--；busyNum是一个竞争资源，加锁访问
  //  
  while (1) {
    pthread_mutex_lock(&pool->mutexPool);
    while (pool->queueSize == 0 && pool->shutdown == 0) {
      //阻塞工作线程-- while判断条件避免虚假唤醒
      pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

      if (pool->exitNum > 0) {
        pool->exitNum--;

        if (pool->aliveNum > pool->minNum) {
          pool->aliveNum--;
          pthread_mutex_unlock(&pool->mutexPool);
          threadExit(pool);
        }
      }
    }
    if (pool->shutdown) {
      pthread_mutex_unlock(&pool->mutexPool);
      threadExit(pool);
    }

    //取任务
    Task task;
    task.function = pool->taskQ[pool->queueFront].function;
    task.arg = pool->taskQ[pool->queueFront].arg;
    //移动头节点
    pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
    pool->queueSize--;
    pthread_cond_signal(&pool->notFull);

    pthread_mutex_unlock(&pool->mutexPool);

    /*调用函数执行任务*/
    {
      pthread_mutex_lock(&pool->mutexBusy);
      pool->busyNum++;
      pthread_mutex_unlock(&pool->mutexBusy);

      task.function(task.arg);
      free(task.arg);
      task.arg = NULL;

      pthread_mutex_lock(&pool->mutexBusy);
      pool->busyNum--;
      pthread_mutex_unlock(&pool->mutexBusy);
    }
  }
  return NULL;
}

void* manager(void* arg) {
  CThreadPool* pool = (CThreadPool*)arg;
  while (!pool->shutdown) {
    sleep(3);//按照一定频率进行任务队列的检测信息

    //取出线程池中任务的数量 和当前线程的数量
    //这些属性是线程池的属性，需要使用线程池的互斥锁加锁
    pthread_mutex_lock(&pool->mutexPool);
    int qsize = pool->queueSize;
    int aliveNum = pool->aliveNum;
    pthread_mutex_unlock(&pool->mutexPool);

    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);

    //添加线程、
    //任务的个数大于存活的线程数，添加线程执行任务
    //and 存活的线程数 小于 最大的线程数
    if (qsize > aliveNum && aliveNum < pool->maxNum) {
      int count = 0;
      pthread_mutex_lock(&pool->mutexPool);
      {
        for (int i = 0; i < pool->maxNum
          && count++ < FIX_MODIFY && pool->aliveNum < pool->maxNum; i++) {
          if (pool->threadIDs[i] == 0) {
            pthread_create(&pool->threadIDs[i], NULL, worker, pool);
            pool->aliveNum++;
          }
        }
      }
      pthread_mutex_unlock(&pool->mutexPool);
    }

    //销毁线程、
    //忙的线程*2 < 存活的线程数 && 存活的线程数大于 最小的线程数
    if (busyNum * 2 < aliveNum && aliveNum > pool->minNum) {
      pthread_mutex_lock(&pool->mutexPool);
      {
        pool->exitNum = FIX_MODIFY;
      }
      pthread_mutex_unlock(&pool->mutexPool);

      //让工作的线程自杀
      for (int i = 0; i < FIX_MODIFY; i++) {
        pthread_cond_signal(&pool->notEmpty);//通知让一些阻塞着的线程自杀
      }
    }
  }
  return NULL;
}




CThreadPool* createThreadPool(int min_, int max_, int qsize_) {
  if (min_ > max_ || min_ > qsize_) return NULL;//set is 不合理的

  //连接池资源分配
  CThreadPool* pool = (CThreadPool*)malloc(sizeof(CThreadPool));
  if (pool == NULL)return NULL;// malloc threadpool fail

  do {
    //连接池配置
    pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max_);
    if (pool->threadIDs == NULL) break;// malloc threadIDs fail
    memset(pool->threadIDs, 0, sizeof(pthread_t) * max_);

    pool->minNum = min_;
    pool->maxNum = max_;
    pool->busyNum = 0;
    pool->aliveNum = min_;//必须达到最小
    pool->exitNum = 0;


    //同步机制初始化
    if (pthread_mutex_init(&pool->mutexPool, NULL) != 0
      || pthread_mutex_init(&pool->mutexBusy, NULL) != 0
      || pthread_cond_init(&pool->notFull, NULL) != 0
      || pthread_cond_init(&pool->notEmpty, NULL) != 0) {
      break;
    }

    //任务队列配置
    pool->taskQ = (Task*)malloc(sizeof(Task) * qsize_);
    if (pool->taskQ == NULL) break;//暂时不管, 如果暂时还没创建成功, 等待添加任务时再判断
    pool->queueCapacity = qsize_;
    pool->queueSize = 0;
    pool->queueFront = 0;
    pool->queueRear = 0;
    pool->shutdown = 0;

    //线程配置
    pthread_create(&pool->managerID, NULL, manager, pool);
    for (int i = 0; i < min_; i++) {
      pthread_create(&pool->threadIDs[i], NULL, worker, pool);
    }
    return pool;
  } while (0);

  //释放资源
  if (pool && !pool->threadIDs) free(pool->threadIDs);
  if (pool && !pool->taskQ) free(pool->taskQ);
  if (pool) free(pool);

  return NULL;
}

void destroyThreadPool(CThreadPool* pool) {
  if (pool == NULL)return;
  pthread_mutex_lock(&pool->mutexPool);
  {
    pool->shutdown = 1;

    printf("1================================================\n");
    //阻塞回收管理者线程
    pthread_join(pool->managerID, NULL);
    //唤醒被阻塞的消费线程
    printf("2================================================\n");
    pthread_cond_signal(&pool->notEmpty);
    printf("3================================================\n");
  }
  pthread_mutex_unlock(&pool->mutexPool);


  if (pool->taskQ != NULL) free(pool->taskQ);
  if (pool->threadIDs != NULL) free(pool->threadIDs);
  pthread_mutex_destroy(&pool->mutexPool);
  pthread_mutex_destroy(&pool->mutexBusy);
  pthread_cond_destroy(&pool->notFull);
  pthread_cond_destroy(&pool->notEmpty);
  free(pool);
}

void addtoThreadPool(CThreadPool* pool, void(*callback)(void*), void* args) {
  pthread_mutex_lock(&pool->mutexPool);
  {
    while (pool->queueSize == pool->queueCapacity && pool->shutdown == 0) {
      //阻塞生产者模型- 不要往里加了 任务队列满了
      pthread_cond_wait(&pool->notFull, &pool->mutexPool);
    }
    if (pool->shutdown == 1) {
      pthread_mutex_unlock(&pool->mutexPool);
      return;
    }
    //添加任务
    pool->taskQ[pool->queueRear].function = callback;
    pool->taskQ[pool->queueRear].arg = args;
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;
    pthread_cond_signal(&pool->notEmpty);
  }
  pthread_mutex_unlock(&pool->mutexPool);
}

int getWorkThreadsCount(CThreadPool* pool_) {
  if (!pool_)return -1;
  int workNum;
  pthread_mutex_lock(&pool_->mutexBusy);
  {
    workNum = pool_->busyNum;
  }
  pthread_mutex_unlock(&pool_->mutexBusy);
  return workNum;
}

int getAliveThreadsCount(CThreadPool* pool_) {
  if (!pool_)return -1;
  int aliveNum;
  pthread_mutex_lock(&pool_->mutexPool);
  {
    aliveNum = pool_->aliveNum;
  }
  pthread_mutex_unlock(&pool_->mutexPool);
  return aliveNum;
}