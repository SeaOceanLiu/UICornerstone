
#ifndef UtilityH
#define UtilityH
#include <type_traits>
#include <SDL3/SDL_rect.h>
#include <array>
#include <cmath>
#include <string>
#include <functional>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class SMultipleSize{
public:
    float scaleX;
    float scaleY;
    SMultipleSize():scaleX(1), scaleY(1){}
    SMultipleSize(float x, float y):scaleX(x), scaleY(y){}
    SMultipleSize(const SMultipleSize& m):scaleX(m.scaleX), scaleY(m.scaleY){}
    SMultipleSize(const SMultipleSize&& m):scaleX(m.scaleX), scaleY(m.scaleY){}

    SMultipleSize& operator=(const SMultipleSize& m){
        scaleX = m.scaleX;
        scaleY = m.scaleY;
        return *this;
    }
    SMultipleSize& operator=(const SMultipleSize&& m){
        scaleX = m.scaleX;
        scaleY = m.scaleY;
        return *this;
    }
    bool operator==(const SMultipleSize& m){
        return scaleX == m.scaleX && scaleY == m.scaleY;
    }
    SMultipleSize operator*(const SMultipleSize& m){
        return SMultipleSize(scaleX * m.scaleX, scaleY * m.scaleY);
    }
};

class Margin{
public:
    float left;
    float top;
    float right;
    float bottom;
    Margin():left(0), top(0), right(0), bottom(0){}
    Margin(float l, float t, float r, float b):left(l), top(t), right(r), bottom(b){}
    Margin(const Margin& m):left(m.left), top(m.top), right(m.right), bottom(m.bottom){}
    Margin(const Margin&& m):left(m.left), top(m.top), right(m.right), bottom(m.bottom){}
    Margin& operator=(const Margin& m){
        left = m.left;
        top = m.top;
        right = m.right;
        bottom = m.bottom;
        return *this;
    }
    Margin& operator=(const Margin&& m){
        left = m.left;
        top = m.top;
        right = m.right;
        bottom = m.bottom;
        return *this;
    }
    bool operator==(const Margin& m){
        return left == m.left && top == m.top && right == m.right && bottom == m.bottom;
    }
    Margin operator+(const Margin& m){
        return Margin(left + m.left, top + m.top, right + m.right, bottom + m.bottom);
    }
    Margin operator-(const Margin& m){
        return Margin(left - m.left, top - m.top, right - m.right, bottom - m.bottom);
    }
    Margin operator*(const SMultipleSize& ms){
        return Margin(left * ms.scaleX, top * ms.scaleY, right * ms.scaleX, bottom * ms.scaleY);
    }
};

class SSize{
public:
    float width;
    float height;
    SSize():width(0), height(0){}
    SSize(float w, float h):width(w), height(h){}
    SSize(const SSize& s):width(s.width), height(s.height){}
    SSize(const SSize&& s):width(s.width), height(s.height){}
    SSize& operator=(const SSize& s){
        width = s.width;
        height = s.height;
        return *this;
    }
    SSize& operator=(const SSize&& s){
        width = s.width;
        height = s.height;
        return *this;
    }
    bool operator==(const SSize& s){
        return width == s.width && height == s.height;
    }
    SSize operator+(const SSize& s){
        return SSize(width + s.width, height + s.height);
    }
    SSize operator-(const SSize& s){
        return SSize(width - s.width, height - s.height);
    }
    SSize operator*(const SMultipleSize& m){
        return SSize(width * m.scaleX, height * m.scaleY);
    }
};
class SPoint{
public:
    float x;
    float y;
    SPoint():x(0), y(0){}
    SPoint(float x, float y):x(x), y(y){}
    SPoint(const SPoint& p):x(p.x), y(p.y){}
    SPoint(const SPoint&& p):x(p.x), y(p.y){}
    SPoint& operator=(const SPoint& p){
        x = p.x;
        y = p.y;
        return *this;
    }
    SPoint& operator=(const SPoint&& p){
        x = p.x;
        y = p.y;
        return *this;
    }
    bool operator==(const SPoint& p) const {
        return x == p.x && y == p.y;
    }
    SPoint operator+(const SPoint& p) const {
        return SPoint(x + p.x, y + p.y);
    }
    SPoint operator+(const SPoint && p) const {
        return SPoint(x + p.x, y + p.y);
    }
    SPoint operator+=(const SPoint& p){
        x += p.x;
        y += p.y;
        return *this;
    }
    SPoint operator+=(const SPoint&& p){
        x += p.x;
        y += p.y;
        return *this;
    }
    SPoint operator-(const SPoint& p) const {
        return SPoint(x - p.x, y - p.y);
    }
    SPoint operator-(const SPoint&& p) const {
        return SPoint(x - p.x, y - p.y);
    }
    SPoint operator-=(const SPoint& p){
        x -= p.x;
        y -= p.y;
        return *this;
    }
    SPoint operator-=(const SPoint&& p){
        x -= p.x;
        y -= p.y;
        return *this;
    }
    SPoint operator*(const SMultipleSize& m) const {
        return SPoint(x * m.scaleX, y * m.scaleY);
    }
    SPoint operator*(const SMultipleSize&& m) const {
        return SPoint(x * m.scaleX, y * m.scaleY);
    }
    SPoint operator*=(const SMultipleSize& m){
        x *= m.scaleX;
        y *= m.scaleY;
        return *this;
    }
    SPoint operator*=(const SMultipleSize&& m){
        x *= m.scaleX;
        y *= m.scaleY;
        return *this;
    }
    SDL_FPoint toSDLFPoint(void) const{
        return SDL_FPoint{x, y};
    }
};

