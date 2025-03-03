
#include <cerrno>
#include <coroutine>
#include <cstddef>
#include <cstdlib>
#ifndef _MY_SOCKET
#define _MY_SOCKET 1

#include "my_base.h"
#include "my_epoll.h"
#include "my_task.h"
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <queue>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace mystd {

class IPAddress {
private:
  sockaddr_in m_address;

public:
  explicit IPAddress(sockaddr_in ip) noexcept { m_address = ip; }

  IPAddress(sockaddr_in ip, uint16_t port) noexcept {
    ip.sin_port = htons(port);

    m_address = ip;
  }

  IPAddress(char b1, char b2, char b3, char b4, uint16_t port) noexcept {
    uint32_t value;

    auto p = (char *)&value;

    p[0] = b1;
    p[1] = b2;
    p[2] = b3;
    p[3] = b4;

    sockaddr_in address{};

    address.sin_family = AF_INET;

    address.sin_port = htons(port);

    in_addr addr{};

    addr.s_addr = value;

    address.sin_addr = addr;

    m_address = address;
  }

  void SetPort(uint16_t port) noexcept { m_address.sin_port = htons(port); }

  sockaddr const *get() const noexcept { return (sockaddr *)&m_address; }

  size_t getlen() const noexcept { return sizeof(sockaddr_in); }
};

template <typename T> class PlaceBox {
  std::queue<size_t> m_index;

  std::vector<T> m_array;

public:
  PlaceBox() : m_index(), m_array() {}

  size_t Add(T &&value) {

    if (m_index.size() == 0) {

      size_t index = m_array.size();

      m_array.push_back(std::move(value));

      return index;
    } else {
      size_t index = m_index.front();
      m_index.pop();

      m_array[index] = std::move(value);

      return index;
    }
  }

  // 不删除元素，要确保元素不再使用或者已经移动
  void SetCanUsed(size_t index) { m_index.push(index); }

  T &Get(size_t index) { return m_array[index]; }
};

class IEvent {
public:
  virtual void OnEvent(EpollEvent flag) = 0;
  virtual ~IEvent() {// Print("IEvent delete call");
   }
};

class MyAsyncData : mystd::Delete_Base {
  Epoll m_epoll;

  PlaceBox<std::unique_ptr<IEvent>> m_not_delete_event;

  std::queue<std::unique_ptr<IEvent>> m_need_delete_event;

public:
  MyAsyncData() : m_epoll(), m_not_delete_event(), m_need_delete_event() {}

  auto AddEvent(std::unique_ptr<IEvent> &&event) {
    size_t index = m_not_delete_event.Add(std::move(event));

    return index;
  }

  void DeleteEvent(size_t index) {
    auto &event = m_not_delete_event.Get(index);

    m_need_delete_event.push(std::move(event));

    m_not_delete_event.SetCanUsed(index);
  }

  auto &GetEpoll() { return m_epoll; }

  template <typename T> 
  void IntoEventLoop(T func) {

    try {

      func();

      std::array<epoll_event, 512> events;

      while (true) {
        {
          while (m_need_delete_event.size() != 0) {
            // auto event = std::move(m_need_delete_event.front());

            m_need_delete_event.pop();
          }
        }

        auto length = m_epoll.Wait(events);

        for (auto i = 0; i < length; i++) {
          auto item = &events[(uint)i];

          auto p = static_cast<IEvent *>(item->data.ptr);

          p->OnEvent(static_cast<EpollEvent>(item->events));
        }
      }
    } catch (std::exception &p) {
      Exit(p.what());
    }
  }
};

class Info {
public:
  static auto &GetMyAsyncData() {
    static MyAsyncData v{};

    return v;
  }
};

class Socket : mystd::Delete_Base {

  int m_handle;

protected:
  
  
  Socket(int handle) noexcept : m_handle(handle) {
  }

  static auto CreateSocket() noexcept {
    auto handle = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (handle == -1) {
      Exit("create socket error");
    }
    
    return handle;
  }

  

public:
  int GetHandle() const noexcept { return m_handle; }

  int GetSocketError() noexcept {
    int value;

    socklen_t length = sizeof(value);

    if (-1 == getsockopt(m_handle, SOL_SOCKET, SO_ERROR, &value, &length)) {
      Exit("get socket error error");
      // 进程会退出
    }

    return value;
  }

  virtual ~Socket() {
    if (-1 == close(m_handle)) {
      Exit("close Socket error");
    }

    Print("socket close");
  }
};

enum class ReadWriteMode { 
    Node, 
    ReadOnly, 
    WriteOnly, 
    RadWrite 
};

using void_coroutine_handle = std::coroutine_handle<>;

template <typename T> struct SocketOP {
  int error;

  T value;
};

struct ReadWriteEvent : mystd::Delete_Base, public IEvent {
  bool m_is_has_read;

  bool m_is_has_write;

  void_coroutine_handle m_read_handle;

  void_coroutine_handle m_write_handle;

  ReadWriteEvent():m_is_has_read(false), m_is_has_write(false), m_read_handle(), m_write_handle(){

  }

  void SetRead(void_coroutine_handle handle) {
    if (m_is_has_read) {
      Exit("has read not over");
    }

    m_is_has_read = true;
    m_read_handle = handle;
  }

  void SetWrite(void_coroutine_handle handle) {
    if (m_is_has_write) {

      Exit("has write not over");
    }
   
    m_is_has_write = true;
    m_write_handle = handle;
  }

  void Read() {

    if (m_is_has_read) {
      m_is_has_read = false;

      m_read_handle.resume();
    }
    else{
      Print("read event is not set");
    }
  }

  void Write() {
    if (m_is_has_write) {
      m_is_has_write = false;

      m_write_handle.resume();
    }
    else{
      Print("write event is not set");
    }
  }

  void OnEvent(EpollEvent flag) {

    if ((flag & EpollEvent::EPollIn) == EpollEvent::EPollIn) {
        //Print("can read event");
        Read();
    }
    
    
    if ((flag & EpollEvent::EPollOut) == EpollEvent::EPollOut) {
        //Print("can write event");
        Write();
    } 
    
    if((flag & EpollEvent::EPollHup) == EpollEvent::EPollHup){
        Print("hup error");
        Read();
    }
    
    if ((flag & EpollEvent::EPollError) == EpollEvent::EPollError) {
        Print("can error event");
        //Exit("epoll event is error");

        Read();

        Write();
    } 




  }
};

class TcpSocket : public Socket {
  ReadWriteEvent *m_event;
  size_t m_index;
  EpollEvent m_on_run_op;

  void RemoveEvent(EpollEvent mode) {

    if((mode& m_on_run_op) != EpollEvent::None){
      m_on_run_op= (~mode)& m_on_run_op;
      Info::GetMyAsyncData().GetEpoll().Reset(GetHandle(),  m_on_run_op, m_event);
    }

  }

  void SetEvent(EpollEvent mode) {
    if((mode& m_on_run_op) != mode){
      m_on_run_op= mode| m_on_run_op;
      Info::GetMyAsyncData().GetEpoll().Reset(GetHandle(),  m_on_run_op, m_event);
    }
     

  }

public:

  TcpSocket():mystd::TcpSocket(Socket::CreateSocket()){

  }

  TcpSocket(int handle):Socket(handle), m_event(nullptr), m_index(0), m_on_run_op(EpollEvent::None) {
    m_on_run_op = EpollEvent::EPollIn | EpollEvent::EPollOut | EpollEvent::EpollOneShot ;
    auto event = std::make_unique<ReadWriteEvent>();
  
    m_event = event.get();

    m_index = Info::GetMyAsyncData().AddEvent(std::move(event));

    Info::GetMyAsyncData().GetEpoll().Add(
        GetHandle(), m_on_run_op, m_event);
  }

  ~TcpSocket() override {
    Info::GetMyAsyncData().GetEpoll().Remove(GetHandle());
    Info::GetMyAsyncData().DeleteEvent(m_index);
  }
  MyTask<SocketOP<ssize_t>> AsyncRead(char *buffer, size_t size) {
     
    struct awaiter {
      ReadWriteEvent &m_event;
      awaiter(ReadWriteEvent &event) : m_event(event) {}
      bool await_ready() const noexcept { return false; }
      void await_suspend(void_coroutine_handle handle)  {
        //Print("set read handle");
        m_event.SetRead(handle);
      }
      void await_resume() const noexcept {}
    };
    //Print("read await start");
    
    SetEvent(EpollEvent::EPollIn);
    co_await awaiter(*m_event);
    RemoveEvent(EpollEvent::EPollIn);
    //Print("read await end");
    auto count = recv(this->GetHandle(), buffer, size, 0);

    if (count == -1) {
      co_return SocketOP<ssize_t>{errno, count};
    } else {
      co_return SocketOP<ssize_t>{0, count};
    }
  }

  MyTask<SocketOP<ssize_t>> AsyncWrite(char *buffer, size_t size) {
    
    struct awaiter {
      ReadWriteEvent &m_event;
      awaiter(ReadWriteEvent &event) : m_event(event) {}
      bool await_ready() const noexcept { return false; }
      void await_suspend(void_coroutine_handle handle) noexcept {
        //Print("set write handle");
        m_event.SetWrite(handle);
      }
      void await_resume() const noexcept {}
    };
   
    SetEvent(EpollEvent::EPollOut);
    co_await awaiter(*m_event);
    RemoveEvent(EpollEvent::EPollOut);

    auto count = send(this->GetHandle(), buffer, size, 0);

    if (count == -1) {
      co_return SocketOP<ssize_t>{GetSocketError(), count};
    } else {
      co_return SocketOP<ssize_t>{0, count};
    }
  }
  inline static int s_count=0;
  MyTask<SocketOP<int>> AsyncConnect(const IPAddress& ip){
    struct awaiter {
      ReadWriteEvent &m_event;
      awaiter(ReadWriteEvent &event) : m_event(event) {}
      bool await_ready() const noexcept { return false; }
      void await_suspend(void_coroutine_handle handle) noexcept {
       
        m_event.SetWrite(handle);
      }
      void await_resume() const noexcept {}
    };

    auto res = connect(this->GetHandle(), ip.get(), (uint)ip.getlen());

    auto err = errno;

    auto issync = res == 0;
    s_count++;
    Print("start connect", s_count, issync);
    RemoveEvent(EpollEvent::EpollOneShot);
    co_await awaiter(*m_event);
    SetEvent(EpollEvent::EpollOneShot);
    
    s_count--;
    Print("end connect", s_count, issync);
    
    if(res == -1 && err != EINPROGRESS){
      co_return SocketOP<int>{err, 0};
    }
    else{
      co_return SocketOP<int>{0, 0};
    }
    
  }
};

struct AcceptEvent : mystd::Delete_Base, public IEvent {
  bool m_is_has_read;

  void_coroutine_handle m_read_handle;

    AcceptEvent():m_is_has_read(false){}
  void SetRead(void_coroutine_handle handle) {
    if (m_is_has_read) {
      Exit("AcceptEvent has read not over");
    }

 
    m_is_has_read = true;
    m_read_handle = handle;
  }

  void Read() {

    if (m_is_has_read) {
      m_is_has_read = false;

      m_read_handle.resume();
    }
  }

  void OnEvent(EpollEvent flag) {

    if (flag == EpollEvent::EPollIn) {
        //Print("can accept  event");
      Read();
    } else if (flag == EpollEvent::EPollError) {
         //Print("can accept error  event");
      Read();

    } else {
      Exit("AcceptEvent unknow flag");
    }
  }
};

class TcpSocketListen : public Socket {

  AcceptEvent *m_event;
  size_t m_index;

public:
  TcpSocketListen():Socket(Socket::CreateSocket()) {
    
    auto event = std::make_unique<AcceptEvent>();
    m_event = event.get();
    m_index = Info::GetMyAsyncData().AddEvent(std::move(event));
    Info::GetMyAsyncData().GetEpoll().Add(GetHandle(), EpollEvent::EPollIn,
                                          m_event);
  }

  ~TcpSocketListen() override {
    Info::GetMyAsyncData().GetEpoll().Remove(GetHandle());
    Info::GetMyAsyncData().DeleteEvent(m_index);
  }
  void Bind(const IPAddress &ip) {

    int on = 1;
    if(-1==setsockopt(this->GetHandle(), SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))){
      Exit("setsockopt SO_REUSEADDR error");
    }  

    if (bind(this->GetHandle(), ip.get(), (uint)ip.getlen()) == -1) {
      Exit("bind error");
    }
  }

  void Listen(int n) {
    if (listen(this->GetHandle(), n) == -1) {
      Exit("Listen error");
    }
  }

  MyTask<SocketOP<std::shared_ptr<TcpSocket>>> AsyncAccept() {

    struct awaiter {
      AcceptEvent &m_event;
      awaiter(AcceptEvent &event) : m_event(event) {}
      bool await_ready() const noexcept { return false; }
      void await_suspend(void_coroutine_handle handle) noexcept {
        m_event.SetRead(handle);
      }

      void await_resume() const noexcept {}
    };

    co_await awaiter(*m_event);

    auto socket = accept(this->GetHandle(), nullptr, 0);

    if (socket == -1) {
      Exit("Accpet error");
    }

    co_return SocketOP<std::shared_ptr<TcpSocket>>{
        0, std::make_shared<TcpSocket>(socket)};
  }
};

