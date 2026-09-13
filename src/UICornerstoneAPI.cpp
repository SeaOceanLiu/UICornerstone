#include "UICornerstoneAPI.h"
#include "UIContext.h"
#include "SColor.h"
#include "CallbackAdapters.h"
#include "BackendPlugin.h"
#include "Bench.h"
#include "Button.h"
#include "Label.h"
#include "CheckBox.h"
#include "EditBox.h"
#include "ProgressBar.h"
#include "Panel.h"
#include "TextArea.h"
#include "Slider.h"
#include "ColorPicker.h"
#include "ComboBox.h"
#include "NumericUpDown.h"
#include "Splitter.h"
#include "Dialog.h"
#include "WinFrame.h"
#include "TreeView.h"
#include "ScrollBar.h"
#include "HandleControl.h"
#include "Menu.h"
#include "LayoutParser.h"
#include "ResourceProvider.h"
#include "PlatformUtils.h"
#include "Actor.h"
#include "LuotiAni.h"
#include "Shape.h"
#include "ListView.h"
#include "StatusBar.h"
#include "ContextMenu.h"
#include "TabControl.h"
#include "EventTypes.h"
#include "PropertyNames.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <functional>
#include <queue>
#include <vector>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <cassert>

// ============================================================
// 调试：实例注册表（仅 _DEBUG 构建；Release 零开销）
// ============================================================
#ifdef _DEBUG
static std::mutex s_registryMutex;
static std::vector<UIInstance> s_aliveInstances;

static void registerInstance(UIInstance instance) {
    std::lock_guard<std::mutex> lock(s_registryMutex);
    s_aliveInstances.push_back(instance);
}

static void unregisterInstance(UIInstance instance) {
    std::lock_guard<std::mutex> lock(s_registryMutex);
    s_aliveInstances.erase(
        std::remove(s_aliveInstances.begin(), s_aliveInstances.end(), instance),
        s_aliveInstances.end());
}

struct LeakDetector {
    ~LeakDetector() {
        if (!s_aliveInstances.empty()) {
            printf("LEAK: %zu UICornerstone instance(s) not destroyed!\n", s_aliveInstances.size());
            for (auto* inst : s_aliveInstances) {
                printf("  - %s (ID=%u)\n", inst->debugLabel.c_str(), inst->instanceId);
            }
            __debugbreak();
        }
    }
};
static LeakDetector s_leakCheck;
#else
static inline void registerInstance(UIInstance) {}
static inline void unregisterInstance(UIInstance) {}
#endif

// ============================================================
// 后端配置：全局默认值（inst == NULL 的 Set/Get 落点）
// ============================================================
static std::mutex s_backendConfigMutex;
static std::unordered_map<std::string, std::string> s_backendDefaults;

// 把全局默认配置应用到已创建 renderer 的实例（创建期参数以字符串下发，
// 由后端自行解析；不支持的后端/键自然返回 0，忽略即可）。
static int applyBackendDefaults(UIInstance inst) {
    if (!inst || !inst->renderDevice) return 0;
    std::unordered_map<std::string, std::string> snapshot;
    {
        std::lock_guard<std::mutex> lock(s_backendConfigMutex);
        snapshot = s_backendDefaults;
    }
    int ok = 0;
    for (const auto& kv : snapshot) {
        if (inst->renderDevice->setConfig(kv.first.c_str(), 0, kv.second.c_str()) > 0) {
            ok = 1;
        }
    }
    return ok;
}

// ============================================================
// 句柄归属校验（仅 _DEBUG；Release 直接透传）
// 跨实例句柄误用是调用方 bug（§7 风险 5）：Debug 下 O(n) 遍历
// 控件树 + popupPool/menuPool 确认句柄属于本实例，否则断言失败
// ============================================================
#ifdef _DEBUG
static bool treeContains(Control* root, Control* target) {
    auto* impl = dynamic_cast<ControlImpl*>(root);
    if (!impl) return false;
    for (auto& child : impl->getChildren()) {
        Control* c = child.get();
        if (c == target) return true;
        if (treeContains(c, target)) return true;
    }
    return false;
}

static bool instanceHoldsControl(UIInstance instance, Control* target) {
    // 沿 parent 链上溯：菜单面板/项经 setParent 挂靠（不在 children 列表），
    // 从 target 逐层上溯到 bench 根或池成员即视为本实例持有
    for (Control* cur = target; cur != nullptr; cur = cur->getParent()) {
        if (instance->bench && (cur == instance->bench || treeContains(instance->bench, cur)))
            return true;
        for (auto& sp : instance->popupPool)
            if (sp.get() == cur || treeContains(sp.get(), cur)) return true;
        for (auto& sp : instance->menuPool)
            if (sp.get() == cur) return true;
    }
    return false;
}

static Control* validateControl(UIInstance instance, UIControlHandle ctl) {
    if (!instance || !ctl) return nullptr;
    Control* target = static_cast<Control*>(ctl);
    if (instanceHoldsControl(instance, target)) return target;
    UI_LOGE(instance, "control handle %p NOT owned by instance", ctl);
    assert(false && "UICornerstone: control handle not owned by this instance");
    return nullptr;
}
#else
static inline Control* validateControl(UIInstance, UIControlHandle ctl) {
    return static_cast<Control*>(ctl);
}
#endif

// ============================================================
// 实例内辅助（menuPool / controlsById）
// ============================================================
static void menuPoolKeep(UIInstance instance, std::shared_ptr<Control> ctl) {
    if (instance) instance->menuPool.push_back(std::move(ctl));
}

static std::shared_ptr<Control> menuPoolTake(UIInstance instance, UIControlHandle ctl) {
    if (!instance || !ctl) return nullptr;
    auto& pool = instance->menuPool;
    for (auto it = pool.begin(); it != pool.end(); ++it) {
        if (static_cast<Control*>(ctl) == it->get()) {
            auto sp = *it;
            pool.erase(it);
            return sp;
        }
    }
    return nullptr;
}

static void registerControlById(UIInstance instance, const std::string& id, UIControlHandle ctl) {
    if (instance && !id.empty()) instance->controlsById[id] = ctl;
}

// ============================================================
// 事件转换（UIEvent → C++ Event）
// ============================================================
static bool uiEventToEvent(const UIEvent& ue, Event& event) {
    event = Event();
    event.customInt = 0;
    event.customPtr = nullptr;

    switch (ue.type) {
    case UI_EVENT_MOUSE_MOVE:
        event.m_type = EventType::MouseMove;
        event.mousePos = EventMousePos{UI_EVENT_MOUSE_X(&ue), UI_EVENT_MOUSE_Y(&ue)};
        return true;
    case UI_EVENT_MOUSE_DOWN:
        event.m_type = EventType::MouseDown;
        event.mouseButton = EventMouseButton{UI_EVENT_MOUSE_X(&ue), UI_EVENT_MOUSE_Y(&ue),
            static_cast<MouseButton>(UI_EVENT_BUTTON(&ue))};
        return true;
    case UI_EVENT_MOUSE_UP:
        event.m_type = EventType::MouseUp;
        event.mouseButton = EventMouseButton{UI_EVENT_MOUSE_X(&ue), UI_EVENT_MOUSE_Y(&ue),
            static_cast<MouseButton>(UI_EVENT_BUTTON(&ue))};
        return true;
    case UI_EVENT_MOUSE_WHEEL:
        event.m_type = EventType::MouseWheel;
        event.mouseWheel = EventMouseWheel{
            UI_EVENT_WHEEL_MOUSE_X(&ue),
            UI_EVENT_WHEEL_MOUSE_Y(&ue),
            0,
            UI_EVENT_WHEEL_DELTA(&ue)
        };
        return true;
    case UI_EVENT_KEY_DOWN:
        event.m_type = EventType::KeyDown;
        event.keyEvent = EventKey{static_cast<KeyCode>(UI_EVENT_KEY_CODE(&ue)),
            static_cast<KeyMod>(UI_EVENT_KEY_MOD(&ue)), 0, false};
        return true;
    case UI_EVENT_KEY_UP:
        event.m_type = EventType::KeyUp;
        event.keyEvent = EventKey{static_cast<KeyCode>(UI_EVENT_KEY_CODE(&ue)),
            static_cast<KeyMod>(UI_EVENT_KEY_MOD(&ue)), 0, false};
        return true;
    case UI_EVENT_TEXT_INPUT:
        event.m_type = EventType::TextInput;
        strncpy(event.textInput.text, UI_EVENT_TEXT(&ue), 31);
        event.textInput.text[31] = '\0';
        return true;
    case UI_EVENT_WINDOW_RESIZE: {
        int w = UI_EVENT_RESIZE_W(&ue), h = UI_EVENT_RESIZE_H(&ue);
        event.m_type = EventType::WindowResize;
        event.resizeEvent = EventResize{w, h};
        return true;
    }
    case UI_EVENT_WINDOW_CLOSE:
        event.m_type = EventType::WindowClose;
        return true;
    case UI_EVENT_FOCUS_GAINED:
        event.m_type = EventType::FocusGained;
        event.focusEvent = EventFocus{true};
        return true;
    case UI_EVENT_FOCUS_LOST:
        event.m_type = EventType::FocusLost;
        event.focusEvent = EventFocus{false};
        return true;
    default:
        return false;
    }
}

// ============================================================
// 多视口路由辅助
// ============================================================
static UIInstance findViewportByCoord(UIInstance owner, float x, float y) {
    for (auto* child : owner->children) {
        auto& r = child->viewport;
        if (x >= r.left && x < r.left + r.width
         && y >= r.top && y < r.top + r.height) {
            return child;
        }
    }
    return nullptr;
}

static UIInstance nextViewport(UIInstance owner, UIInstance cur) {
    auto& cs = owner->children;
    if (cs.empty()) return nullptr;
    if (!cur) return cs.front();
    auto it = std::find(cs.begin(), cs.end(), cur);
    return (it != cs.end() && ++it != cs.end()) ? *it : cs.front();
}

static UIInstance prevViewport(UIInstance owner, UIInstance cur) {
    auto& cs = owner->children;
    if (cs.empty()) return nullptr;
    if (!cur) return cs.back();
    auto it = std::find(cs.begin(), cs.end(), cur);
    return (it != cs.begin()) ? *std::prev(it) : cs.back();
}

// Ctrl+Tab 智能路由：跨视口切换。返回 true 表示事件已消费。
static bool tryViewportScopeSwitch(UIInstance owner, Event& keyEvent) {
    KeyCode code = keyEvent.keyEvent.keycode;
    KeyMod  mod  = keyEvent.keyEvent.mod;
    if (code != KeyCode::Tab || !isModSet(mod, KeyMod::Ctrl)) return false;
    if (owner->children.size() <= 1) return false;  // 单视口：交给视口内处理

    UIInstance cur = owner->activeViewport;
    if (cur && cur->focusManager->getVisibleBoundaryCount() >= 1) return false;

    if (cur) cur->focusManager->clearFocus();
    bool shift = isModSet(mod, KeyMod::Shift);
    owner->activeViewport = shift ? prevViewport(owner, cur) : nextViewport(owner, cur);
    if (!owner->activeViewport) return false;
    owner->activeViewport->focusManager->focusFirstInScope(owner->activeViewport->bench);
    return true;
}

// 将事件分发到指定实例的 bench（供 ProcessEvents 两条通路共用）
static void dispatchToBench(UIInstance instance, const Event& event) {
    if (instance && instance->bench) {
        instance->bench->inputControl(std::make_shared<Event>(event));
    }
}

