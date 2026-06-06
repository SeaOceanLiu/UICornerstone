// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#ifndef HotReloaderH
#define HotReloaderH

#include <memory>
#include <string>
#include <functional>
#include <filesystem>
#include <chrono>

using namespace std;
namespace fs = std::filesystem;

class HotReloader {
public:
    using ReloadCallback = function<void()>;

    HotReloader();
    HotReloader(const fs::path& filePath, ReloadCallback callback);

    void setFile(const fs::path& filePath);
    void setCallback(ReloadCallback callback);
    void setPollInterval(int frames);

    bool poll();
    bool reload();

private:
    fs::path m_filePath;
    ReloadCallback m_callback;
    fs::file_time_type m_lastWriteTime;
    int m_pollInterval;
    int m_frameCounter;
    bool m_initialCheck;
};

#endif
