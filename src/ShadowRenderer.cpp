#include "ShadowRenderer.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <iterator>
#include <vector>

#include "PlatformUtils.h"
#include "Surface.h"
#include "Texture.h"
#include "TextRenderer.h"
#include "UIContext.h"

namespace ShadowRenderer {

namespace {

constexpr float kMaxBlur = 64.0f;
constexpr float kMaxCorner = 4096.0f;
constexpr int kMaxTextureDim = 4096;
constexpr int kMaxPatchDim = 2048;
constexpr size_t kMaxExactPixels = 4u * 1024u * 1024u;

std::atomic<uint32_t> s_generation{1};

float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

uint32_t colorToRGBA(const SColor& c) {
    return (static_cast<uint32_t>(c.redByte()) << 24) |
           (static_cast<uint32_t>(c.greenByte()) << 16) |
           (static_cast<uint32_t>(c.blueByte()) << 8) |
           static_cast<uint32_t>(c.alphaByte());
}

bool insideRoundedRect(float x, float y, float l, float t, float w, float h, float r) {
    if (w <= 0.0f || h <= 0.0f) return false;
    if (x < l || x > l + w || y < t || y > t + h) return false;
    r = std::max(0.0f, std::min(r, std::min(w, h) * 0.5f));
    if (r <= 0.0001f) return true;
    float cx = std::max(l + r, std::min(l + w - r, x));
    float cy = std::max(t + r, std::min(t + h - r, y));
    float dx = x - cx;
    float dy = y - cy;
    return dx * dx + dy * dy <= r * r;
}

// 圆角矩形欧氏有符号距离（<0 内部，>0 外部）。

// 4x4 超采样覆盖率（抗锯齿；blur=0 时也保持圆角平滑）。
float roundedRectCoverage(float px, float py, float l, float t, float w, float h, float r) {
    constexpr int N = 4;
    int inside = 0;
    for (int sy = 0; sy < N; ++sy) {
        for (int sx = 0; sx < N; ++sx) {
            float x = px + (sx + 0.5f) / N;
            float y = py + (sy + 0.5f) / N;
            if (insideRoundedRect(x, y, l, t, w, h, r)) ++inside;
        }
    }
    return static_cast<float>(inside) / static_cast<float>(N * N);
}

std::vector<float> makeGaussianKernel(float sigma) {
    int radius = static_cast<int>(std::ceil(3.0f * sigma));
    if (radius < 1) radius = 1;
    std::vector<float> kernel(static_cast<size_t>(radius * 2 + 1));
    float sum = 0.0f;
    float inv2s2 = 1.0f / (2.0f * sigma * sigma);
    for (int i = -radius; i <= radius; ++i) {
        float v = std::exp(-static_cast<float>(i * i) * inv2s2);
        kernel[static_cast<size_t>(i + radius)] = v;
        sum += v;
    }
    if (sum > 0.0f) {
        for (float& v : kernel) v /= sum;
    }
    return kernel;
}

void gaussianBlur(std::vector<float>& alpha, int w, int h, float sigma) {
    if (sigma <= 0.0f || w <= 0 || h <= 0) return;
    std::vector<float> kernel = makeGaussianKernel(sigma);
    int radius = static_cast<int>(kernel.size() / 2);
    std::vector<float> tmp(alpha.size());
    std::vector<float> line(static_cast<size_t>(std::max(w, h)));

    // horizontal
    for (int y = 0; y < h; ++y) {
        const float* src = alpha.data() + static_cast<size_t>(y) * w;
        for (int x = 0; x < w; ++x) {
            float sum = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                int sx = x + k;
                if (sx < 0) sx = 0;
                else if (sx >= w) sx = w - 1;
                sum += src[sx] * kernel[static_cast<size_t>(k + radius)];
            }
            line[static_cast<size_t>(x)] = sum;
        }
        std::memcpy(tmp.data() + static_cast<size_t>(y) * w, line.data(),
                    sizeof(float) * static_cast<size_t>(w));
    }

    // vertical
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            float sum = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                int sy = y + k;
                if (sy < 0) sy = 0;
                else if (sy >= h) sy = h - 1;
                sum += tmp[static_cast<size_t>(sy) * w + x] * kernel[static_cast<size_t>(k + radius)];
            }
            alpha[static_cast<size_t>(y) * w + x] = sum;
        }
    }
}