class SRect{
private:
    SDL_FRect m_sdlFRect;
public:
    float left;
    float top;
    float width;
    float height;
    bool leftIsPct = false, topIsPct = false, widthIsPct = false, heightIsPct = false;
    float leftPct = 0, topPct = 0, widthPct = 0, heightPct = 0;
    SRect():left(0), top(0), width(0), height(0){}
    SRect(SPoint p, SSize s):left(p.x), top(p.y), width(s.width), height(s.height){ normalize(); }
    SRect(float l, float t, float w, float h):left(l), top(t), width(w), height(h){ normalize(); }
    SRect(const SRect& r):left(r.left), top(r.top), width(r.width), height(r.height),
        leftIsPct(r.leftIsPct), topIsPct(r.topIsPct), widthIsPct(r.widthIsPct), heightIsPct(r.heightIsPct),
        leftPct(r.leftPct), topPct(r.topPct), widthPct(r.widthPct), heightPct(r.heightPct){ normalize(); }
    SRect(const SRect&& r):left(r.left), top(r.top), width(r.width), height(r.height),
        leftIsPct(r.leftIsPct), topIsPct(r.topIsPct), widthIsPct(r.widthIsPct), heightIsPct(r.heightIsPct),
        leftPct(r.leftPct), topPct(r.topPct), widthPct(r.widthPct), heightPct(r.heightPct){ normalize(); }
    SRect(const SPoint& leftTop, const SPoint rightBottom):left(leftTop.x), top(leftTop.y), width(rightBottom.x - leftTop.x), height(rightBottom.y - leftTop.y){ normalize(); }
    // 标准化矩形，使得width和height为正值
    SRect& normalize(void){
        if(width < 0){
            left += width;
            width = -width;
        }
        if(height < 0){
            top += height;
            height = -height;
        }
        return *this;
    }
    SRect& operator=(const SRect& r){
        left = r.left;
        top = r.top;
        width = r.width;
        height = r.height;
        leftIsPct = r.leftIsPct; topIsPct = r.topIsPct;
        widthIsPct = r.widthIsPct; heightIsPct = r.heightIsPct;
        leftPct = r.leftPct; topPct = r.topPct;
        widthPct = r.widthPct; heightPct = r.heightPct;
        return normalize();
    }
    SRect& operator=(const SRect&& r){
        left = r.left;
        top = r.top;
        width = r.width;
        height = r.height;
        leftIsPct = r.leftIsPct; topIsPct = r.topIsPct;
        widthIsPct = r.widthIsPct; heightIsPct = r.heightIsPct;
        leftPct = r.leftPct; topPct = r.topPct;
        widthPct = r.widthPct; heightPct = r.heightPct;
        return normalize();
    }
    bool operator==(const SRect& r) const {
        return left == r.left && top == r.top && width == r.width && height == r.height;
    }
    bool contains(float x, float y){
        return x >= left && x <= left + width && y >= top && y <= top + height;
    }
    bool contains(const SPoint& p){
        return contains(p.x, p.y);
    }
    SRect operator+(const SPoint& p){
        return SRect(left + p.x, top + p.y, width, height);
    }
    SRect operator-(const SPoint& p){
        return SRect(left - p.x, top - p.y, width, height);
    }
    SRect operator*(const SMultipleSize& m){
        return SRect(left, top, width * m.scaleX, height * m.scaleY);
    }
    SDL_FRect* toSDLFRect(void){
        m_sdlFRect = SDL_FRect{left, top, width, height};
        return &m_sdlFRect;
    }
    SDL_Rect toSDLRect(void){
        return SDL_Rect{(int)left, (int)top, (int)width, (int)height};
    }
    float right(void){
        return left + width;
    }
    float bottom(void){
        return top + height;
    }
    SPoint center(void) const {
        return SPoint(left + width / 2, top + height / 2);
    }
    void resolve(float cw, float ch){
        if (leftIsPct) left = cw * leftPct / 100.0f;
        if (topIsPct) top = ch * topPct / 100.0f;
        if (widthIsPct) width = cw * widthPct / 100.0f;
        if (heightIsPct) height = ch * heightPct / 100.0f;
    }
};

