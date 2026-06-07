#include "GraphTool.h"
#include "Texture.h"

namespace GraphTool {

// ==================== DrawingContext 实现 ====================

// 构造函数
DrawingContext::DrawingContext(RenderDevice* device)
    : m_renderDevice(device)
    , m_pen(SColor::Black(), 1.0f)
    , m_brush(SColor::White())
    , m_fontSize(12.0f)
    , m_cornerStyle(CornerStyle::Round)
{
}

// 字体设置
void DrawingContext::setFont(const std::string& fontName, float size) {
    m_fontName = fontName;
    m_fontSize = size > 0 ? size : 12.0f;
}

// drawPoint - 浮点坐标
void DrawingContext::drawPoint(float x, float y) {
    if (!m_renderDevice) return;
    m_renderDevice->setDrawColor(m_pen.color());
    m_renderDevice->drawPoint(x, y);
}

// drawPoint - 指定颜色
void DrawingContext::drawPoint(float x, float y, const SColor& color) {
    if (!m_renderDevice) return;
    m_renderDevice->setDrawColor(color);
    m_renderDevice->drawPoint(x, y);
}

// drawLine - 浮点坐标
void DrawingContext::drawLine(float x1, float y1, float x2, float y2) {
    if (!m_renderDevice || m_pen.width() <= 0) return;

    // 检查是否为虚线样式
    if (!m_pen.isSolid()) {
        drawDashedLine(x1, y1, x2, y2);
        return;
    }

    // 如果线宽为1或更小，使用默认绘制
    if (m_pen.width() <= 1.0f) {
        m_renderDevice->setDrawColor(m_pen.color());
        m_renderDevice->drawLine(x1, y1, x2, y2);
        return;
    }

    // 对于粗线，使用generateLineRectPoints工具函数生成矩形四个点
    ::SPoint start(x1, y1);
    ::SPoint end(x2, y2);

    auto rectPoints = Utils::generateLineRectPoints(start, end, m_pen.width());

    if (!rectPoints.isValid()) {
        drawPoint(x1, y1);
        return;
    }

    drawLine(rectPoints);
}

// 绘制虚线
void DrawingContext::drawDashedLine(float x1, float y1, float x2, float y2) {
    if (!m_renderDevice) return;

    std::vector<float> pattern = m_pen.getStandardDashPattern();
    if (pattern.empty()) {
        drawLine(x1, y1, x2, y2);
        return;
    }

    float totalLength = std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    if (totalLength < 0.001f) return;

    float patternLength = 0.0f;
    for (float len : pattern) {
        patternLength += len;
    }
    if (patternLength <= 0.0f) return;

    float dx = (x2 - x1) / totalLength;
    float dy = (y2 - y1) / totalLength;

    PenStyle oldStyle = m_pen.style();
    m_pen.setStyle(PenStyle::Solid);

    float offset = m_pen.dashOffset();
    float pos = -fmodf(offset, patternLength);
    if (pos > 0) pos -= patternLength;

    bool drawing = true;
    int patternIndex = 0;

    while (pos < totalLength) {
        float segLen = pattern[patternIndex % pattern.size()];
        float segStart = pos;
        float segEnd = pos + segLen;

        if (drawing && segEnd > 0 && segStart < totalLength) {
            float drawStart = std::max<float>(0.0f, segStart);
            float drawEnd = std::min<float>(totalLength, segEnd);

            float sx = x1 + dx * drawStart;
            float sy = y1 + dy * drawStart;
            float ex = x1 + dx * drawEnd;
            float ey = y1 + dy * drawEnd;

            if (m_pen.width() <= 1.0f) {
                m_renderDevice->setDrawColor(m_pen.color());
                m_renderDevice->drawLine(sx, sy, ex, ey);
            } else {
                ::SPoint s(sx, sy);
                ::SPoint e(ex, ey);
                auto rectPoints = Utils::generateLineRectPoints(s, e, m_pen.width());
                if (rectPoints.isValid()) {
                    drawLine(rectPoints);
                }
            }
        }

        pos = segEnd;
        patternIndex++;
        drawing = !drawing;
    }

    m_pen.setStyle(oldStyle);
}

// drawLine - Bresenham算法
void DrawingContext::drawLine(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    while (true) {
        drawPoint(x1, y1);
        if (x1 == x2 && y1 == y2)
            break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

// drawLine - SLineRectPoints
void DrawingContext::drawLine(const SLineRectPoints& rectPoints) {
    if (!m_renderDevice || !rectPoints.isValid()) return;

    m_renderDevice->drawQuad(
        rectPoints.startLeft.x, rectPoints.startLeft.y,
        rectPoints.startRight.x, rectPoints.startRight.y,
        rectPoints.endRight.x, rectPoints.endRight.y,
        rectPoints.endLeft.x, rectPoints.endLeft.y,
        m_pen.color()
    );
}

// drawRect
void DrawingContext::drawRect(const ::SRect& rect, bool filled) {
    if (!m_renderDevice) return;

    if (filled) {
        if (m_brush.isValid() && !m_brush.isTransparent()) {
            m_renderDevice->setDrawColor(m_brush.color());
            m_renderDevice->fillRect(SRect(rect.left, rect.top, rect.width, rect.height));
        }
        if (m_pen.isValid()) {
            m_renderDevice->setDrawColor(m_pen.color());
            m_renderDevice->drawRect(SRect(rect.left, rect.top, rect.width, rect.height));
        }
    } else {
        if (m_pen.width() <= 1.0f) {
            m_renderDevice->setDrawColor(m_pen.color());
            m_renderDevice->drawRect(SRect(rect.left, rect.top, rect.width, rect.height));
        } else {
            float halfWidth = m_pen.width() / 2.0f;

            ::SRect outerRect(
                rect.left - halfWidth,
                rect.top - halfWidth,
                rect.width + m_pen.width(),
                rect.height + m_pen.width()
            );

            ::SRect innerRect(
                rect.left + halfWidth,
                rect.top + halfWidth,
                rect.width - m_pen.width(),
                rect.height - m_pen.width()
            );

            if (innerRect.width <= 0 || innerRect.height <= 0) {
                SColor oldFillColor = m_brush.color();
                setFillColor(m_pen.color());
                drawRect(outerRect, true);
                setFillColor(oldFillColor);
                return;
            }

            m_renderDevice->setDrawColor(m_pen.color());

            // 上边区域
            m_renderDevice->fillRect(SRect(
                outerRect.left,
                outerRect.top,
                outerRect.width,
                halfWidth * 2
            ));

            // 下边区域
            m_renderDevice->fillRect(SRect(
                outerRect.left,
                outerRect.top + outerRect.height - halfWidth * 2,
                outerRect.width,
                halfWidth * 2
            ));

            // 左边区域
            m_renderDevice->fillRect(SRect(
                outerRect.left,
                outerRect.top + halfWidth * 2,
                halfWidth * 2,
                outerRect.height - halfWidth * 4
            ));

            // 右边区域
            m_renderDevice->fillRect(SRect(
                outerRect.left + outerRect.width - halfWidth * 2,
                outerRect.top + halfWidth * 2,
                halfWidth * 2,
                outerRect.height - halfWidth * 4
            ));
        }
    }
}

// drawRoundedRect - 简化版本
void DrawingContext::drawRoundedRect(const ::SRect& rect, float radius, bool filled) {
    drawRoundedRect(rect, SRoundedCorners(radius), filled);
}

// drawRoundedRect - 完整版本
void DrawingContext::drawRoundedRect(const ::SRect& rect, const SRoundedCorners& corners, bool filled) {
    if (!m_renderDevice) return;

    if (filled) {
        auto points = Utils::generateRoundedRectPoints(rect, corners);
        if (points.empty()) return;

        SColor currentColor = m_brush.color();

        if (points.size() >= 3) {
            for (size_t i = 1; i < points.size() - 1; ++i) {
                m_renderDevice->drawTriangle(
                    points[0].x, points[0].y,
                    points[i].x, points[i].y,
                    points[i + 1].x, points[i + 1].y,
                    currentColor
                );
            }
        }
    } else {
        if (m_pen.width() <= 1.0f) {
            auto points = Utils::generateRoundedRectPoints(rect, corners);
            if (points.empty()) return;

            m_renderDevice->setDrawColor(m_pen.color());

            for (size_t i = 0; i < points.size(); ++i) {
                size_t next = (i + 1) % points.size();
                m_renderDevice->drawLine(points[i].x, points[i].y, points[next].x, points[next].y);
            }
        } else {
            float halfWidth = m_pen.width() / 2.0f;

            ::SRect outerRect(
                rect.left - halfWidth,
                rect.top - halfWidth,
                rect.width + m_pen.width(),
                rect.height + m_pen.width()
            );

            ::SRect innerRect(
                rect.left + halfWidth,
                rect.top + halfWidth,
                rect.width - m_pen.width(),
                rect.height - m_pen.width()
            );

            SRoundedCorners outerCorners(
                corners.topLeft + halfWidth,
                corners.topRight + halfWidth,
                corners.bottomRight + halfWidth,
                corners.bottomLeft + halfWidth
            );

            SRoundedCorners innerCorners(
                std::max<float>(0.0f, corners.topLeft - halfWidth),
                std::max<float>(0.0f, corners.topRight - halfWidth),
                std::max<float>(0.0f, corners.bottomRight - halfWidth),
                std::max<float>(0.0f, corners.bottomLeft - halfWidth)
            );

            if (innerRect.width <= 0 || innerRect.height <= 0) {
                SColor oldFillColor = m_brush.color();
                setFillColor(m_pen.color());
                drawRoundedRect(outerRect, outerCorners, true);
                setFillColor(oldFillColor);
                return;
            }

            auto outerPoints = Utils::generateRoundedRectPoints(outerRect, outerCorners);
            auto innerPoints = Utils::generateRoundedRectPoints(innerRect, innerCorners);

            if (outerPoints.empty() || innerPoints.empty()) return;

            m_renderDevice->setDrawColor(m_pen.color());

            int segmentsPerCorner = 8;
            int verticesPerCorner = segmentsPerCorner + 1;

            // 绘制四个角
            for (int corner = 0; corner < 4; ++corner) {
                int startIdx = corner * verticesPerCorner;
                int endIdx = startIdx + verticesPerCorner;

                if (endIdx > (int)outerPoints.size() || endIdx > (int)innerPoints.size()) {
                    continue;
                }

                for (int i = startIdx; i < endIdx - 1; ++i) {
                    int next = i + 1;
                    m_renderDevice->drawQuad(
                        innerPoints[i].x, innerPoints[i].y,
                        outerPoints[i].x, outerPoints[i].y,
                        outerPoints[next].x, outerPoints[next].y,
                        innerPoints[next].x, innerPoints[next].y,
                        m_pen.color()
                    );
                }
            }

            // 绘制四条边
            auto drawEdgeStrip = [&](int start, int end) {
                for (int i = start; i < end; ++i) {
                    if (i + 1 >= (int)outerPoints.size() || i + 1 >= (int)innerPoints.size()) break;

                    m_renderDevice->drawQuad(
                        innerPoints[i].x, innerPoints[i].y,
                        outerPoints[i].x, outerPoints[i].y,
                        outerPoints[i + 1].x, outerPoints[i + 1].y,
                        innerPoints[i + 1].x, innerPoints[i + 1].y,
                        m_pen.color()
                    );
                }
            };

            // 上边
            drawEdgeStrip(verticesPerCorner * 1 - 1, verticesPerCorner * 2);
            // 右边
            drawEdgeStrip(verticesPerCorner * 2 - 1, verticesPerCorner * 3);
            // 下边
            drawEdgeStrip(verticesPerCorner * 3 - 1, verticesPerCorner * 4);

            // 左边（连接最后一个点和第一个点）
            if (outerPoints.size() > 0 && innerPoints.size() > 0) {
                int last = (int)outerPoints.size() - 1;

                m_renderDevice->drawQuad(
                    innerPoints[last].x, innerPoints[last].y,
                    outerPoints[last].x, outerPoints[last].y,
                    outerPoints[0].x, outerPoints[0].y,
                    innerPoints[0].x, innerPoints[0].y,
                    m_pen.color()
                );
            }
        }
    }
}

// drawRoundedRectWithBorder - 简化版本
void DrawingContext::drawRoundedRectWithBorder(const ::SRect& rect, float radius,
                                              float borderWidth, const SColor& borderColor,
                                              const SColor& fillColor) {
    drawRoundedRectWithBorder(rect, SRoundedCorners(radius), borderWidth, borderColor, fillColor);
}

// drawRoundedRectWithBorder - 完整版本
void DrawingContext::drawRoundedRectWithBorder(const ::SRect& rect, const SRoundedCorners& corners,
                                              float borderWidth, const SColor& borderColor,
                                              const SColor& fillColor) {
    if (!m_renderDevice || borderWidth <= 0) return;

    SColor oldPenColor = m_pen.color();
    SColor oldFillColor = m_brush.color();

    setFillColor(fillColor);
    drawRoundedRect(rect, corners, true);

    setPenColor(borderColor);
    setPenWidth(borderWidth);
    drawRoundedRect(rect, corners, false);

    setPenColor(oldPenColor);
    setFillColor(oldFillColor);
}

// drawFilledCircleWithStroke - 核心圆形绘制方法
void DrawingContext::drawFilledCircleWithStroke(int cx, int cy, float R, float thickness,
                                                const SColor& fillColor, const SColor& strokeColor) {
    if (!m_renderDevice || R <= 0) return;

    float Ro = R + thickness * 0.5f;
    float Ri = R - thickness * 0.5f;
    if (Ri < 0.0f) Ri = 0.0f;

    int minX = (int)floorf(cx - Ro) - 1;
    int maxX = (int)ceilf(cx + Ro)  + 1;
    int minY = (int)floorf(cy - Ro) - 1;
    int maxY = (int)ceilf(cy + Ro)  + 1;

    for (int py = minY; py <= maxY; py++) {
        float dy = (py + 0.5f) - cy;
        float dy2 = dy * dy;
        for (int px = minX; px <= maxX; px++) {
            float dx = (px + 0.5f) - cx;
            float D2 = dx*dx + dy2;

            float covInner = (Ri > 0.0f) ? coverageD2(D2, Ri) : 0.0f;
            float covOuter = coverageD2(D2, Ro);

            float covStroke = covOuter - covInner;
            float covFill = covInner;

            float fillAlpha = fillColor.alpha() * covFill;
            SColor fillPremult = {
                fillColor.red()   * fillAlpha,
                fillColor.green() * fillAlpha,
                fillColor.blue()  * fillAlpha,
                fillAlpha
            };

            float strokeAlpha = strokeColor.alpha() * covStroke;
            SColor strokePremult = {
                strokeColor.red()   * strokeAlpha,
                strokeColor.green() * strokeAlpha,
                strokeColor.blue()  * strokeAlpha,
                strokeAlpha
            };

            float invA = 1.0f - strokePremult.alpha();
            SColor out = {
                strokePremult.red()   + fillPremult.red()   * invA,
                strokePremult.green() + fillPremult.green() * invA,
                strokePremult.blue()  + fillPremult.blue()  * invA,
                strokePremult.alpha() + fillPremult.alpha() * invA
            };

            if (out.alpha() > 0.0f) {
                drawPoint(px, py, out);
            }
        }
    }
}

// drawFilledCircleWithStroke - 使用当前画笔/画刷
void DrawingContext::drawFilledCircleWithStroke(int cx, int cy, float R) {
    drawFilledCircleWithStroke(cx, cy, R, m_pen.width(), m_brush.color(), m_pen.color());
}

// drawFilledCircleAA - 抗锯齿填充圆（仅填充，无描边）
void DrawingContext::drawFilledCircleAA(float cx, float cy, float radius, const SColor& color) {
    drawFilledCircleWithStroke(static_cast<int>(cx), static_cast<int>(cy), radius, 0.0f, color, SColor::Transparent());
}

// drawCircleWithThickness - 带线宽的圆环绘制（仅描边，无填充）
void DrawingContext::drawCircleWithThickness(float cx, float cy, float radius, float thickness, const SColor& color) {
    drawFilledCircleWithStroke(static_cast<int>(cx), static_cast<int>(cy), radius, thickness, SColor::Transparent(), color);
}

// drawCircle - 根据filled参数
void DrawingContext::drawCircle(const ::SPoint &center, float radius, bool filled) {
    if (!m_renderDevice || radius <= 0) return;

    if (filled) {
        drawFilledCircleWithStroke(
            static_cast<int>(center.x), static_cast<int>(center.y),
            radius, 0.0f, m_brush.color(), SColor::Transparent());
    } else {
        drawFilledCircleWithStroke(
            static_cast<int>(center.x), static_cast<int>(center.y),
            radius, m_pen.width(), SColor::Transparent(), m_pen.color());
    }
}

// drawCircle - 指定填充色和描边色
void DrawingContext::drawCircle(const ::SPoint &center, float radius, const SColor& fillColor, const SColor& strokeColor, float strokeWidth) {
    if (!m_renderDevice || radius <= 0) return;
    drawFilledCircleWithStroke(
        static_cast<int>(center.x), static_cast<int>(center.y),
        radius, strokeWidth, fillColor, strokeColor);
}

// drawEllipse
void DrawingContext::drawEllipse(const ::SPoint& center, float radiusX, float radiusY, bool filled) {
    if (!m_renderDevice || radiusX <= 0 || radiusY <= 0) return;

    if (filled) {
        auto points = Utils::generateEllipsePoints(center, radiusX, radiusY);
        if (points.empty()) return;

        SColor currentColor = m_brush.color();

        if (points.size() >= 3) {
            for (size_t i = 1; i < points.size() - 1; ++i) {
                m_renderDevice->drawTriangle(
                    center.x, center.y,
                    points[i].x, points[i].y,
                    points[i + 1].x, points[i + 1].y,
                    currentColor
                );
            }
        }
    } else {
        if (m_pen.width() <= 1.0f) {
            auto points = Utils::generateEllipsePoints(center, radiusX, radiusY);
            if (points.empty()) return;

            m_renderDevice->setDrawColor(m_pen.color());

            for (size_t i = 0; i < points.size(); ++i) {
                size_t next = (i + 1) % points.size();
                m_renderDevice->drawLine(points[i].x, points[i].y, points[next].x, points[next].y);
            }
        } else {
            float innerRadiusX = radiusX - m_pen.width() / 2.0f;
            float outerRadiusX = radiusX + m_pen.width() / 2.0f;
            float innerRadiusY = radiusY - m_pen.width() / 2.0f;
            float outerRadiusY = radiusY + m_pen.width() / 2.0f;

            if (innerRadiusX <= 0 || innerRadiusY <= 0) {
                drawEllipse(center, outerRadiusX, outerRadiusY, true);
            } else {
                auto innerPoints = Utils::generateEllipsePoints(center, innerRadiusX, innerRadiusY, 72);
                auto outerPoints = Utils::generateEllipsePoints(center, outerRadiusX, outerRadiusY, 72);

                if (innerPoints.size() != outerPoints.size() || innerPoints.empty()) return;

                for (size_t i = 0; i < innerPoints.size(); ++i) {
                    size_t next = (i + 1) % innerPoints.size();

                    m_renderDevice->drawQuad(
                        innerPoints[i].x, innerPoints[i].y,
                        outerPoints[i].x, outerPoints[i].y,
                        outerPoints[next].x, outerPoints[next].y,
                        innerPoints[next].x, innerPoints[next].y,
                        m_pen.color()
                    );
                }
            }
        }
    }
}

// drawArc
void DrawingContext::drawArc(const ::SPoint& center, float radius, float startAngle, float endAngle, bool filled) {
    if (!m_renderDevice || radius <= 0) return;

    if (filled) {
        auto points = Utils::generateArcPoints(center, radius, startAngle, endAngle);
        if (points.size() < 2) return;

        SColor currentColor = m_brush.color();

        if (points.size() >= 2) {
            for (size_t i = 0; i < points.size() - 1; ++i) {
                m_renderDevice->drawTriangle(
                    center.x, center.y,
                    points[i].x, points[i].y,
                    points[i + 1].x, points[i + 1].y,
                    currentColor
                );
            }
        }
    } else {
        if (m_pen.width() <= 1.0f) {
            auto points = Utils::generateArcPoints(center, radius, startAngle, endAngle);
            if (points.size() < 2) return;

            m_renderDevice->setDrawColor(m_pen.color());

            for (size_t i = 0; i < points.size() - 1; ++i) {
                m_renderDevice->drawLine(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y);
            }
        } else {
            float innerRadius = radius - m_pen.width() / 2.0f;
            float outerRadius = radius + m_pen.width() / 2.0f;

            if (innerRadius <= 0) {
                auto points = Utils::generateArcPoints(center, outerRadius, startAngle, endAngle);
                if (points.size() < 2) return;

                for (size_t i = 0; i < points.size() - 1; ++i) {
                    m_renderDevice->drawTriangle(
                        center.x, center.y,
                        points[i].x, points[i].y,
                        points[i + 1].x, points[i + 1].y,
                        m_pen.color()
                    );
                }
            } else {
                auto innerPoints = Utils::generateArcPoints(center, innerRadius, startAngle, endAngle, 72);
                auto outerPoints = Utils::generateArcPoints(center, outerRadius, startAngle, endAngle, 72);

                if (innerPoints.size() != outerPoints.size() || innerPoints.size() < 2) return;

                for (size_t i = 0; i < innerPoints.size() - 1; ++i) {
                    m_renderDevice->drawQuad(
                        innerPoints[i].x, innerPoints[i].y,
                        outerPoints[i].x, outerPoints[i].y,
                        outerPoints[i + 1].x, outerPoints[i + 1].y,
                        innerPoints[i + 1].x, innerPoints[i + 1].y,
                        m_pen.color()
                    );
                }
            }
        }
    }
}

// drawPolygon
void DrawingContext::drawPolygon(const std::vector<::SPoint>& points, bool filled,
                                bool debugCorner, const SColor& debugColor) {
    if (!m_renderDevice || points.size() < 3) return;

    if (filled) {
        SColor currentColor = m_brush.color();

        if (points.size() >= 3) {
            for (size_t i = 1; i < points.size() - 1; ++i) {
                m_renderDevice->drawTriangle(
                    points[0].x, points[0].y,
                    points[i].x, points[i].y,
                    points[i + 1].x, points[i + 1].y,
                    currentColor
                );
            }
        }
    } else {
        if (m_pen.width() <= 1.0f) {
            m_renderDevice->setDrawColor(m_pen.color());

            for (size_t i = 0; i < points.size(); ++i) {
                size_t next = (i + 1) % points.size();
                m_renderDevice->drawLine(points[i].x, points[i].y, points[next].x, points[next].y);
            }
        } else {
            for (size_t i = 0; i < points.size(); ++i) {
                size_t next = (i + 1) % points.size();
                auto rectPoints = Utils::generateLineRectPoints(points[i], points[next], m_pen.width());
                drawLine(rectPoints);
                size_t prev = (i == 0) ? points.size() - 1 : i - 1;
                auto prevRectPoints = Utils::generateLineRectPoints(points[prev], points[i], m_pen.width());
                SPoint intersectPoint1 = Utils::lineIntersection(rectPoints.startLeft, rectPoints.endLeft,
                                                                prevRectPoints.startLeft, prevRectPoints.endLeft);
                SPoint intersectPoint2 = Utils::lineIntersection(rectPoints.startLeft, rectPoints.endLeft,
                                                                prevRectPoints.startRight, prevRectPoints.endRight);
                SPoint intersectPoint3 = Utils::lineIntersection(rectPoints.startRight, rectPoints.endRight,
                                                                prevRectPoints.startLeft, prevRectPoints.endLeft);
                SPoint intersectPoint4 = Utils::lineIntersection(rectPoints.startRight, rectPoints.endRight,
                                                                prevRectPoints.startRight, prevRectPoints.endRight);
                SRotatedRect currentRect = SRotatedRect(rectPoints.startLeft, rectPoints.startRight,
                                                        rectPoints.endRight, rectPoints.endLeft);
                SRotatedRect prevRect = SRotatedRect(prevRectPoints.startLeft, prevRectPoints.startRight,
                                                        prevRectPoints.endRight, prevRectPoints.endLeft);
                std::vector<SPoint> drawPolygon = {rectPoints.startLeft, rectPoints.startRight,
                                                rectPoints.endRight, rectPoints.endLeft,
                                                prevRectPoints.startLeft, prevRectPoints.startRight,
                                                prevRectPoints.endRight, prevRectPoints.endLeft};
                std::vector<SPoint> candidatePoints = {intersectPoint1, intersectPoint2, intersectPoint3, intersectPoint4};
                SPoint outerPoint;
                bool hasOuterPoint = false;
                for (const auto& point : candidatePoints) {
                    if (isPointOnPolygonSide(drawPolygon, point, false)){
                        hasOuterPoint = true;
                        outerPoint = point;
                        break;
                    }
                }

                if (hasOuterPoint == false) {
                    throw("GraphTool::drawPolygon: not found outerPoint, maybe parallel lines or coinciding lines.");
                    return;
                }

                SPoint cornerPoint1;
                (void)currentRect;
                (void)prevRect;
                // cornerPoint1 and cornerPoint2 calculation continues...
                // This is the complex corner joining logic from the original

                if (m_cornerStyle == CornerStyle::Round) {
                } else {
                    // 硬角：直接绘制四边形连接两个外角点
                    SColor fcolor = debugCorner ? debugColor : m_pen.color();

                    m_renderDevice->drawQuad(
                        points[i].x, points[i].y,
                        cornerPoint1.x, cornerPoint1.y,
                        outerPoint.x, outerPoint.y,
                        cornerPoint1.x, cornerPoint1.y,
                        fcolor
                    );
                }
            }
        }
    }
}

// drawPolyline
void DrawingContext::drawPolyline(const std::vector<::SPoint>& points, bool debugCorner, const SColor& debugColor) {
    if (!m_renderDevice || points.size() < 2) return;

    if (m_pen.width() <= 1.0f) {
        m_renderDevice->setDrawColor(m_pen.color());

        for (size_t i = 0; i < points.size() - 1; ++i) {
            m_renderDevice->drawLine(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y);
        }
    } else {
        for (size_t i = 0; i < points.size() - 1; ++i) {
            drawLine(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y);

            if (i > 0) {
                float dx1 = points[i].x - points[i - 1].x;
                float dy1 = points[i].y - points[i - 1].y;
                float len1 = std::sqrt(dx1 * dx1 + dy1 * dy1);

                float dx2 = points[i + 1].x - points[i].x;
                float dy2 = points[i + 1].y - points[i].y;
                float len2 = std::sqrt(dx2 * dx2 + dy2 * dy2);

                if (len1 > 0.001f && len2 > 0.001f) {
                    float ux1 = dx1 / len1;
                    float uy1 = dy1 / len1;
                    float ux2 = dx2 / len2;
                    float uy2 = dy2 / len2;

                    float halfWidth = m_pen.width() / 2.0f;

                    float nx1 = -uy1;
                    float ny1 = ux1;
                    float offsetX1 = points[i].x + halfWidth * nx1;
                    float offsetY1 = points[i].y + halfWidth * ny1;

                    float nx2 = -uy2;
                    float ny2 = ux2;
                    float offsetX2 = points[i].x + halfWidth * nx2;
                    float offsetY2 = points[i].y + halfWidth * ny2;

                    // Corner joining logic (simplified)
                    float dot = ux1 * ux2 + uy1 * uy2;
                    if (dot > 1.0f) dot = 1.0f;
                    if (dot < -1.0f) dot = -1.0f;
                    float angle = std::acos(dot);
                    float cross = ux1 * uy2 - uy1 * ux2;

                    if (debugCorner) {
                        SColor oldPenColor = m_pen.color();
                        SColor oldFillColor = m_brush.color();

                        setPenColor(debugColor);
                        setFillColor(debugColor);

                        if (m_cornerStyle == CornerStyle::Round) {
                            float angle1 = std::atan2(ny1, nx1);
                            float angle2 = std::atan2(ny2, nx2);

                            if (cross < 0) {
                                std::swap(angle1, angle2);
                                if (angle2 < angle1) angle2 += 2.0f * M_PI;
                            }

                            int segments = (std::max)(8, static_cast<int>(angle * 360.0f / M_PI));

                            for (int j = 0; j < segments; ++j) {
                                float t1 = static_cast<float>(j) / segments;
                                float t2 = static_cast<float>(j + 1) / segments;

                                float angle1_current = angle1 + (angle2 - angle1) * t1;
                                float angle2_current = angle1 + (angle2 - angle1) * t2;

                                float arcX1 = points[i].x + halfWidth * std::cos(angle1_current);
                                float arcY1 = points[i].y + halfWidth * std::sin(angle1_current);
                                float arcX2 = points[i].x + halfWidth * std::cos(angle2_current);
                                float arcY2 = points[i].y + halfWidth * std::sin(angle2_current);

                                m_renderDevice->drawTriangle(
                                    points[i].x, points[i].y,
                                    arcX1, arcY1,
                                    arcX2, arcY2,
                                    debugColor
                                );
                            }
                        } else {
                            float intersectX = (offsetX1 + offsetX2) / 2.0f;
                            float intersectY = (offsetY1 + offsetY2) / 2.0f;

                            m_renderDevice->drawQuad(
                                points[i].x, points[i].y,
                                offsetX1, offsetY1,
                                intersectX, intersectY,
                                offsetX2, offsetY2,
                                debugColor
                            );
                        }

                        setPenColor(oldPenColor);
                        setFillColor(oldFillColor);
                    } else {
                        if (m_cornerStyle == CornerStyle::Round) {
                            float angle1 = std::atan2(ny1, nx1);
                            float angle2 = std::atan2(ny2, nx2);

                            if (cross < 0) {
                                std::swap(angle1, angle2);
                                if (angle2 < angle1) angle2 += 2.0f * M_PI;
                            }

                            int segments = (std::max)(8, static_cast<int>(angle * 360.0f / M_PI));

                            for (int j = 0; j < segments; ++j) {
                                float t1 = static_cast<float>(j) / segments;
                                float t2 = static_cast<float>(j + 1) / segments;

                                float angle1_current = angle1 + (angle2 - angle1) * t1;
                                float angle2_current = angle1 + (angle2 - angle1) * t2;

                                float arcX1 = points[i].x + halfWidth * std::cos(angle1_current);
                                float arcY1 = points[i].y + halfWidth * std::sin(angle1_current);
                                float arcX2 = points[i].x + halfWidth * std::cos(angle2_current);
                                float arcY2 = points[i].y + halfWidth * std::sin(angle2_current);

                                m_renderDevice->drawTriangle(
                                    points[i].x, points[i].y,
                                    arcX1, arcY1,
                                    arcX2, arcY2,
                                    m_pen.color()
                                );
                            }
                        } else {
                            // 硬角
                            float a11 = ux1;
                            float a12 = -ux2;
                            float a21 = uy1;
                            float a22 = -uy2;

                            float det = a11 * a22 - a12 * a21;
                            float intersectX, intersectY;

                            if (std::abs(det) > 0.001f) {
                                float b1 = offsetX2 - offsetX1;
                                float b2 = offsetY2 - offsetY1;

                                float t1 = (b1 * a22 - a12 * b2) / det;
                                float t2 = (a11 * b2 - b1 * a21) / det;

                                intersectX = offsetX1 + t1 * ux1;
                                intersectY = offsetY1 + t1 * uy1;

                                float intersectX2 = offsetX2 + t2 * ux2;
                                float intersectY2 = offsetY2 + t2 * uy2;

                                if (std::abs(intersectX - intersectX2) > 0.1f || std::abs(intersectY - intersectY2) > 0.1f) {
                                    intersectX = (intersectX + intersectX2) / 2.0f;
                                    intersectY = (intersectY + intersectY2) / 2.0f;
                                }
                            } else {
                                intersectX = (offsetX1 + offsetX2) / 2.0f;
                                intersectY = (offsetY1 + offsetY2) / 2.0f;
                            }

                            m_renderDevice->drawQuad(
                                points[i].x, points[i].y,
                                offsetX1, offsetY1,
                                intersectX, intersectY,
                                offsetX2, offsetY2,
                                m_pen.color()
                            );
                        }
                    }
                }
            }
        }

        // 绘制线帽
        if (points.size() >= 2) {
            SColor oldFillColor = m_brush.color();
            setFillColor(m_pen.color());

            drawCircle(points[0], m_pen.width() / 2.0f, true);
            drawCircle(points[points.size() - 1], m_pen.width() / 2.0f, true);

            setFillColor(oldFillColor);
        }
    }
}

// drawText - SPoint版本
void DrawingContext::drawText(const ::SPoint& position, const std::string& text) {
    (void)position;
    (void)text;
}

// drawText - SRect版本
void DrawingContext::drawText(const ::SRect& bounds, const std::string& text, TextAlignment alignment) {
    (void)bounds;
    (void)text;
    (void)alignment;
}

// drawImage - SPoint版本
void DrawingContext::drawImage(const ::SPoint& position, Texture* texture) {
    if (!m_renderDevice || !texture) return;

    float w = (float)texture->width();
    float h = (float)texture->height();
    SRect destRect(position.x, position.y, w, h);
    m_renderDevice->drawTexture(texture, nullptr, &destRect);
}

// drawImage - SRect版本
void DrawingContext::drawImage(const ::SRect& destRect, Texture* texture) {
    if (!m_renderDevice || !texture) return;
    m_renderDevice->drawTexture(texture, nullptr, &destRect);
}

// drawImage - SRect+SRect版本
void DrawingContext::drawImage(const ::SRect& destRect, Texture* texture, const ::SRect& srcRect) {
    if (!m_renderDevice || !texture) return;
    m_renderDevice->drawTexture(texture, &srcRect, &destRect);
}

// pushClipRect
void DrawingContext::pushClipRect(const ::SRect& rect) {
    if (!m_renderDevice) return;
    m_renderDevice->setClipRect(rect);
    m_clipStack.push_back(rect);
}

// popClipRect
void DrawingContext::popClipRect() {
    if (!m_renderDevice || m_clipStack.empty()) return;
    m_clipStack.pop_back();
    if (m_clipStack.empty()) {
        m_renderDevice->clearClipRect();
    } else {
        m_renderDevice->setClipRect(m_clipStack.back());
    }
}

// pushTransform
void DrawingContext::pushTransform() {
    if (!m_renderDevice) return;
}

// popTransform
void DrawingContext::popTransform() {
    if (!m_renderDevice) return;
}

// scale
void DrawingContext::scale(float sx, float sy) {
    if (!m_renderDevice) return;
    (void)sx;
    (void)sy;
}

// ==================== AdvancedDrawing 实现 ====================

void AdvancedDrawing::drawGradientRectWithBrush(DrawingContext& ctx, const ::SRect& rect,
                                                const SLinearGradient& gradient, bool stroke) {
    int steps = 40;

    float dx = gradient.endX() - gradient.startX();
    float dy = gradient.endY() - gradient.startY();
    bool horizontal = std::abs(dx) >= std::abs(dy);

    float stepSize = horizontal ? rect.width / steps : rect.height / steps;

    for (int i = 0; i < steps; ++i) {
        float t = static_cast<float>(i) / steps;
        SColor color = gradient.getColorAt(t);

        ::SRect segment;
        if (horizontal) {
            segment = ::SRect(rect.left + i * stepSize, rect.top, stepSize + 0.5f, rect.height);
        } else {
            segment = ::SRect(rect.left, rect.top + i * stepSize, rect.width, stepSize + 0.5f);
        }

        ctx.setFillColor(color);
        ctx.drawRect(segment, true);
    }

    if (stroke) {
        ctx.drawRect(rect, false);
    }
}

void AdvancedDrawing::drawGradientRect(DrawingContext& ctx, const ::SRect& rect,
                                       const SColor& startColor, const SColor& endColor,
                                       bool horizontal) {
    SLinearGradient gradient;
    if (horizontal) {
        gradient = SLinearGradient(rect.left, rect.top, rect.left + rect.width, rect.top);
    } else {
        gradient = SLinearGradient(rect.left, rect.top, rect.left, rect.top + rect.height);
    }
    gradient.addStop(0.0f, startColor);
    gradient.addStop(1.0f, endColor);
    drawGradientRectWithBrush(ctx, rect, gradient);
}

void AdvancedDrawing::drawRadialGradientRect(DrawingContext& ctx, const ::SRect& rect,
                                             const SRadialGradient& gradient) {
    int stepX = 4;
    int stepY = 4;

    for (float y = rect.top; y < rect.top + rect.height; y += stepY) {
        for (float x = rect.left; x < rect.left + rect.width; x += stepX) {
            float w = std::min<float>(stepX, rect.left + rect.width - x);
            float h = std::min<float>(stepY, rect.top + rect.height - y);
            SColor color = gradient.getColorAt(gradient.getRatio(x, y));
            ctx.setFillColor(color);
            ctx.drawRect(::SRect(x, y, w, h), true);
        }
    }
}

void AdvancedDrawing::drawShadow(DrawingContext& ctx, const ::SRect& rect,
                                 float shadowSize, const SColor& shadowColor,
                                 float blurRadius) {
    int layers = static_cast<int>(blurRadius);
    for (int i = layers; i > 0; --i) {
        float alpha = shadowColor.alpha() * (static_cast<float>(i) / layers);
        float offset = shadowSize * (static_cast<float>(i) / layers);

        SColor layerColor = shadowColor.withAlpha(alpha);
        ::SRect shadowRect = ::SRect(rect.left + offset, rect.top + offset, rect.width, rect.height);

        ctx.setFillColor(layerColor);
        ctx.drawRect(shadowRect, true);
    }
}

void AdvancedDrawing::drawGlow(DrawingContext& ctx, std::function<void()> drawFunc,
                               float glowRadius, const SColor& glowColor,
                               int layers) {
    SPen oldPen = ctx.getPen();
    SBrush oldBrush = ctx.getBrush();

    for (int i = layers; i > 0; --i) {
        float currentRadius = glowRadius * (static_cast<float>(i) / layers);
        float alpha = glowColor.alpha() * (static_cast<float>(i) / layers);
        SColor currentColor = glowColor.withAlpha(alpha);

        ctx.pushTransform();
        ctx.scale(1.0f + currentRadius * 0.01f, 1.0f + currentRadius * 0.01f);

        ctx.setPenColor(currentColor);
        ctx.setFillColor(currentColor);

        drawFunc();

        ctx.popTransform();
    }

    drawFunc();

    ctx.setPen(oldPen);
    ctx.setBrush(oldBrush);
}

void AdvancedDrawing::drawStyledRect(DrawingContext& ctx, const ::SRect& rect,
                                     const SPen& pen, const SBrush& brush) {
    SPen oldPen = ctx.getPen();
    SBrush oldBrush = ctx.getBrush();

    ctx.setPen(pen);
    ctx.setBrush(brush);

    ctx.setFillColor(brush.color());
    ctx.drawRect(rect, true);
    ctx.drawRect(rect, false);

    ctx.setPen(oldPen);
    ctx.setBrush(oldBrush);
}

void AdvancedDrawing::drawStyledRoundedRect(DrawingContext& ctx, const ::SRect& rect,
                                            float radius, const SPen& pen, const SBrush& brush) {
    SPen oldPen = ctx.getPen();
    SBrush oldBrush = ctx.getBrush();

    ctx.setPen(pen);
    ctx.setBrush(brush);

    ctx.setFillColor(brush.color());
    ctx.drawRoundedRect(rect, radius, true);
    ctx.drawRoundedRect(rect, radius, false);

    ctx.setPen(oldPen);
    ctx.setBrush(oldBrush);
}

void AdvancedDrawing::drawStyledCircle(DrawingContext& ctx, const ::SPoint& center, float radius,
                                       const SPen& pen, const SBrush& brush) {
    SPen oldPen = ctx.getPen();
    SBrush oldBrush = ctx.getBrush();

    ctx.setPen(pen);
    ctx.setBrush(brush);

    ctx.drawCircle(center, radius, brush.color(), pen.color(), pen.width());

    ctx.setPen(oldPen);
    ctx.setBrush(oldBrush);
}

void AdvancedDrawing::drawStyledEllipse(DrawingContext& ctx, const ::SPoint& center,
                                        float radiusX, float radiusY,
                                        const SPen& pen, const SBrush& brush) {
    SPen oldPen = ctx.getPen();
    SBrush oldBrush = ctx.getBrush();

    ctx.setPen(pen);
    ctx.setBrush(brush);

    ctx.setFillColor(brush.color());
    ctx.drawEllipse(center, radiusX, radiusY, true);
    ctx.drawEllipse(center, radiusX, radiusY, false);

    ctx.setPen(oldPen);
    ctx.setBrush(oldBrush);
}

} // namespace GraphTool
