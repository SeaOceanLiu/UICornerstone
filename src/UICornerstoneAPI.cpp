#include "UICornerstoneAPI.h"
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
#include "LayoutParser.h"
#include "PlatformUtils.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <functional>

// Forward declaration for InitFromPlugin static link fallback
extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);

// ============================================================
// 全局状态
// ============================================================
namespace {

const UIBackendCallbacks* g_callbacks = nullptr;
Window*                   g_window = nullptr;
RenderDevice*             g_renderDevice = nullptr;
InputBackend*             g_inputBackend = nullptr;
TextRenderer*             g_textRenderer = nullptr;
CallbackResourceProvider* g_resourceProvider = nullptr;
bool g_initialized = false;
bool g_quit = false;

SRect g_viewport(0, 0, 1024, 768);

std::unordered_map<std::string, std::pair<UIActionCallback, void*>> g_actions;
std::unordered_map<std::string, UIControlHandle> g_controlsById;

static void registerControlById(const std::string& id, UIControlHandle ctl) {
    if (!id.empty()) g_controlsById[id] = ctl;
}

} // anonymous namespace

// ============================================================
// UICornerstone_Init / Shutdown
// ============================================================
int UICornerstone_Init(const UIBackendCallbacks* callbacks) {
    if (g_initialized) return 1;
    if (!callbacks || callbacks->version != 1) return 0;

    g_callbacks = callbacks;
    g_quit = false;

    if (!BackendManager::instance()->initialize(callbacks)) {
        printf("UICornerstone: BackendManager::initialize(callbacks) failed\n");
        return 0;
    }

    auto* bm = BackendManager::instance();
    g_window = bm->window();
    g_renderDevice = bm->renderDevice();
    g_textRenderer = bm->textRenderer();
    g_inputBackend = bm->inputBackend();

    if (callbacks->createResourceProvider) {
        std::string rpBasePath = Platform::GetBasePath() + "assets";
        UIResourceProviderHandle rpHandle = callbacks->createResourceProvider(rpBasePath.c_str());
        g_resourceProvider = new CallbackResourceProvider(callbacks, rpHandle);
    }

    BENCH->setRenderDevice(g_renderDevice);
    BENCH->setTextRenderer(g_textRenderer);
    BENCH->setInputBackend(g_inputBackend);
    if (g_resourceProvider) BENCH->setResourceProvider(g_resourceProvider);

    if (g_window) {
        SSize sz = g_window->getSize();
        g_viewport = SRect(0, 0, sz.width, sz.height);
    }
    BENCH->resized(g_viewport);

    g_initialized = true;
    printf("UICornerstone initialized\n");
    return 1;
}

void UICornerstone_Shutdown(void) {
    if (!g_initialized) return;

    g_controlsById.clear();
    g_actions.clear();

    delete g_resourceProvider; g_resourceProvider = nullptr;

    BackendManager::instance()->shutdown();
    g_window = nullptr;
    g_renderDevice = nullptr;
    g_textRenderer = nullptr;
    g_inputBackend = nullptr;

    g_callbacks = nullptr;
    g_initialized = false;
    printf("UICornerstone shutdown\n");
}

int UICornerstone_InitFromPlugin(const char* pluginName) {
    if (!pluginName || !pluginName[0]) return 0;

    UIBackendCallbacks* callbacks = nullptr;

    // Try dynamic loading (Windows HMODULE is available via SDL3 transitively)
    char dllName[128];
    snprintf(dllName, sizeof(dllName), "UIBackend_%s.dll", pluginName);
    HMODULE dll = LoadLibraryA(dllName);
    if (dll) {
        auto getter = (UIBackendCallbacks*(*)())GetProcAddress(dll, "GetUIBackendCallbacks");
        if (getter) callbacks = getter();
        if (!callbacks) { printf("UICornerstone: %s has no GetUIBackendCallbacks\n", dllName); FreeLibrary(dll); return 0; }
        printf("UICornerstone: loaded %s\n", dllName);
    }

    // Static link fallback: backend's GetUIBackendCallbacks is linked directly
    if (!callbacks) {
        callbacks = GetUIBackendCallbacks();
    }

    if (!callbacks) { printf("UICornerstone: InitFromPlugin(%s) failed\n", pluginName); return 0; }
    return UICornerstone_Init(callbacks);
}

