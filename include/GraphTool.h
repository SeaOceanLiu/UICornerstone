// 由AI生成，可能不完整或有错误，请自行检查和修改
#ifndef GRAPHTOOL_H
#define GRAPHTOOL_H

#include "SDL3/SDL.h"
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GRAPH_TOOL_DEBUG

#include "Utility.h"

// 首先声明全局命名空间中的类
// Utility.h中的类在全局命名空间中

namespace GraphTool {

// ==================== 颜色系统 ====================

// 基础颜色类 - 抽象的颜色表示（使用SColor避免命名冲突）
class SColor {
public:
    // 构造函数 - 支持多种格式
    SColor() : m_r(0.0f), m_g(0.0f), m_b(0.0f), m_a(1.0f) {} // 默认黑色

    SColor(float r, float g, float b, float a = 1.0f) :
        m_r(clamp(r)), m_g(clamp(g)), m_b(clamp(b)), m_a(clamp(a)) {}

    SColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) :
        m_r(r / 255.0f), m_g(g / 255.0f), m_b(b / 255.0f), m_a(a / 255.0f) {}

    SColor(uint32_t rgba) {
        m_r = ((rgba >> 24) & 0xFF) / 255.0f;
        m_g = ((rgba >> 16) & 0xFF) / 255.0f;
        m_b = ((rgba >> 8) & 0xFF) / 255.0f;
        m_a = (rgba & 0xFF) / 255.0f;
    }

    // 静态工厂方法 - 常用颜色
    static SColor Black(float alpha = 1.0f) { return SColor(0.0f, 0.0f, 0.0f, alpha); }
    static SColor White(float alpha = 1.0f) { return SColor(1.0f, 1.0f, 1.0f, alpha); }
    static SColor Red(float alpha = 1.0f) { return SColor(1.0f, 0.0f, 0.0f, alpha); }
    static SColor Green(float alpha = 1.0f) { return SColor(0.0f, 1.0f, 0.0f, alpha); }
    static SColor Blue(float alpha = 1.0f) { return SColor(0.0f, 0.0f, 1.0f, alpha); }
    static SColor Yellow(float alpha = 1.0f) { return SColor(1.0f, 1.0f, 0.0f, alpha); }
    static SColor Cyan(float alpha = 1.0f) { return SColor(0.0f, 1.0f, 1.0f, alpha); }
    static SColor Magenta(float alpha = 1.0f) { return SColor(1.0f, 0.0f, 1.0f, alpha); }
    static SColor Gray(float brightness = 0.5f, float alpha = 1.0f) {
        return SColor(brightness, brightness, brightness, alpha);
    }
    static SColor Transparent() { return SColor(0.0f, 0.0f, 0.0f, 0.0f); }

    // 获取颜色分量
    float red() const { return m_r; }
    float green() const { return m_g; }
    float blue() const { return m_b; }
    float alpha() const { return m_a; }

    uint8_t redByte() const { return static_cast<uint8_t>(m_r * 255); }
    uint8_t greenByte() const { return static_cast<uint8_t>(m_g * 255); }
    uint8_t blueByte() const { return static_cast<uint8_t>(m_b * 255); }
    uint8_t alphaByte() const { return static_cast<uint8_t>(m_a * 255); }

    // 颜色操作 - alpha调整、变亮/变暗、混合
    SColor withAlpha(float alpha) const {
        return SColor(m_r, m_g, m_b, clamp(alpha));
    }

    // 颜色变亮 - factor控制变亮程度，0.1表示变亮10%，0.5表示变亮50%
    SColor brighter(float factor = 0.1f) const {
        float f = 1.0f + clamp(factor);
        return SColor(clamp(m_r * f), clamp(m_g * f), clamp(m_b * f), m_a);
    }

    // 颜色变暗 - factor控制变暗程度，0.1表示变暗10%，0.5表示变暗50%
    SColor darker(float factor = 0.1f) const {
        float f = 1.0f - clamp(factor);
        return SColor(clamp(m_r * f), clamp(m_g * f), clamp(m_b * f), m_a);
    }

    // 颜色混合 - 线性插值
    SColor blend(const SColor& other, float ratio = 0.5f) const {
        float r = clamp(ratio);
        float invR = 1.0f - r;
        return SColor(
            m_r * invR + other.m_r * r,
            m_g * invR + other.m_g * r,
            m_b * invR + other.m_b * r,
            m_a * invR + other.m_a * r
        );
    }

    // 转换为平台特定格式
    SDL_Color toSDLColor() const {
        return SDL_Color{redByte(), greenByte(), blueByte(), alphaByte()};
    }
    SDL_FColor toSDLFColor() const {
        return SDL_FColor{m_r, m_g, m_b, m_a};
    }

    // 运算符重载
    bool operator==(const SColor& other) const {
        return m_r == other.m_r && m_g == other.m_g &&
               m_b == other.m_b && m_a == other.m_a;
    }

    bool operator!=(const SColor& other) const {
        return !(*this == other);
    }

private:
    float m_r, m_g, m_b, m_a; // 0-1范围的浮点数值

    // 静态工具函数 - 限制值在0-1之间
    static float clamp(float value) {
        return std::max<float>(0.0f, std::min<float>(1.0f, value));
    }
};

// ==================== 画笔样式枚举 ====================
enum class PenStyle {
    Solid,       // 实线 ────────
    Dash,        // 虚线 ── ── ──
    Dot,         // 点线 · · · · · ·
    DashDot,     // 点划线 ── · ── ·
    DashDotDot,  // 双点划线 ── · · ── · ·
    Custom       // 自定义虚线样式
};

// ==================== 线帽样式枚举 ====================
enum class LineCap {
    Flat,    // 平头 - 线条在端点处平截
    Round,   // 圆头 - 线条在端点处添加半圆
    Square   // 方头 - 线条在端点处添加半方形
};

// ==================== 线连接样式枚举 ====================
enum class LineJoin {
    Miter,   // 尖角 - 外边缘延伸相交
    Round,   // 圆角 - 在连接处添加圆弧
    Bevel    // 斜角 - 截断外边缘
};

// ==================== 画刷样式枚举 ====================
enum class BrushStyle {
    None,            // 无填充（透明）
    Solid,           // 实心填充
    LinearGradient,  // 线性渐变
    RadialGradient,  // 径向渐变
    ConicGradient    // 锥形渐变
};

