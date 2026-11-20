// ============================================================================
// Shape.h -- 形状控件（纯绘制：矩形/圆角/圆/椭圆/折线/多边形，空心与环带）
// 设计：design/Shape_Design.md（决策 3.1-3.13）
// 定位：最简绘制控件——无子控件/无布局引擎/无事件；每个参数的修改均触发
//       rebuildGeometry() 几何重算（§4.8 缓存链路）。
// ============================================================================
#pragma once

#include "ControlBase.h"

#include <vector>

// 形状类型（JSON/CABI 枚举值见 PropertyNames::kShape*）
enum class ShapeType {
    Rect,        // 描边矩形（stroke/lineWidth）
    FilledRect,  // 填充矩形（fill）
    RoundRect,   // 圆角矩形（fill/stroke/lineWidth/radius）
    Circle,      // 圆（内切整个 rect，各向异性拉伸；ringWidth>0 画环带）
    Ellipse,     // 椭圆（同 circle 语义）
    Polyline,    // 折线（开放路径，仅描边）
    Polygon      // 多边形（闭合：凸顶点扇/凹耳切 + 描边）
};

// 多图元（组合图形）：单个 Shape 控件绘制多个图元拼合特定图形。
// 坐标为控件本地 px（同 points 语义，决策 3.3），不参与 baseRect 等比缩放。
struct ShapePrimitive {
    ShapeType type = ShapeType::Rect;
    SRect rect;                               // 本地坐标
    SColor fill{0, 0, 0, 0};                  // 透明 = 空心
    SColor stroke{0, 0, 0, 255};
    float lineWidth = 1.0f;
    float radius = 0.0f;
    float ringWidth = 0.0f;
    std::vector<SPoint> points;               // 本地坐标（polyline/polygon 用）
};

class Shape : public ControlImpl {
public:
    using SPointF = SPoint; // 点 = 本地像素 float 坐标（决策 3.3）

    Shape(Control* parent, const SRect& rect, float xScale = 1.0f, float yScale = 1.0f);

    // ── 参数设置（全部触发 rebuildGeometry）──
    void setShape(ShapeType type);
    void setFillColor(SColor color);          // 透明 = 空心
    void setStrokeColor(SColor color);
    void setLineWidth(float width);           // px，0 = 无描边
    void setRadius(float radius);             // round-rect 生效
    void setRingWidth(float width);           // circle/ellipse 生效；0 = 实心；≥内切半径钳制实心
    // points 以当前 rect 为基准写入（决策 3.3）；resize 时按基准等比缩放
    void setPoints(const std::vector<SPointF>& pts);

    // ── 多图元（组合图形）──
    // 非空时替代单图元 m_shape 渲染；返回图元索引
    int  addPrimitive(ShapeType type, const SRect& localRect);
    void clearPrimitives();
    int  getPrimitiveCount() const { return static_cast<int>(m_primitives.size()); }
    void setPrimitiveFill(int idx, SColor c);
    void setPrimitiveStroke(int idx, SColor c);
    void setPrimitiveLineWidth(int idx, float w);
    void setPrimitiveRadius(int idx, float r);
    void setPrimitiveRingWidth(int idx, float w);
    void setPrimitivePoints(int idx, const std::vector<SPointF>& pts);

    // ── 查询 ──
    ShapeType getShape() const { return m_shape; }
    SColor getFillColor() const { return m_fillColor; }
    SColor getStrokeColor() const { return m_strokeColor; }
    float getLineWidth() const { return m_lineWidth; }
    float getRadius() const { return m_radius; }
    float getRingWidth() const { return m_ringWidth; }
    // 当前缩放后的本地点集（resize 后为等比缩放结果）
    std::vector<SPointF> getPoints() const;
    // 本地 → 全局绘制坐标（主映射 API，决策 3.3）
    SPoint mapToDrawPoint(float lx, float ly) const;
    // 第 index 个顶点的全局坐标 = mapToDrawPoint(points[index])
    SPoint getDrawPoint(int index) const;

    // ── 引擎接口 ──
    void draw(void) override;

    // ── 属性系统 override（CABI 分发；points 走专用 CABI 不走字符串属性） ──
    int setEnumProperty(const char* prop, const char* value) override;   // "shape"
    int setFloatProperty(const char* prop, float value) override;        // "line-width"/"radius"/"ring-width"
    int setColorProperty(const char* prop, SColor color) override;       // "fill"/"stroke"
    int getEnumProperty(const char* prop, const char*& out) override;
    int getFloatProperty(const char* prop, float& out) override;
    int getColorProperty(const char* prop, SColor& out) override;
    int setBoolProperty(const char* prop, int value) override;           // CR20261117001："clip"
    int getBoolProperty(const char* prop, int& out) override;

    // 背景色：复用 ControlBase 状态色体系；显式设置即取消透明（否则 Shape 缺省无底色）
    void setBackgroundStateColor(StateColor stateColor) override;

protected:
    void rebuildGeometry();                                   // 参数变更统一入口（§4.8）
    void drawPrimitiveAt(RenderDevice* dev, const ShapePrimitive& pr);  // 单图元绘制（本地→全局偏移）

private:
    friend class ShapeBuilder;

    ShapeType m_shape = ShapeType::Rect;
    SColor m_fillColor{0, 0, 0, 0};            // 缺省透明 = 空心
    SColor m_strokeColor{0, 0, 0, 255};        // 缺省黑
    float m_lineWidth = 1.0f;
    float m_radius = 0.0f;
    float m_ringWidth = 0.0f;
    std::vector<SPointF> m_points;              // 基准本地坐标（setPoints 写入时的值）
    SRect m_baseRect;                           // setPoints 时的 rect（缩放基准）
    bool m_hasBaseRect = false;
    std::vector<ShapePrimitive> m_primitives;   // 多图元（非空时替代单图元渲染）
    bool m_clipShape = true;                    // CR20261117001：图元绘制裁剪到 getDrawRect()（默认开）
};

// "rect"/"filled-rect"/... → ShapeType（CABI/JSON 共用；PropertyNames::kShape* 值域）
bool shapeTypeFromString(const std::string& s, ShapeType& out);

// ── 声明式 Builder（LabelBuilder 同款惯例）──
class ShapeBuilder {
private:
    std::shared_ptr<Shape> m_shape;
public:
    ShapeBuilder(Control* parent, SRect rect, float xScale = 1.0f, float yScale = 1.0f);
    ShapeBuilder& setShape(ShapeType type);
    ShapeBuilder& setFillColor(SColor color);
    ShapeBuilder& setStrokeColor(SColor color);
    ShapeBuilder& setLineWidth(float width);
    ShapeBuilder& setRadius(float radius);
    ShapeBuilder& setRingWidth(float width);
    ShapeBuilder& setPoints(const std::vector<Shape::SPointF>& pts);
    ShapeBuilder& setBackgroundStateColor(StateColor stateColor);
    // 多图元（组合图形）
    ShapeBuilder& addPrimitive(ShapeType type, const SRect& localRect);
    ShapeBuilder& setPrimitiveFill(int idx, SColor c);
    ShapeBuilder& setPrimitiveStroke(int idx, SColor c);
    ShapeBuilder& setPrimitiveLineWidth(int idx, float w);
    ShapeBuilder& setPrimitiveRadius(int idx, float r);
    ShapeBuilder& setPrimitiveRingWidth(int idx, float w);
    ShapeBuilder& setPrimitivePoints(int idx, const std::vector<Shape::SPointF>& pts);
    std::shared_ptr<Shape> build(void);
};
