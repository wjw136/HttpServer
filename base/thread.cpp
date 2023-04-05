#include "thread.h"
#include <assert.h>
#include <linux/unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace CurrentThread {
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char *t_threadName = "default";

pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

void cacheTid() {
	if (t_cachedTid == 0) {
		t_cachedTid = gettid();
		t_tidStringLength =
			snprintf(t_tidString, sizeof(t_tidString), "%5d", t_cachedTid);
	}
}
} // namespace CurrentThread

struct ThreadData {
	CountDownLatch *latch;
	pid_t *tid;
	typedef std::function<void()> ThreadFunc;
	ThreadFunc func;
	std::string name;

	ThreadData(CountDownLatch *const latch_, pid_t *const tid_,
			   const ThreadFunc &func_, const std::string &name_)
		: latch(latch_), tid(tid_), func(func_), name(name_) {}

	void runInThread() {
		*tid = CurrentThread::tid();
		tid = nullptr;
		CurrentThread::t_threadName = name.empty() ? "Thread" : name.c_str();
		prctl(PR_SET_NAME, CurrentThread::t_threadName);
		latch->countDown();
		latch = nullptr;

		func();
		CurrentThread::t_threadName = "finished";
	}
};

Thread::Thread(const ThreadFunc &func, const std::string name)
	: func_(func), name_(name), countdownlatch_(1) {
	started = false;
	joined = false;
	pthreadId_ = 0;
	tid_ = 0;
	if (name.empty()) {
		char buf[32];
		snprintf(buf, sizeof(buf), "Thread"); // 写入string, 固定长度;
		name_ = buf;
	}
}

void *startHelper(void *data) {
	ThreadData *data_ = static_cast<ThreadData *>(data);
	data_->runInThread();
	delete data_; // 清理data; 堆上的内存;
	return NULL;
}

void Thread::start() {
	assert(!started);
	started = true;

	ThreadData *data = new ThreadData(
		&this->countdownlatch_, &this->tid_, this->func_,
		this->name_); //data 需要在start的之后 也需要存活, 需要在堆上分配.
	// 指针对象是空的 NULL , 和指针指向空的对象nullptr
	if (pthread_create(&this->pthreadId_, NULL, &startHelper, data)) {
		delete data;
		started = false;
	} else {
		countdownlatch_.wait();
		assert(tid_ > 0);
	}
}

int Thread::join() {
	// 使用状态机;
	assert(started);
	assert(!joined);
	joined = true;
	return pthread_join(this->pthreadId_, NULL);
}

Thread::~Thread() {
	if (started && !joined) {
		// 析构的时候, 启动的线程但是没有joined需要 detach; 暂时没有closed的状态
		pthread_detach(this->pthreadId_); //线程运行完, 回收资源
	}
}