// ==================== 渐变停止点 ====================
struct SGradientStop {
    float position;  // 0.0 - 1.0 之间的位置
    SColor color;    // 该位置的颜色

    SGradientStop() : position(0.0f), color(SColor::Black()) {}
    SGradientStop(float pos, const SColor& c) : position(pos), color(c) {}

    bool operator<(const SGradientStop& other) const {
        return position < other.position;
    }
};

// ==================== 渐变类 ====================
class SGradient {
public:
    SGradient() : m_stops() {}
    SGradient(const std::vector<SGradientStop>& stops) : m_stops(stops) { sortStops(); }

    // 添加渐变停止点
    void addStop(float position, const SColor& color) {
        m_stops.push_back(SGradientStop(position, color));
        sortStops();
    }

    // 获取停止点列表
    const std::vector<SGradientStop>& getStops() const { return m_stops; }

    // 获取指定位置的颜色（线性插值）
    SColor getColorAt(float position) const {
        if (m_stops.empty()) return SColor::Black();
        if (m_stops.size() == 1) return m_stops[0].color;
        if (position <= m_stops.front().position) return m_stops.front().color;
        if (position >= m_stops.back().position) return m_stops.back().color;

        // 找到position所在的两个停止点之间
        for (size_t i = 0; i < m_stops.size() - 1; ++i) {
            if (position >= m_stops[i].position && position <= m_stops[i + 1].position) {
                float range = m_stops[i + 1].position - m_stops[i].position;
                if (range < 0.0001f) return m_stops[i].color;
                float t = (position - m_stops[i].position) / range;
                return m_stops[i].color.blend(m_stops[i + 1].color, t);
            }
        }
        return m_stops.back().color;
    }

    // 停止点数量
    size_t stopCount() const { return m_stops.size(); }

    // 是否为空
    bool isEmpty() const { return m_stops.empty(); }

    // 清空停止点
    void clear() { m_stops.clear(); }

private:
    std::vector<SGradientStop> m_stops;

    void sortStops() {
        std::sort(m_stops.begin(), m_stops.end());
    }
};

// ==================== 线性渐变 ====================
class SLinearGradient : public SGradient {
public:
    SLinearGradient() : SGradient(), m_startX(0), m_startY(0), m_endX(1), m_endY(0) {}
    SLinearGradient(float x1, float y1, float x2, float y2)
        : SGradient(), m_startX(x1), m_startY(y1), m_endX(x2), m_endY(y2) {}

    SLinearGradient(const ::SPoint& start, const ::SPoint& end)
        : SGradient(), m_startX(start.x), m_startY(start.y), m_endX(end.x), m_endY(end.y) {}

    // 设置渐变起止点
    void setStart(float x, float y) { m_startX = x; m_startY = y; }
    void setEnd(float x, float y) { m_endX = x; m_endY = y; }

    float startX() const { return m_startX; }
    float startY() const { return m_startY; }
    float endX() const { return m_endX; }
    float endY() const { return m_endY; }

    // 计算某点在渐变方向上的投影比例（0-1）
    float getProjectionRatio(float x, float y) const {
        float dx = m_endX - m_startX;
        float dy = m_endY - m_startY;
        float len2 = dx * dx + dy * dy;
        if (len2 < 0.0001f) return 0.0f;
        float t = ((x - m_startX) * dx + (y - m_startY) * dy) / len2;
        return std::max<float>(0.0f, std::min<float>(1.0f, t));
    }

private:
    float m_startX, m_startY;
    float m_endX, m_endY;
};

// ==================== 径向渐变 ====================
class SRadialGradient : public SGradient {
public:
    SRadialGradient() : SGradient(), m_cx(0.5f), m_cy(0.5f), m_radius(0.5f), m_focalX(0.5f), m_focalY(0.5f) {}
    SRadialGradient(float cx, float cy, float radius)
        : SGradient(), m_cx(cx), m_cy(cy), m_radius(radius), m_focalX(cx), m_focalY(cy) {}

    SRadialGradient(float cx, float cy, float radius, float fx, float fy)
        : SGradient(), m_cx(cx), m_cy(cy), m_radius(radius), m_focalX(fx), m_focalY(fy) {}

    void setCenter(float x, float y) { m_cx = x; m_cy = y; }
    void setRadius(float r) { m_radius = r; }
    void setFocal(float x, float y) { m_focalX = x; m_focalY = y; }

    float centerX() const { return m_cx; }
    float centerY() const { return m_cy; }
    float radius() const { return m_radius; }
    float focalX() const { return m_focalX; }
    float focalY() const { return m_focalY; }

    // 计算某点在径向渐变中的比例（0-1）
    float getRatio(float x, float y) const {
        float dx = x - m_cx;
        float dy = y - m_cy;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (m_radius < 0.0001f) return 1.0f;
        return std::max<float>(0.0f, std::min<float>(1.0f, dist / m_radius));
    }

private:
    float m_cx, m_cy;
    float m_radius;
    float m_focalX, m_focalY;
};

// ==================== 画笔类 ====================
class SPen {
public:
    // 默认构造函数：黑色实线，宽度1
    SPen() :
        m_color(SColor::Black()),
        m_width(1.0f),
        m_style(PenStyle::Solid),
        m_cap(LineCap::Flat),
        m_join(LineJoin::Miter),
        m_dashOffset(0.0f) {}

    // 指定颜色的构造函数
    SPen(const SColor& color) :
        m_color(color),
        m_width(1.0f),
        m_style(PenStyle::Solid),
        m_cap(LineCap::Flat),
        m_join(LineJoin::Miter),
        m_dashOffset(0.0f) {}

    // 指定颜色和宽度的构造函数
    SPen(const SColor& color, float width) :
        m_color(color),
        m_width(width > 0 ? width : 1.0f),
        m_style(PenStyle::Solid),
        m_cap(LineCap::Flat),
        m_join(LineJoin::Miter),
        m_dashOffset(0.0f) {}

    // 指定颜色、宽度和样式的构造函数
    SPen(const SColor& color, float width, PenStyle style) :
        m_color(color),
        m_width(width > 0 ? width : 1.0f),
        m_style(style),
        m_cap(LineCap::Flat),
        m_join(LineJoin::Miter),
        m_dashOffset(0.0f) {}

    // 完整构造函数
    SPen(const SColor& color, float width, PenStyle style, LineCap cap, LineJoin join) :
        m_color(color),
        m_width(width > 0 ? width : 1.0f),
        m_style(style),
        m_cap(cap),
        m_join(join),
        m_dashOffset(0.0f) {}

