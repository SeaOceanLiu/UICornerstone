// UICornerstone C++ Binding — 主类实现
// 许可证 MIT。仅调用 C ABI（经动态 API 层函数指针），不引用核心库内部头。
#include "UICornerstone.h"
#include "Control.h"
#include "Event.h"
#include "Impl.h"
#include "DynamicApi.h"
#include "UIEventFactory.h"   // UICornerstone::Input::* 内联定义

#include <chrono>
#include <algorithm>

// 仅需 kernel32 的 FreeLibrary（HMODULE 即 void*），不引入 windows.h，
// 避免其 UNICODE 条件宏（CreateDialog→CreateDialogA/W 等）污染 API 符号。
extern "C" int __stdcall FreeLibrary(void* hLibModule);

namespace UICornerstone {

// ============================================================
// 帧时钟（跨平台）
// ============================================================
namespace {

uint64_t nowMillis() {
    auto t = std::chrono::steady_clock::now().time_since_epoch();
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(t).count();
}

// 内存资源注册表的懒创建 + 挂载（首次 Register/Adopt 时生效）
bool ensureMemoryProvider(Impl* impl) {
    if (impl->memoryProvider) return true;
    if (!impl->callbacks || !impl->callbacks->createMemoryResourceProvider) return false;
    impl->memoryProvider = impl->callbacks->createMemoryResourceProvider();
    if (!impl->memoryProvider) return false;
    if (impl->instance && impl->callbacks->setResourceProvider)
        impl->callbacks->setResourceProvider(impl->instance, impl->memoryProvider);
    return true;
}

void releaseMemoryProvider(Impl* impl) {
    if (impl->callbacks && impl->callbacks->destroyResourceProvider && impl->memoryProvider) {
        impl->callbacks->destroyResourceProvider(impl->memoryProvider);
    }
    impl->memoryProvider = nullptr;
}

// std::function 无法直接作 C 函数指针：全局 trampoline（rawPtr 唯一，进程内单线程用例）
std::unordered_map<const void*, std::function<void(void*)>> g_adoptFreeFns;
void adoptFreeTrampoline(void* p) {
    auto it = g_adoptFreeFns.find(p);
    if (it == g_adoptFreeFns.end()) return;
    auto fn = std::move(it->second);
    g_adoptFreeFns.erase(it);
    fn(p);
}

} // namespace

// ============================================================
// 生命周期
// ============================================================
UICornerstone::UICornerstone(UIInstance instance, bool ownsInstance)
    : m_impl(new Impl())
{
    m_impl->instance = instance;
    m_impl->ownsInstance = ownsInstance;
    m_impl->initialized = (instance != nullptr);
    m_impl->resourceRoot = m_impl->config.resourceRoot;
}

UICornerstone::~UICornerstone() {
    if (m_impl && m_impl->ownsInstance && m_impl->instance)
        Dyn::API().fnDestroyInstance(m_impl->instance);
    if (m_impl) releaseMemoryProvider(m_impl.get());
    if (m_impl && m_impl->dllHandle) {   // 后端 DLL 句柄（实例级生命周期）
#ifdef _WIN32
        FreeLibrary(m_impl->dllHandle);
#endif
        m_impl->dllHandle = nullptr;
    }
}

UIInstance UICornerstone::Handle() const { return m_impl ? m_impl->instance : nullptr; }

std::unique_ptr<UICornerstone> UICornerstone::Create(const Config& config) {
    if (config.backend.empty()) return nullptr;

    // 纯动态加载模式：核心 DLL + 后端 DLL 全部经 LoadLibrary 显式加载
    // 核心 DLL：coreLibraryDir 非空 → 目录 + "UICornerstone.dll"；空 → 系统搜索（exe 同目录）
    std::string coreDll = config.coreLibraryDir.empty()
        ? std::string("UICornerstone.dll")
        : (config.coreLibraryDir + "/UICornerstone.dll");
    if (!Dyn::LoadCore(coreDll.c_str())) return nullptr;

    UIInstanceConfig cfg = UI_INSTANCE_CONFIG_DEFAULT;
    cfg.resourceRoot  = config.resourceRoot.c_str();
    cfg.windowTitle   = config.windowTitle.c_str();
    cfg.windowWidth   = config.windowWidth;
    cfg.windowHeight  = config.windowHeight;
    cfg.windowFlags   = config.windowFlags;

    // 后端插件 DLL：backendSearchPath 非空 → 指定路径；否则系统搜索（exe 同目录）
    auto [callbacks, h] = Dyn::LoadBackend(config.backendSearchPath, config.backend);
    if (!callbacks) return nullptr;

    UIInstance instance = Dyn::API().fnCreateInstance(callbacks, &cfg);
    if (!instance) {
#ifdef _WIN32
        FreeLibrary(h);
#endif
        return nullptr;
    }

    auto ui = std::unique_ptr<UICornerstone>(new UICornerstone(instance, true));
    ui->m_impl->config = config;
    ui->m_impl->resourceRoot = config.resourceRoot;
    ui->m_impl->callbacks = callbacks;
    ui->m_impl->dllHandle = h;
    return ui;
}

std::unique_ptr<UICornerstone> UICornerstone::Create(const UIBackendCallbacks* callbacks,
                                                     const Config& config) {
    if (!callbacks) return nullptr;
    std::string coreDll = config.coreLibraryDir.empty()
        ? std::string("UICornerstone.dll")
        : (config.coreLibraryDir + "/UICornerstone.dll");
    if (!Dyn::LoadCore(coreDll.c_str())) return nullptr;

    UIInstanceConfig cfg = UI_INSTANCE_CONFIG_DEFAULT;
    cfg.resourceRoot  = config.resourceRoot.c_str();
    cfg.windowTitle   = config.windowTitle.c_str();
    cfg.windowWidth   = config.windowWidth;
    cfg.windowHeight  = config.windowHeight;
    cfg.windowFlags   = config.windowFlags;

    UIInstance instance = Dyn::API().fnCreateInstance(callbacks, &cfg);
    if (!instance) return nullptr;

    auto ui = std::unique_ptr<UICornerstone>(new UICornerstone(instance, true));
    ui->m_impl->config = config;
    ui->m_impl->resourceRoot = config.resourceRoot;
    ui->m_impl->callbacks = callbacks;
    return ui;
}

std::unique_ptr<UICornerstone> UICornerstone::CreateViewport(float x, float y, float w, float h) {
    if (!m_impl->instance) return nullptr;
    UIRect rect{x, y, w, h};
    UIInstance vp = Dyn::API().fnCreateViewport(m_impl->instance, rect);
    if (!vp) return nullptr;
    // 视口共享 owner 后端：析构时不 DestroyInstance（owner 级联销毁）
    auto ui = std::unique_ptr<UICornerstone>(new UICornerstone(vp, false));
    ui->m_impl->config = m_impl->config;
    ui->m_impl->resourceRoot = m_impl->resourceRoot;
    return ui;
}

// ============================================================
// 双模式循环
// ============================================================
int UICornerstone::Run(FrameCallback update, RenderCallback onRender) {
    if (!m_impl->initialized) return 1;

    uint64_t lastTicks = nowMillis();
    while (!IsQuitRequested()) {
        ProcessEvents();

        uint64_t now = nowMillis();
        double dt = (now - lastTicks) / 1000.0;
        lastTicks = now;
        dt = std::min(dt, 0.1);   // 防止长时间挂起后的 dt 暴增

        Update(dt);
        if (update) update(dt);

        Clear();
        Render();
        if (onRender) onRender();
        Present();
    }
    Shutdown();
    return 0;
}

bool UICornerstone::ProcessEvents() { return m_impl->instance ? (Dyn::API().fnProcessEvents(m_impl->instance) != 0) : false; }
void UICornerstone::Update(double deltaTime) { if (m_impl->instance) Dyn::API().fnUpdate(m_impl->instance, deltaTime); }
void UICornerstone::Render() { if (m_impl->instance) Dyn::API().fnRender(m_impl->instance); }
void UICornerstone::Clear() { if (m_impl->instance) Dyn::API().fnClear(m_impl->instance); }
void UICornerstone::Present() { if (m_impl->instance) Dyn::API().fnPresent(m_impl->instance); }
bool UICornerstone::IsQuitRequested() const { return m_impl->instance && Dyn::API().fnIsQuitRequested(m_impl->instance); }
uint32_t UICornerstone::GetBackendCapabilities() const {
    return m_impl->instance && Dyn::API().fnGetBackendCapabilities
        ? Dyn::API().fnGetBackendCapabilities(m_impl->instance) : 0;
}

// ============================================================
// 运行期窗口 API
// ============================================================
bool UICornerstone::GetWindowSize(float& w, float& h) const {
    w = 0.0f; h = 0.0f;
    if (!m_impl->instance || !Dyn::API().fnGetWindowSize) return false;
    return Dyn::API().fnGetWindowSize(m_impl->instance, &w, &h) != 0;
}

bool UICornerstone::SetWindowSize(float w, float h) {
    if (!m_impl->instance || !Dyn::API().fnSetWindowSize) return false;
    return Dyn::API().fnSetWindowSize(m_impl->instance, w, h) != 0;
}

void* UICornerstone::GetNativeWindowHandle() const {
    if (!m_impl->instance || !Dyn::API().fnGetNativeWindowHandle) return nullptr;
    return Dyn::API().fnGetNativeWindowHandle(m_impl->instance);
}

static void WindowResizeThunk(int width, int height, void* userData) {
    auto* cb = static_cast<UICornerstone::WindowResizeCallback*>(userData);
    if (cb) (*cb)(width, height);
}

void UICornerstone::SetWindowResizeCallback(WindowResizeCallback callback) {
    if (!m_impl->instance || !Dyn::API().fnSetWindowResizeCallback) return;
    if (callback) {
        m_impl->windowResize = std::make_shared<WindowResizeCallback>(std::move(callback));
        Dyn::API().fnSetWindowResizeCallback(
            m_impl->instance, &WindowResizeThunk, m_impl->windowResize.get());
    } else {
        m_impl->windowResize = nullptr;
        Dyn::API().fnSetWindowResizeCallback(m_impl->instance, nullptr, nullptr);
    }
}

void UICornerstone::Shutdown() {
    if (m_impl->instance) {
        if (m_impl->ownsInstance) Dyn::API().fnDestroyInstance(m_impl->instance);
        m_impl->instance = nullptr;
        m_impl->initialized = false;
    }
}

// ============================================================
// 资源路径
// ============================================================
void UICornerstone::SetResourceRoot(const std::string& path) { m_impl->resourceRoot = path; }
std::string UICornerstone::GetResourceRoot() const { return m_impl->resourceRoot; }
std::string UICornerstone::ResolveResource(const std::string& relativePath) const {
    return m_impl->resourceRoot + "/" + relativePath;
}

bool UICornerstone::RegisterResource(const std::string& name, const void* data, size_t len) {
    if (!m_impl || !m_impl->instance || name.empty() || !data || len == 0) return false;
    if (!ensureMemoryProvider(m_impl.get())) return false;
    return m_impl->callbacks->memoryProviderRegister(m_impl->memoryProvider, name.c_str(),
                                                     data, (int)len) != 0;
}

bool UICornerstone::AdoptResource(const std::string& name, void* data, size_t len,
                                  std::function<void(void*)> freeFn) {
    if (!m_impl || !m_impl->instance || name.empty() || !data || len == 0) return false;
    if (!ensureMemoryProvider(m_impl.get())) return false;
    if (freeFn) g_adoptFreeFns[data] = std::move(freeFn);
    int rc = m_impl->callbacks->memoryProviderAdopt(m_impl->memoryProvider, name.c_str(),
                                                    data, (int)len,
                                                    g_adoptFreeFns.count(data) ? &adoptFreeTrampoline : nullptr);
    if (!rc) g_adoptFreeFns.erase(data);
    return rc != 0;
}

// ============================================================
// 布局
// ============================================================
bool UICornerstone::LoadLayout(const std::string& jsonContent) {
    return m_impl->instance && Dyn::API().fnLoadLayout(m_impl->instance, jsonContent.c_str()) != 0;
}
bool UICornerstone::LoadLayoutFromFile(const std::string& filePath) {
    return m_impl->instance && Dyn::API().fnLoadLayoutFromFile(m_impl->instance, filePath.c_str()) != 0;
}
Control UICornerstone::FindControl(const std::string& id) {
    if (!m_impl->instance) return Control();
    return MakeControl(Dyn::API().fnFindControl(m_impl->instance, id.c_str()));
}

bool UICornerstone::SetControlId(Control ctl, const std::string& id) {
    if (!m_impl->instance) return false;
    UIControlHandle h = ctl.IsValid() ? ctl.Handle() : nullptr;
    if (!h) return false;
    return Dyn::API().fnSetControlId(m_impl->instance, h, id.c_str()) != 0;
}

Control UICornerstone::FromHandle(UIControlHandle handle) {
    if (!m_impl->instance) return Control();
    return MakeControl(handle);
}

// ============================================================
// Control 生命周期注册
// ============================================================
Control UICornerstone::MakeControl(UIControlHandle h) {
    if (!h) return Control();
    auto it = m_impl->liveControls.find(h);
    if (it != m_impl->liveControls.end()) {
        if (auto state = it->second.lock()) return Control(std::move(state));
        m_impl->liveControls.erase(it);   // 弱引用已死，重注册
    }
    auto state = std::make_shared<ControlState>();
    state->instance = m_impl->instance;
    state->handle = h;
    state->ownerImpl = m_impl.get();
    m_impl->liveControls[h] = state;
    return Control(std::move(state));
}

// ============================================================
// 控件工厂
// ============================================================
#define UI_FACTORY(name, argdecl, ...) \
    Control UICornerstone::name argdecl { \
        if (!m_impl->instance) return Control(); \
        return MakeControl(Dyn::API().fn##name(m_impl->instance, __VA_ARGS__)); \
    }

UI_FACTORY(CreateButton,
    (const std::string& text, float x, float y, float w, float h, float xScale, float yScale),
    text.c_str(), x, y, w, h, xScale, yScale)
UI_FACTORY(CreateLabel,
    (const std::string& text, float fontSize, float x, float y, float w, float h, float xScale, float yScale),
    text.c_str(), fontSize, x, y, w, h, xScale, yScale)
UI_FACTORY(CreateCheckBox,
    (const std::string& text, float x, float y, float w, float h, float xScale, float yScale),
    text.c_str(), x, y, w, h, xScale, yScale)
UI_FACTORY(CreateEditBox,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)
UI_FACTORY(CreateProgressBar,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)
UI_FACTORY(CreateSlider,
    (float x, float y, float w, float h, float min, float max, float value, float xScale, float yScale),
    x, y, w, h, min, max, value, xScale, yScale)
UI_FACTORY(CreatePanel,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)
UI_FACTORY(CreateTextArea,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)
UI_FACTORY(CreateWinFrame,
    (const std::string& title, float x, float y, float w, float h, float xScale, float yScale),
    title.c_str(), x, y, w, h, xScale, yScale)
UI_FACTORY(CreateComboBox,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)
UI_FACTORY(CreateColorPicker,
    (float x, float y, float w, float h, const std::string& color, float xScale, float yScale),
    x, y, w, h, color.c_str(), xScale, yScale)
UI_FACTORY(CreateNumericUpDown,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)
UI_FACTORY(CreateSplitter,
    (float x, float y, float w, float h, int orientation, float xScale, float yScale),
    x, y, w, h, orientation, xScale, yScale)
UI_FACTORY(CreateImage,
    (const std::string& image, float x, float y, float w, float h, float xScale, float yScale),
    image.c_str(), x, y, w, h, xScale, yScale)
UI_FACTORY(CreateAnimation,
    (const std::string& jsoncPath, float x, float y, float w, float h, float xScale, float yScale),
    jsoncPath.c_str(), x, y, w, h, xScale, yScale)
UI_FACTORY(CreateAnimatedButton,
    (const std::string& jsoncPath, float x, float y, float w, float h, float xScale, float yScale),
    jsoncPath.c_str(), x, y, w, h, xScale, yScale)
UI_FACTORY(CreateDialog,
    (const std::string& confirmText, const std::string& cancelText, float x, float y, float w, float h,
     float xScale, float yScale),
    confirmText.c_str(), cancelText.c_str(), x, y, w, h, xScale, yScale)
UI_FACTORY(CreatePopup,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)
UI_FACTORY(CreateConfirmPopup,
    (const std::string& confirmText, float x, float y, float w, float h, float xScale, float yScale),
    confirmText.c_str(), x, y, w, h, xScale, yScale)
UI_FACTORY(CreateMenuBar,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)
UI_FACTORY(CreateScrollBar,
    (float x, float y, float w, float h, int orientation, float xScale, float yScale),
    x, y, w, h, orientation, xScale, yScale)
UI_FACTORY(CreateTreeView,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)
UI_FACTORY(CreateShape,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)

UI_FACTORY(CreateContextMenu,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)

UI_FACTORY(CreateStatusBar,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)

UI_FACTORY(CreateTabControl,
    (float x, float y, float w, float h, float xScale, float yScale),
    x, y, w, h, xScale, yScale)

// ── StatusBar 段操作 ──
bool UICornerstone::StatusBarAddItem(Control& bar, const std::string& id, const std::string& text, bool rightAlign) {
    if (!m_impl->instance || !bar.Handle()) return false;
    return Dyn::API().fnStatusBarAddItem(m_impl->instance, bar.Handle(), id.c_str(), text.c_str(), rightAlign ? 1 : 0) != 0;
}
bool UICornerstone::StatusBarSetItemText(Control& bar, const std::string& id, const std::string& text) {
    if (!m_impl->instance || !bar.Handle()) return false;
    return Dyn::API().fnStatusBarSetItemText(m_impl->instance, bar.Handle(), id.c_str(), text.c_str()) != 0;
}
bool UICornerstone::StatusBarRemoveItem(Control& bar, const std::string& id) {
    if (!m_impl->instance || !bar.Handle()) return false;
    return Dyn::API().fnStatusBarRemoveItem(m_impl->instance, bar.Handle(), id.c_str()) != 0;
}
bool UICornerstone::StatusBarSetItemIcon(Control& bar, const std::string& id, Control& iconControl) {
    if (!m_impl->instance || !bar.Handle()) return false;
    return Dyn::API().fnStatusBarSetItemIcon(m_impl->instance, bar.Handle(), id.c_str(), iconControl.Handle()) != 0;
}
bool UICornerstone::StatusBarSetItemMenu(Control& bar, const std::string& id, Control& menuPanel) {
    if (!m_impl->instance || !bar.Handle()) return false;
    return Dyn::API().fnStatusBarSetItemMenu(m_impl->instance, bar.Handle(), id.c_str(), menuPanel.Handle()) != 0;
}

// ── ContextMenu 组装/显示 ──
bool UICornerstone::ContextMenuAddItem(Control& menu, const std::string& caption, const std::string& shortcut) {
    if (!m_impl->instance || !menu.Handle()) return false;
    return Dyn::API().fnContextMenuAddItem(m_impl->instance, menu.Handle(), caption.c_str(), shortcut.c_str()) != 0;
}
bool UICornerstone::ContextMenuAddSeparator(Control& menu) {
    if (!m_impl->instance || !menu.Handle()) return false;
    return Dyn::API().fnContextMenuAddSeparator(m_impl->instance, menu.Handle()) != 0;
}
bool UICornerstone::ContextMenuShow(Control& menu, float x, float y) {
    if (!m_impl->instance || !menu.Handle()) return false;
    return Dyn::API().fnContextMenuShow(m_impl->instance, menu.Handle(), x, y) != 0;
}
bool UICornerstone::ContextMenuClose(Control& menu) {
    if (!m_impl->instance || !menu.Handle()) return false;
    return Dyn::API().fnContextMenuClose(m_impl->instance, menu.Handle()) != 0;
}

// ── TabControl 页操作 ──
int UICornerstone::TabAddPage(Control& tab, const std::string& title) {
    if (!m_impl->instance || !tab.Handle()) return -1;
    return Dyn::API().fnTabAddPage(m_impl->instance, tab.Handle(), title.c_str());
}
bool UICornerstone::TabSetPage(Control& tab, int index, Control& page) {
    if (!m_impl->instance || !tab.Handle()) return false;
    return Dyn::API().fnTabSetPage(m_impl->instance, tab.Handle(), index, page.Handle()) != 0;
}
bool UICornerstone::TabSetTitle(Control& tab, int index, const std::string& title) {
    if (!m_impl->instance || !tab.Handle()) return false;
    return Dyn::API().fnTabSetTitle(m_impl->instance, tab.Handle(), index, title.c_str()) != 0;
}
bool UICornerstone::TabSetTabLeadingControl(Control& tab, int index, Control& ctl) {
    if (!m_impl->instance || !tab.Handle()) return false;
    return Dyn::API().fnTabSetTabLeadingControl(m_impl->instance, tab.Handle(), index, ctl.Handle()) != 0;
}

// ── Shape 专用（点集与坐标映射） ──
void UICornerstone::ShapeSetPoints(Control& sh, const std::vector<std::pair<float, float>>& pts) {
    if (!m_impl->instance || !sh.Handle() || pts.empty()) return;
    std::vector<float> xs(pts.size()), ys(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) { xs[i] = pts[i].first; ys[i] = pts[i].second; }
    Dyn::API().fnShapeSetPoints(m_impl->instance, sh.Handle(),
                                static_cast<int>(pts.size()), xs.data(), ys.data());
}
std::pair<float, float> UICornerstone::ShapeMapToDrawPoint(Control& sh, float lx, float ly) {
    if (m_impl->instance && sh.Handle()) {
        float gx = 0.f, gy = 0.f;
        if (Dyn::API().fnShapeMapToDrawPoint(m_impl->instance, sh.Handle(), lx, ly, &gx, &gy))
            return {gx, gy};
    }
    return {0.f, 0.f};
}

// ── Shape 多图元（组合图形）──
int UICornerstone::ShapeAddPrimitive(Control& sh, const std::string& type,
                                     float x, float y, float w, float h) {
    if (!m_impl->instance || !sh.Handle()) return -1;
    return Dyn::API().fnShapeAddPrimitive(m_impl->instance, sh.Handle(),
                                          type.c_str(), x, y, w, h);
}
void UICornerstone::ShapeClearPrimitives(Control& sh) {
    if (!m_impl->instance || !sh.Handle()) return;
    Dyn::API().fnShapeClearPrimitives(m_impl->instance, sh.Handle());
}
void UICornerstone::ShapeSetPrimitiveColor(Control& sh, int idx, const char* prop, UIColor value) {
    if (!m_impl->instance || !sh.Handle() || !prop) return;
    Dyn::API().fnShapeSetPrimitiveColor(m_impl->instance, sh.Handle(), idx, prop, value);
}
void UICornerstone::ShapeSetPrimitiveFloat(Control& sh, int idx, const char* prop, float value) {
    if (!m_impl->instance || !sh.Handle() || !prop) return;
    Dyn::API().fnShapeSetPrimitiveFloat(m_impl->instance, sh.Handle(), idx, prop, value);
}
void UICornerstone::ShapeSetPrimitivePoints(Control& sh, int idx,
                                            const std::vector<std::pair<float,float>>& pts) {
    if (!m_impl->instance || !sh.Handle() || pts.empty()) return;
    std::vector<float> xs(pts.size()), ys(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) { xs[i] = pts[i].first; ys[i] = pts[i].second; }
    Dyn::API().fnShapeSetPrimitivePoints(m_impl->instance, sh.Handle(), idx,
                                         static_cast<int>(pts.size()), xs.data(), ys.data());
}

// ── Image/Actor 源矩形 ──
void UICornerstone::ImageSetSourceRect(Control& img, float x, float y, float w, float h) {
    if (!m_impl->instance || !img.Handle()) return;
    Dyn::API().fnActorSetSourceRect(m_impl->instance, img.Handle(), x, y, w, h);
}

Control UICornerstone::CreateMenuPanel(float xScale, float yScale) {
    if (!m_impl->instance) return Control();
    return MakeControl(Dyn::API().fnCreateMenuPanel(m_impl->instance, xScale, yScale));
}
Control UICornerstone::CreateMenuItem(const std::string& caption, int type,
    float xScale, float yScale) {
    if (!m_impl->instance) return Control();
    return MakeControl(Dyn::API().fnCreateMenuItem(m_impl->instance, caption.c_str(), type, xScale, yScale));
}
Control UICornerstone::CreateHandleControl(Control target, float x, float y, float w, float h,
    float xScale, float yScale) {
    if (!m_impl->instance) return Control();
    return MakeControl(Dyn::API().fnCreateHandleControl(m_impl->instance,
        target.Handle(), x, y, w, h, xScale, yScale));
}

bool UICornerstone::SetHandleTarget(Control handle, Control target) {
    if (!m_impl->instance) return false;
    return Dyn::API().fnSetHandleTarget(m_impl->instance, handle.Handle(), target.Handle()) != 0;
}

bool UICornerstone::HandleHitTest(Control handle, float x, float y, int& outHandleType) {
    if (!m_impl->instance) return false;
    return Dyn::API().fnHandleHitTest(m_impl->instance, handle.Handle(), x, y, &outHandleType) != 0;
}

bool UICornerstone::SetHandleRectFilter(Control handle, UIHandleRectFilter filter, void* userData) {
    if (!m_impl->instance) return false;
    return Dyn::API().fnSetHandleRectFilter(m_impl->instance, handle.Handle(), filter, userData) != 0;
}

bool UICornerstone::SetHandleMoveVisible(Control handle, bool show) {
    if (!m_impl->instance) return false;
    return Dyn::API().fnSetHandleMoveVisible(m_impl->instance, handle.Handle(), show ? 1 : 0) != 0;
}

void UICornerstone::MenuBarAddMenu(Control& bar, const std::string& caption, Control& panel) {
    if (m_impl->instance && bar.Handle() && panel.Handle())
        Dyn::API().fnMenuBarAddMenu(m_impl->instance, bar.Handle(), caption.c_str(), panel.Handle());
}

// ── TreeView 节点操作 ──
bool UICornerstone::TreeViewAddNode(Control& tree, const std::string& parentId,
                                    const std::string& id, const std::string& label, bool expanded) {
    if (m_impl->instance && tree.Handle())
        return Dyn::API().fnTreeViewAddNode(m_impl->instance, tree.Handle(),
            parentId.c_str(), id.c_str(), label.c_str(), expanded ? 1 : 0) != 0;
    return false;
}
bool UICornerstone::TreeViewRemoveNode(Control& tree, const std::string& id) {
    if (m_impl->instance && tree.Handle())
        return Dyn::API().fnTreeViewRemoveNode(m_impl->instance, tree.Handle(), id.c_str()) != 0;
    return false;
}
bool UICornerstone::TreeViewSetNodeLabel(Control& tree, const std::string& id, const std::string& label) {
    if (m_impl->instance && tree.Handle())
        return Dyn::API().fnTreeViewSetNodeLabel(m_impl->instance, tree.Handle(),
            id.c_str(), label.c_str()) != 0;
    return false;
}
bool UICornerstone::TreeViewSetNodeUserData(Control& tree, const std::string& id, void* userData) {
    if (m_impl->instance && tree.Handle())
        return Dyn::API().fnTreeViewSetNodeUserData(m_impl->instance, tree.Handle(),
            id.c_str(), userData) != 0;
    return false;
}
bool UICornerstone::TreeViewSelectNode(Control& tree, const std::string& id) {
    if (m_impl->instance && tree.Handle())
        return Dyn::API().fnTreeViewSelectNode(m_impl->instance, tree.Handle(), id.c_str()) != 0;
    return false;
}
void UICornerstone::TreeViewClearSelection(Control& tree) {
    if (m_impl->instance && tree.Handle())
        Dyn::API().fnTreeViewClearSelection(m_impl->instance, tree.Handle());
}
bool UICornerstone::TreeViewExpandNode(Control& tree, const std::string& id) {
    if (m_impl->instance && tree.Handle())
        return Dyn::API().fnTreeViewExpandNode(m_impl->instance, tree.Handle(), id.c_str()) != 0;
    return false;
}
bool UICornerstone::TreeViewCollapseNode(Control& tree, const std::string& id) {
    if (m_impl->instance && tree.Handle())
        return Dyn::API().fnTreeViewCollapseNode(m_impl->instance, tree.Handle(), id.c_str()) != 0;
    return false;
}
void UICornerstone::TreeViewExpandAll(Control& tree) {
    if (m_impl->instance && tree.Handle())
        Dyn::API().fnTreeViewExpandAll(m_impl->instance, tree.Handle());
}
void UICornerstone::TreeViewCollapseAll(Control& tree) {
    if (m_impl->instance && tree.Handle())
        Dyn::API().fnTreeViewCollapseAll(m_impl->instance, tree.Handle());
}
void UICornerstone::TreeViewClearItems(Control& tree) {
    if (m_impl->instance && tree.Handle())
        Dyn::API().fnTreeViewClearItems(m_impl->instance, tree.Handle());
}
std::string UICornerstone::TreeViewGetSelectedId(Control& tree) {
    if (m_impl->instance && tree.Handle()) {
        char buf[512];
        if (Dyn::API().fnTreeViewGetSelectedId(m_impl->instance, tree.Handle(), buf, (int)sizeof(buf)))
            return std::string(buf);
    }
    return std::string();
}

// ── EditBox / TextArea 文本操作 ──
void UICornerstone::EditBoxSelectAll(Control& ctl) {
    if (m_impl->instance && ctl.Handle()) Dyn::API().fnEditBoxSelectAll(m_impl->instance, ctl.Handle());
}
void UICornerstone::EditBoxSetSelection(Control& ctl, int start, int end) {
    if (m_impl->instance && ctl.Handle()) Dyn::API().fnEditBoxSetSelection(m_impl->instance, ctl.Handle(), start, end);
}
void UICornerstone::EditBoxClearSelection(Control& ctl) {
    if (m_impl->instance && ctl.Handle()) Dyn::API().fnEditBoxClearSelection(m_impl->instance, ctl.Handle());
}
bool UICornerstone::EditBoxHasSelection(Control& ctl) {
    if (m_impl->instance && ctl.Handle())
        return Dyn::API().fnEditBoxHasSelection(m_impl->instance, ctl.Handle()) != 0;
    return false;
}
int UICornerstone::EditBoxGetCursorPosition(Control& ctl) {
    if (m_impl->instance && ctl.Handle())
        return Dyn::API().fnEditBoxGetCursorPosition(m_impl->instance, ctl.Handle());
    return -1;
}
void UICornerstone::EditBoxCopy(Control& ctl) {
    if (m_impl->instance && ctl.Handle()) Dyn::API().fnEditBoxCopy(m_impl->instance, ctl.Handle());
}
void UICornerstone::EditBoxCut(Control& ctl) {
    if (m_impl->instance && ctl.Handle()) Dyn::API().fnEditBoxCut(m_impl->instance, ctl.Handle());
}
void UICornerstone::EditBoxPaste(Control& ctl) {
    if (m_impl->instance && ctl.Handle()) Dyn::API().fnEditBoxPaste(m_impl->instance, ctl.Handle());
}
void UICornerstone::EditBoxDeleteSelectedText(Control& ctl) {
    if (m_impl->instance && ctl.Handle()) Dyn::API().fnEditBoxDeleteSelectedText(m_impl->instance, ctl.Handle());
}

// ── NumericUpDown 数值操作 ──
void UICornerstone::NumericUpDownStep(Control& ctl, int dir) {
    if (m_impl->instance && ctl.Handle()) Dyn::API().fnNumericUpDownStep(m_impl->instance, ctl.Handle(), dir);
}

// ── ComboBox 选项操作 ──
bool UICornerstone::ComboBoxAddItem(Control& ctl, const std::string& label, const std::string& value, bool disabled) {
    if (m_impl->instance && ctl.Handle())
        return Dyn::API().fnComboBoxAddItem(m_impl->instance, ctl.Handle(), label.c_str(), value.c_str(), disabled ? 1 : 0) != 0;
    return false;
}
bool UICornerstone::ComboBoxRemoveItem(Control& ctl, int index) {
    if (m_impl->instance && ctl.Handle())
        return Dyn::API().fnComboBoxRemoveItem(m_impl->instance, ctl.Handle(), index) != 0;
    return false;
}
bool UICornerstone::ComboBoxClearItems(Control& ctl) {
    if (m_impl->instance && ctl.Handle())
        return Dyn::API().fnComboBoxClearItems(m_impl->instance, ctl.Handle()) != 0;
    return false;
}
int UICornerstone::ComboBoxGetItemCount(Control& ctl) {
    if (m_impl->instance && ctl.Handle())
        return Dyn::API().fnComboBoxGetItemCount(m_impl->instance, ctl.Handle());
    return -1;
}

// ── ListView 列表控件 ──
Control UICornerstone::CreateListView(float x, float y, float w, float h, float xScale, float yScale) {
    if (!m_impl->instance) return Control();
    return MakeControl(Dyn::API().fnCreateListView(m_impl->instance, x, y, w, h, xScale, yScale));
}
bool UICornerstone::ListViewAddRow(Control& lv, const std::string& id,
                                   const std::vector<std::string>& cells) {
    if (!m_impl->instance || !lv.Handle()) return false;
    std::vector<const char*> ptrs;
    ptrs.reserve(cells.size());
    for (auto& c : cells) ptrs.push_back(c.c_str());
    return Dyn::API().fnListViewAddRow(m_impl->instance, lv.Handle(), id.c_str(),
        static_cast<int>(ptrs.size()), ptrs.data()) != 0;
}
bool UICornerstone::ListViewRemoveRow(Control& lv, int index) {
    if (!m_impl->instance || !lv.Handle()) return false;
    return Dyn::API().fnListViewRemoveRow(m_impl->instance, lv.Handle(), index) != 0;
}
bool UICornerstone::ListViewSetCellText(Control& lv, int row, int col, const std::string& text) {
    if (!m_impl->instance || !lv.Handle()) return false;
    return Dyn::API().fnListViewSetCellText(m_impl->instance, lv.Handle(), row, col, text.c_str()) != 0;
}
std::string UICornerstone::ListViewGetCellText(Control& lv, int row, int col) {
    if (!m_impl->instance || !lv.Handle()) return {};
    char buf[512];
    if (Dyn::API().fnListViewGetCellText(m_impl->instance, lv.Handle(), row, col, buf, sizeof(buf)))
        return std::string(buf);
    return {};
}
int UICornerstone::ListViewAddColumn(Control& lv, const std::string& title, float width, bool sortable) {
    if (!m_impl->instance || !lv.Handle()) return -1;
    return Dyn::API().fnListViewAddColumn(m_impl->instance, lv.Handle(), title.c_str(), width, sortable ? 1 : 0);
}
bool UICornerstone::ListViewSetColumnWidth(Control& lv, int index, float width) {
    if (!m_impl->instance || !lv.Handle()) return false;
    return Dyn::API().fnListViewSetColumnWidth(m_impl->instance, lv.Handle(), index, width) != 0;
}
bool UICornerstone::ListViewInsertRow(Control& lv, int index, const std::string& id,
                                      const std::vector<std::string>& cells) {
    if (!m_impl->instance || !lv.Handle()) return false;
    std::vector<const char*> ptrs;
    ptrs.reserve(cells.size());
    for (auto& c : cells) ptrs.push_back(c.c_str());
    return Dyn::API().fnListViewInsertRow(m_impl->instance, lv.Handle(), index, id.c_str(),
        static_cast<int>(ptrs.size()), ptrs.data()) != 0;
}
bool UICornerstone::ListViewSetRowCells(Control& lv, int index, const std::vector<std::string>& cells) {
    if (!m_impl->instance || !lv.Handle()) return false;
    std::vector<const char*> ptrs;
    ptrs.reserve(cells.size());
    for (auto& c : cells) ptrs.push_back(c.c_str());
    return Dyn::API().fnListViewSetRowCells(m_impl->instance, lv.Handle(), index,
        static_cast<int>(ptrs.size()), ptrs.data()) != 0;
}
int UICornerstone::ListViewInsertColumn(Control& lv, int index, const std::string& title, float width, bool sortable) {
    if (!m_impl->instance || !lv.Handle()) return -1;
    return Dyn::API().fnListViewInsertColumn(m_impl->instance, lv.Handle(), index, title.c_str(), width, sortable ? 1 : 0);
}
bool UICornerstone::ListViewRemoveColumn(Control& lv, int index) {
    if (!m_impl->instance || !lv.Handle()) return false;
    return Dyn::API().fnListViewRemoveColumn(m_impl->instance, lv.Handle(), index) != 0;
}
bool UICornerstone::ListViewSetRowLeadingControl(Control& lv, int index, Control& iconControl) {
    if (!m_impl->instance || !lv.Handle()) return false;
    return Dyn::API().fnListViewSetRowLeadingControl(m_impl->instance, lv.Handle(), index, iconControl.Handle()) != 0;
}
bool UICornerstone::ListViewSetCellLeadingControl(Control& lv, int row, int col, Control& ctl) {
    if (!m_impl->instance || !lv.Handle()) return false;
    return Dyn::API().fnListViewSetCellLeadingControl(m_impl->instance, lv.Handle(), row, col, ctl.Handle()) != 0;
}
bool UICornerstone::ListViewSetCellStyle(Control& lv, int row, int col, UIColor bg, int fontSize) {
    if (!m_impl->instance || !lv.Handle()) return false;
    return Dyn::API().fnListViewSetCellStyle(m_impl->instance, lv.Handle(), row, col,
        bg.r, bg.g, bg.b, bg.a, fontSize) != 0;
}
bool UICornerstone::ListViewSetColumnHeaderStyle(Control& lv, int colIndex, UIColor color, int fontSize) {
    if (!m_impl->instance || !lv.Handle()) return false;
    return Dyn::API().fnListViewSetColumnHeaderStyle(m_impl->instance, lv.Handle(), colIndex,
        color.r, color.g, color.b, color.a, fontSize) != 0;
}
bool UICornerstone::ListViewSetColumnIcon(Control& lv, int colIndex, Control& iconControl) {
    if (!m_impl->instance || !lv.Handle()) return false;
    return Dyn::API().fnListViewSetColumnIcon(m_impl->instance, lv.Handle(), colIndex, iconControl.Handle()) != 0;
}
bool UICornerstone::ListViewSetColumnSorter(Control& lv, int colIndex, ListViewSortFn cmp, void* userData) {
    if (!m_impl->instance || !lv.Handle()) return false;
    return Dyn::API().fnListViewSetColumnSorter(m_impl->instance, lv.Handle(), colIndex, cmp, userData) != 0;
}
bool UICornerstone::ListViewSetCellShadow(Control& lv, int row, int col, UIColor color, float offsetX, float offsetY) {
    if (!m_impl->instance || !lv.Handle()) return false;
    return Dyn::API().fnListViewSetCellShadow(m_impl->instance, lv.Handle(), row, col,
        color.r, color.g, color.b, color.a, offsetX, offsetY) != 0;
}
bool UICornerstone::ListViewSetColumnValues(Control& lv, int colIndex, const std::vector<std::string>& values) {
    if (!m_impl->instance || !lv.Handle()) return false;
    std::vector<const char*> ptrs;
    ptrs.reserve(values.size());
    for (auto& v : values) ptrs.push_back(v.c_str());
    return Dyn::API().fnListViewSetColumnValues(m_impl->instance, lv.Handle(), colIndex,
        static_cast<int>(ptrs.size()), ptrs.data()) != 0;
}

// ── LuotiAni 动画操作 ──
bool UICornerstone::AnimationPrepare(Control& ctl, int startFrame) {
    if (m_impl->instance && ctl.Handle())
        return Dyn::API().fnAnimationPrepare(m_impl->instance, ctl.Handle(), startFrame) != 0;
    return false;
}
bool UICornerstone::AnimationSetFrameFilter(Control& ctl, bool bilinear) {
    if (m_impl->instance && ctl.Handle())
        return Dyn::API().fnAnimationSetFrameFilter(m_impl->instance, ctl.Handle(), bilinear ? 1 : 0) != 0;
    return false;
}

// ── 截图读回 ──
bool UICornerstone::CaptureRect(float x, float y, float w, float h, uint8_t* outPixels, int* outW, int* outH) const {
    if (m_impl->instance)
        return Dyn::API().fnCaptureRect(m_impl->instance, x, y, w, h, outPixels, outW, outH) != 0;
    return false;
}
bool UICornerstone::CaptureViewport(uint8_t* out, int* w, int* h) const {
    if (m_impl->instance)
        return Dyn::API().fnCaptureViewport(m_impl->instance, out, w, h) != 0;
    return false;
}
bool UICornerstone::CaptureBench(uint8_t* out, int* w, int* h) const {
    if (m_impl->instance)
        return Dyn::API().fnCaptureBench(m_impl->instance, out, w, h) != 0;
    return false;
}
bool UICornerstone::CaptureControl(Control& ctl, uint8_t* out, int* w, int* h) const {
    if (m_impl->instance && ctl.Handle())
        return Dyn::API().fnCaptureControl(m_impl->instance, ctl.Handle(), out, w, h) != 0;
    return false;
}
bool UICornerstone::SavePixelsToFile(const uint8_t* pixels, int w, int h, const std::string& filePath) {
    return Dyn::API().fnSavePixelsToFile(pixels, w, h, filePath.c_str()) != 0;
}

// 字体枚举清单（P0-48 扩展②）：静态，不需要实例；核心库未加载时安全降级
int UICornerstone::GetFontCount() {
    auto& api = Dyn::API();
    if (!api.fnGetFontCount) return 0;
    return api.fnGetFontCount();
}
std::string UICornerstone::GetFontName(int index) {
    auto& api = Dyn::API();
    if (!api.fnGetFontName) return std::string();
    char buf[64] = {0};
    if (!api.fnGetFontName(index, buf, static_cast<int>(sizeof(buf)))) return std::string();
    return std::string(buf);
}
std::vector<std::string> UICornerstone::GetFontNames() {
    std::vector<std::string> names;
    const int n = GetFontCount();
    if (n > 0) names.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) names.push_back(GetFontName(i));
    return names;
}
void UICornerstone::MenuPanelAddItem(Control& panel, Control& item) {
    if (m_impl->instance && panel.Handle() && item.Handle())
        Dyn::API().fnMenuPanelAddItem(m_impl->instance, panel.Handle(), item.Handle());
}
void UICornerstone::MenuPanelAddSeparator(Control& panel) {
    if (m_impl->instance && panel.Handle())
        Dyn::API().fnMenuPanelAddSeparator(m_impl->instance, panel.Handle());
}
void UICornerstone::MenuItemSetSubMenu(Control& item, Control& panel) {
    if (m_impl->instance && item.Handle() && panel.Handle())
        Dyn::API().fnMenuItemSetSubMenu(m_impl->instance, item.Handle(), panel.Handle());
}

// ============================================================
// 视口
// ============================================================
void UICornerstone::SetViewport(float x, float y, float w, float h) {
    if (m_impl->instance) Dyn::API().fnSetViewport(m_impl->instance, x, y, w, h);
}

bool UICornerstone::SetViewportBackgroundColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!m_impl->instance || !Dyn::API().fnSetViewportBackgroundColor) return false;
    return Dyn::API().fnSetViewportBackgroundColor(m_impl->instance, r, g, b, a) != 0;
}
UIRect UICornerstone::GetViewport() const {
    UIRect r{0, 0, 0, 0};
    if (m_impl->instance) Dyn::API().fnGetViewport(m_impl->instance, &r.x, &r.y, &r.w, &r.h);
    return r;
}

