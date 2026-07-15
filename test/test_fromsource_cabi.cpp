// =========================================================================
// test_fromsource_cabi.cpp -- single fromsource C ABI test for all backends
// Backend name provided via -DBACKEND_SHORT_NAME / -DBACKEND_DISPLAY_NAME
// =========================================================================

#define NOMINMAX
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <windows.h>

#include "../../include/UICornerstoneAPI.h"

extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);

// ===== C ABI function pointer types =====
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
typedef void* (*UICreateColorPickerFn)(float,float,float,float,const char*);
typedef void  (*UIGetColorPickerColorFn)(void*,char*,int);
typedef void  (*UISetOnColorChangedFn)(void*, void (*)(void*,void*), void*);
typedef void  (*UISetClosedSwatchSizeFn)(void*,float);
typedef void  (*UISetClosedFontSizeFn)(void*,int);
typedef void  (*UISetClosedTextColorFn)(void*,const char*);
typedef void  (*UISetPopupBGColorFn)(void*,const char*);

// ===== C ABI function pointers =====
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
static UICreateColorPickerFn    uiCreateColorPicker    = nullptr;
static UIGetColorPickerColorFn  uiGetColorPickerColor  = nullptr;
static UISetOnColorChangedFn    uiSetOnColorChanged    = nullptr;
static UISetClosedSwatchSizeFn  uiSetClosedSwatchSize  = nullptr;
static UISetClosedFontSizeFn    uiSetClosedFontSize    = nullptr;
static UISetClosedTextColorFn   uiSetClosedTextColor   = nullptr;
static UISetPopupBGColorFn      uiSetPopupBGColor      = nullptr;

// ===== Control handle globals =====
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
static void* g_imgBtnHandle   = nullptr;
static void* g_aniBtnHandle   = nullptr;
static void* g_sliderHandle   = nullptr;
static void* g_colorPickerHandle = nullptr;

static HMODULE g_uiDll = nullptr;
static bool    g_uiInitialized = false;
static int     g_frameCount = 0;

// ===== Button callback =====
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

