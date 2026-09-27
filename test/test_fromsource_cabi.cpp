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
typedef UIInstance (*UICreateInstanceFn)(const UIBackendCallbacks*, const UIInstanceConfig*);
typedef void  (*UISetViewportFn)(UIInstance,float,float,float,float);
typedef void  (*UIProcessEventsFn)(UIInstance);
typedef void  (*UIUpdateFn)(UIInstance,double);
typedef void  (*UIClearFn)(UIInstance);
typedef void  (*UIRenderFn)(UIInstance);
typedef void  (*UIPresentFn)(UIInstance);
typedef int   (*UIIsQuitFn)(UIInstance);
typedef void  (*UIDestroyInstanceFn)(UIInstance);
typedef void* (*UICreateButtonFn)(UIInstance,const char*,float,float,float,float,float,float);
typedef void* (*UICreateLabelFn)(UIInstance,const char*,float,float,float,float,float,float,float);
typedef void* (*UICreateCheckBoxFn)(UIInstance,const char*,float,float,float,float,float,float);
typedef void* (*UICreateEditBoxFn)(UIInstance,float,float,float,float,float,float);
typedef void* (*UICreateProgressBarFn)(UIInstance,float,float,float,float,float,float);
typedef void* (*UICreatePanelFn)(UIInstance,float,float,float,float,float,float);
typedef void* (*UICreateTextAreaFn)(UIInstance,float,float,float,float,float,float);
typedef void* (*UICreateWinFrameFn)(UIInstance,const char*,float,float,float,float,float,float);
typedef void  (*UIAddChildControlFn)(UIInstance,void*,void*);
typedef void  (*UIPushUIEventFn)(UIInstance,const UIEvent*);
typedef void  (*UIDestroyControlFn)(UIInstance,void*);
typedef void* (*UICreateImageButtonFn)(UIInstance,const char*,const char*,const char*,float,float,float,float,float,float);
typedef void* (*UICreateSliderFn)(UIInstance,float,float,float,float,float,float,float,float,float);
typedef void* (*UICreateColorPickerFn)(UIInstance,float,float,float,float,const char*,float,float);
typedef void* (*UICreateNumericUpDownFn)(UIInstance,float,float,float,float,float,float);
typedef void* (*UICreateSplitterFn)(UIInstance,float,float,float,float,int,float,float);

// Property API function pointer types
typedef int   (*UISetColorFn)(UIInstance, void*, const char*, UIColor);
typedef int   (*UISetBoolFn)(UIInstance, void*, const char*, int);
typedef int   (*UISetFloatFn)(UIInstance, void*, const char*, float);
typedef int   (*UISetIntFn)(UIInstance, void*, const char*, int);
typedef int   (*UISetStringFn)(UIInstance, void*, const char*, const char*);
typedef int   (*UISetPtrFn)(UIInstance, void*, const char*, void*);
typedef int   (*UIGetStringFn)(UIInstance, void*, const char*, char*, int);
typedef int   (*UIGetFloatFn)(UIInstance, void*, const char*, float*);
typedef int   (*UIGetBoolFn)(UIInstance, void*, const char*, int*);
typedef int   (*UISetCallbackFn)(UIInstance, void*, const char*, UIEventCallback, void*);

