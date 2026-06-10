#include "TextRenderer.h"
#include "RenderDevice.h"
#include "ConstDef.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <memory>
#include <cstring>
#include <string>

class SFMLFont : public Font {
public:
    SFMLFont(sf::Font* font, int size) : m_font(font), m_size(size) {}
    ~SFMLFont() override { delete m_font; }
    int getSize() const override { return m_size; }
    sf::Font* get() const { return m_font; }
private:
    sf::Font* m_font;
    int m_size;
};

// Font cache key: content hash + pixel size.
struct SFMLFontCacheKey {
    size_t contentHash;
    int size;
    bool operator==(const SFMLFontCacheKey& o) const {
        return contentHash == o.contentHash && size == o.size;
    }
};
struct SFMLFontCacheHash {
    size_t operator()(const SFMLFontCacheKey& k) const {
        size_t h = k.contentHash;
        h ^= k.size * 0x9e3779b9;
        return h;
    }
};

class SFMLTextRenderer : public TextRenderer {
public:
    SFMLTextRenderer(RenderDevice* device)
        : m_device(device)
    {
    }

    ~SFMLTextRenderer() override = default;

    SharedFont loadFont(const std::string& path, int size) override {
        size_t contentHash = std::hash<std::string>{}(path);
        SFMLFontCacheKey key{contentHash, size};
        auto it = m_fontCache.find(key);
        if (it != m_fontCache.end()) return it->second;

        auto* font = new sf::Font();
        if (!font->openFromFile(path)) {
            delete font;
            return nullptr;
        }
        auto sfmlFont = std::make_shared<SFMLFont>(font, size);
        m_fontCache[key] = sfmlFont;
        return sfmlFont;
    }

    SharedFont loadFontFromMemory(const void* data, size_t len, int size) override {
        size_t contentHash = std::hash<std::string_view>{}(std::string_view(static_cast<const char*>(data), len));
        SFMLFontCacheKey key{contentHash, size};
        auto it = m_fontCache.find(key);
        if (it != m_fontCache.end()) return it->second;

        auto* font = new sf::Font();
        if (!font->openFromMemory(data, len)) {
            delete font;
            return nullptr;
        }
        auto sfmlFont = std::make_shared<SFMLFont>(font, size);
        m_fontCache[key] = sfmlFont;
        return sfmlFont;
    }

    int getFontHeight(Font* font) override {
        auto* sfmlFont = static_cast<SFMLFont*>(font);
        return sfmlFont->get()->getLineSpacing(static_cast<unsigned>(sfmlFont->getSize()));
    }

    void* createText(Font* font, const std::string& text) override {
        auto* sfmlFont = static_cast<SFMLFont*>(font);
        auto* sfmlText = new sf::Text(*sfmlFont->get(), sf::String::fromUtf8(text.begin(), text.end()),
                                       static_cast<unsigned>(sfmlFont->getSize()));
        return sfmlText;
    }

    void destroyText(void* text) override {
        delete static_cast<sf::Text*>(text);
    }

    SSize measureText(void* text) override {
        if (!text) return SSize(0, 0);
        auto* sfmlText = static_cast<sf::Text*>(text);
        sf::FloatRect bounds = sfmlText->getLocalBounds();
        return SSize(bounds.position.x + bounds.size.x, bounds.size.y);
    }

    void drawText(void* text, float x, float y, SColor color) override {
        if (!text) return;
        m_device->flush();
        auto* sfmlText = static_cast<sf::Text*>(text);
        sfmlText->setPosition(sf::Vector2f(x, y));
        sfmlText->setFillColor(sf::Color(color.redByte(), color.greenByte(),
                                           color.blueByte(), color.alphaByte()));
        auto* target = static_cast<sf::RenderTarget*>(m_device->getNativeHandle());
        if (target) target->draw(*sfmlText);
    }

    void drawText(void* text, float x, float y, float wrapWidth, SColor color) override {
        if (!text) return;
        m_device->flush();
        auto* sfmlText = static_cast<sf::Text*>(text);
        sfmlText->setFillColor(sf::Color(color.redByte(), color.greenByte(),
                                           color.blueByte(), color.alphaByte()));
        auto* target = static_cast<sf::RenderTarget*>(m_device->getNativeHandle());
        if (!target) return;

            auto utf8Str = sfmlText->getString().toUtf8();
            std::string fullStr(utf8Str.begin(), utf8Str.end());
        const sf::Font* font = &sfmlText->getFont();
        unsigned charSize = sfmlText->getCharacterSize();
        float lineSpacing = font->getLineSpacing(charSize);
        float spaceWidth = font->getGlyph(static_cast<char32_t>(' '), charSize, false).advance;
        if (spaceWidth <= 0) spaceWidth = lineSpacing * 0.3f;

        std::vector<std::string> lines;
        std::string currentLine;
        float currentWidth = 0;
        std::istringstream stream(fullStr);
        std::string word;
        bool firstWord = true;

        while (stream >> word) {
            float wordWidth = 0;
            for (char c : word) {
                wordWidth += font->getGlyph(static_cast<char32_t>(static_cast<unsigned char>(c)),
                                            charSize, false).advance;
            }
            if (!firstWord) wordWidth += spaceWidth;

            if (firstWord) {
                currentLine = word;
                currentWidth = wordWidth;
                firstWord = false;
            } else if (currentWidth + wordWidth <= wrapWidth) {
                currentLine += " " + word;
                currentWidth += wordWidth;
            } else {
                lines.push_back(currentLine);
                currentLine = word;
                currentWidth = wordWidth - (firstWord ? 0 : spaceWidth);
                firstWord = false;
            }
        }
        if (!currentLine.empty()) lines.push_back(currentLine);

        float lineY = y;
        for (const auto& line : lines) {
            sf::Text lineText(*font, sf::String::fromUtf8(line.begin(), line.end()), charSize);
            lineText.setPosition(sf::Vector2f(x, lineY));
            lineText.setFillColor(sfmlText->getFillColor());
            target->draw(lineText);
            lineY += lineSpacing;
        }
    }