    // 颜色设置
    void setColor(const SColor& color) { m_color = color; }
    const SColor& color() const { return m_color; }
    SColor& color() { return m_color; }

    // 宽度设置
    void setWidth(float width) { m_width = width > 0 ? width : 1.0f; }
    float width() const { return m_width; }

    // 样式设置
    void setStyle(PenStyle style) { m_style = style; }
    PenStyle style() const { return m_style; }

    // 线帽设置
    void setCap(LineCap cap) { m_cap = cap; }
    LineCap cap() const { return m_cap; }

    // 线连接设置
    void setJoin(LineJoin join) { m_join = join; }
    LineJoin join() const { return m_join; }

    // 虚线偏移设置
    void setDashOffset(float offset) { m_dashOffset = offset; }
    float dashOffset() const { return m_dashOffset; }

    // 自定义虚线模式设置
    void setDashPattern(const std::vector<float>& pattern) {
        m_dashPattern = pattern;
        m_style = PenStyle::Custom;
    }
    const std::vector<float>& dashPattern() const { return m_dashPattern; }

    // 获取标准虚线模式
    std::vector<float> getStandardDashPattern() const {
        switch (m_style) {
            case PenStyle::Solid:
                return {};
            case PenStyle::Dash:
                return {4.0f * m_width, 2.0f * m_width};
            case PenStyle::Dot:
                return {1.0f * m_width, 2.0f * m_width};
            case PenStyle::DashDot:
                return {4.0f * m_width, 2.0f * m_width, 1.0f * m_width, 2.0f * m_width};
            case PenStyle::DashDotDot:
                return {4.0f * m_width, 2.0f * m_width, 1.0f * m_width, 2.0f * m_width, 1.0f * m_width, 2.0f * m_width};
            case PenStyle::Custom:
                return m_dashPattern;
            default:
                return {};
        }
    }

    // 判断是否为实线
    bool isSolid() const { return m_style == PenStyle::Solid; }

    // 判断画笔是否有效（宽度>0且颜色不透明度为0则无效）
    bool isValid() const { return m_width > 0.0f && m_color.alpha() > 0.0f; }

    // 运算符重载
    bool operator==(const SPen& other) const {
        return m_color == other.m_color &&
               m_width == other.m_width &&
               m_style == other.m_style &&
               m_cap == other.m_cap &&
               m_join == other.m_join &&
               m_dashOffset == other.m_dashOffset &&
               m_dashPattern == other.m_dashPattern;
    }

    bool operator!=(const SPen& other) const {
        return !(*this == other);
    }

    // 静态工厂方法 - 常用画笔
    static SPen NoPen() { return SPen(SColor::Transparent(), 0.0f); }
    static SPen BlackPen(float width = 1.0f) { return SPen(SColor::Black(), width); }
    static SPen WhitePen(float width = 1.0f) { return SPen(SColor::White(), width); }
    static SPen RedPen(float width = 1.0f) { return SPen(SColor::Red(), width); }
    static SPen GreenPen(float width = 1.0f) { return SPen(SColor::Green(), width); }
    static SPen BluePen(float width = 1.0f) { return SPen(SColor::Blue(), width); }
    static SPen DashPen(const SColor& color, float width = 1.0f) {
        return SPen(color, width, PenStyle::Dash);
    }
    static SPen DotPen(const SColor& color, float width = 1.0f) {
        return SPen(color, width, PenStyle::Dot);
    }

private:
    SColor m_color;
    float m_width;
    PenStyle m_style;
    LineCap m_cap;
    LineJoin m_join;
    float m_dashOffset;
    std::vector<float> m_dashPattern;
};

// ==================== 画刷类 ====================
class SBrush {
public:
    // 默认构造函数：黑色实心画刷
    SBrush() :
        m_color(SColor::Black()),
        m_style(BrushStyle::Solid),
        m_gradient() {}

    // 指定颜色的构造函数（实心画刷）
    SBrush(const SColor& color) :
        m_color(color),
        m_style(BrushStyle::Solid),
        m_gradient() {}

    // 指定样式的构造函数
    SBrush(const SColor& color, BrushStyle style) :
        m_color(color),
        m_style(style),
        m_gradient() {}

    // 线性渐变构造函数
    SBrush(const SLinearGradient& gradient) :
        m_color(SColor::Black()),
        m_style(BrushStyle::LinearGradient),
        m_linearGradient(gradient),
        m_gradient() {}

    // 径向渐变构造函数
    SBrush(const SRadialGradient& gradient) :
        m_color(SColor::Black()),
        m_style(BrushStyle::RadialGradient),
        m_radialGradient(gradient),
        m_gradient() {}

    // 颜色设置
    void setColor(const SColor& color) { m_color = color; }
    const SColor& color() const { return m_color; }
    SColor& color() { return m_color; }

    // 样式设置
    void setStyle(BrushStyle style) { m_style = style; }
    BrushStyle style() const { return m_style; }

    // 线性渐变设置
    void setLinearGradient(const SLinearGradient& gradient) {
        m_linearGradient = gradient;
        m_style = BrushStyle::LinearGradient;
    }
    const SLinearGradient& linearGradient() const { return m_linearGradient; }
    SLinearGradient& linearGradient() { return m_linearGradient; }

    // 径向渐变设置
    void setRadialGradient(const SRadialGradient& gradient) {
        m_radialGradient = gradient;
        m_style = BrushStyle::RadialGradient;
    }
    const SRadialGradient& radialGradient() const { return m_radialGradient; }
    SRadialGradient& radialGradient() { return m_radialGradient; }

    // 判断画刷是否有效
    bool isValid() const {
        if (m_style == BrushStyle::None) return false;
        if (m_style == BrushStyle::Solid) return m_color.alpha() > 0.0f;
        return true; // 渐变画刷总是有效的
    }

    // 判断是否为透明画刷
    bool isTransparent() const {
        return m_style == BrushStyle::None ||
               (m_style == BrushStyle::Solid && m_color.alpha() <= 0.0f);
    }