// ===== C ABI function pointers =====
static UICreateInstanceFn   uiCreateInstance   = nullptr;
static UISetViewportFn      uiSetViewport      = nullptr;
static UIProcessEventsFn    uiProcessEvents    = nullptr;
static UIUpdateFn           uiUpdate           = nullptr;
static UIClearFn            uiClear            = nullptr;
static UIRenderFn           uiRender           = nullptr;
static UIPresentFn          uiPresent          = nullptr;
static UIIsQuitFn           uiIsQuit           = nullptr;
static UIDestroyInstanceFn  uiDestroyInstance  = nullptr;
static UICreateButtonFn     uiCreateButton     = nullptr;
static UICreateLabelFn      uiCreateLabel      = nullptr;
static UICreateCheckBoxFn   uiCreateCheckBox   = nullptr;
static UICreateEditBoxFn    uiCreateEditBox    = nullptr;
static UICreateProgressBarFn uiCreateProgressBar = nullptr;
static UICreatePanelFn      uiCreatePanel      = nullptr;
static UICreateTextAreaFn   uiCreateTextArea   = nullptr;
static UICreateWinFrameFn   uiCreateWinFrame   = nullptr;
static UIAddChildControlFn  uiAddChildControl  = nullptr;
static UIPushUIEventFn      uiPushUIEvent      = nullptr;
static UIDestroyControlFn   uiDestroyControl   = nullptr;
static UICreateImageButtonFn    uiCreateImageButton  = nullptr;
static UICreateSliderFn         uiCreateSlider       = nullptr;
static UICreateColorPickerFn    uiCreateColorPicker    = nullptr;
static UICreateNumericUpDownFn  uiCreateNumericUpDown  = nullptr;
static UICreateSplitterFn           uiCreateSplitter           = nullptr;
// New property API function pointers
static UISetColorFn    uiSetColor    = nullptr;
static UISetBoolFn     uiSetBool     = nullptr;
static UISetFloatFn    uiSetFloat    = nullptr;
static UISetIntFn      uiSetInt      = nullptr;
static UISetStringFn   uiSetString   = nullptr;
static UIGetStringFn   uiGetString   = nullptr;
static UIGetFloatFn    uiGetFloat    = nullptr;
static UIGetBoolFn     uiGetBool     = nullptr;
static UISetCallbackFn uiSetCallback = nullptr;
static UISetPtrFn      uiSetPtr      = nullptr;

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
static void* g_nudHandle      = nullptr;
static void* g_splitterHandle = nullptr;
static void* g_spFirstPanel   = nullptr;
static void* g_spSecondPanel  = nullptr;
static void* g_winFrameLabel  = nullptr;

static HMODULE  g_uiDll = nullptr;
static UIInstance g_inst = nullptr;
static int     g_frameCount = 0;
static int     g_autoSec = 0;   // auto=<秒>：到时注入 WINDOW_CLOSE 自行退出（无人值守）

// ===== Button callback =====
static void onButtonClick(void* ctl, const UIEventData* evt, void* userData) {
    (void)ctl; (void)evt; (void)userData;
    printf("Button clicked! Showing WinFrame with TextArea content...\n");
    fflush(stdout);
    if (!g_textAreaHandle || !uiGetString) return;

    char text[4096] = "";
    uiGetString(g_inst, g_textAreaHandle, "text", text, sizeof(text));

    if (!g_winFrameHandle) {
        g_winFrameHandle = uiCreateWinFrame(
            g_inst, "TextArea Content", 60, 40, 500, 300, 1.0f, 1.0f);
        if (!g_winFrameHandle) {
            printf("FAIL: creating WinFrame\n"); fflush(stdout);
            return;
        }
        g_winFrameLabel = uiCreateLabel(g_inst, text, 14.0f, 10, 35, 480, 240, 1.0f, 1.0f);
        if (g_winFrameLabel) uiAddChildControl(g_inst, g_winFrameHandle, g_winFrameLabel);
        printf("OK: created WinFrame with Label\n"); fflush(stdout);
    } else {
        if (uiSetBool) uiSetBool(g_inst, g_winFrameHandle, "visible", 1);
        if (g_winFrameLabel && uiSetString)
            uiSetString(g_inst, g_winFrameLabel, "caption", text);
    }
    if (uiSetString) {
        uiSetString(g_inst, g_winFrameHandle, "title", text);
    }
}

// ===== Splitter callback =====
static void onSplitterMovedCb(void* ctl, const UIEventData* evt, void* userData) {
    (void)ctl; (void)evt; (void)userData;
    float r = 0.0f;
    if (uiGetFloat) uiGetFloat(g_inst, g_splitterHandle, "ratio", &r);
    printf("Splitter ratio: %.3f\n", r);
    fflush(stdout);
}

