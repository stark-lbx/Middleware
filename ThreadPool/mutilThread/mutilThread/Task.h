#ifndef __STARK_THREADPOOL_TASK__H__
#define __STARK_THREADPOOL_TASK__H__


#include <functional>

//template<typename... Args>
//struct Task2 {
//  std::function<void(Args&&... args)> func;
//};

struct Task {
  std::function<void(void*)> func;
  void* args;
};


#endif // !__STARK_THREADPOOL_TASK__H__