#ifndef PLATFORMUTILS_H
#define PLATFORMUTILS_H

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <chrono>
#include <string>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#undef min
#undef max
#endif

namespace Platform {

inline void Log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

inline uint64_t GetTicks() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}

inline std::string GetBasePath() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    if (pos != std::string::npos) {
        fullPath = fullPath.substr(0, pos + 1);
    }
    return fullPath;
#else
    return ".";
#endif
}

} // namespace Platform

#endif // PLATFORMUTILS_H