// ===== Load all C ABI function pointers from DLL =====
static bool loadAllProcs(HMODULE dll) {
    uiCreateInstance = (UICreateInstanceFn)GetProcAddress(dll, "UICornerstone_CreateInstance");
    uiSetViewport   = (UISetViewportFn)GetProcAddress(dll, "UICornerstone_SetViewport");
    uiProcessEvents = (UIProcessEventsFn)GetProcAddress(dll, "UICornerstone_ProcessEvents");
    uiUpdate        = (UIUpdateFn)GetProcAddress(dll, "UICornerstone_Update");
    uiClear         = (UIClearFn)GetProcAddress(dll, "UICornerstone_Clear");
    uiRender        = (UIRenderFn)GetProcAddress(dll, "UICornerstone_Render");
    uiPresent       = (UIPresentFn)GetProcAddress(dll, "UICornerstone_Present");
    uiIsQuit        = (UIIsQuitFn)GetProcAddress(dll, "UICornerstone_IsQuitRequested");
    uiDestroyInstance = (UIDestroyInstanceFn)GetProcAddress(dll, "UICornerstone_DestroyInstance");
    uiCreateButton     = (UICreateButtonFn)GetProcAddress(dll, "UICornerstone_CreateButton");
    uiCreateLabel      = (UICreateLabelFn)GetProcAddress(dll, "UICornerstone_CreateLabel");
    uiCreateCheckBox   = (UICreateCheckBoxFn)GetProcAddress(dll, "UICornerstone_CreateCheckBox");
    uiCreateEditBox    = (UICreateEditBoxFn)GetProcAddress(dll, "UICornerstone_CreateEditBox");
    uiCreateProgressBar = (UICreateProgressBarFn)GetProcAddress(dll, "UICornerstone_CreateProgressBar");
    uiCreatePanel      = (UICreatePanelFn)GetProcAddress(dll, "UICornerstone_CreatePanel");
    uiCreateTextArea   = (UICreateTextAreaFn)GetProcAddress(dll, "UICornerstone_CreateTextArea");
    uiCreateWinFrame   = (UICreateWinFrameFn)GetProcAddress(dll, "UICornerstone_CreateWinFrame");
    uiAddChildControl  = (UIAddChildControlFn)GetProcAddress(dll, "UICornerstone_AddChildControl");
    uiPushUIEvent      = (UIPushUIEventFn)GetProcAddress(dll, "UICornerstone_PushUIEvent");
    uiDestroyControl   = (UIDestroyControlFn)GetProcAddress(dll, "UICornerstone_DestroyControl");
    uiCreateImageButton  = (UICreateImageButtonFn)GetProcAddress(dll, "UICornerstone_CreateImageButton");
    uiCreateSlider       = (UICreateSliderFn)GetProcAddress(dll, "UICornerstone_CreateSlider");
    uiCreateColorPicker    = (UICreateColorPickerFn)GetProcAddress(dll, "UICornerstone_CreateColorPicker");
    uiCreateNumericUpDown  = (UICreateNumericUpDownFn)GetProcAddress(dll, "UICornerstone_CreateNumericUpDown");
    uiCreateSplitter           = (UICreateSplitterFn)GetProcAddress(dll, "UICornerstone_CreateSplitter");
    // New property API
    uiSetColor    = (UISetColorFn)GetProcAddress(dll, "UICornerstone_SetColor");
    uiSetBool     = (UISetBoolFn)GetProcAddress(dll, "UICornerstone_SetBool");
    uiSetFloat    = (UISetFloatFn)GetProcAddress(dll, "UICornerstone_SetFloat");
    uiSetInt      = (UISetIntFn)GetProcAddress(dll, "UICornerstone_SetInt");
    uiSetString   = (UISetStringFn)GetProcAddress(dll, "UICornerstone_SetString");
    uiSetPtr      = (UISetPtrFn)GetProcAddress(dll, "UICornerstone_SetPtr");
    uiGetString   = (UIGetStringFn)GetProcAddress(dll, "UICornerstone_GetString");
    uiGetFloat    = (UIGetFloatFn)GetProcAddress(dll, "UICornerstone_GetFloat");
    uiGetBool     = (UIGetBoolFn)GetProcAddress(dll, "UICornerstone_GetBool");
    uiSetCallback = (UISetCallbackFn)GetProcAddress(dll, "UICornerstone_SetCallback");
    return uiCreateInstance != nullptr;
}

