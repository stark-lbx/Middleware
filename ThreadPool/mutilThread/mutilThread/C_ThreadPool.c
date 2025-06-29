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
  CThreadPool* pool = (CThreadPool*)arg;//����Ĳ���Ӧ����һ���̳߳�

  //ѭ��ִ�У�
  //  ����- �����̳߳ر�����
  //    �����ҵ�������Ƿ������� and �̳߳��Ƿ�û��
  //    ����ǣ���ô��ͨ���������� �ȴ�������Դ, ������ٻ���, ��Ҫʹ��while���
  //    Ȼ���ٴμ�飬����̳߳��ǹرյģ���ôӦ�ý����ͷŵ���Ȼ���߳��˳���
  //    ������������taskQ��ȡ��һ�������ƶ�ͷ�ڵ�ģ���Ƴ�����taskQ����Ϊѭ������ṹ��
  //  ����- ����һֱռ������Դ���º����γ�����
  //  
  //  ִ�к�������ִ��ǰ��busyNum++��ִ�к�busyNum--��busyNum��һ��������Դ����������
  //  
  while (1) {
    pthread_mutex_lock(&pool->mutexPool);
    while (pool->queueSize == 0 && pool->shutdown == 0) {
      //���������߳�-- while�ж�����������ٻ���
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

    //ȡ����
    Task task;
    task.function = pool->taskQ[pool->queueFront].function;
    task.arg = pool->taskQ[pool->queueFront].arg;
    //�ƶ�ͷ�ڵ�
    pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
    pool->queueSize--;
    pthread_cond_signal(&pool->notFull);

    pthread_mutex_unlock(&pool->mutexPool);

    /*���ú���ִ������*/
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
    sleep(3);//����һ��Ƶ�ʽ���������еļ����Ϣ

    //ȡ���̳߳������������ �͵�ǰ�̵߳�����
    //��Щ�������̳߳ص����ԣ���Ҫʹ���̳߳صĻ���������
    pthread_mutex_lock(&pool->mutexPool);
    int qsize = pool->queueSize;
    int aliveNum = pool->aliveNum;
    pthread_mutex_unlock(&pool->mutexPool);

    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);

    //����̡߳�
    //����ĸ������ڴ����߳���������߳�ִ������
    //and �����߳��� С�� �����߳���
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

    //�����̡߳�
    //æ���߳�*2 < �����߳��� && �����߳������� ��С���߳���
    if (busyNum * 2 < aliveNum && aliveNum > pool->minNum) {
      pthread_mutex_lock(&pool->mutexPool);
      {
        pool->exitNum = FIX_MODIFY;
      }
      pthread_mutex_unlock(&pool->mutexPool);

      //�ù������߳���ɱ
      for (int i = 0; i < FIX_MODIFY; i++) {
        pthread_cond_signal(&pool->notEmpty);//֪ͨ��һЩ�����ŵ��߳���ɱ
      }
    }
  }
  return NULL;
}




CThreadPool* createThreadPool(int min_, int max_, int qsize_) {
  if (min_ > max_ || min_ > qsize_) return NULL;//set is �������

  //���ӳ���Դ����
  CThreadPool* pool = (CThreadPool*)malloc(sizeof(CThreadPool));
  if (pool == NULL)return NULL;// malloc threadpool fail

  do {
    //���ӳ�����
    pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max_);
    if (pool->threadIDs == NULL) break;// malloc threadIDs fail
    memset(pool->threadIDs, 0, sizeof(pthread_t) * max_);

    pool->minNum = min_;
    pool->maxNum = max_;
    pool->busyNum = 0;
    pool->aliveNum = min_;//����ﵽ��С
    pool->exitNum = 0;


    //ͬ�����Ƴ�ʼ��
    if (pthread_mutex_init(&pool->mutexPool, NULL) != 0
      || pthread_mutex_init(&pool->mutexBusy, NULL) != 0
      || pthread_cond_init(&pool->notFull, NULL) != 0
      || pthread_cond_init(&pool->notEmpty, NULL) != 0) {
      break;
    }

    //�����������
    pool->taskQ = (Task*)malloc(sizeof(Task) * qsize_);
    if (pool->taskQ == NULL) break;//��ʱ����, �����ʱ��û�����ɹ�, �ȴ��������ʱ���ж�
    pool->queueCapacity = qsize_;
    pool->queueSize = 0;
    pool->queueFront = 0;
    pool->queueRear = 0;
    pool->shutdown = 0;

    //�߳�����
    pthread_create(&pool->managerID, NULL, manager, pool);
    for (int i = 0; i < min_; i++) {
      pthread_create(&pool->threadIDs[i], NULL, worker, pool);
    }
    return pool;
  } while (0);

  //�ͷ���Դ
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
    //�������չ������߳�
    pthread_join(pool->managerID, NULL);
    //���ѱ������������߳�
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
      //����������ģ��- ��Ҫ������� �����������
      pthread_cond_wait(&pool->notFull, &pool->mutexPool);
    }
    if (pool->shutdown == 1) {
      pthread_mutex_unlock(&pool->mutexPool);
      return;
    }
    //�������
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