#pragma once
#include "base/mutexLock.h"
#include "base/thread.h"
#include "channel.h"
#include "epoll.h"
#include "httpconnection.h"
#include "timer.h"
#include <functional>
#include <iostream>
#include <map>
#include <memory>

using namespace std;

class EventLoop {
  public:
	typedef function<void()> Functor;
	typedef shared_ptr<Channel> channel_PTR;
	typedef shared_ptr<HttpConnection> connection_PTR;

	EventLoop();
	~EventLoop();

	void loop();

	void addToPoller(shared_ptr<Channel> channel) {
		// cout << "ddedd" << endl;
		this->fd2channel[channel->getFd()] = channel; // 本身也要记录, 来回调
		// cout << "aaa" << endl;
		this->fd2connection[channel->getFd()] = channel->getHttpContext();
		// cout << channel->getHttpContext() << endl;
		poller->Addfd(channel->getFd(), channel->getEvents());
	}

	void updatePoller(shared_ptr<Channel> channel) {
		// 已经有了fd;
		poller->Modfd(channel->getFd(), channel->getEvents());
	}

	void runInLoop(Functor &&cb);
	void queueInLoop(Functor &&cb);

	void quit();
	// bool isInloopTread() const { return }
	MutexLock mutex;

	void deletefd(int fd) {
		// 取消 epoll对于事件fd的监听.
		this->poller->Delfd(fd);
		fd2channel[fd].reset();
		fd2channel.erase(fd);
		fd2connection[fd].reset();
		fd2connection.erase(fd);
	}

	void setErrorHandler(Functor fr) { this->errorHandler = fr; }

	void addTimerNode(shared_ptr<HttpConnection> shtp, int time) {
		// std::cout << "cccdddss" << std::endl;
		this->timermanager->addTimer(shtp, time);
	}

	void addPollerAndTimer(shared_ptr<HttpConnection> shtp, int time,
						   shared_ptr<Channel> channel) {
		this->addToPoller(channel);
		this->addTimerNode(shtp, time);
	}

	void updateTimerNode(shared_ptr<HttpConnection> htp, int time) {
		this->timermanager->updateTimer(htp, time);
	}

  private:
	// const pid_t threadId;
	unique_ptr<Epoll> poller;

	map<int, channel_PTR> fd2channel;
	map<int, connection_PTR> fd2connection;

	std::vector<Functor>
		pendingFunctors_; //多线程能修改, 需要注意; 多线程能读取, 不需要注意;
	int wakeupfd_;
	shared_ptr<Channel> wakeupChannel;
	// 使类的const函数可以修改对象的mutable数据成员。or 常量对象修改;
	// MutexLock mutex;

	bool inPendingFunctor;

	void doPendingFunctors();
	void wakeup();
	void wakeupRead();
	bool isInLoopThread();

	bool isQuit;
	bool isStart;

	int owerThreadId;

	Functor errorHandler;

	shared_ptr<TimerManager> timermanager;
};

/*
静态变量 不管是类的, 函数还是类的, 全局的, 都在静态区存储, 再编译的时候就分配好内存; (静态: 生存期; 全局: 使用范围)
把局部变量改变为静态变量后是改变了它的存储方式，即改变了它的生存期。
把全局变量改变为静态变量后是改变了它的作用域，限制了它的使用范围。

栈上变量: 编译的时候生成 分配内存的指令
堆上变量: 运行的时候, 有代码分配内存;

类对象的大小: 成员变量 + 虚函数指针; 在栈上; 类本身只是一种code(需要知道成员的大小), 会维护虚函数指针和表;
前向声明: 为了编译的时候需要的大小, 确定符号表;

编译只看符号表; 运行是具体分配内存, 运算;
普通变量的声明: 加入符号表
类的声明: 加入符号表, 确定对象大小(因为需要确定编译的分配内存的指令)
*/