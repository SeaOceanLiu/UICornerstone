#include "RenderDevice.h"
#include "Texture.h"
#include "Surface.h"
#include "ConstDef.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <unordered_map>

// ============================================================
// SDL3Texture
// ============================================================
class SDL3Texture : public Texture {
public:
    SDL3Texture(SDL_Texture* tex, int w, int h)
        : m_texture(tex), m_w(w), m_h(h), m_blendMode(BlendMode::Blend), m_alphaMod(255)
    {
    }

    ~SDL3Texture() override {
        if (m_texture) {
            SDL_DestroyTexture(m_texture);
        }
    }

    int width() const override { return m_w; }
    int height() const override { return m_h; }

    void setBlendMode(BlendMode mode) override {
        m_blendMode = mode;
        if (m_texture) {
            SDL_SetTextureBlendMode(m_texture, toSDLBlendMode(mode));
        }
    }

    void setAlphaMod(uint8_t alpha) override {
        m_alphaMod = alpha;
        if (m_texture) {
            SDL_SetTextureAlphaMod(m_texture, alpha);
        }
    }

    BlendMode getBlendMode() const override { return m_blendMode; }
    uint8_t getAlphaMod() const override { return m_alphaMod; }

    SDL_Texture* native() const { return m_texture; }

    static SDL_BlendMode toSDLBlendMode(BlendMode mode) {
        switch (mode) {
            case BlendMode::None:  return SDL_BLENDMODE_NONE;
            case BlendMode::Blend: return SDL_BLENDMODE_BLEND;
            case BlendMode::Add:   return SDL_BLENDMODE_ADD;
            case BlendMode::Mod:   return SDL_BLENDMODE_MOD;
            case BlendMode::Mul:   return SDL_BLENDMODE_MUL;
            default:               return SDL_BLENDMODE_BLEND;
        }
    }

private:
    SDL_Texture* m_texture;
    int m_w, m_h;
    BlendMode m_blendMode;
    uint8_t m_alphaMod;
};

// ============================================================
// SDL3RenderDevice
// ============================================================
class SDL3RenderDevice : public RenderDevice {
public:
    SDL3RenderDevice(SDL_Renderer* renderer)
        : m_renderer(renderer)
    {
    }

    ~SDL3RenderDevice() override = default;

    // === 渲染状态 ===
    void setDrawColor(SColor color) override {
        SDL_SetRenderDrawColor(m_renderer, color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte());
    }

    void setBlendMode(BlendMode mode) override {
        SDL_SetRenderDrawBlendMode(m_renderer, SDL3Texture::toSDLBlendMode(mode));
    }

    void setClipRect(const SRect& rect) override {
        SDL_Rect sdlRect = { static_cast<int>(rect.left), static_cast<int>(rect.top),
                             static_cast<int>(rect.width), static_cast<int>(rect.height) };
        SDL_SetRenderClipRect(m_renderer, &sdlRect);
    }

    void clearClipRect() override {
        SDL_SetRenderClipRect(m_renderer, nullptr);
    }

    // === 基础图元 ===
    void fillRect(const SRect& rect) override {
        SDL_FRect sdlRect = {rect.left, rect.top, rect.width, rect.height};
        SDL_RenderFillRect(m_renderer, &sdlRect);
    }

    void drawRect(const SRect& rect) override {
        SDL_FRect sdlRect = {rect.left, rect.top, rect.width, rect.height};
        SDL_RenderRect(m_renderer, &sdlRect);
    }

    void drawLine(float x1, float y1, float x2, float y2) override {
        SDL_RenderLine(m_renderer, x1, y1, x2, y2);
    }

    void drawPoint(float x, float y) override {
        SDL_RenderPoint(m_renderer, x, y);
    }

    // === 复杂图元 ===
    void drawTriangles(const Vertex* vertices, int count) override {
        if (!m_renderer || count < 3) return;
        std::vector<SDL_Vertex> sdlVerts(count);
        for (int i = 0; i < count; ++i) {
            sdlVerts[i].position = { vertices[i].x, vertices[i].y };
            sdlVerts[i].color = { vertices[i].color.red(), vertices[i].color.green(),
                                  vertices[i].color.blue(), vertices[i].color.alpha() };
            sdlVerts[i].tex_coord = { 0, 0 };
        }
        SDL_RenderGeometry(m_renderer, nullptr, sdlVerts.data(), count, nullptr, 0);
    }

