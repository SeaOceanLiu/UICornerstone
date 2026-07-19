#ifndef UICORNERSTONE_API_H
#define UICORNERSTONE_API_H

#include <stdint.h>
#include <stddef.h>

/* DLL export/import macro */
#if defined(UICORNERSTONE_BUILD_SHARED)
#  if defined(UICORNERSTONE_API_EXPORT)
#    define UICORNERSTONE_API __declspec(dllexport)
#  else
#    define UICORNERSTONE_API __declspec(dllimport)
#  endif
#else
#  define UICORNERSTONE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 句柄类型 ============ */
typedef void* UIWindowHandle;
typedef void* UIRenderDeviceHandle;
typedef void* UIInputBackendHandle;
typedef void* UITextRendererHandle;
typedef void* UIResourceProviderHandle;
typedef void* UIControlHandle;
typedef void* UIFontHandle;
typedef void* UITextHandle;
typedef void* UITextureHandle;

/* ============ 基础类型 ============ */
typedef struct { float x, y, w, h; }   UIRect;
typedef struct { uint8_t r, g, b, a; } UIColor;

/* ============ 事件类型 ============ */
typedef enum {
    UI_EVENT_NONE = 0,
    UI_EVENT_MOUSE_MOVE,
    UI_EVENT_MOUSE_DOWN,
    UI_EVENT_MOUSE_UP,
    UI_EVENT_MOUSE_WHEEL,
    UI_EVENT_KEY_DOWN,
    UI_EVENT_KEY_UP,
    UI_EVENT_TEXT_INPUT,
    UI_EVENT_WINDOW_RESIZE,
    UI_EVENT_WINDOW_CLOSE,
    UI_EVENT_FOCUS_GAINED,
    UI_EVENT_FOCUS_LOST,
} UIEventType;

#define UI_EVENT_BUF_SIZE 128
#define UI_TEXT_MAX 32

typedef struct {
    UIEventType type;
    uint8_t data[UI_EVENT_BUF_SIZE];
} UIEvent;

/* 便捷访问宏 */
#define UI_EVENT_MOUSE_X(ev)     (*(float*)(ev)->data)
#define UI_EVENT_MOUSE_Y(ev)     (*(float*)((ev)->data + 4))
#define UI_EVENT_BUTTON(ev)      (*(int*)((ev)->data + 8))
#define UI_EVENT_WHEEL_DELTA(ev) (*(float*)(ev)->data)
#define UI_EVENT_WHEEL_MOUSE_X(ev) (*(float*)((ev)->data + 4))
#define UI_EVENT_WHEEL_MOUSE_Y(ev) (*(float*)((ev)->data + 8))
#define UI_EVENT_KEY_CODE(ev)    (*(int*)(ev)->data)
#define UI_EVENT_KEY_MOD(ev)     (*(uint16_t*)((ev)->data + 4))
#define UI_EVENT_TEXT(ev)        ((const char*)(ev)->data)
#define UI_EVENT_RESIZE_W(ev)    (*(int*)(ev)->data)
#define UI_EVENT_RESIZE_H(ev)    (*(int*)((ev)->data + 4))

