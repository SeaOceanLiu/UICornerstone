// ============================================================================
// Shape.cpp -- 形状控件实现（design/Shape_Design.md）
// 绘制 = 参数组装调用 RenderDevice 一期图元（§4.4 基类默认实现）；
// 每个参数 setter 统一触发 rebuildGeometry()（决策 3.7 缓存链路——本控件几何
// 由图元按参数即时生成，rebuildGeometry 仅作统一重算入口与扩展锚点）。
// ============================================================================
#include "Shape.h"
#include "PropertyNames.h"

#include <algorithm>

// "rect"/"filled-rect"/... → ShapeType（外部链接：CABI/JSON 共用，声明见 Shape.h）
bool shapeTypeFromString(const std::string& s, ShapeType& out) {
    if (s == PropertyNames::kShapeRect)        { out = ShapeType::Rect;        return true; }
    if (s == PropertyNames::kShapeFilledRect)  { out = ShapeType::FilledRect;  return true; }
    if (s == PropertyNames::kShapeRoundRect)   { out = ShapeType::RoundRect;   return true; }
    if (s == PropertyNames::kShapeCircle)      { out = ShapeType::Circle;      return true; }
    if (s == PropertyNames::kShapeEllipse)     { out = ShapeType::Ellipse;     return true; }
    if (s == PropertyNames::kShapePolyline)    { out = ShapeType::Polyline;    return true; }
    if (s == PropertyNames::kShapePolygon)     { out = ShapeType::Polygon;     return true; }
    return false;
}

namespace {

const char* shapeTypeToString(ShapeType t) {
    switch (t) {
    case ShapeType::Rect:        return PropertyNames::kShapeRect;
    case ShapeType::FilledRect:  return PropertyNames::kShapeFilledRect;
    case ShapeType::RoundRect:   return PropertyNames::kShapeRoundRect;
    case ShapeType::Circle:      return PropertyNames::kShapeCircle;
    case ShapeType::Ellipse:     return PropertyNames::kShapeEllipse;
    case ShapeType::Polyline:    return PropertyNames::kShapePolyline;
    case ShapeType::Polygon:     return PropertyNames::kShapePolygon;
    }
    return PropertyNames::kShapeRect;
}

} // namespace