// 旋转矩形类 - 使用中心点、尺寸和旋转角度表示
class SRotatedRect {
public:
    SPoint center;
    float width;
    float height;
    float rotation;  // 弧度，0表示不旋转，正角度表示逆时针旋转

    SRotatedRect() : center(0, 0), width(0), height(0), rotation(0) {}

    // 构造函数：从中心点、尺寸和旋转角度创建
    SRotatedRect(const SPoint& c, float w, float h, float rot = 0.0f)
        : center(c), width(w), height(h), rotation(rot) {}

    // 构造函数：从轴对齐矩形创建（无旋转）
    SRotatedRect(const SRect& rect)
        : center(rect.center()), width(rect.width), height(rect.height), rotation(0.0f) {}

    // 构造函数：从四个角点创建（验证是否为矩形）
    SRotatedRect(const SPoint& p0, const SPoint& p1,
                 const SPoint& p2, const SPoint& p3);

    // 拷贝构造函数
    SRotatedRect(const SRotatedRect& r)
        : center(r.center), width(r.width), height(r.height), rotation(r.rotation) {}

    SRotatedRect(const SRotatedRect&& r)
        : center(r.center), width(r.width), height(r.height), rotation(r.rotation) {}

    SRotatedRect& operator=(const SRotatedRect& r) {
        center = r.center;
        width = r.width;
        height = r.height;
        rotation = r.rotation;
        return *this;
    }

    SRotatedRect& operator=(const SRotatedRect&& r) {
        center = r.center;
        width = r.width;
        height = r.height;
        rotation = r.rotation;
        return *this;
    }

    bool operator==(const SRotatedRect& r) const {
        return center == r.center && width == r.width &&
               height == r.height && rotation == r.rotation;
    }

    // 获取四个角点（按顺时针顺序：左上、右上、右下、左下）
    std::array<SPoint, 4> getCorners() const {
        std::array<SPoint, 4> corners;
        float halfW = width / 2.0f;
        float halfH = height / 2.0f;
        float cosA = std::cos(rotation);
        float sinA = std::sin(rotation);

        // 未旋转时的四个角点（相对于中心）
        float dx[4] = {-halfW, halfW, halfW, -halfW};
        float dy[4] = {-halfH, -halfH, halfH, halfH};

        for (int i = 0; i < 4; ++i) {
            // 应用旋转
            float x = dx[i] * cosA - dy[i] * sinA;
            float y = dx[i] * sinA + dy[i] * cosA;
            // 加上中心点坐标
            corners[i] = SPoint(center.x + x, center.y + y);
        }

        return corners;
    }

    // 获取轴对齐的边界框
    SRect getBoundingBox() const {
        auto corners = getCorners();
        float minX = corners[0].x, maxX = corners[0].x;
        float minY = corners[0].y, maxY = corners[0].y;

        for (int i = 1; i < 4; ++i) {
            if (corners[i].x < minX) minX = corners[i].x;
            if (corners[i].x > maxX) maxX = corners[i].x;
            if (corners[i].y < minY) minY = corners[i].y;
            if (corners[i].y > maxY) maxY = corners[i].y;
        }

        return SRect(minX, minY, maxX - minX, maxY - minY);
    }