/* ============ 后端回调表 ============ */
typedef struct {
    int version;  // 必须设为 1

    // --- Window (可选，只为 NULL) ---
    UIWindowHandle  (*createWindow)(const char* title, int w, int h, uint32_t flags);
    void            (*destroyWindow)(UIWindowHandle);
    void            (*getWindowSize)(UIWindowHandle, float* w, float* h);
    void            (*getWindowPosition)(UIWindowHandle, float* x, float* y);
    float           (*getDisplayWidth)(UIWindowHandle);
    float           (*getDisplayHeight)(UIWindowHandle);
    float           (*getDpiScale)(UIWindowHandle);
    void            (*setWindowTitle)(UIWindowHandle, const char*);
    int             (*getMousePosition)(UIWindowHandle, float* x, float* y);

    // --- RenderDevice (必须) ---
    UIRenderDeviceHandle (*createRenderDevice)(void* nativeContext);
    void                 (*destroyRenderDevice)(UIRenderDeviceHandle);
    void                 (*setDrawColor)(UIRenderDeviceHandle, UIColor);
    void                 (*setBlendMode)(UIRenderDeviceHandle, int mode);
    void                 (*setClipRect)(UIRenderDeviceHandle, float x, float y, float w, float h);
    void                 (*clearClipRect)(UIRenderDeviceHandle);
    void                 (*fillRect)(UIRenderDeviceHandle, float x, float y, float w, float h);
    void                 (*drawRect)(UIRenderDeviceHandle, float x, float y, float w, float h);
    void                 (*drawLine)(UIRenderDeviceHandle, float x1, float y1, float x2, float y2);
    void                 (*drawPoint)(UIRenderDeviceHandle, float x, float y);
    void                 (*clear)(UIRenderDeviceHandle, UIColor);
    void                 (*present)(UIRenderDeviceHandle);
    void                 (*flush)(UIRenderDeviceHandle);
    void*                (*getNativeHandle)(UIRenderDeviceHandle);
    // 纹理（可选，可 NULL—图片不可用时跳过）
    UITextureHandle      (*createTextureFromFile)(UIRenderDeviceHandle, const char* path);
    void                 (*destroyTexture)(UIRenderDeviceHandle, UITextureHandle);
    void                 (*drawTexture)(UIRenderDeviceHandle, UITextureHandle, const UIRect* src, const UIRect* dst);
    void                 (*getTextureSize)(UITextureHandle, int* w, int* h);

    // --- InputBackend (必须) ---
    UIInputBackendHandle (*createInputBackend)(void* nativeWindowHandle);
    void                 (*destroyInputBackend)(UIInputBackendHandle);
    int                  (*pollEvent)(UIInputBackendHandle, UIEvent* evt);
    void                 (*startTextInput)(UIInputBackendHandle);
    void                 (*stopTextInput)(UIInputBackendHandle);
    int                  (*getModState)(UIInputBackendHandle);
    void                 (*setClipboardText)(UIInputBackendHandle, const char* text);
    int                  (*getClipboardText)(UIInputBackendHandle, char* buf, int maxLen);

    // --- per-frame tick (可选，可为 NULL) ---
    // 在每个 ProcessEvents 开头被调用。raylib 后端用它调用 PollInputEvents()。
    void                 (*newFrame)(UIInputBackendHandle);

    // --- TextRenderer (必须) ---
    UITextRendererHandle (*createTextRenderer)(UIRenderDeviceHandle);
    void                 (*destroyTextRenderer)(UITextRendererHandle);
    UIFontHandle         (*loadFont)(UITextRendererHandle, const char* path, float size);
    UIFontHandle         (*loadFontFromMemory)(UITextRendererHandle, const void* data, int len, float size);
    void                 (*destroyFont)(UITextRendererHandle, UIFontHandle);
    float                (*measureTextWidth)(UITextRendererHandle, UIFontHandle, const char* text);
    float                (*getFontHeight)(UITextRendererHandle, UIFontHandle);
    void                 (*drawText)(UITextRendererHandle, UIFontHandle, const char* text, float x, float y, UIColor color);
    void                 (*drawTextWrapped)(UITextRendererHandle, UIFontHandle, const char* text, float x, float y, float wrapWidth, UIColor color);

    // --- filled triangle/quad (可选，可为 NULL — 退化到轮廓绘制) ---
    void (*fillTriangle)(UIRenderDeviceHandle, float x0, float y0, float x1, float y1, float x2, float y2, UIColor color);
    void (*fillQuad)(UIRenderDeviceHandle, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, UIColor color);

    // --- Cursor factories (可选 — 可为 NULL；禁用光标反馈) ---
    void* (*createSystemCursor)(int type);
    void* (*getDefaultCursor)();
    void  (*setCurrentCursor)(void* cursor);

    // --- ResourceProvider (可选，可为 NULL) ---
    UIResourceProviderHandle (*createResourceProvider)(const char* basePath);
    void                     (*destroyResourceProvider)(UIResourceProviderHandle);
    int                      (*readFile)(UIResourceProviderHandle, const char* path, void* buf, int maxLen);
    int                      (*fileExists)(UIResourceProviderHandle, const char* path);
} UIBackendCallbacks;

/* ============ 初始化 ============ */
UICORNERSTONE_API int  UICornerstone_Init(const UIBackendCallbacks* callbacks);
UICORNERSTONE_API void UICornerstone_Shutdown(void);
UICORNERSTONE_API int  UICornerstone_InitFromPlugin(const char* pluginName);

/* ============ 视口控制 ============ */
UICORNERSTONE_API void UICornerstone_SetViewport(float x, float y, float w, float h);
UICORNERSTONE_API void UICornerstone_GetViewport(float* x, float* y, float* w, float* h);

/* ============ 帧循环 ============ */
UICORNERSTONE_API void UICornerstone_ProcessEvents(void);
UICORNERSTONE_API void UICornerstone_Update(double deltaTime);

