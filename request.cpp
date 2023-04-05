#include "request.h"
#include <iostream>

void Request::process(std::string &buff) {
	if (processState == STATE_PARSE_URI) {
		if (!processURL(buff)) {
			// std::cout << "aaaa" << std::endl;
			return;
		}
		// std::cout << processState << std::endl;
	}

	if (processState == STATE_PARSE_HEADERS) {
		// std::cout << "ccc" << std::endl;
		if (!processHeader(buff)) {
			return;
			// std::cout << "ccc" << std::endl;
		}
	}

	if (processState == STATE_PARSE_BODY) {
		if (!processBody(buff)) {
			return;
		}
	}
}

bool Request::processURL(std::string &buff) {
	size_t pos = buff.find("\r\n");
	if (pos < 0) {
		return false;
	}

	std::string request_line = buff.substr(0, pos);
	if (buff.size() > (pos + 2)) {
		buff = buff.substr(pos + 2);
	} else {
		buff.clear();
	}

	if ((pos = request_line.find("GET")) >= 0) {
		method_ = METHOD_GET;
	} else if ((pos = request_line.find("POST") >= 0)) {
		method_ = METHOD_POST;
	} else {
		isvalid = false;
		return false;
	}

	// filename
	pos = request_line.find("/", pos);
	if (pos < 0) {
		filename = "/index.html";
		version_ = HTTP_11;
	} else {
		size_t pos_end = request_line.find(" ", pos);
		// std::cout << request_line << std::endl;
		// std::cout << pos_end << " " << pos << std::endl;
		if (pos_end < 0) {
			isvalid = false;
			return false;
		} else {
			if (pos_end - pos > 1) {
				filename = request_line.substr(
					pos, pos_end - pos); // 左闭区间 左+len=开区间
				// std::cout << filename << std::endl;
				size_t pos_2 = filename.find('?');
				if (pos_2 > 0) {
					filename = filename.substr(0, pos_2);
				} else {
					// TODO： url 带参数的情况
				}

				// 处理static中没有后缀的情况，默认是html
				int dot_index = filename.find('.');
				if (dot_index < 0) {
					filename += ".html";
				}

			} else {
				filename = "/index.html";
			}
		}
		request_line = request_line.substr(pos_end + 1);
	}

	// std::cout << buff << std::endl;
	// HTTP version
	pos = request_line.find("/");
	if (pos < 0 || request_line.size() - pos <= 3) {
		isvalid = false;
		return false;
	} else {
		std::string ver = request_line.substr(pos + 1);
		if (ver == "1.0") {
			version_ = HTTP_10;
		} else if (ver == "1.1") {
			version_ = HTTP_11;
		} else {
			isvalid = false;
			return false;
		}
	}

	processState = STATE_PARSE_HEADERS;
	// std::cout << buff << std::endl;
	return true;
}

bool Request::processHeader(std::string &buff) {
	// std::cout << buff << std::endl;
	int index = buff.find("\r\n\r\n");
	if (index < 0) {
		return false;
	} else {
		std::string header = buff.substr(0, index + 2);
		// std::cout << header << std::endl;
		buff = buff.substr(index + 4);

		int index_ = 0;
		while ((index_ = header.find("\r\n")) >= 0) {
			std::string line = header.substr(0, index);
			header = header.substr(index_ + 2);

			int colon_index = line.find(": ");
			if (colon_index < 0) {
				isvalid = false;
				return false;
			}
			this->headers[line.substr(0, colon_index)] =
				line.substr(colon_index + 2);
		}

		if (header.size() > 0) {
			isvalid = false;
			return false;
		}
	}
	// std::cout << buff << std::endl;
	// std::cout << method_ << std::endl;
	if (method_ == METHOD_GET) {
		processState = STATE_ANALYSE;
	} else {
		processState = STATE_PARSE_BODY;
	}
	return true;
}

bool Request::processBody(std::string &buff) {
	// http两个报文的间隔 使用contenet-length， content-length指定了报文的长度;
	// TODO；

	return true;
}

void Request::reset() {
	this->processState = STATE_PARSE_URI;
	this->filename = "";
	headers.clear();
	isvalid = true;
}