// 从 alpha mask 生成 straight-alpha RGBA8888 Surface（RGB 恒为阴影颜色）。
SharedTexture textureFromMask(RenderDevice* device, const std::vector<float>& alpha,
                              int w, int h, const SColor& color) {
    if (!device || w <= 0 || h <= 0) return nullptr;
    SharedSurface surface = Surface::create(w, h);
    if (!surface) return nullptr;
    const float colorAlpha = color.alpha();
    const uint8_t cr = color.redByte();
    const uint8_t cg = color.greenByte();
    const uint8_t cb = color.blueByte();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float a = clampf(alpha[static_cast<size_t>(y) * w + x], 0.0f, 1.0f);
            int ai = static_cast<int>(a * colorAlpha * 255.0f + 0.5f);
            if (ai < 0) ai = 0;
            if (ai > 255) ai = 255;
            surface->setPixelRGBA(x, y, cr, cg, cb, static_cast<uint8_t>(ai));
        }
    }
    return surface->createTexture(device);
}
} // namespace

// ============================================================
// KeyHash
// ============================================================
size_t ShadowCache::KeyHash::operator()(const Key& k) const {
    auto mix = [](size_t h, size_t v) {
        h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        return h;
    };
    auto bits = [](float f) {
        uint32_t u = 0;
        std::memcpy(&u, &f, sizeof(u));
        return u;
    };
    size_t h = 1469598103934665603ull;
    h = mix(h, std::hash<uint32_t>()(k.type));
    h = mix(h, std::hash<uint32_t>()(k.rgba));
    h = mix(h, std::hash<int>()(k.width));
    h = mix(h, std::hash<int>()(k.height));
    h = mix(h, std::hash<uint32_t>()(bits(k.f0)));
    h = mix(h, std::hash<uint32_t>()(bits(k.f1)));
    h = mix(h, std::hash<uint32_t>()(bits(k.f2)));
    h = mix(h, std::hash<uint32_t>()(bits(k.f3)));
    h = mix(h, std::hash<uint64_t>()(k.extra));
    return h;
}

ShadowCache::~ShadowCache() {
    clear();
}

void ShadowCache::bumpGeneration() {
    s_generation.fetch_add(1, std::memory_order_relaxed);
}

uint32_t ShadowCache::generation() {
    return s_generation.load(std::memory_order_relaxed);
}

void ShadowCache::ensureDevice(RenderDevice* device) {
    if (m_device != device) {
        clear();
        m_device = device;
    }
}
ShadowCache::Entry* ShadowCache::find(const Key& key) {
    auto it = m_entries.find(key);
    if (it == m_entries.end()) return nullptr;
    m_lru.splice(m_lru.begin(), m_lru, it->second.lru);
    ++m_hits;
    return &it->second;
}

void ShadowCache::insert(const Key& key, const Entry& entry) {
    auto existing = m_entries.find(key);
    if (existing != m_entries.end()) {
        m_usedBytes -= existing->second.bytes;
        m_lru.erase(existing->second.lru);
        m_entries.erase(existing);
    }
    Entry copy = entry;
    m_lru.push_front(key);
    copy.lru = m_lru.begin();
    m_entries.emplace(key, std::move(copy));
    m_usedBytes += entry.bytes;
    evictIfNeeded();
}

void ShadowCache::evictIfNeeded() {
    while (m_usedBytes > m_budgetBytes && !m_entries.empty()) {
        const Key& victim = m_lru.back();
        auto it = m_entries.find(victim);
        if (it != m_entries.end()) {
            m_usedBytes -= it->second.bytes;
            m_entries.erase(it);
        }
        m_lru.pop_back();
    }
}

void ShadowCache::clear() {
    m_entries.clear();
    m_lru.clear();
    m_pendingText.clear();
    m_textRenderer = nullptr;
    m_usedBytes = 0;
    m_hits = 0;
    m_misses = 0;
}