// ============================================================
// UICornerstone_CreateInstance / DestroyInstance / CreateViewport
// ============================================================
UIInstance UICornerstone_CreateInstance(
    const UIBackendCallbacks* callbacks,
    const UIInstanceConfig* config)
{
    if (!callbacks || callbacks->version != 1) return nullptr;

    auto* ctx = new UIContext();
    ctx->callbacks = callbacks;

    if (config) {
        if (config->debugLabel) ctx->debugLabel = config->debugLabel;
        if (config->resourceRoot) ctx->resourceRoot = config->resourceRoot;
        if (config->windowTitle) ctx->windowTitle = config->windowTitle;
        ctx->windowWidth = config->windowWidth;
        ctx->windowHeight = config->windowHeight;
        // windowFlags 为新增字段：旧客户端 structSize 更小，按大小守卫读取
        if (config->structSize >= offsetof(UIInstanceConfig, windowFlags) + sizeof(config->windowFlags)) {
            ctx->windowFlags = config->windowFlags;
        }
    }
    // 全局默认后端配置（创建前设置，key=vsync）合并进窗口标志：
    // raylib 的 vsync 属创建期参数，须在 InitWindow 前以 FLAG_VSYNC_HINT 生效
    int gVsync = 0;
    if (UICornerstone_GetBackendConfigInt(nullptr, PropertyNames::kBackendKeyVsync, &gVsync) && gVsync)
        ctx->windowFlags |= UIWindowFlags::Vsync;

    if (!ctx->initialize()) {
        delete ctx;
        return nullptr;
    }

    if (ctx->window) {
        SSize sz = ctx->window->getSize();
        ctx->viewport = SRect(0, 0, sz.width, sz.height);
        ctx->bench->resized(ctx->viewport);
    }

    // Perform a dummy clear+present to ensure the OpenGL context is active
    // for subsequent texture creation (relevant for OpenGL-based backends).
    if (ctx->renderDevice) {
        ctx->renderDevice->setDrawColor(SColor(0, 0, 0, 0));
        ctx->renderDevice->clear();
        ctx->renderDevice->present();
    }

    registerInstance(ctx);
    // 应用全局后端默认配置（inst == NULL 的 SetBackendConfig* 落点）
    applyBackendDefaults(ctx);
    // 视口缩放初始配置（canvas + mode）：viewport 已就绪，模式应用即重算根变换
    if (config) {
        if (config->structSize >= offsetof(UIInstanceConfig, viewportScaleMode) + sizeof(config->viewportScaleMode)) {
            if (config->canvasWidth > 0.0f && config->canvasHeight > 0.0f) {
                UICornerstone_SetCanvasSize(ctx, config->canvasWidth, config->canvasHeight);
            }
            if (config->viewportScaleMode > 0) {
                ctx->bench->setViewportScaleMode(
                    static_cast<Bench::ViewportScaleMode>(config->viewportScaleMode));
            }
        }
    }
    UI_LOGI(ctx, "created");
    return ctx;
}

UIInstance UICornerstone_CreateViewport(UIInstance parent, UIRect rect) {
    if (!parent || !parent->initialized || parent->destroying) return nullptr;
    if (!parent->ownsBackend) return nullptr;  // 只能从 owner 创建

    auto* vp = new UIContext();
    vp->owner = parent;
    vp->ownsBackend = false;
    vp->callbacks = parent->callbacks;
    vp->viewport = SRect(rect.x, rect.y, rect.w, rect.h);

    if (!vp->initialize()) {
        delete vp;
        return nullptr;
    }
    // initialize() 内兜底 `viewport = owner->viewport` 会覆盖上文写入的区域，
    // 此处须在 initialize 之后重写视口区域（§5.13.4）
    vp->viewport = SRect(rect.x, rect.y, rect.w, rect.h);
    // 三层模型：视口区（含 left/top 偏移的屏幕可见区域）写入 viewport 数据层，
    // bench 恒为画布——off 默认模式下 resized 会将画布置为视口（含偏移）
    vp->bench->resized(vp->viewport);

    parent->children.push_back(vp);
    // 首个子视口自动设为活动视口（键盘事件投递目标）
    if (!parent->activeViewport) parent->activeViewport = vp;

    registerInstance(vp);
    UI_LOGI(vp, "created (viewport %g,%g %gx%g)",
        vp->viewport.left, vp->viewport.top, vp->viewport.width, vp->viewport.height);
    return vp;
}

void UICornerstone_DestroyInstance(UIInstance instance) {
    if (!instance || instance->destroying) return;
    instance->destroying = true;  // 置位：回调重入的 C ABI 入口直接短路
    unregisterInstance(instance);

    // 快照遍历：子视口销毁时会从 owner->children 摘除自身
    auto snapshot = instance->children;
    for (auto* child : snapshot) {
        if (instance->activeViewport == child) {
            instance->activeViewport = nullptr;
        }
        UICornerstone_DestroyInstance(child);
    }
    instance->children.clear();

    instance->destroy();

    // 从 owner 摘除 + 清 activeViewport 引用（直接销毁子视口路径）
    if (instance->owner) {
        if (instance->owner->activeViewport == instance) {
            instance->owner->activeViewport = nullptr;
        }
        auto& cs = instance->owner->children;
        cs.erase(std::remove(cs.begin(), cs.end(), instance), cs.end());
    }

    UI_LOGI(instance, "destroyed");
    delete instance;
}

#if !UICORNERSTONE_BUILD_SHARED
extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);
#endif

UIInstance UICornerstone_CreateInstanceFromPlugin(
    const char* pluginName,
    const UIInstanceConfig* config)
{
    if (!pluginName || !pluginName[0]) return nullptr;

    char dllName[128];
    snprintf(dllName, sizeof(dllName), "UIBackend_%s.dll", pluginName);
    HMODULE dll = LoadLibraryA(dllName);
    if (!dll) {
#if !UICORNERSTONE_BUILD_SHARED
        printf("UICornerstone: CreateInstanceFromPlugin(%s) — LoadLibrary failed, trying static...\n", pluginName);
        UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
        if (callbacks) {
            printf("UICornerstone: static GetUIBackendCallbacks ready\n");
            return UICornerstone_CreateInstance(callbacks, config);
        }
#endif
        printf("UICornerstone: CreateInstanceFromPlugin(%s) — LoadLibrary failed\n", pluginName);
        return nullptr;
    }

    auto getter = (UIBackendCallbacks*(*)())GetProcAddress(dll, "GetUIBackendCallbacks");
    if (!getter) {
        printf("UICornerstone: %s has no GetUIBackendCallbacks\n", dllName);
        FreeLibrary(dll);
        return nullptr;
    }

    UIBackendCallbacks* callbacks = getter();
    if (!callbacks) {
        printf("UICornerstone: %s GetUIBackendCallbacks returned null\n", dllName);
        FreeLibrary(dll);
        return nullptr;
    }

    printf("UICornerstone: loaded %s\n", dllName);
    return UICornerstone_CreateInstance(callbacks, config);
}

// ============================================================
// 后端配置（inst == NULL → 全局默认；否则运行期当前实例）
// type 约定（与 RenderDevice::setConfig/getConfig 一致）：
//   0 = string, 1 = int, 2 = bool
// ============================================================
int UICornerstone_SetBackendConfig(UIInstance inst, const char* key, const char* value) {
    if (!key || !value) return 0;
    if (!inst) {
        std::lock_guard<std::mutex> lock(s_backendConfigMutex);
        s_backendDefaults[key] = value;
        return 1;
    }
    if (!inst->initialized || inst->destroying || !inst->renderDevice) return 0;
    return inst->renderDevice->setConfig(key, 0, value);
}

int UICornerstone_SetBackendConfigInt(UIInstance inst, const char* key, int value) {
    if (!key) return 0;
    if (!inst) {
        std::lock_guard<std::mutex> lock(s_backendConfigMutex);
        s_backendDefaults[key] = std::to_string(value);
        return 1;
    }
    if (!inst->initialized || inst->destroying || !inst->renderDevice) return 0;
    return inst->renderDevice->setConfig(key, 1, &value);
}

int UICornerstone_SetBackendConfigBool(UIInstance inst, const char* key, int value) {
    return UICornerstone_SetBackendConfigInt(inst, key, value ? 1 : 0);
}

int UICornerstone_GetBackendConfig(UIInstance inst, const char* key, char* value, int maxLen) {
    if (!key || !value || maxLen <= 0) return 0;
    value[0] = '\0';
    if (!inst) {
        std::lock_guard<std::mutex> lock(s_backendConfigMutex);
        auto it = s_backendDefaults.find(key);
        if (it == s_backendDefaults.end()) return 0;
        strncpy(value, it->second.c_str(), static_cast<size_t>(maxLen) - 1);
        value[maxLen - 1] = '\0';
        return 1;
    }
    if (!inst->renderDevice) return 0;
    return inst->renderDevice->getConfig(key, 0, value, maxLen);
}

int UICornerstone_GetBackendConfigInt(UIInstance inst, const char* key, int* value) {
    if (!key || !value) return 0;
    *value = 0;
    if (!inst) {
        std::lock_guard<std::mutex> lock(s_backendConfigMutex);
        auto it = s_backendDefaults.find(key);
        if (it == s_backendDefaults.end()) return 0;
        *value = std::atoi(it->second.c_str());
        return 1;
    }
    if (!inst->renderDevice) return 0;
    return inst->renderDevice->getConfig(key, 1, value, sizeof(int));
}

int UICornerstone_GetBackendConfigBool(UIInstance inst, const char* key, int* value) {
    return UICornerstone_GetBackendConfigInt(inst, key, value);
}

// ============================================================
// 视口控制
// ============================================================
void UICornerstone_SetViewport(UIInstance instance, float x, float y, float w, float h) {
    if (!instance || instance->destroying) return;
    // 三层模型：viewport 恒为屏幕可见区域（数据层），bench 恒为画布顶层
    // （布局空间）——统一经 bench->resized 派发：off 画布=视口（含偏移），
    // fit/stretch 画布尺寸不变，仅重算根变换（anchor 携带视口偏移）
    instance->viewport = SRect(x, y, w, h);
    if (instance->bench) {
        instance->bench->resized(instance->viewport);
    }
}

void UICornerstone_GetViewport(UIInstance instance, float* x, float* y, float* w, float* h) {
    if (!instance) return;
    if (x) *x = instance->viewport.left;
    if (y) *y = instance->viewport.top;
    if (w) *w = instance->viewport.width;
    if (h) *h = instance->viewport.height;
}

int UICornerstone_SetViewportBackgroundColor(UIInstance instance, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!instance || instance->destroying) return 0;
    instance->viewportBackground = SColor(r, g, b, a);
    return 1;
}

// ============================================================
// 视口缩放（ViewportScale）
// ============================================================
int UICornerstone_SetViewportScaleMode(UIInstance instance, int mode) {
    if (!instance || instance->destroying) return 0;
    if (mode < 0 || mode > 2) return 0;
    if (!instance->bench) return 0;
    instance->bench->setViewportScaleMode(static_cast<Bench::ViewportScaleMode>(mode));
    return 1;
}

int UICornerstone_GetViewportScaleMode(UIInstance instance, int* mode) {
    if (!instance || !mode || !instance->bench) return 0;
    *mode = static_cast<int>(instance->bench->getViewportScaleMode());
    return 1;
}

int UICornerstone_SetCanvasSize(UIInstance instance, float w, float h) {
    if (!instance || instance->destroying || !instance->bench) return 0;
    if (w <= 0.0f || h <= 0.0f) return 0;
    instance->canvasWidth = w;
    instance->canvasHeight = h;
    // off 下画布恒跟随窗口（原语义），仅记录显式画布声明；
    // fit/stretch 下画布 = 显式声明，置 rect 并重算根变换。
    // 注意：布局空间（bench rect）= 画布原点 (0,0)——left/top 归零是
    // 有意的分层设计：视口偏移（子视口嵌入、窗口 resize）由
    // recomputeViewportTransform 的 anchorX/Y 携带（含 vp.left/top），
    // getDrawRect 以 {m_rect.left + m_anchorX} 计算不双算。若此处保留
    // bench 原 left/top 会与 anchor 叠加造成嵌入场景双重偏移。
    if (instance->bench->getViewportScaleMode() != Bench::ViewportScaleMode::Off) {
        instance->bench->setRect(SRect(0, 0, w, h));
        instance->bench->recomputeViewportTransform();
    }
    return 1;
}

