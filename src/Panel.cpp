#include "Panel.h"
#include "PropertyNames.h"
#include "UIContext.h"

Panel::Panel(Control *parent, SRect rect, float xScale, float yScale):
    ControlImpl(parent, xScale, yScale)
{
    m_ctlType = ControlType::Panel;
    m_rect = rect;
}

void Panel::update(void){
    if (!getEnable()) return;

    ControlImpl::update();
}
void Panel::draw(void){
    if (!getVisible()) return;

    ControlImpl::beforeDraw();
    if (m_clipChildren) {
        // 子项裁剪：内容 rect = 控件绘制区（含缩放/锚点，与 EditBox 同口径），
        // 子控件递归绘制在其内，超出即裁（clipStack 支持嵌套 Panel）
        getRenderDevice()->pushClipRect(getDrawRect());
        ControlImpl::draw();
        getRenderDevice()->popClipRect();
    } else {
        ControlImpl::draw();
    }
    afterDraw();
}

int Panel::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kClipChildren) == 0) { setClipChildren(value != 0); return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}

int Panel::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kClipChildren) == 0) { out = m_clipChildren ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}

bool Panel::handleEvent(shared_ptr<Event> event){
    if (ControlImpl::handleEvent(event)) return true;   // 子控件/既有逻辑优先（消费后容器不重复回调）
    // 容器级滚轮：wheel 坐标在本面板内且子控件未消费 → 通知应用（载荷 floatVal = scrollY，+1 向上 / -1 向下）
    if (event->m_type == EventType::MouseWheel &&
        isContainsPoint(event->mouseWheel.x, event->mouseWheel.y)) {
        if (hasCallback(PropertyNames::kEventMouseWheel)) {
            float dy = event->mouseWheel.scrollY;
            fireCCallback(PropertyNames::kEventMouseWheel, CCallbackData::Float, &dy);
            return true;                            // 有订阅者：消费（最近订阅者语义）
        }
        return false;                               // 无订阅者：透传给外层 Panel/兄弟
    }
    return false;
}
void Panel::addControl(shared_ptr<Control> control){
    ControlImpl::addControl(control);
    if (m_layoutEngine) reflowChildren();   // 编程式挂入即排（v1.1.1，Bench 同语义）
}

void Panel::removeControl(shared_ptr<Control> child){
    ControlImpl::removeControl(child);
    if (m_layoutEngine) reflowChildren();   // 移除后重排余下项（无布局残留）
}

void Panel::removeAllControls() {
    m_flowItemProps.clear();
    m_anchorItemProps.clear();
    m_gridItemProps.clear();
    m_children.clear();
}

void Panel::reflowChildren() {
    if (!m_layoutEngine) return;
    string type = m_layoutEngine->getType();
    if (type == PropertyNames::kLayoutTypeGrid) {
        m_layoutEngine->applyGrid(m_rect, m_children, m_gridItemProps);
    } else if (type == PropertyNames::kLayoutTypeAnchor) {
        m_layoutEngine->applyAnchor(m_rect, m_children, m_anchorItemProps);
    } else {
        m_layoutEngine->apply(m_rect, m_children, m_flowItemProps);
    }
}

void Panel::resolveChildPercentages() {
    for (auto& child : m_children) {
        if (!child->getVisible()) continue;
        SRect childRect = child->getRect();
        childRect.resolve(m_rect.width, m_rect.height);
        child->setRect(childRect);
    }
}

void Panel::setRect(SRect rect) {
    ControlImpl::setRect(rect);
    if (m_layoutEngine) {
        reflowChildren();
    } else {
        resolveChildPercentages();
    }
}

void Panel::resized(SRect newRect) {
    ControlImpl::resized(newRect);
    if (m_layoutEngine) {
        reflowChildren();
    } else {
        resolveChildPercentages();
    }
}

