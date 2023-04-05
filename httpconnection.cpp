#include "httpconnection.h"
#include "eventloop.h" // 没有循环引用的问题
#include "eventloop.h" // 实现必须引用前向引用的实现.
#include "httpserver.h"
#include "socket.h"
#include "timer.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

HttpConnection::HttpConnection(std::shared_ptr<EventLoop> loop_, int fd,
							   HttpServer *server)
	: loop(loop_), connectionfd(fd), channel(new Channel(connectionfd)),
	  ownerServer(server), nevents(0), request(STATE_PARSE_URI) {
	this->connectionstate = H_DISCONNECTED;
	// this->processstate = STATE_PARSE_URI;
}

void HttpConnection::initConnection() {

	channel->setHttpContext(
		shared_from_this()); // share_from_this 不能在构造函数; 需要初始化 shared_ptr(enable_shared_from_this, class A, shared_ptr); shared_ptr为enablexxx赋值
	channel->setEvents(DEFAULT_EVENT); //设置channel的事件;
	channel->setReadCallBack(std::bind(&HttpConnection::handleRead, this));
	channel->setConnCallback(std::bind(&HttpConnection::handleConn, this));
	channel->setWriteCallBack(std::bind(&HttpConnection::handleWrite, this));

	// 绑定channel的事件
	// 加入监听channel;
	std::shared_ptr<EventLoop> sloop(loop.lock());
	// cout << "aa" << endl;
	// sloop->queueInLoop(
	// 	std::bind(&EventLoop::addToPoller, loop.lock(), channel));
	//! 合并在一起初始化 否则 addtimenode 下一轮执行， 会造成timenode的未初始化。
	sloop->queueInLoop(std::bind(&EventLoop::addPollerAndTimer, loop.lock(),
								 shared_from_this(), DEFAULT_EXPIRED_TIME,
								 channel));
	this->connectionstate = H_CONNECTED;
	fprintf(stdout, "\tconnection initial for fd %d\n", this->connectionfd);
	// sloop->addTimerNode(shared_from_this(), DEFAULT_EXPIRED_TIME);
	// sloop->queueInLoop(std::bind(&EventLoop::addTimerNode, loop.lock(),
	// 							 shared_from_this(), DEFAULT_EXPIRED_TIME));
}

void HttpConnection::handleRead() {
	// cout << "aaa" << endl;
	//读取数据
	bool zero = false;
	int read_num = Socket_op::readn(this->connectionfd, inbuff, zero);

	// cout << read_num << zero << endl;
	if (read_num < 0) {
		perror("read error");
		error_ = true;
		errorPage(400, "Bad Request"); // http层处理错误
	} else if (zero) {
		this->connectionstate = H_DISCONNECTING;
	} else {
		// 处理读取的数据
		cout << "rev req: " << this->inbuff << endl;
		// errorPage(200, "Test"); // http层处理错误
		// inbuff.clear();
		// this->close();
		// return;
		this->request.process(inbuff);
		// cout << "aaa" << request.getValid() << endl;
		if (!request.getValid()) {
			error_ = true;
			errorPage(400, "Bad Request (error when parsing)...");
		}

		Response *res;
		if ((res = analyse(request))) {
			// cout << res->make_response() << endl;
			outbuff += res->make_response();
			delete res;
		} else {
			error_ = true;
			errorPage(404, "Bad Request (error when response)...");
		}
	}

	if (!error_) {
		// edge trigger 每次先尝试写 再注册事件; level 先注册事件, 在写;
		if (outbuff.size() > 0) {
			handleWrite();
		}
		// cout << this->request.getState() << endl;
		if (this->request.getState() == STATE_FINISH) {
			// 清空request; 可能的话 继续读;
			request.reset();
			if (inbuff.size() > 0) {
				handleRead();
			}

		} else {
			// cout << nevents << endl;
			nevents |= EPOLLIN;
		}
	}
}

void HttpConnection::handleConn() { //注册的事件会失效
	// 如果是 HTTP1.1 keepalive,需要超时. 非alive 需要主动关闭;
	// HTTP 1.0 需要每次客户端会用新连接, 需要服务端关闭, 要等一段时间, 防止拥塞;
	// 浏览器 不会发送 close
	// 非浏览器, 只用考虑被动关闭+超时;)
	if (!error_) {
		if (connectionstate == H_CONNECTED) {
			// 服务端都是 保持连接;
			// cout << "处理正常的继续连接" << endl;
			if (request.getHeader("Connection") == "Keep-Alive" ||
				request.getHeader("Connection") == "Keep-alive") {
				cout << "keepa alive" << endl;
				loop.lock()->updateTimerNode(shared_from_this(),
											 DEFAULT_KEEP_ALIVE_TIME);
			} else {
				// cout << "aaa" << endl;
				loop.lock()->updateTimerNode(shared_from_this(),
											 DEFAULT_EXPIRED_TIME);
			}
			// cout << nevents << endl;
			if ((nevents & EPOLLIN && nevents & EPOLLOUT)) {
				// 优先读取 防止太多的待消费数据
				nevents = (0 | EPOLLOUT);
			}
		} else if (connectionstate == H_DISCONNECTING) {
			//简化客户端的关闭=> shutdown 和 close的时候, 都尝试发送数据, 关闭连接.
			if (outbuff.size() > 0) {
				nevents |= EPOLLOUT;
				loop.lock()->updateTimerNode(shared_from_this(),
											 DEFAULT_EXPIRED_TIME);
			} else {
				// cout << "关闭连接1" << endl;
				this->close();
				return;
			}
		}
	} else {
		// cout << "关闭连接2" << endl;
		this->close();
		return;
	}
	// cout << nevents << endl;
	this->channel->setEvents(nevents | EPOLLET);
	loop.lock()->updatePoller(this->channel);
	nevents = 0;
}