int UICornerstone_GetViewportScale(UIInstance instance, float* sx, float* sy) {
    if (!instance || !instance->bench) return 0;
    if (sx) *sx = instance->bench->getScaleXX();
    if (sy) *sy = instance->bench->getScaleYY();
    return 1;
}

int UICornerstone_SetViewportAnchor(UIInstance instance, float ax, float ay) {
    if (!instance || instance->destroying || !instance->bench) return 0;
    instance->bench->setViewportAnchor(ax, ay);
    return 1;
}

uint32_t UICornerstone_GetBackendCapabilities(UIInstance instance) {
    if (!instance || !instance->backendManager) return 0;
    return instance->backendManager->capabilities();
}

// ============================================================
// 运行期窗口 API
// ============================================================
int UICornerstone_GetWindowSize(UIInstance instance, float* w, float* h) {
    if (!instance || !instance->window) return 0;
    SSize sz = instance->window->getSize();
    if (w) *w = sz.width;
    if (h) *h = sz.height;
    return 1;
}

int UICornerstone_SetWindowSize(UIInstance instance, float w, float h) {
    if (!instance || !instance->window) return 0;
    auto* win = instance->window;
    if (win->isHeadless()) return 0;
    int iw = w > 0 ? (int)(w + 0.5f) : 0;
    int ih = h > 0 ? (int)(h + 0.5f) : 0;
    win->setSize(iw, ih);
    return 1;
}

void* UICornerstone_GetNativeWindowHandle(UIInstance instance) {
    if (!instance || !instance->window) return nullptr;
    return instance->window->nativeHandle();
}

void UICornerstone_SetWindowResizeCallback(UIInstance instance,
    UIWindowResizeCallback cb, void* userData) {
    if (!instance) return;
    instance->windowResizeCb = cb;
    instance->windowResizeCbUserData = userData;
}

// WindowResize 分发（pumpInstanceEvents 与注入队列两通路共用）
static void DispatchWindowResize(UIInstance instance, const EventResize& ev) {
    instance->viewport = SRect(0, 0, (float)ev.width, (float)ev.height);
    instance->bench->resized(instance->viewport);
    if (instance->windowResizeCb) {
        instance->windowResizeCb(ev.width, ev.height, instance->windowResizeCbUserData);
    }
}

// ============================================================
// 帧循环
// ============================================================
void UICornerstone_PushUIEvent(UIInstance instance, const UIEvent* ue) {
    if (!instance || !instance->initialized || instance->destroying) return;
    if (ue) {
        std::lock_guard<std::mutex> lock(instance->queuedEventsMutex);
        instance->queuedEvents.push(*ue);
    }
}

// 单实例事件泵：poll 属于本实例窗口的事件并分发（只消费自己的事件，
// 不消费其他窗口的事件，多实例共享全局队列时的隔离保证）。
// 返回是否处理了至少一个事件。
static bool pumpInstanceEvents(UIInstance instance) {
    if (!instance || !instance->initialized || instance->destroying) return false;
    if (!instance->ownsBackend || !instance->inputBackend) return false;

    bool handled = false;
    Event evt;
    while (instance->inputBackend->pollEvent(evt)) {
        handled = true;
        switch (evt.m_type) {
        case EventType::MouseMove:
        case EventType::MouseDown:
        case EventType::MouseUp:
        case EventType::MouseWheel: {
            float mx = (evt.m_type == EventType::MouseWheel)
                ? evt.mouseWheel.x : evt.mousePos.x;
            float my = (evt.m_type == EventType::MouseWheel)
                ? evt.mouseWheel.y : evt.mousePos.y;
            UIInstance target = findViewportByCoord(instance, mx, my);
            if (!target) {
                // 兜底：点击 owner 区域视为焦点回到 owner 树
                if (instance->activeViewport
                    && (evt.m_type == EventType::MouseDown || evt.m_type == EventType::MouseUp)) {
                    instance->activeViewport->focusManager->clearFocus();
                    instance->activeViewport = nullptr;
                }
                dispatchToBench(instance, evt);
                break;
            }
            // 跨视口焦点转移（仅按下/抬起触发）
            if (target != instance->activeViewport
                && (evt.m_type == EventType::MouseDown || evt.m_type == EventType::MouseUp)) {
                if (instance->activeViewport) {
                    instance->activeViewport->focusManager->clearFocus();
                }
                instance->activeViewport = target;
            }
            // 事件保持窗口绝对坐标分发（控件命中/绘制均基于绝对坐标
            // getDrawRect；若在此转视口本地坐标，则子视口偏移非零时
            // 命中测试与坐标换算全部错位）
            dispatchToBench(target, evt);
            break;
        }
        case EventType::KeyDown:
        case EventType::KeyUp:
            // 键盘事件：先经 Ctrl+Tab 智能路由，未消费则发到当前活动视口
            // （activeViewport 为 null 时回退 owner 自身 bench）
            if (!tryViewportScopeSwitch(instance, evt)) {
                UIInstance kbdTarget = instance->activeViewport
                    ? instance->activeViewport : instance;
                dispatchToBench(kbdTarget, evt);
            }
            break;
        case EventType::FocusLost:
            // 窗口失去系统焦点（用户点击了其他窗口/实例）→ 清除本实例焦点，
            // 避免跨实例同时存在焦点环（每个实例的 FocusManager 相互独立）。
            if (instance->focusManager) instance->focusManager->clearFocus();
            if (instance->activeViewport && instance->activeViewport->focusManager)
                instance->activeViewport->focusManager->clearFocus();
            break;
        case EventType::FocusGained:
            break;
        case EventType::TextInput:
            // 文本输入事件：直接发到当前活动视口（焦点控件处理）
            {
                UIInstance kbdTarget = instance->activeViewport
                    ? instance->activeViewport : instance;
                dispatchToBench(kbdTarget, evt);
            }
            break;
        default:
            // 窗口事件 → owner 自身处理
            if (evt.m_type == EventType::WindowClose) {
                instance->quit = true;
            } else if (evt.m_type == EventType::WindowResize) {
                DispatchWindowResize(instance, evt.resizeEvent);
            }
            break;
        }
    }
    return handled;
}

// 返回是否处理了至少一个事件（供多实例主循环调度：处理完所有实例的
// 事件为止——每个实例只消费自己窗口的事件，总有一个实例能处理队头）
int UICornerstone_ProcessEvents(UIInstance instance) {
    if (!instance || !instance->initialized || instance->destroying) return 0;

    bool handled = false;
    if (instance->ownsBackend) {
        // 窗口级别：轮询属于本窗口的输入并分发到子视口（产出 C++ Event）。
        // newFrame 每帧一次（多实例主循环每帧调用一次 ProcessEvents）。
        if (instance->inputBackend) {
            instance->inputBackend->newFrame();
            if (pumpInstanceEvents(instance)) handled = true;
        }
    }

    // 注入队列通路（UIEvent → Event）：所有实例（owner 和 viewport）都处理自己的 queuedEvents
    while (true) {
        UIEvent ue;
        {
            std::lock_guard<std::mutex> lock(instance->queuedEventsMutex);
            if (instance->queuedEvents.empty()) break;
            ue = instance->queuedEvents.front();
            instance->queuedEvents.pop();
        }

        Event event;
        if (!uiEventToEvent(ue, event)) continue;

        handled = true;
        if (event.m_type == EventType::WindowClose) {
            instance->quit = true;
        } else if (event.m_type == EventType::WindowResize) {
            DispatchWindowResize(instance, event.resizeEvent);
        } else if (event.m_type == EventType::FocusLost) {
            // 与轮询通路（pumpInstanceEvents）语义一致：窗口失去系统焦点
            // → 清除本实例焦点（含活动子视口），避免跨实例焦点环并存
            if (instance->focusManager) instance->focusManager->clearFocus();
            if (instance->activeViewport && instance->activeViewport->focusManager)
                instance->activeViewport->focusManager->clearFocus();
        } else if (event.m_type == EventType::KeyDown || event.m_type == EventType::KeyUp) {
            if (!tryViewportScopeSwitch(instance, event)) {
                dispatchToBench(instance, event);
            }
        } else {
            dispatchToBench(instance, event);
        }
    }
    return handled ? 1 : 0;
}

void UICornerstone_Update(UIInstance instance, double deltaTime) {
    (void)deltaTime;
    if (!instance || !instance->initialized || instance->destroying) return;
    if (instance->bench) {
        instance->bench->eventLoopEntry();
        instance->bench->update();
    }
}

void UICornerstone_Render(UIInstance instance) {
    if (!instance || !instance->initialized || instance->destroying) return;
    if (!instance->renderDevice || !instance->bench) return;
    instance->renderDevice->pushClipRect(instance->viewport);
    // 视口背景色：填充整个视口区域（fit/stretch 画布留白处也生效），
    // 默认透明（alpha=0）时跳过，行为与原"仅 clear 透出"一致
    if (instance->viewportBackground.alpha() > 0.0f) {
        instance->renderDevice->setDrawColor(instance->viewportBackground);
        instance->renderDevice->fillRect(instance->viewport);
    }
    instance->bench->draw();
    instance->renderDevice->popClipRect();
}

void UICornerstone_Clear(UIInstance instance) {
    if (!instance || !instance->initialized || instance->destroying) return;
    if (!instance->renderDevice) return;
    instance->renderDevice->setDrawColor(SColor(0.2f, 0.2f, 0.22f, 1.0f));
    instance->renderDevice->clear();
}

void UICornerstone_Present(UIInstance instance) {
    if (!instance || !instance->initialized || instance->destroying) return;
    if (instance->renderDevice) instance->renderDevice->present();
}

int UICornerstone_IsQuitRequested(UIInstance instance) {
    return (instance && instance->quit) ? 1 : 0;
}

// ============================================================
// Debug 辅助
// ============================================================
int UICornerstone_Debug_GetAliveCount(void) {
#ifdef _DEBUG
    std::lock_guard<std::mutex> lock(s_registryMutex);
    return (int)s_aliveInstances.size();
#else
    return 0;
#endif
}

UIInstance UICornerstone_Debug_GetAliveInstance(int index) {
#ifdef _DEBUG
    std::lock_guard<std::mutex> lock(s_registryMutex);
    if (index >= 0 && index < (int)s_aliveInstances.size())
        return s_aliveInstances[index];
    return nullptr;
#else
    (void)index;
    return nullptr;
#endif
}

UIInstance UICornerstone_Debug_GetActiveViewport(UIInstance instance) {
    if (!instance || instance->destroying) return nullptr;
    return instance->activeViewport;
}

int UICornerstone_Debug_IsControlFocused(UIInstance instance, UIControlHandle control) {
    if (!instance || instance->destroying || !control) return 0;
    auto* c = static_cast<Control*>(control);
    return c->getFocused() ? 1 : 0;
}

int UICornerstone_Debug_IsControlHovered(UIInstance instance, UIControlHandle control) {
#ifdef _DEBUG
    if (!instance || instance->destroying || !control) return 0;
    auto* c = static_cast<Control*>(control);
    return c->isMouseInside() ? 1 : 0;
#else
    (void)instance; (void)control;
    return 0;
#endif
}

int UICornerstone_Debug_SetMousePosition(UIInstance instance, float x, float y) {
#ifdef _DEBUG
    if (!instance || instance->destroying) return 0;
    instance->debugMouseOverride = true;
    instance->debugMouseX = x;
    instance->debugMouseY = y;
    return 1;
#else
    (void)instance; (void)x; (void)y;
    return 0;
#endif
}

int UICornerstone_Debug_ClearMousePosition(UIInstance instance) {
#ifdef _DEBUG
    if (!instance || instance->destroying) return 0;
    instance->debugMouseOverride = false;
    return 1;
#else
    (void)instance;
    return 0;
#endif
}

