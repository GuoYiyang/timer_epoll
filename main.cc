#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "error.h"
#include "timer.h"

std::mutex mtx;

template <class T>
class Singleton {
 private:
  static T* instance;
  static std::mutex mutex;
  //禁止拷贝构造和赋值运算符
  Singleton(const Singleton& src) {}
  Singleton& operator=(const Singleton& src) {}

 protected:
  //使继承者无法public构造函数和析构函数
  Singleton() {}
  ~Singleton() {}

 public:
  static T* GetInstance() {
    int ret = 0;
    // 双重校验锁
    if (instance == NULL) {
      std::unique_lock<std::mutex> lock(mutex);
      if (instance == NULL) {
        instance = new T();
        ret = instance->Init();
      }
    }
    if (ret != 0) {
      return NULL;
    } else {
      return instance;
    }
  }
};

template <class T>
T* Singleton<T>::instance = NULL;
template <class T>
std::mutex Singleton<T>::mutex;

class TimerThread : public Singleton<TimerThread> {
  friend class Singleton<TimerThread>;

 private:
  TimerThread() {}
  Timer timer_;

 public:
  int Init() {
    int ret = timer_.Init();
    if (ret != 0) {
      std::cout << Error::GetErrorString(ret) << std::endl;
    }
    std::thread timer_thread(&TimerThread::Start, this);
    timer_thread.detach();
  }

  int Add(int interval, std::function<void()> task) {
    return timer_.SetTimer(interval, std::bind(task));
  }

  int Start() {
    int ret = timer_.StartTimer();
    // 线程异常退出
    if (ret != 0) {
      std::cout << Error::GetErrorString(ret) << std::endl;
    }
  }
};

class A {
 public:
  A() {}
  int member_a_;
  int member_b_;
  int member_c_;

  void callback(void* value) {
    // 加锁，异步线程操作共享变量时保证线程安全
    std::lock_guard<std::mutex> lk(mtx);
    std::cout << "A: " << *((int*)value) << std::endl;
  }
};

class B {
 public:
  B() {}
  int member_a_;
  int member_b_;
  int member_c_;
  void callback(void* value) {
    std::lock_guard<std::mutex> lk(mtx);
    std::cout << "B: " << *((int*)value) << std::endl;
  }
};

class C {
 public:
  C() {}
  int member_a_;
  int member_b_;
  int member_c_;
  void callback(void* value) {
    std::lock_guard<std::mutex> lk(mtx);
    std::cout << "C: " << *((int*)value) << std::endl;
  }
};

int main() {
  int ret;

  A a;
  a.member_a_ = 1;
  ret = TimerThread::GetInstance()->Add(
      1, std::bind(&A::callback, &a, &a.member_a_));
  if (ret == 0) {
    std::cout << "add success!" << std::endl;
  } else {
    std::cout << Error::GetErrorString(ret) << std::endl;
  }

  B b;
  b.member_b_ = 2;
  ret = TimerThread::GetInstance()->Add(
      5, std::bind(&B::callback, &b, &b.member_b_));
  if (ret == 0) {
    std::cout << "add success!" << std::endl;
  } else {
    std::cout << Error::GetErrorString(ret) << std::endl;
  }

  C c;
  c.member_c_ = 3;
  ret = TimerThread::GetInstance()->Add(
      10, std::bind(&C::callback, &c, &c.member_c_));
  if (ret == 0) {
    std::cout << "add success!" << std::endl;
  } else {
    std::cout << Error::GetErrorString(ret) << std::endl;
  }

  while (1) {
    std::cout << "main running!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}