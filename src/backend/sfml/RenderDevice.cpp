#include "RenderDevice.h"
#include "Texture.h"
#include "Surface.h"
#include "ConstDef.h"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cmath>

// nanosvg expects these C headers in global namespace
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

class SFMLTexture : public Texture {
    sf::Texture* m_texture;
    sf::RenderTexture* m_renderTexture;
    int m_w, m_h;
    BlendMode m_blendMode;
    std::uint8_t m_alphaMod;
public:
    SFMLTexture(sf::Texture* tex, int w, int h)
        : m_texture(tex), m_renderTexture(nullptr), m_w(w), m_h(h)
        , m_blendMode(BlendMode::Blend), m_alphaMod(255)
    {
        if (m_texture) m_texture->setSmooth(true);
    }

    SFMLTexture(sf::RenderTexture* rt, int w, int h)
        : m_texture(rt ? const_cast<sf::Texture*>(&rt->getTexture()) : nullptr)
        , m_renderTexture(rt), m_w(w), m_h(h)
        , m_blendMode(BlendMode::Blend), m_alphaMod(255)
    {
    }

    ~SFMLTexture() override {
        delete m_renderTexture;
        delete m_texture;
    }

    int width() const override { return m_w; }
    int height() const override { return m_h; }

    void setBlendMode(BlendMode mode) override {
        m_blendMode = mode;
    }

    void setAlphaMod(std::uint8_t alpha) override {
        m_alphaMod = alpha;
    }

    BlendMode getBlendMode() const override { return m_blendMode; }
    std::uint8_t getAlphaMod() const override { return m_alphaMod; }

    sf::Texture* native() const { return m_texture; }
    sf::RenderTexture* renderTexture() const { return m_renderTexture; }

    static sf::BlendMode toSFMLBlendMode(BlendMode mode) {
        switch (mode) {
            case BlendMode::None:  return sf::BlendNone;
            case BlendMode::Blend: return sf::BlendAlpha;
            case BlendMode::Add:   return sf::BlendAdd;
            case BlendMode::Mod:   return sf::BlendMultiply;
            case BlendMode::Mul:   return sf::BlendMultiply;
            default:               return sf::BlendAlpha;
        }
    }
};

// Forward declaration for context activation in SFMLSurface::createTexture
extern sf::RenderWindow* g_sfmlActiveWindow;

class SFMLSurface : public Surface {
    sf::Image* m_image;
public:
    SFMLSurface(sf::Image* image) : m_image(image) {}
    ~SFMLSurface() override { delete m_image; }

    int width() const override { return static_cast<int>(m_image->getSize().x); }
    int height() const override { return static_cast<int>(m_image->getSize().y); }

    void* pixels() override { return const_cast<std::uint8_t*>(m_image->getPixelsPtr()); }
    const void* pixels() const override { return m_image->getPixelsPtr(); }

    std::uint32_t getPixel(int x, int y) const override {
        if (x < 0 || x >= width() || y < 0 || y >= height()) return 0;
        sf::Color c = m_image->getPixel(sf::Vector2u(static_cast<unsigned>(x), static_cast<unsigned>(y)));
        return (std::uint32_t(c.a) << 24) | (std::uint32_t(c.b) << 16) | (std::uint32_t(c.g) << 8) | std::uint32_t(c.r);
    }

    void setPixel(int x, int y, std::uint32_t pixel) override {
        if (x < 0 || x >= width() || y < 0 || y >= height()) return;
        m_image->setPixel(sf::Vector2u(static_cast<unsigned>(x), static_cast<unsigned>(y)),
            sf::Color(static_cast<std::uint8_t>(pixel & 0xFF),
                      static_cast<std::uint8_t>((pixel >> 8) & 0xFF),
                      static_cast<std::uint8_t>((pixel >> 16) & 0xFF),
                      static_cast<std::uint8_t>((pixel >> 24) & 0xFF)));
    }