Shape::Shape(Control* parent, const SRect& rect, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
{
    m_ctlType = ControlType::Shape;
    m_rect = rect;
    m_baseRect = rect;
    setTransparent(true);   // 缺省无底色（纯图元）；setBackgroundStateColor 显式设色后自动取消透明
}

void Shape::setBackgroundStateColor(StateColor stateColor) {
    ControlImpl::setBackgroundStateColor(stateColor);
    setTransparent(false);  // 显式设背景色即绘制底色
}

// ── 参数设置（全部触发重算，§4.8） ──

void Shape::setShape(ShapeType type) {
    if (m_shape == type) return;
    m_shape = type;
    rebuildGeometry();
}
void Shape::setFillColor(SColor color) {
    m_fillColor = color;
    rebuildGeometry();
}
void Shape::setStrokeColor(SColor color) {
    m_strokeColor = color;
    rebuildGeometry();
}
void Shape::setLineWidth(float width) {
    width = std::max(0.f, width);
    if (m_lineWidth == width) return;
    m_lineWidth = width;
    rebuildGeometry();
}
void Shape::setRadius(float radius) {
    radius = std::max(0.f, radius);
    if (m_radius == radius) return;
    m_radius = radius;
    rebuildGeometry();
}
void Shape::setRingWidth(float width) {
    width = std::max(0.f, width);                       // 负值钳制 0
    if (m_ringWidth == width) return;
    m_ringWidth = width;   // ≥内切半径时由 drawEllipse 钳制为实心
    rebuildGeometry();
}
void Shape::setPoints(const std::vector<SPointF>& pts) {
    m_points = pts;
    m_baseRect = m_rect;                                // 以当前 rect 为缩放基准（决策 3.3）
    m_hasBaseRect = true;
    rebuildGeometry();
}

std::vector<Shape::SPointF> Shape::getPoints() const {
    if (!m_hasBaseRect || m_baseRect.width <= 0.f || m_baseRect.height <= 0.f)
        return m_points;
    const float sx = m_rect.width / m_baseRect.width;
    const float sy = m_rect.height / m_baseRect.height;
    std::vector<SPointF> out;
    out.reserve(m_points.size());
    for (const auto& p : m_points)
        out.push_back({p.x * sx, p.y * sy});
    return out;
}

SPoint Shape::mapToDrawPoint(float lx, float ly) const {
    auto* self = const_cast<Shape*>(this);          // getDrawRect/getScaleXX 为非 const 接口
    const SRect dr = self->getDrawRect();           // 缩放后绘制区（scale>1 时绘制坐标随之放大）
    return {dr.left + lx * self->getScaleXX(), dr.top + ly * self->getScaleYY()};
}
SPoint Shape::getDrawPoint(int index) const {
    auto scaled = getPoints();
    if (index < 0 || index >= static_cast<int>(scaled.size())) return {m_rect.left, m_rect.top};
    return mapToDrawPoint(scaled[index].x, scaled[index].y);
}

void Shape::rebuildGeometry() {
    // 几何由 RenderDevice 图元在 draw 期按成员即时生成（顶点数恒定、零分配热点路径）；
    // 此入口为缓存链路统一锚点：未来引入预烘焙顶点缓存时在此重建（设计 §4.8）。
}

// ── 多图元（组合图形） ──

int Shape::addPrimitive(ShapeType type, const SRect& localRect) {
    ShapePrimitive pr;
    pr.type = type;
    pr.rect = localRect;
    m_primitives.push_back(std::move(pr));
    rebuildGeometry();
    return static_cast<int>(m_primitives.size()) - 1;
}
void Shape::clearPrimitives() {
    if (m_primitives.empty()) return;
    m_primitives.clear();
    rebuildGeometry();
}
void Shape::setPrimitiveFill(int idx, SColor c) {
    if (idx < 0 || idx >= static_cast<int>(m_primitives.size())) return;
    m_primitives[idx].fill = c;
    rebuildGeometry();
}
void Shape::setPrimitiveStroke(int idx, SColor c) {
    if (idx < 0 || idx >= static_cast<int>(m_primitives.size())) return;
    m_primitives[idx].stroke = c;
    rebuildGeometry();
}
void Shape::setPrimitiveLineWidth(int idx, float w) {
    if (idx < 0 || idx >= static_cast<int>(m_primitives.size())) return;
    m_primitives[idx].lineWidth = std::max(0.f, w);
    rebuildGeometry();
}
void Shape::setPrimitiveRadius(int idx, float r) {
    if (idx < 0 || idx >= static_cast<int>(m_primitives.size())) return;
    m_primitives[idx].radius = std::max(0.f, r);
    rebuildGeometry();
}
void Shape::setPrimitiveRingWidth(int idx, float w) {
    if (idx < 0 || idx >= static_cast<int>(m_primitives.size())) return;
    m_primitives[idx].ringWidth = std::max(0.f, w);
    rebuildGeometry();
}
void Shape::setPrimitivePoints(int idx, const std::vector<SPointF>& pts) {
    if (idx < 0 || idx >= static_cast<int>(m_primitives.size())) return;
    m_primitives[idx].points = pts;
    rebuildGeometry();
}

// ── 绘制 ──

void Shape::drawPrimitiveAt(RenderDevice* dev, const ShapePrimitive& pr) {
    const SRect dr = getDrawRect();
    const float sx = getScaleXX(), sy = getScaleYY();
    const float ox = dr.left, oy = dr.top;
    const SRect r(ox + pr.rect.left * sx, oy + pr.rect.top * sy,
                  pr.rect.width * sx, pr.rect.height * sy);
    switch (pr.type) {
    case ShapeType::Rect:
        if (pr.fill.alpha() > 0) { dev->setDrawColor(pr.fill); dev->fillRect(r); }
        if (pr.stroke.alpha() > 0 && pr.lineWidth > 0.f) {
            dev->drawLine(r.left, r.top, r.left + r.width - 1, r.top, pr.lineWidth, pr.stroke);
            dev->drawLine(r.left, r.top + r.height - 1, r.left + r.width - 1, r.top + r.height - 1, pr.lineWidth, pr.stroke);
            dev->drawLine(r.left, r.top, r.left, r.top + r.height - 1, pr.lineWidth, pr.stroke);
            dev->drawLine(r.left + r.width - 1, r.top, r.left + r.width - 1, r.top + r.height - 1, pr.lineWidth, pr.stroke);
        }
        break;
    case ShapeType::FilledRect:
        if (pr.fill.alpha() > 0) { dev->setDrawColor(pr.fill); dev->fillRect(r); }
        break;
    case ShapeType::RoundRect:
        dev->drawRoundRect(r, pr.radius, pr.fill, pr.stroke, pr.lineWidth);
        break;
    case ShapeType::Circle:
    case ShapeType::Ellipse: {
        const float rx = r.width * 0.5f, ry = r.height * 0.5f;
        dev->drawEllipse(r.left + rx, r.top + ry, rx, ry,
                         pr.fill, pr.stroke, pr.lineWidth, pr.ringWidth);
        break;
    }
    case ShapeType::Polyline: {
        if (pr.points.size() < 2) break;
        std::vector<SPoint> g;
        g.reserve(pr.points.size());
        for (const auto& p : pr.points) g.push_back({ox + p.x * sx, oy + p.y * sy});
        dev->drawPolyline(g.data(), static_cast<int>(g.size()), pr.stroke, pr.lineWidth);
        break;
    }
    case ShapeType::Polygon: {
        if (pr.points.size() < 3) break;
        std::vector<SPoint> g;
        g.reserve(pr.points.size());
        for (const auto& p : pr.points) g.push_back({ox + p.x * sx, oy + p.y * sy});
        dev->drawPolygon(g.data(), static_cast<int>(g.size()),
                         pr.fill, pr.stroke, pr.lineWidth);
        break;
    }
    }
}

void Shape::draw(void) {
    if (!m_visible) return;   // 可见性守卫（beforeDraw 仅辅助，不能依赖）
    ControlImpl::beforeDraw();   // 背景 + box-shadow：在裁剪之外（CR20261112001）
    RenderDevice* dev = getRenderDevice();
    if (!dev) return;

    const SRect r = getDrawRect();   // 缩放后绘制区（xScale/yScale 生效）
    // CR20261117001：仅裁剪图元绘制；背景/阴影/边框/子控件不受影响。
    const bool doClip = m_clipShape && r.width > 0.0f && r.height > 0.0f;
    if (doClip) dev->pushClipRect(r);

    if (!m_primitives.empty()) {
        for (const auto& pr : m_primitives) drawPrimitiveAt(dev, pr);
    } else {
        switch (m_shape) {
        case ShapeType::Rect:
            if (m_fillColor.alpha() > 0) { dev->setDrawColor(m_fillColor); dev->fillRect(r); }
            // 四边描边（lineWidth<=1 走像素对齐 1px；>1 走宽线边带）
            if (m_strokeColor.alpha() > 0 && m_lineWidth > 0.f) {
                dev->drawLine(r.left, r.top, r.left + r.width - 1, r.top, m_lineWidth, m_strokeColor);
                dev->drawLine(r.left, r.top + r.height - 1, r.left + r.width - 1, r.top + r.height - 1, m_lineWidth, m_strokeColor);
                dev->drawLine(r.left, r.top, r.left, r.top + r.height - 1, m_lineWidth, m_strokeColor);
                dev->drawLine(r.left + r.width - 1, r.top, r.left + r.width - 1, r.top + r.height - 1, m_lineWidth, m_strokeColor);
            }
            break;
        case ShapeType::FilledRect:
            if (m_fillColor.alpha() > 0) { dev->setDrawColor(m_fillColor); dev->fillRect(r); }
            break;
        case ShapeType::RoundRect:
            dev->drawRoundRect(r, m_radius, m_fillColor, m_strokeColor, m_lineWidth);
            break;
        case ShapeType::Circle:
        case ShapeType::Ellipse: {
            const float rx = r.width * 0.5f, ry = r.height * 0.5f;
            dev->drawEllipse(r.left + rx, r.top + ry, rx, ry,
                             m_fillColor, m_strokeColor, m_lineWidth, m_ringWidth);
            break;
        }
        case ShapeType::Polyline: {
            auto local = getPoints();
            if (local.size() < 2) break;
            std::vector<SPoint> gpts;                          // 本地 → 全局（决策 3.3 映射）
            gpts.reserve(local.size());
            for (const auto& p : local) gpts.push_back(mapToDrawPoint(p.x, p.y));
            dev->drawPolyline(gpts.data(), static_cast<int>(gpts.size()), m_strokeColor, m_lineWidth);
            break;
        }
        case ShapeType::Polygon: {
            auto local = getPoints();
            if (local.size() < 3) break;
            std::vector<SPoint> gpts;
            gpts.reserve(local.size());
            for (const auto& p : local) gpts.push_back(mapToDrawPoint(p.x, p.y));
            dev->drawPolygon(gpts.data(), static_cast<int>(gpts.size()),
                             m_fillColor, m_strokeColor, m_lineWidth);
            break;
        }
        }
    }

    if (doClip) dev->popClipRect();
    ControlImpl::draw();         // 子控件（不受 Shape 图元裁剪影响）
    ControlImpl::afterDraw();    // 边框 / 焦点环（不裁剪）
}

// ── 属性系统 override ──

int Shape::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kClipShape) == 0) { m_clipShape = (value != 0); return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}
int Shape::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kClipShape) == 0) { out = m_clipShape ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}