bool UICornerstone::SetViewportScaleMode(int mode) {
    if (!m_impl->instance || !Dyn::API().fnSetViewportScaleMode) return false;
    return Dyn::API().fnSetViewportScaleMode(m_impl->instance, mode) != 0;
}
int UICornerstone::GetViewportScaleMode() const {
    int mode = 0;
    if (m_impl->instance && Dyn::API().fnGetViewportScaleMode)
        Dyn::API().fnGetViewportScaleMode(m_impl->instance, &mode);
    return mode;
}
bool UICornerstone::SetCanvasSize(float w, float h) {
    if (!m_impl->instance || !Dyn::API().fnSetCanvasSize) return false;
    return Dyn::API().fnSetCanvasSize(m_impl->instance, w, h) != 0;
}
bool UICornerstone::GetViewportScale(float& sx, float& sy) const {
    sx = sy = 0.0f;
    if (!m_impl->instance || !Dyn::API().fnGetViewportScale) return false;
    return Dyn::API().fnGetViewportScale(m_impl->instance, &sx, &sy) != 0;
}
bool UICornerstone::SetViewportAnchor(float ax, float ay) {
    if (!m_impl->instance || !Dyn::API().fnSetViewportAnchor) return false;
    return Dyn::API().fnSetViewportAnchor(m_impl->instance, ax, ay) != 0;
}
bool UICornerstone::SetInstanceScale(float xScale, float yScale) {
    if (!m_impl->instance || !Dyn::API().fnSetInstanceScale) return false;
    return Dyn::API().fnSetInstanceScale(m_impl->instance, xScale, yScale) != 0;
}
float UICornerstone::GetInstanceScaleX() const {
    float x = 1.0f, y = 1.0f;
    if (m_impl->instance && Dyn::API().fnGetInstanceScale)
        Dyn::API().fnGetInstanceScale(m_impl->instance, &x, &y);
    return x;
}
float UICornerstone::GetInstanceScaleY() const {
    float x = 1.0f, y = 1.0f;
    if (m_impl->instance && Dyn::API().fnGetInstanceScale)
        Dyn::API().fnGetInstanceScale(m_impl->instance, &x, &y);
    return y;
}