    void setBlendMode(BlendMode) override {}
    void setAlphaMod(std::uint8_t) override {}

    void blit(Surface* src, int srcX, int srcY, int srcW, int srcH,
              int dstX, int dstY, int dstW, int dstH) override
    {
        if (!m_image || !src) return;
        SFMLSurface* sfmlSrc = static_cast<SFMLSurface*>(src);
        const sf::Image& srcImg = *sfmlSrc->m_image;
        unsigned sW = srcImg.getSize().x, sH = srcImg.getSize().y;
        for (int dy = 0; dy < dstH; ++dy) {
            for (int dx = 0; dx < dstW; ++dx) {
                int sx = srcX + dx * srcW / dstW;
                int sy = srcY + dy * srcH / dstH;
                if (sx < 0 || sx >= static_cast<int>(sW) ||
                    sy < 0 || sy >= static_cast<int>(sH)) continue;
                int dxx = dstX + dx, dyy = dstY + dy;
                if (dxx < 0 || dxx >= width() || dyy < 0 || dyy >= height()) continue;
                m_image->setPixel(sf::Vector2u(static_cast<unsigned>(dxx), static_cast<unsigned>(dyy)),
                    srcImg.getPixel(sf::Vector2u(static_cast<unsigned>(sx), static_cast<unsigned>(sy))));
            }
        }
    }

    void blit(Surface* src, int dstX, int dstY) override {
        blit(src, 0, 0, src->width(), src->height(), dstX, dstY, src->width(), src->height());
    }

    SharedTexture createTexture(RenderDevice* device) override {
        if (!m_image || !device) return nullptr;
        bool ctxOk = true;
        if (g_sfmlActiveWindow)
            ctxOk = g_sfmlActiveWindow->setActive(true);
        auto* tex = new sf::Texture();
        bool loaded = tex->loadFromImage(*m_image);
        printf("SFML: createTexture(Image) ctxOk=%d loaded=%d handle=%u size=%dx%d\n",
               ctxOk, loaded, tex->getNativeHandle(),
               tex->getSize().x, tex->getSize().y);
        fflush(stdout);
        return std::make_shared<SFMLTexture>(tex, width(), height());
    }

    SharedSurface rotate(float angle, RenderDevice* device) override {
        if (!m_image || !device) return nullptr;
        int w = width(), h = height();
        sf::RenderTexture rt(sf::Vector2u(static_cast<unsigned>(w), static_cast<unsigned>(h)));
        rt.clear(sf::Color::Transparent);
        sf::Texture tex;
        (void)tex.loadFromImage(*m_image);
        sf::Sprite sprite(tex);
        sprite.setPosition(sf::Vector2f(static_cast<float>(w) / 2, static_cast<float>(h) / 2));
        sprite.setOrigin(sf::Vector2f(static_cast<float>(w) / 2, static_cast<float>(h) / 2));
        sprite.setRotation(sf::degrees(-angle));
        rt.draw(sprite);
        rt.display();
        auto* result = new sf::Image(rt.getTexture().copyToImage());
        return std::make_shared<SFMLSurface>(result);
    }

    sf::Image* native() const { return m_image; }
};

#ifdef UICORNERSTONE_BUILD_SHARED
// ============================================================
// Surface factory direct implementations (fromsource/plugin path)
// ============================================================
SharedSurface Surface::create(int width, int height) {
    if (width <= 0 || height <= 0) return nullptr;
    auto* img = new sf::Image(sf::Vector2u(static_cast<unsigned>(width), static_cast<unsigned>(height)), sf::Color::Transparent);
    return std::make_shared<SFMLSurface>(img);
}

SharedSurface Surface::loadFromFile(const std::string& path) {
    try {
        auto* img = new sf::Image(std::filesystem::path(path));
        return std::make_shared<SFMLSurface>(img);
    } catch (...) {
        return nullptr;
    }
}