void HttpConnection::close() {
	shared_ptr<TimerNode> stn(this->nodeptr.lock());
	stn->setDeleted();
	this->nodeptr.reset();
}

void HttpConnection::handleClose() {
	// 优雅关闭 连接;
	// cout << "aaa" << endl;
	fprintf(stdout, "关闭连接: %d\n", this->connectionfd);
	// cout << "aaa" << endl;
	shared_ptr<EventLoop> sloop(loop.lock());
	sloop->queueInLoop(std::bind(
		&EventLoop::deletefd, sloop,
		this->getConnectionfd())); // 多线程 需要 统一处理, 保证安全性, 无锁的安全队列;
	ownerServer->del_connection(this->getConnectionfd());
	// cout << "aaa" << endl;
}

void HttpConnection::errorPage(int code, string desc) {
	string header, body;
	body += "<html><title>Erro Page</title>";
	body += "<body bgcolor=\"ffffff\">";
	body += to_string(code) + desc;
	body += "<hr><em> William Http Sever</em>\n</body></html>";

	header += "HTTP/1.1 " + to_string(code) + " " + desc + "\r\n";
	header += "Content-Type: text/html\r\n";
	header += "Connection: Close\r\n";
	header += "Content-length: " + to_string(body.size()) + "\r\n";
	header += "Server: william Server\r\n";
	header += "\r\n";

	// sprintf(char *__restrict s, const char *__restrict format, ...)
	string msg = header + body;
	Socket_op::writen(connectionfd, msg);
	// 不保证 都会传输;
}

void HttpConnection::handleWrite() {
	// cout << outbuff.size() << endl;
	if (Socket_op::writen(connectionfd, outbuff) < 0) {
		// 如果 无法写入: 可能是对端返回了rst造成的sig_pipe
		perror("write error");
		error_ = true;
	}
	if (outbuff.size() > 0)
		nevents |= EPOLLOUT;
}

Response *HttpConnection::analyse(Request rq) {
	// std::cout << "aaa" << std::endl;
	Response *response = new Response(200, rq.getVersion(), "william server");
	if (rq.getHeader("Connection") == "Keep-Alive" ||
		rq.getHeader("Connection") == "Keep-alive") {
		response->appendHeader("Connection", "Keep-Alive");
		response->appendHeader("Keep-Alive",
							   "timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME));
	}

	// string body = "";
	// body += "<html><title>Erro Page</title>";
	// body += "<body bgcolor=\"ffffff\">";
	// // body += to_string(code) + desc;
	// body += "<hr><em> William Http Sever</em>\n</body></html>";
	// response->appendBody(body);
	// std::cout << "aaa" << std::endl;
	int index = 0;
	if ((index = rq.getPath().find("static/"))) {
		// string filename = rq.getPath().substr(index + 7);
		string filename = rq.getPath();
		string filepath = "/home/ubuntu/cpp/fileserver/static" + filename;
		int suff_index = filename.rfind('.');
		string type = MimeType::getMime(filename.substr(suff_index));
		struct stat sbuf;
		// cout << filename << endl;
		if (stat(filepath.c_str(), &sbuf) < 0) {
			delete response;
			return nullptr;
		}

		response->appendHeader("Content-Type", type);
		response->appendHeader("Content-Length", to_string(sbuf.st_size));
		response->appendHeader("Server", "William's Web Server");

		int src_fd = open(filepath.c_str(), O_RDONLY, 0);
		if (src_fd < 0) {
			delete response;
			return nullptr;
		}
		void *mmapRet =
			mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
		::close(src_fd);
		if (mmapRet == (void *)-1) {
			// cout << "aaa" << endl;
			munmap(mmapRet, sbuf.st_size);
			delete response;
			return nullptr;
		}
		char *src_addr = static_cast<char *>(mmapRet);
		response->appendBody(string(src_addr, src_addr + sbuf.st_size));
		// cout << string(src_addr, src_addr + sbuf.st_size) << endl;
		munmap(mmapRet, sbuf.st_size);
	}

	rq.setState(STATE_FINISH);
	return response;
}

pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string>
	MimeType::mime; // static 函数 不能引用其他的static， 因为没有this

void MimeType::init() {
	mime[".html"] = "text/html";
	mime[".avi"] = "video/x-msvideo";
	mime[".bmp"] = "image/bmp";
	mime[".c"] = "text/plain";
	mime[".doc"] = "application/msword";
	mime[".gif"] = "image/gif";
	mime[".gz"] = "application/x-gzip";
	mime[".htm"] = "text/html";
	mime[".ico"] = "image/x-icon";
	mime[".jpg"] = "image/jpeg";
	mime[".png"] = "image/png";
	mime[".txt"] = "text/plain";
	mime[".mp3"] = "audio/mp3";
	mime["default"] = "text/plain";
	mime[".css"] = "text/css";
	mime[".js"] = "text/javascript";
	mime["mp4"] = "video/mp4";
}

std::string MimeType::getMime(const std::string &suffix) {
	pthread_once(&once_control,
				 MimeType::init); // 每个线程只执行一次; 等价于静态初始化
	if (mime.find(suffix) == mime.end())
		return mime["default"];
	else
		return mime[suffix];
}