void ShadowCache::drawNineSlice(RenderDevice* device, const Entry& entry, const SRect& rect,
                                float offsetX, float offsetY) {
    if (!device || !entry.texture) return;
    int tw = entry.texture->width();
    int th = entry.texture->height();
    int inset = entry.inset;
    int pad = entry.padX;
    int span = tw - inset * 2;
    if (inset <= 0 || pad <= 0 || span < 2 || th != tw) return;

    // 目标外框 = 控件 rect 外扩 pad（阴影最大可见范围）；角区尺寸 = inset
    // （外包 + spread + blur support，向控件内部延伸，避免边条在角部跳变）。
    float l = rect.left - pad + offsetX;
    float t = rect.top - pad + offsetY;
    float r = rect.left + rect.width + pad + offsetX;
    float b = rect.top + rect.height + pad + offsetY;
    if (rect.width <= 0.0f || rect.height <= 0.0f) return;

    float il = static_cast<float>(inset);
    float is = static_cast<float>(inset + span);
    float center = static_cast<float>(inset + span / 2);
    float edgeW = (r - l) - 2.0f * il;
    float edgeH = (b - t) - 2.0f * il;
    if (edgeW < 0.0f || edgeH < 0.0f) return;

    // 四角 1:1；四边取中央直边列/行拉伸；中心透明不绘制。
    SRect srcTL(0.0f, 0.0f, il, il);
    SRect srcTR(is, 0.0f, il, il);
    SRect srcBL(0.0f, is, il, il);
    SRect srcBR(is, is, il, il);
    SRect srcT(center, 0.0f, 1.0f, il);
    SRect srcB(center, is, 1.0f, il);
    SRect srcL(0.0f, center, il, 1.0f);
    SRect srcR(is, center, il, 1.0f);

    SRect dstTL(l, t, il, il);
    SRect dstTR(r - il, t, il, il);
    SRect dstBL(l, b - il, il, il);
    SRect dstBR(r - il, b - il, il, il);
    SRect dstT(l + il, t, edgeW, il);
    SRect dstB(l + il, b - il, edgeW, il);
    SRect dstL(l, t + il, il, edgeH);
    SRect dstR(r - il, t + il, il, edgeH);

    device->drawTexture(entry.texture.get(), &srcTL, &dstTL);
    device->drawTexture(entry.texture.get(), &srcTR, &dstTR);
    device->drawTexture(entry.texture.get(), &srcBL, &dstBL);
    device->drawTexture(entry.texture.get(), &srcBR, &dstBR);
    device->drawTexture(entry.texture.get(), &srcT, &dstT);
    device->drawTexture(entry.texture.get(), &srcB, &dstB);
    device->drawTexture(entry.texture.get(), &srcL, &dstL);
    device->drawTexture(entry.texture.get(), &srcR, &dstR);
}

void ShadowCache::drawProgrammatic(RenderDevice* device, const ShadowSpec& spec,
                                   const SRect& rect, float scaleX, float scaleY) {
    if (!device) return;
    float sx = std::max(0.01f, scaleX);
    float sy = std::max(0.01f, scaleY);
    float blurPx = clampf(spec.blur, 0.0f, kMaxBlur) * std::max(sx, sy);
    float spreadX = std::max(0.0f, spec.spread) * sx;
    float spreadY = std::max(0.0f, spec.spread) * sy;
    float radius = std::max(0.0f, spec.cornerRadius);

    int layers = blurPx > 0.0f ? 4 : 1;
    for (int i = 0; i < layers; ++i) {
        float t = (layers == 1) ? 1.0f : (1.0f - (static_cast<float>(i) / static_cast<float>(layers)));
        float expandX = spreadX + blurPx * 0.5f * (1.0f - t);
        float expandY = spreadY + blurPx * 0.5f * (1.0f - t);
        float a = spec.color.alpha() / static_cast<float>(layers);
        SColor layerColor(spec.color.red(), spec.color.green(), spec.color.blue(), a);
        float shapeRadius = radius > 0.0f ? radius + spreadX : 0.0f;
        SRect r(rect.left - expandX + spec.offsetX * sx,
                rect.top - expandY + spec.offsetY * sy,
                rect.width + 2.0f * expandX,
                rect.height + 2.0f * expandY);
        device->drawRoundRect(r, shapeRadius, layerColor, SColor(0, 0, 0, 0), 0.0f);
    }
}

