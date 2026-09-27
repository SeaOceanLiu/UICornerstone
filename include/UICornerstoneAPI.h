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

/* 实例句柄：一个 UICornerstone 实例（窗口或视口）。不透明指针。 */
typedef struct UIContext* UIInstance;

/* 跨后端统一窗口标志（值对齐 SDL_WINDOW_*，UIWindowFlags 的单一事实来源：
   Window.h / C++ Binding 的 UIWindowFlags 成员值均引用此处宏，改动不致漂移。
   raylib 按 SDL_WINDOW_RESIZABLE=0x20 约定，sfml 按 0x01=Fullscreen 约定） */
#define UICORN_WINDOW_FLAG_NONE        0x00000000u
#define UICORN_WINDOW_FLAG_FULLSCREEN  0x00000001u /* SDL_WINDOW_FULLSCREEN */
#define UICORN_WINDOW_FLAG_RESIZABLE   0x00000020u /* SDL_WINDOW_RESIZABLE */
#define UICORN_WINDOW_FLAG_VSYNC       0x40000000u /* 应用层保留位：请求垂直同步（raylib 创建期有效） */

/* 实例配置：可选项，全部可 NULL/0 表示默认。structSize 用于 C API 版本兼容检查。 */
typedef struct {
    uint32_t    structSize;         /* 必须填 sizeof(UIInstanceConfig) */
    const char* debugLabel;         /* 调试标签，null → "Instance_<id>" */
    const char* resourceRoot;       /* 资源根目录，null → 默认 */
    const char* windowTitle;        /* 窗口标题，null → "UICornerstone" */
    int         windowWidth;        /* 0 → 默认 1024 */
    int         windowHeight;       /* 0 → 默认 768 */
    uint32_t    windowFlags;        /* 跨后端统一窗口标志（UIWindowFlags，值对齐 SDL_WINDOW_*） */
    float       canvasWidth;        /* 显式基准画布宽，0 → 跟随窗口（viewport） */
    float       canvasHeight;       /* 显式基准画布高，0 → 跟随窗口 */
    int         viewportScaleMode;  /* 初始视口缩放模式：0=off 1=fit 2=stretch */
    uint32_t    reserved[3];        /* 未来扩展预留 */
} UIInstanceConfig;

#define UI_INSTANCE_CONFIG_DEFAULT \
    { sizeof(UIInstanceConfig), NULL, NULL, NULL, 0, 0, 0, 0.0f, 0.0f, 0, {0} }

/* ============ 基础类型 ============ */
typedef struct { float x, y, w, h; }   UIRect;
typedef struct { uint8_t r, g, b, a; } UIColor;

typedef struct {
    UIColor normal;
    UIColor hover;
    UIColor pressed;
    UIColor disabled;
} UIStateColor;

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
/* 后端能力位（UIBackendCallbacks::capabilities，0 = 无声明能力）：
   - MULTI_WINDOW：支持多实例独立窗口渲染（单窗口架构的后端如 raylib 不声明）
   - RENDER_TARGET / CLIP_RECT / READBACK：渲染设备能力，原生 GPU 后端（OpenGL/
     DirectX/Vulkan）天然具备；测试与调用方可按位查询后决定行为 */
#define UICORN_BACKEND_CAP_MULTI_WINDOW  (1u << 0)
#define UICORN_BACKEND_CAP_RENDER_TARGET (1u << 1)
#define UICORN_BACKEND_CAP_CLIP_RECT     (1u << 2)
#define UICORN_BACKEND_CAP_READBACK      (1u << 3)
/* WINDOW_SET_SIZE：运行期 SetWindowSize 生效（有真实窗口的后端声明；
   raylib 多实例 headless 实例无窗口，调用方按此位守卫以免空操作或越界） */
#define UICORN_BACKEND_CAP_WINDOW_SET_SIZE (1u << 4)

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
    /* 请求窗口 resize（可选，可 NULL；运行期窗口 API SetWindowSize 的后端实现入口） */
    void            (*setWindowSize)(UIWindowHandle, int w, int h);
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
    // 后端配置键值（可选，可 NULL — 未识别 key 返回 0）
    // type: 0=string 1=int 2=bool，value 为统一指针（const char* / int*）
    int                  (*setBackendConfig)(UIRenderDeviceHandle, const char* key, int type, const void* value);
    int                  (*getBackendConfig)(UIRenderDeviceHandle, const char* key, int type, void* value, int maxLen);
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

    // --- Memory ResourceProvider（内存资源注册表，可选）---
    // 创建内存注册表：name → 字节，readFile 命中返回；自动剥离 "provider:"（PropertyNames::kProviderPrefix）前缀
    UIResourceProviderHandle (*createMemoryResourceProvider)(void);
    // 拷贝注册：引擎内部复制 data，调用方可立即释放；同名覆盖旧条目
    int  (*memoryProviderRegister)(UIResourceProviderHandle, const char* name, const void* data, int len);
    // 零拷贝注册：转移 data 所有权给引擎；析构/覆盖时经 freeFn 释放（freeFn NULL → free）
    int  (*memoryProviderAdopt)(UIResourceProviderHandle, const char* name,
                                const void* data, int len, void (*freeFn)(void*));
    // 挂载到实例：替换 UIContext::resourceProvider（控件经级联传播动态生效）
    int  (*setResourceProvider)(UIInstance, UIResourceProviderHandle);

    // 后端能力位（UICORN_BACKEND_CAP_*），0 = 无声明能力
    uint32_t                 capabilities;

    // --- 截图读回（可选，可为 NULL；声明 READBACK 能力位时须提供） ---
    // 将屏幕坐标系 rect 读回为 RGBA8888（R,G,B,A，top-down 行序）到 buffer（调用方分配 w*h*4）
    void (*readPixels)(UIRenderDeviceHandle dev, void* buffer, int left, int top, int width, int height);
} UIBackendCallbacks;

