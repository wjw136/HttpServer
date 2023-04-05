#include "timer.h"
#include <sys/time.h>

TimerNode::TimerNode(shared_ptr<HttpConnection> httpconn, int timeout)
	: httpConnPtr(httpconn), isDelete(false) {
	// isDelete 等所有的数据成员 都必须 初始花。
	struct timeval now;
	gettimeofday(&now, NULL);
	expireTime = ((now.tv_sec % 10000) * 1000 + now.tv_usec / 1000) + timeout;
	// cout << expireTime << endl;
}

TimerNode::~TimerNode() {
	// cout << "aaa" << endl;

	// cout << expireTime << endl;
	if (this->httpConnPtr)
		this->httpConnPtr->handleClose();
}

void TimerNode::update(int timeout) {
	struct timeval now;
	gettimeofday(&now, NULL);
	expireTime = ((now.tv_sec % 10000) * 1000 + now.tv_usec / 1000) + timeout;
}

bool TimerNode::isvalid() {
	struct timeval now;
	gettimeofday(&now, NULL);
	size_t nowTime = ((now.tv_sec % 10000) * 1000 + now.tv_usec / 1000);

	// cout << nowTime << endl;
	// cout << expireTime << endl;

	return nowTime < expireTime;
}

TimerManager::TimerManager() {}

void TimerManager::addTimer(std::shared_ptr<HttpConnection> httpconn,
							int timeout) {
	TimerNodePtr new_node(new TimerNode(httpconn, timeout));
	// cout << new_node.get() << endl;
	pq.push(new_node);
	httpconn->bindTimeNode(new_node);
}

void TimerManager::updateTimer(std::shared_ptr<HttpConnection> htp, int time) {
	htp->getTimerNode()->unlink();
	addTimer(htp, time);
}

TimerManager::~TimerManager() {}

/*
 优先队列不支持随机访问和修改;
*/
void TimerManager::handleExpiredEvent() {
	// cout << pq.size() << endl;

	while (!pq.empty()) {
		TimerNodePtr timenode = pq.top();
		if (timenode->isDeleted()) {
			pq.pop();
		} else if (!timenode->isvalid()) {
			pq.pop();
		} else {
			break;
		}
	}
}
