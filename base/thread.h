
#pragma once
#include "countDownLatch.h"
#include "noncopyable.h"
#include <functional>
#include <string>

namespace CurrentThread {
extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char *t_threadName; // 在源文件定义

void cacheTid();
inline int tid() {
	if (__builtin_expect(t_cachedTid == 0, 0)) {
		cacheTid();
	}
	return t_cachedTid;
} // inline函数必须包含定义

} // namespace CurrentThread

class Thread : noncopyable {

  private:
	typedef std::function<void()> ThreadFunc;
	pthread_t pthreadId_;
	pid_t tid_;
	ThreadFunc func_;
	std::string name_;
	CountDownLatch countdownlatch_;

	bool started;
	bool joined;

  public:
	explicit Thread(const ThreadFunc &, const std::string name);
	~Thread();
	void start();
	int join();
};