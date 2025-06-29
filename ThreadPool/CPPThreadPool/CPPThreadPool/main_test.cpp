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
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main() {
  std::cout << "begin test my_thread_pool" << std::endl;
  MyThreadPool pool(10, 5, 30);
  pool.start();
  for (int i = 0; i < 10; i++) {
    int* num = new int(i + 100);
    try {
      pool.submit(taskFunc, num);
    } catch (std::exception ex) {
      std::cerr << ex.what() << std::endl;
    }
  }


  std::this_thread::sleep_for(std::chrono::seconds(5));
  pool.stop();
  std::cout << "test over02" << std::endl;
  return 0;
}