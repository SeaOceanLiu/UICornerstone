#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cstdio>
#include <cstdint>
#include <windows.h>

// UICornerstoneAPI.h 提供 UIEvent 结构体和 UI_EVENT_* 宏定义
#include "../../include/UICornerstoneAPI.h"
#include "../../include/EventTypes.h"

// ============================================================
// 全局变量：由 SDL_AppInit 创建，被后端代码复用
// ============================================================
static SDL_Window*   g_window     = nullptr;
static SDL_Renderer* g_renderer   = nullptr;
static HMODULE       g_uiDll      = nullptr;
static bool g_uiInitialized = false;

// ============================================================
// 后端函数声明 — 编译为独立翻译单元，链接时解析
// ============================================================
extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);
void SDL3Backend_SetReuseWindow(SDL_Window* w, SDL_Renderer* r);

// sdl3/InputBackend.cpp (signatures match InputBackend.h)
KeyCode SDLKeycodeToKeyCode(int sdlKey);
KeyMod SDLKeymodToKeyMod(uint16_t sdlMod);

// ============================================================
// C ABI 函数指针
// ============================================================
typedef int   (*UIInitFn)(void*);
typedef void  (*UISetViewportFn)(float,float,float,float);
typedef void  (*UIProcessEventsFn)(void);
typedef void  (*UIUpdateFn)(double);
typedef void  (*UIClearFn)(void);
typedef void  (*UIRenderFn)(void);
typedef void  (*UIPresentFn)(void);
typedef int   (*UIIsQuitFn)(void);
typedef void  (*UIShutdownFn)(void);
typedef void* (*UICreateButtonFn)(const char*,float,float,float,float);
typedef void* (*UICreateLabelFn)(const char*,float,float,float,float,float);
typedef void* (*UICreateCheckBoxFn)(const char*,float,float,float,float);
typedef void* (*UICreateEditBoxFn)(float,float,float,float);
typedef void* (*UICreateProgressBarFn)(float,float,float,float);
typedef void* (*UICreatePanelFn)(float,float,float,float);
typedef void* (*UICreateTextAreaFn)(float,float,float,float);
typedef void* (*UICreateWinFrameFn)(const char*,float,float,float,float);
typedef void  (*UISetBGColorFn)(void*,uint8_t,uint8_t,uint8_t,uint8_t);
typedef void  (*UISetTextFn)(void*,const char*);
typedef void  (*UISetProgressFn)(void*,float);
typedef void  (*UISetCheckedFn)(void*,int);
typedef void  (*UIAddChildFn)(void*,void*);
typedef const char* (*UIGetTextFn)(void*);
typedef int   (*UIGetCheckedFn)(void*);
typedef float (*UIGetProgressFn)(void*);
typedef void  (*UIPushUIEventFn)(const void*);
typedef void  (*UISetOnClickFn)(void*, void (*)(void*,void*), void*);
typedef void  (*UISetVisibleFn)(void*, int);
typedef void  (*UIDestroyControlFn)(void*);
typedef void  (*UIWinFrameSetClientTextFn)(void*, const char*);
typedef void* (*UICreateImageButtonFn)(const char*,const char*,const char*,float,float,float,float);
typedef void  (*UISetButtonAnimationFn)(void*, const char*);
typedef void* (*UICreateSliderFn)(float,float,float,float,float,float,float);
typedef float (*UIGetSliderValueFn)(void*);
typedef void  (*UISetSliderValueFn)(void*,float);
typedef void  (*UISetOnSliderChangedFn)(void*, void (*)(void*,void*), void*);

