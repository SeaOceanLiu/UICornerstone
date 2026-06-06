#include "ResourceProvider.h"
#include <cstdio>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

class FilesystemResourceProvider : public ResourceProvider {
    fs::path m_basePath;
    std::unordered_map<std::string, std::shared_ptr<std::vector<char>>> m_cache;
public:
    explicit FilesystemResourceProvider(const std::string& basePath)
        : m_basePath(basePath) {}

    std::shared_ptr<std::vector<char>> readFile(const std::string& path) override {
        auto it = m_cache.find(path);
        if (it != m_cache.end()) {
            return it->second;
        }

        fs::path fullPath = m_basePath / path;
        FILE* f = fopen(fullPath.string().c_str(), "rb");
        if (!f) return nullptr;

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        if (size <= 0) {
            fclose(f);
            return nullptr;
        }
        fseek(f, 0, SEEK_SET);

        auto buffer = std::make_shared<std::vector<char>>(static_cast<size_t>(size));
        size_t bytesRead = fread(buffer->data(), 1, static_cast<size_t>(size), f);
        fclose(f);

        if (bytesRead != static_cast<size_t>(size)) return nullptr;

        m_cache[path] = buffer;
        return buffer;
    }

    bool exists(const std::string& path) override {
        fs::path fullPath = m_basePath / path;
        return fs::exists(fullPath);
    }
};

ResourceProvider* ResourceProvider::createFilesystem(const std::string& basePath) {
    return new FilesystemResourceProvider(basePath);
}
