#include "my_base.h"
#include <coroutine>
#ifndef _MY_TASK
#define _MY_TASK 1

namespace mystd {

// 当前类型的所有东西,不管是承诺还是awaiter都是一个协程函数的状态
// 跟协程函数中await的其他对象没有关系
// 所以当协程函数中没有发生挂起时,需要处理在承诺跟awaiter之间同步状态
// 假如协程函数挂起多次需要看是否需要处理状态
// 当协程函数实际控制流程返回这个类型给调用方时, 要么协程函数挂起,
// 要么没有挂起直接执行完毕 只有返回时调用方才会调用类型的awaiter方法
template <typename T> struct MyTask {
  struct promise_type;
  using handle_type = std::coroutine_handle<promise_type>;

  using up_await_handle_type = std::coroutine_handle<void>;

  handle_type m_handle;

  MyTask(handle_type handle) : m_handle(handle) {}

  auto &getpromise() const { return m_handle.promise(); }

  bool await_ready() const noexcept { return false; }

  void await_suspend(up_await_handle_type handle) noexcept {

    getpromise().m_up_await_handle = handle;
  }

  T await_resume() const { return getpromise().m_return_data; }

  //! promise_type就是承诺对象，承诺对象用于协程内外交流
  struct promise_type {

    bool m_is_set_return;
    up_await_handle_type m_up_await_handle;
    T m_return_data;

    promise_type() = default;

    ~promise_type() { 
        //Print("~promise_type", (void *)this);
    }

    //! 生成协程返回值
    auto get_return_object() {
        //Print("get_return_object", (void *)this);
      return MyTask<T>{handle_type::from_promise(*this)};
    }

    auto initial_suspend() { return std::suspend_never{}; }

    void resume_up_await() {
      //Print("resume_up_await", (void *)this);
      if (m_up_await_handle) {
        m_up_await_handle.resume();
      }
    }

    void return_value(T v) {

      m_return_data = v;

      m_is_set_return = true;

      resume_up_await();
    }

    auto final_suspend() noexcept {
      //Print("final_suspend", (void *)this);
      return std::suspend_never{};
    }

    void unhandled_exception() {
      Exit("unhandled_exception");
    }
  };
};

}; // namespace mystd
#endif