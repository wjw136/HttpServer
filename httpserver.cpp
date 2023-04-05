#include "httpserver.h"
#include "httpconnection.h"
#include "socket.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

HttpServer::HttpServer(int port, int threadNum, EventLoop *loop)
	: mainloop(loop), port(port), threadnum(threadNum), loopPool(threadNum) {
	int listen_fd = socket_bind_listen(port);
	assert(listen_fd != -1);
	assert(Socket_op::setSocketNonBlocking(listen_fd) == 0);
	accept_channel.reset(new Channel(
		listen_fd)); // unique_ptr没有拷贝赋值的函数(其他的智能指针);
	// 只能reset和swap和release
	// 最好是在初始化列表中
	this->listenfd_ = listen_fd;
	cout << "listen port: " << port << endl;
}

void HttpServer::start() {
	// TODO 线程池的启动
	// cout << listenfd_ << endl;
	loopPool.start();
	accept_channel->setEvents(EPOLLIN | EPOLLET);
	accept_channel->setReadCallBack(
		bind(&HttpServer::handleNewConnection, this));
	// cout << "aaaa" << endl;
	mainloop->addToPoller(accept_channel);

	// cout<<listenfd_.
	// cout << "aaaa" << endl;
}

void HttpServer::handleNewConnection() {
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	int accept_fd = 0;
	socklen_t client_addr_len = sizeof(client_addr);

	/*
	文件描述符的操作的返回值:
	不管阻塞还是非阻塞, 如果返回的是>0, 是操作的字节数; 如果是0, 文件末尾或关闭连接; 如果-1, 出现错误, EAGAIN(resource not available) || EWOULDBLOCK || EINTR 需要重试;
	EAGIN是非阻塞, EINTER是阻塞;
	connect遇到EINTER的时候 EINPROGRESS(非阻塞), 不能重试, 要select文件描述符(poll(结构体数组, len, timeout))来关注写事件, 再用getsockopt(SO_ERROR)判断是否!=0(有错误)
	因为fd的connect的连接成功和失败都会让fd可写.
  */
	while ((accept_fd =
				accept(accept_channel->getFd(), (struct sockaddr *)&client_addr,
					   &client_addr_len)) > 0) {
		// edge trigger 接受所有的连接.
		cout << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"
			 << ntohs(client_addr.sin_port) << endl;
		// perror("hello");
		if (accept_fd >= MAX_CON) {
			perror("Too much connection from single server! ");
			close(accept_fd);
			return;
		}

		Socket_op::setSocketNodelay(accept_fd);
		Socket_op::setSocketNonBlocking(accept_fd);

		// close(accept_fd);
		// 如果是统一用shared_ptr的时候 可以share_from_this; + weak_ptr
		// 否则传递裸指针, 手动管理.
		std::shared_ptr<HttpConnection> httpconnection(
			new HttpConnection(loopPool.getNextLoop(), accept_fd, this));
		httpconnection->initConnection();

		fd2hp[accept_fd] = httpconnection;

		// httpconnection->handleClose();

		fprintf(stdout, "[INFO] existing connection %d\n", this->getConnNum());
	}
	// char *error = strerror(errno);
	// cout << error << endl;
	// cout << accept_fd << endl;
}

int HttpServer::socket_bind_listen(int port) {
	assert(port > 0 && port <= 65525);

	int listen_fd = 0;
	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return -1;

	// time_wait的时候可以立马 复用端口;
	// 多播系统的 重复绑定
	int optval = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
				   sizeof(optval)) == -1) {
		close(listen_fd);
		return -1;
	}

	struct sockaddr_in server_addr;
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 多个网卡的端口
	server_addr.sin_port = htons((uint16_t)port);
	if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
		-1) {
		close(listen_fd);
		return -1;
	}

	// 等待的队列;  SYN_RECV (客户端的SYN_SEND)
	if (listen(listen_fd, 2048) == -1) {
		close(listen_fd);
		return -1;
	}

	if (listen_fd == -1) {
		close(listen_fd);
		return -1;
	}

	return listen_fd;
}
