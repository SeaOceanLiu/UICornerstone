#ifndef SCOLOR_H
#define SCOLOR_H

#include <cstdint>
#include <algorithm>
#include "SDL3/SDL.h"

static constexpr float sClamp(float value) {
    return std::max<float>(0.0f, std::min<float>(1.0f, value));
}

class SColor {
public:
    constexpr SColor() : m_r(0.0f), m_g(0.0f), m_b(0.0f), m_a(1.0f) {}

    constexpr SColor(float r, float g, float b, float a = 1.0f) :
        m_r(sClamp(r)), m_g(sClamp(g)), m_b(sClamp(b)), m_a(sClamp(a)) {}

    constexpr SColor(int r, int g, int b, int a = 255) :
        m_r(sClamp(r / 255.0f)), m_g(sClamp(g / 255.0f)),
        m_b(sClamp(b / 255.0f)), m_a(sClamp(a / 255.0f)) {}

    SColor(uint32_t rgba) {
        m_r = ((rgba >> 24) & 0xFF) / 255.0f;
        m_g = ((rgba >> 16) & 0xFF) / 255.0f;
        m_b = ((rgba >> 8) & 0xFF) / 255.0f;
        m_a = (rgba & 0xFF) / 255.0f;
    }

    static constexpr SColor Black(float alpha = 1.0f) { return SColor(0.0f, 0.0f, 0.0f, alpha); }
    static constexpr SColor White(float alpha = 1.0f) { return SColor(1.0f, 1.0f, 1.0f, alpha); }
    static constexpr SColor Red(float alpha = 1.0f) { return SColor(1.0f, 0.0f, 0.0f, alpha); }
    static constexpr SColor Green(float alpha = 1.0f) { return SColor(0.0f, 1.0f, 0.0f, alpha); }
    static constexpr SColor Blue(float alpha = 1.0f) { return SColor(0.0f, 0.0f, 1.0f, alpha); }
    static constexpr SColor Yellow(float alpha = 1.0f) { return SColor(1.0f, 1.0f, 0.0f, alpha); }
    static constexpr SColor Cyan(float alpha = 1.0f) { return SColor(0.0f, 1.0f, 1.0f, alpha); }
    static constexpr SColor Magenta(float alpha = 1.0f) { return SColor(1.0f, 0.0f, 1.0f, alpha); }
    static constexpr SColor Gray(float brightness = 0.5f, float alpha = 1.0f) {
        return SColor(brightness, brightness, brightness, alpha);
    }
    static constexpr SColor Transparent() { return SColor(0.0f, 0.0f, 0.0f, 0.0f); }

    float red() const { return m_r; }
    float green() const { return m_g; }
    float blue() const { return m_b; }
    float alpha() const { return m_a; }

    uint8_t redByte() const { return static_cast<uint8_t>(m_r * 255); }
    uint8_t greenByte() const { return static_cast<uint8_t>(m_g * 255); }
    uint8_t blueByte() const { return static_cast<uint8_t>(m_b * 255); }
    uint8_t alphaByte() const { return static_cast<uint8_t>(m_a * 255); }

    SColor withAlpha(float alpha) const {
        return SColor(m_r, m_g, m_b, sClamp(alpha));
    }

    SColor brighter(float factor = 0.1f) const {
        float f = 1.0f + sClamp(factor);
        return SColor(sClamp(m_r * f), sClamp(m_g * f), sClamp(m_b * f), m_a);
    }

    SColor darker(float factor = 0.1f) const {
        float f = 1.0f - sClamp(factor);
        return SColor(sClamp(m_r * f), sClamp(m_g * f), sClamp(m_b * f), m_a);
    }

    SColor blend(const SColor& other, float ratio = 0.5f) const {
        float r = sClamp(ratio);
        float invR = 1.0f - r;
        return SColor(
            m_r * invR + other.m_r * r,
            m_g * invR + other.m_g * r,
            m_b * invR + other.m_b * r,
            m_a * invR + other.m_a * r
        );
    }

    SDL_Color toSDLColor() const {
        return SDL_Color{redByte(), greenByte(), blueByte(), alphaByte()};
    }
    SDL_FColor toSDLFColor() const {
        return SDL_FColor{m_r, m_g, m_b, m_a};
    }

    bool operator==(const SColor& other) const {
        return m_r == other.m_r && m_g == other.m_g &&
               m_b == other.m_b && m_a == other.m_a;
    }

    bool operator!=(const SColor& other) const {
        return !(*this == other);
    }

private:
    float m_r, m_g, m_b, m_a;
};

#endif