// ============================================================
// 事件注入
// ============================================================
void UICornerstone::PushEvent(const UIEvent& event) {
    if (m_impl->instance) Dyn::API().fnPushUIEvent(m_impl->instance, &event);
}
void UICornerstone::PushMouseButton(int button, float x, float y, bool down) {
    PushEvent(Input::MouseButton(button, x, y, down));
}
void UICornerstone::PushMouseMove(float x, float y) {
    PushEvent(Input::MouseMove(x, y));
}
void UICornerstone::PushMouseWheel(float dx, float dy, float x, float y) {
    PushEvent(Input::MouseWheel(dx, dy, x, y));
}
void UICornerstone::PushKey(int keyCode, uint16_t mod, bool down) {
    PushEvent(Input::Key(keyCode, mod, down));
}
void UICornerstone::PushTextInput(const std::string& text) {
    PushEvent(Input::TextInput(text));
}

// ============================================================
// Action 注册
// ============================================================
void UICornerstone::RegisterAction(const std::string& name, ActionCallback callback) {
    if (!m_impl->instance) return;
    auto cb = std::make_shared<ActionCallback>(std::move(callback));
    m_impl->actions[name] = cb;
    Dyn::API().fnRegisterAction(m_impl->instance, name.c_str(),
        [](UIControlHandle ctl, void* userData) {
            auto& fn = *static_cast<ActionCallback*>(userData);
            if (!ctl) return;
            auto state = std::make_shared<ControlState>();
            state->instance = nullptr;  // 动作回调不绑定实例上下文（ctl 裸句柄）
            state->handle = ctl;
            fn(Control(std::move(state)));
        },
        cb.get());
}