    // 获取指定位置的颜色（用于渐变）
    SColor getColorAt(float x, float y) const {
        switch (m_style) {
            case BrushStyle::None:
                return SColor::Transparent();
            case BrushStyle::Solid:
                return m_color;
            case BrushStyle::LinearGradient:
                return m_linearGradient.getColorAt(
                    m_linearGradient.getProjectionRatio(x, y));
            case BrushStyle::RadialGradient:
                return m_radialGradient.getColorAt(
                    m_radialGradient.getRatio(x, y));
            case BrushStyle::ConicGradient:
                return m_gradient.getColorAt(0.5f); // 简化处理
            default:
                return m_color;
        }
    }

    // 运算符重载
    bool operator==(const SBrush& other) const {
        return m_color == other.m_color && m_style == other.m_style;
    }

    bool operator!=(const SBrush& other) const {
        return !(*this == other);
    }

    // 静态工厂方法 - 常用画刷
    static SBrush NoBrush() { return SBrush(SColor::Transparent(), BrushStyle::None); }
    static SBrush BlackBrush() { return SBrush(SColor::Black()); }
    static SBrush WhiteBrush() { return SBrush(SColor::White()); }
    static SBrush RedBrush() { return SBrush(SColor::Red()); }
    static SBrush GreenBrush() { return SBrush(SColor::Green()); }
    static SBrush BlueBrush() { return SBrush(SColor::Blue()); }

private:
    SColor m_color;
    BrushStyle m_style;
    SLinearGradient m_linearGradient;
    SRadialGradient m_radialGradient;
    SGradient m_gradient; // 通用渐变（用于锥形等）
};

// ==================== 圆角半径结构 ====================
struct SRoundedCorners {
    float topLeft;
    float topRight;
    float bottomRight;
    float bottomLeft;

    SRoundedCorners(float allCorners) :
        topLeft(allCorners), topRight(allCorners),
        bottomRight(allCorners), bottomLeft(allCorners) {}

    SRoundedCorners(float tl, float tr, float br, float bl) :
        topLeft(tl), topRight(tr), bottomRight(br), bottomLeft(bl) {}
};

// ==================== 线段矩形点结构 ====================
struct SLineRectPoints {
    ::SPoint startLeft;    // 起点左旋转90度平移半线宽的点
    ::SPoint startRight;   // 起点右旋转90度平移半线宽的点
    ::SPoint endLeft;      // 终点左旋转90度平移半线宽的点
    ::SPoint endRight;     // 终点右旋转90度平移半线宽的点

    // 构造函数
    SLineRectPoints() = default;

    SLineRectPoints(const ::SPoint& sl, const ::SPoint& sr,
                   const ::SPoint& el, const ::SPoint& er) :
        startLeft(sl), startRight(sr), endLeft(el), endRight(er) {}

    // 转换为向量（保持向后兼容）
    std::vector<::SPoint> toVector() const {
        return {startLeft, startRight, endLeft, endRight};
    }

    // 获取起点两个点
    std::pair<::SPoint, ::SPoint> getStartPoints() const {
        return {startLeft, startRight};
    }

    // 获取终点两个点
    std::pair<::SPoint, ::SPoint> getEndPoints() const {
        return {endLeft, endRight};
    }

    // 检查是否有效（所有点都有效）
    bool isValid() const {
        return !(startLeft.x == 0 && startLeft.y == 0 &&
                startRight.x == 0 && startRight.y == 0 &&
                endLeft.x == 0 && endLeft.y == 0 &&
                endRight.x == 0 && endRight.y == 0);
    }
};

// ==================== 文本对齐枚举 ====================
enum class TextAlignment {
    Left,
    Center,
    Right
};

// ==================== 拐角样式枚举 ====================
enum class CornerStyle {
    Hard,    // 硬角（直接连接）
    Round    // 圆弧角（使用圆弧连接）
};

// ==================== 实用工具函数 ====================
namespace Utils {
    // 颜色工具：对两个颜色进行线性插值
    inline SColor interpolateColor(const SColor& c1, const SColor& c2, float t) {
        return c1.blend(c2, t);
    }

    inline SColor multiplyColor(const SColor& color, float factor) {
        return SColor(
            color.red() * factor,
            color.green() * factor,
            color.blue() * factor,
            color.alpha()
        );
    }

