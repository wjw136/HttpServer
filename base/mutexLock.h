#pragma once
#include "noncopyable.h"
#include <cstdio>
#include <iostream>
#include <pthread.h>

class MutexLock : noncopyable {
  public:
	MutexLock() { pthread_mutex_init(&mutex, NULL); }
	~MutexLock() {
		pthread_mutex_lock(&mutex);
		pthread_mutex_destroy(&mutex);
	}
	void lock() {
		// std::cout << &mutex << std::endl;
		pthread_mutex_lock(&mutex);
	}
	void unlock() { pthread_mutex_unlock(&mutex); }
	pthread_mutex_t *get() { return &mutex; }

  private:
	pthread_mutex_t mutex;
	friend class
		Condition; // 不一定需要声明; 类的成员变量, 需要声明和展开类的折叠 变成基本类型 所以需要前向声明和指针;
};

class MutexLockGuard : noncopyable {
  public:
	// 默认构造函数
	explicit MutexLockGuard(MutexLock &mutex_) : mutex(mutex_) { mutex.lock(); }
	~MutexLockGuard() { mutex.unlock(); }

  private:
	MutexLock &mutex; //是个引用, 不会拷贝
};