// ===== Load all C ABI function pointers from DLL =====
static bool loadAllProcs(HMODULE dll) {
    uiInit          = (UIInitFn)GetProcAddress(dll, "UICornerstone_Init");
    uiSetViewport   = (UISetViewportFn)GetProcAddress(dll, "UICornerstone_SetViewport");
    uiProcessEvents = (UIProcessEventsFn)GetProcAddress(dll, "UICornerstone_ProcessEvents");
    uiUpdate        = (UIUpdateFn)GetProcAddress(dll, "UICornerstone_Update");
    uiClear         = (UIClearFn)GetProcAddress(dll, "UICornerstone_Clear");
    uiRender        = (UIRenderFn)GetProcAddress(dll, "UICornerstone_Render");
    uiPresent       = (UIPresentFn)GetProcAddress(dll, "UICornerstone_Present");
    uiIsQuit        = (UIIsQuitFn)GetProcAddress(dll, "UICornerstone_IsQuitRequested");
    uiShutdown      = (UIShutdownFn)GetProcAddress(dll, "UICornerstone_Shutdown");
    uiCreateButton     = (UICreateButtonFn)GetProcAddress(dll, "UICornerstone_CreateButton");
    uiCreateLabel      = (UICreateLabelFn)GetProcAddress(dll, "UICornerstone_CreateLabel");
    uiCreateCheckBox   = (UICreateCheckBoxFn)GetProcAddress(dll, "UICornerstone_CreateCheckBox");
    uiCreateEditBox    = (UICreateEditBoxFn)GetProcAddress(dll, "UICornerstone_CreateEditBox");
    uiCreateProgressBar = (UICreateProgressBarFn)GetProcAddress(dll, "UICornerstone_CreateProgressBar");
    uiCreatePanel      = (UICreatePanelFn)GetProcAddress(dll, "UICornerstone_CreatePanel");
    uiCreateTextArea   = (UICreateTextAreaFn)GetProcAddress(dll, "UICornerstone_CreateTextArea");
    uiCreateWinFrame   = (UICreateWinFrameFn)GetProcAddress(dll, "UICornerstone_CreateWinFrame");
    uiSetBGColor       = (UISetBGColorFn)GetProcAddress(dll, "UICornerstone_SetBGColor");
    uiSetText          = (UISetTextFn)GetProcAddress(dll, "UICornerstone_SetText");
    uiSetProgress      = (UISetProgressFn)GetProcAddress(dll, "UICornerstone_SetProgress");
    uiSetChecked       = (UISetCheckedFn)GetProcAddress(dll, "UICornerstone_SetChecked");
    uiAddChild         = (UIAddChildFn)GetProcAddress(dll, "UICornerstone_AddChild");
    uiGetText          = (UIGetTextFn)GetProcAddress(dll, "UICornerstone_GetText");
    uiGetChecked       = (UIGetCheckedFn)GetProcAddress(dll, "UICornerstone_GetChecked");
    uiGetProgress      = (UIGetProgressFn)GetProcAddress(dll, "UICornerstone_GetProgress");
    uiPushUIEvent      = (UIPushUIEventFn)GetProcAddress(dll, "UICornerstone_PushUIEvent");
    uiSetOnClick       = (UISetOnClickFn)GetProcAddress(dll, "UICornerstone_SetOnClick");
    uiSetVisible       = (UISetVisibleFn)GetProcAddress(dll, "UICornerstone_SetVisible");
    uiDestroyControl   = (UIDestroyControlFn)GetProcAddress(dll, "UICornerstone_DestroyControl");
    uiSetWinFrameClientText = (UIWinFrameSetClientTextFn)GetProcAddress(dll, "UICornerstone_WinFrameSetClientText");
    uiCreateImageButton  = (UICreateImageButtonFn)GetProcAddress(dll, "UICornerstone_CreateImageButton");
    uiSetButtonAnimation = (UISetButtonAnimationFn)GetProcAddress(dll, "UICornerstone_SetButtonAnimation");
    uiCreateSlider       = (UICreateSliderFn)GetProcAddress(dll, "UICornerstone_CreateSlider");
    uiGetSliderValue     = (UIGetSliderValueFn)GetProcAddress(dll, "UICornerstone_GetSliderValue");
    uiSetSliderValue     = (UISetSliderValueFn)GetProcAddress(dll, "UICornerstone_SetSliderValue");
    uiSetOnSliderChanged = (UISetOnSliderChangedFn)GetProcAddress(dll, "UICornerstone_SetOnSliderChanged");
    uiCreateColorPicker    = (UICreateColorPickerFn)GetProcAddress(dll, "UICornerstone_CreateColorPicker");
    uiGetColorPickerColor  = (UIGetColorPickerColorFn)GetProcAddress(dll, "UICornerstone_GetColorPickerColor");
    uiSetOnColorChanged    = (UISetOnColorChangedFn)GetProcAddress(dll, "UICornerstone_SetOnColorChanged");
    uiSetClosedSwatchSize  = (UISetClosedSwatchSizeFn)GetProcAddress(dll, "UICornerstone_SetClosedSwatchSize");
    uiSetClosedFontSize    = (UISetClosedFontSizeFn)GetProcAddress(dll, "UICornerstone_SetClosedFontSize");
    uiSetClosedTextColor   = (UISetClosedTextColorFn)GetProcAddress(dll, "UICornerstone_SetClosedTextColor");
    uiSetPopupBGColor      = (UISetPopupBGColorFn)GetProcAddress(dll, "UICornerstone_SetPopupBGColor");
    return uiInit != nullptr;
}

// ===== Common functions =====
static bool initCABI(void* cbs, int viewportW, int viewportH) {
    if (!uiInit(cbs)) return false;
    uiSetViewport(0, 0, (float)viewportW, (float)viewportH);
    g_uiInitialized = true;
    return true;
}

