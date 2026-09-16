#include "Bench.h"
#include "PropertyNames.h"
#include "PlatformUtils.h"
#include "EventTypes.h"
#include <cstring>

void Bench::initial(void){
    // setBGColor(INITIAL_BG_COLOR);
    // setBorderColor(INITIAL_BORDER_COLOR);
    setTransparent(false);

    m_isInitialed = true;
    Platform::Log("Loading finished, waiting user starting game................................");
    if (m_onInitial != nullptr){
        m_onInitial(shared_ptr<Bench>(this, [](Bench*){}));
    }
    fireCCallback(PropertyNames::kEventInitial, CCallbackData::None, nullptr);
}

Bench::Bench(UIContext* ctx):
    TopControl(ctx),
    Panel(nullptr, {0, 0, INITIAL_WIDTH, INITIAL_HEIGHT}),
    Control(ctx),
    m_isLoading(true),
    m_isInitialed(false),
    m_nextTick(0),
    m_nextRepeatTick(0),
    m_isExiting(0),
    m_onInitial(nullptr)
{
    setTransparent(true);
    // 根作用域边界（FocusSystem_Design §4.3）：外层 Tab 循环不含任何边界容器内部
    setFocusBoundary(true);

    Platform::Log("Loading resources.....................................");
    m_isLoading = false;
    m_isFocusBoundary = true;
    initial();
}

void Bench::inputControl(shared_ptr<Event> event) {
    triggerEvent(event);
}

void Bench::repeatTrigger(void){
    if (m_lastAction != nullptr){
        uint64_t currentTick = Platform::GetTicks();
        if (currentTick < m_nextRepeatTick || currentTick < m_eventJitter[m_lastAction->m_type]){
            return;
        }

        switch(m_lastAction->m_type){
            case EventType::FingerDown:
            case EventType::FingerMotion:
            case EventType::MouseDown:
                triggerEvent(m_lastAction);
                break;
            default:
                break;
        }
        m_nextRepeatTick = Platform::GetTicks() + DEFAULT_BTN_MS_REPEAT;
    }
}
void Bench::update() {
    if (m_isLoading){
        m_isLoading = false;
        initial();
    }else {
        if (m_lastAction != nullptr){
            repeatTrigger();
        }
        Panel::update();
    }
}
void Bench::draw(void){
    if (!m_visible) return;

    Panel::draw();
}

bool Bench::handleEvent(shared_ptr<Event> event) {
    // Intercept Tab / Ctrl+Tab before passing to children
    if (event->m_type == EventType::KeyDown &&
        event->keyEvent.keycode == KeyCode::Tab) {
        bool ctrl = isModSet(event->keyEvent.mod, KeyMod::LCtrl) || isModSet(event->keyEvent.mod, KeyMod::RCtrl);
        bool shift = isModSet(event->keyEvent.mod, KeyMod::Shift);

        if (ctrl) {
            if (shift)
                GET_FOCUSMANAGER->focusPrevScope();
            else
                GET_FOCUSMANAGER->focusNextScope();
        } else {
            Control* current = GET_FOCUSMANAGER->getCurrentFocused();
            if (shift)
                GET_FOCUSMANAGER->focusPrev(current);
            else
                GET_FOCUSMANAGER->focusNext(current);
            return true;
        }
        return true;
    }
    return Panel::handleEvent(event);
}

int Bench::isExiting(void) {
    return m_isExiting;
}

void Bench::setOnInitial(OnInitialHandler handler) {
    if (m_isInitialed) {
        handler(shared_ptr<Bench>(this, [](Bench*){}));
    } else {
        m_onInitial = handler;
    }
}

// 三层模型中 Bench 恒为画布顶层（布局空间），视口（屏幕可见区域）仅由
// m_context->viewport 持有；此函数是视口变化的唯一派发入口：
//   off      —— 画布跟随窗口：Panel::resized 仅更新宽高（不碰 left/top，
//              视口偏移由 anchor 携带），随后 recompute 把 anchor 指向视口原点
//   fit/stretch —— 画布尺寸不变，仅重算根变换（scale + anchor 携带视口偏移）
void Bench::resized(SRect newRect) {
    if (m_vpMode == ViewportScaleMode::Off) {
        Panel::resized(newRect);
        recomputeViewportTransform();
    } else {
        recomputeViewportTransform();
    }
}