    // 几何工具
    inline float distance(const ::SPoint& p1, const ::SPoint& p2) {
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    inline ::SPoint interpolate(const ::SPoint& p1, const ::SPoint& p2, float t) {
        return ::SPoint(
            p1.x + (p2.x - p1.x) * t,
            p1.y + (p2.y - p1.y) * t
        );
    }

    inline bool pointInPolygon(const ::SPoint& point, const std::vector<::SPoint>& polygon) {
        if (polygon.size() < 3) return false;

        bool inside = false;
        for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
            if (((polygon[i].y > point.y) != (polygon[j].y > point.y)) &&
                (point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) /
                          (polygon[j].y - polygon[i].y) + polygon[i].x)) {
                inside = !inside;
            }
        }
        return inside;
    }

    inline ::SRect boundingBox(const std::vector<::SPoint>& points) {
        if (points.empty()) return ::SRect();

        float minX = points[0].x, maxX = points[0].x;
        float minY = points[0].y, maxY = points[0].y;

        for (const auto& p : points) {
            if (p.x < minX) minX = p.x;
            if (p.x > maxX) maxX = p.x;
            if (p.y < minY) minY = p.y;
            if (p.y > maxY) maxY = p.y;
        }

        return ::SRect(minX, minY, maxX - minX, maxY - minY);
    }

    // 图形生成
    inline std::vector<::SPoint> generateCirclePoints(const ::SPoint& center, float radius, int segments = 36) {
        std::vector<::SPoint> points;
        if (radius <= 0 || segments < 3) return points;

        points.reserve(segments);
        float angleStep = 2.0f * M_PI / segments;

        for (int i = 0; i < segments; ++i) {
            float angle = i * angleStep;
            points.push_back(::SPoint(
                center.x + radius * std::cos(angle),
                center.y + radius * std::sin(angle)
            ));
        }

        return points;
    }

    // 根据线段起点、终点和线宽生成矩形的四个点
    // 返回SLineRectPoints结构体，包含起点左右旋转点和终点左右旋转点
    inline SLineRectPoints generateLineRectPoints(const ::SPoint& start, const ::SPoint& end, float lineWidth) {
        if (lineWidth <= 0.0f) return SLineRectPoints();

        // 计算线段方向向量
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        float length = std::sqrt(dx * dx + dy * dy);

        // 如果起点和终点相同，返回空结构体
        if (length < 0.001f) return SLineRectPoints();

        // 计算单位方向向量
        float ux = dx / length;
        float uy = dy / length;

        // 计算半线宽
        float halfWidth = lineWidth / 2.0f;

        // 计算左旋转90度的向量（逆时针旋转90度）：(-uy, ux)
        float leftX = -uy;
        float leftY = ux;

        // 计算右旋转90度的向量（顺时针旋转90度）：(uy, -ux)
        float rightX = uy;
        float rightY = -ux;

        // 生成四个点并返回结构体
        return SLineRectPoints(
            // 起点左旋转90度平移半线宽的点
            ::SPoint(start.x + halfWidth * leftX, start.y + halfWidth * leftY),
            // 起点右旋转90度平移半线宽的点
            ::SPoint(start.x + halfWidth * rightX, start.y + halfWidth * rightY),
            // 终点左旋转90度平移半线宽的点
            ::SPoint(end.x + halfWidth * leftX, end.y + halfWidth * leftY),
            // 终点右旋转90度平移半线宽的点
            ::SPoint(end.x + halfWidth * rightX, end.y + halfWidth * rightY)
        );
    }

    // 向后兼容版本：返回向量
    inline std::vector<::SPoint> generateLineRectPointsVector(const ::SPoint& start, const ::SPoint& end, float lineWidth) {
        auto points = generateLineRectPoints(start, end, lineWidth);
        return points.toVector();
    }

    inline std::vector<::SPoint> generateEllipsePoints(const ::SPoint& center, float radiusX, float radiusY, int segments = 36) {
        std::vector<::SPoint> points;
        if (radiusX <= 0 || radiusY <= 0 || segments < 3) return points;

        points.reserve(segments);
        float angleStep = 2.0f * M_PI / segments;

        for (int i = 0; i < segments; ++i) {
            float angle = i * angleStep;
            points.push_back(::SPoint(
                center.x + radiusX * std::cos(angle),
                center.y + radiusY * std::sin(angle)
            ));
        }

        return points;
    }

    inline std::vector<::SPoint> generateArcPoints(const ::SPoint& center, float radius,
                                                float startAngle, float endAngle, int segments = 36) {
        std::vector<::SPoint> points;
        if (radius <= 0 || segments < 2) return points;

        // 确保角度在合理范围内
        while (endAngle < startAngle) {
            endAngle += 2.0f * M_PI;
        }

        float angleRange = endAngle - startAngle;
        int actualSegments = std::max<int>(2, static_cast<int>(segments * angleRange / (2.0f * M_PI)));
        float angleStep = angleRange / (actualSegments - 1);

        points.reserve(actualSegments);
        for (int i = 0; i < actualSegments; ++i) {
            float angle = startAngle + i * angleStep;
            points.push_back(::SPoint(
                center.x + radius * std::cos(angle),
                center.y + radius * std::sin(angle)
            ));
        }

        return points;
    }

    inline std::vector<::SPoint> generateRoundedRectPoints(const ::SRect& rect, const SRoundedCorners& corners,
                                                        int segmentsPerCorner = 8) {
        std::vector<::SPoint> points;
        if (rect.width <= 0 || rect.height <= 0 || segmentsPerCorner < 1) return points;

        // 限制圆角半径不超过矩形尺寸的一半
        float maxRadiusX = rect.width / 2.0f;
        float maxRadiusY = rect.height / 2.0f;

        float tl = std::min<float>(corners.topLeft, std::min<float>(maxRadiusX, maxRadiusY));
        float tr = std::min<float>(corners.topRight, std::min<float>(maxRadiusX, maxRadiusY));
        float br = std::min<float>(corners.bottomRight, std::min<float>(maxRadiusX, maxRadiusY));
        float bl = std::min<float>(corners.bottomLeft, std::min<float>(maxRadiusX, maxRadiusY));

        // 生成四个角的点
        // 左上角
        if (tl > 0) {
            float centerX = rect.left + tl;
            float centerY = rect.top + tl;
            for (int i = 0; i <= segmentsPerCorner; ++i) {
                float angle = M_PI + i * (M_PI / 2) / segmentsPerCorner;
                points.push_back(::SPoint(
                    centerX + tl * std::cos(angle),
                    centerY + tl * std::sin(angle)
                ));
            }
        } else {
            points.push_back(::SPoint(rect.left, rect.top));
        }

        // 右上角
        if (tr > 0) {
            float centerX = rect.left + rect.width - tr;
            float centerY = rect.top + tr;
            for (int i = 0; i <= segmentsPerCorner; ++i) {
                float angle = M_PI * 1.5f + i * (M_PI / 2) / segmentsPerCorner;
                points.push_back(::SPoint(
                    centerX + tr * std::cos(angle),
                    centerY + tr * std::sin(angle)
                ));
            }
        } else {
            points.push_back(::SPoint(rect.left + rect.width, rect.top));
        }

        // 右下角
        if (br > 0) {
            float centerX = rect.left + rect.width - br;
            float centerY = rect.top + rect.height - br;
            for (int i = 0; i <= segmentsPerCorner; ++i) {
                float angle = i * (M_PI / 2) / segmentsPerCorner;
                points.push_back(::SPoint(
                    centerX + br * std::cos(angle),
                    centerY + br * std::sin(angle)
                ));
            }
        } else {
            points.push_back(::SPoint(rect.left + rect.width, rect.top + rect.height));
        }

        // 左下角
        if (bl > 0) {
            float centerX = rect.left + bl;
            float centerY = rect.top + rect.height - bl;
            for (int i = 0; i <= segmentsPerCorner; ++i) {
                float angle = M_PI / 2 + i * (M_PI / 2) / segmentsPerCorner;
                points.push_back(::SPoint(
                    centerX + bl * std::cos(angle),
                    centerY + bl * std::sin(angle)
                ));
            }
        } else {
            points.push_back(::SPoint(rect.left, rect.top + rect.height));
        }

        return points;
    }

    inline std::vector<::SPoint> generateRoundedRectPointsSimple(const ::SRect& rect, float radius, int segmentsPerCorner = 8) {
        return generateRoundedRectPoints(rect, SRoundedCorners(radius), segmentsPerCorner);
    }

    // 检查点是否在圆角矩形内
    inline bool pointInRoundedRect(const ::SPoint& point, const ::SRect& rect, const SRoundedCorners& corners) {
        // 首先检查是否在矩形边界内
        if (point.x < rect.left || point.x > rect.left + rect.width ||
            point.y < rect.top || point.y > rect.top + rect.height) {
            return false;
        }

        // 检查四个角区域
        // 左上角
        if (point.x < rect.left + corners.topLeft && point.y < rect.top + corners.topLeft) {
            float dx = point.x - (rect.left + corners.topLeft);
            float dy = point.y - (rect.top + corners.topLeft);
            return (dx * dx + dy * dy) <= (corners.topLeft * corners.topLeft);
        }

        // 右上角
        if (point.x > rect.left + rect.width - corners.topRight && point.y < rect.top + corners.topRight) {
            float dx = point.x - (rect.left + rect.width - corners.topRight);
            float dy = point.y - (rect.top + corners.topRight);
            return (dx * dx + dy * dy) <= (corners.topRight * corners.topRight);
        }

        // 右下角
        if (point.x > rect.left + rect.width - corners.bottomRight &&
            point.y > rect.top + rect.height - corners.bottomRight) {
            float dx = point.x - (rect.left + rect.width - corners.bottomRight);
            float dy = point.y - (rect.top + rect.height - corners.bottomRight);
            return (dx * dx + dy * dy) <= (corners.bottomRight * corners.bottomRight);
        }

        // 左下角
        if (point.x < rect.left + corners.bottomLeft &&
            point.y > rect.top + rect.height - corners.bottomLeft) {
            float dx = point.x - (rect.left + corners.bottomLeft);
            float dy = point.y - (rect.top + rect.height - corners.bottomLeft);
            return (dx * dx + dy * dy) <= (corners.bottomLeft * corners.bottomLeft);
        }

        // 不在角区域，但在矩形内
        return true;
    }

    inline bool pointInRoundedRectSimple(const ::SPoint& point, const ::SRect& rect, float radius) {
        return pointInRoundedRect(point, rect, SRoundedCorners(radius));
    }

    // 计算两条线段的交点或延长线的交点
    // 输入：两条线段的起点和终点 (p1, p2) 和 (p3, p4)
    // 输出：交点坐标，如果线段平行则返回无效点 (0, 0)
    // 参数 segmentOnly: 如果为true，只计算线段实际交点；如果为false，计算延长线交点
    inline ::SPoint lineIntersection(const ::SPoint& p1, const ::SPoint& p2,
                                    const ::SPoint& p3, const ::SPoint& p4,
                                    bool segmentOnly = false) {
        // 计算两条线的方向向量
        float dx1 = p2.x - p1.x;
        float dy1 = p2.y - p1.y;
        float dx2 = p4.x - p3.x;
        float dy2 = p4.y - p3.y;

        // 计算分母（叉积）
        float denominator = dx1 * dy2 - dy1 * dx2;

        // 如果分母接近0，说明两条线平行或重合
        if (std::abs(denominator) < 0.0001f) {
            return ::SPoint(0.0f, 0.0f); // 平行线，无交点
        }

        // 计算参数t和u
        float t = ((p3.x - p1.x) * dy2 - (p3.y - p1.y) * dx2) / denominator;
        float u = ((p3.x - p1.x) * dy1 - (p3.y - p1.y) * dx1) / denominator;

        // 如果只需要线段交点，检查t和u是否在[0,1]范围内
        if (segmentOnly) {
            if (t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f) {
                return ::SPoint(0.0f, 0.0f); // 交点不在线段上
            }
        }

        // 计算交点坐标
        float intersectX = p1.x + t * dx1;
        float intersectY = p1.y + t * dy1;

        return ::SPoint(intersectX, intersectY);
    }

    // 计算两条线段的交点（仅当线段实际相交时）
    inline ::SPoint segmentIntersection(const ::SPoint& p1, const ::SPoint& p2,
                                       const ::SPoint& p3, const ::SPoint& p4) {
        return lineIntersection(p1, p2, p3, p4, true);
    }

    // 计算两条线的延长线交点（不考虑线段范围）
    inline ::SPoint lineExtensionIntersection(const ::SPoint& p1, const ::SPoint& p2,
                                             const ::SPoint& p3, const ::SPoint& p4) {
        return lineIntersection(p1, p2, p3, p4, false);
    }

    // 检查点是否在线段上（包括端点）
    inline bool pointOnSegment(const ::SPoint& point, const ::SPoint& segStart,
                              const ::SPoint& segEnd, float epsilon = 0.001f) {
        // 首先检查点是否在由线段端点形成的边界框内
        float minX = std::min<float>(segStart.x, segEnd.x) - epsilon;
        float maxX = std::max<float>(segStart.x, segEnd.x) + epsilon;
        float minY = std::min<float>(segStart.y, segEnd.y) - epsilon;
        float maxY = std::max<float>(segStart.y, segEnd.y) + epsilon;

        if (point.x < minX || point.x > maxX || point.y < minY || point.y > maxY) {
            return false;
        }

        // 检查点是否在直线上（使用叉积）
        float cross = (segEnd.x - segStart.x) * (point.y - segStart.y) -
                     (segEnd.y - segStart.y) * (point.x - segStart.x);

        return std::abs(cross) <= epsilon;
    }

    // 检查两条线段是否相交（包括端点）
    inline bool segmentsIntersect(const ::SPoint& p1, const ::SPoint& p2,
                                 const ::SPoint& p3, const ::SPoint& p4) {
        // 快速排斥实验
        if (std::max<float>(p1.x, p2.x) < std::min<float>(p3.x, p4.x) ||
            std::max<float>(p3.x, p4.x) < std::min<float>(p1.x, p2.x) ||
            std::max<float>(p1.y, p2.y) < std::min<float>(p3.y, p4.y) ||
            std::max<float>(p3.y, p4.y) < std::min<float>(p1.y, p2.y)) {
            return false;
        }

        // 跨立实验
        auto cross = [](const ::SPoint& a, const ::SPoint& b, const ::SPoint& c) {
            return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        };

        float c1 = cross(p1, p2, p3);
        float c2 = cross(p1, p2, p4);
        float c3 = cross(p3, p4, p1);
        float c4 = cross(p3, p4, p2);

        // 允许在端点上相交
        if (std::abs(c1) < 0.0001f || std::abs(c2) < 0.0001f ||
            std::abs(c3) < 0.0001f || std::abs(c4) < 0.0001f) {
            // 检查是否在端点上
            return pointOnSegment(p3, p1, p2) || pointOnSegment(p4, p1, p2) ||
                   pointOnSegment(p1, p3, p4) || pointOnSegment(p2, p3, p4);
        }

        return (c1 * c2 < 0) && (c3 * c4 < 0);
    }

    // 判断点是否位于矩形内或矩形边线上
    // 参数：
    //   point: 要检查的点
    //   rect: 矩形
    //   epsilon: 容差，用于判断点是否在边线上
    // 返回值：
    //   true: 点在矩形内或边线上
    //   false: 点在矩形外
    inline bool pointInRect(const ::SPoint& point, const ::SRect& rect, float epsilon = 0.001f) {
        // 计算矩形的边界，处理负宽度和负高度的情况
        float left = rect.width >= 0 ? rect.left : rect.left + rect.width;
        float right = rect.width >= 0 ? rect.left + rect.width : rect.left;
        float top = rect.height >= 0 ? rect.top : rect.top + rect.height;
        float bottom = rect.height >= 0 ? rect.top + rect.height : rect.top;

        // 检查点是否在矩形边界内（包括容差）
        // 点在矩形内：left <= x <= right 且 top <= y <= bottom
        // 考虑容差：left - epsilon <= x <= right + epsilon 且 top - epsilon <= y <= bottom + epsilon
        bool inside = (point.x >= left - epsilon) && (point.x <= right + epsilon) &&
                      (point.y >= top - epsilon) && (point.y <= bottom + epsilon);

        return inside;
    }

    // 判断点是否严格位于矩形内部（不在边线上）
    // 参数：
    //   point: 要检查的点
    //   rect: 矩形
    //   epsilon: 容差，用于判断点是否在边线上
    // 返回值：
    //   true: 点在矩形内部（不在边线上）
    //   false: 点在矩形外部或边线上
    inline bool pointInsideRect(const ::SPoint& point, const ::SRect& rect, float epsilon = 0.001f) {
        // 计算矩形的边界，处理负宽度和负高度的情况
        float left = rect.width >= 0 ? rect.left : rect.left + rect.width;
        float right = rect.width >= 0 ? rect.left + rect.width : rect.left;
        float top = rect.height >= 0 ? rect.top : rect.top + rect.height;
        float bottom = rect.height >= 0 ? rect.top + rect.height : rect.top;

        // 检查点是否在矩形内部（排除边线）
        // 点在矩形内部：left < x < right 且 top < y < bottom
        // 考虑容差：left + epsilon < x < right - epsilon 且 top + epsilon < y < bottom - epsilon
        bool inside = (point.x > left + epsilon) && (point.x < right - epsilon) &&
                      (point.y > top + epsilon) && (point.y < bottom - epsilon);

        return inside;
    }

    // 判断点是否在矩形边线上
    // 参数：
    //   point: 要检查的点
    //   rect: 矩形
    //   epsilon: 容差，用于判断点是否在边线上
    // 返回值：
    //   true: 点在矩形边线上
    //   false: 点不在矩形边线上（可能在内部或外部）
    inline bool pointOnRectBorder(const ::SPoint& point, const ::SRect& rect, float epsilon = 0.001f) {
        // 计算矩形的边界，处理负宽度和负高度的情况
        float left = rect.width >= 0 ? rect.left : rect.left + rect.width;
        float right = rect.width >= 0 ? rect.left + rect.width : rect.left;
        float top = rect.height >= 0 ? rect.top : rect.top + rect.height;
        float bottom = rect.height >= 0 ? rect.top + rect.height : rect.top;

        // 检查点是否在矩形边线上
        // 点在边线上需要满足以下条件之一：
        // 1. 在左边界或右边界上，且y在top和bottom之间
        // 2. 在上边界或下边界上，且x在left和right之间

        // 检查是否在左边界或右边界上
        bool onVerticalEdge = ((std::abs(point.x - left) <= epsilon) || (std::abs(point.x - right) <= epsilon)) &&
                              (point.y >= top - epsilon) && (point.y <= bottom + epsilon);

        // 检查是否在上边界或下边界上
        bool onHorizontalEdge = ((std::abs(point.y - top) <= epsilon) || (std::abs(point.y - bottom) <= epsilon)) &&
                                (point.x >= left - epsilon) && (point.x <= right + epsilon);

        return onVerticalEdge || onHorizontalEdge;
    }

    // 判断点是否在矩形角上
    // 参数：
    //   point: 要检查的点
    //   rect: 矩形
    //   epsilon: 容差，用于判断点是否在角上
    // 返回值：
    //   true: 点在矩形角上
    //   false: 点不在矩形角上
    inline bool pointOnRectCorner(const ::SPoint& point, const ::SRect& rect, float epsilon = 0.001f) {
        // 计算矩形的边界，处理负宽度和负高度的情况
        float left = rect.width >= 0 ? rect.left : rect.left + rect.width;
        float right = rect.width >= 0 ? rect.left + rect.width : rect.left;
        float top = rect.height >= 0 ? rect.top : rect.top + rect.height;
        float bottom = rect.height >= 0 ? rect.top + rect.height : rect.top;

        // 计算矩形的四个角
        ::SPoint topLeft(left, top);
        ::SPoint topRight(right, top);
        ::SPoint bottomRight(right, bottom);
        ::SPoint bottomLeft(left, bottom);

        // 检查点是否接近任何一个角
        return (distance(point, topLeft) <= epsilon) ||
               (distance(point, topRight) <= epsilon) ||
               (distance(point, bottomRight) <= epsilon) ||
               (distance(point, bottomLeft) <= epsilon);
    }

    // 判断点是否在矩形边线上（包括角）
    // 这是pointOnRectBorder的别名，提供更直观的命名
    inline bool pointOnRectEdge(const ::SPoint& point, const ::SRect& rect, float epsilon = 0.001f) {
        return pointOnRectBorder(point, rect, epsilon);
    }
}

