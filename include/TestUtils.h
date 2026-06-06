#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <cstdio>
#include <cstdarg>
#include <chrono>
#include <string>
#include <filesystem>

namespace TestUtil {

inline void log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

inline uint64_t getTicks() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct Timer {
    std::chrono::steady_clock::time_point m_start;

    Timer() : m_start(std::chrono::steady_clock::now()) {}

    void reset() { m_start = std::chrono::steady_clock::now(); }

    uint64_t elapsed() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_start).count();
    }
};

} // namespace TestUtil

#endif
