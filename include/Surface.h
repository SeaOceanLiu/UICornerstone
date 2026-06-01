#ifndef SURFACE_H
#define SURFACE_H

#include <cstdint>
#include <string>
#include "RenderDevice.h"

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

    static SharedSurface create(int width, int height);
    static SharedSurface loadFromFile(const std::string& path);
    static SharedSurface loadFromMemory(const void* data, size_t len);
};

#endif
