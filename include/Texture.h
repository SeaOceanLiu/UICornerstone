#ifndef TEXTURE_H
#define TEXTURE_H

#include <cstdint>
#include "RenderDevice.h"

class Texture {
public:
    virtual ~Texture() = default;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void setBlendMode(BlendMode mode) = 0;
    virtual void setAlphaMod(uint8_t alpha) = 0;
    virtual BlendMode getBlendMode() const = 0;
    virtual uint8_t getAlphaMod() const = 0;
};

#endif
