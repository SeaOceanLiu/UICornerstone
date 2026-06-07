#define NOMINMAX
#include "RenderDevice.h"
#include "Texture.h"
#include "Surface.h"
#include "RaylibCompat.h"
#include "rlgl.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

// nanosvg
#include <string.h>
#include <stdlib.h>
#include <math.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

// ============================================================
// RaylibTexture
// ============================================================
class RaylibTexture : public Texture {
public:
    RaylibTexture(Texture2D tex, int w, int h)
        : m_texture(tex), m_renderTexture({0, {0,0,0,0,0}, {0,0,0,0,0}})
        , m_isRenderTexture(false), m_w(w), m_h(h)
        , m_blendMode(BlendMode::Blend), m_alphaMod(255)
    {
    }

    RaylibTexture(RenderTexture2D rt, int w, int h)
        : m_texture(rt.texture), m_renderTexture(rt)
        , m_isRenderTexture(true), m_w(w), m_h(h)
        , m_blendMode(BlendMode::Blend), m_alphaMod(255)
    {
    }

    ~RaylibTexture() override {
        if (m_isRenderTexture) {
            UnloadRenderTexture(m_renderTexture);
        } else if (m_texture.id != 0) {
            UnloadTexture(m_texture);
        }
    }

    int width() const override { return m_w; }
    int height() const override { return m_h; }

    void setBlendMode(BlendMode mode) override { m_blendMode = mode; }
    void setAlphaMod(uint8_t alpha) override { m_alphaMod = alpha; }
    BlendMode getBlendMode() const override { return m_blendMode; }
    uint8_t getAlphaMod() const override { return m_alphaMod; }

    Texture2D native() const { return m_texture; }
    RenderTexture2D renderTexture() const { return m_renderTexture; }
    bool isRenderTexture() const { return m_isRenderTexture; }

private:
    Texture2D m_texture;
    RenderTexture2D m_renderTexture;
    bool m_isRenderTexture;
    int m_w, m_h;
    BlendMode m_blendMode;
    uint8_t m_alphaMod;
};

// ============================================================
// RaylibSurface
// ============================================================
class RaylibSurface : public Surface {
public:
    RaylibSurface(Image image)
        : m_image(image)
    {
    }

    ~RaylibSurface() override {
        if (m_image.data) {
            UnloadImage(m_image);
        }
    }

    int width() const override { return m_image.width; }
    int height() const override { return m_image.height; }
    void* pixels() override { return m_image.data; }
    const void* pixels() const override { return m_image.data; }

    uint32_t getPixel(int x, int y) const override {
        if (!m_image.data || x < 0 || x >= m_image.width || y < 0 || y >= m_image.height)
            return 0;
        Color c = GetImageColor(m_image, x, y);
        return static_cast<uint32_t>(c.r) |
               (static_cast<uint32_t>(c.g) << 8) |
               (static_cast<uint32_t>(c.b) << 16) |
               (static_cast<uint32_t>(c.a) << 24);
    }

    void setPixel(int x, int y, uint32_t pixel) override {
        if (!m_image.data || x < 0 || x >= m_image.width || y < 0 || y >= m_image.height)
            return;
        Color c;
        c.r = static_cast<unsigned char>(pixel & 0xFF);
        c.g = static_cast<unsigned char>((pixel >> 8) & 0xFF);
        c.b = static_cast<unsigned char>((pixel >> 16) & 0xFF);
        c.a = static_cast<unsigned char>((pixel >> 24) & 0xFF);
        ImageDrawPixel(&m_image, x, y, c);
    }

    void setBlendMode(BlendMode mode) override {
        (void)mode;
    }

    void setAlphaMod(uint8_t alpha) override {
        (void)alpha;
    }

