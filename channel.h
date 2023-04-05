#pragma once
// #include "httpconnection.h"
#include <functional>
#include <iostream>
#include <memory>
#include <unistd.h>

class HttpConnection;

class Channel {
  public:
	typedef std::function<void()> functionCallback;

	Channel(int epollFd_, functionCallback readCallBack_,
			functionCallback writeCallBack_)
		: readCallBack(readCallBack_), writeCallBack(writeCallBack_),
		  epollFd(epollFd_) {}
	Channel(int epollFd_) : epollFd(epollFd_) {}
	~Channel() {
		fprintf(stdout, "close fd %d\n", epollFd);
		// fflush(stdout); 没有换行的时候.
		close(epollFd);
	}; // channel 对fd负责;

	void handleRead() { this->readCallBack(); }
	void handleConn() {
		// 有可能没有更新回调
		if (connCallBack)
			this->connCallBack();
	}
	void handleWrite() { this->writeCallBack(); }

	void setReadCallBack(functionCallback &&readCallBack_) {
		this->readCallBack =
			std::move(readCallBack_); // 这里会调用移动构造函数;
		// 不具名的右值引用; 将亡值 返回类型是T&&的函数或者表达式, 都是右值,
		// static_cast的原理;

		// 具名的右值引用是左值; (需要move)
		// 一个值 可能有左值引用和右值引用;
	}
	void setWriteCallBack(functionCallback &&writeCallBack) {
		this->writeCallBack = std::move(writeCallBack);
	}
	void setConnCallback(functionCallback &&connCallBack) {
		this->connCallBack = std::move(connCallBack);
	}

	int getFd() { return epollFd; }

	void setEvents(__uint32_t ev) { events_ = ev; }
	uint32_t getEvents() { return events_; }

	void setHttpContext(std::shared_ptr<HttpConnection> tmp) {
		http_context = tmp;
	}

	// weak_ptr是持有的时候, 使用的时候要用(返回) shared_ptr;
	std::shared_ptr<HttpConnection> getHttpContext() {
		return http_context.lock();
	}

  private:
	functionCallback readCallBack;
	functionCallback writeCallBack;
	functionCallback connCallBack; // 读, 写, 重置

	int epollFd;

	__uint32_t events_;
	__uint32_t revents_;

	std::weak_ptr<HttpConnection>
		http_context; // 只有属于http的channel才有httpconext; 有一些功能性的channel没有contex, TODO: 可以用继承取代;
	// 空悬和野指针 智能指针自动置NULL.
	// 清空指针 delete 和 置空
	// 空指针 空对象 NULL
};