    // 检查点是否在旋转矩形内
    bool contains(const SPoint& point) const {
        // 将点转换到矩形局部坐标系（考虑旋转）
        float cosA = std::cos(-rotation);  // 反向旋转
        float sinA = std::sin(-rotation);

        // 相对于中心点的向量
        float dx = point.x - center.x;
        float dy = point.y - center.y;

        // 应用反向旋转
        float localX = dx * cosA - dy * sinA;
        float localY = dx * sinA + dy * cosA;

        // 检查是否在未旋转的矩形内
        float halfW = width / 2.0f;
        float halfH = height / 2.0f;

        // 添加小的误差容忍度，处理浮点精度问题
        const float epsilon = 0.0f;//0.0001f;
        return (localX >= -halfW - epsilon && localX <= halfW + epsilon &&
                localY >= -halfH - epsilon && localY <= halfH + epsilon);
    }

    // 转换为轴对齐矩形（丢弃旋转信息）
    SRect toAxisAlignedRect() const {
        return SRect(center.x - width/2, center.y - height/2, width, height);
    }

    // 应用变换
    SRotatedRect operator+(const SPoint& p) const {
        return SRotatedRect(center + p, width, height, rotation);
    }

    SRotatedRect operator-(const SPoint& p) const {
        return SRotatedRect(center - p, width, height, rotation);
    }

    SRotatedRect operator*(const SMultipleSize& m) const {
        return SRotatedRect(center * m, width * m.scaleX, height * m.scaleY, rotation);
    }

    // 设置旋转角度（弧度）
    void setRotation(float rot) {
        rotation = rot;
        // 规范化角度到 [0, 2π)
        while (rotation < 0) rotation += 2.0f * M_PI;
        while (rotation >= 2.0f * M_PI) rotation -= 2.0f * M_PI;
    }

    // 设置旋转角度（度）
    void setRotationDegrees(float deg) {
        setRotation(deg * M_PI / 180.0f);
    }

    // 获取旋转角度（度）
    float getRotationDegrees() const {
        return rotation * 180.0f / M_PI;
    }

    // 验证矩形是否有效（尺寸非负）
    bool isValid() const {
        return width >= 0 && height >= 0;
    }
};