SharedSurface Surface::loadFromMemory(const void* data, size_t len) {
    if (!data || len == 0) return nullptr;

    // Detect SVG by checking for XML/SVG signatures in the first bytes
    const char* str = static_cast<const char*>(data);
    bool isSvg = (len > 4 && (strncmp(str, "<?xm", 4) == 0 ||
                               strncmp(str, "<svg", 4) == 0 ||
                               strncmp(str, "<!DO", 4) == 0));

    if (isSvg) {
        // nanosvg needs a mutable null-terminated copy
        char* svgData = static_cast<char*>(malloc(len + 1));
        if (!svgData) return nullptr;
        memcpy(svgData, data, len);
        svgData[len] = '\0';

        NSVGimage* svgImage = nsvgParse(svgData, "px", 96.0f);
        free(svgData);

        if (!svgImage) return nullptr;

        int w = static_cast<int>(ceilf(svgImage->width));
        int h = static_cast<int>(ceilf(svgImage->height));
        if (w <= 0 || h <= 0) {
            nsvgDelete(svgImage);
            return nullptr;
        }

        unsigned char* pixels = static_cast<unsigned char*>(
            malloc(static_cast<size_t>(w) * h * 4));
        if (!pixels) {
            nsvgDelete(svgImage);
            return nullptr;
        }

        NSVGrasterizer* rast = nsvgCreateRasterizer();
        if (!rast) {
            free(pixels);
            nsvgDelete(svgImage);
            return nullptr;
        }

        nsvgRasterize(rast, svgImage, 0.0f, 0.0f, 1.0f,
                      pixels, w, h, w * 4);

        nsvgDeleteRasterizer(rast);
        nsvgDelete(svgImage);

        auto* img = new sf::Image(
            sf::Vector2u(static_cast<unsigned>(w), static_cast<unsigned>(h)),
            sf::Color::Transparent);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                const unsigned char* src = pixels + (y * w + x) * 4;
                img->setPixel(
                    sf::Vector2u(static_cast<unsigned>(x), static_cast<unsigned>(y)),
                    sf::Color(src[0], src[1], src[2], src[3]));
            }
        }

        free(pixels);
        return std::make_shared<SFMLSurface>(img);
    }

    // Non-SVG: use SFML's built-in image loader
    try {
        auto* img = new sf::Image(data, len);
        if (img->getSize().x == 0 || img->getSize().y == 0) {
            delete img;
            return nullptr;
        }
        return std::make_shared<SFMLSurface>(img);
    } catch (...) {
        return nullptr;
    }
}
#endif // UICORNERSTONE_BUILD_SHARED

// Global SFML window for context activation (used by SFMLSurface::createTexture)
static sf::RenderWindow* g_sfmlActiveWindow = nullptr;

class SFMLRenderDevice : public RenderDevice {
    sf::RenderWindow* m_window;
    sf::RenderTarget* m_target;
    sf::Color m_currentColor;
    bool m_clipEnabled;
    sf::VertexArray m_fillBatch;
    sf::VertexArray m_lineBatch;
    bool m_batchDirty = false;
public:
    SFMLRenderDevice(sf::RenderWindow* window)
        : m_window(window), m_target(window)
        , m_currentColor(sf::Color::White), m_clipEnabled(false)
        , m_fillBatch(sf::PrimitiveType::Triangles)
        , m_lineBatch(sf::PrimitiveType::Lines)
    {
    }

    ~SFMLRenderDevice() override = default;

    void setDrawColor(SColor color) override {
        m_currentColor = sf::Color(color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte());
    }

    void setBlendMode(BlendMode mode) override {
        (void)mode;
        m_batchDirty = false;
    }

    void flushBatches() {
        if (m_fillBatch.getVertexCount() > 0) {
            m_target->draw(m_fillBatch);
            m_fillBatch.clear();
        }
        if (m_lineBatch.getVertexCount() > 0) {
            m_target->draw(m_lineBatch);
            m_lineBatch.clear();
        }
    }

