#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

class Socket_op {
  public:
	static const int MAX_BUFF = 4096;
	static int setSocketNonBlocking(int fd) {
		// 参数 F_SETFD:设置文件描述符的状态标志(FD_CLOEXEC) 和 F_SETFL设置文件状态标志
		int flag = fcntl(fd, F_GETFL, 0); //忽略arg参数
		if (flag == -1)
			return -1;

		flag |= O_NONBLOCK;
		if (fcntl(fd, F_SETFL, flag) == -1)
			return -1;
		return 0;
	}

	static int setSocketNodelay(int fd) {
		int enable = 1;
		// 设置socket的状态 val和val_len
		return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable,
						  sizeof(enable));
	}

	static int createEventfd() {
		int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); //初始化 计时器是0;
		if (evtfd < 0) {
			perror("Fail in eventfd:");
			abort(); //core dump, 不错清理工作; OS层面
		}
		return evtfd;
	}

	static ssize_t readn(int fd, std::string &inBuffer, bool &zero) {
		// std::cout << "bbb" << std::endl;
		ssize_t nread = 0;
		ssize_t readSum = 0;
		while (true) {
			char buff[Socket_op::MAX_BUFF];
			if ((nread = read(fd, buff, sizeof(buff))) < 0) {
				if (errno == EINTR)
					continue;
				else if (errno == EAGAIN) {
					break;
				} else {
					return -1;
				}
			} else if (nread == 0) {
				zero = true;
				break;
			} else {
				inBuffer += std::string(buff, buff + nread);
				readSum += nread;
			}

			// std::cout << nread << std::endl;
		}
		// std::cout << "bbb" << std::endl;
		return readSum;
	}

	static ssize_t writen(int fd, std::string &outbuff) {
		ssize_t nwrite = 0; // 必须用ssize_t 因为要保证 可以是signed
		ssize_t writeSum = 0;
		// const char *ptr = outbuff.c_str();

		// 不能用 strlen(xxx) 因为img的图像编码转换成字符编码的时候会出现 \0
		// 但是 其他的文件 基于字符编码的文件， 转换成字符编码不会。
		while (outbuff.size() > 0) {
			if ((nwrite = write(fd, &outbuff[0], outbuff.size())) <= 0) {
				// 有可能写一部分; 写不了; 不停写入 直到EAGAIN 这样下次才能有edge trigger;
				// 等于0 有可能出错
				if (errno == EINTR)
					continue;
				else if (errno == EAGAIN)
					break;
				else
					return -1;
			} else {
				std::cout << "写入长度 AND 全部长度 AND 剩余长度" << std::endl;
				// std::cout << "[ " << nwrite << std::endl;
				// std::cout << outbuff.size() << std::endl;
				printf("[ %ld, %ld, %ld ]\n", nwrite, outbuff.size(),
					   outbuff.size() - nwrite);
				writeSum += nwrite;
				// ptr += nwrite;
				outbuff = outbuff.substr(nwrite);
				// std::cout << outbuff.size() << std::endl;
			}
		}
		return writeSum;
	}
};