// ============================================================
// 布局系统
// ============================================================
int UICornerstone_LoadLayout(UIInstance instance, const char* jsonContent) {
    if (!instance || !instance->initialized || instance->destroying || !jsonContent) return 0;

    // 顶层 resourceProviders 挂载：path 型条目注册进内存 provider（懒加载缓存）
    if (auto* mem = dynamic_cast<MemoryResourceProvider*>(instance->resourceProvider)) {
        try {
            auto j = json::parse(std::string(jsonContent));
            if (j.is_object() && j.contains(PropertyNames::kJsonResourceProviders) && j[PropertyNames::kJsonResourceProviders].is_array()) {
                for (auto& item : j[PropertyNames::kJsonResourceProviders]) {
                    if (!item.is_object()) continue;
                    std::string name = item.value(PropertyNames::kJsonRPMountName, "");
                    std::string path = item.value(PropertyNames::kJsonRPMountPath, "");
                    if (!name.empty() && !path.empty()) {
                        mem->mountPath(name, path, instance->resourceRoot);
                    }
                }
            }
        } catch (...) { /* 布局语法错误由 parseLayout 报告 */ }
    }

    LayoutParser parser(instance->dataContext);
    parser.setViewportTarget(instance);

    // 注册所有 actions 到 LayoutParser
    for (auto& [name, pair] : instance->actions) {
        UIActionCallback cb = pair.first;
        void* userData = pair.second;
        parser.registerHandler(name, [cb, userData](shared_ptr<Control> ctl) {
            cb(reinterpret_cast<UIControlHandle>(ctl.get()), userData);
        });
    }

    auto root = parser.parseLayout(std::string(jsonContent));
    if (!root) return 0;

    instance->bench->addControl(root);

    // 多平级根控件布局：parseLayout 只返回最后一个控件，其余未挂载
    // （无父）的控件若随 parser 析构销毁，FindControl 会返回悬垂句柄。
    // 将无父控件也挂到 bench 保持生命期与可见性（容器内子控件有父，跳过）。
    for (auto& id : parser.getAllControlIds()) {
        auto ctl = parser.findControlById(id);
        if (ctl && ctl != root && ctl->getParent() == nullptr) {
            instance->bench->addControl(ctl);
        }
    }

    for (auto& mb : parser.getMenuBars()) {
        instance->bench->addControl(mb);
    }

    // 将 JSON 定义的 Dialog 加入 popupPool 保持生命期
    for (auto& pop : parser.getDialogs()) {
        instance->popupPool.push_back(pop);
    }

    for (auto& id : parser.getAllControlIds()) {
        auto ctl = parser.findControlById(id);
        if (ctl) instance->controlsById[id] = reinterpret_cast<UIControlHandle>(ctl.get());
    }

    // 布局动画补 prepare：解析阶段无渲染设备，LuotiAni 挂树后设备就绪；
    // 此处统一补 prepare（语义同 CreateAnimation：创建不自动播放，SetBool "playing" 控制）
    std::function<void(Control*)> prepareLayoutAnis = [&](Control* node) {
        if (auto* ani = dynamic_cast<LuotiAni*>(node)) {
            if (!ani->isPrepared()) {
                try {
                    ani->prepare();
                } catch (...) {
                    UI_LOGE(instance, "layout animation prepare failed");
                }
            }
        }
        if (auto* impl = dynamic_cast<ControlImpl*>(node)) {
            for (auto& child : impl->getChildren()) {
                prepareLayoutAnis(child.get());
            }
        }
    };
    prepareLayoutAnis(instance->bench);

    UI_LOGI(instance, "LoadLayout OK (%zu control ids, %zu menu bars, %zu dialogs)",
        parser.getAllControlIds().size(),
        parser.getMenuBars().size(), parser.getDialogs().size());
    return 1;
}

int UICornerstone_LoadLayoutFromFile(UIInstance instance, const char* filePath) {
    if (!instance || !filePath) return 0;
    if (!instance->resourceProvider) {
        printf("UICornerstone: LoadLayoutFromFile requires ResourceProvider\n");
        return 0;
    }
    auto data = instance->resourceProvider->readFile(filePath);
    if (!data || data->empty()) return 0;
    data->push_back('\0');
    return UICornerstone_LoadLayout(instance, data->data());
}

UIControlHandle UICornerstone_FindControl(UIInstance instance, const char* id) {
    if (!instance || !id) return nullptr;
    auto it = instance->controlsById.find(id);
    return (it != instance->controlsById.end()) ? it->second : nullptr;
}

void UICornerstone_RegisterAction(UIInstance instance, const char* name, UIActionCallback cb, void* userData) {
    if (instance && name) instance->actions[name] = {cb, userData};
}

