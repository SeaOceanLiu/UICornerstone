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
#include "TextArea.h"
#include "Slider.h"
#include "ColorPicker.h"
#include "ComboBox.h"
#include "NumericUpDown.h"
#include "Dialog.h"
#include "WinFrame.h"
#include "LayoutParser.h"
#include "PlatformUtils.h"
#include "Actor.h"
#include "LuotiAni.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <functional>
#include <queue>
#include <vector>
#include <algorithm>

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
std::queue<UIEvent> g_queuedEvents;

// 保持 Dialog/Popup 生命期：CreateDialog 中创建后立即加入，
// close() 的 onClose 回调中自动清理。
static std::vector<std::shared_ptr<Popup>> g_popupPool;

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

    // Perform a dummy clear+present to ensure the OpenGL context is active
    // for subsequent texture creation (relevant for OpenGL-based backends).
    // Without this, textures created during init have no valid GPU data.
    if (g_renderDevice) {
        g_renderDevice->setDrawColor(SColor(0, 0, 0, 0));
        g_renderDevice->clear();
        g_renderDevice->present();
    }

    g_initialized = true;
    return 1;
}

void UICornerstone_Shutdown(void) {
    if (!g_initialized) return;

    g_controlsById.clear();
    g_actions.clear();

    // 清理所有 Dialog/Popup，确保在 BackendManager shutdown 前析构
    g_popupPool.clear();

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

#if !UICORNERSTONE_BUILD_SHARED
extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);
#endif

