#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <string>
#include <memory>
#include "Font.h"
#include "SColor.h"
#include "Utility.h"

class TextRenderer {
public:
    virtual ~TextRenderer() = default;

    virtual SharedFont loadFont(const std::string& path, int size) = 0;
    virtual SharedFont loadFontFromMemory(const void* data, size_t len, int size) = 0;

    virtual SharedFont loadFontWithText(const std::string& path, int size, const std::string& text) {
        (void)text; return loadFont(path, size);
    }
    virtual SharedFont loadFontFromMemoryWithText(const void* data, size_t len, int size, const std::string& text) {
        (void)text; return loadFontFromMemory(data, len, size);
    }

    virtual int getFontHeight(Font* font) = 0;

    virtual void* createText(Font* font, const std::string& text) = 0;
    virtual void destroyText(void* text) = 0;
    virtual SSize measureText(void* text) = 0;
    virtual void drawText(void* text, float x, float y, SColor color) = 0;
    virtual void drawText(void* text, float x, float y, float wrapWidth, SColor color) = 0;

    virtual SSize measureText(Font* font, const std::string& text) = 0;
    virtual void drawText(Font* font, const std::string& text, float x, float y, SColor color) = 0;
    virtual void drawText(Font* font, const std::string& text, float x, float y, float wrapWidth, SColor color) = 0;
};

// 工厂函数：创建SDL3实现的TextRenderer
class RenderDevice;
TextRenderer* CreateSDL3TextRenderer(RenderDevice* device);

#endif