// ============================================================
// 控件工厂
// ============================================================
UIControlHandle UICornerstone_CreateButton(UIInstance instance, const char* text,
    float x, float y, float w, float h,
    float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<Button>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    if (text) ctl->setCaption(text);
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateLabel(UIInstance instance, const char* text, float fontSize,
    float x, float y, float w, float h,
    float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<Label>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    if (text) ctl->setCaption(text);
    ctl->setFont(FontName::HarmonyOS_Sans_SC_Regular);
    if (fontSize > 0) ctl->setFontSize((int)fontSize);
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateCheckBox(UIInstance instance, const char* text,
    float x, float y, float w, float h,
    float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<CheckBox>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    ctl->createCaption();
    if (text) ctl->getCaption()->setCaption(text);
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateEditBox(UIInstance instance,
    float x, float y, float w, float h,
    float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<EditBox>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateProgressBar(UIInstance instance,
    float x, float y, float w, float h,
    float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<ProgressBar>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateSlider(UIInstance instance,
    float x, float y, float w, float h, float min, float max, float value,
    float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<Slider>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    ctl->setRange(min, max);
    ctl->setValue(value);
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreatePanel(UIInstance instance,
    float x, float y, float w, float h,
    float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<Panel>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateTextArea(UIInstance instance,
    float x, float y, float w, float h,
    float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<TextArea>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateWinFrame(UIInstance instance,
    const char* title, float x, float y, float w, float h,
    float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<WinFrame>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    if (title) ctl->setTitle(title);
    ctl->setTitleTextColor(SColor(0, 0, 0, 255));
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->show();
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateMenuBar(UIInstance instance, float x, float y, float w, float h,
    float xScale, float yScale) {
    if (!instance || !instance->initialized) return nullptr;
    auto bar = make_shared<MenuBar>(instance->bench, xScale, yScale);
    bar->setRect(SRect(x, y, w, h));
    instance->bench->addControl(bar);
    bar->create();
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(bar.get()));
}

UIControlHandle UICornerstone_CreateMenuPanel(UIInstance instance,
    float xScale, float yScale) {
    if (!instance || !instance->initialized) return nullptr;
    auto panel = make_shared<MenuPanel>(nullptr, xScale, yScale);
    panel->create();
    auto* p = reinterpret_cast<UIControlHandle>(static_cast<Control*>(panel.get()));
    menuPoolKeep(instance, panel);
    return p;
}

UIControlHandle UICornerstone_CreateMenuItem(UIInstance instance, const char* caption, int type,
    float xScale, float yScale) {
    if (!instance || !instance->initialized) return nullptr;
    if (type < 0 || type > 2) return nullptr;
    auto item = make_shared<MenuItem>(nullptr, static_cast<MenuItemType>(type), xScale, yScale);
    if (caption) item->setCaption(caption);
    item->create();
    auto* p = reinterpret_cast<UIControlHandle>(static_cast<Control*>(item.get()));
    menuPoolKeep(instance, item);
    return p;
}

void UICornerstone_MenuBarAddMenu(UIInstance instance, UIControlHandle bar, const char* caption, UIControlHandle panel) {
    if (!instance || !bar || !panel) return;
    Control* barV = validateControl(instance, bar);
    Control* panelV = validateControl(instance, panel);
    if (!barV || !panelV) return;
    auto sp = menuPoolTake(instance, panel);
    if (!sp) return;
    auto* mb = dynamic_cast<MenuBar*>(barV);
    if (mb) mb->addMenu(caption ? caption : "", std::dynamic_pointer_cast<MenuPanel>(sp));
}

void UICornerstone_MenuPanelAddItem(UIInstance instance, UIControlHandle panel, UIControlHandle item) {
    if (!instance || !panel || !item) return;
    Control* panelV = validateControl(instance, panel);
    Control* itemV = validateControl(instance, item);
    if (!panelV || !itemV) return;
    auto sp = menuPoolTake(instance, item);
    if (!sp) return;
    auto* pnl = dynamic_cast<MenuPanel*>(panelV);
    if (pnl) pnl->addItem(std::dynamic_pointer_cast<MenuItem>(sp));
}

void UICornerstone_MenuPanelAddSeparator(UIInstance instance, UIControlHandle panel) {
    if (!instance || !panel) return;
    Control* panelV = validateControl(instance, panel);
    if (!panelV) return;
    auto* pnl = dynamic_cast<MenuPanel*>(panelV);
    if (pnl) pnl->addSeparator();
}

void UICornerstone_MenuItemSetSubMenu(UIInstance instance, UIControlHandle item, UIControlHandle panel) {
    if (!instance || !item || !panel) return;
    Control* itemV = validateControl(instance, item);
    Control* panelV = validateControl(instance, panel);
    if (!itemV || !panelV) return;
    auto sp = menuPoolTake(instance, panel);
    if (!sp) return;
    auto* it = dynamic_cast<MenuItem*>(itemV);
    if (it) it->setSubMenu(std::dynamic_pointer_cast<MenuPanel>(sp));
}

UIControlHandle UICornerstone_CreateImageButton(UIInstance instance,
    const char* normalImage, const char* hoverImage, const char* pressedImage,
    float x, float y, float w, float h, float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<Button>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    // Actor 依附于按钮（parent=ctl），累进缩放 = xScale * parent 的累计缩放；
    // 这里必须传 1.0f，否则按钮 2x 时 Actor 再乘 2 变成 4x（图片溢出按钮区域）。
    if (normalImage) {
        auto actor = std::make_shared<Actor>(ctl.get(), fs::path(normalImage), true, 1.0f, 1.0f);
        ctl->setNormalStateActor(actor);
    }
    if (hoverImage) {
        auto actor = std::make_shared<Actor>(ctl.get(), fs::path(hoverImage), true, 1.0f, 1.0f);
        ctl->setHoverStateActor(actor);
    }
    if (pressedImage) {
        auto actor = std::make_shared<Actor>(ctl.get(), fs::path(pressedImage), true, 1.0f, 1.0f);
        ctl->setPressedStateActor(actor);
    }
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateActor(UIInstance instance,
    const char* image, float x, float y, float w, float h, float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    // Actor 构造家族无 SRect 参数（Actor.h 仅 parent/xScale/yScale/filePath/resourceId），
    // 用构造 + setRect（构造时已 setParent(bench)，Actor.cpp:10）
    auto actor = std::make_shared<Actor>(instance->bench, xScale, yScale);
    actor->setRect(SRect(x, y, w, h));                 // 显式 rect 优先（见 Image_Design §6.1）
    if (image) actor->loadFromFile(fs::path(image));   // 两阶段：挂树后 create() 加载
    instance->bench->addControl(actor);                // shared_ptr 生命周期安全
    actor->create();
    actor->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(actor.get()));
}

static Actor* actorOf(UIInstance instance, UIControlHandle handle) {
    if (!instance || !handle) return nullptr;
    Control* ctl = validateControl(instance, handle);
    if (!ctl) return nullptr;
    return dynamic_cast<Actor*>(ctl);
}

UIControlHandle UICornerstone_CreateImage(UIInstance instance,
    const char* image, float x, float y, float w, float h, float xScale, float yScale)
{
    return UICornerstone_CreateActor(instance, image, x, y, w, h, xScale, yScale);
}

// Actor 源矩形（纹理子区域；对 STRETCH 与 TILE 生效）。
// 清除语义：x < 0 或 w ≤ 0 或 h ≤ 0 → 恢复整图。返回 1 成功 / 0 无效句柄。
int UICornerstone_ActorSetSourceRect(UIInstance instance, UIControlHandle ctl,
    float x, float y, float w, float h)
{
    auto* actor = actorOf(instance, ctl);
    if (!actor) return 0;
    if (x < 0.0f || w <= 0.0f || h <= 0.0f) {
        actor->clearSourceRect();
        return 1;
    }
    actor->setSourceRect(SRect(x, y, w, h));
    return 1;
}

UIControlHandle UICornerstone_CreateAnimation(UIInstance instance,
    const char* jsoncPath, float x, float y, float w, float h, float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ani = std::make_shared<LuotiAni>(instance->bench, xScale, yScale);   // 构造不加载（同 Button.cpp:345 用法）
    ani->setRect(SRect(x, y, w, h));                          // w/h 传 0 → prepare 回退到画布尺寸
    instance->bench->addControl(ani);                         // setContext 传播
    if (jsoncPath) {
        try {                                                 // §6.4-1：异常边界，失败回滚 + 返回 nullptr
            fs::path p(jsoncPath);
            // provider: 前缀是资源引用（非文件路径）：不拼 base，由 loadFromFile 分流
            if (p.is_relative() && p.string().rfind(PropertyNames::kProviderPrefix, 0) != 0) p = fs::path(Platform::GetBasePath()) / p;
            ani->loadFromFile(p);
            ani->prepare();
            // 设计语义（test_animation A2）：创建后不自动播放，由调用方 SetBool "playing" 启动
        } catch (...) {
            printf("UICornerstone_CreateAnimation: load/prepare failed for '%s'\n", jsoncPath);
            instance->bench->removeControl(ani);
            return nullptr;
        }
    }
    ani->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ani.get()));
}

// ── Shape 形状控件（design/Shape_Design.md §4.7）──
UIControlHandle UICornerstone_CreateShape(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto shape = std::make_shared<Shape>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    instance->bench->addControl(shape);
    shape->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(shape.get()));
}

int UICornerstone_ShapeSetPoints(UIInstance instance, UIControlHandle sh,
    int count, const float* xs, const float* ys)
{
    if (!instance || !sh || count < 0 || (count > 0 && (!xs || !ys))) return 0;
    Control* ctlV = validateControl(instance, sh);
    if (!ctlV) return 0;
    auto* shape = dynamic_cast<Shape*>(ctlV);
    if (!shape) return 0;
    std::vector<Shape::SPointF> pts;
    pts.reserve(count);
    for (int i = 0; i < count; ++i) pts.push_back({xs[i], ys[i]});
    shape->setPoints(pts);
    return 1;
}

// ── Shape 多图元（组合图形） ──
static Shape* shapeOf(UIInstance instance, UIControlHandle handle) {
    if (!instance || !handle) return nullptr;
    Control* ctl = validateControl(instance, handle);
    if (!ctl) return nullptr;
    return dynamic_cast<Shape*>(ctl);
}

int UICornerstone_ShapeAddPrimitive(UIInstance instance, UIControlHandle sh,
    const char* type, float x, float y, float w, float h)
{
    auto* shape = shapeOf(instance, sh);
    if (!shape || !type) return -1;
    ShapeType t;
    if (!shapeTypeFromString(type, t)) return -1;
    return shape->addPrimitive(t, SRect(x, y, w, h));
}

int UICornerstone_ShapeSetPrimitiveColor(UIInstance instance, UIControlHandle sh,
    int index, const char* prop, UIColor value)
{
    auto* shape = shapeOf(instance, sh);
    if (!shape || !prop) return 0;
    const SColor c(value.r, value.g, value.b, value.a);
    if (strcmp(prop, PropertyNames::kFill) == 0)         { shape->setPrimitiveFill(index, c);    return 1; }
    if (strcmp(prop, PropertyNames::kStroke) == 0)       { shape->setPrimitiveStroke(index, c);  return 1; }
    return 0;
}

int UICornerstone_ShapeSetPrimitiveFloat(UIInstance instance, UIControlHandle sh,
    int index, const char* prop, float value)
{
    auto* shape = shapeOf(instance, sh);
    if (!shape || !prop) return 0;
    if (strcmp(prop, PropertyNames::kLineWidthProp) == 0) { shape->setPrimitiveLineWidth(index, value); return 1; }
    if (strcmp(prop, PropertyNames::kRadius) == 0)        { shape->setPrimitiveRadius(index, value);    return 1; }
    if (strcmp(prop, PropertyNames::kRingWidth) == 0)     { shape->setPrimitiveRingWidth(index, value); return 1; }
    return 0;
}

int UICornerstone_ShapeSetPrimitivePoints(UIInstance instance, UIControlHandle sh,
    int index, int count, const float* xs, const float* ys)
{
    auto* shape = shapeOf(instance, sh);
    if (!shape || count < 0 || (count > 0 && (!xs || !ys))) return 0;
    std::vector<Shape::SPointF> pts;
    pts.reserve(count);
    for (int i = 0; i < count; ++i) pts.push_back({xs[i], ys[i]});
    shape->setPrimitivePoints(index, pts);
    return 1;
}

int UICornerstone_ShapeClearPrimitives(UIInstance instance, UIControlHandle sh)
{
    auto* shape = shapeOf(instance, sh);
    if (!shape) return 0;
    shape->clearPrimitives();
    return 1;
}

int UICornerstone_ShapeMapToDrawPoint(UIInstance instance, UIControlHandle sh,
    float lx, float ly, float* outX, float* outY)
{
    if (!instance || !sh || !outX || !outY) return 0;
    Control* ctlV = validateControl(instance, sh);
    if (!ctlV) return 0;
    auto* shape = dynamic_cast<Shape*>(ctlV);
    if (!shape) return 0;
    const SPoint g = shape->mapToDrawPoint(lx, ly);
    *outX = g.x;
    *outY = g.y;
    return 1;
}

// ── ListView 列表控件（design/ListView_Design.md §7）──
static ListView* listViewOf(UIInstance instance, UIControlHandle handle) {
    if (!instance || !handle) return nullptr;
    Control* ctl = validateControl(instance, handle);
    if (!ctl) return nullptr;
    return dynamic_cast<ListView*>(ctl);
}


static StatusBar* statusBarOf(UIInstance instance, UIControlHandle handle) {
    if (!instance || !handle) return nullptr;
    Control* ctl = validateControl(instance, handle);
    if (!ctl) return nullptr;
    return dynamic_cast<StatusBar*>(ctl);
}

UIControlHandle UICornerstone_CreateStatusBar(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto bar = std::make_shared<StatusBar>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    instance->bench->addControl(bar);
    bar->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(bar.get()));
}

int UICornerstone_StatusBarAddItem(UIInstance instance, UIControlHandle bar,
    const char* id, const char* text, int rightAlign)
{
    auto* v = statusBarOf(instance, bar);
    if (!v || !id || !text) return 0;
    v->addStatusItem(id, text, rightAlign != 0);
    return 1;
}

int UICornerstone_StatusBarSetItemText(UIInstance instance, UIControlHandle bar,
    const char* id, const char* text)
{
    auto* v = statusBarOf(instance, bar);
    if (!v || !id || !text) return 0;
    v->updateStatusItemText(id, text);
    return 1;
}

int UICornerstone_StatusBarRemoveItem(UIInstance instance, UIControlHandle bar, const char* id) {
    auto* v = statusBarOf(instance, bar);
    if (!v || !id) return 0;
    v->removeStatusItem(id);
    return 1;
}

int UICornerstone_StatusBarSetItemMenu(UIInstance instance, UIControlHandle bar,
    const char* id, UIControlHandle menuPanel)
{
    auto* v = statusBarOf(instance, bar);
    if (!v || !id) return 0;
    Control* mp = menuPanel ? validateControl(instance, menuPanel) : nullptr;
    if (menuPanel && !mp) return 0;
    v->setStatusItemMenu(id, mp ? std::dynamic_pointer_cast<MenuPanel>(
        mp->getThis()) : nullptr);
    return 1;
}

int UICornerstone_StatusBarSetItemIcon(UIInstance instance, UIControlHandle bar,
    const char* id, UIControlHandle iconControl)
{
    auto* v = statusBarOf(instance, bar);
    if (!v || !id) return 0;
    Control* icon = iconControl ? validateControl(instance, iconControl) : nullptr;
    if (iconControl && !icon) return 0;
    v->setStatusItemLeadingControl(id, icon ? icon->getThis() : nullptr);
    return 1;
}

// ── ContextMenu C API ──
static ContextMenu* contextMenuOf(UIInstance instance, UIControlHandle handle) {
    if (!instance || !handle) return nullptr;
    Control* ctl = validateControl(instance, handle);
    if (!ctl) return nullptr;
    return dynamic_cast<ContextMenu*>(ctl);
}

UIControlHandle UICornerstone_CreateContextMenu(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto menu = std::make_shared<ContextMenu>(nullptr, xScale, yScale);
    menu->setRect(SRect(x, y, w, h));
    instance->bench->addControl(menu);   // 挂树，确保上下文/渲染设备就绪
    menu->setVisible(false);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(menu.get()));
}

int UICornerstone_ContextMenuAddItem(UIInstance instance, UIControlHandle menu,
    const char* caption, const char* shortcut)
{
    auto* v = contextMenuOf(instance, menu);
    if (!v || !caption) return 0;
    auto item = std::make_shared<MenuItem>(v->getMenuPanel().get(), MenuItemType::Normal, 1.f, 1.f);
    item->setCaption(caption);
    if (shortcut) item->setShortcut(shortcut);
    v->addItem(item);
    return 1;
}

int UICornerstone_ContextMenuAddSeparator(UIInstance instance, UIControlHandle menu) {
    auto* v = contextMenuOf(instance, menu);
    if (!v) return 0;
    v->addSeparator();
    return 1;
}

int UICornerstone_ContextMenuShow(UIInstance instance, UIControlHandle menu, float x, float y) {
    auto* v = contextMenuOf(instance, menu);
    if (!v) return 0;
    v->show(x, y);
    return 1;
}

int UICornerstone_ContextMenuClose(UIInstance instance, UIControlHandle menu) {
    auto* v = contextMenuOf(instance, menu);
    if (!v) return 0;
    v->close();
    return 1;
}

// ── TabControl C API ──
static TabControl* tabControlOf(UIInstance instance, UIControlHandle handle) {
    if (!instance || !handle) return nullptr;
    Control* ctl = validateControl(instance, handle);
    if (!ctl) return nullptr;
    return dynamic_cast<TabControl*>(ctl);
}

UIControlHandle UICornerstone_CreateTabControl(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto tc = std::make_shared<TabControl>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    instance->bench->addControl(tc);
    tc->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(tc.get()));
}

int UICornerstone_TabAddPage(UIInstance instance, UIControlHandle tab, const char* title) {
    auto* v = tabControlOf(instance, tab);
    if (!v || !title) return -1;
    return v->addTab(title, nullptr);
}

int UICornerstone_TabSetTitle(UIInstance instance, UIControlHandle tab,
    int index, const char* title)
{
    auto* v = tabControlOf(instance, tab);
    if (!v || !title) return 0;
    v->setTabText(index, title);
    return 1;
}

int UICornerstone_TabSetPage(UIInstance instance, UIControlHandle tab,
    int index, UIControlHandle page)
{
    auto* v = tabControlOf(instance, tab);
    if (!v || index < 0) return 0;
    Control* pg = page ? validateControl(instance, page) : nullptr;
    if (page && !pg) return 0;
    v->setTabPage(index, pg ? pg->getThis() : nullptr);
    return 1;
}

int UICornerstone_TabSetTabLeadingControl(UIInstance instance, UIControlHandle tab,
    int index, UIControlHandle handle)
{
    auto* v = tabControlOf(instance, tab);
    if (!v || index < 0) return 0;
    Control* ctl = handle ? validateControl(instance, handle) : nullptr;
    if (handle && !ctl) return 0;
    v->setTabLeadingControl(index, ctl ? ctl->getThis() : nullptr);
    return 1;
}

UIControlHandle UICornerstone_CreateListView(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto lv = std::make_shared<ListView>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    instance->bench->addControl(lv);
    lv->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(lv.get()));
}

int UICornerstone_ListViewAddRow(UIInstance instance, UIControlHandle lv,
    const char* id, int count, const char* const* cells)
{
    auto* v = listViewOf(instance, lv);
    if (!v || !id || count < 0 || (count > 0 && !cells)) return 0;
    std::vector<std::string> row;
    for (int i = 0; i < count; ++i) row.push_back(cells[i] ? cells[i] : "");
    return v->addRow(id ? id : "", row) >= 0 ? 1 : 0;
}

int UICornerstone_ListViewInsertRow(UIInstance instance, UIControlHandle lv,
    int index, const char* id, int count, const char* const* cells)
{
    auto* v = listViewOf(instance, lv);
    if (!v || !id || count < 0 || (count > 0 && !cells)) return 0;
    std::vector<std::string> row;
    for (int i = 0; i < count; ++i) row.push_back(cells[i] ? cells[i] : "");
    return v->insertRow(index, id ? id : "", row) >= 0 ? 1 : 0;
}

int UICornerstone_ListViewRemoveRow(UIInstance instance, UIControlHandle lv, int index) {
    auto* v = listViewOf(instance, lv);
    if (!v) return 0;
    v->removeRow(index);
    return 1;
}

int UICornerstone_ListViewSetCellText(UIInstance instance, UIControlHandle lv,
    int row, int col, const char* text)
{
    auto* v = listViewOf(instance, lv);
    if (!v || !text) return 0;
    v->setCell(row, col, text);
    return 1;
}

int UICornerstone_ListViewGetCellText(UIInstance instance, UIControlHandle lv,
    int row, int col, char* outBuf, int maxLen)
{
    auto* v = listViewOf(instance, lv);
    if (!v || !outBuf || maxLen <= 0) return 0;
    const std::string t = v->getCell(row, col);
    snprintf(outBuf, static_cast<size_t>(maxLen), "%s", t.c_str());
    return 1;
}

int UICornerstone_ListViewSetRowCells(UIInstance instance, UIControlHandle lv,
    int index, int count, const char* const* cells)
{
    auto* v = listViewOf(instance, lv);
    if (!v || count < 0 || (count > 0 && !cells)) return 0;
    std::vector<std::string> row;
    for (int i = 0; i < count; ++i) row.push_back(cells[i] ? cells[i] : "");
    v->setRowCells(index, row);
    return 1;
}

int UICornerstone_ListViewSetColumnValues(UIInstance instance, UIControlHandle lv,
    int colIndex, int count, const char* const* values)
{
    auto* v = listViewOf(instance, lv);
    if (!v || colIndex < 0 || count < 0 || (count > 0 && !values)) return 0;
    std::vector<std::string> vals;
    for (int i = 0; i < count; ++i) vals.push_back(values[i] ? values[i] : "");
    v->setColumnValues(colIndex, vals);
    return 1;
}

int UICornerstone_ListViewAddColumn(UIInstance instance, UIControlHandle lv,
    const char* title, float width, int sortable)
{
    auto* v = listViewOf(instance, lv);
    if (!v || !title) return 0;
    return v->addColumn(title, width, sortable != 0) >= 0 ? 1 : 0;
}

int UICornerstone_ListViewInsertColumn(UIInstance instance, UIControlHandle lv,
    int index, const char* title, float width, int sortable)
{
    auto* v = listViewOf(instance, lv);
    if (!v || !title) return 0;
    return v->insertColumn(index, title ? title : "", width, sortable != 0) >= 0 ? 1 : 0;
}

int UICornerstone_ListViewRemoveColumn(UIInstance instance, UIControlHandle lv, int index) {
    auto* v = listViewOf(instance, lv);
    if (!v) return 0;
    v->removeColumn(index);
    return 1;
}

int UICornerstone_ListViewSetColumnWidth(UIInstance instance, UIControlHandle lv,
    int index, float width)
{
    auto* v = listViewOf(instance, lv);
    if (!v) return 0;
    v->setColumnWidth(index, width);
    return 1;
}

int UICornerstone_ListViewSetColumnIcon(UIInstance instance, UIControlHandle lv,
    int colIndex, UIControlHandle iconControl)
{
    auto* v = listViewOf(instance, lv);
    if (!v) return 0;
    Control* icon = iconControl ? validateControl(instance, iconControl) : nullptr;
    if (iconControl && !icon) return 0;
    v->setColumnLeadingControl(colIndex,
        icon ? icon->getThis() : nullptr);
    return 1;
}

int UICornerstone_ListViewSetRowLeadingControl(UIInstance instance, UIControlHandle lv,
    int index, UIControlHandle iconControl)
{
    auto* v = listViewOf(instance, lv);
    if (!v) return 0;
    Control* icon = iconControl ? validateControl(instance, iconControl) : nullptr;
    if (iconControl && !icon) return 0;
    v->setRowLeadingControl(index, icon ? icon->getThis() : nullptr);
    return 1;
}

int UICornerstone_ListViewSetCellLeadingControl(UIInstance instance, UIControlHandle lv,
    int row, int col, UIControlHandle control)
{
    auto* v = listViewOf(instance, lv);
    if (!v) return 0;
    Control* ctl = control ? validateControl(instance, control) : nullptr;
    if (control && !ctl) return 0;
    v->setCellLeadingControl(row, col, ctl ? ctl->getThis() : nullptr);
    return 1;
}

int UICornerstone_ListViewSetCellStyle(UIInstance instance, UIControlHandle lv,
    int row, int col, uint8_t bgR, uint8_t bgG, uint8_t bgB, uint8_t bgA, int fontSize)
{
    auto* v = listViewOf(instance, lv);
    if (!v) return 0;
    CellStyle st;
    st.bgColor = SColor(bgR, bgG, bgB, bgA);
    st.fontSize = fontSize;
    v->setCellStyle(row, col, st);
    return 1;
}

int UICornerstone_ListViewSetColumnHeaderStyle(UIInstance instance, UIControlHandle lv,
    int colIndex, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int fontSize)
{
    auto* v = listViewOf(instance, lv);
    if (!v) return 0;
    HeaderStyle hs;
    hs.textColor = SColor(r, g, b, a);
    hs.fontSize = fontSize;
    v->setColumnHeaderStyle(colIndex, hs);
    return 1;
}

int UICornerstone_ListViewSetColumnSorter(UIInstance instance, UIControlHandle lv,
    int colIndex, ListViewSortFn cmp, void* userData)
{
    auto* v = listViewOf(instance, lv);
    if (!v) return 0;
    if (!cmp) { v->clearColumnSorter(colIndex); return 1; }
    v->setColumnSorter(colIndex, [cmp, userData](const std::string& a, const std::string& b) {
        return cmp(a.c_str(), b.c_str(), userData) < 0;
    });
    return 1;
}

 UIControlHandle UICornerstone_CreateAnimatedButton(UIInstance instance,
    const char* jsoncPath, float x, float y, float w, float h, float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto btn = std::make_shared<Button>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    // 内嵌资源：不挂树，不响应鼠标；构造 scale 恒为 1.0，按钮 scale 经
    // setParent 复合缩放作用于动画（传 xScale/yScale 会造成双重缩放）
    auto ani = std::make_shared<LuotiAni>(btn.get(), 1.0f, 1.0f);   // 内嵌资源：不挂树，不响应鼠标
    ani->setRect(SRect(0, 0, w, h));
    if (jsoncPath) {
        try {
            fs::path p(jsoncPath);
            // provider: 前缀是资源引用（非文件路径）：不拼 base，由 loadFromFile 分流
            if (p.is_relative() && p.string().rfind(PropertyNames::kProviderPrefix, 0) != 0) p = fs::path(Platform::GetBasePath()) / p;
            ani->loadFromFile(p);
            ani->prepare();
        } catch (...) {
            printf("UICornerstone_CreateAnimatedButton: load/prepare failed for '%s'\n", jsoncPath);
            return nullptr;
        }
    }
    btn->setLuotiAni(ani);
    instance->bench->addControl(btn);
    btn->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(btn.get()));
}