int UICornerstone_InitFromPlugin(const char* pluginName) {
    if (!pluginName || !pluginName[0]) return 0;

    char dllName[128];
    snprintf(dllName, sizeof(dllName), "UIBackend_%s.dll", pluginName);
    HMODULE dll = LoadLibraryA(dllName);
    if (!dll) {
#if !UICORNERSTONE_BUILD_SHARED
        printf("UICornerstone: InitFromPlugin(%s) — LoadLibrary failed, trying static...\n", pluginName);
        UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
        if (callbacks) {
            printf("UICornerstone: static GetUIBackendCallbacks ready\n");
            return UICornerstone_Init(callbacks);
        }
#endif
        printf("UICornerstone: InitFromPlugin(%s) — LoadLibrary failed\n", pluginName);
        return 0;
    }

    auto getter = (UIBackendCallbacks*(*)())GetProcAddress(dll, "GetUIBackendCallbacks");
    if (!getter) {
        printf("UICornerstone: %s has no GetUIBackendCallbacks\n", dllName);
        FreeLibrary(dll);
        return 0;
    }

    UIBackendCallbacks* callbacks = getter();
    if (!callbacks) {
        printf("UICornerstone: %s GetUIBackendCallbacks returned null\n", dllName);
        FreeLibrary(dll);
        return 0;
    }

    printf("UICornerstone: loaded %s\n", dllName);
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

void UICornerstone_PushUIEvent(const UIEvent* ue) {
    if (ue) g_queuedEvents.push(*ue);
}

void UICornerstone_ProcessEvents(void) {
    // 处理外部注入的队列事件
    while (!g_queuedEvents.empty()) {
        UIEvent ue = g_queuedEvents.front();
        g_queuedEvents.pop();

        Event event;
        if (!uiEventToEvent(ue, event)) continue;

        if (event.m_type == EventType::WindowClose) {
            g_quit = true;
        } else if (event.m_type == EventType::WindowResize) {
            BENCH->resized(SRect(0, 0, (float)event.resizeEvent.width, (float)event.resizeEvent.height));
        } else {
            auto sharedEvent = std::make_shared<Event>(event);
            BENCH->inputControl(sharedEvent);
        }
    }

    // 后备：从 InputBackend 轮询事件（非回调模式使用）
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

    // 将 JSON 定义的 Dialog 加入 g_popupPool 保持生命期
    for (auto& pop : parser.getDialogs()) {
        g_popupPool.push_back(pop);
    }

    for (auto& id : parser.getAllControlIds()) {
        auto ctl = parser.findControlById(id);
        if (ctl) g_controlsById[id] = reinterpret_cast<UIControlHandle>(ctl.get());
    }

    printf("UICornerstone: LoadLayout OK (%zu control ids, %zu menu bars, %zu dialogs)\n",
           parser.getAllControlIds().size(), parser.getMenuBars().size(),
           parser.getDialogs().size());
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
    auto ctl = std::make_shared<Button>(BENCH, SRect(x, y, w, h));
    if (text) ctl->setCaption(text);
    BENCH->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateLabel(const char* text, float fontSize,
    float x, float y, float w, float h)
{
    auto ctl = std::make_shared<Label>(BENCH, SRect(x, y, w, h));
    if (text) ctl->setCaption(text);
    ctl->setFont(FontName::HarmonyOS_Sans_SC_Regular);
    if (fontSize > 0) ctl->setFontSize((int)fontSize);
    BENCH->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateCheckBox(const char* text,
    float x, float y, float w, float h)
{
    auto ctl = std::make_shared<CheckBox>(BENCH, SRect(x, y, w, h));
    ctl->createCaption();
    if (text) ctl->getCaption()->setCaption(text);
    BENCH->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateEditBox(
    float x, float y, float w, float h)
{
    auto ctl = std::make_shared<EditBox>(BENCH, SRect(x, y, w, h));
    BENCH->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateProgressBar(
    float x, float y, float w, float h)
{
    auto ctl = std::make_shared<ProgressBar>(BENCH, SRect(x, y, w, h));
    BENCH->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateSlider(
    float x, float y, float w, float h, float min, float max, float value)
{
    auto ctl = std::make_shared<Slider>(BENCH, SRect(x, y, w, h));
    ctl->setRange(min, max);
    ctl->setValue(value);
    BENCH->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreatePanel(
    float x, float y, float w, float h)
{
    auto ctl = std::make_shared<Panel>(BENCH, SRect(x, y, w, h));
    BENCH->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateTextArea(
    float x, float y, float w, float h)
{
    auto ctl = std::make_shared<TextArea>(BENCH, SRect(x, y, w, h));
    BENCH->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateWinFrame(
    const char* title, float x, float y, float w, float h)
{
    auto ctl = std::make_shared<WinFrame>(BENCH, SRect(x, y, w, h));
    if (title) ctl->setTitle(title);
    ctl->setTitleTextColor(SColor(0, 0, 0, 255));
    BENCH->addControl(ctl);
    ctl->create();
    ctl->show();
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

UIControlHandle UICornerstone_CreateMenu(void) {
    printf("UICornerstone: CreateMenu not implemented yet\n");
    return nullptr;
}

UIControlHandle UICornerstone_CreateImageButton(
    const char* normalImage, const char* hoverImage, const char* pressedImage,
    float x, float y, float w, float h)
{
    auto ctl = std::make_shared<Button>(BENCH, SRect(x, y, w, h));
    if (normalImage) {
        auto actor = std::make_shared<Actor>(ctl.get(), fs::path(normalImage), true);
        ctl->setNormalStateActor(actor);
    }
    if (hoverImage) {
        auto actor = std::make_shared<Actor>(ctl.get(), fs::path(hoverImage), true);
        ctl->setHoverStateActor(actor);
    }
    if (pressedImage) {
        auto actor = std::make_shared<Actor>(ctl.get(), fs::path(pressedImage), true);
        ctl->setPressedStateActor(actor);
    }
    BENCH->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

void UICornerstone_SetButtonAnimation(UIControlHandle btn, const char* jsoncPath) {
    if (!btn || !jsoncPath) return;
    auto* b = dynamic_cast<Button*>(static_cast<Control*>(btn));
    if (!b) {
        printf("UICornerstone_SetButtonAnimation: handle is not a Button\n");
        return;
    }
    fs::path p(jsoncPath);
    if (p.is_relative()) {
        p = fs::path(Platform::GetBasePath()) / p;
    }
    auto luotiAni = std::make_shared<LuotiAni>(b);
    luotiAni->loadFromFile(p);
    b->setLuotiAni(luotiAni);
    luotiAni->prepare();
    luotiAni->play();
    printf("UICornerstone_SetButtonAnimation: OK\n");
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
    } else if (auto* eb = dynamic_cast<EditBox*>(static_cast<Control*>(ctl))) {
        eb->setText(s);
    }
}

void UICornerstone_AddChild(UIControlHandle parent, UIControlHandle child) {
    if (!parent || !child) return;
    auto* ctlImpl = dynamic_cast<ControlImpl*>(static_cast<Control*>(child));
    auto* panel = dynamic_cast<Panel*>(static_cast<Control*>(parent));
    if (!ctlImpl || !panel) return;
    auto sp = ctlImpl->shared_from_this();
    BENCH->removeControl(sp);
    panel->addControl(sp);
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

void UICornerstone_SetBGColor(UIControlHandle ctl, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!ctl) return;
    SColor normal((int)r, (int)g, (int)b, (int)a);
    static_cast<Control*>(ctl)->setNormalStateBGColor(normal);
    static_cast<Control*>(ctl)->setHoverStateBGColor(normal.brighter(0.3f));
    static_cast<Control*>(ctl)->setPressedStateBGColor(normal.darker(0.3f));
}

void UICornerstone_SetProgress(UIControlHandle ctl, float value) {
    if (!ctl) return;
    auto* pb = dynamic_cast<ProgressBar*>(static_cast<Control*>(ctl));
    if (pb) pb->setValue(value);
}

void UICornerstone_SetChecked(UIControlHandle ctl, int checked) {
    if (!ctl) return;
    auto* cb = dynamic_cast<CheckBox*>(static_cast<Control*>(ctl));
    if (cb) cb->setCheckState(checked ? CheckState::Checked : CheckState::Unchecked);
}

static char g_getTextBuf[4096];

const char* UICornerstone_GetText(UIControlHandle ctl) {
    if (!ctl) return "";
    Control* c = static_cast<Control*>(ctl);
    if (auto* btn = dynamic_cast<Button*>(c)) {
        strncpy(g_getTextBuf, btn->getCaption().c_str(), sizeof(g_getTextBuf) - 1);
    } else if (auto* lbl = dynamic_cast<Label*>(c)) {
        strncpy(g_getTextBuf, lbl->getCaption().c_str(), sizeof(g_getTextBuf) - 1);
    } else if (auto* eb = dynamic_cast<EditBox*>(c)) {
        strncpy(g_getTextBuf, eb->getText().c_str(), sizeof(g_getTextBuf) - 1);
    } else {
        g_getTextBuf[0] = '\0';
    }
    g_getTextBuf[sizeof(g_getTextBuf) - 1] = '\0';
    return g_getTextBuf;
}

const char* UICornerstone_GetControlId(UIControlHandle ctl) {
    if (!ctl) return "";
    static char buf[256];
    for (const auto& pair : g_controlsById) {
        if (pair.second == ctl) {
            strncpy(buf, pair.first.c_str(), sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            return buf;
        }
    }
    buf[0] = '\0';
    return buf;
}

int UICornerstone_GetChecked(UIControlHandle ctl) {
    if (!ctl) return 0;
    auto* cb = dynamic_cast<CheckBox*>(static_cast<Control*>(ctl));
    if (!cb) return 0;
    switch (cb->getCheckState()) {
        case CheckState::Unchecked: return 0;
        case CheckState::Checked:   return 1;
        case CheckState::Indeterminate: return 2;
    }
    return 0;
}

float UICornerstone_GetProgress(UIControlHandle ctl) {
    if (!ctl) return 0.0f;
    auto* pb = dynamic_cast<ProgressBar*>(static_cast<Control*>(ctl));
    return pb ? pb->getValue() : 0.0f;
}

// ============================================================
// ColorPicker
// ============================================================
UIControlHandle UICornerstone_CreateColorPicker(
    float x, float y, float w, float h, const char* color)
{
    auto ctl = std::make_shared<ColorPicker>(BENCH, SRect(x, y, w, h));
    if (color) ctl->setColor(color);
    BENCH->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

void UICornerstone_GetColorPickerColor(UIControlHandle ctl, char* hexOut, int maxLen) {
    if (!ctl || !hexOut || maxLen <= 0) return;
    auto* cp = dynamic_cast<ColorPicker*>(static_cast<Control*>(ctl));
    if (cp) {
        std::string hex = cp->getColorHex();
        strncpy(hexOut, hex.c_str(), (size_t)maxLen - 1);
        hexOut[maxLen - 1] = '\0';
    } else {
        hexOut[0] = '\0';
    }
}

void UICornerstone_SetOnColorChanged(UIControlHandle ctl, UIActionCallback cb, void* userData) {
    if (!ctl) return;
    auto* cp = dynamic_cast<ColorPicker*>(static_cast<Control*>(ctl));
    if (cp) {
        cp->setOnColorChanged([cb, userData](std::shared_ptr<ColorPicker>, const SColor&) {
            if (cb) cb(nullptr, userData);
        });
    }
}

void UICornerstone_SetClosedSwatchSize(UIControlHandle ctl, float size) {
    if (!ctl) return;
    auto* cp = dynamic_cast<ColorPicker*>(static_cast<Control*>(ctl));
    if (cp) cp->setClosedSwatchSize(size);
}

void UICornerstone_SetClosedFontSize(UIControlHandle ctl, int size) {
    if (!ctl) return;
    auto* cp = dynamic_cast<ColorPicker*>(static_cast<Control*>(ctl));
    if (cp) cp->setClosedFontSize(size);
}

void UICornerstone_SetClosedTextColor(UIControlHandle ctl, const char* hex) {
    if (!ctl || !hex) return;
    auto* cp = dynamic_cast<ColorPicker*>(static_cast<Control*>(ctl));
    if (cp) {
        SColor c;
        if (SColor::fromHex(hex, c))
            cp->setClosedTextColor(c);
    }
}

void UICornerstone_SetPopupBGColor(UIControlHandle ctl, const char* hex) {
    if (!ctl || !hex) return;
    auto* cp = dynamic_cast<ColorPicker*>(static_cast<Control*>(ctl));
    if (cp) {
        SColor c;
        if (SColor::fromHex(hex, c))
            cp->setPopupBGColor(c);
    }
}

// ============================================================
// ComboBox
// ============================================================
UIControlHandle UICornerstone_CreateComboBox(
    float x, float y, float w, float h)
{
    auto ctl = std::make_shared<ComboBox>(BENCH, SRect(x, y, w, h));
    BENCH->addControl(ctl);
    ctl->create();
    ctl->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

void UICornerstone_SetComboItems(UIControlHandle ctl, const char* jsonItems) {
    if (!ctl || !jsonItems) return;
    auto* combo = dynamic_cast<ComboBox*>(static_cast<Control*>(ctl));
    if (!combo) return;
    try {
        auto j = nlohmann::json::parse(jsonItems);
        vector<ComboBoxItem> items;
        for (auto& jitem : j) {
            ComboBoxItem item;
            item.label = jitem.value("label", "");
            item.value = jitem.value("value", item.label);
            item.disabled = jitem.value("disabled", false);
            items.push_back(item);
        }
        combo->setItems(items);
    } catch (...) {}
}

void UICornerstone_SetSelectedIndex(UIControlHandle ctl, int index) {
    if (!ctl) return;
    auto* combo = dynamic_cast<ComboBox*>(static_cast<Control*>(ctl));
    if (combo) combo->setSelectedIndex(index);
}

int UICornerstone_GetSelectedIndex(UIControlHandle ctl) {
    if (!ctl) return -1;
    auto* combo = dynamic_cast<ComboBox*>(static_cast<Control*>(ctl));
    return combo ? combo->getSelectedIndex() : -1;
}

const char* UICornerstone_GetSelectedLabel(UIControlHandle ctl) {
    if (!ctl) return "";
    auto* combo = dynamic_cast<ComboBox*>(static_cast<Control*>(ctl));
    if (!combo) return "";
    static char buf[256];
    string label = combo->getSelectedLabel();
    strncpy(buf, label.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    return buf;
}

void UICornerstone_SetOnSelectionChanged(UIControlHandle ctl, UIActionCallback cb, void* userData) {
    if (!ctl) return;
    auto* combo = dynamic_cast<ComboBox*>(static_cast<Control*>(ctl));
    if (combo) {
        combo->setOnSelectionChanged([cb, userData](std::shared_ptr<ComboBox>, int, const string&) {
            if (cb) cb(nullptr, userData);
        });
    }
}

// ============================================================
// Dialog / Popup
// ============================================================
UIControlHandle UICornerstone_CreateDialog(
    const char* confirmText, const char* cancelText,
    float x, float y, float w, float h)
{
    auto ctl = std::make_shared<Dialog>(BENCH, SRect(x, y, w, h));
    if (confirmText) ctl->setConfirmButtonText(confirmText);
    if (cancelText) ctl->setCancelButtonText(cancelText);
    ctl->setCentered();
    ctl->create();

    // 保持 Dialog 生命期：加入 g_popupPool，close() 时自动清理
    g_popupPool.push_back(ctl);

    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl.get()));
}

void UICornerstone_Show(UIControlHandle ctl) {
    if (!ctl) return;
    auto* dlg = dynamic_cast<Dialog*>(static_cast<Control*>(ctl));
    if (dlg) { dlg->open(); return; }
    auto* cp = dynamic_cast<ConfirmPopup*>(static_cast<Control*>(ctl));
    if (cp) { cp->open(); return; }
    auto* pop = dynamic_cast<Popup*>(static_cast<Control*>(ctl));
    if (pop) { pop->open(); }
}

void UICornerstone_Close(UIControlHandle ctl) {
    if (!ctl) return;
    auto* pop = dynamic_cast<Popup*>(static_cast<Control*>(ctl));
    if (pop) {
        // 清理 pool（如果 close() 触发 m_onClose，也会在 SetOnClose 的包装器中清理）
        auto& pool = g_popupPool;
        auto it = std::find_if(pool.begin(), pool.end(),
            [pop](const std::shared_ptr<Popup>& p) { return p.get() == pop; });
        if (it != pool.end()) pool.erase(it);
        pop->close();
    }
}

void UICornerstone_SetDialogCentered(UIControlHandle ctl, int centered) {
    if (!ctl || !centered) return;
    auto* dlg = dynamic_cast<Dialog*>(static_cast<Control*>(ctl));
    if (dlg) { dlg->setCentered(); return; }
    auto* cp = dynamic_cast<ConfirmPopup*>(static_cast<Control*>(ctl));
    if (cp) { cp->setCentered(); return; }
    auto* pop = dynamic_cast<Popup*>(static_cast<Control*>(ctl));
    if (pop) { pop->setCentered(); }
}

void UICornerstone_SetDialogPosition(UIControlHandle ctl, float x, float y, float w, float h) {
    if (!ctl) return;
    auto* pop = dynamic_cast<Popup*>(static_cast<Control*>(ctl));
    if (pop) pop->setAbsolute(SRect(x, y, w, h));
}

void UICornerstone_SetContent(UIControlHandle dlg, UIControlHandle content) {
    if (!dlg || !content) return;
    auto* pop = dynamic_cast<Popup*>(static_cast<Control*>(dlg));
    if (!pop) return;
    auto* contentCtrl = dynamic_cast<ControlImpl*>(static_cast<Control*>(content));
    if (!contentCtrl) return;
    try {
        auto sp = contentCtrl->shared_from_this();
        Control* parent = contentCtrl->getParent();
        if (parent && parent != BENCH) {
            parent->removeControl(sp);
        } else {
            BENCH->removeControl(sp);
        }
        pop->setContent(std::dynamic_pointer_cast<ControlImpl>(sp));
    } catch (...) {}
}

void UICornerstone_SetOnConfirm(UIControlHandle ctl, UIActionCallback cb, void* userData) {
    if (!ctl) return;
    auto* dlg = dynamic_cast<Dialog*>(static_cast<Control*>(ctl));
    if (dlg) {
        dlg->setOnConfirm([cb, userData](std::shared_ptr<ConfirmPopup>) {
            if (cb) cb(nullptr, userData);
        });
        return;
    }
    auto* cp = dynamic_cast<ConfirmPopup*>(static_cast<Control*>(ctl));
    if (cp) {
        cp->setOnConfirm([cb, userData](std::shared_ptr<ConfirmPopup>) {
            if (cb) cb(nullptr, userData);
        });
    }
}

void UICornerstone_SetOnCancel(UIControlHandle ctl, UIActionCallback cb, void* userData) {
    if (!ctl) return;
    auto* dlg = dynamic_cast<Dialog*>(static_cast<Control*>(ctl));
    if (dlg) {
        dlg->setOnCancel([cb, userData](std::shared_ptr<Dialog>) {
            if (cb) cb(nullptr, userData);
        });
    }
}

void UICornerstone_SetOnClose(UIControlHandle ctl, UIActionCallback cb, void* userData) {
    if (!ctl) return;
    auto* pop = dynamic_cast<Popup*>(static_cast<Control*>(ctl));
    if (pop) {
        pop->setOnClose([cb, userData](std::shared_ptr<Popup> p, DialogResult r) {
            if (cb) cb(nullptr, userData);
            // 清理 pool
            auto& pool = g_popupPool;
            auto it = std::find(pool.begin(), pool.end(), p);
            if (it != pool.end()) pool.erase(it);
        });
    }
}

void UICornerstone_SetConfirmButtonText(UIControlHandle ctl, const char* text) {
    if (!ctl || !text) return;
    auto* dlg = dynamic_cast<Dialog*>(static_cast<Control*>(ctl));
    if (dlg) { dlg->setConfirmButtonText(text); return; }
    auto* cp = dynamic_cast<ConfirmPopup*>(static_cast<Control*>(ctl));
    if (cp) cp->setConfirmButtonText(text);
}

void UICornerstone_SetCancelButtonText(UIControlHandle ctl, const char* text) {
    if (!ctl || !text) return;
    auto* dlg = dynamic_cast<Dialog*>(static_cast<Control*>(ctl));
    if (dlg) dlg->setCancelButtonText(text);
}

float UICornerstone_GetSliderValue(UIControlHandle ctl) {
    if (!ctl) return 0.0f;
    auto* sl = dynamic_cast<Slider*>(static_cast<Control*>(ctl));
    return sl ? sl->getValue() : 0.0f;
}

void UICornerstone_SetSliderValue(UIControlHandle ctl, float value) {
    if (!ctl) return;
    auto* sl = dynamic_cast<Slider*>(static_cast<Control*>(ctl));
    if (sl) sl->setValue(value);
}

void UICornerstone_SetOnSliderChanged(UIControlHandle ctl, UIActionCallback cb, void* userData) {
    if (!ctl) return;
    auto* sl = dynamic_cast<Slider*>(static_cast<Control*>(ctl));
    if (sl) {
        sl->setOnValueChanged([cb, userData](std::shared_ptr<Slider>, float) {
            if (cb) cb(nullptr, userData);
        });
    }
}

// ── NumericUpDown C ABI ──

UIControlHandle UICornerstone_CreateNumericUpDown(float x, float y, float w, float h) {
    auto nud = make_shared<NumericUpDown>(BENCH, SRect(x, y, w, h));
    BENCH->addControl(nud);
    nud->create();
    nud->setVisible(true);
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(nud.get()));
}

void UICornerstone_SetNumericUpDownValue(UIControlHandle ctl, double val) {
    if (!ctl) return;
    auto* nud = dynamic_cast<NumericUpDown*>(static_cast<Control*>(ctl));
    if (nud) nud->setValue(val);
}

double UICornerstone_GetNumericUpDownValue(UIControlHandle ctl) {
    if (!ctl) return 0.0;
    auto* nud = dynamic_cast<NumericUpDown*>(static_cast<Control*>(ctl));
    return nud ? nud->getValue() : 0.0;
}

void UICornerstone_SetNumericUpDownRange(UIControlHandle ctl, double min, double max) {
    if (!ctl) return;
    auto* nud = dynamic_cast<NumericUpDown*>(static_cast<Control*>(ctl));
    if (nud) nud->setRange(min, max);
}

void UICornerstone_SetNumericUpDownStep(UIControlHandle ctl, double step) {
    if (!ctl) return;
    auto* nud = dynamic_cast<NumericUpDown*>(static_cast<Control*>(ctl));
    if (nud) nud->setStep(step);
}

void UICornerstone_SetNumericUpDownPageStep(UIControlHandle ctl, double ps) {
    if (!ctl) return;
    auto* nud = dynamic_cast<NumericUpDown*>(static_cast<Control*>(ctl));
    if (nud) nud->setPageStep(ps);
}

void UICornerstone_SetNumericUpDownDecimals(UIControlHandle ctl, int decimals) {
    if (!ctl) return;
    auto* nud = dynamic_cast<NumericUpDown*>(static_cast<Control*>(ctl));
    if (nud) nud->setDecimals(decimals);
}

void UICornerstone_SetNumericUpDownPlaceholder(UIControlHandle ctl, const char* placeholder) {
    if (!ctl) return;
    auto* nud = dynamic_cast<NumericUpDown*>(static_cast<Control*>(ctl));
    if (nud) nud->setPlaceholder(placeholder ? string(placeholder) : "");
}

void UICornerstone_SetNumericUpDownReadOnly(UIControlHandle ctl, int readOnly) {
    if (!ctl) return;
    auto* nud = dynamic_cast<NumericUpDown*>(static_cast<Control*>(ctl));
    if (nud) nud->setReadOnly(readOnly != 0);
}

void UICornerstone_SetNumericUpDownButtonWidth(UIControlHandle ctl, float width) {
    if (!ctl) return;
    auto* nud = dynamic_cast<NumericUpDown*>(static_cast<Control*>(ctl));
    if (nud) nud->setButtonWidth(width);
}

void UICornerstone_SetOnNumericUpDownValueChanged(
    UIControlHandle ctl,
    void (*callback)(void* userData, double newValue),
    void* userData)
{
    if (!ctl) return;
    auto* nud = dynamic_cast<NumericUpDown*>(static_cast<Control*>(ctl));
    if (nud) {
        nud->setOnValueChanged([callback, userData](shared_ptr<Control>, double v) {
            if (callback) callback(userData, v);
        });
    }
}

void UICornerstone_DestroyControl(UIControlHandle ctl) {
    if (!ctl) return;
    auto* ctrl = dynamic_cast<ControlImpl*>(static_cast<Control*>(ctl));
    if (!ctrl) return;
    try {
        auto sp = ctrl->shared_from_this();
        Control* parent = ctrl->getParent();
        if (parent && parent != BENCH) {
            parent->removeControl(sp);
        } else {
            BENCH->removeControl(sp);
        }
    } catch (...) {}
}

void UICornerstone_WinFrameSetClientText(UIControlHandle wf, const char* text) {
    if (!wf) return;
    auto* winFrame = dynamic_cast<WinFrame*>(static_cast<Control*>(wf));
    if (!winFrame) return;
    auto client = winFrame->getClientPanel();
    if (!client) return;

    // 移除 client panel 中已有的所有子控件
    auto children = client->getChildren();
    for (auto& child : children) {
        client->removeControl(child);
    }

    // 创建新的 Label 显示文本
    SRect cr = client->getRect();
    SRect labelRect(0, 0, cr.width, cr.height);
    auto label = std::make_shared<Label>(client.get(), labelRect);
    label->setCaption(text ? text : "");
    label->setFont(FontName::HarmonyOS_Sans_SC_Regular);
    label->setFontSize(14);
    label->setAlignmentMode(AlignmentMode::AM_MID_LEFT);
    label->setTextNormalStateColor(SColor(220, 220, 220, 255));
    label->setEnableExpand(false);
    client->addControl(label);
    label->create();
    label->setVisible(true);
}

