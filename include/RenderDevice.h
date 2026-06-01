#ifndef RENDERDEVICE_H
#define RENDERDEVICE_H

#include "SColor.h"
#include "Utility.h"
#include <string>
#include <memory>

class Texture;
class Surface;
using SharedTexture = std::shared_ptr<Texture>;
using SharedSurface = std::shared_ptr<Surface>;

enum class BlendMode { None, Blend, Add, Mod, Mul };

struct SDL_Renderer;
struct SDL_Surface;

class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    // === 渲染状态 ===
    virtual void setDrawColor(SColor color) = 0;
    virtual void setBlendMode(BlendMode mode) = 0;
    virtual void setClipRect(const SRect& rect) = 0;
    virtual void clearClipRect() = 0;

    // === 基础图元 ===
    virtual void fillRect(const SRect& rect) = 0;
    virtual void drawRect(const SRect& rect) = 0;
    virtual void drawLine(float x1, float y1, float x2, float y2) = 0;
    virtual void drawPoint(float x, float y) = 0;

    // === 复杂图元 ===
    struct Vertex {
        float x, y;
        SColor color;
    };
    virtual void drawTriangles(const Vertex* vertices, int count) = 0;
    virtual void drawTriangleStrip(const Vertex* vertices, int count) = 0;
    virtual void drawTriangleFan(const Vertex* vertices, int count) = 0;

    // 便捷方法：单色三角形和四边形
    virtual void drawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, SColor color) = 0;
    virtual void drawQuad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, SColor color) = 0;

    // === 纹理操作 ===
    virtual SharedTexture createTextureFromFile(const std::string& path) = 0;
    virtual SharedTexture createTextureFromSurface(Surface* surface) = 0;
    virtual SharedTexture createRenderTexture(int width, int height) = 0;
    virtual void destroyTexture(Texture* texture) = 0;
    virtual void drawTexture(Texture* texture, const SRect* srcRect, const SRect* dstRect) = 0;
    virtual void drawTextureRotated(Texture* texture, const SRect* srcRect, const SRect* dstRect, float angle) = 0;

    // 临时桥接方法，Phase 3迁移完成前使用
    virtual SharedTexture createTextureFromSDLSurface(SDL_Surface* surface) = 0;
    virtual SDL_Renderer* getNativeRenderer() = 0;

    // === 渲染到纹理 ===
    virtual void setRenderTarget(Texture* texture) = 0;
    virtual void resetRenderTarget() = 0;
    virtual void readPixels(void* buffer, const SRect& rect) = 0;

    // === 帧操作 ===
    virtual void clear() = 0;
    virtual void present() = 0;
};

// 工厂函数：创建SDL3实现的RenderDevice
RenderDevice* CreateSDL3RenderDevice(SDL_Renderer* renderer);

#endif