// ── LuotiAni 动画操作 ──
int UICornerstone_AnimationPrepare(UIInstance instance, UIControlHandle ctl, int startFrame) {
    if (!instance || !ctl) return 0;
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return 0;
    auto* ani = dynamic_cast<LuotiAni*>(ctlV);
    if (!ani) return 0;
    ani->prepare(static_cast<uint32_t>(startFrame < 0 ? 0 : startFrame));
    return 1;
}

int UICornerstone_AnimationSetFrameFilter(UIInstance instance, UIControlHandle ctl, int bilinear) {
    if (!instance || !ctl) return 0;
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return 0;
    auto* ani = dynamic_cast<LuotiAni*>(ctlV);
    if (!ani) return 0;
    ani->setFrameFilter(bilinear != 0);
    return 1;
}

// ============================================================
// 控件通用操作
// ============================================================
void UICornerstone_SetRect(UIInstance instance, UIControlHandle ctl, float x, float y, float w, float h) {
    if (!instance || !ctl) return;
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return;
    ctlV->setRect(SRect(x, y, w, h));
}

void UICornerstone_GetRect(UIInstance instance, UIControlHandle ctl, float* x, float* y, float* w, float* h) {
    if (!instance || !ctl) return;
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return;
    SRect r = ctlV->getRect();
    if (x) *x = r.left;
    if (y) *y = r.top;
    if (w) *w = r.width;
    if (h) *h = r.height;
}

void UICornerstone_AddChildControl(UIInstance instance, UIControlHandle parent, UIControlHandle child) {
    if (!instance || !parent || !child) return;
    Control* parentV = validateControl(instance, parent);
    Control* childV = validateControl(instance, child);
    if (!parentV || !childV) return;
    auto* ctlImpl = dynamic_cast<ControlImpl*>(childV);
    auto* panel = dynamic_cast<Panel*>(parentV);
    if (!ctlImpl || !panel) return;
    auto sp = ctlImpl->shared_from_this();
    instance->bench->removeControl(sp);
    panel->addControl(sp);
}

const char* UICornerstone_GetControlId(UIInstance instance, UIControlHandle ctl) {
    if (!instance || !ctl) return "";
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return "";
    for (const auto& pair : instance->controlsById) {
        if (pair.second == ctl) {
            instance->strBuf = pair.first;
            return instance->strBuf.c_str();
        }
    }
    instance->strBuf.clear();
    return instance->strBuf.c_str();
}

void UICornerstone_DestroyControl(UIInstance instance, UIControlHandle ctl) {
    if (!instance || !ctl) return;
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return;
    auto* ctrl = dynamic_cast<ControlImpl*>(ctlV);
    if (!ctrl) return;
    try {
        auto sp = ctrl->shared_from_this();
        Control* parent = ctrl->getParent();
        if (parent && parent != instance->bench) {
            parent->removeControl(sp);
        } else {
            instance->bench->removeControl(sp);
        }
    } catch (...) {}
}

// ============================================================
// 截图（Capture_*，测试辅助）
// ============================================================
static bool captureRectRaw(UIInstance instance, float x, float y, float w, float h,
                           uint8_t* outPixels, int* outW, int* outH) {
    if (!instance || !instance->initialized || instance->destroying) return false;
    if (!instance->bench || !instance->renderDevice) return false;
    if (!outPixels || w <= 0.0f || h <= 0.0f) return false;
    if (!(instance->backendManager->capabilities() & UICORN_BACKEND_CAP_READBACK)) return false;
    // 与视口求交集：部分越界裁剪；交集为空 → 失败
    const SRect& vp = instance->viewport;
    float ix = x > vp.left ? x : vp.left;
    float iy = y > vp.top ? y : vp.top;
    float ix2 = x + w < vp.left + vp.width ? x + w : vp.left + vp.width;
    float iy2 = y + h < vp.top + vp.height ? y + h : vp.top + vp.height;
    if (ix2 <= ix || iy2 <= iy) return false;
    instance->renderDevice->readPixels(outPixels, SRect(ix, iy, ix2 - ix, iy2 - iy));
    if (outW) *outW = static_cast<int>(ix2 - ix);
    if (outH) *outH = static_cast<int>(iy2 - iy);
    return true;
}

int UICornerstone_CaptureRect(UIInstance instance, float x, float y, float w, float h,
                              uint8_t* outPixels, int* outW, int* outH) {
    return captureRectRaw(instance, x, y, w, h, outPixels, outW, outH) ? 1 : 0;
}

