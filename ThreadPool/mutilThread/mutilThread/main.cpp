//extern "C" {
//#include <unistd.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include "C_ThreadPool.h"
//}
#include <iostream>
#include "MyThreadPool.hpp"
std::mutex ostream_mutex;

static int cnt = 0;
void taskFunc(void* arg) {
  int num = *(int*)arg;
  {
    auto tid = std::this_thread::get_id();
    {
      std::lock_guard<std::mutex> lock(ostream_mutex);
      std::cout << "\t" << ++cnt << "=== thread is working, number=" << num
        << ", tid=" << tid << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  {
    //pthread_t tid = pthread_self();
    //printf("\t%d=== thread is working, number=%d, tid = %ld\n", ++cnt, num, tid % (int(1e8)));
    //sleep(0.5); 
  }
}

int main() {
  {
    std::cout << "begin test my_thread_pool" << std::endl;
    MyThreadPool pool(3, 10, 100);
    pool.start();
    for (int i = 0; i < 10; i++) {
      int* num = new int(i + 100);
      try {
        pool.submit(taskFunc, num);
      } catch (std::exception ex) {
        std::cerr << ex.what() << std::endl;
      }
    }
    

    std::this_thread::sleep_for(std::chrono::seconds(30));
    pool.stop();
    std::cout << "test over!" << std::endl;
  }





  //{
  //  CThreadPool* pool = createThreadPool(3, 10, 100);
  //  for (int i = 0; i < 100; i++) {
  //    int* num = (int*)malloc(sizeof(int));
  //    *num = (i + 100);
  //    addtoThreadPool(pool, taskFunc, num);
  //  }

  //  printf("111111111111111111111111111111111111\n");
  //  sleep(30);
  //  printf("2222222222222222222222222222222222222\n");
  //  destroyThreadPool(pool);
  //  printf("33333333333333333333333333333333333333\n");
  //}
  return 0;
}