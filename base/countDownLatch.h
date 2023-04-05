// TODO

#pragma once
#include "condition.h"
#include "mutexLock.h"

class CountDownLatch : noncopyable {
  public:
	explicit CountDownLatch(int count);
	void wait();
	void countDown();

  private:
	MutexLock mutex_;
	Condition condition_;
	int count_;
};