#ifndef TEXTDRAW_H
#define TEXTDRAW_H

// 文本绘制助手（P0-26：Label 阴影逻辑抽取，供 StatusBar/TabControl/Menu/List/Tree 等复用）
// 语义：shadowEnabled 时先在 (shadowX, shadowY) 画阴影色文本，再在 (x, y) 画正文色文本。
// 坐标与偏移的缩放由调用方负责（与各自控件坐标系一致）。

#include "TextRenderer.h"
#include "Utility.h"

namespace TextDraw {

inline void withShadow(TextRenderer* r, Font* font, const std::string& text,
                       float x, float y, float shadowX, float shadowY,
                       const SColor& color, bool shadowEnabled, const SColor& shadowColor) {
    if (r == nullptr) return;
    if (shadowEnabled) r->drawText(font, text, shadowX, shadowY, shadowColor);
    r->drawText(font, text, x, y, color);
}

inline void withShadowCached(TextRenderer* r, void* cachedText,
                             float x, float y, float shadowX, float shadowY,
                             const SColor& color, bool shadowEnabled, const SColor& shadowColor) {
    if (r == nullptr) return;
    if (shadowEnabled) r->drawText(cachedText, shadowX, shadowY, shadowColor);
    r->drawText(cachedText, x, y, color);
}

} // namespace TextDraw

#endif
