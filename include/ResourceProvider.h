#ifndef ResourceProviderH
#define ResourceProviderH

#include <memory>
#include <string>
#include <vector>
#include <SDL3/SDL_iostream.h>

class ResourceProvider {
public:
    virtual ~ResourceProvider() = default;

    virtual std::shared_ptr<std::vector<char>> readFile(const std::string& path) = 0;
    virtual SDL_IOStream* openFileStream(const std::string& path, const char* mode) = 0;
    virtual bool exists(const std::string& path) = 0;

    static ResourceProvider* createFilesystem(const std::string& basePath);
};

#endif // ResourceProviderH
