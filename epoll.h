#pragma once
#include <assert.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
using namespace std;

class Epoll {
  public:
	Epoll(int maxEvent = 20000);
	~Epoll();

	bool Addfd(int fd, uint32_t events);
	bool Modfd(int fd, uint32_t events);
	bool Delfd(int fd);

	int Wait(int timeoutMs = -1);

	int GetEventFd(int i) const;

	uint32_t GetEvents(int t) const;

  private:
	int epollFd;

	vector<epoll_event> events;
};