SharedTexture ShadowCache::bakeRectTexture(RenderDevice* device, const ShadowSpec& spec,
                                           const SRect& deviceRect, float scaleX, float scaleY,
                                           bool nineSlice, int* outPadX, int* outPadY,
                                           int* outInset) {
    if (!device || deviceRect.width <= 0.0f || deviceRect.height <= 0.0f) return nullptr;

    float scale = std::max(0.01f, std::max(scaleX, scaleY));
    float blurPx = clampf(spec.blur, 0.0f, kMaxBlur) * scale;
    float spreadPx = std::max(0.0f, spec.spread) * scale;
    float sigma = blurPx > 0.0f ? blurPx * 0.5f : 0.0f;
    int support = sigma > 0.0f ? static_cast<int>(std::ceil(3.0f * sigma)) : 0;
    int pad = static_cast<int>(std::ceil(spreadPx + static_cast<float>(support))) + 1;

    float corner = clampf(spec.cornerRadius, 0.0f, kMaxCorner);
    int cornerCeil = static_cast<int>(std::ceil(corner));
    int texW = 0;
    int texH = 0;
    float controlW = 0.0f;
    float controlH = 0.0f;
    int inset = 0;

    if (nineSlice) {
        // 9-slice 中央保留直边采样区；角区必须包含 spread + blur support 的
        // “角→直边”过渡，否则拉伸后的边条会在四角产生亮度台阶。
        int centerSpan = support * 2 + 2;
        int spreadCeil = static_cast<int>(std::ceil(spreadPx));
        int inner = spreadCeil + support;
        inset = cornerCeil + pad + inner;
        texW = inset * 2 + centerSpan;
        texH = texW;
        controlW = static_cast<float>(cornerCeil * 2 + inner * 2 + centerSpan);
        controlH = controlW;
        if (outInset) *outInset = inset;
    } else {
        controlW = deviceRect.width;
        controlH = deviceRect.height;
        texW = static_cast<int>(std::ceil(controlW)) + pad * 2;
        texH = static_cast<int>(std::ceil(controlH)) + pad * 2;
        corner = std::min(corner, std::min(controlW, controlH) * 0.5f);
    }

    if (texW <= 0 || texH <= 0 || texW > kMaxTextureDim || texH > kMaxTextureDim) return nullptr;
    if (static_cast<size_t>(texW) * static_cast<size_t>(texH) > kMaxExactPixels) return nullptr;

    std::vector<float> mask(static_cast<size_t>(texW) * texH, 0.0f);

    // 阴影轮廓 = 控件矩形外扩 spread；圆角随 spread 增大。
    float shapeL = static_cast<float>(pad) - spreadPx;
    float shapeT = static_cast<float>(pad) - spreadPx;
    float shapeW = controlW + spreadPx * 2.0f;
    float shapeH = controlH + spreadPx * 2.0f;
    float shapeRadius = (corner > 0.0f) ? (corner + spreadPx) : 0.0f;

    // 真二维高斯柔化：保留最自然的圆润外缘；9-slice 角区已包含
    // spread+support 过渡，边条从直边中心取样，不会在四角产生接缝。
    for (int y = 0; y < texH; ++y) {
        for (int x = 0; x < texW; ++x) {
            mask[static_cast<size_t>(y) * texW + x] =
                roundedRectCoverage(static_cast<float>(x), static_cast<float>(y),
                                    shapeL, shapeT, shapeW, shapeH, shapeRadius);
        }
    }
    if (sigma > 0.0f) {
        gaussianBlur(mask, texW, texH, sigma);
    }

    // outer shadow：裁剪到控件原始圆角矩形内部（不是 spread 外扩矩形），
    // 保证圆角缺口处仍有阴影，同时不覆盖控件内容。
    float clipL = static_cast<float>(pad);
    float clipT = static_cast<float>(pad);
    float clipW = controlW;
    float clipH = controlH;
    for (int y = 0; y < texH; ++y) {
        for (int x = 0; x < texW; ++x) {
            float clipCov = roundedRectCoverage(static_cast<float>(x), static_cast<float>(y),
                                                clipL, clipT, clipW, clipH, corner);
            float& a = mask[static_cast<size_t>(y) * texW + x];
            a *= (1.0f - clipCov);
        }
    }

    SharedTexture texture = textureFromMask(device, mask, texW, texH, spec.color);
    if (texture) {
        texture->setBlendMode(BlendMode::Blend);
        texture->setAlphaMod(255);
        if (outPadX) *outPadX = pad;
        if (outPadY) *outPadY = pad;
    }
    return texture;
}

