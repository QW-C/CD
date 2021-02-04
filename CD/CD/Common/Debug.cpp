#include <CD/Common/Debug.hpp>
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <codecvt>

#include <sstream>
#include <Windows.h>

namespace CD::Debug {

void error_box(const std::string& message, const char* file, int line, const char* function) {
	std::stringstream buffer;
	buffer << message << "\n" << file << "\n" << line << "\n" << function;
	std::wstring msg(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(buffer.str()));

	::MessageBoxW(nullptr, msg.c_str(), L"Assertion Failure", MB_ICONERROR);
}

}