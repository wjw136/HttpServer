#include "response.h"
using namespace std;

Response::Response(int code, string version, string msg) {
	line = version + " " + to_string(code) + " " + msg + "\r\n";
}

string Response::make_response() {
	string tmp = line;
	for (auto i : headers) {
		tmp += (i.first + ": " + i.second + "\r\n");
	}
	tmp += "\r\n";
	tmp += body;
	// printf("%s\n", tmp.c_str());
	return tmp;
}

void Response::appendBody(std::string info) { body += info; }

void Response::appendHeader(std::string key, std::string value) {
	headers[key] = value;
}