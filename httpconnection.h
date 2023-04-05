#pragma once // //头文件只被编译一次
#include "channel.h"
#include "epoll.h"
#include "request.h"
#include "response.h"
#include <memory>
#include <unordered_map>

// 两个类不能互相include对方的头文件，两者也不能都是实体对象，必须其中一个为指针(weak_ptr的意思)
// shared_ptr可以实现共同拥有的功能; 需要其中一个创建, 其他的构造函数传入;

// 可以重复声明, 不会产生实际的代码 extern int;
// static定义具有局部特性;
class EventLoop;
class HttpServer;
class TimerNode;
// class Channel;

const int DEFAULT_EXPIRED_TIME =
	2000; // ms 由于网络导致的收到ack很慢或者重传, 不能立马close; close:操作系统回收 发送fin, 等待一段时间自动关闭, shutdown需要对面的fin, tcp的关闭;
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000; // ms

const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;

enum ConnectionState { H_CONNECTED = 0, H_DISCONNECTING, H_DISCONNECTED };
// c++的默认继承是private
// 要加jpublic;

class MimeType {
  private:
	static void init();
	static std::unordered_map<std::string, std::string> mime;
	MimeType();
	MimeType(const MimeType &m);

  public:
	static std::string getMime(const std::string &suffix);

  private:
	static pthread_once_t once_control;
};

class HttpConnection : public std::enable_shared_from_this<HttpConnection> {
  public:
	HttpConnection(std::shared_ptr<EventLoop>, int,
				   HttpServer *); // channel是自己需要创建的变量
	void initConnection();
	// ~HttpConnection(){}; // 声明了析构函数, 会取消默认的析构函数实现; 必须实现; 继承 虚析构函数;

	void handleRead();
	void handleWrite();
	void handleConn();

	Response *analyse(Request); // 可以做成函数回调；

	// 优雅关闭连接
	void handleClose();
	void close();

	int getConnectionfd() { return connectionfd; }

	void bindTimeNode(shared_ptr<TimerNode> timenode) {
		this->nodeptr = timenode;
		// cout << timenode.get() << endl;
	}

	shared_ptr<TimerNode> getTimerNode() {
		// cout << nodeptr.expired() << endl;
		return nodeptr.lock();
	}

  private:
	// 生命周期依赖于自己的使用share. 自己依赖, 只是可能需要用到的 用weak;
	std::weak_ptr<EventLoop> loop;
	int connectionfd;
	std::shared_ptr<Channel> channel;

	std::string inbuff;
	std::string outbuff;

	ConnectionState connectionstate;
	// ProcessState processstate;
	bool error_; // 出错了需要关闭连接, 返回错误页面;

	HttpServer *
		ownerServer; // 如果要用 shared_ptr, 必须保证 使用的对象 已经有了 share_ptr;

	// ResquestInfo req;
	int nevents;

	void errorPage(int code, string desc);

	Request request;

	weak_ptr<TimerNode> nodeptr; // 可能出现循环引用
};