SharedTexture ShadowCache::bakeTextTexture(RenderDevice* device, TextRenderer* textRenderer,
                                           Font* font, const std::string& text, int fontStyle,
                                           float scaleX, float scaleY,
                                           const SColor& color, float blur,
                                           float offsetX, float offsetY,
                                           int* outPadX, int* outPadY) {
    if (!device || !textRenderer || !font) return nullptr;
    if (!device->supportsTextShadowTexture()) return nullptr;

    float scale = std::max(0.01f, std::max(scaleX, scaleY));
    SSize textSize = textRenderer->measureText(font, text);
    int textW = static_cast<int>(std::ceil(textSize.width));
    int textH = static_cast<int>(std::ceil(textSize.height));
    if (textW <= 0 || textH <= 0) return nullptr;

    float blurPx = clampf(blur, 0.0f, kMaxBlur) * scale;
    float sigma = blurPx > 0.0f ? blurPx * 0.5f : 0.0f;
    int support = sigma > 0.0f ? static_cast<int>(std::ceil(3.0f * sigma)) : 0;
    float offsetXPx = offsetX * scaleX;
    float offsetYPx = offsetY * scaleY;
    int padX = support + static_cast<int>(std::ceil(std::fabs(offsetXPx))) + 2;
    int padY = support + static_cast<int>(std::ceil(std::fabs(offsetYPx))) + 2;
    int texW = textW + padX * 2;
    int texH = textH + padY * 2;
    if (texW <= 0 || texH <= 0 || texW > kMaxTextureDim || texH > kMaxTextureDim) return nullptr;
    if (static_cast<size_t>(texW) * static_cast<size_t>(texH) > kMaxExactPixels) return nullptr;

    SharedTexture target = device->createRenderTexture(texW, texH);
    if (!target) return nullptr;

    device->flush();
    device->suspendClipRect();
    device->setRenderTarget(target.get());
    device->setBlendMode(BlendMode::Blend);
    device->setDrawColor(SColor(0.0f, 0.0f, 0.0f, 0.0f));
    device->clear();
    textRenderer->setFontStyle(font, fontStyle);
    textRenderer->drawText(font, text, static_cast<float>(padX) + offsetXPx,
                           static_cast<float>(padY) + offsetYPx, SColor(1.0f, 1.0f, 1.0f, 1.0f));
    device->flush();

    std::vector<uint8_t> rgba(static_cast<size_t>(texW) * texH * 4, 0);
    bool ok = device->readTexturePixels(target.get(), rgba.data(), texW, texH);
    device->resetRenderTarget();
    device->resumeClipRect();
    if (!ok) return nullptr;

    std::vector<float> mask(static_cast<size_t>(texW) * texH, 0.0f);
    for (size_t i = 0; i < mask.size(); ++i) {
        mask[i] = static_cast<float>(rgba[i * 4 + 3]) / 255.0f;
    }
    if (sigma > 0.0f) {
        gaussianBlur(mask, texW, texH, sigma);
    }

    SharedTexture texture = textureFromMask(device, mask, texW, texH, color);
    if (texture) {
        texture->setBlendMode(BlendMode::Blend);
        texture->setAlphaMod(255);
        if (outPadX) *outPadX = padX;
        if (outPadY) *outPadY = padY;
    }
    return texture;
}