static UIInitFn             uiInit             = nullptr;
static UISetViewportFn      uiSetViewport      = nullptr;
static UIProcessEventsFn    uiProcessEvents    = nullptr;
static UIUpdateFn           uiUpdate           = nullptr;
static UIClearFn            uiClear            = nullptr;
static UIRenderFn           uiRender           = nullptr;
static UIPresentFn          uiPresent          = nullptr;
static UIIsQuitFn           uiIsQuit           = nullptr;
static UIShutdownFn         uiShutdown         = nullptr;
static UICreateButtonFn     uiCreateButton     = nullptr;
static UICreateLabelFn      uiCreateLabel      = nullptr;
static UICreateCheckBoxFn   uiCreateCheckBox   = nullptr;
static UICreateEditBoxFn    uiCreateEditBox    = nullptr;
static UICreateProgressBarFn uiCreateProgressBar = nullptr;
static UICreatePanelFn      uiCreatePanel      = nullptr;
static UICreateTextAreaFn   uiCreateTextArea   = nullptr;
static UICreateWinFrameFn   uiCreateWinFrame   = nullptr;
static UISetBGColorFn       uiSetBGColor       = nullptr;
static UISetTextFn          uiSetText          = nullptr;
static UISetProgressFn      uiSetProgress      = nullptr;
static UISetCheckedFn       uiSetChecked       = nullptr;
static UIAddChildFn         uiAddChild         = nullptr;
static UIGetTextFn          uiGetText          = nullptr;
static UIGetCheckedFn       uiGetChecked       = nullptr;
static UIGetProgressFn      uiGetProgress      = nullptr;
static UIPushUIEventFn      uiPushUIEvent      = nullptr;
static UISetOnClickFn       uiSetOnClick       = nullptr;
static UISetVisibleFn       uiSetVisible       = nullptr;
static UIDestroyControlFn   uiDestroyControl   = nullptr;
static UIWinFrameSetClientTextFn uiSetWinFrameClientText = nullptr;
static UICreateImageButtonFn    uiCreateImageButton  = nullptr;
static UISetButtonAnimationFn   uiSetButtonAnimation = nullptr;
static UICreateSliderFn         uiCreateSlider       = nullptr;
static UIGetSliderValueFn       uiGetSliderValue     = nullptr;
static UISetSliderValueFn       uiSetSliderValue     = nullptr;
static UISetOnSliderChangedFn   uiSetOnSliderChanged = nullptr;

static void* g_btnHandle     = nullptr;
static void* g_checkHandle   = nullptr;
static void* g_editHandle    = nullptr;
static void* g_progressHandle = nullptr;
static void* g_panelHandle   = nullptr;
static void* g_textAreaHandle = nullptr;
static void* g_chkStatus      = nullptr;
static void* g_prgStatus      = nullptr;
static void* g_edtStatus      = nullptr;
static void* g_winFrameHandle = nullptr;
static void* g_imgBtnHandle   = nullptr;
static void* g_aniBtnHandle   = nullptr;
static void* g_sliderHandle   = nullptr;
static int   g_frameCount     = 0;

// SDL_Event → UIEvent 转换
static void sdlEventToUIEvent(const SDL_Event* sdl, UIEvent* ue) {
    memset(ue, 0, sizeof(*ue));
    switch (sdl->type) {
    case SDL_EVENT_MOUSE_MOTION:
        ue->type = UI_EVENT_MOUSE_MOVE;
        memcpy(ue->data, &sdl->motion.x, sizeof(float));
        memcpy(ue->data + 4, &sdl->motion.y, sizeof(float));
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        ue->type = UI_EVENT_MOUSE_DOWN;
        { float fx = (float)sdl->button.x, fy = (float)sdl->button.y;
          memcpy(ue->data, &fx, sizeof(float));
          memcpy(ue->data + 4, &fy, sizeof(float)); }
        { int btn = (int)sdl->button.button; memcpy(ue->data + 8, &btn, sizeof(int)); }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        ue->type = UI_EVENT_MOUSE_UP;
        { float fx = (float)sdl->button.x, fy = (float)sdl->button.y;
          memcpy(ue->data, &fx, sizeof(float));
          memcpy(ue->data + 4, &fy, sizeof(float)); }
        { int btn = (int)sdl->button.button; memcpy(ue->data + 8, &btn, sizeof(int)); }
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        ue->type = UI_EVENT_MOUSE_WHEEL;
        { float sy = sdl->wheel.y * -120; memcpy(ue->data, &sy, sizeof(float)); }
        break;
    case SDL_EVENT_KEY_DOWN:
        ue->type = UI_EVENT_KEY_DOWN;
        { int kc = (int)SDLKeycodeToKeyCode((int)sdl->key.key);
          memcpy(ue->data, &kc, sizeof(int)); }
        { uint16_t mod = (uint16_t)SDLKeymodToKeyMod((uint16_t)sdl->key.mod);
          memcpy(ue->data + 4, &mod, sizeof(uint16_t)); }
        break;
    case SDL_EVENT_KEY_UP:
        ue->type = UI_EVENT_KEY_UP;
        { int kc = (int)SDLKeycodeToKeyCode((int)sdl->key.key);
          memcpy(ue->data, &kc, sizeof(int)); }
        { uint16_t mod = (uint16_t)SDLKeymodToKeyMod((uint16_t)sdl->key.mod);
          memcpy(ue->data + 4, &mod, sizeof(uint16_t)); }
        break;
    case SDL_EVENT_TEXT_INPUT:
        ue->type = UI_EVENT_TEXT_INPUT;
        strncpy((char*)ue->data, sdl->text.text, 31);
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        ue->type = UI_EVENT_WINDOW_RESIZE;
        memcpy(ue->data, &sdl->window.data1, sizeof(int));
        memcpy(ue->data + 4, &sdl->window.data2, sizeof(int));
        break;
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        ue->type = UI_EVENT_WINDOW_CLOSE;
        break;
    default:
        break;
    }
}