// SRotatedRect从四个角点创建的实现（支持任意顺序）
inline SRotatedRect::SRotatedRect(const SPoint& p0, const SPoint& p1,
                                 const SPoint& p2, const SPoint& p3) {
    // 更简单且健壮的方法：找到轴对齐边界框，然后确定旋转

    // 1. 收集所有点
    SPoint points[4] = {p0, p1, p2, p3};

    // 2. 找到最小和最大的x、y值
    float minX = points[0].x, maxX = points[0].x;
    float minY = points[0].y, maxY = points[0].y;

    for (int i = 1; i < 4; ++i) {
        if (points[i].x < minX) minX = points[i].x;
        if (points[i].x > maxX) maxX = points[i].x;
        if (points[i].y < minY) minY = points[i].y;
        if (points[i].y > maxY) maxY = points[i].y;
    }

    // 3. 计算宽度、高度和中心点
    width = maxX - minX;
    height = maxY - minY;
    center = SPoint((minX + maxX) / 2.0f, (minY + maxY) / 2.0f);

    // 4. 初始假设旋转角度为0
    rotation = 0.0f;

    // 5. 尝试确定旋转角度
    // 方法：对于每个点，计算它到中心的距离和角度
    // 对于矩形，应该有4个不同的角度，每90度一个

    // 计算所有点到中心的距离和角度
    float distances[4];
    float angles[4];

    for (int i = 0; i < 4; ++i) {
        float dx = points[i].x - center.x;
        float dy = points[i].y - center.y;
        distances[i] = std::sqrt(dx * dx + dy * dy);
        angles[i] = std::atan2(dy, dx);
    }

    // 6. 确定旋转角度
    // 优化：检查是否为轴对齐矩形
    // 轴对齐矩形的特征：有两对点具有相同的x坐标，两对点具有相同的y坐标

    // 收集所有x和y坐标
    float xCoords[4], yCoords[4];
    for (int i = 0; i < 4; ++i) {
        xCoords[i] = points[i].x;
        yCoords[i] = points[i].y;
    }

    // 检查x坐标：应该有两个不同的值，每个值出现两次
    // 检查y坐标：应该有两个不同的值，每个值出现两次
    bool isAxisAligned = true;

    // 检查x坐标
    int xCount[4] = {0};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (std::abs(xCoords[i] - xCoords[j]) < 0.001f) {
                xCount[i]++;
            }
        }
    }

    // 每个x坐标应该出现2次
    for (int i = 0; i < 4; ++i) {
        if (xCount[i] != 2) {
            isAxisAligned = false;
            break;
        }
    }

    // 检查y坐标
    if (isAxisAligned) {
        int yCount[4] = {0};
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (std::abs(yCoords[i] - yCoords[j]) < 0.001f) {
                    yCount[i]++;
                }
            }
        }

        // 每个y坐标应该出现2次
        for (int i = 0; i < 4; ++i) {
            if (yCount[i] != 2) {
                isAxisAligned = false;
                break;
            }
        }
    }

    if (isAxisAligned) {
        // 轴对齐矩形，旋转角度为0
        rotation = 0.0f;
    } else {
        // 可能是旋转矩形
        // 尝试找到矩形的方向
        // 方法：计算所有可能的边，找到最接近水平或垂直的边

        float bestAngle = 0.0f;
        float bestScore = 1000.0f; // 初始化为大值

        // 检查所有可能的边（点对）
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) {
                float dx = points[j].x - points[i].x;
                float dy = points[j].y - points[i].y;
                float length = std::sqrt(dx * dx + dy * dy);

                if (length > 0.001f) {
                    // 计算边的角度
                    float angle = std::atan2(dy, dx);

                    // 规范化角度到[-π/4, π/4]或[3π/4, 5π/4]范围内
                    // 这表示边接近水平或垂直
                    float normalized = angle;
                    while (normalized < -M_PI/4) normalized += M_PI/2;
                    while (normalized > M_PI/4) normalized -= M_PI/2;

                    float score = std::abs(normalized);
                    if (score < bestScore) {
                        bestScore = score;
                        bestAngle = angle;
                    }
                }
            }
        }

        rotation = bestAngle;
    }

    // 7. 规范化旋转角度到[0, 2π)
    while (rotation < 0) rotation += 2.0f * M_PI;
    while (rotation >= 2.0f * M_PI) rotation -= 2.0f * M_PI;

    // 注意：对于轴对齐矩形，这个算法应该能正确工作
    // 对于旋转矩形，可能需要更复杂的算法，但这是一个好的起点
}

// ============================================================================
// 确保函数在对象析构时调用
// ============================================================================

// final_action类，用于确保在对象析构时调用某个函数
// 使用方法：在对象析构时调用某个函数，可以使用finally()函数
// 例如：finally([](){ std::cout << "Object is being destroyed!" << std::endl; });
template<class F>
class final_action{
public:
    static_assert(!std::is_reference<F>::value && !std::is_const<F>::value &&
                    !std::is_volatile<F>::value,
                    "Final_action must be a callable type");
    // 构造函数，传进来的东西，比如函数，就会赋值给自己的成员F m_f;
    explicit final_action(F f) noexcept : m_f(std::move(f)){}
    final_action(final_action&& other) noexcept :
        m_f(std::move(other.m_f)), m_invoke(other.m_invoke)
    {}
    final_action(const final_action& other) = delete;
    final_action& operator=(const final_action& other) = delete;
    final_action& operator=(final_action&& other) = delete;

    // 析构函数时候，就调用m_f()函数，也就是构造函数时候传进来的函数
    ~final_action() noexcept{
        if(m_invoke){
            m_f();
        }
    }
private:
    F m_f;
    bool m_invoke = true; // 默认为true，表示析构时调用m_f
};
template<class F>
final_action<typename std::remove_cv<typename std::remove_reference<F>::type>::type>
finally(F&& f) noexcept{
    return final_action<typename std::remove_cv<typename std::remove_reference<F>::type>::type>(
        std::forward<F>(f));
}

// ============================================================================
// 几何工具函数
// ============================================================================

