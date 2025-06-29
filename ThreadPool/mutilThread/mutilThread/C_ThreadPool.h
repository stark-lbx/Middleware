#ifndef __STARK_CTHREAD_POOL__H__
#define __STARK_CTHREAD_POOL__H__

#include <pthread.h>

//����ṹ��
typedef struct Task {
  void(*function)(void*);
  void* arg;
}Task;

//�̳߳ؽṹ��
typedef struct CThreadPool {
  //�������
  Task* taskQ;
  int queueCapacity;//����
  int queueSize;//��ǰ�������
  int queueFront;//��ͷ-ȡ����
  int queueRear;//��β-������

  //�̳߳�����
  pthread_t managerID;//�������߳�ID
  pthread_t* threadIDs;//�����߳�ID
  int minNum;//��С�߳���
  int maxNum;//����߳���
  int busyNum;//����æ���߳���
  int aliveNum;//�����߳���
  int exitNum;//Ҫ���ٵ��߳���

  //���̡߳�ͬ������
  pthread_mutex_t mutexPool;
  pthread_mutex_t mutexBusy;
  pthread_cond_t notFull;
  pthread_cond_t notEmpty;

  int shutdown;//�ǲ���Ҫ�����̳߳���
}CThreadPool;

//�����̳߳�
CThreadPool* createThreadPool(int min_, int max_, int qsize_);
//�̳߳�����
void destroyThreadPool(CThreadPool* pool);
//���̳߳��������
void addtoThreadPool(CThreadPool* pool, void(*callback)(void*), void*);
//��ȡ�̳߳��й������̸߳���
int getWorkThreadsCount(CThreadPool* pool_);
//��ȡ�̳߳��л��ŵ��̸߳���
int getAliveThreadsCount(CThreadPool* pool_);


#endif // !__STARK_CTHREAD_POOL__H__