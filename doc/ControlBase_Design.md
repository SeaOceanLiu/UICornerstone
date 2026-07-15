# ControlBase 控件基类设计文档

## 1. 概述

ControlBase 是所有 UI 控件的基类，提供了控件生命周期管理、事件处理、绘制、缩放等核心功能。本文档描述了基类的设计规格和实现细节。

## 2. 设计规则

> 参见 [AGENTS.md](./AGENTS.md) 中的设计规则章节。

1. **所有位置数据存储规则**：
   - 所有的屏幕位置数据都存储为未缩放的值
   - 生成字体时，使用缩放后的字体大小
   - 绘制时才对缩放进行处理

2. **控件生命周期**：
   - `create()`: 初始创建控件，缺省情况下只有初始创建控件后，才能显示和处理事件
   - `recreate()`: 重新创建控件，主要用于在一些属性改变时需要重新创建控件的情况
   - `preDraw()`: 在绘制之前调用，负责绘制背景和边框

## 3. 类设计

### 3.1 Control 接口

```cpp
class Control{
protected:
    EventQueue *m_eventQueueInstance;

    virtual void recreate() = 0;

public:
    virtual ~Control() = default;
    virtual void create(void) = 0;
    virtual void setId(int id) = 0;
    virtual int getId(void) const = 0;
    virtual void update(void) = 0;
    virtual void preDraw() = 0;
    virtual void draw(void) = 0;
    virtual void resized(SRect newRect) = 0;
    virtual void moved(SRect newRect) = 0;

    virtual void setParent(Control *parent) = 0;
    virtual Control* getParent(void) = 0;

    virtual void setRect(SRect rect) = 0;
    virtual SRect getRect(void) = 0;
    virtual void setVisible(bool visible) = 0;
    virtual bool getVisible(void) = 0;
    virtual void setEnable(bool enable) = 0;
    virtual bool getEnable(void) = 0;

    virtual void setScaleX(float scale) = 0;
    virtual void setScaleY(float scale) = 0;
    virtual float getScaleXX(void) = 0;
    virtual float getScaleYY(void) = 0;

    virtual void setState(ControlState state) = 0;
    virtual ControlState getState(void) = 0;

    virtual shared_ptr<Control> getThis(void) = 0;
    virtual SRect getDrawRect(void) = 0;
    virtual SRect mapToDrawRect(SRect rect) = 0;
    virtual SPoint mapToDrawPoint(SPoint point) = 0;
    virtual bool isContainsPoint(float x, float y) = 0;

    virtual void onMouseEnter(float x, float y) = 0;
    virtual void onMouseLeave(float x, float y) = 0;

    //=== 后端抽象接口（Phase 2/5/8/11）===
    virtual RenderDevice* getRenderDevice(void) = 0;
    virtual void setRenderDevice(RenderDevice *device) = 0;
    virtual TextRenderer* getTextRenderer(void) = 0;
    virtual void setTextRenderer(TextRenderer *renderer) = 0;
    virtual ResourceProvider* getResourceProvider(void) = 0;
    virtual void setResourceProvider(ResourceProvider *provider) = 0;
};
```

### 3.2 ControlImpl 类