/* ============ 实例生命周期 ============ */
/* 创建实例：alloc + init 一次性完成。失败返回 NULL。 */
UICORNERSTONE_API UIInstance UICornerstone_CreateInstance(
    const UIBackendCallbacks* callbacks,
    const UIInstanceConfig* config);       /* NULL → 全默认 */

/* 销毁实例：级联销毁子视口，owner 才 shutdown BackendManager。NULL 安全。 */
UICORNERSTONE_API void UICornerstone_DestroyInstance(UIInstance instance);

/* 从后端插件 DLL（UIBackend_<pluginName>.dll）创建实例。
   静态链接路径回退 GetUIBackendCallbacks。 */
UICORNERSTONE_API UIInstance UICornerstone_CreateInstanceFromPlugin(
    const char* pluginName,
    const UIInstanceConfig* config);

/* 查询实例后端能力位（UICORN_BACKEND_CAP_*，按位与）。instance 为 NULL 时返回 0。
   用途：调用方/测试据此决定行为——例如多实例双窗口视觉断言仅在
   MULTI_WINDOW 能力下执行，单窗口架构的后端（raylib）跳过。 */
UICORNERSTONE_API uint32_t UICornerstone_GetBackendCapabilities(UIInstance instance);

/* ============ 后端配置 ============ */
/* 后端键值配置（vsync / swap-ratio / renderer-name 等，各后端支持子集见文档）。
   inst == NULL → 设置/查询全局后端默认值，在后续 CreateInstance 创建 renderer 前生效
                  （对应 raylib FLAG_VSYNC_HINT 这类须创建期生效的参数）；
   inst != NULL → 设置/查询当前实例的 connecter 运行期可调参数（sdl3/sfml 的 vsync 可运行期切换）。
   成功返回 1；未识别的 key / 后端不支持返回 0。 */
UICORNERSTONE_API int UICornerstone_SetBackendConfig(UIInstance inst, const char* key, const char* value);
UICORNERSTONE_API int UICornerstone_SetBackendConfigInt(UIInstance inst, const char* key, int value);
UICORNERSTONE_API int UICornerstone_SetBackendConfigBool(UIInstance inst, const char* key, int value);
UICORNERSTONE_API int UICornerstone_GetBackendConfig(UIInstance inst, const char* key, char* value, int maxLen);
UICORNERSTONE_API int UICornerstone_GetBackendConfigInt(UIInstance inst, const char* key, int* value);
UICORNERSTONE_API int UICornerstone_GetBackendConfigBool(UIInstance inst, const char* key, int* value);

/* 在父实例（owner）的窗口中创建子视口：共享后端，独立控制树/事件队列/DataContext。
   rect 为窗口坐标系下的视口区域。 */
UICORNERSTONE_API UIInstance UICornerstone_CreateViewport(
    UIInstance parent, UIRect rect);

/* ============ 视口控制 ============ */
UICORNERSTONE_API void UICornerstone_SetViewport(UIInstance instance, float x, float y, float w, float h);
UICORNERSTONE_API void UICornerstone_GetViewport(UIInstance instance, float* x, float* y, float* w, float* h);

/* 视口背景色（RGBA8888）：Render 前填充视口区域（fit/stretch 留白处也生效）。
   默认透明（a=0）＝不填充。返回 0 = 参数非法。 */
