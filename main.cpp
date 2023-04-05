#include "eventloop.h"
#include "httpserver.h"

int main() {
	EventLoop mainLoop;
	HttpServer myHTTPServer(8001, 10, &mainLoop);
	myHTTPServer.start();
	mainLoop.loop();
	return 0;
}

/*
	双缓冲：
	在Buffer中准备 tmpbuffer， nextbuffer 和 vec_buffer
	前端 争夺锁， 写入buffer， 如果满了或者超时了， 就用条件变量通知后端
	后端 通知后， 争夺锁， 交换外部的buffer， 和内部的空buffer， 写入文件， 等待在条件变量之上。
	四缓冲可以进一步的防止前端等待在后端的消费之后的空buffer之上。
*/