// ==================== 绘图上下文 ====================
class DrawingContext {
public:
    DrawingContext(SDL_Renderer* renderer);

    // ==================== 画笔设置 ====================
    void setPen(const SPen& pen) { m_pen = pen; }
    const SPen& getPen() const { return m_pen; }
    SPen& getPen() { return m_pen; }

    void setBrush(const SBrush& brush) { m_brush = brush; }
    const SBrush& getBrush() const { return m_brush; }
    SBrush& getBrush() { return m_brush; }

    // 向后兼容
    void setPenColor(const SColor& color) { m_pen.setColor(color); }
    SColor getPenColor() const { return m_pen.color(); }
    void setPenWidth(float width) { m_pen.setWidth(width); }
    float getPenWidth() const { return m_pen.width(); }
    void setFillColor(const SColor& color) { m_brush.setColor(color); }
    SColor getFillColor() const { return m_brush.color(); }

    void setPenStyle(PenStyle style) { m_pen.setStyle(style); }
    PenStyle getPenStyle() const { return m_pen.style(); }
    void setLineCap(LineCap cap) { m_pen.setCap(cap); }
    LineCap getLineCap() const { return m_pen.cap(); }
    void setLineJoin(LineJoin join) { m_pen.setJoin(join); }
    LineJoin getLineJoin() const { return m_pen.join(); }
    void setDashPattern(const std::vector<float>& pattern) { m_pen.setDashPattern(pattern); }
    const std::vector<float>& getDashPattern() const { return m_pen.dashPattern(); }
    void setBrushStyle(BrushStyle style) { m_brush.setStyle(style); }
    BrushStyle getBrushStyle() const { return m_brush.style(); }
    void setFont(const std::string& fontName, float size);