// ===== Common functions =====
static bool initCABI(const UIBackendCallbacks* cbs, int viewportW, int viewportH) {
    UIInstanceConfig cfg = UI_INSTANCE_CONFIG_DEFAULT;
    cfg.windowTitle = "test_fromsource_cabi";
    cfg.windowWidth = viewportW;
    cfg.windowHeight = viewportH;
    g_inst = uiCreateInstance(cbs, &cfg);
    if (!g_inst) return false;
    uiSetViewport(g_inst, 0, 0, (float)viewportW, (float)viewportH);
    return true;
}

static void createAllControls() {
    if (uiCreateCheckBox) {
        g_checkHandle = uiCreateCheckBox(g_inst, "Check me", 20, 15, 180, 30, 1.0f, 1.0f);
        if (g_checkHandle) {
            printf("OK: created CheckBox\n");
            if (uiSetBool) uiSetBool(g_inst, g_checkHandle, "checked", 1);
        }
    }
    if (uiCreateLabel) {
        g_chkStatus = uiCreateLabel(g_inst, "CheckBox: Checked", 12.0f, 20, 50, 180, 16, 1.0f, 1.0f);
        if (g_chkStatus) printf("OK: created chkStatus\n");
    }

    if (uiCreateEditBox) {
        g_editHandle = uiCreateEditBox(g_inst, 220, 15, 560, 30, 1.0f, 1.0f);
        if (g_editHandle) {
            printf("OK: created EditBox\n");
            if (uiSetString) uiSetString(g_inst, g_editHandle, "text", "Type here...");
        }
    }
    if (uiCreateLabel) {
        g_edtStatus = uiCreateLabel(g_inst, "Edit: ", 12.0f, 220, 50, 560, 16, 1.0f, 1.0f);
        if (g_edtStatus) printf("OK: created edtStatus\n");
    }

    if (uiCreateProgressBar) {
        g_progressHandle = uiCreateProgressBar(g_inst, 20, 80, 760, 20, 1.0f, 1.0f);
        if (g_progressHandle) {
            printf("OK: created ProgressBar\n");
            if (uiSetColor) uiSetColor(g_inst, g_progressHandle, "background", UIColor{60, 60, 60, 255});
            if (uiSetFloat) uiSetFloat(g_inst, g_progressHandle, "value", 0.0f);
        }
    }
    if (uiCreateLabel) {
        g_prgStatus = uiCreateLabel(g_inst, "Progress: 0.0%", 12.0f, 20, 105, 230, 16, 1.0f, 1.0f);
        if (g_prgStatus) printf("OK: created prgStatus\n");
    }

    if (uiCreatePanel && uiCreateTextArea && uiAddChildControl) {
        g_panelHandle = uiCreatePanel(g_inst, 20, 135, 760, 220, 1.0f, 1.0f);
        if (g_panelHandle) {
            printf("OK: created Panel\n");
            if (uiSetColor) uiSetColor(g_inst, g_panelHandle, "background", UIColor{50, 55, 60, 255});
        }

        g_textAreaHandle = uiCreateTextArea(g_inst, 5, 5, 750, 160, 1.0f, 1.0f);
        if (g_textAreaHandle) {
            printf("OK: created TextArea\n");
            if (uiSetString) uiSetString(g_inst, g_textAreaHandle, "text",
                "Hello from TextArea!\nEdit me and click the button.");
            uiAddChildControl(g_inst, g_panelHandle, g_textAreaHandle);
            printf("OK: added TextArea to Panel\n");
        }

        if (uiCreateSlider) {
            g_sliderHandle = uiCreateSlider(g_inst, 20, 470, 300, 30, 0, 100, 50, 1.0f, 1.0f);
            if (g_sliderHandle) printf("OK: created Slider\n");
        }

        if (uiCreateColorPicker) {
            g_colorPickerHandle = uiCreateColorPicker(g_inst, 450, 470, 96, 24, "#FF6600", 1.0f, 1.0f);
            if (g_colorPickerHandle) {
                printf("OK: created ColorPicker\n");
                if (uiSetFloat)
                    uiSetFloat(g_inst, g_colorPickerHandle, "closed-swatch-size", 16.0f);
                if (uiSetInt)
                    uiSetInt(g_inst, g_colorPickerHandle, "closed-font-size", 12);
            }
        }

        if (uiCreateNumericUpDown) {
            g_nudHandle = uiCreateNumericUpDown(g_inst, 360, 470, 80, 32, 1.0f, 1.0f);
            if (g_nudHandle) {
                printf("OK: created NumericUpDown\n");
                if (uiSetFloat)
                    uiSetFloat(g_inst, g_nudHandle, "value", 50.0f);
            }
        }

        // ── Splitter test ──
        if (uiCreateSplitter && uiCreatePanel && uiAddChildControl) {
            g_spFirstPanel = uiCreatePanel(g_inst, 20, 370, 60, 30, 1.0f, 1.0f);
            if (g_spFirstPanel) {
                printf("OK: created splitter first panel\n");
                if (uiSetColor) uiSetColor(g_inst, g_spFirstPanel, "background", UIColor{60, 70, 80, 255});
            }
            g_spSecondPanel = uiCreatePanel(g_inst, 86, 370, 60, 30, 1.0f, 1.0f);
            if (g_spSecondPanel) {
                printf("OK: created splitter second panel\n");
                if (uiSetColor) uiSetColor(g_inst, g_spSecondPanel, "background", UIColor{80, 70, 60, 255});
            }
            g_splitterHandle = uiCreateSplitter(g_inst, 80, 370, 6, 30, 1, 1.0f, 1.0f);
            if (g_splitterHandle) {
                printf("OK: created Splitter\n");
                if (uiSetPtr) {
                    uiSetPtr(g_inst, g_splitterHandle, "first-linked", g_spFirstPanel);
                    uiSetPtr(g_inst, g_splitterHandle, "second-linked", g_spSecondPanel);
                }
                if (uiSetFloat) {
                    uiSetFloat(g_inst, g_splitterHandle, "first-min", 20.0f);
                    uiSetFloat(g_inst, g_splitterHandle, "second-min", 20.0f);
                }
                if (uiSetCallback)
                    uiSetCallback(g_inst, g_splitterHandle, "moved", onSplitterMovedCb, nullptr);
            }
        }

        if (uiCreateButton && uiSetString) {   // P0-31：ImageButton 已移除 → 状态图 Button 等价用例
            g_imgBtnHandle = uiCreateButton(g_inst, "", 5, 175, 200, 30, 1.0f, 1.0f);
            if (g_imgBtnHandle) {
                uiSetString(g_inst, g_imgBtnHandle, "normal-image", "assets/images/cross_up.png");
                uiSetString(g_inst, g_imgBtnHandle, "hover-image", "assets/images/cross_over.png");
                uiSetString(g_inst, g_imgBtnHandle, "pressed-image", "assets/images/cross_down.png");
                printf("OK: created state-image Button (ImageButton 等价)\n");
                if (uiSetCallback)
                    uiSetCallback(g_inst, g_imgBtnHandle, "click", onButtonClick, nullptr);
                uiAddChildControl(g_inst, g_panelHandle, g_imgBtnHandle);
            }
        }

        if (uiCreateButton && uiSetString) {
            g_aniBtnHandle = uiCreateButton(g_inst, "Ani Test", 210, 175, 200, 30, 1.0f, 1.0f);
            if (g_aniBtnHandle) {
                printf("OK: created Animation Button\n");
                uiSetString(g_inst, g_aniBtnHandle, "animation",
                    "assets/animations/rotateBtn/rotateBtn.jsonc");
                if (uiSetCallback)
                    uiSetCallback(g_inst, g_aniBtnHandle, "click", onButtonClick, nullptr);
                uiAddChildControl(g_inst, g_panelHandle, g_aniBtnHandle);
            }
        }

        if (uiCreateButton) {
            g_btnHandle = uiCreateButton(
                g_inst, "Read TextArea Content", 555, 175, 200, 30, 1.0f, 1.0f);
            if (g_btnHandle) {
                printf("OK: created Button (in Panel)\n");
                if (uiSetColor)
                    uiSetColor(g_inst, g_btnHandle, "background", UIColor{100, 149, 237, 255});
                if (uiSetCallback)
                    uiSetCallback(g_inst, g_btnHandle, "click", onButtonClick, nullptr);
                uiAddChildControl(g_inst, g_panelHandle, g_btnHandle);
                printf("OK: added Button to Panel\n");
            }
        }
    }
    fflush(stdout);
}

