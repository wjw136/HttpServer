#pragma once
#include <map>
#include <string>

enum ProcessState {
	STATE_PARSE_URI = 1,
	STATE_PARSE_HEADERS,
	STATE_PARSE_BODY,
	STATE_ANALYSE,
	STATE_FINISH
};

enum HttpMethod { METHOD_POST = 1, METHOD_GET };

enum HttpVersion { HTTP_10 = 1, HTTP_11 };

class Request {
  private:
	ProcessState processState;
	HttpMethod method_;
	HttpVersion version_;
	std::string filename;
	std::map<std::string, std::string> headers;

	bool isvalid;

  public:
	void process(std::string &buff);
	bool processHeader(std::string &buff);
	bool processURL(std::string &buff);
	bool processBody(std::string &buff);

	Request(ProcessState processStat = STATE_PARSE_URI)
		: processState(processStat), isvalid(true) {}

	ProcessState getState() { return processState; }

	void setState(ProcessState st) { this->processState = st; }

	bool getValid() { return isvalid; }
	void noValid() { isvalid = false; }

	std::string getVersion() {
		return version_ == HTTP_11 ? "HTTP/1.1" : "HTTP/1.0";
	}

	std::string getHeader(std::string key) { return headers[key]; }

	void reset();

	std::string getPath() { return filename; }
};
