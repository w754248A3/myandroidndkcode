#include "include/my_base.h"
#include "my_socket.h"
#include <cstddef>
#include <cstdio>
#include <memory>




//last也是有效char
bool from_chars(const char *first, const char *last, int32_t &value)
{
    constexpr char map[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    int32_t memory = 0;
    auto end = last + 1;
    for (auto p = first; p != end; p++)
    {
        uint32_t i = *p;

        i -= 48;

        if (i < sizeof(map))
        {
            memory *= 10;

            memory += map[i];
        }
        else
        {
            return false;
        }
    }

    value = memory;
    return true;
}

template <typename T>
const char *Find(const char *first, const char *last, T func)
{
    auto end = last + 1;
    for (auto p = first; p != end; p++)
    {
        if (func(*p))
        {
            return p;
        }
    }

    return nullptr;
}

bool GetHostAndPortFrom(const char *first, const char *last, std::pair<std::string, uint16_t> &value)
{

    auto func = [](char c) { return c == ' '; };

    first = Find(first, last, func);

    if (first != nullptr)
    {
        first++;

        last = Find(first, last, func);

        if (last != nullptr)
        {
            last--;

            auto index = Find(first, last, [](char c) { return c == ':'; });

            if (index != nullptr)
            {

                int32_t port;

                if (from_chars(index + 1, last, port))
                {
                    size_t length = (size_t)(index - first);
                    std::string host{first, length};

                    value = std::make_pair(host, static_cast<uint16_t>(port));

                    return true;
                }
            }
        }
    }

    return false;
}

bool GetIPAddress(const std::string &host, uint16_t port, mystd::IPAddress &ip)
{
    if(mystd::Dns::GetIPv4AddressByNameFirst(host, ip)){
        ip.SetPort(port);
        return true;
    }
    else{
        return false;
    }

}

mystd::MyTask<bool> WriteResponse(std::shared_ptr<mystd::TcpSocket> socket)
{
    char buffer[] = "HTTP/1.1 200 OK\r\n\r\n";
    size_t length = sizeof(buffer) - 1;


    auto res = co_await socket->AsyncWrite(buffer,length);

    co_return res.error==0;
}

mystd::MyTask<bool> ReadRequest(std::shared_ptr<mystd::TcpSocket> socket, mystd::IPAddress& ip){
    
    char buffer[1024];

    auto res = co_await socket->AsyncRead(buffer, sizeof(buffer));

    if(res.error != 0 || res.value==-1){
        co_return false;
    }
    std::pair<std::string, uint16_t> value{};
    if (!GetHostAndPortFrom(buffer, buffer + res.value, value))
    {
        co_return false;
    }
    

    if(GetIPAddress(value.first, value.second, ip)){
        co_return true;
    }
    else{
        co_return false;
    }
}


mystd::MyTask<int> Copy(std::shared_ptr<mystd::TcpSocket> left, std::shared_ptr<mystd::TcpSocket> right){

char buffer[65536];
    while (true) {  
        auto op = co_await left->AsyncRead(buffer, sizeof(buffer));

        if(op.error!=0 || op.value==0){
            co_return 0;


        }
        //mystd::Print("read over loop", op.value);
        op = co_await right->AsyncWrite(buffer,(size_t)op.value);
        if(op.error!=0){
            co_return 0;
        }
         //mystd::Print("write over loop", op.value);

    }
}


mystd::MyTask<int> MyConnect(std::shared_ptr<mystd::TcpSocket> socket){
    mystd::IPAddress ip{0,0,0,0, 0};
    mystd::Print("start ReadRequest");
    auto red = co_await ReadRequest(socket, ip);
    mystd::Print("end ReadRequest");
    if(!red){
        co_return 0;
    }

    auto wri = co_await WriteResponse(socket);

    if(!wri){
        co_return 0;
    }

    
    auto connect = std::make_shared<mystd::TcpSocket>();
    //mystd::Print("start connect");
    auto con = co_await connect->AsyncConnect(ip);
    //mystd::Print("end connect");

    
    if(con.error != 0){
        mystd::Print("connect error");
        co_return 0;
    }

    Copy(socket, connect);
    Copy(connect, socket);

    co_return 0;
}


mystd::MyTask<int> ListenLoop(){
    mystd::TcpSocketListen listen;
    listen.Bind(mystd::IPAddress(0,0,0,0,6785));

    listen.Listen(10);

    while (true) {
        auto op = co_await listen.AsyncAccept();

        mystd::Print("in link");
       MyConnect(op.value);
    }
}


int main(void){


    mystd::Info::GetMyAsyncData().IntoEventLoop([](){

        ListenLoop();


    });
}