    void blit(Surface* src, int srcX, int srcY, int srcW, int srcH,
              int dstX, int dstY, int dstW, int dstH) override {
        if (!src || !m_image.data) return;
        RaylibSurface* rlSrc = static_cast<RaylibSurface*>(src);
        Rectangle srcRec = { static_cast<float>(srcX), static_cast<float>(srcY),
                             static_cast<float>(srcW), static_cast<float>(srcH) };
        Rectangle dstRec = { static_cast<float>(dstX), static_cast<float>(dstY),
                             static_cast<float>(dstW), static_cast<float>(dstH) };
        ImageDraw(&m_image, rlSrc->m_image, srcRec, dstRec, WHITE);
    }

    void blit(Surface* src, int dstX, int dstY) override {
        blit(src, 0, 0, src->width(), src->height(), dstX, dstY, src->width(), src->height());
    }

    SharedTexture createTexture(RenderDevice* device) override;

    SharedSurface rotate(float angle, RenderDevice* device) override;

private:
    Image m_image;
};

// ============================================================
// Surface factory implementations
// ============================================================
SharedSurface Surface::create(int width, int height) {
    if (width <= 0 || height <= 0) return nullptr;
    Image img = GenImageColor(width, height, BLANK);
    if (!img.data) return nullptr;
    return std::make_shared<RaylibSurface>(img);
}

SharedSurface Surface::loadFromFile(const std::string& path) {
    Image img = LoadImage(path.c_str());
    if (!img.data) {
        printf("RaylibSurface::loadFromFile: failed to load %s\n", path.c_str());
        return nullptr;
    }
    return std::make_shared<RaylibSurface>(img);
}

SharedSurface Surface::loadFromMemory(const void* data, size_t len) {
    if (!data || len == 0) return nullptr;

    const char* str = static_cast<const char*>(data);
    bool isSvg = (len > 4 && (strncmp(str, "<?xm", 4) == 0 ||
                               strncmp(str, "<svg", 4) == 0 ||
                               strncmp(str, "<!DO", 4) == 0));

    if (isSvg) {
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

        Image img = GenImageColor(w, h, BLANK);
        if (!img.data) {
            free(pixels);
            return nullptr;
        }
        // nanosvg outputs straight RGBA — copy directly into the Image data.
        // GenImageColor already allocates the data array at the right format.
        size_t pitch = static_cast<size_t>(w) * 4;
        unsigned char* dst = static_cast<unsigned char*>(img.data);
        for (int y = 0; y < h; y++)
            memcpy(dst + y * pitch, pixels + y * pitch, pitch);

        free(pixels);
        return std::make_shared<RaylibSurface>(img);
    }

    Image img = LoadImageFromMemory(".png", static_cast<const unsigned char*>(data),
                                     static_cast<int>(len));
    if (!img.data) {
        printf("RaylibSurface::loadFromMemory: failed\n");
        return nullptr;
    }
    return std::make_shared<RaylibSurface>(img);
}

SharedTexture RaylibSurface::createTexture(RenderDevice* device) {
    (void)device;
    if (!m_image.data) return nullptr;
    Texture2D tex = LoadTextureFromImage(m_image);
    if (tex.id == 0) return nullptr;
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return std::make_shared<RaylibTexture>(tex, tex.width, tex.height);
}