    void drawTriangleStrip(const Vertex* vertices, int count) override {
        if (!m_renderer || count < 3) return;
        std::vector<SDL_Vertex> sdlVerts(count);
        for (int i = 0; i < count; ++i) {
            sdlVerts[i].position = { vertices[i].x, vertices[i].y };
            sdlVerts[i].color = { vertices[i].color.red(), vertices[i].color.green(),
                                  vertices[i].color.blue(), vertices[i].color.alpha() };
            sdlVerts[i].tex_coord = { 0, 0 };
        }
        int numTriangles = (count - 2);
        std::vector<int> indices(numTriangles * 3);
        for (int i = 0; i < numTriangles; ++i) {
            int idx = i * 3;
            if (i % 2 == 0) {
                indices[idx] = i; indices[idx + 1] = i + 1; indices[idx + 2] = i + 2;
            } else {
                indices[idx] = i + 1; indices[idx + 1] = i; indices[idx + 2] = i + 2;
            }
        }
        SDL_RenderGeometry(m_renderer, nullptr, sdlVerts.data(), count, indices.data(), numTriangles * 3);
    }

    void drawTriangleFan(const Vertex* vertices, int count) override {
        if (!m_renderer || count < 3) return;
        std::vector<SDL_Vertex> sdlVerts(count);
        for (int i = 0; i < count; ++i) {
            sdlVerts[i].position = { vertices[i].x, vertices[i].y };
            sdlVerts[i].color = { vertices[i].color.red(), vertices[i].color.green(),
                                  vertices[i].color.blue(), vertices[i].color.alpha() };
            sdlVerts[i].tex_coord = { 0, 0 };
        }
        int numTriangles = (count - 2);
        std::vector<int> indices(numTriangles * 3);
        for (int i = 0; i < numTriangles; ++i) {
            int idx = i * 3;
            indices[idx] = 0; indices[idx + 1] = i + 1; indices[idx + 2] = i + 2;
        }
        SDL_RenderGeometry(m_renderer, nullptr, sdlVerts.data(), count, indices.data(), numTriangles * 3);
    }

    void drawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, SColor color) override {
        SDL_Vertex verts[3];
        SDL_FColor fcolor = { color.red(), color.green(), color.blue(), color.alpha() };
        verts[0] = {{ x0, y0 }, fcolor, { 0, 0 }};
        verts[1] = {{ x1, y1 }, fcolor, { 0, 0 }};
        verts[2] = {{ x2, y2 }, fcolor, { 0, 0 }};
        SDL_RenderGeometry(m_renderer, nullptr, verts, 3, nullptr, 0);
    }

    void drawQuad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, SColor color) override {
        SDL_Vertex verts[4];
        SDL_FColor fcolor = { color.red(), color.green(), color.blue(), color.alpha() };
        verts[0] = {{ x0, y0 }, fcolor, { 0, 0 }};
        verts[1] = {{ x1, y1 }, fcolor, { 0, 0 }};
        verts[2] = {{ x2, y2 }, fcolor, { 0, 0 }};
        verts[3] = {{ x3, y3 }, fcolor, { 0, 0 }};
        int indices[6] = { 0, 1, 2, 0, 2, 3 };
        SDL_RenderGeometry(m_renderer, nullptr, verts, 4, indices, 6);
    }

    // === 纹理操作 ===
    SharedTexture createTextureFromFile(const std::string& path) override {
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            SDL_Log("createTextureFromFile: %s", SDL_GetError());
            return nullptr;
        }
        SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surface);
        int w = surface->w, h = surface->h;
        SDL_DestroySurface(surface);
        if (!tex) {
            SDL_Log("createTextureFromFile: SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
            return nullptr;
        }
        return std::make_shared<SDL3Texture>(tex, w, h);
    }

    SharedTexture createTextureFromSurface(Surface* surface) override {
        if (!surface) return nullptr;
        return surface->createTexture(this);
    }

    SharedTexture createRenderTexture(int width, int height) override {
        SDL_Texture* tex = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888,
                                              SDL_TEXTUREACCESS_TARGET, width, height);
        if (!tex) {
            SDL_Log("createRenderTexture: %s", SDL_GetError());
            return nullptr;
        }
        return std::make_shared<SDL3Texture>(tex, width, height);
    }

    void destroyTexture(Texture* texture) override {
        delete texture;
    }

    void drawTexture(Texture* texture, const SRect* srcRect, const SRect* dstRect) override {
        if (!m_renderer || !texture || !dstRect) return;
        SDL3Texture* sdlTex = static_cast<SDL3Texture*>(texture);
        SDL_FRect sdlDst = { dstRect->left, dstRect->top, dstRect->width, dstRect->height };
        if (srcRect) {
            SDL_FRect sdlSrc = { srcRect->left, srcRect->top, srcRect->width, srcRect->height };
            SDL_RenderTexture(m_renderer, sdlTex->native(), &sdlSrc, &sdlDst);
        } else {
            SDL_RenderTexture(m_renderer, sdlTex->native(), nullptr, &sdlDst);
        }
    }

    void drawTextureRotated(Texture* texture, const SRect* srcRect, const SRect* dstRect, float angle) override {
        if (!m_renderer || !texture || !dstRect) return;
        SDL3Texture* sdlTex = static_cast<SDL3Texture*>(texture);
        SDL_FRect sdlDst = { dstRect->left, dstRect->top, dstRect->width, dstRect->height };
        if (srcRect) {
            SDL_FRect sdlSrc = { srcRect->left, srcRect->top, srcRect->width, srcRect->height };
            SDL_RenderTextureRotated(m_renderer, sdlTex->native(), &sdlSrc, &sdlDst, angle, nullptr, SDL_FLIP_NONE);
        } else {
            SDL_RenderTextureRotated(m_renderer, sdlTex->native(), nullptr, &sdlDst, angle, nullptr, SDL_FLIP_NONE);
        }
    }

    // === 渲染到纹理 ===
    void setRenderTarget(Texture* texture) override {
        SDL_Texture* tex = texture ? static_cast<SDL3Texture*>(texture)->native() : nullptr;
        SDL_SetRenderTarget(m_renderer, tex);
    }

    void resetRenderTarget() override {
        SDL_SetRenderTarget(m_renderer, nullptr);
    }

    void readPixels(void* buffer, const SRect& rect) override {
        SDL_Rect sdlRect = { static_cast<int>(rect.left), static_cast<int>(rect.top),
                             static_cast<int>(rect.width), static_cast<int>(rect.height) };
        SDL_RenderReadPixels(m_renderer, &sdlRect);
    }

    // === Frame operations ===
    void clear() override {
        SDL_RenderClear(m_renderer);
    }

    void present() override {
        SDL_RenderPresent(m_renderer);
    }

    SDL_Renderer* getSDL3Renderer() const { return m_renderer; }