class Dns {

  static auto GetIPv4AddressByName(const std::string &name) {
    addrinfo *res;
    int code = getaddrinfo(name.c_str(), nullptr, nullptr, &res);

    if (code != 0) {
      Exit("GetIPv4AddressByName error");
      // 进程会退出不会往下执行
    }

    auto del = [](auto p) { freeaddrinfo(p); };

    return std::unique_ptr<addrinfo, decltype(del)>{res, del};
  }

public:
  static auto GetIPv4AddressByNameList(const std::string &name) {
    auto result = GetIPv4AddressByName(name);

    std::vector<IPAddress> list{};

    for (auto value = result.get(); value != nullptr; value = value->ai_next) {
      if (value->ai_family == AF_INET) {
        auto item = reinterpret_cast<sockaddr_in *>(value->ai_addr);

        list.push_back(IPAddress{*item});
      }
    }

    return list;
  }

  static bool GetIPv4AddressByNameFirst(const std::string &name, IPAddress &ip) {
    auto result = GetIPv4AddressByName(name);

    for (auto value = result.get(); value != nullptr; value = value->ai_next) {
      if (value->ai_family == AF_INET) {
        auto item = reinterpret_cast<sockaddr_in *>(value->ai_addr);
        Print("set ipadress ref");
        ip = IPAddress{*item};
        return true;
      }
    }

    return false;
  }
};

} // namespace mystd

#endif // _MY_SOCKET