int Panel::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kChildTargetId) == 0) { m_childTargetId = value ? value : ""; return 1; }
    return ControlImpl::setStringProperty(prop, value);
}
int Panel::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kChildTargetId) == 0) { out = m_childTargetId.c_str(); return 1; }
    return ControlImpl::getStringProperty(prop, out);
}

static Control* findChildById(Panel* panel, const string& id) {
    if (id.empty()) return nullptr;
    auto* ctx = panel->getContext();
    if (!ctx) return nullptr;
    auto it = ctx->controlsById.find(id);
    if (it == ctx->controlsById.end()) return nullptr;
    return static_cast<Control*>(it->second);
}

int Panel::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kChildFlowWeight) == 0) {
        auto* child = findChildById(this, m_childTargetId);
        if (!child) return 0;
        FlowItemProps p = m_flowItemProps[child];
        p.flexWeight = value;
        setChildFlowProps(child, p);
        reflowChildren();   // P0-60：运行期修改立即重排
        return 1;
    }
    if (strcmp(prop, PropertyNames::kChildAnchorOffsetX) == 0) {
        auto* child = findChildById(this, m_childTargetId);
        if (!child) return 0;
        AnchorInfo a = m_anchorItemProps[child];
        a.offset.left = value;
        setChildAnchorProps(child, a);
        reflowChildren();   // P0-60：偏移立即应用（hasAnchor=true 的条目）
        return 1;
    }
    if (strcmp(prop, PropertyNames::kChildAnchorOffsetY) == 0) {
        auto* child = findChildById(this, m_childTargetId);
        if (!child) return 0;
        AnchorInfo a = m_anchorItemProps[child];
        a.offset.top = value;
        setChildAnchorProps(child, a);
        reflowChildren();   // P0-60：偏移立即应用（hasAnchor=true 的条目）
        return 1;
    }
    return ControlImpl::setFloatProperty(prop, value);
}
int Panel::setIntProperty(const char* prop, int value) {
    bool isGridProp = strcmp(prop, PropertyNames::kChildGridRow) == 0 ||
                      strcmp(prop, PropertyNames::kChildGridCol) == 0 ||
                      strcmp(prop, PropertyNames::kChildGridRowSpan) == 0 ||
                      strcmp(prop, PropertyNames::kChildGridColSpan) == 0;
    if (isGridProp) {
        auto* child = findChildById(this, m_childTargetId);
        if (!child) return 0;
        GridItemProps g = m_gridItemProps[child];
        if (strcmp(prop, PropertyNames::kChildGridRow) == 0) {
            g.row = value; setChildGridProps(child, g); reflowChildren(); return 1;   // P0-60
        }
        if (strcmp(prop, PropertyNames::kChildGridCol) == 0) {
            g.col = value; setChildGridProps(child, g); reflowChildren(); return 1;   // P0-60
        }
        if (strcmp(prop, PropertyNames::kChildGridRowSpan) == 0) {
            g.rowSpan = value; setChildGridProps(child, g); reflowChildren(); return 1;   // P0-60
        }
        g.colSpan = value; setChildGridProps(child, g); reflowChildren(); return 1;   // P0-60
    }
    return ControlImpl::setIntProperty(prop, value);
}
int Panel::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kChildAnchor) == 0) {
        auto* child = findChildById(this, m_childTargetId);
        if (!child || !value) return 0;
        AnchorInfo a = m_anchorItemProps[child];
        a.anchor = value;
        a.hasAnchor = true;   // P0-60：显式设锚点才激活管理
        setChildAnchorProps(child, a);
        reflowChildren();     // P0-60：立即应用
        return 1;
    }
    // P0-59：运行期布局模式切换（absolute/空串=清除引擎回到自由布局；gap/padding 默认 0）
    if (strcmp(prop, PropertyNames::kLayout) == 0) {
        if (!value || value[0] == '\0' || strcmp(value, PropertyNames::kLayoutTypeAbsolute) == 0) {
            m_layoutEngine.reset();
            return 1;
        }
        Margin pad{0, 0, 0, 0};
        shared_ptr<LayoutEngine> engine;
        if (strcmp(value, PropertyNames::kLayoutTypeVFlow) == 0)       engine = make_shared<VFlowLayout>(0.0f, pad);
        else if (strcmp(value, PropertyNames::kLayoutTypeAnchor) == 0) engine = make_shared<AnchorLayout>(pad);
        else if (strcmp(value, PropertyNames::kLayoutTypeGrid) == 0)   engine = make_shared<GridLayout>(0.0f, pad);
        else if (strcmp(value, PropertyNames::kLayoutTypeHFlow) == 0)  engine = make_shared<HFlowLayout>(0.0f, pad);
        else return 0;
        setLayoutEngine(engine);
        reflowChildren();
        return 1;
    }
    return ControlImpl::setEnumProperty(prop, value);
}