int Shape::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kShape) == 0 && value) {
        ShapeType t;
        if (!shapeTypeFromString(value, t)) return 0;   // 未知形状枚举拒绝（负例）
        setShape(t);
        return 1;
    }
    return ControlImpl::setEnumProperty(prop, value);
}
int Shape::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kLineWidthProp) == 0) { setLineWidth(value); return 1; }
    if (strcmp(prop, PropertyNames::kRadius) == 0)       { setRadius(value); return 1; }
    if (strcmp(prop, PropertyNames::kRingWidth) == 0)    { setRingWidth(value); return 1; }
    return ControlImpl::setFloatProperty(prop, value);
}
int Shape::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, PropertyNames::kFill) == 0)   { setFillColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kStroke) == 0) { setStrokeColor(color); return 1; }
    return ControlImpl::setColorProperty(prop, color);
}
int Shape::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kShape) == 0) { out = shapeTypeToString(m_shape); return 1; }
    return ControlImpl::getEnumProperty(prop, out);
}
int Shape::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kLineWidthProp) == 0) { out = m_lineWidth; return 1; }
    if (strcmp(prop, PropertyNames::kRadius) == 0)       { out = m_radius; return 1; }
    if (strcmp(prop, PropertyNames::kRingWidth) == 0)    { out = m_ringWidth; return 1; }
    return ControlImpl::getFloatProperty(prop, out);
}
int Shape::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kFill) == 0)   { out = m_fillColor; return 1; }
    if (strcmp(prop, PropertyNames::kStroke) == 0) { out = m_strokeColor; return 1; }
    return ControlImpl::getColorProperty(prop, out);
}

