#ifndef ResourceProviderH
#define ResourceProviderH

#include <memory>
#include <string>
#include <vector>
#include <cstdio>

// Export/import for cross-DLL safety
#ifndef UICORNERSTONE_CORE_API_DEFINED
#define UICORNERSTONE_CORE_API_DEFINED
#if defined(UICORNERSTONE_CORE_API_EXPORT)
  #define CORE_API __declspec(dllexport)
#elif defined(UICORNERSTONE_BUILD_SHARED)
  #define CORE_API __declspec(dllimport)
#else
  #define CORE_API
#endif
#endif

class CORE_API ResourceProvider {
public:
    virtual ~ResourceProvider() = default;

    virtual std::shared_ptr<std::vector<char>> readFile(const std::string& path) = 0;
    virtual bool exists(const std::string& path) = 0;

    static ResourceProvider* createFilesystem(const std::string& basePath);
};

// 内存资源注册表：name → 字节。
// 两种注册模式：
//   registerMemory —— 拷贝（引擎内部复制字节，调用方可立即释放 data）；
//   adoptMemory   —— 零拷贝（引擎不复制、仅引用调用方 buffer；调用方须保持有效直至
//                    销毁/覆盖，届时经 freeFn 回调释放，freeFn 可 NULL → 默认 free）。
// 另支持 mountPath 懒加载（布局 JSON resourceProviders 的 path 条目）：
//   首次 readFile 时经内部文件系统兜底读入并缓存。
// readFile 自动剥离 PropertyNames::kProviderPrefix 前缀（工厂路径 / image-resource 等直接透传的场景）。
class CORE_API MemoryResourceProvider : public ResourceProvider {
public:
    MemoryResourceProvider();
    ~MemoryResourceProvider() override;

    std::shared_ptr<std::vector<char>> readFile(const std::string& path) override;
    bool exists(const std::string& path) override;

    bool registerMemory(const std::string& name, const void* data, size_t len);
    bool adoptMemory(const std::string& name, void* data, size_t len, void (*freeFn)(void*));
    // P0-78：文件系统回退（未注册/未挂载键按 basePath 读盘；幂等，与 mountPath 懒建共用 delegate）
    void setFilesystemFallback(const std::string& basePath);
    bool hasFilesystemFallback() const;   // P0-80：是否已具备回退（替换 provider 时继承判断）
    // P0-80：替换 provider 时继承实例级文件系统回退（resourceRoot 空 → pathPrefix；幂等；
    // 逻辑在引擎侧实现，避免插件/源码混合编译引用 ConstDef 数据符号）
    void inheritFilesystemFallback(const std::string& resourceRoot);
    void mountPath(const std::string& name, const std::string& path, const std::string& basePath);

private:
    struct Entry;
    struct Impl;
    Impl* m_impl;
};

#endif // ResourceProviderH