```cpp
class ControlImpl: virtual public Control, public enable_shared_from_this<ControlImpl>{
protected:
    bool m_isCreated;
    int m_id;
    bool m_visible;
    bool m_enable;

    float m_xScale;    // 自身缩放比例
    float m_yScale;
    float m_xxScale;   // 实际缩放比例（父控件缩放 × 自身缩放）
    float m_yyScale;

    RenderDevice *m_renderDevice;
    TextRenderer *m_textRenderer;
    ResourceProvider *m_resourceProvider;

    SRect m_rect;
    Margin m_margin;
    Control *m_parent;
    vector<shared_ptr<Control>> m_children;

    bool m_isTransparent;
    bool m_isBorderVisible;

    ControlState m_state;
    bool m_mouseInside;

    // ── 焦点系统（Phase 15+）──
    bool m_focused;                     // 是否聚焦
    bool m_focusable;                   // 能否聚焦
    int  m_tabIndex;                    // Tab 顺序，-1 = 不参与
    bool m_focusByKeyboard;             // 通过 Tab 而非鼠标聚焦
    bool m_showFocusRing;               // 是否绘制焦点环
    bool m_focusRingAlwaysVisible;      // 始终显示环（默认 true）
    SColor m_focusRingColor;            // 环颜色
    FocusRingStyle m_focusRingStyle;    // Solid / Dashed
    bool m_isFocusBoundary;             // 焦点作用域边界
    SRect m_frameDrawRect;              // 当前帧绘制矩形
    bool m_frameDrawRectValid;          // m_frameDrawRect 是否有效

    void recreate(void) override;
public:
    ControlImpl(Control *parent, float xScale=1.0f, float yScale=1.0f);
    ControlImpl(const ControlImpl& other);
    ~ControlImpl();   // 非默认析构，负责从 FocusManager 注销

    void create(void) override;
    void setId(int id) override { m_id = id; }
    int getId(void) const override { return m_id; }
    ControlImpl& operator=(const ControlImpl& other);

    void update(void) override;
    void preDraw() override;
    void draw(void) override;
    void resized(SRect newRect) override;
    void moved(SRect newRect) override;

    void setParent(Control *parent) override;
    Control* getParent(void) override;

    void setScaleX(float scale) override;
    void setScaleY(float scale) override;
    float getScaleXX(void) override;
    float getScaleYY(void) override;

    //=== RenderDevice 抽象接口 ===
    RenderDevice* getRenderDevice(void) override;
    void setRenderDevice(RenderDevice *device) override;
    TextRenderer* getTextRenderer(void) override;
    void setTextRenderer(TextRenderer *renderer) override;
    ResourceProvider* getResourceProvider(void) override;
    void setResourceProvider(ResourceProvider *provider) override;

    void setRect(SRect rect) override;
    SRect getRect(void) override;
    SRect getMarginedRect(void);
    virtual void setMargin(Margin margin);
    Margin getMargin(void) const;

    void show(void) override;
    void hide(void) override;
    void setVisible(bool visible) override;
    bool getVisible(void) override { return m_visible; }

    void setEnable(bool enable) override;
    bool getEnable(void) override { return m_enable; }

    void setTransparent(bool transparent);
    bool isTransparent(void) const;
    void setBorderVisible(bool visible);
    bool isBorderVisible(void) const;

    void setState(ControlState state) override;
    ControlState getState(void) override { return m_state; }

    void addControl(shared_ptr<Control> child);
    void removeControl(shared_ptr<Control> child);

    shared_ptr<Control> getThis(void) override;
    SRect getDrawRect(void) override;
    SRect mapToDrawRect(SRect rect) override;
    SPoint mapToDrawPoint(SPoint point) override;
    bool isContainsPoint(float x, float y) override;

    void onMouseEnter(float x, float y) override;
    void onMouseLeave(float x, float y) override;

    // ── 焦点 API ──
    void setFocused(bool focused, bool byKeyboard = false) override;
    bool getFocused() const override;
    bool isFocusable() const override;
    int getTabIndex() const override;
    void setTabIndex(int index) override;
    void setFocusable(bool focusable) override;
    bool isFocusBoundary() const override;
    void setShowFocusRing(bool show) override;
    void setFocusRingAlwaysVisible(bool always) override;
    void setFocusRingColor(SColor color) override;
    SColor getFocusRingColor() const;
    void setFocusRingStyle(FocusRingStyle style) override;
    FocusRingStyle getFocusRingStyle() const;
    virtual void onFocusGained(bool byKeyboard);
    virtual void onFocusLost();
    void drawFocusRing();
    void afterDraw();

    void inheritRenderer(void);
    void setDrawRectDirty(void);

protected:
    void drawBackground(const SRect *pDrawRect);
    void drawBorder(const SRect *pDrawRect);
};
```

### 3.3 Margin 结构

```cpp
struct Margin {
    float left;
    float top;
    float right;
    float bottom;
};
```

## 4. 核心功能

### 4.1 生命周期管理

#### create() - 初始创建

```cpp
void ControlImpl::create(void){
    if(!m_isCreated) {
        m_isCreated = true;
        setVisible(true);
    }
}
```

- 标记控件已创建
- 默认显示控件

#### recreate() - 重新创建

```cpp
void ControlImpl::recreate(void) {
    if(!m_isCreated) return;
    m_isCreated = false;

    if (typeid(*this) == typeid(ControlImpl)) {
        create();
    }
}
```

- 释放资源
- 重置创建标志
- 重新创建资源