// 注入外部事件（例如从 SDL_AppEvent 回调传入的 SDL 事件）。
// 事件会在下一次 UICornerstone_ProcessEvents 中被处理。
UICORNERSTONE_API void UICornerstone_PushUIEvent(const UIEvent* ue);

// 只渲染视口区域。不清除帧缓冲区、不 present。
// 调用者必须在外层自行 clear + render + present。
UICORNERSTONE_API void UICornerstone_Render(void);

// 清除帧缓冲区和翻转缓冲区（可选，仅在调用者不自行管理帧时使用）
UICORNERSTONE_API void UICornerstone_Clear(void);
UICORNERSTONE_API void UICornerstone_Present(void);

UICORNERSTONE_API int  UICornerstone_IsQuitRequested(void);

/* ============ 布局系统 ============ */
UICORNERSTONE_API int               UICornerstone_LoadLayout(const char* jsonContent);
UICORNERSTONE_API int               UICornerstone_LoadLayoutFromFile(const char* filePath);
UICORNERSTONE_API UIControlHandle   UICornerstone_FindControl(const char* id);

typedef void (*UIActionCallback)(UIControlHandle ctl, void* userData);
UICORNERSTONE_API void UICornerstone_RegisterAction(const char* name, UIActionCallback cb, void* userData);

/* ============ 编程式控件创建 ============ */
UICORNERSTONE_API UIControlHandle UICornerstone_CreateButton(const char* text,
    float x, float y, float w, float h);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateLabel(const char* text, float fontSize,
    float x, float y, float w, float h);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateCheckBox(const char* text,
    float x, float y, float w, float h);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateEditBox(
    float x, float y, float w, float h);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateProgressBar(
    float x, float y, float w, float h);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateSlider(
    float x, float y, float w, float h, float min, float max, float value);
UICORNERSTONE_API UIControlHandle UICornerstone_CreatePanel(
    float x, float y, float w, float h);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateTextArea(
    float x, float y, float w, float h);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateWinFrame(
    const char* title, float x, float y, float w, float h);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateMenu(void);

