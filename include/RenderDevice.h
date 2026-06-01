#ifndef RENDERDEVICE_H
#define RENDERDEVICE_H

#include "SColor.h"
#include "Utility.h"
#include <memory>
#include <string>

enum class BlendMode { None, Blend, Add, Mod, Mul };

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
    virtual void* createTextureFromFile(const std::string& path) = 0;
    virtual void* createRenderTexture(int width, int height) = 0;
    virtual void destroyTexture(void* texture) = 0;
    virtual void getTextureSize(void* texture, float& outWidth, float& outHeight) = 0;
    virtual void drawTexture(void* texture, const SRect* srcRect, const SRect* dstRect) = 0;
    virtual void drawTextureRotated(void* texture, const SRect* srcRect, const SRect* dstRect, float angle) = 0;

    // === 渲染到纹理 ===
    virtual void setRenderTarget(void* texture) = 0;
    virtual void resetRenderTarget() = 0;
    virtual void readPixels(void* buffer, const SRect& rect) = 0;

    // === 帧操作 ===
    virtual void clear() = 0;
    virtual void present() = 0;
};

using SharedTexture = void*;

// 工厂函数：创建SDL3实现的RenderDevice
struct SDL_Renderer;
RenderDevice* CreateSDL3RenderDevice(SDL_Renderer* renderer);

#endif