// P0-59：布局模式/锚点读回（配合 child-id）
int Panel::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kLayout) == 0) {
        if (!m_layoutEngine) { out = PropertyNames::kLayoutTypeAbsolute; return 1; }
        const std::string type = m_layoutEngine->getType();   // getType 按值返回，须映射到静态常量
        if (type == PropertyNames::kLayoutTypeHFlow)       out = PropertyNames::kLayoutTypeHFlow;
        else if (type == PropertyNames::kLayoutTypeVFlow)  out = PropertyNames::kLayoutTypeVFlow;
        else if (type == PropertyNames::kLayoutTypeAnchor) out = PropertyNames::kLayoutTypeAnchor;
        else if (type == PropertyNames::kLayoutTypeGrid)   out = PropertyNames::kLayoutTypeGrid;
        else out = PropertyNames::kLayoutTypeAbsolute;
        return 1;
    }
    if (strcmp(prop, PropertyNames::kChildAnchor) == 0) {
        auto* child = findChildById(this, m_childTargetId);
        if (!child) return 0;
        auto it = m_anchorItemProps.find(child);
        if (it == m_anchorItemProps.end() || !it->second.hasAnchor) return 0;   // 未设置/仅偏移（P0-60）
        out = it->second.anchor.c_str();
        return 1;
    }
    return ControlImpl::getEnumProperty(prop, out);
}

int Panel::getFloatProperty(const char* prop, float& out) {
    const bool isOffsetX = strcmp(prop, PropertyNames::kChildAnchorOffsetX) == 0;
    const bool isOffsetY = strcmp(prop, PropertyNames::kChildAnchorOffsetY) == 0;
    if (isOffsetX || isOffsetY) {
        auto* child = findChildById(this, m_childTargetId);
        if (!child) return 0;
        auto it = m_anchorItemProps.find(child);
        if (it == m_anchorItemProps.end()) return 0;   // 未设置
        out = isOffsetX ? it->second.offset.left : it->second.offset.top;
        return 1;
    }
    return ControlImpl::getFloatProperty(prop, out);
}

// *********************************************************************************************
PanelBuilder::PanelBuilder(Control *parent, SRect rect, float xScale, float yScale):
    m_panel(nullptr)
{
    m_panel = make_shared<Panel>(parent, rect, xScale, yScale);
}
PanelBuilder& PanelBuilder::setBGColor(SColor color){
    m_panel->setNormalStateBGColor(color);
    return *this;
}
PanelBuilder& PanelBuilder::setBorderColor(SColor color){
    m_panel->setNormalStateBDColor(color);
    return *this;
}
PanelBuilder& PanelBuilder::setTransparent(bool isTransparent){
    m_panel->setTransparent(isTransparent);
    return *this;
}
PanelBuilder& PanelBuilder::setBorderVisible(bool isBorderVisible){
    m_panel->setBorderVisible(isBorderVisible);
    return *this;
}
PanelBuilder& PanelBuilder::addControl(shared_ptr<Control> control){
    m_panel->addControl(control);
    return *this;
}
shared_ptr<Panel> PanelBuilder::build(void){
    m_panel->create();
    return m_panel;
}