    // ==================== 基本图形绘制 ====================
    void drawPoint(const ::SPoint& point) { drawPoint(point.x, point.y); }
    void drawPoint(float x, float y);
    void drawPoint(float x, float y, const SColor& color);

    void drawLine(const ::SPoint& start, const ::SPoint& end) { drawLine(start.x, start.y, end.x, end.y); }
    void drawLine(float x1, float y1, float x2, float y2);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawLine(const SLineRectPoints& rectPoints);

    void drawDashedLine(float x1, float y1, float x2, float y2);
    void drawDashedLine(const ::SPoint& start, const ::SPoint& end) { drawDashedLine(start.x, start.y, end.x, end.y); }

    void drawRect(const ::SRect& rect, bool filled = false);
    void drawRoundedRect(const ::SRect& rect, float radius, bool filled = false);
    void drawRoundedRect(const ::SRect& rect, const SRoundedCorners& corners, bool filled = false);
    void drawRoundedRectWithBorder(const ::SRect& rect, float radius, float borderWidth, const SColor& borderColor, const SColor& fillColor);
    void drawRoundedRectWithBorder(const ::SRect& rect, const SRoundedCorners& corners, float borderWidth, const SColor& borderColor, const SColor& fillColor);

    static inline float coverageD2(float D2, float R) {
        float inner = R - 0.5f;
        float outer = R + 0.5f;
        float inner2 = inner * inner;
        float outer2 = outer * outer;
        if (D2 <= inner2) return 1.0f;
        if (D2 >= outer2) return 0.0f;
        return (outer2 - D2) / (outer2 - inner2);
    }