    SSize measureText(Font* font, const std::string& text) override {
        auto* sfmlFont = static_cast<SFMLFont*>(font);
        sf::Text tmp(*sfmlFont->get(), sf::String::fromUtf8(text.begin(), text.end()),
                      static_cast<unsigned>(sfmlFont->getSize()));
        sf::FloatRect bounds = tmp.getLocalBounds();
        return SSize(bounds.position.x + bounds.size.x, bounds.size.y);
    }

    void drawText(Font* font, const std::string& text, float x, float y, SColor color) override {
        m_device->flush();
        auto* sfmlFont = static_cast<SFMLFont*>(font);
        auto* target = static_cast<sf::RenderTarget*>(m_device->getNativeHandle());
        if (!target) return;

        std::string key = std::to_string(reinterpret_cast<uintptr_t>(sfmlFont->get())) + "|"
                         + std::to_string(sfmlFont->getSize()) + "|" + text;
        auto it = m_textCache.find(key);
        if (it == m_textCache.end()) {
            auto cached = std::make_shared<sf::Text>(
                *sfmlFont->get(), sf::String::fromUtf8(text.begin(), text.end()),
                static_cast<unsigned>(sfmlFont->getSize()));
            it = m_textCache.emplace(key, std::move(cached)).first;
        }
        sf::Text* cachedText = it->second.get();
        cachedText->setPosition(sf::Vector2f(x, y));
        cachedText->setFillColor(sf::Color(color.redByte(), color.greenByte(),
                                            color.blueByte(), color.alphaByte()));
        target->draw(*cachedText);
    }

    void drawText(Font* font, const std::string& text, float x, float y, float wrapWidth, SColor color) override {
        m_device->flush();
        auto* sfmlFont = static_cast<SFMLFont*>(font);
        auto* target = static_cast<sf::RenderTarget*>(m_device->getNativeHandle());
        if (!target) return;

        std::string wrapKey = std::to_string(reinterpret_cast<uintptr_t>(sfmlFont->get())) + "|"
                             + std::to_string(sfmlFont->getSize()) + "|"
                             + std::to_string(static_cast<int>(wrapWidth)) + "|" + text;
        auto wit = m_wrapCache.find(wrapKey);
        if (wit == m_wrapCache.end()) {
            WrapResult result;
            wrapText(*sfmlFont->get(), text, sfmlFont->getSize(), wrapWidth, result);
            wit = m_wrapCache.emplace(wrapKey, std::move(result)).first;
        }
        WrapResult& wrap = wit->second;

        sf::Color c(color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte());
        for (size_t i = 0; i < wrap.lines.size(); ++i) {
            wrap.lines[i]->setPosition(sf::Vector2f(x, y + static_cast<float>(i) * wrap.lineHeight));
            wrap.lines[i]->setFillColor(c);
            target->draw(*wrap.lines[i]);
        }
    }

private:
    struct WrapResult {
        std::vector<std::unique_ptr<sf::Text>> lines;
        float lineHeight = 0;
    };

    static void wrapText(const sf::Font& font, const std::string& text, int size, float wrapWidth,
                         WrapResult& out) {
        unsigned charSize = static_cast<unsigned>(size);
        out.lineHeight = font.getLineSpacing(charSize);
        float spaceWidth = font.getGlyph(static_cast<char32_t>(' '), charSize, false).advance;
        if (spaceWidth <= 0) spaceWidth = out.lineHeight * 0.3f;

        std::vector<std::string> lineStrings;
        std::string currentLine;
        float currentWidth = 0;
        std::istringstream stream(text);
        std::string word;
        bool firstWord = true;

        while (stream >> word) {
            float wordWidth = 0;
            for (char c : word) {
                wordWidth += font.getGlyph(static_cast<char32_t>(static_cast<unsigned char>(c)),
                                           charSize, false).advance;
            }
            if (!firstWord) wordWidth += spaceWidth;

            if (firstWord) {
                currentLine = word;
                currentWidth = wordWidth;
                firstWord = false;
            } else if (currentWidth + wordWidth <= wrapWidth) {
                currentLine += " " + word;
                currentWidth += wordWidth;
            } else {
                lineStrings.push_back(currentLine);
                currentLine = word;
                currentWidth = wordWidth;
            }
        }
        if (!currentLine.empty()) lineStrings.push_back(currentLine);

        out.lines.clear();
        out.lines.reserve(lineStrings.size());
        for (const auto& ls : lineStrings) {
            auto lineText = std::make_unique<sf::Text>(
                font, sf::String::fromUtf8(ls.begin(), ls.end()), charSize);
            out.lines.push_back(std::move(lineText));
        }
    }

    RenderDevice* m_device;
    std::unordered_map<SFMLFontCacheKey, SharedFont, SFMLFontCacheHash> m_fontCache;
    std::unordered_map<std::string, std::shared_ptr<sf::Text>> m_textCache;
    std::unordered_map<std::string, WrapResult> m_wrapCache;
};

TextRenderer* CreateSFMLTextRenderer(RenderDevice* device) {
    return new SFMLTextRenderer(device);
}
