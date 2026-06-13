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

#endif // ResourceProviderH