SharedSurface RaylibSurface::rotate(float angle, RenderDevice* device) {
    (void)device;
    if (!m_image.data || m_image.width <= 0 || m_image.height <= 0)
        return nullptr;

    int w = m_image.width, h = m_image.height;

    // Only support RGBA format (guaranteed by SVG loading path)
    if (m_image.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
        return nullptr;

    Image result = GenImageColor(w, h, BLANK);
    if (!result.data) return nullptr;

    float rad = angle * (3.14159265358979323846f / 180.0f);
    float cos_a = cosf(rad);
    float sin_a = sinf(rad);
    float cx = w / 2.0f;
    float cy = h / 2.0f;

    unsigned char* src = static_cast<unsigned char*>(m_image.data);
    unsigned char* dst = static_cast<unsigned char*>(result.data);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float dx = static_cast<float>(x) - cx;
            float dy = static_cast<float>(y) - cy;
            float sx = dx * cos_a - dy * sin_a + cx;
            float sy = dx * sin_a + dy * cos_a + cy;

            int ix = static_cast<int>(floorf(sx));
            int iy = static_cast<int>(floorf(sy));
            float fx = sx - ix;
            float fy = sy - iy;

            if (ix < 0 || ix >= w - 1 || iy < 0 || iy >= h - 1) continue;

            unsigned char* p00 = src + (iy * w + ix) * 4;
            unsigned char* p10 = src + (iy * w + ix + 1) * 4;
            unsigned char* p01 = src + ((iy + 1) * w + ix) * 4;
            unsigned char* p11 = src + ((iy + 1) * w + ix + 1) * 4;

            unsigned char* out = dst + (y * w + x) * 4;
            for (int c = 0; c < 4; c++) {
                float v = (1 - fx) * (1 - fy) * p00[c] +
                           fx * (1 - fy) * p10[c] +
                          (1 - fx) * fy       * p01[c] +
                           fx * fy            * p11[c];
                out[c] = static_cast<unsigned char>(v + 0.5f);
            }
        }
    }

    return std::make_shared<RaylibSurface>(result);
}

// ============================================================
// RaylibRenderDevice
// ============================================================
class RaylibRenderDevice : public RenderDevice {
public:
    RaylibRenderDevice()
        : m_currentColor({255, 255, 255, 255})
        , m_frameActive(false)
        , m_targetActive(false)
    {
    }

    ~RaylibRenderDevice() override = default;

    void setDrawColor(SColor color) override {
        m_currentColor.r = color.redByte();
        m_currentColor.g = color.greenByte();
        m_currentColor.b = color.blueByte();
        m_currentColor.a = color.alphaByte();
    }

    void setBlendMode(BlendMode mode) override {
        int rlMode;
        switch (mode) {
            case BlendMode::Blend: rlMode = BLEND_ALPHA; break;
            case BlendMode::Add:   rlMode = BLEND_ADDITIVE; break;
            case BlendMode::Mod:
            case BlendMode::Mul:   rlMode = BLEND_MULTIPLIED; break;
            default:
                return;
        }
        BeginBlendMode(rlMode);
    }

    void setClipRect(const SRect& rect) override {
        if (m_scissorActive) EndScissorMode();
        BeginScissorMode(
            static_cast<int>(rect.left),
            static_cast<int>(rect.top),
            static_cast<int>(rect.width),
            static_cast<int>(rect.height));
        m_scissorActive = true;
    }

    void clearClipRect() override {
        if (m_scissorActive) {
            EndScissorMode();
            m_scissorActive = false;
        }
    }

    void fillRect(const SRect& rect) override {
        DrawRectangle(
            static_cast<int>(rect.left),
            static_cast<int>(rect.top),
            static_cast<int>(rect.width),
            static_cast<int>(rect.height),
            m_currentColor);
    }

    void drawRect(const SRect& rect) override {
        DrawRectangleLinesEx(
            {rect.left, rect.top, rect.width, rect.height},
            1.0f,
            m_currentColor);
    }

    void drawLine(float x1, float y1, float x2, float y2) override {
        DrawLineEx({x1, y1}, {x2, y2}, 1.0f, m_currentColor);
    }

    void drawPoint(float x, float y) override {
        DrawPixel(static_cast<int>(x), static_cast<int>(y), m_currentColor);
    }

    void drawTriangles(const Vertex* vertices, int count) override {
        if (count < 3) return;
        rlBegin(RL_TRIANGLES);
        for (int i = 0; i < count; ++i) {
            rlColor4ub(vertices[i].color.redByte(), vertices[i].color.greenByte(),
                       vertices[i].color.blueByte(), vertices[i].color.alphaByte());
            rlVertex2f(vertices[i].x, vertices[i].y);
        }
        rlEnd();
    }