void ShadowCache::drawRectShadow(RenderDevice* device, const ShadowSpec& spec,
                                 const SRect& deviceRect, float scaleX, float scaleY) {
    if (!device || !spec.enabled) return;
    if (spec.color.alpha() <= 0.0f) return;
    if (deviceRect.width <= 0.0f || deviceRect.height <= 0.0f) return;

    ensureDevice(device);
    if (spec.blur < 0.0f || spec.blur > kMaxBlur || spec.spread < 0.0f) {
        static bool logged = false;
        if (!logged) {
            Platform::Log("ShadowRenderer: blur/spread out of range (blur=%.2f spread=%.2f), clamped",
                          spec.blur, spec.spread);
            logged = true;
        }
    }

    float scale = std::max(0.01f, std::max(scaleX, scaleY));
    float blurPx = clampf(spec.blur, 0.0f, kMaxBlur) * scale;
    float spreadPx = std::max(0.0f, spec.spread) * scale;
    float cornerPx = clampf(spec.cornerRadius, 0.0f, kMaxCorner);

    float sigma = blurPx > 0.0f ? blurPx * 0.5f : 0.0f;
    int support = sigma > 0.0f ? static_cast<int>(std::ceil(3.0f * sigma)) : 0;
    int pad = static_cast<int>(std::ceil(spreadPx + static_cast<float>(support))) + 1;
    int spreadCeil = static_cast<int>(std::ceil(spreadPx));
    int inner = spreadCeil + support;
    int inset = static_cast<int>(std::ceil(cornerPx)) + pad + inner;
    int centerSpan = support * 2 + 2;
    int patchDim = inset * 2 + centerSpan;

    bool exact = deviceRect.width < 2.0f * (cornerPx + spreadPx + static_cast<float>(support)) ||
                 deviceRect.height < 2.0f * (cornerPx + spreadPx + static_cast<float>(support)) ||
                 patchDim > kMaxPatchDim;

    Key key;
    key.type = exact ? 1u : 0u;
    key.rgba = colorToRGBA(spec.color);
    key.width = exact ? static_cast<int>(std::ceil(deviceRect.width)) : 0;
    key.height = exact ? static_cast<int>(std::ceil(deviceRect.height)) : 0;
    key.f0 = blurPx;
    key.f1 = spreadPx;
    key.f2 = cornerPx;
    key.f3 = scale;
    key.extra = static_cast<uint64_t>(generation());

    Entry* entry = find(key);
    if (!entry) {
        ++m_misses;
        int padX = pad;
        int padY = pad;
        int inset = 0;
        SharedTexture texture = bakeRectTexture(device, spec, deviceRect, scaleX, scaleY,
                                                !exact, &padX, &padY, &inset);
        if (!texture) {
            drawProgrammatic(device, spec, deviceRect, scaleX, scaleY);
            return;
        }
        Entry newEntry;
        newEntry.texture = texture;
        newEntry.bytes = static_cast<size_t>(texture->width()) *
                         static_cast<size_t>(texture->height()) * 4u;
        newEntry.padX = padX;
        newEntry.padY = padY;
        newEntry.inset = inset;
        newEntry.nineSlice = !exact;
        insert(key, newEntry);
        entry = find(key);
        if (!entry) {
            drawProgrammatic(device, spec, deviceRect, scaleX, scaleY);
            return;
        }
    }

    if (entry->nineSlice) {
        drawNineSlice(device, *entry, deviceRect, spec.offsetX * scaleX, spec.offsetY * scaleY);
    } else {
        SRect dst(deviceRect.left - entry->padX + spec.offsetX * scaleX,
                  deviceRect.top - entry->padY + spec.offsetY * scaleY,
                  static_cast<float>(entry->texture->width()),
                  static_cast<float>(entry->texture->height()));
        device->drawTexture(entry->texture.get(), nullptr, &dst);
    }
}