UICORNERSTONE_API int UICornerstone_SetViewportBackgroundColor(UIInstance instance, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* ============ 视口缩放（ViewportScale） ============ */
/* mode: 0=off（画布跟随窗口，原语义） 1=fit（等比居中） 2=stretch（拉伸铺满） */
UICORNERSTONE_API int UICornerstone_SetViewportScaleMode(UIInstance instance, int mode);
UICORNERSTONE_API int UICornerstone_GetViewportScaleMode(UIInstance instance, int* mode);
/* 显式基准画布尺寸（fit/stretch 的适配基准），0/0 → 跟随窗口 */
UICORNERSTONE_API int UICornerstone_SetCanvasSize(UIInstance instance, float w, float h);
/* 当前复合缩放（含画布比例链） */
UICORNERSTONE_API int UICornerstone_GetViewportScale(UIInstance instance, float* sx, float* sy);

// 实例缩放（画布语义）：控件逻辑 rect 不变，视觉 rect/字号按比例复合缩放；
// 置手动 override（off 分支 recompute 尊重）；fit/stretch 切换时引擎接管。
// x/y ≤ 0 拒绝。返回 1 成功 / 0 参数无效。
UICORNERSTONE_API int UICornerstone_SetInstanceScale(UIInstance instance, float xScale, float yScale);
UICORNERSTONE_API int UICornerstone_GetInstanceScale(UIInstance instance, float* outX, float* outY);
/* 手动锚点偏移（增量叠加，off 模式手动平移用） */
UICORNERSTONE_API int UICornerstone_SetViewportAnchor(UIInstance instance, float ax, float ay);

/* ============ 运行期窗口 API ============ */
/* 窗口尺寸查询：headless 实例（见 CAP_MULTI_WINDOW）返回 0×0。返回 1 = ok。 */
UICORNERSTONE_API int UICornerstone_GetWindowSize(UIInstance instance, float* w, float* h);
/* 请求窗口 resize（实现经后端 Window::setSize，尺寸变化经 WindowResize 事件回流）。
   headless / 无窗口实例返回 0。 */
UICORNERSTONE_API int UICornerstone_SetWindowSize(UIInstance instance, float w, float h);
/* 原生窗口句柄（嵌入/hosted 场景）；无窗口返回 NULL。 */
UICORNERSTONE_API void* UICornerstone_GetNativeWindowHandle(UIInstance instance);

typedef void (*UIWindowResizeCallback)(int width, int height, void* userData);
/* 窗口尺寸变化用户回调（ProcessEvents 处理 WindowResize 事件时分发，替代用户帧内轮询）。 */
UICORNERSTONE_API void UICornerstone_SetWindowResizeCallback(UIInstance instance,
    UIWindowResizeCallback cb, void* userData);

/* ============ 帧循环 ============ */
UICORNERSTONE_API int UICornerstone_ProcessEvents(UIInstance instance);
UICORNERSTONE_API void UICornerstone_Update(UIInstance instance, double deltaTime);

// 注入外部事件（例如从 SDL_AppEvent 回调传入的 SDL 事件）。
// 事件会在下一次 UICornerstone_ProcessEvents 中被处理。
UICORNERSTONE_API void UICornerstone_PushUIEvent(UIInstance instance, const UIEvent* ue);

// 只渲染视口区域。不清除帧缓冲区、不 present。
// 调用者必须在外层自行 clear + render + present。
UICORNERSTONE_API void UICornerstone_Render(UIInstance instance);

// 清除帧缓冲区和翻转缓冲区（可选，仅在调用者不自行管理帧时使用）
UICORNERSTONE_API void UICornerstone_Clear(UIInstance instance);
UICORNERSTONE_API void UICornerstone_Present(UIInstance instance);

UICORNERSTONE_API int  UICornerstone_IsQuitRequested(UIInstance instance);

/* ============ Debug 辅助 ============ */
// 存活实例数（Release 构建返回 0）
UICORNERSTONE_API int      UICornerstone_Debug_GetAliveCount(void);
// 按索引取存活实例（Release 构建返回 NULL）
UICORNERSTONE_API UIInstance UICornerstone_Debug_GetAliveInstance(int index);
// 查询当前活动视口（子视口为 null；viewport 实例查询返回 owner 的活动视口）
UICORNERSTONE_API UIInstance UICornerstone_Debug_GetActiveViewport(UIInstance instance);
// 查询控件是否拥有焦点
UICORNERSTONE_API int      UICornerstone_Debug_IsControlFocused(UIInstance instance, UIControlHandle control);
// 查询控件是否处于鼠标悬停状态（m_mouseInside，_DEBUG；Release 恒 0）
UICORNERSTONE_API int      UICornerstone_Debug_IsControlHovered(UIInstance instance, UIControlHandle control);
// 调试鼠标注入（_DEBUG；Release 返回 0 不生效）：用注入坐标替代 Window::getMousePosition
// 驱动本实例 hover 状态——无人值守测试验证跨窗口/跨视口 hover 隔离。坐标为窗口坐标系绝对坐标。
UICORNERSTONE_API int      UICornerstone_Debug_SetMousePosition(UIInstance instance, float x, float y);
// 清除调试鼠标注入，恢复真实鼠标位置
UICORNERSTONE_API int      UICornerstone_Debug_ClearMousePosition(UIInstance instance);

/* ============ 布局系统 ============ */
UICORNERSTONE_API int               UICornerstone_LoadLayout(UIInstance instance, const char* jsonContent);
UICORNERSTONE_API int               UICornerstone_LoadLayoutFromFile(UIInstance instance, const char* filePath);
UICORNERSTONE_API UIControlHandle   UICornerstone_FindControl(UIInstance instance, const char* id);
// 给编程式创建的控件设置 id（注册 controlsById，供 FindControl 查询）。id 空则移除该控件名下 id。
UICORNERSTONE_API int               UICornerstone_SetControlId(UIInstance instance, UIControlHandle ctl, const char* id);

typedef void (*UIActionCallback)(UIControlHandle ctl, void* userData);
UICORNERSTONE_API void UICornerstone_RegisterAction(UIInstance instance, const char* name, UIActionCallback cb, void* userData);

/* ============ 编程式控件创建 ============ */
// xScale/yScale 为初始缩放系数（内容按系数缩放渲染，默认 1.0f = 原尺寸）
UICORNERSTONE_API UIControlHandle UICornerstone_CreateButton(UIInstance instance, const char* text,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateLabel(UIInstance instance, const char* text, float fontSize,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateCheckBox(UIInstance instance, const char* text,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateEditBox(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateProgressBar(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateSlider(UIInstance instance,
    float x, float y, float w, float h, float min, float max, float value, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreatePanel(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateTextArea(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateWinFrame(UIInstance instance,
    const char* title, float x, float y, float w, float h, float xScale, float yScale);
/* ============ Menu 三件套（MenuBar / MenuPanel / MenuItem） ============ */
// Menu 是一组控件：MenuBar 为顶层菜单栏，MenuPanel 为下拉面板，
// MenuItem 为菜单项。组装顺序：
//   panel = CreateMenuPanel(inst);  item = CreateMenuItem(inst, "Open", 0);
//   MenuPanelAddItem(inst, panel, item);  MenuItemSetSubMenu(inst, item, subPanel);
//   bar = CreateMenuBar(inst, ...);  MenuBarAddMenu(inst, bar, "File", panel);
// type: 0=Normal, 1=Separator, 2=SubMenu
// MenuItem 的 caption/checked/shortcut/click 走统一属性系统
// （SetString/SetBool/SetCallback，事件名 "click"）
UICORNERSTONE_API UIControlHandle UICornerstone_CreateMenuBar(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateMenuPanel(UIInstance instance, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateMenuItem(UIInstance instance, const char* caption, int type,
    float xScale, float yScale);
UICORNERSTONE_API void UICornerstone_MenuBarAddMenu(UIInstance instance,
    UIControlHandle bar, const char* caption, UIControlHandle panel);
UICORNERSTONE_API void UICornerstone_MenuPanelAddItem(UIInstance instance,
    UIControlHandle panel, UIControlHandle item);
UICORNERSTONE_API void UICornerstone_MenuPanelAddSeparator(UIInstance instance, UIControlHandle panel);
UICORNERSTONE_API void UICornerstone_MenuItemSetSubMenu(UIInstance instance,
    UIControlHandle item, UIControlHandle panel);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateColorPicker(UIInstance instance,
    float x, float y, float w, float h, const char* color, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateNumericUpDown(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateComboBox(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateSplitter(UIInstance instance,
    float x, float y, float w, float h, int orientation, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateScrollBar(UIInstance instance,
    float x, float y, float w, float h, int orientation, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateTreeView(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);

/* ============ TreeView 节点操作 ============ */
// parentId 为空串 = 插入为根节点；返回 1 成功 / 0 失败（父节点不存在 / id 已存在由树内部策略决定）。
// 展开/折叠类也可经属性系统（SetString "expand" / "collapse" / SetBool "expand-all" 等）实现，函数式接口更顺手。
// 返回值语义：1 = 成功（节点存在/已执行），0 = 失败（父或节点不存在）。
UICORNERSTONE_API int UICornerstone_TreeViewAddNode(UIInstance instance,
    UIControlHandle tree, const char* parentId, const char* id, const char* label, int expanded);
UICORNERSTONE_API int UICornerstone_TreeViewRemoveNode(UIInstance instance,
    UIControlHandle tree, const char* id);
UICORNERSTONE_API int UICornerstone_TreeViewSetNodeLabel(UIInstance instance,
    UIControlHandle tree, const char* id, const char* label);
UICORNERSTONE_API int UICornerstone_TreeViewSetNodeUserData(UIInstance instance,
    UIControlHandle tree, const char* id, void* userData);
UICORNERSTONE_API int UICornerstone_TreeViewSelectNode(UIInstance instance,
    UIControlHandle tree, const char* id);
UICORNERSTONE_API void UICornerstone_TreeViewClearSelection(UIInstance instance, UIControlHandle tree);
UICORNERSTONE_API int UICornerstone_TreeViewExpandNode(UIInstance instance,
    UIControlHandle tree, const char* id);
UICORNERSTONE_API int UICornerstone_TreeViewCollapseNode(UIInstance instance,
    UIControlHandle tree, const char* id);
UICORNERSTONE_API void UICornerstone_TreeViewExpandAll(UIInstance instance, UIControlHandle tree);
UICORNERSTONE_API void UICornerstone_TreeViewCollapseAll(UIInstance instance, UIControlHandle tree);
UICORNERSTONE_API void UICornerstone_TreeViewClearItems(UIInstance instance, UIControlHandle tree);
// 读取选中节点 id（无选中返回 0；有选中将 id 拷入 outBuf 并返回 1）
UICORNERSTONE_API int UICornerstone_TreeViewGetSelectedId(UIInstance instance,
    UIControlHandle tree, char* outBuf, int outSize);

/* ============ EditBox / TextArea 文本操作 ============ */
// 选区与剪贴板命令式操作（对应 EditBox/TextArea 方法）。返回 1 成功 / 0 失败（非文本控件）。
UICORNERSTONE_API int UICornerstone_EditBoxSelectAll(UIInstance instance, UIControlHandle ctl);
UICORNERSTONE_API int UICornerstone_EditBoxSetSelection(UIInstance instance, UIControlHandle ctl, int start, int end);
UICORNERSTONE_API int UICornerstone_EditBoxClearSelection(UIInstance instance, UIControlHandle ctl);
UICORNERSTONE_API int UICornerstone_EditBoxHasSelection(UIInstance instance, UIControlHandle ctl);
UICORNERSTONE_API int UICornerstone_EditBoxGetCursorPosition(UIInstance instance, UIControlHandle ctl);
UICORNERSTONE_API int UICornerstone_EditBoxCopy(UIInstance instance, UIControlHandle ctl);
UICORNERSTONE_API int UICornerstone_EditBoxCut(UIInstance instance, UIControlHandle ctl);
UICORNERSTONE_API int UICornerstone_EditBoxPaste(UIInstance instance, UIControlHandle ctl);
UICORNERSTONE_API int UICornerstone_EditBoxDeleteSelectedText(UIInstance instance, UIControlHandle ctl);

/* ============ NumericUpDown 数值操作 ============ */
// 按方向步进（dir=+1 增 / -1 减，超界自动 clamp）。返回 1 成功 / 0 失败。
UICORNERSTONE_API int UICornerstone_NumericUpDownStep(UIInstance instance, UIControlHandle ctl, int dir);

/* ============ ComboBox 选项操作 ============ */
// 运行期增删选项。label/value 为文本；disabled=0/1。返回 1 成功 / 0 失败（非 ComboBox）。
UICORNERSTONE_API int UICornerstone_ComboBoxAddItem(UIInstance instance, UIControlHandle ctl,
    const char* label, const char* value, int disabled);
// 按索引移除选项。index 越界返回 0。
UICORNERSTONE_API int UICornerstone_ComboBoxRemoveItem(UIInstance instance, UIControlHandle ctl, int index);
UICORNERSTONE_API int UICornerstone_ComboBoxClearItems(UIInstance instance, UIControlHandle ctl);
// 返回当前选项数量；非 ComboBox 返回 -1。
UICORNERSTONE_API int UICornerstone_ComboBoxGetItemCount(UIInstance instance, UIControlHandle ctl);

UICORNERSTONE_API UIControlHandle UICornerstone_CreateHandleControl(UIInstance instance,
    UIControlHandle target, float x, float y, float w, float h, float xScale, float yScale);
/* ============ HandleControl 集成扩展（第二批）============ */
// 拖拽/移动几何过滤回调：HandleControl 每帧计算出新 rect 后、setRect 前调用。
// target 为目标控件句柄；ioX/ioY/ioW/ioH 为 target 局部坐标（逻辑 rect）。
// 返回 1 用修正值，返回 0 用原计算值。注意修改 left/top 时请自行与 width/height 联动（保持对缘不变）。
typedef int (*UIHandleRectFilter)(UIControlHandle target, float* ioX, float* ioY,
                                  float* ioW, float* ioH, void* userData);
// 切换手柄附加目标。target==NULL → detach（自移出父容器 + 恢复光标）。返回 1 成功。
UICORNERSTONE_API int UICornerstone_SetHandleTarget(UIInstance instance,
    UIControlHandle handle, UIControlHandle target);
// 手柄命中查询。x/y 为**窗口全局坐标原样**（与 UIEvent 的 mousePos/mouseWheel.x,y 及 getDrawRect 同域：
// 父链递归累加至窗口全局，视口根 rect 已含窗口位置——子视口场景**不要**再减视口偏移）。
// 命中返回 1 并写 *outHandleType（None=0、Move=1、NW=2、N=3、NE=4、E=5、SE=6、S=7、SW=8、W=9）；未命中/非手柄控件返回 0。
UICORNERSTONE_API int UICornerstone_HandleHitTest(UIInstance instance,
    UIControlHandle handle, float x, float y, int* outHandleType);
// 设置拖拽几何过滤回调（吸附/对齐线扩展点）。filter==NULL 清除。返回 1 成功。
UICORNERSTONE_API int UICornerstone_SetHandleRectFilter(UIInstance instance, UIControlHandle handle,
    UIHandleRectFilter filter, void* userData);
// Move 手柄可见性（策略开关：设计器可关闭 Move 手柄，移动统一走本体拖动+filter）。返回 1 成功。
UICORNERSTONE_API int UICornerstone_SetHandleMoveVisible(UIInstance instance, UIControlHandle handle, int show);
/* ============ Image 图片控件 ============ */
// image 为文件路径（可为 NULL，之后经 UICornerstone_SetString(inst, ctl, "image", path) 设置；
// 资源 ID 经 "image-resource" 设置）。w/h 传 0 表示按纹理自然尺寸。
UICORNERSTONE_API UIControlHandle UICornerstone_CreateImage(
    UIInstance instance,
    const char* image,
    float x, float y, float w, float h, float xScale, float yScale);

// Actor 源矩形（纹理子区域；对 scale-type=stretch 与 tile 生效）。
// 清除语义：x<0 或 w≤0 或 h≤0 → 恢复整图。返回 1 成功 / 0 无效句柄。
UICORNERSTONE_API int UICornerstone_ActorSetSourceRect(UIInstance instance,
    UIControlHandle ctl, float x, float y, float w, float h);

// 直接创建独立 Actor（图片显示控件），与 CreateImage 等价——图片按钮等组合控件
// 不需要专用工厂：经 UICornerstone_CreateButton 创建后，用字符串属性
// "normal-image"/"hover-image"/"pressed-image"/"disabled-image" 设置三态图片即可。
UICORNERSTONE_API UIControlHandle UICornerstone_CreateActor(
    UIInstance instance,
    const char* image,
    float x, float y, float w, float h, float xScale, float yScale);

/* ============ LuotiAni 动画控件 ============ */
// jsoncPath 为动画描述文件路径（相对路径经基路径拼接），可为 NULL 或空串（占位创建：
// 不加载/不 prepare，之后经 UICornerstone_SetString(inst, ctl, "animation", path)（"path" 同义）
// 设置并加载）；创建后不自动播放（显式经 UICornerstone_SetBool(inst, ctl, "playing", 1) 控制）；
// w/h 传 0 → prepare 回退到 JSON overview.view 画布尺寸；非空路径加载失败返回 NULL。
// xScale/yScale 为初始缩放系数（默认 1.0f）。
UICORNERSTONE_API UIControlHandle UICornerstone_CreateAnimation(
    UIInstance instance,
    const char* jsoncPath,
    float x, float y, float w, float h, float xScale, float yScale);

/* ============ Shape 形状控件 ============ */
// 形状参数经通用属性 setter：SetEnum("shape","circle"/...)/SetColor("fill"/"stroke")/
// SetFloat("line-width"/"radius"/"ring-width")；点集为浮点数组走专用接口（决策 3.6）。
// 所有参数修改在控件内统一触发几何重算。
UICORNERSTONE_API UIControlHandle UICornerstone_CreateShape(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
// 写入点集（本地像素坐标；以当前 rect 为缩放基准，决策 3.3）。返回 1 成功 / 0 失败。
UICORNERSTONE_API int UICornerstone_ShapeSetPoints(UIInstance instance, UIControlHandle sh,
    int count, const float* xs, const float* ys);
// 本地 → 全局绘制坐标映射（主查询 API）。成功返回 1 并写入 outX/outY。
UICORNERSTONE_API int UICornerstone_ShapeMapToDrawPoint(UIInstance instance, UIControlHandle sh,
    float lx, float ly, float* outX, float* outY);

/* ---- Shape 多图元（组合图形：非空时替代单 shape 渲染） ---- */
// 添加图元（type 为 kShape* 枚举串；rect 为控件本地坐标）。返回图元索引 / -1 失败。
UICORNERSTONE_API int UICornerstone_ShapeAddPrimitive(UIInstance instance, UIControlHandle sh,
    const char* type, float x, float y, float w, float h);
// 图元颜色（prop: "fill"/"stroke"）
UICORNERSTONE_API int UICornerstone_ShapeSetPrimitiveColor(UIInstance instance, UIControlHandle sh,
    int index, const char* prop, UIColor value);
// 图元数值（prop: "line-width"/"radius"/"ring-width"）
UICORNERSTONE_API int UICornerstone_ShapeSetPrimitiveFloat(UIInstance instance, UIControlHandle sh,
    int index, const char* prop, float value);
// 图元点集（本地像素；polyline/polygon 用）
UICORNERSTONE_API int UICornerstone_ShapeSetPrimitivePoints(UIInstance instance, UIControlHandle sh,
    int index, int count, const float* xs, const float* ys);
UICORNERSTONE_API int UICornerstone_ShapeClearPrimitives(UIInstance instance, UIControlHandle sh);

/* ============ ListView 列表控件 ============ */
// 属性走通用 setter：SetEnum("mode","multi"/"single")、SetBool("multi-select"/"gridlines"/
// "horizontal-gridlines"/"hover"/"sort-ascending")、SetFloat("row-height"/"header-height"/
// "min-column-width")、SetInt("selected-index"/"sort-column")；数据/对象类走下列专用函数。
// 返回值约定：int 1=成功 / 0=失败（非 list-view 或越界）。
typedef int (*ListViewSortFn)(const char* a, const char* b, void* userData);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateListView(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
// 行（cells 为字符串数组，自动补足/截断到列数）
UICORNERSTONE_API int UICornerstone_ListViewAddRow(UIInstance instance, UIControlHandle lv,
    const char* id, int count, const char* const* cells);
UICORNERSTONE_API int UICornerstone_ListViewInsertRow(UIInstance instance, UIControlHandle lv,
    int index, const char* id, int count, const char* const* cells);
UICORNERSTONE_API int UICornerstone_ListViewRemoveRow(UIInstance instance, UIControlHandle lv, int index);
UICORNERSTONE_API int UICornerstone_ListViewSetCellText(UIInstance instance, UIControlHandle lv,
    int row, int col, const char* text);
UICORNERSTONE_API int UICornerstone_ListViewGetCellText(UIInstance instance, UIControlHandle lv,
    int row, int col, char* outBuf, int maxLen);
UICORNERSTONE_API int UICornerstone_ListViewSetRowCells(UIInstance instance, UIControlHandle lv,
    int index, int count, const char* const* cells);
UICORNERSTONE_API int UICornerstone_ListViewSetColumnValues(UIInstance instance, UIControlHandle lv,
    int colIndex, int count, const char* const* values);
// 列
UICORNERSTONE_API int UICornerstone_ListViewAddColumn(UIInstance instance, UIControlHandle lv,
    const char* title, float width, int sortable);
UICORNERSTONE_API int UICornerstone_ListViewInsertColumn(UIInstance instance, UIControlHandle lv,
    int index, const char* title, float width, int sortable);
UICORNERSTONE_API int UICornerstone_ListViewRemoveColumn(UIInstance instance, UIControlHandle lv, int index);
UICORNERSTONE_API int UICornerstone_ListViewSetColumnWidth(UIInstance instance, UIControlHandle lv,
    int index, float width);
// 对象注入（leadingControl 语义）
UICORNERSTONE_API int UICornerstone_ListViewSetColumnIcon(UIInstance instance, UIControlHandle lv,
    int colIndex, UIControlHandle iconControl);
UICORNERSTONE_API int UICornerstone_ListViewSetRowLeadingControl(UIInstance instance, UIControlHandle lv,
    int index, UIControlHandle iconControl);
UICORNERSTONE_API int UICornerstone_ListViewSetCellLeadingControl(UIInstance instance, UIControlHandle lv,
    int row, int col, UIControlHandle control);
// 样式（bg 颜色 + fontSize；font 缺省；JSON/Binding 二期）
UICORNERSTONE_API int UICornerstone_ListViewSetCellStyle(UIInstance instance, UIControlHandle lv,
    int row, int col, uint8_t bgR, uint8_t bgG, uint8_t bgB, uint8_t bgA, int fontSize);
// 单元格文本阴影（P0-26：单色 + 偏移；color 的 alpha>0 即启用）。
UICORNERSTONE_API int UICornerstone_ListViewSetCellShadow(UIInstance instance, UIControlHandle lv,
    int row, int col, uint8_t r, uint8_t g, uint8_t b, uint8_t a, float offsetX, float offsetY);
UICORNERSTONE_API int UICornerstone_ListViewSetColumnHeaderStyle(UIInstance instance, UIControlHandle lv,
    int colIndex, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int fontSize);
// 自定义排序比较器（返回 <0/0/>0；传 NULL 清除该列回调恢复字典序）
UICORNERSTONE_API int UICornerstone_ListViewSetColumnSorter(UIInstance instance, UIControlHandle lv,
    int colIndex, ListViewSortFn cmp, void* userData);

/* ============ StatusBar 状态栏控件 ============ */
// 属性走通用 setter:SetFloat("font-size"/"item-height")；数据/对象类走下列专用函数。
UICORNERSTONE_API UIControlHandle UICornerstone_CreateStatusBar(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API int UICornerstone_StatusBarAddItem(UIInstance instance, UIControlHandle bar,
    const char* id, const char* text, int rightAlign);
UICORNERSTONE_API int UICornerstone_StatusBarSetItemText(UIInstance instance, UIControlHandle bar,
    const char* id, const char* text);
UICORNERSTONE_API int UICornerstone_StatusBarRemoveItem(UIInstance instance, UIControlHandle bar,
    const char* id);
UICORNERSTONE_API int UICornerstone_StatusBarSetItemMenu(UIInstance instance, UIControlHandle bar,
    const char* id, UIControlHandle menuPanel);
UICORNERSTONE_API int UICornerstone_StatusBarSetItemIcon(UIInstance instance, UIControlHandle bar,
    const char* id, UIControlHandle iconControl);

/* ============ ContextMenu 右键上下文菜单 ============ */
// 决策点 2/5/6：A 纯 API（Create/AddItem/AddSeparator/Show/Close）+ B 控件绑定走
// SetPtr(inst, ctl, "context-menu", menuHandle)；JSON 层 contextMenu 键后续追加。
UICORNERSTONE_API UIControlHandle UICornerstone_CreateContextMenu(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API int UICornerstone_ContextMenuAddItem(UIInstance instance, UIControlHandle menu,
    const char* caption, const char* shortcut);
UICORNERSTONE_API int UICornerstone_ContextMenuAddSeparator(UIInstance instance, UIControlHandle menu);
UICORNERSTONE_API int UICornerstone_ContextMenuShow(UIInstance instance, UIControlHandle menu, float x, float y);
UICORNERSTONE_API int UICornerstone_ContextMenuClose(UIInstance instance, UIControlHandle menu);

/* ============ TabControl 选项卡控件 ============ */
// 控件级属性走通用 setter:SetEnum("position")=top/bottom/left/right、
//   SetInt("current-index")、SetFloat("font-size")；数据/对象类走下列专用函数。
UICORNERSTONE_API UIControlHandle UICornerstone_CreateTabControl(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API int UICornerstone_TabAddPage(UIInstance instance, UIControlHandle tab, const char* title);
UICORNERSTONE_API int UICornerstone_TabSetTitle(UIInstance instance, UIControlHandle tab,
    int index, const char* title);
UICORNERSTONE_API int UICornerstone_TabSetPage(UIInstance instance, UIControlHandle tab,
    int index, UIControlHandle page);
UICORNERSTONE_API int UICornerstone_TabSetTabLeadingControl(UIInstance instance, UIControlHandle tab,
    int index, UIControlHandle handle);

// 创建"动画按钮"：Button 承载内嵌 LuotiAni 动画（LuotiAni 仅作为按钮的内部
// 绘制资源，不响应鼠标事件；按钮自身响应点击并触发 "click" 回调）。
// 语义同 CreateAnimation：创建后不自动播放（SetBool "playing" 启动）、
// 加载失败返回 NULL。
UICORNERSTONE_API UIControlHandle UICornerstone_CreateAnimatedButton(
    UIInstance instance,
    const char* jsoncPath,
    float x, float y, float w, float h, float xScale, float yScale);

/* ============ LuotiAni 动画操作 ============ */
// 预渲染动画帧（两阶段 prepare；startFrame 指定起始帧）。返回 1 成功 / 0 失败（非动画控件）。
UICORNERSTONE_API int UICornerstone_AnimationPrepare(UIInstance instance, UIControlHandle ctl, int startFrame);
// 帧位图采样过滤开关（1=双线性，0=最近邻；prepare 时生效）。返回 1 成功 / 0 失败。
UICORNERSTONE_API int UICornerstone_AnimationSetFrameFilter(UIInstance instance, UIControlHandle ctl, int bilinear);

/* ============ 控件通用操作 ============ */
UICORNERSTONE_API void UICornerstone_SetRect(UIInstance instance, UIControlHandle ctl, float x, float y, float w, float h);
// 读取控件 rect（**直接父局部坐标**，即 m_rect 原值；与 getDrawRect 的窗口全局域不同——
// 全局域 = 父链递归累加，容器位置判定请用全局域换算）。
UICORNERSTONE_API void UICornerstone_GetRect(UIInstance instance, UIControlHandle ctl, float* x, float* y, float* w, float* h);
UICORNERSTONE_API void UICornerstone_AddChildControl(UIInstance instance, UIControlHandle parent, UIControlHandle child);
UICORNERSTONE_API void UICornerstone_DestroyControl(UIInstance instance, UIControlHandle ctl);
UICORNERSTONE_API const char* UICornerstone_GetControlId(UIInstance instance, UIControlHandle ctl);

/* ============ 截图（Capture_*，测试辅助） ============ */
// 像素级测试辅助：截取渲染结果（RGBA8888，R,G,B,A 内存序，top-down 行序）。
// 能力位 UICORN_BACKEND_CAP_READBACK 未声明时返回 0。
// 需在 UICornerstone_Render 之后、下一次 Clear/Present 之前调用（帧内读回）；
// Present 后调用行为未定义（读到旧帧）。坐标为屏幕像素（逻辑坐标由
// GetViewportScale 换算）。rect 与视口求交集：部分越界裁剪，交集为空返回 0。
// outPixels 由调用方分配 w*h*4 字节；outW/outH 可传 NULL。
UICORNERSTONE_API int UICornerstone_CaptureRect(UIInstance instance,
    float x, float y, float w, float h,
    uint8_t* outPixels, int* outW, int* outH);
UICORNERSTONE_API int UICornerstone_CaptureViewport(UIInstance instance, uint8_t* out, int* w, int* h);
UICORNERSTONE_API int UICornerstone_CaptureBench(UIInstance instance, uint8_t* out, int* w, int* h);
UICORNERSTONE_API int UICornerstone_CaptureControl(UIInstance instance, UIControlHandle ctl,
    uint8_t* out, int* w, int* h);
/* 将内存中的 RGBA8888 像素缓冲保存为文件（当前格式：BMP 32 位 BGRA，零依赖自编码）。
   与后端无关，不需要 instance。 */
UICORNERSTONE_API int UICornerstone_SavePixelsToFile(
    const uint8_t* pixels, int w, int h, const char* filePath);

/* ============ Dialog/Popup ============ */
UICORNERSTONE_API UIControlHandle UICornerstone_CreateDialog(UIInstance instance,
    const char* confirmText, const char* cancelText,
    float x, float y, float w, float h, float xScale, float yScale);
// Popup / ConfirmPopup（P0-27d）：与 CreateDialog 同语义，但**不自动 open**
// （保持隐藏；显隐经 SetBool(inst, ctl, "visible", 1/0)）；ConfirmPopup 居中（同 Dialog）。
UICORNERSTONE_API UIControlHandle UICornerstone_CreatePopup(UIInstance instance,
    float x, float y, float w, float h, float xScale, float yScale);
UICORNERSTONE_API UIControlHandle UICornerstone_CreateConfirmPopup(UIInstance instance,
    const char* confirmText,
    float x, float y, float w, float h, float xScale, float yScale);

/* ============ 属性系统 (统一字符串名 + 多类型入口) ============ */

/* ── Setter ── */
// 设置单色属性。prop 是属性名，如 "selected"、"hover"、"background"
// 返回 1 成功，0 不识别的属性
UICORNERSTONE_API int UICornerstone_SetColor(   UIInstance instance, UIControlHandle ctl, const char* prop, UIColor       value);

// 设置 4 态颜色属性。prop 如 "background"、"border"、"text"
UICORNERSTONE_API int UICornerstone_SetStateColor( UIInstance instance, UIControlHandle ctl, const char* prop, UIStateColor  value);

// 设置布尔属性。prop 如 "visible"、"enabled"，value: 0=假, 非0=真
UICORNERSTONE_API int UICornerstone_SetBool(     UIInstance instance, UIControlHandle ctl, const char* prop, int           value);

// 设置整数属性。prop 如 "font-size"
UICORNERSTONE_API int UICornerstone_SetInt(     UIInstance instance, UIControlHandle ctl, const char* prop, int           value);

// 设置浮点属性。prop 如 "indent-width"、"row-height"
UICORNERSTONE_API int UICornerstone_SetFloat(   UIInstance instance, UIControlHandle ctl, const char* prop, float         value);

// 设置字符串属性。prop 如 "text"、"font"
UICORNERSTONE_API int UICornerstone_SetString(  UIInstance instance, UIControlHandle ctl, const char* prop, const char*   value);

// 设置枚举属性。value 为枚举值名称字符串，如 "vertical"、"checked"
UICORNERSTONE_API int UICornerstone_SetEnum(    UIInstance instance, UIControlHandle ctl, const char* prop, const char*   value);

// 设置指针属性。prop 如 "content"、"first-linked"，value 为 UIControlHandle
UICORNERSTONE_API int UICornerstone_SetPtr(     UIInstance instance, UIControlHandle ctl, const char* prop, void*         value);

/* ── Getter ── */
// 读取属性值。返回 1 成功，0 属性不识别
UICORNERSTONE_API int UICornerstone_GetColor(   UIInstance instance, UIControlHandle ctl, const char* prop, UIColor*       out);
UICORNERSTONE_API int UICornerstone_GetStateColor( UIInstance instance, UIControlHandle ctl, const char* prop, UIStateColor*  out);
UICORNERSTONE_API int UICornerstone_GetBool(     UIInstance instance, UIControlHandle ctl, const char* prop, int*           out);
UICORNERSTONE_API int UICornerstone_GetInt(     UIInstance instance, UIControlHandle ctl, const char* prop, int*           out);
UICORNERSTONE_API int UICornerstone_GetFloat(   UIInstance instance, UIControlHandle ctl, const char* prop, float*         out);
UICORNERSTONE_API int UICornerstone_GetString(  UIInstance instance, UIControlHandle ctl, const char* prop, char* out, int maxLen);
UICORNERSTONE_API int UICornerstone_GetEnum(    UIInstance instance, UIControlHandle ctl, const char* prop, char* out, int maxLen);

// 读取指针属性。out 返回属性值。返回 1 成功，0 属性不识别
UICORNERSTONE_API int UICornerstone_GetPtr(     UIInstance instance, UIControlHandle ctl, const char* prop, void**        out);

// 查询控件运行时类型（如 "check-box" / "image" / "tree-view"，与 JSON "type" 值一致，
// 全部 22 种见 PropertyNames::kControlType*）。out 写入小写 kebab-case 类型名。
// 返回 1 成功，0 未知类型或参数无效。image-button 本质为 button，返回 "button"。
// 典型用法：GetPtr(item-leading-control) 后经本函数判断容器控件类型再走对应属性。
UICORNERSTONE_API int UICornerstone_GetControlType(UIInstance instance, UIControlHandle ctl, char* out, int maxLen);

/* ── Callback ── */
typedef struct {
    const char* eventName;
    union {
        float           floatVal;
        double          doubleVal;
        int             intVal;
        const char*     strVal;
        void*           ptrVal;
        struct { int idx; const char* val; } selection;
        struct { const char* id; void* userData; } treeNode;
        struct { uint8_t r,g,b,a; } color;
        struct { int row; int col; int asc; } grid;
    } data;
} UIEventData;

typedef void (*UIEventCallback)(UIControlHandle ctl, const UIEventData* event, void* userData);

// 绑定事件回调。event 为事件名，如 "click"、"value-changed"
UICORNERSTONE_API int UICornerstone_SetCallback(UIInstance instance, UIControlHandle ctl, const char* event, UIEventCallback cb, void* userData);

#ifdef __cplusplus
}
#endif

#endif // UICORNERSTONE_API_H

