#include <coroutine>
#include <cstdio>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>


void Print() {
	std::cout << std::endl;
}

template<typename T, typename ...TS>
void Print(T value, TS ...values) {
	std::cout << value << "   ";
	Print(values...);
}

template <typename T> struct MyTask {
  struct promise_type;
  using handle_type = std::coroutine_handle<promise_type>;

  using up_await_handle_type = std::coroutine_handle<void>;

  using up_await_handle_ptr_type = std::shared_ptr<up_await_handle_type>;
  
  handle_type m_handle;

  up_await_handle_ptr_type m_up_await_handle;

  MyTask(handle_type h, up_await_handle_ptr_type up_handle) : m_handle(h) {

    m_up_await_handle = up_handle;
  }
  // coro_ret(const coro_ret&) = delete;
  // coro_ret(coro_ret&& s)=delete;

  // coro_ret& operator=(const coro_ret&) = delete;
  // coro_ret& operator=(coro_ret&& s)=delete;

  //! 通过promise获取数据，返回值
  T get() const { return m_handle.promise().m_return_data; }

  bool await_ready() const noexcept { return false; }

  void await_suspend(up_await_handle_type handle) noexcept {
   
    *m_up_await_handle = handle;
  }

  T await_resume() const noexcept { return get(); }

  //! promise_type就是承诺对象，承诺对象用于协程内外交流
  struct promise_type {

    up_await_handle_ptr_type m_up_await_handle;
    T m_return_data;
    promise_type() {
    
      m_up_await_handle = std::make_shared<up_await_handle_type>();
    }

    //! 生成协程返回值
    auto get_return_object() {
      
      return MyTask<T>{handle_type::from_promise(*this), m_up_await_handle};
    }

  
    auto initial_suspend() {
      return std::suspend_never{};
     
    }
   
    void return_value(T v) {

      
      m_return_data = v;

      auto handle = *m_up_await_handle.get();

      if (handle) {
       
        handle.resume();
      }
     
    }

   
    auto final_suspend() noexcept {
     
      return std::suspend_never{};
    }
  
    void unhandled_exception() {
      Print("unhandled_exception exit");
      std::exit(1);
    }
  
    
  };
};

std::coroutine_handle<> s_hanlde;

// 这就是一个协程函数
MyTask<std::string> coroutine_1() {
  struct Atawer {

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept {
      Print("1 suspend");
      s_hanlde = handle;
    }

    void await_resume() const noexcept {}
  };
  Print("1 start");
  char buf[65535];
  co_await Atawer{};
  Print("1 end");
  co_return "8848 ";
}

MyTask<std::string> coroutine2() {
  Print("2 start");
  auto task = coroutine_1();
  char buf[65535];
  auto s = co_await task;
  Print("2 end");

 
  co_return s + std::string{"114514"};
}

MyTask<std::string> coroutine3() {
  Print("3 start");
  char buf[65535];

  auto s = co_await coroutine2();
  Print("3 end");
  co_return s + "zroe";
}

int main2(int argc, char *argv[]) {
 
  auto v = argv[argc-1];

  for (int n = 0; n < 100; n++) {
    auto c_r = coroutine3();
  
    if (s_hanlde) {
      Print("s_handle resume");
      s_hanlde.resume();

    } else {

      Print("s_handle error");
    }
    Print("main return ");

    Print(c_r.get());
  }

  int n;

  std::cin >> n;
  Print("over1");
  return 0;
}


void f(){
  //throw std::exception{};
}

MyTask<int> asyncThrow(){

Print("1 start");
co_await std::suspend_never{};
Print("1end");
  f();
co_return 1;


}



MyTask<int> asyncThrow2(){
  Print("2 start");
  try{
    co_await asyncThrow();
    Print("2 end");
  }
  catch(std::exception& e){

  }

  co_return 2;



}



int main(){

  auto c_r = asyncThrow2();


   Print("over1");
  return 0;

}