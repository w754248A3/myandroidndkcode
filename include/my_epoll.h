
#ifndef _MY_EPOLL
#define _MY_EPOLL 1

#include "my_base.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <array>

namespace mystd
{

    enum class EpollMod
    {
        Add = EPOLL_CTL_ADD,
        Reset = EPOLL_CTL_MOD,
        Delete = EPOLL_CTL_DEL
    };

    enum class EpollEvent
    {
        EPollIn = EPOLLIN,

        EPollOut = EPOLLOUT,

        EPollError = EPOLLERR, //始终监听

        //EPollEdgeTriggered = static_cast<int32_t>(EPOLLET),

        EPollHup = EPOLLHUP, //对等端写入完毕，始终监听

        EPollRdhup = EPOLLRDHUP,

        EPollPri = EPOLLPRI, //特殊情况，外代数据

        EpollOneShot = EPOLLONESHOT

    };

    class Epoll : mystd::Delete_Base
    {

        int m_handle;

        void Set(EpollMod mod, int handle, EpollEvent epollEvent, void *ptr) noexcept
        {
            epoll_event event = {};

            event.events = static_cast<decltype(event.events)>(epollEvent);

            event.data.ptr = ptr;

            if (-1 == epoll_ctl(m_handle, static_cast<int>(mod), handle, &event))
            {
                Exit("set epoll error");
            }
        }

    public:
        Epoll() noexcept
        {
            m_handle = epoll_create1(EPOLL_CLOEXEC);

            if (m_handle == -1)
            {
                Exit("create epoll error");
            }
        }

        void Add(int handle, EpollEvent epollEvent, void *ptr) noexcept
        {
            Set(EpollMod::Add, handle, epollEvent, ptr);
        }

        void Remove(int handle) noexcept
        {
            if (-1 == epoll_ctl(m_handle, EPOLL_CTL_DEL, handle, nullptr))
            {

                Exit("remove epoll error");
            }
        }

        void Reset(int handle, EpollEvent epollEvent, void *ptr) noexcept
        {
            Set(EpollMod::Reset, handle, epollEvent, ptr);
        }

        template <std::size_t TSIZE>
        int Wait(std::array<epoll_event, TSIZE> &events) noexcept
        {

            auto value = epoll_wait(m_handle, events.data(), (int)events.size(), -1);

            if (value == -1)
            {
                Exit("wait epoll error");
                return 0;
            }
            else
            {
                return value;
            }
        }

        ~Epoll()
        {
            if (-1 == close(m_handle))
            {
                Exit("close Epoll error");
            }
        }
    };
} // namespace mystd

#endif