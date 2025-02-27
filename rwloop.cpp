#include "my_socket.h"
#include <cstddef>
#include <cstdio>
#include <memory>


mystd::MyTask<int> ReadWriteLoop(std::shared_ptr<mystd::TcpSocket> handle){
    char buffer[65536];
    while (true) {  
        auto op = co_await handle->AsyncRead(buffer, sizeof(buffer));

        if(op.error!=0 || op.value==0){
            co_return 0;


        }
        //mystd::Print("read over loop", op.value);
        op = co_await handle->AsyncWrite(buffer,(size_t)op.value);
        if(op.error!=0){
            co_return 0;
        }
         //mystd::Print("write over loop", op.value);

    }
}


mystd::MyTask<int> ListenLoop(){
    mystd::TcpSocketListen listen;
    listen.Bind(mystd::IPAddress(0,0,0,0,8086));

    listen.Listen(10);

    while (true) {
        auto op = co_await listen.AsyncAccept();

        
        ReadWriteLoop(op.value);
    }
}


int main(void){


    mystd::Info::GetMyAsyncData().IntoEventLoop([](){

        ListenLoop();


    });
}