private:
    SDL_Renderer* m_renderer;
};

// ============================================================
// SDL3Surface
// ============================================================
class SDL3Surface : public Surface {
public:
    SDL3Surface(SDL_Surface* surface)
        : m_surface(surface)
    {
    }

    ~SDL3Surface() override {
        if (m_surface) {
            SDL_DestroySurface(m_surface);
        }
    }

    int width() const override { return m_surface->w; }
    int height() const override { return m_surface->h; }
    void* pixels() override { return m_surface->pixels; }
    const void* pixels() const override { return m_surface->pixels; }

    uint32_t getPixel(int x, int y) const override {
        if (!m_surface || x < 0 || x >= m_surface->w || y < 0 || y >= m_surface->h) return 0;
        uint8_t r, g, b, a;
        const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(m_surface->format);
        SDL_GetRGBA(static_cast<const uint32_t*>(m_surface->pixels)[y * m_surface->w + x], fmt, nullptr, &r, &g, &b, &a);
        return (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(g) << 8) | uint32_t(r);
    }

    void setPixel(int x, int y, uint32_t pixel) override {
        if (!m_surface || x < 0 || x >= m_surface->w || y < 0 || y >= m_surface->h) return;
        static_cast<uint32_t*>(m_surface->pixels)[y * m_surface->w + x] = pixel;
    }

    void setBlendMode(BlendMode mode) override {
        if (m_surface) {
            SDL_SetSurfaceBlendMode(m_surface, SDL3Texture::toSDLBlendMode(mode));
        }
    }

    void setAlphaMod(uint8_t alpha) override {
        if (m_surface) {
            SDL_SetSurfaceAlphaMod(m_surface, alpha);
        }
    }

    void blit(Surface* src, int srcX, int srcY, int srcW, int srcH,
              int dstX, int dstY, int dstW, int dstH) override
    {
        if (!m_surface || !src) return;
        SDL3Surface* sdlSrc = static_cast<SDL3Surface*>(src);
        SDL_Rect srcRect = { srcX, srcY, srcW, srcH };
        SDL_Rect dstRect = { dstX, dstY, dstW, dstH };
        SDL_BlitSurfaceScaled(sdlSrc->m_surface, &srcRect, m_surface, &dstRect, SDL_SCALEMODE_NEAREST);
    }

