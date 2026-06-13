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

    virtual void setRenderer(SDL_Renderer *renderer) = 0;
    virtual SDL_Renderer* getRenderer(void) = 0;

    virtual void setState(ControlState state) = 0;
    virtual ControlState getState(void) = 0;

    virtual shared_ptr<Control> getThis(void) = 0;
    virtual SRect getDrawRect(void) = 0;
    virtual SRect mapToDrawRect(SRect rect) = 0;
    virtual SPoint mapToDrawPoint(SPoint point) = 0;
    virtual bool isContainsPoint(float x, float y) = 0;

    virtual void onMouseEnter(float x, float y) = 0;
    virtual void onMouseLeave(float x, float y) = 0;
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

    SDL_Renderer *m_renderer;

    SRect m_rect;
    Margin m_margin;
    Control *m_parent;
    vector<shared_ptr<Control>> m_children;

    bool m_isTransparent;
    bool m_isBorderVisible;

    ControlState m_state;
    bool m_mouseInside;

    void recreate(void) override;
public:
    ControlImpl(Control *parent, float xScale=1.0f, float yScale=1.0f);
    ControlImpl(const ControlImpl& other);
    ~ControlImpl() = default;

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

    void setRenderer(SDL_Renderer *renderer) override;
    SDL_Renderer* getRenderer(void) override;

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
    child->setRenderer(getRenderer());
    // 不再手动设置子控件缩放，而是通过 setParent 触发
}

void ControlImpl::removeControl(shared_ptr<Control> child){
    // 从子控件列表中移除
}
```

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