int UICornerstone_CaptureViewport(UIInstance instance, uint8_t* out, int* w, int* h) {
    if (!instance) return 0;
    return UICornerstone_CaptureRect(instance, instance->viewport.left, instance->viewport.top,
                                     instance->viewport.width, instance->viewport.height, out, w, h);
}

int UICornerstone_CaptureBench(UIInstance instance, uint8_t* out, int* w, int* h) {
    if (!instance || !instance->bench) return 0;
    SRect r = instance->bench->getDrawRect();
    return UICornerstone_CaptureRect(instance, r.left, r.top, r.width, r.height, out, w, h);
}

int UICornerstone_CaptureControl(UIInstance instance, UIControlHandle ctl,
                                 uint8_t* out, int* w, int* h) {
    if (!instance || !ctl) return 0;
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return 0;
    SRect r = ctlV->getDrawRect();
    return UICornerstone_CaptureRect(instance, r.left, r.top, r.width, r.height, out, w, h);
}

int UICornerstone_SavePixelsToFile(const uint8_t* pixels, int w, int h, const char* filePath) {
    if (!pixels || !filePath || w <= 0 || h <= 0) return 0;
    const int rowSize = w * 4;
    const int dataSize = rowSize * h;
    const int fileSize = 54 + dataSize;
    FILE* fp = fopen(filePath, "wb");
    if (!fp) return 0;
    // BMP 32 位 BGRA：14 字节 BITMAPFILEHEADER + 40 字节 BITMAPINFOHEADER，
    // 像素 bottom-up（内存 top-down 逐行倒序写入），每像素 B,G,R,A
    uint8_t hdr[54] = {0};
    hdr[0] = 'B'; hdr[1] = 'M';
    hdr[2] = static_cast<uint8_t>(fileSize);         hdr[3] = static_cast<uint8_t>(fileSize >> 8);
    hdr[4] = static_cast<uint8_t>(fileSize >> 16);   hdr[5] = static_cast<uint8_t>(fileSize >> 24);
    hdr[10] = 54;                                    // bfOffBits
    hdr[14] = 40;                                    // biSize
    hdr[18] = static_cast<uint8_t>(w);               hdr[19] = static_cast<uint8_t>(w >> 8);
    hdr[20] = static_cast<uint8_t>(w >> 16);         hdr[21] = static_cast<uint8_t>(w >> 24);
    hdr[22] = static_cast<uint8_t>(h);               hdr[23] = static_cast<uint8_t>(h >> 8);
    hdr[24] = static_cast<uint8_t>(h >> 16);         hdr[25] = static_cast<uint8_t>(h >> 24);
    hdr[26] = 1;                                     // biPlanes
    hdr[28] = 32;                                    // biBitCount
    hdr[34] = static_cast<uint8_t>(dataSize);        hdr[35] = static_cast<uint8_t>(dataSize >> 8);
    hdr[36] = static_cast<uint8_t>(dataSize >> 16);  hdr[37] = static_cast<uint8_t>(dataSize >> 24);
    if (fwrite(hdr, 1, 54, fp) != 54) { fclose(fp); return 0; }
    std::vector<uint8_t> row(static_cast<size_t>(rowSize));
    for (int y = h - 1; y >= 0; --y) {
        const uint8_t* src = pixels + static_cast<size_t>(y) * rowSize;
        for (int x = 0; x < w; ++x) {
            row[x * 4 + 0] = src[x * 4 + 2];   // B
            row[x * 4 + 1] = src[x * 4 + 1];   // G
            row[x * 4 + 2] = src[x * 4 + 0];   // R
            row[x * 4 + 3] = src[x * 4 + 3];   // A
        }
        if (fwrite(row.data(), 1, static_cast<size_t>(rowSize), fp) != static_cast<size_t>(rowSize)) {
            fclose(fp); return 0;
        }
    }
    fclose(fp);
    return 1;
}

// ============================================================
// ColorPicker
// ============================================================
UIControlHandle UICornerstone_CreateColorPicker(UIInstance instance,
    float x, float y, float w, float h, const char* color,
    float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<ColorPicker>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    if (color) ctl->setColor(color);
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

// ============================================================
// ComboBox
// ============================================================
UIControlHandle UICornerstone_CreateComboBox(UIInstance instance,
    float x, float y, float w, float h,
    float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<ComboBox>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    instance->bench->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

// ============================================================
// Dialog / Popup
// ============================================================
UIControlHandle UICornerstone_CreateDialog(UIInstance instance,
    const char* confirmText, const char* cancelText,
    float x, float y, float w, float h, float xScale, float yScale)
{
    if (!instance || !instance->initialized) return nullptr;
    auto ctl = std::make_shared<Dialog>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    if (confirmText) ctl->setConfirmButtonText(confirmText);
    if (cancelText) ctl->setCancelButtonText(cancelText);
    ctl->setCentered();
    ctl->create();
    ctl->open();    // create() 内 setVisible(false)，须 open() 才显示（computeTargetRect 居中定位）

    // 保持 Dialog 生命期：加入 popupPool，close() 时自动清理
    instance->popupPool.push_back(ctl);

    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

// ── NumericUpDown C ABI ──

UIControlHandle UICornerstone_CreateNumericUpDown(UIInstance instance, float x, float y, float w, float h,
    float xScale, float yScale) {
    if (!instance || !instance->initialized) return nullptr;
    auto nud = make_shared<NumericUpDown>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    instance->bench->addControl(nud);
    nud->create();
    nud->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(nud.get()));
}

// ── Splitter C ABI ──

UIControlHandle UICornerstone_CreateSplitter(UIInstance instance, float x, float y, float w, float h, int orientation,
    float xScale, float yScale) {
    if (!instance || !instance->initialized) return nullptr;
    auto sp = make_shared<Splitter>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    sp->setOrientation(orientation != 0);
    instance->bench->addControl(sp);
    sp->create();
    sp->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(sp.get()));
}

UIControlHandle UICornerstone_CreateScrollBar(UIInstance instance, float x, float y, float w, float h, int orientation,
    float xScale, float yScale) {
    if (!instance || !instance->initialized) return nullptr;
    auto sb = make_shared<ScrollBar>(instance->bench, SRect(x, y, w, h),
        orientation != 0 ? ScrollBarOrientation::Horizontal : ScrollBarOrientation::Vertical,
        xScale, yScale);
    instance->bench->addControl(sb);
    sb->create();
    sb->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(sb.get()));
}

UIControlHandle UICornerstone_CreateTreeView(UIInstance instance, float x, float y, float w, float h,
    float xScale, float yScale) {
    if (!instance || !instance->initialized) return nullptr;
    auto tv = make_shared<TreeView>(instance->bench, SRect(x, y, w, h), xScale, yScale);
    instance->bench->addControl(tv);
    tv->create();
    tv->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(tv.get()));
}

// ── TreeView 节点操作 ──
static TreeView* treeViewOf(UIInstance instance, UIControlHandle handle) {
    if (!instance || !handle) return nullptr;
    Control* ctl = validateControl(instance, handle);
    if (!ctl) return nullptr;
    return dynamic_cast<TreeView*>(ctl);
}

// ── EditBox / TextArea 文本操作 ──
static EditBox* editBoxOf(UIInstance instance, UIControlHandle handle) {
    if (!instance || !handle) return nullptr;
    Control* ctl = validateControl(instance, handle);
    if (!ctl) return nullptr;
    return dynamic_cast<EditBox*>(ctl);
}

int UICornerstone_EditBoxSelectAll(UIInstance instance, UIControlHandle ctl) {
    EditBox* eb = editBoxOf(instance, ctl);
    if (!eb) return 0;
    eb->selectAll();
    return 1;
}

int UICornerstone_EditBoxSetSelection(UIInstance instance, UIControlHandle ctl, int start, int end) {
    EditBox* eb = editBoxOf(instance, ctl);
    if (!eb) return 0;
    eb->setSelection(start, end);
    return 1;
}

int UICornerstone_EditBoxClearSelection(UIInstance instance, UIControlHandle ctl) {
    EditBox* eb = editBoxOf(instance, ctl);
    if (!eb) return 0;
    eb->clearSelection();
    return 1;
}

int UICornerstone_EditBoxHasSelection(UIInstance instance, UIControlHandle ctl) {
    EditBox* eb = editBoxOf(instance, ctl);
    if (!eb) return 0;
    return eb->hasSelection() ? 1 : 0;
}

int UICornerstone_EditBoxGetCursorPosition(UIInstance instance, UIControlHandle ctl) {
    EditBox* eb = editBoxOf(instance, ctl);
    if (!eb) return -1;
    return eb->getCursorPosition();
}

int UICornerstone_EditBoxCopy(UIInstance instance, UIControlHandle ctl) {
    EditBox* eb = editBoxOf(instance, ctl);
    if (!eb) return 0;
    eb->copy();
    return 1;
}

int UICornerstone_EditBoxCut(UIInstance instance, UIControlHandle ctl) {
    EditBox* eb = editBoxOf(instance, ctl);
    if (!eb) return 0;
    eb->cut();
    return 1;
}

int UICornerstone_EditBoxPaste(UIInstance instance, UIControlHandle ctl) {
    EditBox* eb = editBoxOf(instance, ctl);
    if (!eb) return 0;
    eb->paste();
    return 1;
}

int UICornerstone_EditBoxDeleteSelectedText(UIInstance instance, UIControlHandle ctl) {
    EditBox* eb = editBoxOf(instance, ctl);
    if (!eb) return 0;
    eb->deleteSelectedText();
    return 1;
}

// ── NumericUpDown 数值操作 ──
int UICornerstone_NumericUpDownStep(UIInstance instance, UIControlHandle ctl, int dir) {
    if (!instance || !ctl) return 0;
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return 0;
    auto* nud = dynamic_cast<NumericUpDown*>(ctlV);
    if (!nud) return 0;
    nud->stepValue(dir);
    return 1;
}

// ── ComboBox 选项操作 ──
int UICornerstone_ComboBoxAddItem(UIInstance instance, UIControlHandle ctl,
    const char* label, const char* value, int disabled) {
    if (!instance || !ctl || !label) return 0;
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return 0;
    auto* combo = dynamic_cast<ComboBox*>(ctlV);
    if (!combo) return 0;
    combo->addItem(label ? label : "", value ? value : "", disabled != 0);
    return 1;
}

int UICornerstone_ComboBoxRemoveItem(UIInstance instance, UIControlHandle ctl, int index) {
    if (!instance || !ctl || index < 0) return 0;
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return 0;
    auto* combo = dynamic_cast<ComboBox*>(ctlV);
    if (!combo) return 0;
    if (index >= combo->getItemCount()) return 0;
    combo->removeItem(index);
    return 1;
}

int UICornerstone_ComboBoxClearItems(UIInstance instance, UIControlHandle ctl) {
    if (!instance || !ctl) return 0;
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return 0;
    auto* combo = dynamic_cast<ComboBox*>(ctlV);
    if (!combo) return 0;
    combo->clearItems();
    return 1;
}

int UICornerstone_ComboBoxGetItemCount(UIInstance instance, UIControlHandle ctl) {
    if (!instance || !ctl) return -1;
    Control* ctlV = validateControl(instance, ctl);
    if (!ctlV) return -1;
    auto* combo = dynamic_cast<ComboBox*>(ctlV);
    if (!combo) return -1;
    return combo->getItemCount();
}

int UICornerstone_TreeViewAddNode(UIInstance instance, UIControlHandle tree,
    const char* parentId, const char* id, const char* label, int expanded) {
    TreeView* tv = treeViewOf(instance, tree);
    if (!tv || !id || !label) return 0;
    auto node = makeNode(id ? id : "", label ? label : "", expanded != 0);
    if (parentId && parentId[0] != '\0')
        return tv->addChild(parentId, node) ? 1 : 0;
    return tv->addRootItem(node) ? 1 : 0;
}