    void drawFilledCircleWithStroke(int cx, int cy, float R, float thickness, const SColor& fillColor, const SColor& strokeColor);
    void drawFilledCircleWithStroke(int cx, int cy, float R);
    void drawFilledCircleAA(float cx, float cy, float radius, const SColor& color);
    void drawCircleWithThickness(float cx, float cy, float radius, float thickness, const SColor& color);
    void drawCircle(const ::SPoint& center, float radius, bool filled = false);
    void drawCircle(const ::SPoint& center, float radius, const SColor& fillColor, const SColor& strokeColor, float strokeWidth);

    void drawEllipse(const ::SPoint& center, float radiusX, float radiusY, bool filled = false);
    void drawArc(const ::SPoint& center, float radius, float startAngle, float endAngle, bool filled = false);
    void drawPolygon(const std::vector<::SPoint>& points, bool filled = false, bool debugCorner = false, const SColor& debugColor = SColor::Green());
    void drawPolyline(const std::vector<::SPoint>& points, bool debugCorner = false, const SColor& debugColor = SColor::Green());

    void drawText(const ::SPoint& position, const std::string& text);
    void drawText(const ::SRect& bounds, const std::string& text, TextAlignment alignment = TextAlignment::Left);

    void drawImage(const ::SPoint& position, SDL_Texture* texture);
    void drawImage(const ::SRect& destRect, SDL_Texture* texture);
    void drawImage(const ::SRect& destRect, SDL_Texture* texture, const ::SRect& srcRect);

    void pushClipRect(const ::SRect& rect);
    void popClipRect();

    void pushTransform();
    void popTransform();

    void setCornerStyle(CornerStyle style) { m_cornerStyle = style; }
    CornerStyle getCornerStyle() const { return m_cornerStyle; }
    SDL_Renderer* getRenderer() const { return m_renderer; }

    void scale(float sx, float sy);

private:
    SDL_Renderer* m_renderer;
    SPen m_pen;
    SBrush m_brush;
    std::string m_fontName;
    float m_fontSize;
    CornerStyle m_cornerStyle;
    std::vector<::SRect> m_clipStack;
    std::vector<SDL_Renderer*> m_transformStack;
};

// ==================== 高级绘图功能 ====================
class AdvancedDrawing {
public:
    static void drawGradientRectWithBrush(DrawingContext& ctx, const ::SRect& rect,
                                          const SLinearGradient& gradient, bool stroke = false);
    static void drawGradientRect(DrawingContext& ctx, const ::SRect& rect,
                                const SColor& startColor, const SColor& endColor,
                                bool horizontal = true);
    static void drawRadialGradientRect(DrawingContext& ctx, const ::SRect& rect,
                                       const SRadialGradient& gradient);
    static void drawShadow(DrawingContext& ctx, const ::SRect& rect,
                          float shadowSize, const SColor& shadowColor,
                          float blurRadius = 5.0f);
    static void drawGlow(DrawingContext& ctx, std::function<void()> drawFunc,
                        float glowRadius, const SColor& glowColor,
                        int layers = 5);
    static void drawStyledRect(DrawingContext& ctx, const ::SRect& rect,
                               const SPen& pen, const SBrush& brush);
    static void drawStyledRoundedRect(DrawingContext& ctx, const ::SRect& rect,
                                      float radius, const SPen& pen, const SBrush& brush);
    static void drawStyledCircle(DrawingContext& ctx, const ::SPoint& center, float radius,
                                 const SPen& pen, const SBrush& brush);
    static void drawStyledEllipse(DrawingContext& ctx, const ::SPoint& center,
                                  float radiusX, float radiusY,
                                  const SPen& pen, const SBrush& brush);
};

}
#endif // GRAPHTOOL_H
