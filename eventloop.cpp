#include "eventloop.h"
#include "base/thread.h"
#include "socket.h"
#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

EventLoop::EventLoop()
	: poller(new Epoll()), inPendingFunctor(false),
	  wakeupfd_(Socket_op::createEventfd()),
	  wakeupChannel(new Channel(wakeupfd_)), isQuit(false), isStart(false),
	  owerThreadId(CurrentThread::tid()), timermanager(new TimerManager()) {
	wakeupChannel->setEvents(EPOLLIN | EPOLLET);
	wakeupChannel->setReadCallBack(
		bind(&EventLoop::wakeupRead, this)); //不需要写和重置;
	addToPoller(wakeupChannel);
}
EventLoop::~EventLoop() {
	// 需要释放需要的资源
	for (auto i : fd2connection) {
		i.second->close();
	}
}

void EventLoop::loop() {
	assert(!isStart);
	assert(!isQuit);
	assert(isInLoopThread());
	isStart = true;
	isQuit = false;
	printf("LOOP in thread %d\n", CurrentThread::tid());
	while (!isQuit) {
		int event_cnt = poller->Wait();
		inPendingFunctor = false;
		// cout << event_cnt << endl;
		for (int i = 0; i < event_cnt; ++i) {
			int fd = poller->GetEventFd(i);
			uint32_t revents = poller->GetEvents(i);
			if (revents & EPOLLHUP && !(revents & EPOLLIN)) {
				// 客户端 发送close, 服务端读取完数据就结束 EPOLLHUP持续触发
				// 关闭双端 不注册到poller 上
				fd2connection[fd]->close();
				return;
			} else if (revents & EPOLLERR) {
				// 出现错误 比如读取到RST的时候 客户端已经close, 发送数据, 就会RST
				// close时，如果接收缓冲区还有数据未read到应用层，则不会走四次挥手流程，直接发RST包
				// 如果在收到RST包后，又向对端发送数据，会收到sigpipe异常
				if (errorHandler)
					errorHandler();
				fprintf(stdin, "[INFO] client call close\n");
				fd2connection[fd]->close();
				return;
			} else if (revents & (EPOLLRDHUP | EPOLLIN | EPOLLPRI)) {
				// 客户端会尽力发送 本机内的 out-buffer的数据, 再发送fin; 不会管in-buffer是否存在数据.
				// 如果是shutdown的客户端, 就会尝试发送完服务器端的数据, 关闭连接.
				fd2channel[fd]->handleRead();
			} else if (revents & EPOLLOUT) {
				fd2channel[fd]->handleWrite();
			} else {
				// 未定义的poll事件
				std::cout << revents << endl;
			}
			fd2channel[fd]->handleConn();
		}

		doPendingFunctors();
		timermanager->handleExpiredEvent();
		// cout << "aaa" << endl;
	}
	isStart = false;
}

void EventLoop::doPendingFunctors() {
	std::vector<Functor> functors;
	{
		MutexLockGuard guard(mutex);
		functors.swap(pendingFunctors_);
	}
	inPendingFunctor = true;

	for (size_t i = 0; i < functors.size(); ++i) {
		functors[i]();
	}
}

void EventLoop::queueInLoop(Functor &&func) {

	// cout << &mutex << endl;
	{
		// mutex.lock();
		MutexLockGuard guard(mutex);
		pendingFunctors_.push_back(std::move(func));
	}
	if (!isInLoopThread() || inPendingFunctor) {

		wakeup();
	}
}

void EventLoop::runInLoop(Functor &&cb) {
	if (isInLoopThread()) {
		cb();
	} else {
		queueInLoop(std::move(cb));
	}
}

void EventLoop::wakeup() {
	// cout << wakeupfd_ << endl;
	int64_t one = 1;
	ssize_t n = write(wakeupfd_, &one, sizeof(one));
	if (n == -1 && errno != EAGAIN) {
		perror("Wake up fail for");
	}
	// 如果写入错误, 说明已经慢了
	// 如果其他的错误, 暂时不考虑;
}

bool EventLoop::isInLoopThread() {
	return this->owerThreadId == CurrentThread::tid();
}

void EventLoop::quit() {
	this->isQuit = false;
	if (!isInLoopThread())
		wakeup();
}

void EventLoop::wakeupRead() {
	// cout << "aa" << endl;
	uint64_t count = 1;
	read(wakeupfd_, (void *)count, sizeof(count));
}