static void onButtonClick(void* ctl, void* userData) {
    (void)ctl; (void)userData;
    printf("Button clicked! Showing WinFrame with TextArea content...\n");
    fflush(stdout);
    if (!g_textAreaHandle || !uiGetText) return;

    const char* text = uiGetText(g_textAreaHandle);

    if (!g_winFrameHandle) {
        g_winFrameHandle = uiCreateWinFrame(
            "TextArea Content", 60, 40, 500, 300);
        if (!g_winFrameHandle) {
            printf("FAIL: creating WinFrame\n"); fflush(stdout);
            return;
        }
        printf("OK: created WinFrame\n"); fflush(stdout);
    } else if (uiSetVisible) {
        uiSetVisible(g_winFrameHandle, 1);
    }

    if (uiSetWinFrameClientText) {
        uiSetWinFrameClientText(g_winFrameHandle, text);
        printf("OK: updated WinFrame content from TextArea\n"); fflush(stdout);
    }
}

// ============================================================
// SDL App Callbacks
// ============================================================
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    (void)appstate; (void)argc; (void)argv;
    printf("SDL_AppInit\n"); fflush(stdout);

    if (!SDL_CreateWindowAndRenderer("test_fromsource_sdl3", 800, 600, 0,
                                      &g_window, &g_renderer)) {
        printf("FAIL: SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL3Backend_SetReuseWindow(g_window, g_renderer);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) {
        printf("FAIL: LoadLibrary(UICornerstone.dll)\n");
        return SDL_APP_FAILURE;
    }
    printf("OK: loaded UICornerstone.dll\n"); fflush(stdout);

    uiInit          = (UIInitFn)GetProcAddress(g_uiDll, "UICornerstone_Init");
    uiSetViewport   = (UISetViewportFn)GetProcAddress(g_uiDll, "UICornerstone_SetViewport");
    uiProcessEvents = (UIProcessEventsFn)GetProcAddress(g_uiDll, "UICornerstone_ProcessEvents");
    uiUpdate        = (UIUpdateFn)GetProcAddress(g_uiDll, "UICornerstone_Update");
    uiClear         = (UIClearFn)GetProcAddress(g_uiDll, "UICornerstone_Clear");
    uiRender        = (UIRenderFn)GetProcAddress(g_uiDll, "UICornerstone_Render");
    uiPresent       = (UIPresentFn)GetProcAddress(g_uiDll, "UICornerstone_Present");
    uiIsQuit        = (UIIsQuitFn)GetProcAddress(g_uiDll, "UICornerstone_IsQuitRequested");
    uiShutdown      = (UIShutdownFn)GetProcAddress(g_uiDll, "UICornerstone_Shutdown");
    uiCreateButton     = (UICreateButtonFn)GetProcAddress(g_uiDll, "UICornerstone_CreateButton");
    uiCreateLabel      = (UICreateLabelFn)GetProcAddress(g_uiDll, "UICornerstone_CreateLabel");
    uiCreateCheckBox   = (UICreateCheckBoxFn)GetProcAddress(g_uiDll, "UICornerstone_CreateCheckBox");
    uiCreateEditBox    = (UICreateEditBoxFn)GetProcAddress(g_uiDll, "UICornerstone_CreateEditBox");
    uiCreateProgressBar = (UICreateProgressBarFn)GetProcAddress(g_uiDll, "UICornerstone_CreateProgressBar");
    uiCreatePanel      = (UICreatePanelFn)GetProcAddress(g_uiDll, "UICornerstone_CreatePanel");
    uiCreateTextArea   = (UICreateTextAreaFn)GetProcAddress(g_uiDll, "UICornerstone_CreateTextArea");
    uiCreateWinFrame   = (UICreateWinFrameFn)GetProcAddress(g_uiDll, "UICornerstone_CreateWinFrame");
    uiSetBGColor       = (UISetBGColorFn)GetProcAddress(g_uiDll, "UICornerstone_SetBGColor");
    uiSetText          = (UISetTextFn)GetProcAddress(g_uiDll, "UICornerstone_SetText");
    uiSetProgress      = (UISetProgressFn)GetProcAddress(g_uiDll, "UICornerstone_SetProgress");
    uiSetChecked       = (UISetCheckedFn)GetProcAddress(g_uiDll, "UICornerstone_SetChecked");
    uiAddChild         = (UIAddChildFn)GetProcAddress(g_uiDll, "UICornerstone_AddChild");
    uiGetText          = (UIGetTextFn)GetProcAddress(g_uiDll, "UICornerstone_GetText");
    uiGetChecked       = (UIGetCheckedFn)GetProcAddress(g_uiDll, "UICornerstone_GetChecked");
    uiGetProgress      = (UIGetProgressFn)GetProcAddress(g_uiDll, "UICornerstone_GetProgress");
    uiPushUIEvent      = (UIPushUIEventFn)GetProcAddress(g_uiDll, "UICornerstone_PushUIEvent");
    uiSetOnClick       = (UISetOnClickFn)GetProcAddress(g_uiDll, "UICornerstone_SetOnClick");
    uiSetVisible       = (UISetVisibleFn)GetProcAddress(g_uiDll, "UICornerstone_SetVisible");
    uiDestroyControl   = (UIDestroyControlFn)GetProcAddress(g_uiDll, "UICornerstone_DestroyControl");
    uiSetWinFrameClientText = (UIWinFrameSetClientTextFn)GetProcAddress(g_uiDll, "UICornerstone_WinFrameSetClientText");
    uiCreateImageButton  = (UICreateImageButtonFn)GetProcAddress(g_uiDll, "UICornerstone_CreateImageButton");
    uiSetButtonAnimation = (UISetButtonAnimationFn)GetProcAddress(g_uiDll, "UICornerstone_SetButtonAnimation");
    uiCreateSlider       = (UICreateSliderFn)GetProcAddress(g_uiDll, "UICornerstone_CreateSlider");
    uiGetSliderValue     = (UIGetSliderValueFn)GetProcAddress(g_uiDll, "UICornerstone_GetSliderValue");
    uiSetSliderValue     = (UISetSliderValueFn)GetProcAddress(g_uiDll, "UICornerstone_SetSliderValue");
    uiSetOnSliderChanged = (UISetOnSliderChangedFn)GetProcAddress(g_uiDll, "UICornerstone_SetOnSliderChanged");

    if (!uiInit) {
        printf("FAIL: GetProcAddress(UICornerstone_Init)\n");
        return SDL_APP_FAILURE;
    }

    void* cbs = GetUIBackendCallbacks();
    if (!cbs) {
        printf("FAIL: GetUIBackendCallbacks\n");
        return SDL_APP_FAILURE;
    }
    if (!uiInit(cbs)) {
        printf("FAIL: UICornerstone_Init\n");
        return SDL_APP_FAILURE;
    }
    uiSetViewport(0, 0, 800, 480);
    g_uiInitialized = true;
    printf("OK: UICornerstone initialized\n"); fflush(stdout);

    // 创建测试控件 — 按关联分组布局
    // 分组 1: CheckBox + 状态标签（靠左）
    if (uiCreateCheckBox) {
        g_checkHandle = uiCreateCheckBox("Check me", 20, 15, 180, 30);
        if (g_checkHandle) {
            printf("OK: created CheckBox\n");
            if (uiSetChecked) uiSetChecked(g_checkHandle, 1);
        }
    }
    if (uiCreateLabel) {
        g_chkStatus = uiCreateLabel("CheckBox: Checked", 12.0f, 20, 50, 180, 16);
        if (g_chkStatus) printf("OK: created chkStatus\n");
    }
    // 分组 2: EditBox + 状态标签（紧接 CheckBox，宽度延伸到窗体边缘）
    if (uiCreateEditBox) {
        g_editHandle = uiCreateEditBox(220, 15, 560, 30);
        if (g_editHandle) {
            printf("OK: created EditBox\n");
            if (uiSetText) uiSetText(g_editHandle, "Type here...");
        }
    }
    if (uiCreateLabel) {
        g_edtStatus = uiCreateLabel("Edit: ", 12.0f, 220, 50, 560, 16);
        if (g_edtStatus) printf("OK: created edtStatus\n");
    }
    // 分组 3: ProgressBar + 状态标签（宽度与 EditBox 对齐）
    if (uiCreateProgressBar) {
        g_progressHandle = uiCreateProgressBar(20, 80, 760, 20);
        if (g_progressHandle) {
            printf("OK: created ProgressBar\n");
            if (uiSetBGColor) uiSetBGColor(g_progressHandle, 60, 60, 60, 255);
            if (uiSetProgress) uiSetProgress(g_progressHandle, 0.0f);
        }
    }
    if (uiCreateLabel) {
        g_prgStatus = uiCreateLabel("Progress: 0.0%", 12.0f, 20, 105, 230, 16);
        if (g_prgStatus) printf("OK: created prgStatus\n");
    }
    // 分组 4: Panel + TextArea + Button（按钮放在面板内）
    if (uiCreatePanel && uiCreateTextArea && uiAddChild) {
        g_panelHandle = uiCreatePanel(20, 135, 760, 330);
        if (g_panelHandle) {
            printf("OK: created Panel\n");
            if (uiSetBGColor) uiSetBGColor(g_panelHandle, 50, 55, 60, 255);
        }
        // TextArea 占据面板大部分区域
        g_textAreaHandle = uiCreateTextArea(5, 5, 750, 260);
        if (g_textAreaHandle) {
            printf("OK: created TextArea\n");
            if (uiSetText) uiSetText(g_textAreaHandle, "Hello from TextArea!\nEdit me and click the button.");
            uiAddChild(g_panelHandle, g_textAreaHandle);
            printf("OK: added TextArea to Panel\n");
        }
        // 分组 5: Slider — range 0-100, horizontal, with callback
    if (uiCreateSlider) {
        g_sliderHandle = uiCreateSlider(20, 470, 300, 30, 0, 100, 50);
        if (g_sliderHandle) {
            printf("OK: created Slider\n");
        }
    }

    // 图片测试按钮（cross_up/cross_over/cross_down, 左对齐）
        if (uiCreateImageButton) {
            g_imgBtnHandle = uiCreateImageButton(
                "assets/images/cross_up.png",
                "assets/images/cross_over.png",
                "assets/images/cross_down.png",
                5, 270, 200, 30);
            if (g_imgBtnHandle) {
                printf("OK: created ImageButton\n");
                if (uiSetOnClick)
                    uiSetOnClick(g_imgBtnHandle, onButtonClick, nullptr);
                uiAddChild(g_panelHandle, g_imgBtnHandle);
            }
        }
        // LuotiAni 动画测试按钮
        if (uiCreateButton && uiSetButtonAnimation) {
            g_aniBtnHandle = uiCreateButton("Ani Test", 210, 270, 200, 30);
            if (g_aniBtnHandle) {
                printf("OK: created Animation Button\n");
                uiSetButtonAnimation(g_aniBtnHandle, "assets/animations/rotateBtn/rotateBtn.jsonc");
                if (uiSetOnClick)
                    uiSetOnClick(g_aniBtnHandle, onButtonClick, nullptr);
                uiAddChild(g_panelHandle, g_aniBtnHandle);
            }
        }
        // Button 放在 TextArea 下方右侧
        if (uiCreateButton) {
            g_btnHandle = uiCreateButton(u8"读取TextArea文本内容", 555, 270, 200, 30);
            if (g_btnHandle) {
                printf("OK: created Button (in Panel)\n");
                if (uiSetBGColor)
                    uiSetBGColor(g_btnHandle, 100, 149, 237, 255);
                if (uiSetOnClick)
                    uiSetOnClick(g_btnHandle, onButtonClick, nullptr);
                uiAddChild(g_panelHandle, g_btnHandle);
                printf("OK: added Button to Panel\n");
            }
        }
    }
    fflush(stdout);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    (void)appstate;
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (g_uiInitialized && uiPushUIEvent) {
        UIEvent ue;
        sdlEventToUIEvent(event, &ue);
        uiPushUIEvent(&ue);
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    (void)appstate;

    if (g_uiInitialized) {
        g_frameCount++;

        // 进度条动画
        if (g_progressHandle && uiSetProgress) {
            float p = ((g_frameCount % 120) / 120.0f) * 100.0f;
            uiSetProgress(g_progressHandle, p);
        }

        uiProcessEvents();
        uiClear();
        uiUpdate(1.0/60.0);

        // 轮询 Slider 值
        if (g_sliderHandle && uiGetSliderValue) {
            float sv = uiGetSliderValue(g_sliderHandle);
            printf("Slider: %.1f\r", sv);
        }

        // 轮询状态并更新标签
        char buf[256];
        if (g_checkHandle && uiGetChecked && g_chkStatus && uiSetText) {
            int st = uiGetChecked(g_checkHandle);
            const char* label = "Unchecked";
            if (st == 1) label = "Checked";
            else if (st == 2) label = "Indeterminate";
            snprintf(buf, sizeof(buf), "CheckBox: %s", label);
            uiSetText(g_chkStatus, buf);
        }
        if (g_progressHandle && uiGetProgress && g_prgStatus && uiSetText) {
            float v = uiGetProgress(g_progressHandle);
            snprintf(buf, sizeof(buf), "Progress: %.1f%%", v);
            uiSetText(g_prgStatus, buf);
        }
        if (g_editHandle && uiGetText && g_edtStatus && uiSetText) {
            const char* t = uiGetText(g_editHandle);
            size_t tlen = strlen(t);
            if (tlen > 32) {
                memcpy(buf, t, 32);
                buf[32] = '\0';
                snprintf(buf + 32, sizeof(buf) - 32, "...(%zu)", tlen);
            } else {
                snprintf(buf, sizeof(buf), "Edit: %s", t);
            }
            uiSetText(g_edtStatus, buf);
        }

        uiRender();
    }

    SDL_RenderPresent(g_renderer);

    if (g_uiInitialized && uiIsQuit()) {
        printf("UICornerstone requested quit\n");
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    (void)appstate; (void)result;
    printf("SDL_AppQuit\n"); fflush(stdout);

    // UICornerstone_Shutdown 会通过 ~SDL3Window 销毁 g_window/g_renderer
    if (g_uiInitialized && uiShutdown) {
        uiShutdown();
        g_uiInitialized = false;
    }

    if (g_uiDll) {
        FreeLibrary(g_uiDll);
        g_uiDll = nullptr;
    }

    // 窗口/渲染器已被 ~SDL3Window 销毁，不再重复销毁
    g_window = nullptr;
    g_renderer = nullptr;
}