bool ShadowCache::drawTextShadow(RenderDevice* device, TextRenderer* textRenderer,
                                 const std::shared_ptr<Font>& font,
                                 const std::string& text, int fontStyle,
                                 uint64_t contentKey, float deviceX, float deviceY,
                                 float scaleX, float scaleY, const SColor& color,
                                 float blur, float offsetX, float offsetY) {
    if (!device || !textRenderer || !font || text.empty() || blur <= 0.0f ||
        color.alpha() <= 0.0f) return false;
    if (!device->supportsTextShadowTexture()) return false;

    ensureDevice(device);
    m_textRenderer = textRenderer;

    float scale = std::max(0.01f, std::max(scaleX, scaleY));
    SSize textSize = textRenderer->measureText(font.get(), text);
    int textW = static_cast<int>(std::ceil(textSize.width));
    int textH = static_cast<int>(std::ceil(textSize.height));
    if (textW <= 0 || textH <= 0) return false;

    float blurPx = clampf(blur, 0.0f, kMaxBlur) * scale;
    float sigma = blurPx * 0.5f;
    int support = static_cast<int>(std::ceil(3.0f * sigma));
    float offsetXPx = offsetX * scaleX;
    float offsetYPx = offsetY * scaleY;
    int padX = support + static_cast<int>(std::ceil(std::fabs(offsetXPx))) + 2;
    int padY = support + static_cast<int>(std::ceil(std::fabs(offsetYPx))) + 2;
    int texW = textW + padX * 2;
    int texH = textH + padY * 2;
    if (texW > kMaxTextureDim || texH > kMaxTextureDim) return false;
    if (static_cast<size_t>(texW) * static_cast<size_t>(texH) > kMaxExactPixels) return false;

    Key key;
    key.type = 2u;
    key.rgba = colorToRGBA(color);
    key.width = texW;
    key.height = texH;
    key.f0 = blurPx;
    key.f1 = offsetXPx;
    key.f2 = offsetYPx;
    key.f3 = scale;
    key.extra = contentKey ^ (static_cast<uint64_t>(generation()) << 32);

    Entry* entry = find(key);
    if (!entry) {
        // 帧中不切换 RenderTarget；排队到下一帧起始烘焙，本帧由 Label 走字形回退。
        bool queued = false;
        for (const auto& pending : m_pendingText) {
            if (pending.key == key) { queued = true; break; }
        }
        if (!queued) {
            PendingTextBake pending;
            pending.key = key;
            pending.font = font;
            pending.text = text;
            pending.fontStyle = fontStyle;
            pending.color = color;
            pending.blur = blur;
            pending.offsetX = offsetX;
            pending.offsetY = offsetY;
            pending.scaleX = scaleX;
            pending.scaleY = scaleY;
            m_pendingText.push_back(std::move(pending));
        }
        return false;
    }

    SRect dst(deviceX - entry->padX, deviceY - entry->padY,
              static_cast<float>(entry->texture->width()),
              static_cast<float>(entry->texture->height()));
    device->drawTexture(entry->texture.get(), nullptr, &dst);
    return true;
}

void ShadowCache::processPending() {
    if (m_pendingText.empty() || m_device == nullptr || m_textRenderer == nullptr) return;
    std::vector<PendingTextBake> pending;
    pending.swap(m_pendingText);
    // 限制单帧烘焙数量，避免首帧大量 Label 同时柔化造成长卡顿。
    constexpr size_t kMaxBakesPerFrame = 16;
    size_t count = std::min(pending.size(), kMaxBakesPerFrame);
    for (size_t i = 0; i < count; ++i) {
        auto& req = pending[i];
        if (!req.font) continue;
        int bakedPadX = 0;
        int bakedPadY = 0;
        SharedTexture texture = bakeTextTexture(m_device, m_textRenderer, req.font.get(),
                                                req.text, req.fontStyle,
                                                req.scaleX, req.scaleY, req.color,
                                                req.blur, req.offsetX, req.offsetY,
                                                &bakedPadX, &bakedPadY);
        if (!texture) continue;
        ++m_misses;
        Entry newEntry;
        newEntry.texture = texture;
        newEntry.bytes = static_cast<size_t>(texture->width()) *
                         static_cast<size_t>(texture->height()) * 4u;
        newEntry.padX = bakedPadX;
        newEntry.padY = bakedPadY;
        newEntry.nineSlice = false;
        insert(req.key, newEntry);
    }
    if (pending.size() > count) {
        m_pendingText.insert(m_pendingText.end(),
                             std::make_move_iterator(pending.begin() + count),
                             std::make_move_iterator(pending.end()));
    }
}

ShadowCache* getCache(UIContext* context) {
    if (!context) return nullptr;
    if (!context->shadowCache) {
        context->shadowCache = std::make_shared<ShadowCache>();
    }
    return context->shadowCache.get();
}

void releaseCache(UIContext* context) {
    if (context && context->shadowCache) {
        context->shadowCache->clear();
        context->shadowCache.reset();
    }
}

} // namespace ShadowRenderer