// ============================================================
// 视口控制
// ============================================================
void UICornerstone_SetViewport(float x, float y, float w, float h) {
    g_viewport = SRect(x, y, w, h);
    BENCH->resized(g_viewport);
}

void UICornerstone_GetViewport(float* x, float* y, float* w, float* h) {
    if (x) *x = g_viewport.left;
    if (y) *y = g_viewport.top;
    if (w) *w = g_viewport.width;
    if (h) *h = g_viewport.height;
}

// ============================================================
// 帧循环
// ============================================================
void UICornerstone_ProcessEvents(void) {
    if (!g_inputBackend) return;
    g_inputBackend->newFrame();

    Event event;
    while (g_inputBackend->pollEvent(event)) {
        switch (event.m_type) {
        case EventType::WindowClose:
            g_quit = true;
            break;
        case EventType::WindowResize:
            BENCH->resized(SRect(0, 0, (float)event.resizeEvent.width, (float)event.resizeEvent.height));
            break;
        default:
            {
                auto sharedEvent = std::make_shared<Event>(event);
                BENCH->inputControl(sharedEvent);
            }
            break;
        }
    }
}

void UICornerstone_Update(double deltaTime) {
    (void)deltaTime;
    BENCH->eventLoopEntry();
    BENCH->update();
}

void UICornerstone_Render(void) {
    // 只渲染视口区域，不做 clear / present
    // 调用者负责在外部 clear 和 present 整个帧缓冲区
    if (!g_renderDevice) return;
    g_renderDevice->setClipRect(g_viewport);
    BENCH->draw();
    g_renderDevice->clearClipRect();
}

void UICornerstone_Clear(void) {
    if (!g_renderDevice) return;
    g_renderDevice->setDrawColor(SColor(0.2f, 0.2f, 0.22f, 1.0f));
    g_renderDevice->clear();
}

void UICornerstone_Present(void) {
    if (!g_renderDevice) return;
    g_renderDevice->present();
}

int UICornerstone_IsQuitRequested(void) {
    return g_quit ? 1 : 0;
}

// ============================================================
// 布局系统
// ============================================================
int UICornerstone_LoadLayout(const char* jsonContent) {
    if (!jsonContent) return 0;

    LayoutParser parser;

    // 注册所有 g_actions 到 LayoutParser
    for (auto& [name, pair] : g_actions) {
        UIActionCallback cb = pair.first;
        void* userData = pair.second;
        parser.registerHandler(name, [cb, userData](shared_ptr<Control> ctl) {
            cb(reinterpret_cast<UIControlHandle>(ctl.get()), userData);
        });
    }

    auto root = parser.parseLayout(std::string(jsonContent));
    if (!root) return 0;

    BENCH->addControl(root);

    for (auto& mb : parser.getMenuBars()) {
        BENCH->addControl(mb);
    }

    for (auto& id : parser.getAllControlIds()) {
        auto ctl = parser.findControlById(id);
        if (ctl) g_controlsById[id] = reinterpret_cast<UIControlHandle>(ctl.get());
    }

    printf("UICornerstone: LoadLayout OK (%zu control ids, %zu menu bars)\n",
           parser.getAllControlIds().size(), parser.getMenuBars().size());
    return 1;
}

int UICornerstone_LoadLayoutFromFile(const char* filePath) {
    if (!filePath) return 0;
    if (!g_resourceProvider) {
        printf("UICornerstone: LoadLayoutFromFile requires ResourceProvider\n");
        return 0;
    }
    auto data = g_resourceProvider->readFile(filePath);
    if (!data || data->empty()) return 0;
    data->push_back('\0');
    return UICornerstone_LoadLayout(data->data());
}

