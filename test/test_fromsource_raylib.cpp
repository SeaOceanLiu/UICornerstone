#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <windows.h>

// ============================================================
// GetUIBackendCallbacks — compiled from BackendPlugin.cpp
// ============================================================
extern "C" struct UIBackendCallbacks* GetUIBackendCallbacks(void);

// ============================================================
// 全局变量
// ============================================================
static HMODULE g_uiDll = nullptr;
static bool g_uiInitialized = false;
static int  g_frameCount = 0;

// ============================================================
// 测试纹理 DIAG
// ============================================================

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
typedef void* (*UICreateColorPickerFn)(float,float,float,float,const char*);
typedef void  (*UIGetColorPickerColorFn)(void*,char*,int);
typedef void  (*UISetOnColorChangedFn)(void*, void (*)(void*,void*), void*);
typedef void  (*UISetClosedSwatchSizeFn)(void*,float);
typedef void  (*UISetClosedFontSizeFn)(void*,int);
typedef void  (*UISetClosedTextColorFn)(void*,const char*);
typedef void  (*UISetPopupBGColorFn)(void*,const char*);

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
static UISetOnClickFn       uiSetOnClick       = nullptr;
static UISetVisibleFn       uiSetVisible       = nullptr;
static UIDestroyControlFn   uiDestroyControl   = nullptr;
static UIWinFrameSetClientTextFn uiSetWinFrameClientText = nullptr;
static UICreateImageButtonFn uiCreateImageButton  = nullptr;
static UISetButtonAnimationFn uiSetButtonAnimation = nullptr;
static UICreateSliderFn         uiCreateSlider       = nullptr;
static UIGetSliderValueFn       uiGetSliderValue     = nullptr;
static UISetSliderValueFn       uiSetSliderValue     = nullptr;
static UISetOnSliderChangedFn   uiSetOnSliderChanged = nullptr;
static UICreateColorPickerFn    uiCreateColorPicker    = nullptr;
static UIGetColorPickerColorFn  uiGetColorPickerColor  = nullptr;
static UISetOnColorChangedFn    uiSetOnColorChanged    = nullptr;
static UISetClosedSwatchSizeFn  uiSetClosedSwatchSize  = nullptr;
static UISetClosedFontSizeFn    uiSetClosedFontSize    = nullptr;
static UISetClosedTextColorFn   uiSetClosedTextColor   = nullptr;
static UISetPopupBGColorFn      uiSetPopupBGColor      = nullptr;

static void* g_btnHandle      = nullptr;
static void* g_checkHandle    = nullptr;
static void* g_editHandle     = nullptr;
static void* g_progressHandle = nullptr;
static void* g_panelHandle    = nullptr;
static void* g_textAreaHandle = nullptr;
static void* g_chkStatus      = nullptr;
static void* g_prgStatus      = nullptr;
static void* g_edtStatus      = nullptr;
static void* g_winFrameHandle = nullptr;
static void* g_imgBtnHandle   = nullptr;  // 图片测试按钮
static void* g_aniBtnHandle   = nullptr;  // LuotiAni 测试按钮
static void* g_sliderHandle   = nullptr;
static void* g_colorPickerHandle = nullptr;

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

