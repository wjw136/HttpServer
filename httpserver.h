#pragma once
#include "base/loopThreadPool.h"
#include "channel.h"
#include "eventloop.h"
#include "httpconnection.h"
#include <unordered_map>

class HttpServer {
  public:
	HttpServer(int port, int threadNum, EventLoop *loop);
	// 指针需要初始化, 但是对象不需要初始化,
	// 自动调用无参构造函数来初始化;
	~HttpServer(){
		// 需要停止 所有的server的资源; 和 stop一样;
	};

	void del_connection(int fd) {
		fd2hp.erase(fd);
		// cout << fd2hp.size() << endl;
	}

	void start();
	void handleNewConnection();
	// TODO
	void stop();
	int getConnNum() { return fd2hp.size(); }

  private:
	unique_ptr<EventLoop> mainloop; // 对象自动析构 RAII; 释放指针自动;
	shared_ptr<Channel> accept_channel;
	int port;
	int listenfd_; // 不负责
	static const int MAX_CON = 500;

	int threadnum;
	// TODO: 线程池
	LoopThreadPool loopPool;

	// server 和time, poll 共同持有 HTTP connection;
	typedef shared_ptr<HttpConnection> http_ptr;
	unordered_map<int, http_ptr> fd2hp;

	int socket_bind_listen(int port);
};