    void setClipRect(const SRect& rect) override {
        flushBatches();
        m_clipEnabled = true;
        glEnable(GL_SCISSOR_TEST);
        int fbH = static_cast<int>(m_target->getSize().y);
        glScissor(static_cast<int>(rect.left), fbH - static_cast<int>(rect.top + rect.height),
                  static_cast<int>(rect.width), static_cast<int>(rect.height));
    }

    void clearClipRect() override {
        flushBatches();
        m_clipEnabled = false;
        glDisable(GL_SCISSOR_TEST);
    }

    void fillRect(const SRect& rect) override {
        // flush pending lines first to preserve z-order across controls
        if (m_lineBatch.getVertexCount() > 0) {
            m_target->draw(m_lineBatch);
            m_lineBatch.clear();
        }
        m_batchDirty = true;
        float l = rect.left, t = rect.top, r = rect.left + rect.width, b = rect.top + rect.height;
        m_fillBatch.append(sf::Vertex{sf::Vector2f(l, t), m_currentColor});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(r, t), m_currentColor});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(l, b), m_currentColor});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(l, b), m_currentColor});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(r, t), m_currentColor});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(r, b), m_currentColor});
    }

    void drawRect(const SRect& rect) override {
        // flush pending fills first to preserve z-order across controls
        if (m_fillBatch.getVertexCount() > 0) {
            m_target->draw(m_fillBatch);
            m_fillBatch.clear();
        }
        m_batchDirty = true;

        // sf::RectangleShape with outline guarantees correct border at all 4 corners.
        // Negative thickness draws inside the rect, matching SDL3/Raylib behavior.
        sf::RectangleShape shape(sf::Vector2f(rect.width, rect.height));
        shape.setPosition(rect.left, rect.top);
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(m_currentColor);
        shape.setOutlineThickness(-1.0f);
        m_target->draw(shape);
    }

    void drawLine(float x1, float y1, float x2, float y2) override {
        // flush pending fills first to preserve z-order
        if (m_fillBatch.getVertexCount() > 0) {
            m_target->draw(m_fillBatch);
            m_fillBatch.clear();
        }
        m_batchDirty = true;
        m_lineBatch.append(sf::Vertex{sf::Vector2f(x1, y1), m_currentColor});
        m_lineBatch.append(sf::Vertex{sf::Vector2f(x2, y2), m_currentColor});
    }

    void drawPoint(float x, float y) override {
        flushBatches();
        sf::Vertex point{sf::Vector2f(x, y), m_currentColor};
        m_target->draw(&point, 1, sf::PrimitiveType::Points);
    }

    void drawTriangles(const Vertex* vertices, int count) override {
        if (count < 3) return;
        if (m_lineBatch.getVertexCount() > 0) {
            m_target->draw(m_lineBatch);
            m_lineBatch.clear();
        }
        m_batchDirty = true;
        for (int i = 0; i < count; ++i) {
            m_fillBatch.append(sf::Vertex{
                sf::Vector2f(vertices[i].x, vertices[i].y),
                sf::Color(vertices[i].color.redByte(), vertices[i].color.greenByte(),
                          vertices[i].color.blueByte(), vertices[i].color.alphaByte())});
        }
    }

    void drawTriangleStrip(const Vertex* vertices, int count) override {
        if (count < 3) return;
        if (m_lineBatch.getVertexCount() > 0) {
            m_target->draw(m_lineBatch);
            m_lineBatch.clear();
        }
        m_batchDirty = true;
        for (int i = 0; i < count; ++i) {
            m_fillBatch.append(sf::Vertex{
                sf::Vector2f(vertices[i].x, vertices[i].y),
                sf::Color(vertices[i].color.redByte(), vertices[i].color.greenByte(),
                          vertices[i].color.blueByte(), vertices[i].color.alphaByte())});
        }
    }

    void drawTriangleFan(const Vertex* vertices, int count) override {
        if (count < 3) return;
        if (m_lineBatch.getVertexCount() > 0) {
            m_target->draw(m_lineBatch);
            m_lineBatch.clear();
        }
        m_batchDirty = true;
        int numTriangles = count - 2;
        for (int i = 0; i < numTriangles; ++i) {
            for (int j = 0; j < 3; ++j) {
                int idx = (j == 0) ? 0 : (i + j - 1);
                m_fillBatch.append(sf::Vertex{
                    sf::Vector2f(vertices[idx].x, vertices[idx].y),
                    sf::Color(vertices[idx].color.redByte(), vertices[idx].color.greenByte(),
                              vertices[idx].color.blueByte(), vertices[idx].color.alphaByte())});
            }
        }
    }

    void drawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, SColor color) override {
        if (m_lineBatch.getVertexCount() > 0) {
            m_target->draw(m_lineBatch);
            m_lineBatch.clear();
        }
        m_batchDirty = true;
        sf::Color c(color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte());
        m_fillBatch.append(sf::Vertex{sf::Vector2f(x0, y0), c});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(x1, y1), c});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(x2, y2), c});
    }

    void drawQuad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, SColor color) override {
        if (m_lineBatch.getVertexCount() > 0) {
            m_target->draw(m_lineBatch);
            m_lineBatch.clear();
        }
        m_batchDirty = true;
        sf::Color c(color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte());
        m_fillBatch.append(sf::Vertex{sf::Vector2f(x0, y0), c});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(x1, y1), c});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(x2, y2), c});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(x0, y0), c});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(x2, y2), c});
        m_fillBatch.append(sf::Vertex{sf::Vector2f(x3, y3), c});
    }

    SharedTexture createTextureFromFile(const std::string& path) override {
        bool ctxOk = true;
        if (g_sfmlActiveWindow) {
            ctxOk = g_sfmlActiveWindow->setActive(true);
        }
        auto* tex = new sf::Texture();
        bool loaded = tex->loadFromFile(path);
        printf("SFML: createTextureFromFile('%s') ctxOk=%d loaded=%d handle=%u size=%dx%d\n",
               path.c_str(), ctxOk, loaded, tex->getNativeHandle(),
               tex->getSize().x, tex->getSize().y);
        if (!loaded) {
            delete tex;
            return nullptr;
        }
        sf::Vector2u size = tex->getSize();
        return std::make_shared<SFMLTexture>(tex, static_cast<int>(size.x), static_cast<int>(size.y));
    }

    SharedTexture createTextureFromSurface(Surface* surface) override {
        if (!surface) return nullptr;
        return surface->createTexture(this);
    }

    SharedTexture createRenderTexture(int width, int height) override {
        auto* rt = new sf::RenderTexture(
            sf::Vector2u(static_cast<unsigned>(width), static_cast<unsigned>(height)));
        return std::make_shared<SFMLTexture>(rt, width, height);
    }

    void destroyTexture(Texture* texture) override {
        delete texture;
    }

    void drawTexture(Texture* texture, const SRect* srcRect, const SRect* dstRect) override {
        flushBatches();
        if (!m_target || !texture || !dstRect) return;
        SFMLTexture* sfmlTex = static_cast<SFMLTexture*>(texture);
        sf::Texture* nativeTex = sfmlTex->native();
        if (!nativeTex) {
            printf("SFML: drawTexture - nativeTex is null!\n"); fflush(stdout);
            return;
        }
        float texW = static_cast<float>(nativeTex->getSize().x);
        float texH = static_cast<float>(nativeTex->getSize().y);
        float dstL = dstRect->left, dstT = dstRect->top;
        sf::Sprite sprite(*nativeTex);
        if (srcRect && srcRect->width > 0 && srcRect->height > 0) {
            sprite.setTextureRect(sf::IntRect(
                sf::Vector2i(static_cast<int>(srcRect->left), static_cast<int>(srcRect->top)),
                sf::Vector2i(static_cast<int>(srcRect->width), static_cast<int>(srcRect->height))));
        }
        sprite.setPosition(sf::Vector2f(dstL, dstT));
        sf::FloatRect bounds = sprite.getLocalBounds();
        if (bounds.size.x > 0 && bounds.size.y > 0) {
            sprite.setScale(sf::Vector2f(
                dstRect->width / bounds.size.x,
                dstRect->height / bounds.size.y));
        }
        sprite.setColor(sf::Color(255, 255, 255, sfmlTex->getAlphaMod()));
        m_target->draw(sprite);
    }

    void drawTextureRotated(Texture* texture, const SRect* srcRect, const SRect* dstRect, float angle) override {
        flushBatches();
        if (!m_target || !texture || !dstRect) return;
        SFMLTexture* sfmlTex = static_cast<SFMLTexture*>(texture);
        sf::Texture* nativeTex = sfmlTex->native();
        if (!nativeTex) return;
        float texW = static_cast<float>(nativeTex->getSize().x);
        float texH = static_cast<float>(nativeTex->getSize().y);
        if (texW <= 0 || texH <= 0) return;
        float cx = dstRect->left + dstRect->width / 2;
        float cy = dstRect->top + dstRect->height / 2;
        float halfW = dstRect->width / 2, halfH = dstRect->height / 2;
        float cosA = cosf(angle * 3.14159265f / 180.0f);
        float sinA = sinf(angle * 3.14159265f / 180.0f);
        auto rot = [&](float dx, float dy) -> sf::Vector2f {
            return sf::Vector2f(cx + dx * cosA - dy * sinA, cy + dx * sinA + dy * cosA);
        };
        float u0, v0, u1, v1;
        if (srcRect && srcRect->width > 0 && srcRect->height > 0) {
            u0 = srcRect->left / texW; v0 = srcRect->top / texH;
            u1 = (srcRect->left + srcRect->width) / texW;
            v1 = (srcRect->top + srcRect->height) / texH;
        } else {
            u0 = 0; v0 = 0; u1 = 1; v1 = 1;
        }
        sf::Color color(255, 255, 255, sfmlTex->getAlphaMod());
        sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);
        quad[0].position = rot(-halfW, -halfH);
        quad[0].color = color;
        quad[0].texCoords = sf::Vector2f(u0, v0);
        quad[1].position = rot( halfW, -halfH);
        quad[1].color = color;
        quad[1].texCoords = sf::Vector2f(u1, v0);
        quad[2].position = rot(-halfW,  halfH);
        quad[2].color = color;
        quad[2].texCoords = sf::Vector2f(u0, v1);
        quad[3].position = rot( halfW,  halfH);
        quad[3].color = color;
        quad[3].texCoords = sf::Vector2f(u1, v1);
        sf::RenderStates states;
        states.texture = nativeTex;
        m_target->draw(quad, states);
    }

    void setRenderTarget(Texture* texture) override {
        flushBatches();
        if (!texture) {
            m_target = m_window;
            return;
        }
        SFMLTexture* sfmlTex = static_cast<SFMLTexture*>(texture);
        if (auto* rt = sfmlTex->renderTexture()) {
            m_target = rt;
        }
    }

    void resetRenderTarget() override {
        flushBatches();
        m_target = m_window;
    }

    void readPixels(void* buffer, const SRect& rect) override {
        flushBatches();
        if (!m_target || !buffer) return;
        sf::RenderTexture rt(sf::Vector2u(m_target->getSize().x, m_target->getSize().y));
        sf::Texture tex = rt.getTexture();
        sf::Image img = tex.copyToImage();
        int bw = static_cast<int>(rect.width);
        int bh = static_cast<int>(rect.height);
        for (int y = 0; y < bh; ++y) {
            for (int x = 0; x < bw; ++x) {
                sf::Color c = img.getPixel(sf::Vector2u(
                    static_cast<unsigned>(rect.left + x),
                    static_cast<unsigned>(rect.top + y)));
                std::uint8_t* dst = static_cast<std::uint8_t*>(buffer) + (y * bw + x) * 4;
                dst[0] = c.r; dst[1] = c.g; dst[2] = c.b; dst[3] = c.a;
            }
        }
    }

    void flush() override {
        if (m_batchDirty) flushBatches();
    }

    void clear() override {
        flushBatches();
        m_target->clear(m_currentColor);
    }

    void present() override {
        flushBatches();
        if (m_window) {
            m_window->display();
        }
    }

    void* getNativeHandle() override { return m_target; }

    sf::RenderWindow* getWindow() const { return m_window; }
    sf::RenderTarget* getTarget() const { return m_target; }
};