static void updateStatusLabels() {
    char buf[256];

    if (g_checkHandle && uiGetBool && g_chkStatus && uiSetString) {
        int st = 0;
        uiGetBool(g_inst, g_checkHandle, "checked", &st);
        const char* label = "Unchecked";
        if (st == 1) label = "Checked";
        snprintf(buf, sizeof(buf), "CheckBox: %s", label);
        uiSetString(g_inst, g_chkStatus, "caption", buf);
    }

    if (g_progressHandle && uiGetFloat && g_prgStatus && uiSetString) {
        float v = 0.0f;
        uiGetFloat(g_inst, g_progressHandle, "value", &v);
        snprintf(buf, sizeof(buf), "Progress: %.1f%%", v);
        uiSetString(g_inst, g_prgStatus, "caption", buf);
    }

    if (g_editHandle && uiGetString && g_edtStatus && uiSetString) {
        char text[4096] = "";
        uiGetString(g_inst, g_editHandle, "text", text, sizeof(text));
        size_t tlen = strlen(text);
        if (tlen > 32) {
            memcpy(buf, text, 32);
            buf[32] = '\0';
            snprintf(buf + 32, sizeof(buf) - 32, "...(%zu)", tlen);
        } else {
            snprintf(buf, sizeof(buf), "Edit: %s", text);
        }
        uiSetString(g_inst, g_edtStatus, "caption", buf);
    }

    if (g_sliderHandle && uiGetFloat) {
        float sv = 0.0f;
        uiGetFloat(g_inst, g_sliderHandle, "value", &sv);
    }

    if (g_nudHandle && uiGetFloat) {
        float nv = 0.0f;
        uiGetFloat(g_inst, g_nudHandle, "value", &nv);
    }

    if (g_colorPickerHandle && uiGetString) {
        char hex[16];
        uiGetString(g_inst, g_colorPickerHandle, "color", hex, sizeof(hex));
    }

    if (g_splitterHandle && uiGetFloat) {
        float sr = 0.0f;
        uiGetFloat(g_inst, g_splitterHandle, "ratio", &sr);
    }
}

