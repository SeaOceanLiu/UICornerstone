#include "TextRenderer.h"
#include "RenderDevice.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <unordered_map>
#include <cstring>

// ============================================================
// SDL3Font
// ============================================================
class SDL3Font : public Font {
public:
    SDL3Font(TTF_Font* font, int size) : m_font(font), m_size(size) {}
    ~SDL3Font() override { if (m_font) TTF_CloseFont(m_font); }
    int getSize() const override { return m_size; }
    TTF_Font* get() const { return m_font; }
private:
    TTF_Font* m_font;
    int m_size;
};

// Font cache key: content hash + pixel size
struct FontCacheKey {
    size_t contentHash;
    int size;
    bool operator==(const FontCacheKey& o) const {
        return contentHash == o.contentHash && size == o.size;
    }
};
struct FontCacheHash {
    size_t operator()(const FontCacheKey& k) const {
        size_t h = k.contentHash;
        h ^= k.size * 0x9e3779b9;
        return h;
    }
};

// ============================================================
// SDL3TextRenderer
// ============================================================
class SDL3TextRenderer : public TextRenderer {
public:
    SDL3TextRenderer(RenderDevice* device)
        : m_device(device)
    {
        SDL_Renderer* renderer = static_cast<SDL_Renderer*>(m_device->getNativeHandle());
        m_textEngine = TTF_CreateRendererTextEngine(renderer);
    }

    ~SDL3TextRenderer() override {
        if (m_textEngine) {
            TTF_DestroyRendererTextEngine(m_textEngine);
        }
        TTF_Quit();
    }

    SharedFont loadFont(const std::string& path, int size) override {
        return loadOrCreate(path, size);
    }

    SharedFont loadFontFromMemory(const void* data, size_t len, int size) override {
        size_t contentHash = std::hash<std::string_view>{}(std::string_view(static_cast<const char*>(data), len));
        FontCacheKey key{contentHash, size};
        auto it = m_fontCache.find(key);
        if (it != m_fontCache.end()) {
            return it->second;
        }
        SDL_IOStream* stream = SDL_IOFromConstMem(data, len);
        if (!stream) {
            SDL_Log("SDL3TextRenderer::loadFontFromMemory: SDL_IOFromConstMem failed");
            return nullptr;
        }
        TTF_Font* font = TTF_OpenFontIO(stream, true, size);
        if (!font) {
            SDL_Log("SDL3TextRenderer::loadFontFromMemory: %s", SDL_GetError());
            return nullptr;
        }
        SharedFont sf = std::make_shared<SDL3Font>(font, size);
        m_fontCache[key] = sf;
        return sf;
    }

    SSize measureText(Font* font, const std::string& text) override {
        auto* sdl3Font = static_cast<SDL3Font*>(font);
        TTF_Text* tempText = TTF_CreateText(m_textEngine, sdl3Font->get(), text.c_str(), text.length());
        if (!tempText) return SSize(0, 0);
        int w = 0, h = 0;
        TTF_GetTextSize(tempText, &w, &h);
        TTF_DestroyText(tempText);
        return SSize(static_cast<float>(w), static_cast<float>(h));
    }

    void* createText(Font* font, const std::string& text) override {
        auto* sdl3Font = static_cast<SDL3Font*>(font);
        return TTF_CreateText(m_textEngine, sdl3Font->get(), text.c_str(), text.length());
    }

    void destroyText(void* text) override {
        if (text) TTF_DestroyText(static_cast<TTF_Text*>(text));
    }

    SSize measureText(void* text) override {
        if (!text) return SSize(0, 0);
        int w = 0, h = 0;
        TTF_GetTextSize(static_cast<TTF_Text*>(text), &w, &h);
        return SSize(static_cast<float>(w), static_cast<float>(h));
    }

    void drawText(void* text, float x, float y, SColor color) override {
        if (!text) return;
        TTF_Text* t = static_cast<TTF_Text*>(text);
        TTF_SetTextColor(t, color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte());
        TTF_DrawRendererText(t, x, y);
    }

    void drawText(void* text, float x, float y, float wrapWidth, SColor color) override {
        if (!text) return;
        TTF_Text* t = static_cast<TTF_Text*>(text);
        TTF_SetTextColor(t, color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte());
        TTF_SetTextWrapWidth(t, static_cast<int>(wrapWidth));
        TTF_DrawRendererText(t, x, y);
    }

    int getFontHeight(Font* font) override {
        auto* sdl3Font = static_cast<SDL3Font*>(font);
        return TTF_GetFontHeight(sdl3Font->get());
    }

    void drawText(Font* font, const std::string& text, float x, float y, SColor color) override {
        auto* sdl3Font = static_cast<SDL3Font*>(font);
        TTF_Text* textObj = TTF_CreateText(m_textEngine, sdl3Font->get(), text.c_str(), static_cast<uint32_t>(text.length()));
        if (!textObj) return;
        TTF_SetTextColor(textObj, color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte());
        TTF_DrawRendererText(textObj, x, y);
        TTF_DestroyText(textObj);
    }

    void drawText(Font* font, const std::string& text, float x, float y, float wrapWidth, SColor color) override {
        auto* sdl3Font = static_cast<SDL3Font*>(font);
        TTF_Text* textObj = TTF_CreateText(m_textEngine, sdl3Font->get(), text.c_str(), static_cast<uint32_t>(text.length()));
        if (!textObj) return;
        TTF_SetTextColor(textObj, color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte());
        TTF_SetTextWrapWidth(textObj, static_cast<int>(wrapWidth));
        TTF_DrawRendererText(textObj, x, y);
        TTF_DestroyText(textObj);
    }

private:
    SharedFont loadOrCreate(const std::string& path, int size) {
        auto it = m_pathCache.find(path);
        if (it != m_pathCache.end()) {
            if (it->second.find(size) != it->second.end()) {
                return it->second[size];
            }
        }
        TTF_Font* font = TTF_OpenFont(path.c_str(), size);
        if (!font) {
            SDL_Log("SDL3TextRenderer::loadFont: %s", SDL_GetError());
            return nullptr;
        }
        SharedFont sf = std::make_shared<SDL3Font>(font, size);
        m_pathCache[path][size] = sf;
        return sf;
    }

    RenderDevice* m_device;
    TTF_TextEngine* m_textEngine;
    std::unordered_map<FontCacheKey, SharedFont, FontCacheHash> m_fontCache;
    std::unordered_map<std::string, std::unordered_map<int, SharedFont>> m_pathCache;
};

// ============================================================
// Factory
// ============================================================
TextRenderer* CreateSDL3TextRenderer(RenderDevice* device) {
    if (!TTF_Init()) {
        SDL_Log("TTF_Init failed: %s", SDL_GetError());
    }
    return new SDL3TextRenderer(device);
}