RenderDevice* CreateSFMLRenderDevice(sf::RenderWindow* window) {
    g_sfmlActiveWindow = window;
    return new SFMLRenderDevice(window);
}

// ============================================================
// Surface factory registration
// ============================================================
void RegisterSFMLSurfaceFactories() {
    Surface::registerFactories(
        [](int w, int h) -> SharedSurface {
            auto* img = new sf::Image(sf::Vector2u(w, h), sf::Color::Transparent);
            return std::make_shared<SFMLSurface>(img);
        },
        [](const std::string& path) -> SharedSurface {
            try {
                auto* img = new sf::Image(std::filesystem::path(path));
                return std::make_shared<SFMLSurface>(img);
            } catch (...) { return nullptr; }
        },
        [](const void* data, size_t len) -> SharedSurface {
            if (!data || len == 0) return nullptr;
            const char* str = static_cast<const char*>(data);
            bool isSvg = (len > 4 && (strncmp(str, "<?xm", 4) == 0 ||
                                       strncmp(str, "<svg", 4) == 0 ||
                                       strncmp(str, "<!DO", 4) == 0));
            if (isSvg) {
                char* svgData = static_cast<char*>(malloc(len + 1));
                if (!svgData) return nullptr;
                memcpy(svgData, data, len); svgData[len] = '\0';
                NSVGimage* svgImage = nsvgParse(svgData, "px", 96.0f);
                free(svgData);
                if (!svgImage) return nullptr;
                int w = static_cast<int>(ceilf(svgImage->width));
                int h = static_cast<int>(ceilf(svgImage->height));
                if (w <= 0 || h <= 0) { nsvgDelete(svgImage); return nullptr; }
                unsigned char* pixels = static_cast<unsigned char*>(malloc(static_cast<size_t>(w) * h * 4));
                if (!pixels) { nsvgDelete(svgImage); return nullptr; }
                NSVGrasterizer* rast = nsvgCreateRasterizer();
                if (!rast) { free(pixels); nsvgDelete(svgImage); return nullptr; }
                nsvgRasterize(rast, svgImage, 0.0f, 0.0f, 1.0f, pixels, w, h, w * 4);
                nsvgDeleteRasterizer(rast); nsvgDelete(svgImage);
                auto* img = new sf::Image(sf::Vector2u(w, h), sf::Color::Transparent);
                for (int y = 0; y < h; y++)
                    for (int x = 0; x < w; x++) {
                        const unsigned char* src = pixels + (y * w + x) * 4;
                        img->setPixel(sf::Vector2u(x, y), sf::Color(src[0], src[1], src[2], src[3]));
                    }
                free(pixels);
                return std::make_shared<SFMLSurface>(img);
            }
            try {
                auto* img = new sf::Image(data, len);
                if (img->getSize().x == 0 || img->getSize().y == 0) { delete img; return nullptr; }
                return std::make_shared<SFMLSurface>(img);
            } catch (...) { return nullptr; }
        }
    );
}
