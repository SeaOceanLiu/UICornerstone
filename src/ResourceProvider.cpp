#include "ResourceProvider.h"
#include "PropertyNames.h"
#include "ConstDef.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

// ============================================================
// MemoryResourceProvider
// ============================================================

struct MemoryResourceProvider::Entry {
    // registerMemory 条目：注册即拷贝完成，data 即最终存储
    std::shared_ptr<std::vector<char>> data;
    // adoptMemory 条目：零拷贝——持有调用方缓冲，首读时包装进 vector 缓存
    const void* rawPtr = nullptr;
    size_t rawLen = 0;
    void (*freeFn)(void*) = nullptr;
};

struct MemoryResourceProvider::Impl {
    std::unordered_map<std::string, Entry> entries;     // 内存注册表（含 adopt 持有的缓冲）
    std::unordered_map<std::string, std::string> paths; // name → 文件路径（JSON 挂载，懒加载）
    std::unordered_map<std::string, std::shared_ptr<std::vector<char>>> lazy; // 懒加载缓存
    std::unique_ptr<ResourceProvider> delegate;         // 文件系统兜底（懒创建）
};

MemoryResourceProvider::MemoryResourceProvider() : m_impl(new Impl()) {}
MemoryResourceProvider::~MemoryResourceProvider() {
    for (auto& [name, e] : m_impl->entries) {
        if (e.freeFn && e.rawPtr) e.freeFn(const_cast<void*>(e.rawPtr));
    }
    delete m_impl;
}

bool MemoryResourceProvider::registerMemory(const std::string& name, const void* data, size_t len) {
    if (name.empty() || !data || len == 0) return false;
    auto& e = m_impl->entries[name];
    if (e.freeFn && e.rawPtr) e.freeFn(const_cast<void*>(e.rawPtr));  // 覆盖 adopt 旧块
    e.data = std::make_shared<std::vector<char>>(static_cast<const char*>(data),
                                                 static_cast<const char*>(data) + len);
    e.rawPtr = nullptr; e.rawLen = 0; e.freeFn = nullptr;
    return true;
}

bool MemoryResourceProvider::adoptMemory(const std::string& name, void* data, size_t len,
                                         void (*freeFn)(void*)) {
    if (name.empty() || !data || len == 0) return false;
    auto& e = m_impl->entries[name];
    if (e.freeFn && e.rawPtr) e.freeFn(const_cast<void*>(e.rawPtr));  // 覆盖旧块
    e.data = nullptr;
    e.rawPtr = data; e.rawLen = len;
    e.freeFn = freeFn ? freeFn : std::free;
    m_impl->lazy.erase(name);  // 旧缓存失效（内容已替换）
    return true;
}

void MemoryResourceProvider::setFilesystemFallback(const std::string& basePath) {
    if (!m_impl->delegate) {
        m_impl->delegate.reset(ResourceProvider::createFilesystem(basePath));
    }
}

bool MemoryResourceProvider::hasFilesystemFallback() const {
    return m_impl->delegate != nullptr;
}

void MemoryResourceProvider::inheritFilesystemFallback(const std::string& resourceRoot) {
    if (m_impl->delegate) return;   // 幂等（与 mountPath 懒建共存）
    m_impl->delegate.reset(ResourceProvider::createFilesystem(
        resourceRoot.empty() ? ConstDef::pathPrefix.string() : resourceRoot));
}

void MemoryResourceProvider::mountPath(const std::string& name, const std::string& path,
                                       const std::string& basePath) {
    if (name.empty() || path.empty()) return;
    if (!m_impl->delegate) {
        m_impl->delegate.reset(ResourceProvider::createFilesystem(basePath));
    }
    // P0-70②：目录挂载前缀拼接统一以 "/" 结尾（去尾部重复分隔符）
    std::string p = path;
    while (p.size() > 1 && (p.back() == '/' || p.back() == '\\')) p.pop_back();
    m_impl->paths[name] = p;
}