// ── ShapeBuilder（声明式构建，LabelBuilder 同款惯例） ──

ShapeBuilder::ShapeBuilder(Control* parent, SRect rect, float xScale, float yScale)
    : m_shape(nullptr)
{
    m_shape = std::make_shared<Shape>(parent, rect, xScale, yScale);
}
ShapeBuilder& ShapeBuilder::setShape(ShapeType type)            { m_shape->setShape(type); return *this; }
ShapeBuilder& ShapeBuilder::setFillColor(SColor color)          { m_shape->setFillColor(color); return *this; }
ShapeBuilder& ShapeBuilder::setStrokeColor(SColor color)        { m_shape->setStrokeColor(color); return *this; }
ShapeBuilder& ShapeBuilder::setLineWidth(float width)           { m_shape->setLineWidth(width); return *this; }
ShapeBuilder& ShapeBuilder::setRadius(float radius)             { m_shape->setRadius(radius); return *this; }
ShapeBuilder& ShapeBuilder::setRingWidth(float width)           { m_shape->setRingWidth(width); return *this; }
ShapeBuilder& ShapeBuilder::setPoints(const std::vector<Shape::SPointF>& pts) { m_shape->setPoints(pts); return *this; }
ShapeBuilder& ShapeBuilder::setBackgroundStateColor(StateColor sc) { m_shape->setBackgroundStateColor(sc); return *this; }
ShapeBuilder& ShapeBuilder::addPrimitive(ShapeType type, const SRect& localRect) {
    m_shape->addPrimitive(type, localRect); return *this;
}
ShapeBuilder& ShapeBuilder::setPrimitiveFill(int idx, SColor c)       { m_shape->setPrimitiveFill(idx, c); return *this; }
ShapeBuilder& ShapeBuilder::setPrimitiveStroke(int idx, SColor c)     { m_shape->setPrimitiveStroke(idx, c); return *this; }
ShapeBuilder& ShapeBuilder::setPrimitiveLineWidth(int idx, float w)   { m_shape->setPrimitiveLineWidth(idx, w); return *this; }
ShapeBuilder& ShapeBuilder::setPrimitiveRadius(int idx, float r)      { m_shape->setPrimitiveRadius(idx, r); return *this; }
ShapeBuilder& ShapeBuilder::setPrimitiveRingWidth(int idx, float w)   { m_shape->setPrimitiveRingWidth(idx, w); return *this; }
ShapeBuilder& ShapeBuilder::setPrimitivePoints(int idx, const std::vector<Shape::SPointF>& pts) { m_shape->setPrimitivePoints(idx, pts); return *this; }
std::shared_ptr<Shape> ShapeBuilder::build(void) {
    m_shape->create();
    return m_shape;
}