static void createAllControls() {
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

    if (uiCreatePanel && uiCreateTextArea && uiAddChild) {
        g_panelHandle = uiCreatePanel(20, 135, 760, 330);
        if (g_panelHandle) {
            printf("OK: created Panel\n");
            if (uiSetBGColor) uiSetBGColor(g_panelHandle, 50, 55, 60, 255);
        }

        g_textAreaHandle = uiCreateTextArea(5, 5, 750, 260);
        if (g_textAreaHandle) {
            printf("OK: created TextArea\n");
            if (uiSetText) uiSetText(g_textAreaHandle,
                "Hello from TextArea!\nEdit me and click the button.");
            uiAddChild(g_panelHandle, g_textAreaHandle);
            printf("OK: added TextArea to Panel\n");
        }

        if (uiCreateSlider) {
            g_sliderHandle = uiCreateSlider(20, 470, 300, 30, 0, 100, 50);
            if (g_sliderHandle) printf("OK: created Slider\n");
        }

        if (uiCreateColorPicker) {
            g_colorPickerHandle = uiCreateColorPicker(450, 470, 96, 24, "#FF6600");
            if (g_colorPickerHandle) {
                printf("OK: created ColorPicker\n");
                if (uiSetClosedSwatchSize)
                    uiSetClosedSwatchSize(g_colorPickerHandle, 16.0f);
                if (uiSetClosedFontSize)
                    uiSetClosedFontSize(g_colorPickerHandle, 12);
            }
        }

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

        if (uiCreateButton && uiSetButtonAnimation) {
            g_aniBtnHandle = uiCreateButton("Ani Test", 210, 270, 200, 30);
            if (g_aniBtnHandle) {
                printf("OK: created Animation Button\n");
                uiSetButtonAnimation(g_aniBtnHandle,
                    "assets/animations/rotateBtn/rotateBtn.jsonc");
                if (uiSetOnClick)
                    uiSetOnClick(g_aniBtnHandle, onButtonClick, nullptr);
                uiAddChild(g_panelHandle, g_aniBtnHandle);
            }
        }

        if (uiCreateButton) {
            g_btnHandle = uiCreateButton(
                "Read TextArea Content", 555, 270, 200, 30);
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
}

static void updateStatusLabels() {
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

    if (g_sliderHandle && uiGetSliderValue) {
        float sv = uiGetSliderValue(g_sliderHandle);
        printf("Slider: %.1f\r", sv);
    }

    if (g_colorPickerHandle && uiGetColorPickerColor) {
        char hex[16];
        uiGetColorPickerColor(g_colorPickerHandle, hex, sizeof(hex));
        printf("Color: %s\r", hex);
    }
}

static void doFrame() {
    g_frameCount++;

    if (g_progressHandle && uiSetProgress) {
        float p = ((g_frameCount % 120) / 120.0f) * 100.0f;
        uiSetProgress(g_progressHandle, p);
    }

    uiProcessEvents();
    uiClear();
    uiUpdate(1.0 / 60.0);

    updateStatusLabels();

    uiRender();
}

static void shutdownApp() {
    if (g_uiInitialized && uiShutdown) {
        uiShutdown();
        g_uiInitialized = false;
    }
    if (g_uiDll) {
        FreeLibrary(g_uiDll);
        g_uiDll = nullptr;
    }
}

static int runTest(const char* shortName, const char* displayName) {
    printf("=== test_fromsource_%s: UICornerstone.dll + %s backend ===\n",
           shortName, displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n"); fflush(stdout);

    if (!loadAllProcs(g_uiDll)) {
        printf("FAIL: GetProcAddress\n");
        FreeLibrary(g_uiDll);
        return 1;
    }

    void* cbs = GetUIBackendCallbacks();
    if (!cbs || !initCABI(cbs, 800, 480)) {
        printf("FAIL: UICornerstone_Init\n");
        FreeLibrary(g_uiDll);
        return 1;
    }
    printf("OK: UICornerstone initialized (%s backend)\n", displayName);
    fflush(stdout);

    createAllControls();
    printf("Starting frame loop...\n"); fflush(stdout);

    while (!uiIsQuit()) {
        doFrame();
        uiPresent();
    }

    printf("Done, %d frames\n", g_frameCount); fflush(stdout);
    shutdownApp();
    printf("test_fromsource_%s: done\n", shortName);
    return 0;
}

int main() { return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME); }