// P0-70②：目录挂载子路径安全校验（拒绝 .. 段 / 盘符 / 前导分隔符，防目录逃逸）
static bool isSafeSubPath(const std::string& rest) {
    if (rest.empty()) return false;
    if (rest[0] == '/' || rest[0] == '\\') return false;
    if (rest.find(':') != std::string::npos) return false;
    size_t start = 0;
    while (start <= rest.size()) {
        size_t end = rest.find_first_of("/\\", start);
        std::string seg = rest.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (seg == "..") return false;
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return true;
}

// P0-70②：目录挂载前缀解析——最长挂载名匹配 key = M + "/" + rest；返回拼接后的实际路径
static bool resolveDirMount(const std::unordered_map<std::string, std::string>& paths,
                            const std::string& key, std::string& outFull) {
    std::string norm = key;
    for (char& c : norm) if (c == '\\') c = '/';
    const std::string* bestName = nullptr;
    size_t bestLen = 0;
    for (const auto& [mname, mpath] : paths) {
        if (norm.size() > mname.size() && norm.compare(0, mname.size(), mname) == 0
            && norm[mname.size()] == '/') {
            if (mname.size() > bestLen) { bestLen = mname.size(); bestName = &mname; }
        }
    }
    if (!bestName) return false;
    std::string rest = norm.substr(bestLen + 1);
    if (!isSafeSubPath(rest)) return false;
    outFull = paths.at(*bestName);
    if (!outFull.empty() && outFull.back() != '/' && outFull.back() != '\\') outFull += '/';
    outFull += rest;
    return true;
}

static std::string stripProviderPrefix(const std::string& s) {
    if (s.rfind(PropertyNames::kProviderPrefix, 0) == 0) return s.substr(strlen(PropertyNames::kProviderPrefix));
    return s;
}

// P0-54：绝对路径前缀剥离（assets 前缀 → 相对键重试；内存 provider 精确键匹配场景）
static std::string stripAssetPrefix(const std::string& s) {
    const std::string prefix = ConstDef::pathPrefix.string();
    if (!prefix.empty() && s.rfind(prefix, 0) == 0) {
        size_t pos = prefix.size();
        if (pos < s.size() && (s[pos] == '/' || s[pos] == '\\')) ++pos;
        return s.substr(pos);
    }
    return s;
}

std::shared_ptr<std::vector<char>> MemoryResourceProvider::readFile(const std::string& path) {
    const std::string name = stripProviderPrefix(path);

    auto lookup = [this](const std::string& key) -> std::shared_ptr<std::vector<char>> {
        auto it = m_impl->entries.find(key);
        if (it != m_impl->entries.end()) {
            const Entry& e = it->second;
            if (e.data) return e.data;  // register 条目：直接共享
            // adopt 条目：首次读取时包装进 vector（仅此一次拷贝），之后共享
            auto cached = m_impl->lazy.find(key);
            if (cached != m_impl->lazy.end()) return cached->second;
            auto buf = std::make_shared<std::vector<char>>(
                static_cast<const char*>(e.rawPtr), static_cast<const char*>(e.rawPtr) + e.rawLen);
            m_impl->lazy[key] = buf;
            return buf;
        }
        auto p = m_impl->paths.find(key);
        if (p != m_impl->paths.end()) {
            auto cached = m_impl->lazy.find(key);
            if (cached != m_impl->lazy.end()) return cached->second;
            if (!m_impl->delegate) return nullptr;
            auto buf = m_impl->delegate->readFile(p->second);
            if (buf) m_impl->lazy[key] = buf;
            return buf;
        }
        // P0-70②：目录挂载前缀解析（精确优先；最长挂载名；懒加载缓存键=完整子路径）
        std::string full;
        if (!m_impl->delegate) return nullptr;
        if (resolveDirMount(m_impl->paths, key, full)) {
            auto cached = m_impl->lazy.find(key);
            if (cached != m_impl->lazy.end()) return cached->second;
            auto buf = m_impl->delegate->readFile(full);
            if (buf) m_impl->lazy[key] = buf;
            return buf;
        }
        return nullptr;
    };

    auto hit = lookup(name);
    if (hit) return hit;
    // P0-54 回退：绝对路径（assets 前缀）→ 剥离后按相对键重试（精确命中优先）
    const std::string rel = stripAssetPrefix(name);
    if (rel != name) {
        hit = lookup(rel);
        if (hit) return hit;
    }
    // P0-78：文件系统回退（未注册/未挂载键按 resourceRoot 读盘；替换内存 provider 后相对键资源不失效）
    if (m_impl->delegate) {
        if (auto buf = m_impl->delegate->readFile(name)) return buf;
        if (rel != name) return m_impl->delegate->readFile(rel);
    }
    return nullptr;
}

bool MemoryResourceProvider::exists(const std::string& path) {
    const std::string name = stripProviderPrefix(path);
    if (m_impl->entries.find(name) != m_impl->entries.end()) return true;
    if (m_impl->paths.find(name) != m_impl->paths.end()) return true;
    // P0-70②：目录挂载子路径存在性（与 readFile 同规则）
    {
        std::string full;
        if (m_impl->delegate && resolveDirMount(m_impl->paths, name, full)) {
            if (m_impl->delegate->exists(full)) return true;
        }
    }
    const std::string rel = stripAssetPrefix(name);
    if (rel != name) {
        if (m_impl->entries.find(rel) != m_impl->entries.end()) return true;
        if (m_impl->paths.find(rel) != m_impl->paths.end()) return true;
    }
    // P0-78：文件系统回退
    if (m_impl->delegate) {
        if (m_impl->delegate->exists(name)) return true;
        if (rel != name && m_impl->delegate->exists(rel)) return true;
    }
    return false;
}

ResourceProvider* ResourceProvider::createFilesystem(const std::string& basePath) {
    return new FilesystemResourceProvider(basePath);
}