// ============================================================
// 后端配置
// ============================================================
bool UICornerstone::SetBackendConfig(const char* key, const char* value) {
    return m_impl->instance && Dyn::API().fnSetBackendConfig(m_impl->instance, key, value) != 0;
}
bool UICornerstone::SetBackendConfigBool(const char* key, bool value) {
    return m_impl->instance && Dyn::API().fnSetBackendConfigBool(m_impl->instance, key, value ? 1 : 0) != 0;
}
bool UICornerstone::GetBackendConfigBool(const char* key, bool& out) const {
    if (!m_impl->instance) return false;
    int v = 0;
    if (!Dyn::API().fnGetBackendConfigBool(m_impl->instance, key, &v)) return false;
    out = (v != 0);
    return true;
}

// ============================================================
// 错误查询 / Debug
// ============================================================
const std::string& UICornerstone::GetLastError() const { return m_impl->lastError; }

int UICornerstone::DebugGetAliveCount() {
    auto& api = Dyn::API();
    return api.fnDebug_GetAliveCount ? api.fnDebug_GetAliveCount() : 0;
}
UIInstance UICornerstone::DebugGetAliveInstance(int index) {
    auto& api = Dyn::API();
    return api.fnDebug_GetAliveInstance ? api.fnDebug_GetAliveInstance(index) : nullptr;
}
UIInstance UICornerstone::DebugGetActiveViewport(UIInstance instance) {
    auto& api = Dyn::API();
    return api.fnDebug_GetActiveViewport ? api.fnDebug_GetActiveViewport(instance) : nullptr;
}
bool UICornerstone::DebugIsControlFocused(UIInstance instance, UIControlHandle control) {
    auto& api = Dyn::API();
    return api.fnDebug_IsControlFocused ? api.fnDebug_IsControlFocused(instance, control) != 0 : false;
}

} // namespace UICornerstone