#include "epoll.h"
#include <sys/epoll.h>

// 512是epoll可以监听的数量
// 一次返回的事件数量
Epoll::Epoll(int maxEvent) : epollFd(epoll_create(512)), events(maxEvent)
{
    assert(epollFd > 0 && events.size() > 0);
}


Epoll::~Epoll()
{
    close(epollFd);
}

bool Epoll::Addfd(int fd, uint32_t events)
{
    if (fd < 0)
        return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool Epoll ::Modfd(int fd, uint32_t events)
{
    if (fd < 0)
        return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev) == 0;
}

bool Epoll::Delfd(int fd)
{
    if (fd < 0)
        return false;
    epoll_event ev = {0};
    return epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
}

int Epoll::Wait(int timeout)
{
    return epoll_wait(epollFd, &events[0], events.size(), timeout);
}

int Epoll::GetEventFd(int i) const 
{
    assert(i < events.size() && i >= 0);
    return events[i].data.fd;
}

uint32_t Epoll::GetEvents(int i) const 
{
    assert(i < events.size() && i >= 0);
    return events[i].events;
}
