#include "loopThreadPool.h"

void LoopThreadPool::start() {
	for (int i = 0; i < numThread; ++i) {
		// cout << i << endl;
		char buff[100];
		snprintf(buff, sizeof(buff), "Thread Pool's thread %dth", i);
		shared_ptr<LoopThread> t(new LoopThread(buff));
		this->threads.push_back(t);
		t->start(); //先start才能有有效的loop
		this->loops.push_back(t->getLoop());
	}
}

shared_ptr<EventLoop> LoopThreadPool::getNextLoop() {
	shared_ptr<EventLoop> tmp = loops[index];
	// cout << &(tmp->mutex) << endl;
	index = (index + 1) % numThread;
	return tmp;
}