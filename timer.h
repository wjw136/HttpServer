#pragma once
#include "httpconnection.h"
#include <memory>
#include <queue>

class TimerNode {
  private:
	bool isDelete; //主动删除, 不用等待时间;
	size_t expireTime; // 两种时间 keepalive or not
	shared_ptr<HttpConnection> httpConnPtr;

  public:
	TimerNode(shared_ptr<HttpConnection> httpconn, int timeout);
	~TimerNode();

	void update(int timeout); // not to use
	bool isvalid();

	void setDeleted() { isDelete = true; }
	bool isDeleted() const { return isDelete; }
	size_t getExpTime() const { return expireTime; }

	void unlink() {
		// printf("bbbb\n");
		// printf("aaaaa: %ld\n", httpConnPtr.use_count());
		httpConnPtr.reset();
	}
};

struct TimerCmp {
	bool operator()(shared_ptr<TimerNode> node1, shared_ptr<TimerNode> node2) {
		return node1->getExpTime() > node2->getExpTime();
	}
};

class TimerManager {
  private:
	typedef shared_ptr<TimerNode> TimerNodePtr;
	// 只在 最开始的时候确定 使用的位置, 之后通过指针修改 不会改变位置;
	std::priority_queue<TimerNodePtr, std::deque<TimerNodePtr>, TimerCmp> pq;

  public:
	TimerManager();
	~TimerManager();
	void addTimer(std::shared_ptr<HttpConnection>, int);
	void updateTimer(std::shared_ptr<HttpConnection>, int);
	void handleExpiredEvent();
};