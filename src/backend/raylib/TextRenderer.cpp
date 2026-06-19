#define NOMINMAX
#include "TextRenderer.h"
#include "RenderDevice.h"
#include "ConstDef.h"
#include "RaylibCompat.h"
#include <vector>
#include <sstream>
#include <unordered_map>
#include <memory>
#include <cstring>
#include <cmath>
#include <string>
#include <unordered_set>


// ============================================================
// RaylibFont
// ============================================================
class RaylibFont : public Font {
public:
    RaylibFont(rlFont font, int size)
        : m_font(font), m_size(size)
    {
    }

    ~RaylibFont() override {
        UnloadFont(m_font);
    }

    // Reload the font glyphs in-place without changing the object identity.
    // The old rlFont is unloaded and replaced with newFont.
    void reload(rlFont newFont) {
        UnloadFont(m_font);
        m_font = newFont;
    }

    int getSize() const override { return m_size; }
    rlFont get() const { return m_font; }

private:
    rlFont m_font;
    int m_size;
};

// ============================================================
// Cached text wrapper
// ============================================================
struct RaylibCachedText {
    std::string text;
    SharedFont font;
};

// Font cache key: content hash of font data + pixel size.
// The cached font stores a union of all codepoint sets ever requested.
// When a cache hit occurs but the requested codepoints are not fully loaded,
// the font is reloaded with the merged set (lazy expansion).
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

struct FontCacheEntry {
    SharedFont font;
    int scaledSize;
    std::unordered_set<int> loadedCps; // union of all loaded codepoints

    // Original font data for on-demand reload
    bool fromFile = false;
    std::string path;
    std::vector<char> data; // for memory-loaded fonts
};

// DPI conversion: SDL3_ttf renders points at 96 DPI (pixels = points * 96/72).
// Raylib uses raw pixel sizes, so we scale draw positions and font size by 96/72
// to match SDL3's visual output without affecting layout metrics.
static constexpr float FONT_DPI_SCALE = 96.0f / 72.0f;

// ============================================================
// RaylibTextRenderer
// ============================================================
class RaylibTextRenderer : public TextRenderer {
public:
    explicit RaylibTextRenderer(RenderDevice* device)
        : m_device(device)
    {
    }

    ~RaylibTextRenderer() override = default;