UIControlHandle UICornerstone_FindControl(const char* id) {
    if (!id) return nullptr;
    auto it = g_controlsById.find(id);
    return (it != g_controlsById.end()) ? it->second : nullptr;
}

void UICornerstone_RegisterAction(const char* name, UIActionCallback cb, void* userData) {
    if (name) g_actions[name] = {cb, userData};
}

// ============================================================
// 控件工厂
// ============================================================
UIControlHandle UICornerstone_CreateButton(const char* text,
    float x, float y, float w, float h)
{
    auto* ctl = new Button(BENCH, SRect(x, y, w, h));
    if (text) ctl->setCaption(text);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl));
}

UIControlHandle UICornerstone_CreateLabel(const char* text, float fontSize,
    float x, float y, float w, float h)
{
    auto* ctl = new Label(BENCH, SRect(x, y, w, h));
    if (text) ctl->setCaption(text);
    (void)fontSize;
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl));
}

UIControlHandle UICornerstone_CreateCheckBox(const char* text,
    float x, float y, float w, float h)
{
    auto* ctl = new CheckBox(BENCH, SRect(x, y, w, h));
    ctl->createCaption();
    if (text) ctl->getCaption()->setCaption(text);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl));
}

UIControlHandle UICornerstone_CreateEditBox(
    float x, float y, float w, float h)
{
    auto* ctl = new EditBox(BENCH, SRect(x, y, w, h));
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl));
}

UIControlHandle UICornerstone_CreateProgressBar(
    float x, float y, float w, float h)
{
    auto* ctl = new ProgressBar(BENCH, SRect(x, y, w, h));
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl));
}

UIControlHandle UICornerstone_CreatePanel(
    float x, float y, float w, float h)
{
    auto* ctl = new Panel(BENCH, SRect(x, y, w, h));
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl));
}

UIControlHandle UICornerstone_CreateMenu(void) {
    printf("UICornerstone: CreateMenu not implemented yet\n");
    return nullptr;
}

// ============================================================
// 控件通用操作
// ============================================================
void UICornerstone_SetRect(UIControlHandle ctl, float x, float y, float w, float h) {
    if (ctl) static_cast<Control*>(ctl)->setRect(SRect(x, y, w, h));
}

void UICornerstone_GetRect(UIControlHandle ctl, float* x, float* y, float* w, float* h) {
    if (!ctl) return;
    SRect r = static_cast<Control*>(ctl)->getRect();
    if (x) *x = r.left;
    if (y) *y = r.top;
    if (w) *w = r.width;
    if (h) *h = r.height;
}

void UICornerstone_SetVisible(UIControlHandle ctl, int visible) {
    if (ctl) static_cast<Control*>(ctl)->setVisible(visible != 0);
}

void UICornerstone_SetEnabled(UIControlHandle ctl, int enabled) {
    if (ctl) static_cast<Control*>(ctl)->setEnable(enabled != 0);
}

void UICornerstone_SetText(UIControlHandle ctl, const char* text) {
    if (!ctl) return;
    const std::string s = text ? text : "";
    if (auto* btn = dynamic_cast<Button*>(static_cast<Control*>(ctl))) {
        btn->setCaption(s);
    } else if (auto* lbl = dynamic_cast<Label*>(static_cast<Control*>(ctl))) {
        lbl->setCaption(s);
    } else if (auto* cb = dynamic_cast<CheckBox*>(static_cast<Control*>(ctl))) {
        auto caption = cb->getCaption();
        if (caption) caption->setCaption(s);
    }
}

void UICornerstone_AddChild(UIControlHandle parent, UIControlHandle child) {
    (void)parent;
    (void)child;
    printf("UICornerstone: AddChild not implemented\n");
}

void UICornerstone_SetOnClick(UIControlHandle ctl, UIActionCallback cb, void* userData) {
    if (!ctl) return;
    auto* btn = dynamic_cast<Button*>(static_cast<Control*>(ctl));
    if (btn) {
        btn->setOnClick([cb, userData](std::shared_ptr<Button>) {
            if (cb) cb(nullptr, userData);
        });
    }
}
