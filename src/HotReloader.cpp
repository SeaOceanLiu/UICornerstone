// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#include "HotReloader.h"
#include <SDL3/SDL.h>

HotReloader::HotReloader()
    : m_pollInterval(30)
    , m_frameCounter(0)
    , m_initialCheck(false)
{
}

HotReloader::HotReloader(const fs::path& filePath, ReloadCallback callback)
    : m_filePath(filePath)
    , m_callback(callback)
    , m_pollInterval(30)
    , m_frameCounter(0)
    , m_initialCheck(false)
{
}

void HotReloader::setFile(const fs::path& filePath) {
    m_filePath = filePath;
    m_initialCheck = false;
}

void HotReloader::setCallback(ReloadCallback callback) {
    m_callback = callback;
}

void HotReloader::setPollInterval(int frames) {
    m_pollInterval = max(1, frames);
}

bool HotReloader::poll() {
    if (!m_callback || m_filePath.empty()) return false;
    if (!fs::exists(m_filePath)) return false;

    m_frameCounter++;

    if (!m_initialCheck) {
        m_lastWriteTime = fs::last_write_time(m_filePath);
        m_initialCheck = true;
        return false;
    }

    if (m_frameCounter % m_pollInterval != 0) return false;

    auto currentTime = fs::last_write_time(m_filePath);
    if (currentTime != m_lastWriteTime) {
        m_lastWriteTime = currentTime;
        SDL_Log("[HotReloader] Detected change in: %s", m_filePath.filename().string().c_str());
        return reload();
    }

    return false;
}

bool HotReloader::reload() {
    if (m_callback) {
        m_callback();
        return true;
    }
    return false;
}
