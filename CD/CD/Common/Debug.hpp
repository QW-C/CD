#pragma once

#include <CD/Common/Common.hpp>
#include <string>

namespace CD::Debug {

void error_box(const std::string& message, const char* file, int line, const char* function);

#define CD_FAIL(message) (Debug::error_box(message, __FILE__, __LINE__, __FUNCSIG__), __debugbreak())

#define CD_ASSERT_ENABLED
#ifdef CD_ASSERT_ENABLED

#define CD_ASSERT(condition) (condition ? void(0) : CD_FAIL(#condition))

#else
#define CD_ASSERT(condition) (condition)
#endif

}