    static std::vector<int> extractCodepoints(const std::string& text) {
        std::vector<int> cps;
        if (text.empty()) {
            // Only ASCII printable; CJK codepoints (~0x4E00-0x9FFF) are
            // loaded on demand when the actual text is known.
            for (int i = 0x20; i <= 0x7E; i++) cps.push_back(i);
            return cps;
        }
        // Always include ASCII printable
        for (int i = 0x20; i <= 0x7E; i++) cps.push_back(i);
        // Decode UTF-8 and add each codepoint
        size_t i = 0;
        while (i < text.size()) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            int cp;
            size_t len;
            if (c < 0x80) { cp = c; len = 1; }
            else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; len = 2; }
            else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; len = 3; }
            else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; len = 4; }
            else { i++; continue; }
            if (i + len > text.size()) break;
            for (size_t j = 1; j < len; j++) {
                cp = (cp << 6) | (static_cast<unsigned char>(text[i + j]) & 0x3F);
            }
            cps.push_back(cp);
            i += len;
        }
        return cps;
    }

    static size_t hashVector(const std::vector<int>& v) {
        size_t h = v.size();
        for (int i : v) h ^= static_cast<size_t>(i) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
    SharedFont loadOrCreate(const void* data, size_t len, int size, const std::vector<int>& cps, bool fromFile, const std::string& path) {
        size_t contentHash = fromFile
            ? std::hash<std::string>{}(path)
            : std::hash<std::string_view>{}(std::string_view(static_cast<const char*>(data), len));
        int scaledSize = static_cast<int>(roundf(size * FONT_DPI_SCALE));
        FontCacheKey key{contentHash, size};
        auto it = m_fontCache.find(key);
        if (it != m_fontCache.end()) {
            // Lazy expansion: if the cached font already has all needed codepoints, reuse it.
            // Otherwise, merge codepoint sets and reload.
            bool needReload = false;
            for (int cp : cps) {
                if (it->second.loadedCps.find(cp) == it->second.loadedCps.end()) {
                    needReload = true;
                    it->second.loadedCps.insert(cp);
                }
            }
            if (!needReload) {
                return it->second.font;
            }

            // Reload with merged codepoint set — update in-place so existing
            // shared_ptr<Font> handles (e.g. from bridge_loadFontFromMemory) stay valid.
            std::vector<int> mergedCps(it->second.loadedCps.begin(), it->second.loadedCps.end());
            rlFont f;
            if (it->second.fromFile) {
                f = LoadFontEx(it->second.path.c_str(), it->second.scaledSize, mergedCps.data(), static_cast<int>(mergedCps.size()));
            } else {
                f = LoadFontFromMemory(".ttf",
                    reinterpret_cast<const unsigned char*>(it->second.data.data()),
                    static_cast<int>(it->second.data.size()),
                    it->second.scaledSize, mergedCps.data(), static_cast<int>(mergedCps.size()));
            }
            if (f.texture.id == 0) return it->second.font; // fallback on failure
            SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
            static_cast<RaylibFont*>(it->second.font.get())->reload(f);
            return it->second.font;
        }

        rlFont f;
        if (fromFile) {
            f = LoadFontEx(path.c_str(), scaledSize, cps.data(), static_cast<int>(cps.size()));
        } else {
            f = LoadFontFromMemory(".ttf", static_cast<const unsigned char*>(data),
                static_cast<int>(len), scaledSize, cps.data(), static_cast<int>(cps.size()));
        }
        if (f.texture.id == 0) {
            return nullptr;
        }
        SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
        auto font = std::make_shared<RaylibFont>(f, scaledSize);
        FontCacheEntry entry;
        entry.font = font;
        entry.scaledSize = scaledSize;
        entry.loadedCps = std::unordered_set<int>(cps.begin(), cps.end());
        entry.fromFile = fromFile;
        if (fromFile) {
            entry.path = path;
        } else {
            entry.data.assign(static_cast<const char*>(data), static_cast<const char*>(data) + len);
        }
        m_fontCache[key] = entry;
        return font;
    }

    // Ensure a font has all codepoints for the given text. Reloads the font
    // on demand if new codepoints are encountered, updating the cache entry.
    void ensureFontCodepoints(SharedFont& fontRef, const std::string& text) {
        if (text.empty() || !fontRef) return;
        // Extract codepoints from text
        auto cps = extractCodepoints(text);

        // Check if all codepoints are already loaded
        for (auto it = m_fontCache.begin(); it != m_fontCache.end(); ++it) {
            if (it->second.font == fontRef) {
                bool needReload = false;
                for (int cp : cps) {
                    if (it->second.loadedCps.find(cp) == it->second.loadedCps.end()) {
                        needReload = true;
                        it->second.loadedCps.insert(cp);
                    }
                }
                if (!needReload) return;

                // Reload with merged set — reload in-place so existing
                // shared_ptr<Font> handles (e.g. from bridge_loadFontFromMemory) stay valid.
                std::vector<int> mergedCps(it->second.loadedCps.begin(), it->second.loadedCps.end());
                rlFont f;
                if (it->second.fromFile) {
                    f = LoadFontEx(it->second.path.c_str(), it->second.scaledSize,
                                   mergedCps.data(), static_cast<int>(mergedCps.size()));
                } else {
                    f = LoadFontFromMemory(".ttf",
                        reinterpret_cast<const unsigned char*>(it->second.data.data()),
                        static_cast<int>(it->second.data.size()),
                        it->second.scaledSize, mergedCps.data(), static_cast<int>(mergedCps.size()));
                }
                if (f.texture.id == 0) return;
                SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
                static_cast<RaylibFont*>(it->second.font.get())->reload(f);
                fontRef = it->second.font;
                return;
            }
        }
    }

    SharedFont loadFont(const std::string& path, int size) override {
        auto cps = extractCodepoints("");
        return loadOrCreate(nullptr, 0, size, cps, true, path);
    }

    SharedFont loadFontFromMemory(const void* data, size_t len, int size) override {
        auto cps = extractCodepoints("");
        return loadOrCreate(data, len, size, cps, false, "");
    }

    SharedFont loadFontWithText(const std::string& path, int size, const std::string& text) override {
        auto cps = extractCodepoints(text);
        return loadOrCreate(nullptr, 0, size, cps, true, path);
    }

    SharedFont loadFontFromMemoryWithText(const void* data, size_t len, int size, const std::string& text) override {
        auto cps = extractCodepoints(text);
        return loadOrCreate(data, len, size, cps, false, "");
    }

    int getFontHeight(Font* font) override {
        auto* rf = static_cast<RaylibFont*>(font);
        // Use actual font measurement instead of a fixed multiplier
        // so the returned height closely matches TTF_GetFontHeight in SDL3_ttf.
        Vector2 sz = MeasureTextEx(rf->get(), "Ay", static_cast<float>(rf->getSize()), 0);
        return static_cast<int>(roundf(sz.y));
    }

    void* createText(Font* font, const std::string& text) override {
        // Find SharedFont from cache that wraps this Font*
        SharedFont sf;
        for (auto& [key, entry] : m_fontCache) {
            if (entry.font.get() == font) {
                sf = entry.font;
                break;
            }
        }
        auto* ct = new RaylibCachedText();
        ct->text = text;
        ct->font = sf;
        return ct;
    }

    void destroyText(void* text) override {
        delete static_cast<RaylibCachedText*>(text);
    }

    SSize measureText(void* text) override {
        if (!text) return SSize(0, 0);
        auto* ct = static_cast<RaylibCachedText*>(text);
        SharedFont font = ct->font;
        ensureFontCodepoints(font, ct->text);
        ct->font = font;
        auto* rf = static_cast<RaylibFont*>(font.get());
        Vector2 sz = MeasureTextEx(rf->get(), ct->text.c_str(),
                                    static_cast<float>(rf->getSize()), 0);
        return SSize(static_cast<int>(sz.x + 0.5f), static_cast<int>(sz.y + 0.5f));
    }

    void drawText(void* text, float x, float y, SColor color) override {
        if (!text) return;
        m_device->flush();
        auto* ct = static_cast<RaylibCachedText*>(text);
        SharedFont font = ct->font;
        ensureFontCodepoints(font, ct->text);
        ct->font = font;
        auto* rf = static_cast<RaylibFont*>(font.get());
        Color c = {color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()};
        DrawTextEx(rf->get(), ct->text.c_str(), {x, y},
                   static_cast<float>(rf->getSize()), 0, c);
    }

    void drawText(void* text, float x, float y, float wrapWidth, SColor color) override {
        if (!text) return;
        m_device->flush();
        auto* ct = static_cast<RaylibCachedText*>(text);
        SharedFont font = ct->font;
        ensureFontCodepoints(font, ct->text);
        ct->font = font;
        auto* rf = static_cast<RaylibFont*>(font.get());
        Color c = {color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()};

        rlFont fnt = rf->get();
        float fontSize = static_cast<float>(rf->getSize());
        float scaledWW = wrapWidth * FONT_DPI_SCALE;
        float lineSpacing = MeasureTextEx(fnt, "Ay", fontSize, 0).y;
        float spaceWidth = MeasureTextEx(fnt, " ", fontSize, 0).x;
        if (spaceWidth <= 0) spaceWidth = lineSpacing * 0.3f;

        std::vector<std::string> lines;
        std::string currentLine;
        float currentWidth = 0;
        std::istringstream stream(ct->text);
        std::string word;
        bool firstWord = true;

        while (stream >> word) {
            float wordWidth = MeasureTextEx(fnt, word.c_str(), fontSize, 0).x;
            if (!firstWord) wordWidth += spaceWidth;

            if (firstWord) {
                currentLine = word;
                currentWidth = wordWidth;
                firstWord = false;
            } else if (currentWidth + wordWidth <= scaledWW) {
                currentLine += " " + word;
                currentWidth += wordWidth;
            } else {
                lines.push_back(currentLine);
                currentLine = word;
                currentWidth = wordWidth;
                firstWord = false;
            }
        }
        if (!currentLine.empty()) lines.push_back(currentLine);

        float lineY = y;
        for (const auto& line : lines) {
            DrawTextEx(fnt, line.c_str(), {x, lineY}, fontSize, 0, c);
            lineY += lineSpacing;
        }
    }

    SSize measureText(Font* font, const std::string& text) override {
        SharedFont sf;
        for (auto& [key, entry] : m_fontCache) {
            if (entry.font.get() == font) {
                sf = entry.font;
                break;
            }
        }
        if (sf) ensureFontCodepoints(sf, text);
        auto* rf = static_cast<RaylibFont*>(sf ? sf.get() : font);
        Vector2 sz = MeasureTextEx(rf->get(), text.c_str(),
                                    static_cast<float>(rf->getSize()), 0);
        return SSize(static_cast<int>(sz.x + 0.5f), static_cast<int>(sz.y + 0.5f));
    }

    void drawText(Font* font, const std::string& text, float x, float y, SColor color) override {
        m_device->flush();
        SharedFont sf;
        for (auto& [key, entry] : m_fontCache) {
            if (entry.font.get() == font) {
                sf = entry.font;
                break;
            }
        }
        if (sf) ensureFontCodepoints(sf, text);
        auto* rf = static_cast<RaylibFont*>(sf ? sf.get() : font);
        Color c = {color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()};
        DrawTextEx(rf->get(), text.c_str(), {x, y},
                   static_cast<float>(rf->getSize()), 0, c);
    }

    void drawText(Font* font, const std::string& text, float x, float y, float wrapWidth, SColor color) override {
        m_device->flush();
        SharedFont sf;
        for (auto& [key, entry] : m_fontCache) {
            if (entry.font.get() == font) {
                sf = entry.font;
                break;
            }
        }
        if (sf) ensureFontCodepoints(sf, text);
        auto* rf = static_cast<RaylibFont*>(sf ? sf.get() : font);
        Color c = {color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()};

        rlFont fnt = rf->get();
        float fontSize = static_cast<float>(rf->getSize());
        float scaledWW = wrapWidth * FONT_DPI_SCALE;
        float lineSpacing = MeasureTextEx(fnt, "Ay", fontSize, 0).y;
        float spaceWidth = MeasureTextEx(fnt, " ", fontSize, 0).x;
        if (spaceWidth <= 0) spaceWidth = lineSpacing * 0.3f;

        std::vector<std::string> lines;
        std::string currentLine;
        float currentWidth = 0;
        std::istringstream stream(text);
        std::string word;
        bool firstWord = true;

        while (stream >> word) {
            float wordWidth = MeasureTextEx(fnt, word.c_str(), fontSize, 0).x;
            if (!firstWord) wordWidth += spaceWidth;

            if (firstWord) {
                currentLine = word;
                currentWidth = wordWidth;
                firstWord = false;
            } else if (currentWidth + wordWidth <= scaledWW) {
                currentLine += " " + word;
                currentWidth += wordWidth;
            } else {
                lines.push_back(currentLine);
                currentLine = word;
                currentWidth = wordWidth;
                firstWord = false;
            }
        }
        if (!currentLine.empty()) lines.push_back(currentLine);

        float lineY = y;
        for (const auto& line : lines) {
            DrawTextEx(fnt, line.c_str(), {x, lineY}, fontSize, 0, c);
            lineY += lineSpacing;
        }
    }

private:
    RenderDevice* m_device;
    std::unordered_map<FontCacheKey, FontCacheEntry, FontCacheHash> m_fontCache;
};

// ============================================================
// Factory entry point
// ============================================================
TextRenderer* CreateRaylibTextRenderer(RenderDevice* device) {
    return new RaylibTextRenderer(device);
}
