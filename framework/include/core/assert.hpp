
#pragma once

#include "core/defines.hpp"

#if defined VKS_DEBUG
    #define ASSERT(EXPRESSION, MESSAGE, ...) vks::debug_assert(#EXPRESSION, EXPRESSION, __FILE__, __func__, __LINE__, MESSAGE, ##__VA_ARGS__)
#else
    #define ASSERT(EXPRESSION, MESSAGE, ...)
#endif

namespace vks {
    void debug_assert(const char* expression, bool result, const char* file, const char* function, int line, const char* format, ...);
}