int UICornerstone_TreeViewRemoveNode(UIInstance instance, UIControlHandle tree, const char* id) {
    TreeView* tv = treeViewOf(instance, tree);
    if (!tv || !id) return 0;
    return tv->removeNode(id) ? 1 : 0;
}

int UICornerstone_TreeViewSetNodeLabel(UIInstance instance, UIControlHandle tree,
    const char* id, const char* label) {
    TreeView* tv = treeViewOf(instance, tree);
    if (!tv || !id || !label) return 0;
    return tv->setNodeLabel(id, label) ? 1 : 0;
}

int UICornerstone_TreeViewSetNodeUserData(UIInstance instance, UIControlHandle tree,
    const char* id, void* userData) {
    TreeView* tv = treeViewOf(instance, tree);
    if (!tv || !id) return 0;
    return tv->setNodeUserData(id, userData) ? 1 : 0;
}

int UICornerstone_TreeViewSelectNode(UIInstance instance, UIControlHandle tree, const char* id) {
    TreeView* tv = treeViewOf(instance, tree);
    if (!tv || !id) return 0;
    return tv->selectNode(id) ? 1 : 0;
}

void UICornerstone_TreeViewClearSelection(UIInstance instance, UIControlHandle tree) {
    TreeView* tv = treeViewOf(instance, tree);
    if (tv) tv->clearSelection();
}

int UICornerstone_TreeViewExpandNode(UIInstance instance, UIControlHandle tree, const char* id) {
    TreeView* tv = treeViewOf(instance, tree);
    if (!tv || !id) return 0;
    return tv->expandNode(id) ? 1 : 0;
}

int UICornerstone_TreeViewCollapseNode(UIInstance instance, UIControlHandle tree, const char* id) {
    TreeView* tv = treeViewOf(instance, tree);
    if (!tv || !id) return 0;
    return tv->collapseNode(id) ? 1 : 0;
}

void UICornerstone_TreeViewExpandAll(UIInstance instance, UIControlHandle tree) {
    TreeView* tv = treeViewOf(instance, tree);
    if (tv) tv->expandAll();
}

void UICornerstone_TreeViewCollapseAll(UIInstance instance, UIControlHandle tree) {
    TreeView* tv = treeViewOf(instance, tree);
    if (tv) tv->collapseAll();
}

void UICornerstone_TreeViewClearItems(UIInstance instance, UIControlHandle tree) {
    TreeView* tv = treeViewOf(instance, tree);
    if (tv) tv->clearItems();
}

int UICornerstone_TreeViewGetSelectedId(UIInstance instance, UIControlHandle tree,
    char* outBuf, int outSize) {
    TreeView* tv = treeViewOf(instance, tree);
    if (!tv || !outBuf || outSize <= 0) return 0;
    const string& id = tv->getSelectedId();
    if (id.empty()) return 0;
    int n = (int)id.size();
    if (n >= outSize) n = outSize - 1;
    memcpy(outBuf, id.c_str(), (size_t)n);
    outBuf[n] = '\0';
    return 1;
}

UIControlHandle UICornerstone_CreateHandleControl(UIInstance instance,
    UIControlHandle target, float x, float y, float w, float h,
    float xScale, float yScale) {
    if (!instance || !target) return nullptr;
    Control* targetV = validateControl(instance, target);
    if (!targetV) return nullptr;
    auto hc = make_shared<HandleControl>();
    hc->setRect(SRect(x, y, w, h));
    hc->setScaleX(xScale);
    hc->setScaleY(yScale);
    hc->create();
    hc->setTarget(targetV);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(hc.get()));
}

// ============================================================
// Property system (string-based, multi-type)
// ============================================================

int UICornerstone_SetColor(UIInstance instance, UIControlHandle ctl, const char* prop, UIColor value) {
    if (!instance || !ctl || !prop) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->setColorProperty(prop, SColor(value.r, value.g, value.b, value.a));
}

int UICornerstone_SetStateColor(UIInstance instance, UIControlHandle ctl, const char* prop, UIStateColor value) {
    if (!instance || !ctl || !prop) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->setStateColorProperty(prop,
        StateColor(
            SColor(value.normal.r, value.normal.g, value.normal.b, value.normal.a),
            SColor(value.hover.r, value.hover.g, value.hover.b, value.hover.a),
            SColor(value.pressed.r, value.pressed.g, value.pressed.b, value.pressed.a),
            SColor(value.disabled.r, value.disabled.g, value.disabled.b, value.disabled.a)
        ));
}

int UICornerstone_SetInt(UIInstance instance, UIControlHandle ctl, const char* prop, int value) {
    if (!instance || !ctl || !prop) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->setIntProperty(prop, value);
}

int UICornerstone_SetFloat(UIInstance instance, UIControlHandle ctl, const char* prop, float value) {
    if (!instance || !ctl || !prop) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->setFloatProperty(prop, value);
}

int UICornerstone_SetString(UIInstance instance, UIControlHandle ctl, const char* prop, const char* value) {
    if (!instance || !ctl || !prop) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->setStringProperty(prop, value);
}

int UICornerstone_SetBool(UIInstance instance, UIControlHandle ctl, const char* prop, int value) {
    if (!instance || !ctl || !prop) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->setBoolProperty(prop, value);
}

int UICornerstone_SetEnum(UIInstance instance, UIControlHandle ctl, const char* prop, const char* value) {
    if (!instance || !ctl || !prop || !value) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->setEnumProperty(prop, value);
}

int UICornerstone_SetPtr(UIInstance instance, UIControlHandle ctl, const char* prop, void* value) {
    if (!instance || !ctl || !prop) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->setPtrProperty(prop, value);
}

int UICornerstone_GetPtr(UIInstance instance, UIControlHandle ctl, const char* prop, void** out) {
    if (!instance || !ctl || !prop || !out) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->getPtrProperty(prop, *out);
}

// 控件运行时类型查询：dynamic_cast 链（具体类在前），返回 JSON "type" 小写 kebab-case。
// 新增控件类型时须在此补充分支（与 PropertyNames::kControlType* 保持一致）。
int UICornerstone_GetControlType(UIInstance instance, UIControlHandle ctl, char* out, int maxLen) {
    if (!instance || !ctl || !out || maxLen <= 0) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    // 枚举→字符串（构造时由子类设置的 m_type，O(1) 查询）
    const char* type = nullptr;
    switch (c->getControlType()) {
        case ControlType::Label:         type = PropertyNames::kControlTypeLabel; break;
        case ControlType::Button:        type = PropertyNames::kControlTypeButton; break;   // 含 image-button
        case ControlType::EditBox:       type = PropertyNames::kControlTypeEditBox; break;
        case ControlType::ComboBox:      type = PropertyNames::kControlTypeComboBox; break;
        case ControlType::TextArea:      type = PropertyNames::kControlTypeTextArea; break;
        case ControlType::CheckBox:      type = PropertyNames::kControlTypeCheckBox; break;
        case ControlType::ProgressBar:   type = PropertyNames::kControlTypeProgressBar; break;
        case ControlType::Slider:        type = PropertyNames::kControlTypeSlider; break;
        case ControlType::ScrollBar:     type = PropertyNames::kControlTypeScrollBar; break;
        case ControlType::Panel:         type = PropertyNames::kControlTypePanel; break;
        case ControlType::WinFrame:      type = PropertyNames::kControlTypeWinFrame; break;
        case ControlType::ColorPicker:   type = PropertyNames::kControlTypeColorPicker; break;
        case ControlType::Splitter:      type = PropertyNames::kControlTypeSplitter; break;
        case ControlType::TreeView:      type = PropertyNames::kControlTypeTreeView; break;
        case ControlType::NumericUpDown: type = PropertyNames::kControlTypeNumericUpDown; break;
        case ControlType::Popup:         type = PropertyNames::kControlTypePopup; break;
        case ControlType::ConfirmPopup:  type = PropertyNames::kControlTypeConfirmPopup; break;
        case ControlType::Dialog:        type = PropertyNames::kControlTypeDialog; break;
        case ControlType::MenuItem:      type = PropertyNames::kControlTypeMenuItem; break;
        case ControlType::MenuPanel:     type = PropertyNames::kControlTypeMenuPanel; break;
        case ControlType::MenuBar:       type = PropertyNames::kControlTypeMenuBar; break;
        case ControlType::Image:         type = PropertyNames::kControlTypeImage; break;
        case ControlType::Animation:     type = PropertyNames::kControlTypeAnimation; break;
        case ControlType::Shape:         type = PropertyNames::kControlTypeShape; break;
        case ControlType::ListView:      type = PropertyNames::kControlTypeListView; break;
        case ControlType::StatusBar:     type = PropertyNames::kControlTypeStatusBar; break;
        case ControlType::TabControl:    type = PropertyNames::kControlTypeTabControl; break;
        case ControlType::HandleControl: type = PropertyNames::kControlTypeHandleControl; break;
        default:                         break;
    }
    if (!type) return 0;
    strncpy_s(out, maxLen, type, _TRUNCATE);
    return 1;
}

int UICornerstone_SetCallback(UIInstance instance, UIControlHandle ctl, const char* event, UIEventCallback cb, void* userData) {
    if (!instance || !ctl || !event) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->setCallbackProperty(event, reinterpret_cast<void(*)(void*, const void*, void*)>(cb), userData);
}

int UICornerstone_GetColor(UIInstance instance, UIControlHandle ctl, const char* prop, UIColor* out) {
    if (!instance || !ctl || !prop || !out) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    SColor s;
    if (!c->getColorProperty(prop, s)) return 0;
    out->r = s.redByte(); out->g = s.greenByte();
    out->b = s.blueByte(); out->a = s.alphaByte();
    return 1;
}

int UICornerstone_GetStateColor(UIInstance instance, UIControlHandle ctl, const char* prop, UIStateColor* out) {
    if (!instance || !ctl || !prop || !out) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    SColor def = SColor();
    StateColor sc{def, def, def, def};
    if (!c->getStateColorProperty(prop, sc)) return 0;
    out->normal   = { sc.getNormal().redByte(),   sc.getNormal().greenByte(),   sc.getNormal().blueByte(),   sc.getNormal().alphaByte() };
    out->hover    = { sc.getHover().redByte(),    sc.getHover().greenByte(),    sc.getHover().blueByte(),    sc.getHover().alphaByte() };
    out->pressed  = { sc.getPressed().redByte(),  sc.getPressed().greenByte(),  sc.getPressed().blueByte(),  sc.getPressed().alphaByte() };
    out->disabled = { sc.getDisabled().redByte(), sc.getDisabled().greenByte(), sc.getDisabled().blueByte(), sc.getDisabled().alphaByte() };
    return 1;
}

int UICornerstone_GetBool(UIInstance instance, UIControlHandle ctl, const char* prop, int* out) {
    if (!instance || !ctl || !prop || !out) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->getBoolProperty(prop, *out);
}

int UICornerstone_GetInt(UIInstance instance, UIControlHandle ctl, const char* prop, int* out) {
    if (!instance || !ctl || !prop || !out) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->getIntProperty(prop, *out);
}

int UICornerstone_GetFloat(UIInstance instance, UIControlHandle ctl, const char* prop, float* out) {
    if (!instance || !ctl || !prop || !out) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    return c->getFloatProperty(prop, *out);
}

int UICornerstone_GetString(UIInstance instance, UIControlHandle ctl, const char* prop, char* out, int maxLen) {
    if (!instance || !ctl || !prop || !out || maxLen <= 0) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    const char* s = nullptr;
    if (!c->getStringProperty(prop, s) || !s) return 0;
    strncpy_s(out, maxLen, s, _TRUNCATE);
    return 1;
}

int UICornerstone_GetEnum(UIInstance instance, UIControlHandle ctl, const char* prop, char* out, int maxLen) {
    if (!instance || !ctl || !prop || !out || maxLen <= 0) return 0;
    Control* c = validateControl(instance, ctl);
    if (!c) return 0;
    const char* s = nullptr;
    if (!c->getEnumProperty(prop, s) || !s) return 0;
    strncpy_s(out, maxLen, s, _TRUNCATE);
    return 1;
}