/**
 * @brief 计算两点之间的叉积 (p1-p0) × (p2-p0)
 * @param p0 起点
 * @param p1 第一个点
 * @param p2 第二个点
 * @return 叉积值（标量，对于2D点，实际上是z分量）
 */
inline float cross(const SPoint& p0, const SPoint& p1, const SPoint& p2) {
    return (p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y);
}

/**
 * @brief 判断点p是否在由点a和点b定义的直线的同一侧（相对于参考点ref）
 * @param a 直线上的第一个点
 * @param b 直线上的第二个点
 * @param ref 参考点（用于确定哪一侧）
 * @param p 要测试的点
 * @param epsilon 浮点误差容忍度
 * @return 如果p和ref在直线的同一侧返回true，否则返回false
 */
inline bool isPointOnSameSide(const SPoint& a, const SPoint& b,
                             const SPoint& ref, const SPoint& p,
                             float epsilon = 1e-6f) {
    float cross1 = cross(a, b, ref);
    float cross2 = cross(a, b, p);

    // 处理接近零的情况
    if (std::abs(cross1) < epsilon) cross1 = 0;
    if (std::abs(cross2) < epsilon) cross2 = 0;

    // 如果两个叉积同号（或其中一个为0），则在同侧
    return (cross1 * cross2 >= 0);
}

/**
 * @brief 判断点是否在凸多边形内（点集必须按顺时针或逆时针顺序排列）
 * @param points 多边形顶点数组（按顺序）
 * @param pointCount 顶点数量
 * @param testPoint 要测试的点
 * @return 如果点在凸多边形内返回true，否则返回false
 */
inline bool isPointInConvexPolygon(const SPoint* points, int pointCount,
                                  const SPoint& testPoint) {
    if (pointCount < 3) return false;

    // 计算第一个叉积作为参考
    float firstCross = cross(points[0], points[1], testPoint);

    for (int i = 1; i < pointCount; ++i) {
        int next = (i + 1) % pointCount;
        float currentCross = cross(points[i], points[next], testPoint);

        // 如果叉积符号与第一个不同，则点不在多边形内
        if (firstCross * currentCross < 0) {
            return false;
        }
    }

    return true;
}

/**
 * @brief 判断点是否在凸多边形内（使用std::vector或数组的版本）
 * @tparam Container 容器类型（必须支持size()和operator[]）
 * @param points 多边形顶点容器
 * @param testPoint 要测试的点
 * @return 如果点在凸多边形内返回true，否则返回false
 */
template<typename Container>
inline bool isPointInConvexPolygon(const Container& points,
                                  const SPoint& testPoint) {
    if (points.size() < 3) return false;

    // 计算第一个叉积作为参考
    float firstCross = cross(points[0], points[1], testPoint);

    for (size_t i = 1; i < points.size(); ++i) {
        size_t next = (i + 1) % points.size();
        float currentCross = cross(points[i], points[next], testPoint);

        // 如果叉积符号与第一个不同，则点不在多边形内
        if (firstCross * currentCross < 0) {
            return false;
        }
    }

    return true;
}

/**
 * @brief 使用射线法判断点是否在简单多边形内
 * @param points 多边形顶点数组
 * @param pointCount 顶点数量
 * @param testPoint 要测试的点
 * @return 如果点在多边形内返回true，在边界上或外部返回false
 */
inline bool isPointInPolygon(const SPoint* points, int pointCount,
                            const SPoint& testPoint) {
    if (pointCount < 3) return false;

    bool inside = false;

    for (int i = 0, j = pointCount - 1; i < pointCount; j = i++) {
        const SPoint& p1 = points[i];
        const SPoint& p2 = points[j];

        // 检查点是否在多边形边界上
        if ((p1.y == testPoint.y && p2.y == testPoint.y &&
             ((p1.x <= testPoint.x && testPoint.x <= p2.x) ||
              (p2.x <= testPoint.x && testPoint.x <= p1.x)))) {
            return false; // 在水平边上
        }

        if ((p1.x == testPoint.x && p2.x == testPoint.x &&
             ((p1.y <= testPoint.y && testPoint.y <= p2.y) ||
              (p2.y <= testPoint.y && testPoint.y <= p1.y)))) {
            return false; // 在垂直边上
        }

        // 射线法核心逻辑
        if (((p1.y > testPoint.y) != (p2.y > testPoint.y)) &&
            (testPoint.x < (p2.x - p1.x) * (testPoint.y - p1.y) / (p2.y - p1.y) + p1.x)) {
            inside = !inside;
        }
    }

    return inside;
}

