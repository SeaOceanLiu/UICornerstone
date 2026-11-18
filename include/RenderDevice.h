#ifndef RENDERDEVICE_H
#define RENDERDEVICE_H

#include "SColor.h"
#include "Utility.h"
#include <string>
#include <memory>

struct SDL_Renderer;

class Texture;
class Surface;
using SharedTexture = std::shared_ptr<Texture>;
using SharedSurface = std::shared_ptr<Surface>;

enum class BlendMode { None, Blend, Add, Mod, Mul };

class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    // === 渲染状态 ===
    virtual void setDrawColor(SColor color) = 0;
    virtual void setBlendMode(BlendMode mode) = 0;
    virtual void setClipRect(const SRect& rect) = 0;
    virtual void clearClipRect() = 0;
    virtual void pushClipRect(const SRect& rect) = 0;
    virtual void popClipRect() = 0;

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

    // === 形状图元（基类默认实现：CPU 顶点生成 + 既有纯虚提交；后端零改动自动获得，
    //     可选 override 原生加速。设计：design/Shape_Design.md §4.4） ===
    // 圆角矩形（radius=0 退化为直角）；fill 透明则仅描边；绘制顺序 fill 先于 stroke
    virtual void drawRoundRect(const SRect& rect, float radius, SColor fill, SColor stroke, float lineWidth);
    // 椭圆（rx=ry 即圆）内切语义由调用方保证；ringWidth>0 绘制环带（fill 填充环带、内孔透明），stroke 外缘描边
    virtual void drawEllipse(float cx, float cy, float rx, float ry,
                             SColor fill, SColor stroke, float lineWidth, float ringWidth = 0.0f);
    // 宽线（width<=1 走像素对齐 1px drawLine；>1 端帽四边形边带）
    virtual void drawLine(float x1, float y1, float x2, float y2, float width, SColor color);
    // 折线（开放路径，逐段宽线）
    virtual void drawPolyline(const SPoint* pts, int count, SColor color, float width);
    // 多边形（闭合；凸顶点扇 / 凹耳切三角剖分 PhaseA[无自交无洞] + 沿轮廓宽线描边；fill 透明=仅描边）
    virtual void drawPolygon(const SPoint* pts, int count, SColor fill, SColor stroke, float lineWidth);

    // === 纹理操作 ===
    virtual SharedTexture createTextureFromFile(const std::string& path) = 0;
    virtual SharedTexture createTextureFromSurface(Surface* surface) = 0;
    virtual SharedTexture createRenderTexture(int width, int height) = 0;
    virtual void destroyTexture(Texture* texture) = 0;
    virtual void drawTexture(Texture* texture, const SRect* srcRect, const SRect* dstRect) = 0;
    virtual void drawTextureRotated(Texture* texture, const SRect* srcRect, const SRect* dstRect, float angle) = 0;
    // 纹理采样过滤开关（true=双线性，false=最近邻）；默认实现无操作
    virtual void setTextureFilter(Texture* texture, bool bilinear) { (void)texture; (void)bilinear; }

    // === 渲染到纹理 ===
    virtual void setRenderTarget(Texture* texture) = 0;
    virtual void resetRenderTarget() = 0;
    virtual void readPixels(void* buffer, const SRect& rect) = 0;

    // === CR20261112001：文字阴影离屏烘焙辅助（内部接口，默认不支持） ===
    // 是否支持“渲染到纹理 + 读回 RGBA”用于 Label 字形柔光；不支持时走有限层回退。
    virtual bool supportsTextShadowTexture() const { return false; }
    // 读回整张渲染纹理（RGBA 字节序，top-down）。成功返回 true。
    virtual bool readTexturePixels(Texture* texture, void* buffer, int width, int height) {
        (void)texture; (void)buffer; (void)width; (void)height; return false;
    }
    // 离屏烘焙期间临时保存并禁用裁剪，结束后恢复（含嵌套裁剪栈）。
    virtual void suspendClipRect() {}
    virtual void resumeClipRect() {}

    // === 帧操作 ===
    virtual void clear() = 0;
    virtual void present() = 0;

    // === 后端配置键值入口（后端 C ABI 配置转发落点）===
    // type: 0=string 1=int 2=bool，value 为统一指针（const char* / int*）。
    // 未识别 key 返回 0（与控件属性一致）；各后端实现自己的支持子集。
    virtual int setConfig(const char* key, int type, const void* value) {
        (void)key; (void)type; (void)value; return 0;
    }
    virtual int getConfig(const char* key, int type, void* value, int maxLen) {
        (void)key; (void)type; (void)value; (void)maxLen; return 0;
    }

    // === 批处理刷新（供 TextRenderer 等在直接绘制前刷新批处理）===
    virtual void flush() {}

    // === 原生句柄（供后端实现使用，如 TextRenderer 需要 SDL_Renderer*）===
    virtual void* getNativeHandle() = 0;
};

// 工厂函数：创建SDL3实现的RenderDevice
RenderDevice* CreateSDL3RenderDevice(SDL_Renderer* renderer);

#endif

