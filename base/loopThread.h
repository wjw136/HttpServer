#pragma once
#include "../eventloop.h"
#include "noncopyable.h"
#include "thread.h"

class LoopThread : noncopyable {
  public:
	LoopThread(string loopName_)
		: loopName(loopName_), loop(nullptr), latch(1),
		  thread(bind(&LoopThread::threadFunc, this), loopName){};
	~LoopThread() {
		loop->quit(); // 防止线程detach之后始终运行;
	};
	void start() {
		thread.start();
		latch.wait();
	};
	// void loopStarter_wrapper() { loop.loop(); }
	shared_ptr<EventLoop> getLoop() {
		latch.wait();
		return loop;
	}

	void threadFunc() {
		this->loop.reset(new EventLoop());
		latch.countDown(); //等待 loop初始化

		loop->loop();
	}

  private:
	// 和列表初始化的依赖顺序一样;
	string loopName;
	shared_ptr<EventLoop> loop;
	Thread thread;
	CountDownLatch latch;

	void stop(); //TODO: 一次性的 不可重开的 thread; 修改状态机;
};