    void blit(Surface* src, int dstX, int dstY) override {
        if (!m_surface || !src) return;
        SDL3Surface* sdlSrc = static_cast<SDL3Surface*>(src);
        SDL_Rect dstRect = { dstX, dstY, 0, 0 };
        SDL_BlitSurface(sdlSrc->m_surface, nullptr, m_surface, &dstRect);
    }

    SharedTexture createTexture(RenderDevice* device) override {
        if (!m_surface || !device) return nullptr;
        SDL_Renderer* renderer = static_cast<SDL3RenderDevice*>(device)->getSDL3Renderer();
        if (!renderer) return nullptr;

        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, m_surface);
        if (!tex) {
            SDL_Log("SDL3Surface::createTexture: %s", SDL_GetError());
            return nullptr;
        }
        return std::make_shared<SDL3Texture>(tex, m_surface->w, m_surface->h);
    }

    SharedSurface rotate(float angle, RenderDevice* device) override {
        if (!m_surface || !device) return nullptr;
        SDL_Renderer* renderer = static_cast<SDL3RenderDevice*>(device)->getSDL3Renderer();
        if (!renderer) return nullptr;

        SDL_Texture* src_tex = SDL_CreateTextureFromSurface(renderer, m_surface);
        if (!src_tex) {
            SDL_Log("SDL3Surface::rotate: SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
            return nullptr;
        }

        SDL_Texture* dst_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                                  SDL_TEXTUREACCESS_TARGET, m_surface->w, m_surface->h);
        if (!dst_tex) {
            SDL_Log("SDL3Surface::rotate: SDL_CreateTexture failed: %s", SDL_GetError());
            SDL_DestroyTexture(src_tex);
            return nullptr;
        }

        bool success = false;
        SDL_Surface* result = nullptr;

        do {
            if (!SDL_SetRenderTarget(renderer, dst_tex)) break;
            if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0)) break;
            if (!SDL_RenderClear(renderer)) break;
            if (!SDL_SetTextureBlendMode(src_tex, SDL_BLENDMODE_BLEND)) break;

            SDL_FRect dst_rect = { 0, 0, (float)m_surface->w, (float)m_surface->h };
            SDL_FPoint center = { (float)m_surface->w / 2, (float)m_surface->h / 2 };

            if (!SDL_RenderTextureRotated(renderer, src_tex, nullptr, &dst_rect, -angle, &center, SDL_FLIP_NONE)) break;
            if (!SDL_RenderPresent(renderer)) break;

            result = SDL_RenderReadPixels(renderer, nullptr);
            if (!result) break;

            success = true;
        } while (false);

        SDL_SetRenderTarget(renderer, nullptr);
        SDL_DestroyTexture(src_tex);
        SDL_DestroyTexture(dst_tex);

        if (!success) {
            if (result) SDL_DestroySurface(result);
            SDL_Log("SDL3Surface::rotate: GPU rotation failed: %s", SDL_GetError());
            return nullptr;
        }

        return std::make_shared<SDL3Surface>(result);
    }

    SDL_Surface* native() const { return m_surface; }

private:
    SDL_Surface* m_surface;
};

// ============================================================
// Surface factory implementations
// ============================================================
std::shared_ptr<Surface> Surface::create(int width, int height) {
    SDL_Surface* s = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888);
    if (!s) {
        SDL_Log("Surface::create: %s", SDL_GetError());
        return nullptr;
    }
    return std::make_shared<SDL3Surface>(s);
}

std::shared_ptr<Surface> Surface::loadFromFile(const std::string& path) {
    SDL_Surface* s = IMG_Load(path.c_str());
    if (!s) {
        SDL_Log("Surface::loadFromFile: %s", SDL_GetError());
        return nullptr;
    }
    return std::make_shared<SDL3Surface>(s);
}

std::shared_ptr<Surface> Surface::loadFromMemory(const void* data, size_t len) {
    SDL_IOStream* stream = SDL_IOFromConstMem(data, len);
    if (!stream) return nullptr;
    SDL_Surface* s = IMG_Load_IO(stream, true);
    if (!s) {
        SDL_Log("Surface::loadFromMemory: %s", SDL_GetError());
        return nullptr;
    }
    return std::make_shared<SDL3Surface>(s);
}

// ============================================================
// Factory entry point
// ============================================================
RenderDevice* CreateSDL3RenderDevice(SDL_Renderer* renderer) {
    return new SDL3RenderDevice(renderer);
}
