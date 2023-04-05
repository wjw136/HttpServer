#pragma once
#include "string"
#include <map>

// enum ResponseState {
// 	STATE_RES_PREPARE = 1,
// 	STATE_RES_FINISH,
// };

class Response {
  private:
	std::string body;
	std::map<std::string, std::string> headers;
	std::string line;

  public:
	Response();
	Response(int code, std::string httpvesion, std::string msg);
	std::string make_response();
	void appendBody(std::string info);
	void appendHeader(std::string key, std::string value);
};