    void drawTriangleStrip(const Vertex* vertices, int count) override {
        if (count < 3) return;
        rlBegin(RL_TRIANGLES);
        for (int i = 0; i < count - 2; ++i) {
            int i0, i1, i2;
            if (i % 2 == 0) { i0 = i; i1 = i + 1; i2 = i + 2; }
            else             { i0 = i + 1; i1 = i; i2 = i + 2; }
            int idxs[3] = {i0, i1, i2};
            for (int j = 0; j < 3; ++j) {
                int idx = idxs[j];
                rlColor4ub(vertices[idx].color.redByte(), vertices[idx].color.greenByte(),
                           vertices[idx].color.blueByte(), vertices[idx].color.alphaByte());
                rlVertex2f(vertices[idx].x, vertices[idx].y);
            }
        }
        rlEnd();
    }

    void drawTriangleFan(const Vertex* vertices, int count) override {
        if (count < 3) return;
        rlBegin(RL_TRIANGLES);
        for (int i = 1; i < count - 1; ++i) {
            int idxs[3] = {0, i, i + 1};
            for (int j = 0; j < 3; ++j) {
                int idx = idxs[j];
                rlColor4ub(vertices[idx].color.redByte(), vertices[idx].color.greenByte(),
                           vertices[idx].color.blueByte(), vertices[idx].color.alphaByte());
                rlVertex2f(vertices[idx].x, vertices[idx].y);
            }
        }
        rlEnd();
    }