#### preDraw() - 预绘制

```cpp
void ControlImpl::preDraw() {
    if (!getVisible()) return;

    SRect drawRect = getDrawRect();
    drawBackground(&drawRect);
    drawBorder(&drawRect);
}
```

- 绘制控件自身的背景
- 绘制控件自身的边框
- 不递归子控件（子控件的背景由各自的 draw() 调用 preDraw() 时处理）

#### draw() — 各控件绘制流程

所有控件的 `draw()` 遵循统一的调用顺序：

```cpp
void SomeControl::draw(void) {
    if (!getVisible()) return;

    ControlImpl::preDraw();    // ① 绘制自身背景 + 边框
    // ... 绘制自身内容 ...    // ② 绘制内容（文字、图片等）
    ControlImpl::draw();       // ③ 递归绘制子控件（父容器调用）
}
```

- ① `preDraw()` — 每个控件在 draw 开头绘制自己的背景和边框
- ② 绘制内容 — 控件特有的绘制逻辑
- ③ `ControlImpl::draw()` — 仅容器类（Panel、Button 等）需要，遍历 `m_children` 调用每个子控件的 `draw()`

**绘制入口**：`Bench::draw()` 负责启动整个绘制链：

```cpp
void Bench::draw(void){
    if(m_isLoading){
        ControlImpl::preDraw();
        // 绘制加载进度条...
        return;
    }
    if (!m_visible) return;
    Panel::draw();  // 内部调用 ControlImpl::preDraw() + ControlImpl::draw()
}
```

### 4.2 缩放处理

#### 实际缩放计算

```cpp
// 设置父控件时更新实际缩放
void ControlImpl::setParent(Control *parent){
    m_parent = parent;
    inheritRenderer();
    m_xxScale = (parent==nullptr?m_xScale:m_xScale*parent->getScaleXX());
    m_yyScale = (parent==nullptr?m_yScale:m_yScale*parent->getScaleYY());
}
```

`inheritRenderer()` 从父控件传播三个后端抽象实例（Phase 2/5/8/11）：
- `m_renderDevice` ← `parent->getRenderDevice()`
- `m_textRenderer` ← `parent->getTextRenderer()`
- `m_resourceProvider` ← `parent->getResourceProvider()`

三者通过 `setRenderDevice()`/`setTextRenderer()`/`setResourceProvider()` 设置，并递归传播到所有子控件。`setParent()` 中调用 `inheritRenderer()` 一并完成传播。

#### 坐标转换

```cpp
// 坐标点转换到绘制坐标
SPoint ControlImpl::mapToDrawPoint(SPoint point){
    SRect drawRect = getDrawRect();
    return {point.x * getScaleXX() + drawRect.left,
        point.y * getScaleYY() + drawRect.top};
}

// 矩形转换到绘制坐标
SRect ControlImpl::mapToDrawRect(SRect rect){
    SRect drawRect = getDrawRect();
    return {rect.left * getScaleXX() + drawRect.left,
        rect.top * getScaleYY() + drawRect.top,
        rect.width * getScaleXX(),
        rect.height * getScaleYY()};
}
```

#### 获取绘制区域

```cpp
SRect ControlImpl::getDrawRect(void){
    Control *parent = getParent();
    SRect parentDrawRect;
    if (parent != nullptr){
        parentDrawRect = parent->getDrawRect();
        return {m_rect.left * parent->getScaleXX() + parentDrawRect.left,
            m_rect.top * parent->getScaleYY() + parentDrawRect.top,
            m_rect.width * getScaleXX(),
            m_rect.height * getScaleYY()};
    }
    return {m_rect.left, m_rect.top, m_rect.width * getScaleXX(), m_rect.height * getScaleYY()};
}
```

### 4.3 Margin 支持

```cpp
void ControlImpl::setMargin(Margin margin){
    m_margin = margin;
    recreate();
}

SRect ControlImpl::getMarginedRect(void) {
    Margin margin = getMargin();
    SRect marginRect = {
        margin.left,
        margin.top,
        getRect().width - margin.left - margin.right,
        getRect().height - margin.top - margin.bottom
    };
    return marginRect.normalize();
}
```

### 4.4 子控件管理

```cpp
void ControlImpl::addControl(shared_ptr<Control> child){
    child->setParent(this);
    // RenderDevice/TextRenderer/ResourceProvider 通过 setParent → inheritRenderer() 传播
}

void ControlImpl::removeControl(shared_ptr<Control> child){
    // 从子控件列表中移除
}
```

