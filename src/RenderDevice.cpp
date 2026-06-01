#include "RenderDevice.h"
#include "ConstDef.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <vector>

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
        SDL_BlendMode sdlMode;
        switch (mode) {
            case BlendMode::None:  sdlMode = SDL_BLENDMODE_NONE;  break;
            case BlendMode::Blend: sdlMode = SDL_BLENDMODE_BLEND; break;
            case BlendMode::Add:   sdlMode = SDL_BLENDMODE_ADD;   break;
            case BlendMode::Mod:   sdlMode = SDL_BLENDMODE_MOD;   break;
            case BlendMode::Mul:   sdlMode = SDL_BLENDMODE_MUL;   break;
            default:               sdlMode = SDL_BLENDMODE_BLEND; break;
        }
        SDL_SetRenderDrawBlendMode(m_renderer, sdlMode);
    }

    void setClipRect(const SRect& rect) override {
        SDL_Rect sdlRect = {
            static_cast<int>(rect.left),
            static_cast<int>(rect.top),
            static_cast<int>(rect.width),
            static_cast<int>(rect.height)
        };
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
            sdlVerts[i].color = {
                vertices[i].color.red(),
                vertices[i].color.green(),
                vertices[i].color.blue(),
                vertices[i].color.alpha()
            };
            sdlVerts[i].tex_coord = { 0, 0 };
        }
        SDL_RenderGeometry(m_renderer, nullptr, sdlVerts.data(), count, nullptr, 0);
    }

    void drawTriangleStrip(const Vertex* vertices, int count) override {
        if (!m_renderer || count < 3) return;

        std::vector<SDL_Vertex> sdlVerts(count);
        for (int i = 0; i < count; ++i) {
            sdlVerts[i].position = { vertices[i].x, vertices[i].y };
            sdlVerts[i].color = {
                vertices[i].color.red(),
                vertices[i].color.green(),
                vertices[i].color.blue(),
                vertices[i].color.alpha()
            };
            sdlVerts[i].tex_coord = { 0, 0 };
        }

        int numTriangles = (count - 2);
        std::vector<int> indices(numTriangles * 3);
        for (int i = 0; i < numTriangles; ++i) {
            int idx = i * 3;
            if (i % 2 == 0) {
                indices[idx] = i;
                indices[idx + 1] = i + 1;
                indices[idx + 2] = i + 2;
            } else {
                indices[idx] = i + 1;
                indices[idx + 1] = i;
                indices[idx + 2] = i + 2;
            }
        }
        SDL_RenderGeometry(m_renderer, nullptr, sdlVerts.data(), count, indices.data(), numTriangles * 3);
    }

    void drawTriangleFan(const Vertex* vertices, int count) override {
        if (!m_renderer || count < 3) return;

        std::vector<SDL_Vertex> sdlVerts(count);
        for (int i = 0; i < count; ++i) {
            sdlVerts[i].position = { vertices[i].x, vertices[i].y };
            sdlVerts[i].color = {
                vertices[i].color.red(),
                vertices[i].color.green(),
                vertices[i].color.blue(),
                vertices[i].color.alpha()
            };
            sdlVerts[i].tex_coord = { 0, 0 };
        }

        int numTriangles = (count - 2);
        std::vector<int> indices(numTriangles * 3);
        for (int i = 0; i < numTriangles; ++i) {
            int idx = i * 3;
            indices[idx] = 0;
            indices[idx + 1] = i + 1;
            indices[idx + 2] = i + 2;
        }
        SDL_RenderGeometry(m_renderer, nullptr, sdlVerts.data(), count, indices.data(), numTriangles * 3);
    }

    // === 纹理操作 ===
    void* createTextureFromFile(const std::string& path) override {
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            SDL_Log("SDL3RenderDevice::createTexture: IMG_Load failed for '%s': %s", path.c_str(), SDL_GetError());
            return nullptr;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
        SDL_DestroySurface(surface);
        if (!texture) {
            SDL_Log("SDL3RenderDevice::createTexture: SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
            return nullptr;
        }
        return texture;
    }

    void* createRenderTexture(int width, int height) override {
        SDL_Texture* texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
        if (!texture) {
            SDL_Log("SDL3RenderDevice::createRenderTexture: failed: %s", SDL_GetError());
            return nullptr;
        }
        return texture;
    }

    void destroyTexture(void* texture) override {
        if (texture) {
            SDL_DestroyTexture(static_cast<SDL_Texture*>(texture));
        }
    }

    void getTextureSize(void* texture, float& width, float& height) override {
        if (texture) {
            SDL_GetTextureSize(static_cast<SDL_Texture*>(texture), &width, &height);
        } else {
            width = 0;
            height = 0;
        }
    }

    void drawTexture(void* texture, const SRect* srcRect, const SRect* dstRect) override {
        if (!m_renderer || !texture || !dstRect) return;

        SDL_Texture* tex = static_cast<SDL_Texture*>(texture);
        SDL_FRect sdlDst = { dstRect->left, dstRect->top, dstRect->width, dstRect->height };

        if (srcRect) {
            SDL_FRect sdlSrc = { srcRect->left, srcRect->top, srcRect->width, srcRect->height };
            SDL_RenderTexture(m_renderer, tex, &sdlSrc, &sdlDst);
        } else {
            SDL_RenderTexture(m_renderer, tex, nullptr, &sdlDst);
        }
    }

    void drawTextureRotated(void* texture, const SRect* srcRect, const SRect* dstRect, float angle) override {
        if (!m_renderer || !texture || !dstRect) return;

        SDL_Texture* tex = static_cast<SDL_Texture*>(texture);
        SDL_FRect sdlDst = { dstRect->left, dstRect->top, dstRect->width, dstRect->height };

        if (srcRect) {
            SDL_FRect sdlSrc = { srcRect->left, srcRect->top, srcRect->width, srcRect->height };
            SDL_RenderTextureRotated(m_renderer, tex, &sdlSrc, &sdlDst, angle, nullptr, SDL_FLIP_NONE);
        } else {
            SDL_RenderTextureRotated(m_renderer, tex, nullptr, &sdlDst, angle, nullptr, SDL_FLIP_NONE);
        }
    }

    // === 渲染到纹理 ===
    void setRenderTarget(void* texture) override {
        SDL_SetRenderTarget(m_renderer, static_cast<SDL_Texture*>(texture));
    }

    void resetRenderTarget() override {
        SDL_SetRenderTarget(m_renderer, nullptr);
    }

    void readPixels(void* buffer, const SRect& rect) override {
        SDL_Rect sdlRect = {
            static_cast<int>(rect.left),
            static_cast<int>(rect.top),
            static_cast<int>(rect.width),
            static_cast<int>(rect.height)
        };
        SDL_RenderReadPixels(m_renderer, &sdlRect);
    }

    // === 帧操作 ===
    void clear() override {
        SDL_RenderClear(m_renderer);
    }

    void present() override {
        SDL_RenderPresent(m_renderer);
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

    SDL_Renderer* getSDL3Renderer() const { return m_renderer; }

private:
    SDL_Renderer* m_renderer;
    std::string m_lastPath;
};

RenderDevice* CreateSDL3RenderDevice(SDL_Renderer* renderer) {
    return new SDL3RenderDevice(renderer);
}
