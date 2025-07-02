#include <iostream>
#include "MyThreadPool.h"

std::mutex ostream_mutex;
static int cnt = 0;
void taskFunc(void* arg) {
  int num = *(int*)arg;

  auto tid = std::this_thread::get_id();
  {
    std::lock_guard<std::mutex> lock(ostream_mutex);
    std::cout << "\t" << ++cnt << "=== thread is working, number=" << num
      << ", tid=" << tid << std::endl;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 100 + 1));//1-100ms
}


void test_MyThreadPool() {
  std::cout << "begin test my_thread_pool" << std::endl;
  MyThreadPool pool(100, 5, 30);
  pool.start();

  std::thread th([&]() {
    for (int i = 0; i < 1000; i++) {
      int* num = new int(i + 100);
      try {
        pool.submit(taskFunc, num);
      } catch (std::exception ex) {
        std::cerr << ex.what() << std::endl;
      }
    }
  });
  th.join();//将所有任务提交完

  std::this_thread::sleep_for(std::chrono::seconds(3));
  pool.stop();//然后关闭线程池
}


int main() {
  auto now = std::chrono::system_clock::now();
  test_MyThreadPool();

  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now() - now
  ).count() << "ms" << std::endl;

  return 0;
}