static void doFrame() {
    g_frameCount++;

    if (g_progressHandle && uiSetFloat) {
        float p = ((g_frameCount % 120) / 120.0f) * 100.0f;
        uiSetFloat(g_inst, g_progressHandle, "value", p);
    }

    uiProcessEvents(g_inst);
    uiClear(g_inst);
    uiUpdate(g_inst, 1.0 / 60.0);

    updateStatusLabels();

    uiRender(g_inst);
}

static void shutdownApp() {
    if (g_inst && uiDestroyInstance) {
        uiDestroyInstance(g_inst);
        g_inst = nullptr;
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

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks || !initCABI(callbacks, 800, 550)) {
        printf("FAIL: UICornerstone_CreateInstance\n");
        FreeLibrary(g_uiDll);
        return 1;
    }
    printf("OK: UICornerstone initialized (%s backend)\n", displayName);
    fflush(stdout);

    createAllControls();
    printf("Starting frame loop...\n"); fflush(stdout);

    ULONGLONG autoT0 = GetTickCount64();
    while (!uiIsQuit(g_inst)) {
        if (g_autoSec > 0 && (GetTickCount64() - autoT0) >= (ULONGLONG)g_autoSec * 1000) {
            UIEvent ue; memset(&ue, 0, sizeof(ue)); ue.type = UI_EVENT_WINDOW_CLOSE;
            uiPushUIEvent(g_inst, &ue);
        }
        doFrame();
        uiPresent(g_inst);
    }

    printf("Done, %d frames\n", g_frameCount); fflush(stdout);
    shutdownApp();
    printf("test_fromsource_%s: done\n", shortName);
    return 0;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "auto=", 5) == 0) g_autoSec = atoi(argv[i] + 5);
    }
    return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME);
}