### 4.5 焦点 API

焦点系统由 `ControlImpl` 基类统一管理，详见 [FocusSystem_Design.md](FocusSystem_Design.md)。

**核心成员**：

| 成员 | 类型 | 默认值 | 说明 |
|------|------|:------:|------|
| `m_focused` | `bool` | `false` | 当前是否聚焦 |
| `m_focusable` | `bool` | `false` | 是否可聚焦 |
| `m_tabIndex` | `int` | `-1` | Tab 顺序（-1 = 不参与） |
| `m_showFocusRing` | `bool` | `true` | 是否绘制焦点环 |
| `m_focusRingAlwaysVisible` | `bool` | `true` | 始终显示焦点环 |
| `m_focusRingColor` | `SColor` | (66,133,244) | 环颜色 |
| `m_focusRingStyle` | `FocusRingStyle` | Solid | 实线/虚线 |
| `m_isFocusBoundary` | `bool` | `false` | 焦点作用域边界 |

**焦点环绘制**（`drawFocusRing()`，在 `afterDraw()` 中被调用）：
- 3 层环：黑(inset 0, alpha 150) + 白(inset 1, alpha 150) + 用户颜色(inset 2) — 保证在任何背景色下可见
- Solid 模式：3 层 `drawRect()` 叠加
- Dashed 模式：3 次循环绘制虚线

**构造函数调用 `setFocusable(true)`**：Button、CheckBox、EditBox、Slider 在构造函数中调用 `setFocusable(true)` 而非直接赋值 `m_focusable = true`，确保 `FocusManager::registerControl()` 执行，控件可参与 Tab 导航。

**析构注销**：`~ControlImpl()` 非默认析构，自动调用 `FocusManager::unregisterControl(this)`。

**setFocused 实现**：

```cpp
void ControlImpl::setFocused(bool focused, bool byKeyboard) {
    if (m_focused == focused) return;
    m_focused = focused;
    m_focusByKeyboard = byKeyboard && focused;
    if (focused) onFocusGained(byKeyboard);
    else onFocusLost();
    FocusManager* fm = MAINWIN ? MAINWIN->getFocusManager() : nullptr;
    if (fm) fm->notifyControlFocused(this, byKeyboard);
}
```

`notifyControlFocused` 仅更新 `FocusManager::m_currentFocused`，**不会**再次调用 `setFocused`（避免递归）。

## 5. 事件处理

### 5.1 鼠标进入/退出

```cpp
void ControlImpl::onMouseEnter(float x, float y) {
    if (!getEnable() || !getVisible()) return;
    if (!m_mouseInside) {
        m_mouseInside = true;
        setState(ControlState::Hover);
    }
}

void ControlImpl::onMouseLeave(float x, float y) {
    if (m_mouseInside) {
        m_mouseInside = false;
        setState(ControlState::Normal);
    }
}
```

### 5.2 点命中测试

```cpp
bool ControlImpl::isContainsPoint(float x, float y){
    SRect drawRect = getDrawRect();
    return drawRect.contains(x, y);
}
```

## 6. 常量定义

在 `ConstDef.h` 中无新增常量。

## 7. 派生类实现要求

派生类需要实现以下纯虚函数：

```cpp
class DerivedControl : public ControlImpl {
protected:
    void recreate(void) override {
        if(!m_isCreated) return;

        // 释放资源
        releaseResources();

        m_isCreated = false;
        if (typeid(*this) == typeid(DerivedControl)) {
            create();
        }
    }

public:
    void create(void) override {
        if (m_isCreated) return;

        // 创建资源
        createResources();

        ControlImpl::create();
    }

    void preDraw() override {
        ControlImpl::preDraw();
    }

    void draw(void) override {
        // 绘制内容（不含背景边框，由 preDraw 处理）
    }

    void setRect(SRect rect) override {
        ControlImpl::setRect(rect);
        recreate();
    }
};
```

## 8. 文件结构

```
UICornerstone/
├── include/
│   └── ControlBase.h      # 头文件
├── src/
│   └── ControlBase.cpp     # 实现文件
└── CMakeLists.txt

test/
└── (各控件测试文件)

doc/
└── ControlBase_Design.md  # 本文档
```