// 根变换重算：统一公式 rootDR = { m_rect.left+anchorX, m_rect.top+anchorY,
// m_rect.width*SXX, m_rect.height*SYY }，SXY = 当前复合值（setScaleX/Y 写入）
// 每个分支必须先归位 rect 再算 anchor，保证任何状态切换自洽：
//   off      —— 画布 = 视口（rect 含 left/top 偏移，anchor=0）
//   fit/stretch —— 画布原点 (0,0)，视口偏移全部由 anchor 携带
// （否则 off→fit 切换时 rect 残留 vp 偏移 + anchor 再叠加 vp 偏移 → 双算，
//   非零视口偏移的子视口内容被推出视口裁切）
void Bench::recomputeViewportTransform(void) {
    SRect vp;
    if (m_context != nullptr) {
        vp = m_context->viewport;
    } else {
        vp = {0, 0, m_rect.width, m_rect.height};
    }
    // 显式基准画布（SetCanvasSize / JSON viewport 键）优先，缺省画布 = bench rect
    float canvasW = m_rect.width;
    float canvasH = m_rect.height;
    if (m_context != nullptr && m_context->canvasWidth > 0.0f && m_context->canvasHeight > 0.0f) {
        canvasW = m_context->canvasWidth;
        canvasH = m_context->canvasHeight;
    }
    if (m_vpMode == ViewportScaleMode::Fit) {
        // 等比缩放整体容纳，剩余空间四周居中
        float sx = vp.width / canvasW;
        float sy = vp.height / canvasH;
        float f = (sx < sy) ? sx : sy;
        m_scaleOverride = false;   // 引擎自动缩放接管（清除手动标志）
        setScale(f, f);
        setRect(SRect(0, 0, canvasW, canvasH));
        m_anchorX = vp.left + (vp.width - canvasW * f) / 2.0f;
        m_anchorY = vp.top + (vp.height - canvasH * f) / 2.0f;
    } else if (m_vpMode == ViewportScaleMode::Stretch) {
        // 拉伸填满视口，锚点贴视口原点
        m_scaleOverride = false;   // 引擎自动缩放接管
        setScale(vp.width / canvasW, vp.height / canvasH);
        setRect(SRect(0, 0, canvasW, canvasH));
        m_anchorX = vp.left;
        m_anchorY = vp.top;
    } else {
        // off：无自动缩放（默认 sx=sy=1）；手动缩放（SetInstanceScale）时尊重
        // override 标志，不再强制重置——anchor 指向视口原点
        if (!m_scaleOverride) setScale(1.0f, 1.0f);
        setRect(SRect(0, 0, canvasW, canvasH));
        m_anchorX = vp.left;
        m_anchorY = vp.top;
    }
}

void Bench::setViewportScaleMode(ViewportScaleMode mode) {
    if (m_vpMode == mode) return;
    m_vpMode = mode;
    recomputeViewportTransform();
}

// 属性系统运行期切换（§ViewportScale_Design）：SetEnum("viewport-scale-mode",
// "off"|"fit"|"stretch") → 重算根变换，子树字号/布局随复合缩放自动重建
int Bench::setEnumProperty(const char* prop, const char* value) {
    if (prop && strcmp(prop, PropertyNames::kViewportScaleMode) == 0) {
        if (!value) return 0;
        if (strcmp(value, PropertyNames::kViewportScaleOff) == 0) {
            setViewportScaleMode(ViewportScaleMode::Off);
        } else if (strcmp(value, PropertyNames::kViewportScaleFit) == 0) {
            setViewportScaleMode(ViewportScaleMode::Fit);
        } else if (strcmp(value, PropertyNames::kViewportScaleStretch) == 0) {
            setViewportScaleMode(ViewportScaleMode::Stretch);
        } else {
            return 0;
        }
        return 1;
    }
    return Panel::setEnumProperty(prop, value);
}

void Bench::setViewportAnchor(float ax, float ay) {
    m_anchorX += ax;
    m_anchorY += ay;
}

// 根：布局缩放 = 复合缩放（无父级），且必须保持 m_rect 不变（画布语义）
void Bench::setScaleX(float xScale) {
    setScale(xScale, m_yScale);
}

void Bench::setScaleY(float yScale) {
    setScale(m_xScale, yScale);
}

void Bench::setScale(float xScale, float yScale) {
    // 幂等：与当前值均相等时快速返回（不触发整树 refresh）
    if (xScale == m_xScale && yScale == m_yScale) return;
    m_xScale = xScale;
    m_yScale = yScale;
    m_xxScale = xScale;
    m_yyScale = yScale;
    for (auto& child : m_children) {
        child->refreshScaleWith(m_xxScale, m_yyScale);
    }
}

void Bench::setManualScale(float xScale, float yScale) {
    m_scaleOverride = true;
    setScale(xScale, yScale);
}

SRect Bench::getDrawRect(void) {
    return {m_rect.left + m_anchorX, m_rect.top + m_anchorY,
            m_rect.width * getScaleXX(), m_rect.height * getScaleYY()};
}

void Bench::addControl(shared_ptr<Control> control) {
    Panel::addControl(control);
    resolveChildPercentages();
    auto panel = dynamic_pointer_cast<Panel>(control);
    if (panel) {
        panel->resolveChildPercentages();
        if (panel->getLayoutEngine()) {
            panel->reflowChildren();
        }
    }
}
