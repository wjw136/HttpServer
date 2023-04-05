#pragma once
#include "../eventloop.h"
#include "loopThread.h"
#include "noncopyable.h"
#include "thread.h"
#include <memory>
#include <vector>

class LoopThreadPool : noncopyable {
  private:
	int numThread;
	std::vector<shared_ptr<LoopThread>> threads;
	std::vector<shared_ptr<EventLoop>> loops;
	int index;

  public:
	LoopThreadPool(int numThread_) : numThread(numThread_), index(0) {
		if (numThread_ <= 0) {
			perror("num in loopThreadPool fail!");
			abort(); // 不会回收; exit不会回收局部对象; 异常;
		}
	};
	~LoopThreadPool() { std::cout << "end of eventlooppool" << endl; };
	void start();
	shared_ptr<EventLoop> getNextLoop();
};