/**
 * @brief 使用射线法判断点是否在简单多边形内（容器版本）
 * @tparam Container 容器类型（必须支持size()和operator[]）
 * @param points 多边形顶点容器
 * @param testPoint 要测试的点
 * @return 如果点在多边形内返回true，否则返回false
 */
template<typename Container>
inline bool isPointInPolygon(const Container& points,
                            const SPoint& testPoint) {
    if (points.size() < 3) return false;

    bool inside = false;

    for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
        const SPoint& p1 = points[i];
        const SPoint& p2 = points[j];

        // 检查点是否在多边形边界上
        if ((p1.y == testPoint.y && p2.y == testPoint.y &&
             ((p1.x <= testPoint.x && testPoint.x <= p2.x) ||
              (p2.x <= testPoint.x && testPoint.x <= p1.x)))) {
            return false; // 在水平边上
        }

        if ((p1.x == testPoint.x && p2.x == testPoint.x &&
             ((p1.y <= testPoint.y && testPoint.y <= p2.y) ||
              (p2.y <= testPoint.y && testPoint.y <= p1.y)))) {
            return false; // 在垂直边上
        }

        // 射线法核心逻辑
        if (((p1.y > testPoint.y) != (p2.y > testPoint.y)) &&
            (testPoint.x < (p2.x - p1.x) * (testPoint.y - p1.y) / (p2.y - p1.y) + p1.x)) {
            inside = !inside;
        }
    }

    return inside;
}

/**
 * @brief 判断点是否在多边形的一侧（内部或外部）
 * @param points 多边形顶点数组
 * @param pointCount 顶点数量
 * @param testPoint 要测试的点
 * @param checkInside 如果为true检查是否在多边形内，如果为false检查是否在多边形外
 * @return 根据checkInside参数返回点是否在指定的一侧
 */
inline bool isPointOnPolygonSide(const SPoint* points, int pointCount,
                                const SPoint& testPoint, bool checkInside = true) {
    bool inside = isPointInPolygon(points, pointCount, testPoint);
    return checkInside ? inside : !inside;
}

/**
 * @brief 判断点是否在多边形的一侧（容器版本）
 * @tparam Container 容器类型
 * @param points 多边形顶点容器
 * @param testPoint 要测试的点
 * @param checkInside 如果为true检查是否在多边形内，如果为false检查是否在多边形外
 * @return 根据checkInside参数返回点是否在指定的一侧
 */
template<typename Container>
inline bool isPointOnPolygonSide(const Container& points,
                                const SPoint& testPoint, bool checkInside = true) {
    bool inside = isPointInPolygon(points, testPoint);
    return checkInside ? inside : !inside;
}

/**
 * @brief 用省略号截断文本，使其不超过最大宽度
 * @param text 原始文本
 * @param maxWidth 最大允许宽度（像素）
 * @param measureFn 测量字符串宽度的回调
 * @return 截断后的文本（可能追加"..."）
 */
inline std::string truncateText(const std::string& text, float maxWidth,
    const std::function<float(const std::string&)>& measureFn)
{
    if (maxWidth <= 0 || text.empty()) return "";
    if (measureFn(text) <= maxWidth) return text;

    const std::string ellipsis3 = "...";
    const std::string ellipsis2 = "..";
    const std::string ellipsis1 = ".";

    float w3 = measureFn(ellipsis3);
    float w2 = measureFn(ellipsis2);
    float w1 = measureFn(ellipsis1);

    if (maxWidth >= w3) {
        int low = 0;
        int high = (int)text.length();
        while (low < high) {
            int mid = (low + high + 1) / 2;
            std::string test = text.substr(0, mid) + ellipsis3;
            if (measureFn(test) <= maxWidth)
                low = mid;
            else
                high = mid - 1;
        }
        return text.substr(0, low) + ellipsis3;
    } else if (maxWidth >= w2) {
        return ellipsis2;
    } else if (maxWidth >= w1) {
        return ellipsis1;
    }
    return "";
}

#endif // UtilityH