int main() {
    printf("=== test_fromsource_raylib: UICornerstone.dll + raylib backend ===\n");

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) {
        printf("FAIL: LoadLibrary(UICornerstone.dll)\n");
        return 1;
    }
    printf("OK: loaded UICornerstone.dll\n"); fflush(stdout);

    #define GET_PROC(name) (void*)GetProcAddress(g_uiDll, "UICornerstone_" name)
    uiInit             = (UIInitFn)GET_PROC("Init");
    uiSetViewport      = (UISetViewportFn)GET_PROC("SetViewport");
    uiProcessEvents    = (UIProcessEventsFn)GET_PROC("ProcessEvents");
    uiUpdate           = (UIUpdateFn)GET_PROC("Update");
    uiClear            = (UIClearFn)GET_PROC("Clear");
    uiRender           = (UIRenderFn)GET_PROC("Render");
    uiPresent          = (UIPresentFn)GET_PROC("Present");
    uiIsQuit           = (UIIsQuitFn)GET_PROC("IsQuitRequested");
    uiShutdown         = (UIShutdownFn)GET_PROC("Shutdown");
    uiCreateButton     = (UICreateButtonFn)GET_PROC("CreateButton");
    uiCreateLabel      = (UICreateLabelFn)GET_PROC("CreateLabel");
    uiCreateCheckBox   = (UICreateCheckBoxFn)GET_PROC("CreateCheckBox");
    uiCreateEditBox    = (UICreateEditBoxFn)GET_PROC("CreateEditBox");
    uiCreateProgressBar = (UICreateProgressBarFn)GET_PROC("CreateProgressBar");
    uiCreatePanel      = (UICreatePanelFn)GET_PROC("CreatePanel");
    uiCreateTextArea   = (UICreateTextAreaFn)GET_PROC("CreateTextArea");
    uiCreateWinFrame   = (UICreateWinFrameFn)GET_PROC("CreateWinFrame");
    uiSetBGColor       = (UISetBGColorFn)GET_PROC("SetBGColor");
    uiSetText          = (UISetTextFn)GET_PROC("SetText");
    uiSetProgress      = (UISetProgressFn)GET_PROC("SetProgress");
    uiSetChecked       = (UISetCheckedFn)GET_PROC("SetChecked");
    uiAddChild         = (UIAddChildFn)GET_PROC("AddChild");
    uiGetText          = (UIGetTextFn)GET_PROC("GetText");
    uiGetChecked       = (UIGetCheckedFn)GET_PROC("GetChecked");
    uiGetProgress      = (UIGetProgressFn)GET_PROC("GetProgress");
    uiSetOnClick       = (UISetOnClickFn)GET_PROC("SetOnClick");
    uiSetVisible       = (UISetVisibleFn)GET_PROC("SetVisible");
    uiDestroyControl   = (UIDestroyControlFn)GET_PROC("DestroyControl");
    uiSetWinFrameClientText = (UIWinFrameSetClientTextFn)GET_PROC("WinFrameSetClientText");
    uiCreateImageButton  = (UICreateImageButtonFn)GET_PROC("CreateImageButton");
    uiSetButtonAnimation = (UISetButtonAnimationFn)GET_PROC("SetButtonAnimation");
    uiCreateSlider       = (UICreateSliderFn)GET_PROC("CreateSlider");
    uiGetSliderValue     = (UIGetSliderValueFn)GET_PROC("GetSliderValue");
    uiSetSliderValue     = (UISetSliderValueFn)GET_PROC("SetSliderValue");
    uiSetOnSliderChanged = (UISetOnSliderChangedFn)GET_PROC("SetOnSliderChanged");
    uiCreateColorPicker    = (UICreateColorPickerFn)GET_PROC("CreateColorPicker");
    uiGetColorPickerColor  = (UIGetColorPickerColorFn)GET_PROC("GetColorPickerColor");
    uiSetOnColorChanged    = (UISetOnColorChangedFn)GET_PROC("SetOnColorChanged");
    uiSetClosedSwatchSize  = (UISetClosedSwatchSizeFn)GET_PROC("SetClosedSwatchSize");
    uiSetClosedFontSize    = (UISetClosedFontSizeFn)GET_PROC("SetClosedFontSize");
    uiSetClosedTextColor   = (UISetClosedTextColorFn)GET_PROC("SetClosedTextColor");
    uiSetPopupBGColor      = (UISetPopupBGColorFn)GET_PROC("SetPopupBGColor");
    #undef GET_PROC

    if (!uiInit) {
        printf("FAIL: GetProcAddress(UICornerstone_Init)\n");
        FreeLibrary(g_uiDll);
        return 1;
    }

    // 后端自动创建 raylib 窗口
    void* cbs = GetUIBackendCallbacks();
    if (!cbs) {
        printf("FAIL: GetUIBackendCallbacks\n");
        FreeLibrary(g_uiDll);
        return 1;
    }
    if (!uiInit(cbs)) {
        printf("FAIL: UICornerstone_Init\n");
        FreeLibrary(g_uiDll);
        return 1;
    }

    // 后端已创建窗口后再设视口
    uiSetViewport(0, 0, 800, 480);
    g_uiInitialized = true;
    printf("OK: UICornerstone initialized (raylib backend)\n"); fflush(stdout);

    // ============================================================
    // 创建测试控件（与 SDL3 版本相同布局）
    // ============================================================
    // 分组 1: CheckBox + 状态标签（靠左）
    if (uiCreateCheckBox) {
        g_checkHandle = uiCreateCheckBox("Check me", 20, 15, 180, 30);
        if (g_checkHandle) {
            printf("OK: created CheckBox\n");
            if (uiSetChecked) uiSetChecked(g_checkHandle, 1);
        }
    }
    if (uiCreateLabel) {
        g_chkStatus = uiCreateLabel(u8"CheckBox: Checked", 12.0f, 20, 50, 180, 16);
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
        g_prgStatus = uiCreateLabel(u8"Progress: 0.0%", 12.0f, 20, 105, 230, 16);
        if (g_prgStatus) printf("OK: created prgStatus\n");
    }
    // 分组 4: Panel + TextArea + Button（按钮放在面板内）
    if (uiCreatePanel && uiCreateTextArea && uiAddChild) {
        g_panelHandle = uiCreatePanel(20, 135, 760, 330);
        if (g_panelHandle) {
            printf("OK: created Panel\n");
            if (uiSetBGColor) uiSetBGColor(g_panelHandle, 50, 55, 60, 255);
        }
        g_textAreaHandle = uiCreateTextArea(5, 5, 750, 260);
        if (g_textAreaHandle) {
            printf("OK: created TextArea\n");
            if (uiSetText) uiSetText(g_textAreaHandle, "Hello from TextArea!\nEdit me and click the button.");
            uiAddChild(g_panelHandle, g_textAreaHandle);
            printf("OK: added TextArea to Panel\n");
        }
    // Slider: range 0-100
    if (uiCreateSlider) {
        g_sliderHandle = uiCreateSlider(20, 470, 300, 30, 0, 100, 50);
        if (g_sliderHandle) {
            printf("OK: created Slider\n");
        }
    }

    // ColorPicker: hex color picker with presets
    if (uiCreateColorPicker) {
        g_colorPickerHandle = uiCreateColorPicker(340, 470, 96, 24, "#FF6600");
        if (g_colorPickerHandle) {
            printf("OK: created ColorPicker\n");
            if (uiSetClosedSwatchSize)
                uiSetClosedSwatchSize(g_colorPickerHandle, 16.0f);
            if (uiSetClosedFontSize)
                uiSetClosedFontSize(g_colorPickerHandle, 12);
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
                if (uiSetBGColor)
                    uiSetBGColor(g_imgBtnHandle, 40, 40, 40, 255);   // 深灰色背景
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
                if (uiSetBGColor)
                    uiSetBGColor(g_aniBtnHandle, 40, 40, 40, 255);   // 深灰色背景
                uiSetButtonAnimation(g_aniBtnHandle, "assets/animations/rotateBtn/rotateBtn.jsonc");
                if (uiSetOnClick)
                    uiSetOnClick(g_aniBtnHandle, onButtonClick, nullptr);
                uiAddChild(g_panelHandle, g_aniBtnHandle);
            }
        }
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

    // ============================================================
    // 帧循环（由 InputBackend 自动轮询 raylib 事件）
    // ============================================================
    printf("Starting frame loop...\n"); fflush(stdout);

    while (!uiIsQuit()) {
        g_frameCount++;

        // 进度条动画
        if (g_progressHandle && uiSetProgress) {
            float p = ((g_frameCount % 120) / 120.0f) * 100.0f;
            uiSetProgress(g_progressHandle, p);
        }

        uiProcessEvents();
        uiClear();
        uiUpdate(1.0 / 60.0);

        // Slider value polling
        if (g_sliderHandle && uiGetSliderValue) {
            float sv = uiGetSliderValue(g_sliderHandle);
            printf("Slider: %.1f\r", sv);
        }

        // ColorPicker polling
        if (g_colorPickerHandle && uiGetColorPickerColor) {
            char hex[16];
            uiGetColorPickerColor(g_colorPickerHandle, hex, sizeof(hex));
            printf("Color: %s\r", hex);
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
            snprintf(buf, sizeof(buf), u8"Progress: %.1f%%", v);
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
        uiPresent();
    }

    printf("Done, %d frames\n", g_frameCount); fflush(stdout);

    // ============================================================
    // 清理
    // ============================================================
    if (g_uiInitialized && uiShutdown) {
        uiShutdown();
        g_uiInitialized = false;
    }
    if (g_uiDll) {
        FreeLibrary(g_uiDll);
        g_uiDll = nullptr;
    }

    printf("test_fromsource_raylib: done\n");
    return 0;
}