/* ============ 控件通用操作 ============ */
UICORNERSTONE_API void UICornerstone_SetRect(UIControlHandle ctl, float x, float y, float w, float h);
UICORNERSTONE_API void UICornerstone_GetRect(UIControlHandle ctl, float* x, float* y, float* w, float* h);
UICORNERSTONE_API void UICornerstone_SetVisible(UIControlHandle ctl, int visible);
UICORNERSTONE_API void UICornerstone_SetEnabled(UIControlHandle ctl, int enabled);
UICORNERSTONE_API void UICornerstone_SetText(UIControlHandle ctl, const char* text);
UICORNERSTONE_API void UICornerstone_AddChild(UIControlHandle parent, UIControlHandle child);
UICORNERSTONE_API void UICornerstone_SetOnClick(UIControlHandle ctl, UIActionCallback cb, void* userData);
UICORNERSTONE_API void UICornerstone_SetBGColor(UIControlHandle ctl, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
UICORNERSTONE_API void UICornerstone_SetProgress(UIControlHandle ctl, float value);
UICORNERSTONE_API void UICornerstone_SetChecked(UIControlHandle ctl, int checked);
UICORNERSTONE_API void UICornerstone_DestroyControl(UIControlHandle ctl);
UICORNERSTONE_API const char* UICornerstone_GetText(UIControlHandle ctl);
UICORNERSTONE_API const char* UICornerstone_GetControlId(UIControlHandle ctl);
UICORNERSTONE_API int UICornerstone_GetChecked(UIControlHandle ctl);
UICORNERSTONE_API float UICornerstone_GetProgress(UIControlHandle ctl);
UICORNERSTONE_API void UICornerstone_WinFrameSetClientText(UIControlHandle wf, const char* text);

/* ============ Slider ============ */
UICORNERSTONE_API float UICornerstone_GetSliderValue(UIControlHandle ctl);
UICORNERSTONE_API void UICornerstone_SetSliderValue(UIControlHandle ctl, float value);
UICORNERSTONE_API void UICornerstone_SetOnSliderChanged(UIControlHandle ctl, UIActionCallback cb, void* userData);

/* ============ ColorPicker ============ */
UICORNERSTONE_API UIControlHandle UICornerstone_CreateColorPicker(
    float x, float y, float w, float h, const char* color);
UICORNERSTONE_API void UICornerstone_GetColorPickerColor(UIControlHandle ctl, char* hexOut, int maxLen);
UICORNERSTONE_API void UICornerstone_SetOnColorChanged(UIControlHandle ctl, UIActionCallback cb, void* userData);
UICORNERSTONE_API void UICornerstone_SetClosedSwatchSize(UIControlHandle ctl, float size);
UICORNERSTONE_API void UICornerstone_SetClosedFontSize(UIControlHandle ctl, int size);
UICORNERSTONE_API void UICornerstone_SetClosedTextColor(UIControlHandle ctl, const char* hex);
UICORNERSTONE_API void UICornerstone_SetPopupBGColor(UIControlHandle ctl, const char* hex);

/* ============ ComboBox ============ */
UICORNERSTONE_API UIControlHandle UICornerstone_CreateComboBox(
    float x, float y, float w, float h);
UICORNERSTONE_API void UICornerstone_SetComboItems(UIControlHandle ctl, const char* jsonItems);
UICORNERSTONE_API void UICornerstone_SetSelectedIndex(UIControlHandle ctl, int index);
UICORNERSTONE_API int  UICornerstone_GetSelectedIndex(UIControlHandle ctl);
UICORNERSTONE_API const char* UICornerstone_GetSelectedLabel(UIControlHandle ctl);
UICORNERSTONE_API void UICornerstone_SetOnSelectionChanged(UIControlHandle ctl, UIActionCallback cb, void* userData);

/* ============ Dialog/Popup ============ */
UICORNERSTONE_API UIControlHandle UICornerstone_CreateDialog(
    const char* confirmText, const char* cancelText,
    float x, float y, float w, float h);
UICORNERSTONE_API void UICornerstone_Show(UIControlHandle ctl);
UICORNERSTONE_API void UICornerstone_Close(UIControlHandle ctl);
UICORNERSTONE_API void UICornerstone_SetDialogCentered(UIControlHandle ctl, int centered);
UICORNERSTONE_API void UICornerstone_SetDialogPosition(UIControlHandle ctl, float x, float y, float w, float h);
UICORNERSTONE_API void UICornerstone_SetContent(UIControlHandle dlg, UIControlHandle content);
UICORNERSTONE_API void UICornerstone_SetOnConfirm(UIControlHandle ctl, UIActionCallback cb, void* userData);
UICORNERSTONE_API void UICornerstone_SetOnCancel(UIControlHandle ctl, UIActionCallback cb, void* userData);
UICORNERSTONE_API void UICornerstone_SetOnClose(UIControlHandle ctl, UIActionCallback cb, void* userData);
UICORNERSTONE_API void UICornerstone_SetConfirmButtonText(UIControlHandle ctl, const char* text);
UICORNERSTONE_API void UICornerstone_SetCancelButtonText(UIControlHandle ctl, const char* text);

/* ============ NumericUpDown ============ */
UICORNERSTONE_API UIControlHandle UICornerstone_CreateNumericUpDown(
    float x, float y, float w, float h);
UICORNERSTONE_API void   UICornerstone_SetNumericUpDownValue(UIControlHandle ctl, double val);
UICORNERSTONE_API double UICornerstone_GetNumericUpDownValue(UIControlHandle ctl);
UICORNERSTONE_API void   UICornerstone_SetNumericUpDownRange(UIControlHandle ctl, double min, double max);
UICORNERSTONE_API void   UICornerstone_SetNumericUpDownStep(UIControlHandle ctl, double step);
UICORNERSTONE_API void   UICornerstone_SetNumericUpDownPageStep(UIControlHandle ctl, double ps);
UICORNERSTONE_API void   UICornerstone_SetNumericUpDownDecimals(UIControlHandle ctl, int decimals);
UICORNERSTONE_API void   UICornerstone_SetNumericUpDownPlaceholder(UIControlHandle ctl, const char* placeholder);
UICORNERSTONE_API void   UICornerstone_SetNumericUpDownReadOnly(UIControlHandle ctl, int readOnly);
UICORNERSTONE_API void   UICornerstone_SetNumericUpDownButtonWidth(UIControlHandle ctl, float width);
UICORNERSTONE_API void   UICornerstone_SetOnNumericUpDownValueChanged(
    UIControlHandle ctl, void (*callback)(void* userData, double newValue), void* userData);

/* ============ 图片/动画按钮 ============ */
UICORNERSTONE_API UIControlHandle UICornerstone_CreateImageButton(
    const char* normalImage,
    const char* hoverImage,
    const char* pressedImage,
    float x, float y, float w, float h);

UICORNERSTONE_API void UICornerstone_SetButtonAnimation(
    UIControlHandle btn,
    const char* jsoncPath);

#ifdef __cplusplus
}
#endif

#endif // UICORNERSTONE_API_H

