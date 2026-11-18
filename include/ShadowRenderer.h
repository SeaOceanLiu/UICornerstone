#ifndef SHADOWRENDERER_H
#define SHADOWRENDERER_H

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "RenderDevice.h"
#include "SColor.h"
#include "Utility.h"

class TextRenderer;
class Font;
class UIContext;

// CR20261112001：预烘焙柔光阴影
//
// - 通用控件/菜单的 box-shadow：按规格烘焙一张 RGBA8888 straight-alpha 纹理，
//   9-slice 拉伸复用；同规格跨控件共享。
// - Label 文字柔光：按行烘焙字形 alpha mask，经 CPU 高斯柔化后生成阴影纹理。
//
// 约定：
// - ShadowSpec 的 blur/spread 使用逻辑像素，绘制时乘 effectiveScale 转设备像素；
// - ShadowSpec::cornerRadius 已是设备像素（与既有 drawRoundRect 半径口径一致）；
// - 纹理 RGB = 阴影颜色 RGB，A = coverage * colorAlpha，不预乘；
//   透明 gutter 同样填充 RGB，避免双线性过滤产生暗边。
namespace ShadowRenderer {

struct ShadowSpec {
    bool enabled = false;
    SColor color{0.0f, 0.0f, 0.0f, 0.0f};
    float offsetX = 0.0f;      // 逻辑像素，绘制期应用
    float offsetY = 0.0f;
    float blur = 0.0f;         // 逻辑像素 [0, 64]
    float spread = 0.0f;       // 逻辑像素 [0, ∞)
    float cornerRadius = 0.0f; // 设备像素（与控件实际绘制半径一致）
};

class ShadowCache {
public:
    ShadowCache() = default;
    ~ShadowCache();

    ShadowCache(const ShadowCache&) = delete;
    ShadowCache& operator=(const ShadowCache&) = delete;

    // 绘制控件 rect 外侧柔光阴影（不覆盖 rect 内部、不影响命中）。
    void drawRectShadow(RenderDevice* device, const ShadowSpec& spec,
                        const SRect& deviceRect, float scaleX, float scaleY);

    // Label 逐行字形柔光。成功绘制返回 true；backend 不支持/纹理过大/失败返回 false，
    // 由调用方走有限层字形回退。offset 已烘入纹理，因此也进入缓存键。
    bool drawTextShadow(RenderDevice* device, TextRenderer* textRenderer,
                        const std::shared_ptr<Font>& font,
                        const std::string& text, int fontStyle,
                        uint64_t contentKey, float deviceX, float deviceY,
                        float scaleX, float scaleY, const SColor& color,
                        float blur, float offsetX, float offsetY);

    // 每帧绘制前调用：处理上一帧排队的文字柔光烘焙请求。
    // 原因：文字 mask 需要切换 RenderTarget，帧中切换可能使已绘制窗口内容失效，
    // 因此只在帧起始（clear 前）集中烘焙；当帧先回退有限层字形绘制。
    void processPending();

    // 释放当前缓存的全部纹理（实例销毁/RenderTarget 重建前调用）。
    void clear();

    void setBudgetBytes(size_t bytes) { m_budgetBytes = bytes; }
    size_t budgetBytes() const { return m_budgetBytes; }
    size_t usedBytes() const { return m_usedBytes; }
    size_t textureCount() const { return m_entries.size(); }
    size_t hitCount() const { return m_hits; }
    size_t missCount() const { return m_misses; }

    // 主题/变体切换：递增全局代次，旧纹理不再命中（由 LRU 回收）。
    static void bumpGeneration();
    static uint32_t generation();

private:
    struct Key {
        uint32_t type = 0;   // 0=rect 9-slice, 1=rect exact, 2=text
        uint32_t rgba = 0;
        int width = 0;
        int height = 0;
        float f0 = 0.0f;
        float f1 = 0.0f;
        float f2 = 0.0f;
        float f3 = 0.0f;
        uint64_t extra = 0;

        bool operator==(const Key& o) const {
            return type == o.type && rgba == o.rgba && width == o.width &&
                   height == o.height && f0 == o.f0 && f1 == o.f1 &&
                   f2 == o.f2 && f3 == o.f3 && extra == o.extra;
        }
    };

    struct KeyHash {
        size_t operator()(const Key& k) const;
    };

    struct Entry {
        SharedTexture texture;
        size_t bytes = 0;
        int padX = 0;
        int padY = 0;
        int inset = 0;
        bool nineSlice = false;
        std::list<Key>::iterator lru;
    };

    void ensureDevice(RenderDevice* device);
    Entry* find(const Key& key);
    void insert(const Key& key, const Entry& entry);
    void evictIfNeeded();
    void drawNineSlice(RenderDevice* device, const Entry& entry, const SRect& rect,
                       float offsetX, float offsetY);
    void drawProgrammatic(RenderDevice* device, const ShadowSpec& spec,
                          const SRect& deviceRect, float scaleX, float scaleY);
    SharedTexture bakeRectTexture(RenderDevice* device, const ShadowSpec& spec,
                                  const SRect& deviceRect, float scaleX, float scaleY,
                                  bool nineSlice, int* outPadX, int* outPadY, int* outInset);
    SharedTexture bakeTextTexture(RenderDevice* device, TextRenderer* textRenderer, Font* font,
                                  const std::string& text, int fontStyle,
                                  float scaleX, float scaleY, const SColor& color,
                                  float blur, float offsetX, float offsetY,
                                  int* outPadX, int* outPadY);

    struct PendingTextBake {
        Key key;
        std::shared_ptr<Font> font;
        std::string text;
        int fontStyle = 0;
        SColor color;
        float blur = 0.0f;
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float scaleX = 1.0f;
        float scaleY = 1.0f;
    };

    RenderDevice* m_device = nullptr;
    TextRenderer* m_textRenderer = nullptr;
    std::vector<PendingTextBake> m_pendingText;
    size_t m_budgetBytes = 32u * 1024u * 1024u;
    size_t m_usedBytes = 0;
    size_t m_hits = 0;
    size_t m_misses = 0;
    std::unordered_map<Key, Entry, KeyHash> m_entries;
    std::list<Key> m_lru;   // 前端 = 最近使用
};

// 实例级缓存（懒创建，随 UIContext 销毁释放）。
ShadowCache* getCache(UIContext* context);
void releaseCache(UIContext* context);

} // namespace ShadowRenderer

#endif // SHADOWRENDERER_H