    void drawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, SColor color) override {
        Color c = {color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()};
        // raylib DrawTriangle requires CCW winding; ensure it by checking signed area
        // In y-down coordinates: area > 0 → CW, area < 0 → CCW
        float area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
        if (area < 0)
            DrawTriangle({x0, y0}, {x1, y1}, {x2, y2}, c);
        else
            DrawTriangle({x0, y0}, {x2, y2}, {x1, y1}, c);
    }

    void drawQuad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, SColor color) override {
        Color c = {color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()};
        // Split quad into two CCW triangles
        // In y-down coordinates: area > 0 → CW, area < 0 → CCW
        // Triangle (0,3,2): area1 < 0 means it's CCW
        float area1 = (x3 - x0) * (y2 - y0) - (x2 - x0) * (y3 - y0);
        if (area1 < 0)
            DrawTriangle({x0, y0}, {x3, y3}, {x2, y2}, c);
        else
            DrawTriangle({x0, y0}, {x2, y2}, {x3, y3}, c);
        // Triangle (0,2,1): area2 < 0 means it's CCW
        float area2 = (x2 - x0) * (y1 - y0) - (x1 - x0) * (y2 - y0);
        if (area2 < 0)
            DrawTriangle({x0, y0}, {x2, y2}, {x1, y1}, c);
        else
            DrawTriangle({x0, y0}, {x1, y1}, {x2, y2}, c);
    }

    SharedTexture createTextureFromFile(const std::string& path) override {
        Texture2D tex = LoadTexture(path.c_str());
        if (tex.id == 0) return nullptr;
        SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
        return std::make_shared<RaylibTexture>(tex, tex.width, tex.height);
    }

    SharedTexture createTextureFromSurface(Surface* surface) override {
        if (!surface) return nullptr;
        SharedTexture tex = surface->createTexture(this);
        if (tex) {
            RaylibTexture* rlTex = static_cast<RaylibTexture*>(tex.get());
            Texture2D nativeTex = rlTex->native();
            if (nativeTex.id != 0) SetTextureFilter(nativeTex, TEXTURE_FILTER_BILINEAR);
        }
        return tex;
    }

    SharedTexture createRenderTexture(int width, int height) override {
        RenderTexture2D rt = LoadRenderTexture(width, height);
        if (rt.id == 0) return nullptr;
        SetTextureFilter(rt.texture, TEXTURE_FILTER_BILINEAR);
        return std::make_shared<RaylibTexture>(rt, width, height);
    }

    void destroyTexture(Texture* texture) override {
        delete texture;
    }

    void drawTexture(Texture* texture, const SRect* srcRect, const SRect* dstRect) override {
        if (!texture || !dstRect) return;
        RaylibTexture* rlTex = static_cast<RaylibTexture*>(texture);
        Texture2D nativeTex = rlTex->native();
        if (nativeTex.id == 0) return;

        Rectangle src, dst;
        dst = {dstRect->left, dstRect->top, dstRect->width, dstRect->height};
        if (srcRect) {
            src = {srcRect->left, srcRect->top, srcRect->width, srcRect->height};
        } else {
            src = {0, 0, static_cast<float>(nativeTex.width),
                         static_cast<float>(nativeTex.height)};
        }
        Color tint = {255, 255, 255, rlTex->getAlphaMod()};
        DrawTexturePro(nativeTex, src, dst, {0, 0}, 0.0f, tint);
    }

    void drawTextureRotated(Texture* texture, const SRect* srcRect, const SRect* dstRect, float angle) override {
        if (!texture || !dstRect) return;
        RaylibTexture* rlTex = static_cast<RaylibTexture*>(texture);
        Texture2D nativeTex = rlTex->native();
        if (nativeTex.id == 0) return;

        Rectangle src, dst;
        dst = {dstRect->left, dstRect->top, dstRect->width, dstRect->height};
        if (srcRect) {
            src = {srcRect->left, srcRect->top, srcRect->width, srcRect->height};
        } else {
            src = {0, 0, static_cast<float>(nativeTex.width),
                         static_cast<float>(nativeTex.height)};
        }
        Vector2 origin = {dstRect->width / 2, dstRect->height / 2};
        Color tint = {255, 255, 255, rlTex->getAlphaMod()};
        DrawTexturePro(nativeTex, src, dst, origin, angle, tint);
    }

    void setRenderTarget(Texture* texture) override {
        if (m_targetActive) {
            EndTextureMode();
            m_targetActive = false;
        }
        if (!texture) return;
        RaylibTexture* rlTex = static_cast<RaylibTexture*>(texture);
        if (rlTex->isRenderTexture()) {
            BeginTextureMode(rlTex->renderTexture());
            m_targetActive = true;
        }
    }

    void resetRenderTarget() override {
        if (m_targetActive) {
            EndTextureMode();
            m_targetActive = false;
        }
    }

    void readPixels(void* buffer, const SRect& rect) override {
        if (!buffer) return;
        Image img = LoadImageFromScreen();
        if (!img.data) return;
        int bw = static_cast<int>(rect.width);
        int bh = static_cast<int>(rect.height);
        for (int y = 0; y < bh; ++y) {
            for (int x = 0; x < bw; ++x) {
                int sx = static_cast<int>(rect.left) + x;
                int sy = static_cast<int>(rect.top) + y;
                if (sx < 0 || sx >= img.width || sy < 0 || sy >= img.height) continue;
                Color c = GetImageColor(img, sx, sy);
                uint8_t* dst = static_cast<uint8_t*>(buffer) + (y * bw + x) * 4;
                dst[0] = c.r; dst[1] = c.g; dst[2] = c.b; dst[3] = c.a;
            }
        }
        UnloadImage(img);
    }

    void flush() override {
        rlDrawRenderBatchActive();
    }

    void clear() override {
        if (!m_frameActive) {
            BeginDrawing();
            m_frameActive = true;
        }
        ClearBackground(m_currentColor);
    }

    void present() override {
        if (m_frameActive) {
            if (m_scissorActive) {
                EndScissorMode();
                m_scissorActive = false;
            }
            EndDrawing();
            m_frameActive = false;
        }
    }

    void* getNativeHandle() override {
        return nullptr;
    }

private:
    Color m_currentColor;
    bool m_frameActive;
    bool m_targetActive;
    bool m_scissorActive = false;
};

// ============================================================
// Factory entry points
// ============================================================
RenderDevice* CreateRaylibRenderDevice() {
    return new RaylibRenderDevice();
}
