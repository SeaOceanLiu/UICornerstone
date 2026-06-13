#ifndef SURFACE_H
#define SURFACE_H

#include <cstdint>
#include <string>
#include "RenderDevice.h"

// Factory function pointer types — set by backend plugin
using SurfaceCreateFn      = SharedSurface(*)(int width, int height);
using SurfaceLoadFromFileFn = SharedSurface(*)(const std::string& path);
using SurfaceLoadFromMemFn  = SharedSurface(*)(const void* data, size_t len);

class Surface {
public:
    virtual ~Surface() = default;

    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void* pixels() = 0;
    virtual const void* pixels() const = 0;

    virtual uint32_t getPixel(int x, int y) const = 0;
    virtual void setPixel(int x, int y, uint32_t pixel) = 0;

    virtual void setBlendMode(BlendMode mode) = 0;
    virtual void setAlphaMod(uint8_t alpha) = 0;

    virtual void blit(Surface* src, int srcX, int srcY, int srcW, int srcH,
                      int dstX, int dstY, int dstW, int dstH) = 0;
    virtual void blit(Surface* src, int dstX, int dstY) = 0;

    virtual SharedTexture createTexture(RenderDevice* device) = 0;
    virtual SharedSurface rotate(float angle, RenderDevice* device) = 0;

    static SharedSurface create(int width, int height);
    static SharedSurface loadFromFile(const std::string& path);
    static SharedSurface loadFromMemory(const void* data, size_t len);

    // Backend plugin registration (called from UIBackend_xxx.dll during init)
    // Exported via CORE_API so backend plugin can call it
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
    static CORE_API void registerFactories(SurfaceCreateFn c, SurfaceLoadFromFileFn lf, SurfaceLoadFromMemFn lm);
};

#endif
