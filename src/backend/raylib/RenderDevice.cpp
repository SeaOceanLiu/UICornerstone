#include "RenderDevice.h"
#include "Texture.h"
#include "Surface.h"
#include "RaylibCompat.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

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
    Image img = LoadImageFromMemory(".png", static_cast<const unsigned char*>(data), static_cast<int>(len));
    if (!img.data) {
        printf("RaylibSurface::loadFromMemory: failed\n");
        return nullptr;
    }
    return std::make_shared<RaylibSurface>(img);
}

SharedTexture RaylibSurface::createTexture(RenderDevice* device) {
    (void)device;
    return nullptr;
}

SharedSurface RaylibSurface::rotate(float angle, RenderDevice* device) {
    (void)angle;
    (void)device;
    return nullptr;
}

// ============================================================
// Factory entry point
// ============================================================
RenderDevice* CreateRaylibRenderDevice() {
    return nullptr;
}
