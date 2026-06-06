#ifndef ResourceProviderH
#define ResourceProviderH

#include <memory>
#include <string>
#include <vector>
#include <cstdio>

class ResourceProvider {
public:
    virtual ~ResourceProvider() = default;

    virtual std::shared_ptr<std::vector<char>> readFile(const std::string& path) = 0;
    virtual bool exists(const std::string& path) = 0;

    static ResourceProvider* createFilesystem(const std::string& basePath);
};

#endif // ResourceProviderH
