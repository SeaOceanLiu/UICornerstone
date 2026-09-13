// UICornerstone C++ Binding — 动态 API 层（纯 LoadLibrary 模式）
// 许可证 MIT。核心 DLL 与后端 DLL 全部经 LoadLibrary 显式加载，
// binding 不链接任何导入库（UICornerstone_dll.lib / UICornerstone.lib）。
#ifndef UICORNERSTONE_BINDING_DYNAMIC_API_H
#define UICORNERSTONE_BINDING_DYNAMIC_API_H

#include "UICornerstoneAPI.h"
#include <string>

namespace UICornerstone {
namespace Dyn {

// C ABI 函数指针表（与 UICornerstoneAPI.h 导出符号一一对应）。
// 成员统一 fn 前缀：规避 windows.h UNICODE 条件宏（CreateDialog→CreateDialogA/W、
// DialogBoxParamA/W 等）对成员名的预处理替换，无需 #undef。
struct Api {
    // 实例生命周期
    UIInstance (*fnCreateInstance)(const UIBackendCallbacks*, const UIInstanceConfig*) = nullptr;
    void       (*fnDestroyInstance)(UIInstance) = nullptr;
    UIInstance (*fnCreateInstanceFromPlugin)(const char*, const UIInstanceConfig*) = nullptr;

    // 后端配置
    int (*fnSetBackendConfig)(UIInstance, const char*, const char*) = nullptr;
    int (*fnSetBackendConfigInt)(UIInstance, const char*, int) = nullptr;
    int (*fnSetBackendConfigBool)(UIInstance, const char*, int) = nullptr;
    int (*fnGetBackendConfig)(UIInstance, const char*, char*, int) = nullptr;
    int (*fnGetBackendConfigInt)(UIInstance, const char*, int*) = nullptr;
    int (*fnGetBackendConfigBool)(UIInstance, const char*, int*) = nullptr;

    // 视口
    UIInstance (*fnCreateViewport)(UIInstance, UIRect) = nullptr;
    void (*fnSetViewport)(UIInstance, float, float, float, float) = nullptr;
    void (*fnGetViewport)(UIInstance, float*, float*, float*, float*) = nullptr;
    int (*fnSetViewportBackgroundColor)(UIInstance, uint8_t, uint8_t, uint8_t, uint8_t) = nullptr;

    // 视口缩放
    int (*fnSetViewportScaleMode)(UIInstance, int) = nullptr;
    int (*fnGetViewportScaleMode)(UIInstance, int*) = nullptr;
    int (*fnSetCanvasSize)(UIInstance, float, float) = nullptr;
    int (*fnGetViewportScale)(UIInstance, float*, float*) = nullptr;
    int (*fnSetViewportAnchor)(UIInstance, float, float) = nullptr;

    // 运行期窗口 API
    int (*fnGetWindowSize)(UIInstance, float*, float*) = nullptr;
    int (*fnSetWindowSize)(UIInstance, float, float) = nullptr;
    void* (*fnGetNativeWindowHandle)(UIInstance) = nullptr;
    void (*fnSetWindowResizeCallback)(UIInstance, UIWindowResizeCallback, void*) = nullptr;

    // 帧循环
    int (*fnProcessEvents)(UIInstance) = nullptr;   // 返回是否处理了事件（多实例调度）
    void (*fnUpdate)(UIInstance, double) = nullptr;
    void (*fnPushUIEvent)(UIInstance, const UIEvent*) = nullptr;
    void (*fnRender)(UIInstance) = nullptr;
    void (*fnClear)(UIInstance) = nullptr;
    void (*fnPresent)(UIInstance) = nullptr;
    int  (*fnIsQuitRequested)(UIInstance) = nullptr;
    uint32_t (*fnGetBackendCapabilities)(UIInstance) = nullptr;

    // Debug 辅助
    int        (*fnDebug_GetAliveCount)(void) = nullptr;
    UIInstance (*fnDebug_GetAliveInstance)(int) = nullptr;
    UIInstance (*fnDebug_GetActiveViewport)(UIInstance) = nullptr;
    int        (*fnDebug_IsControlFocused)(UIInstance, UIControlHandle) = nullptr;

    // 布局系统
    int             (*fnLoadLayout)(UIInstance, const char*) = nullptr;
    int             (*fnLoadLayoutFromFile)(UIInstance, const char*) = nullptr;
    UIControlHandle (*fnFindControl)(UIInstance, const char*) = nullptr;
    void            (*fnRegisterAction)(UIInstance, const char*, UIActionCallback, void*) = nullptr;

    // 编程式控件创建
    UIControlHandle (*fnCreateButton)(UIInstance, const char*, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateLabel)(UIInstance, const char*, float, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateCheckBox)(UIInstance, const char*, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateEditBox)(UIInstance, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateProgressBar)(UIInstance, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateSlider)(UIInstance, float, float, float, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreatePanel)(UIInstance, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateTextArea)(UIInstance, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateWinFrame)(UIInstance, const char*, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateMenuBar)(UIInstance, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateMenuPanel)(UIInstance, float, float) = nullptr;
    UIControlHandle (*fnCreateMenuItem)(UIInstance, const char*, int, float, float) = nullptr;
    void (*fnMenuBarAddMenu)(UIInstance, UIControlHandle, const char*, UIControlHandle) = nullptr;
    void (*fnMenuPanelAddItem)(UIInstance, UIControlHandle, UIControlHandle) = nullptr;
    void (*fnMenuPanelAddSeparator)(UIInstance, UIControlHandle) = nullptr;
    void (*fnMenuItemSetSubMenu)(UIInstance, UIControlHandle, UIControlHandle) = nullptr;
    UIControlHandle (*fnCreateColorPicker)(UIInstance, float, float, float, float, const char*, float, float) = nullptr;
    UIControlHandle (*fnCreateNumericUpDown)(UIInstance, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateComboBox)(UIInstance, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateSplitter)(UIInstance, float, float, float, float, int, float, float) = nullptr;
    UIControlHandle (*fnCreateScrollBar)(UIInstance, float, float, float, float, int, float, float) = nullptr;
    UIControlHandle (*fnCreateTreeView)(UIInstance, float, float, float, float, float, float) = nullptr;

    // Shape 形状控件
    UIControlHandle (*fnCreateShape)(UIInstance, float, float, float, float, float, float) = nullptr;
    // ListView 列表控件（属性走 Control::Set* 通用接口；数据/对象类专用）
    UIControlHandle (*fnCreateListView)(UIInstance, float, float, float, float, float, float) = nullptr;
    // StatusBar 状态栏控件
    UIControlHandle (*fnCreateStatusBar)(UIInstance, float, float, float, float, float, float) = nullptr;
    int (*fnStatusBarAddItem)(UIInstance, UIControlHandle, const char*, const char*, int) = nullptr;
    int (*fnStatusBarSetItemText)(UIInstance, UIControlHandle, const char*, const char*) = nullptr;
    int (*fnStatusBarRemoveItem)(UIInstance, UIControlHandle, const char*) = nullptr;
    int (*fnListViewAddRow)(UIInstance, UIControlHandle, const char*, int, const char* const*) = nullptr;
    int (*fnListViewRemoveRow)(UIInstance, UIControlHandle, int) = nullptr;
    int (*fnListViewSetCellText)(UIInstance, UIControlHandle, int, int, const char*) = nullptr;
    int (*fnListViewGetCellText)(UIInstance, UIControlHandle, int, int, char*, int) = nullptr;
    int (*fnListViewAddColumn)(UIInstance, UIControlHandle, const char*, float, int) = nullptr;
    int (*fnListViewSetColumnWidth)(UIInstance, UIControlHandle, int, float) = nullptr;
    int (*fnListViewSetCellLeadingControl)(UIInstance, UIControlHandle, int, int, void*) = nullptr;
    int (*fnShapeSetPoints)(UIInstance, UIControlHandle, int, const float*, const float*) = nullptr;
    int (*fnShapeMapToDrawPoint)(UIInstance, UIControlHandle, float, float, float*, float*) = nullptr;
    int (*fnShapeAddPrimitive)(UIInstance, UIControlHandle, const char*, float, float, float, float) = nullptr;
    int (*fnShapeSetPrimitiveColor)(UIInstance, UIControlHandle, int, const char*, UIColor) = nullptr;
    int (*fnShapeSetPrimitiveFloat)(UIInstance, UIControlHandle, int, const char*, float) = nullptr;
    int (*fnShapeSetPrimitivePoints)(UIInstance, UIControlHandle, int, int, const float*, const float*) = nullptr;
    int (*fnShapeClearPrimitives)(UIInstance, UIControlHandle) = nullptr;
    int (*fnActorSetSourceRect)(UIInstance, UIControlHandle, float, float, float, float) = nullptr;
    int  (*fnTreeViewAddNode)(UIInstance, UIControlHandle, const char*, const char*, const char*, int) = nullptr;
    int  (*fnTreeViewRemoveNode)(UIInstance, UIControlHandle, const char*) = nullptr;
    int  (*fnTreeViewSetNodeLabel)(UIInstance, UIControlHandle, const char*, const char*) = nullptr;
    int  (*fnTreeViewSetNodeUserData)(UIInstance, UIControlHandle, const char*, void*) = nullptr;
    int  (*fnTreeViewSelectNode)(UIInstance, UIControlHandle, const char*) = nullptr;
    void (*fnTreeViewClearSelection)(UIInstance, UIControlHandle) = nullptr;
    int  (*fnTreeViewExpandNode)(UIInstance, UIControlHandle, const char*) = nullptr;
    int  (*fnTreeViewCollapseNode)(UIInstance, UIControlHandle, const char*) = nullptr;
    void (*fnTreeViewExpandAll)(UIInstance, UIControlHandle) = nullptr;
    void (*fnTreeViewCollapseAll)(UIInstance, UIControlHandle) = nullptr;
    void (*fnTreeViewClearItems)(UIInstance, UIControlHandle) = nullptr;
    int  (*fnTreeViewGetSelectedId)(UIInstance, UIControlHandle, char*, int) = nullptr;

    // EditBox / TextArea 文本操作
    int  (*fnEditBoxSelectAll)(UIInstance, UIControlHandle) = nullptr;
    int  (*fnEditBoxSetSelection)(UIInstance, UIControlHandle, int, int) = nullptr;
    int  (*fnEditBoxClearSelection)(UIInstance, UIControlHandle) = nullptr;
    int  (*fnEditBoxHasSelection)(UIInstance, UIControlHandle) = nullptr;
    int  (*fnEditBoxGetCursorPosition)(UIInstance, UIControlHandle) = nullptr;
    int  (*fnEditBoxCopy)(UIInstance, UIControlHandle) = nullptr;
    int  (*fnEditBoxCut)(UIInstance, UIControlHandle) = nullptr;
    int  (*fnEditBoxPaste)(UIInstance, UIControlHandle) = nullptr;
    int  (*fnEditBoxDeleteSelectedText)(UIInstance, UIControlHandle) = nullptr;

    // NumericUpDown 数值操作
    int  (*fnNumericUpDownStep)(UIInstance, UIControlHandle, int) = nullptr;

    // ComboBox 选项操作
    int  (*fnComboBoxAddItem)(UIInstance, UIControlHandle, const char*, const char*, int) = nullptr;
    int  (*fnComboBoxRemoveItem)(UIInstance, UIControlHandle, int) = nullptr;
    int  (*fnComboBoxClearItems)(UIInstance, UIControlHandle) = nullptr;
    int  (*fnComboBoxGetItemCount)(UIInstance, UIControlHandle) = nullptr;
    UIControlHandle (*fnCreateHandleControl)(UIInstance, UIControlHandle, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateImageButton)(UIInstance, const char*, const char*, const char*, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateImage)(UIInstance, const char*, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateAnimation)(UIInstance, const char*, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateAnimatedButton)(UIInstance, const char*, float, float, float, float, float, float) = nullptr;
    UIControlHandle (*fnCreateDialog)(UIInstance, const char*, const char*, float, float, float, float, float, float) = nullptr;

    // 控件通用操作
    void (*fnSetRect)(UIInstance, UIControlHandle, float, float, float, float) = nullptr;
    void (*fnGetRect)(UIInstance, UIControlHandle, float*, float*, float*, float*) = nullptr;
    void (*fnAddChildControl)(UIInstance, UIControlHandle, UIControlHandle) = nullptr;
    void (*fnDestroyControl)(UIInstance, UIControlHandle) = nullptr;
    const char* (*fnGetControlId)(UIInstance, UIControlHandle) = nullptr;

    // LuotiAni 动画操作
    int (*fnAnimationPrepare)(UIInstance, UIControlHandle, int) = nullptr;
    int (*fnAnimationSetFrameFilter)(UIInstance, UIControlHandle, int) = nullptr;

    // 截图读回
    int (*fnCaptureRect)(UIInstance, float, float, float, float, uint8_t*, int*, int*) = nullptr;
    int (*fnCaptureViewport)(UIInstance, uint8_t*, int*, int*) = nullptr;
    int (*fnCaptureBench)(UIInstance, uint8_t*, int*, int*) = nullptr;
    int (*fnCaptureControl)(UIInstance, UIControlHandle, uint8_t*, int*, int*) = nullptr;
    int (*fnSavePixelsToFile)(const uint8_t*, int, int, const char*) = nullptr;

    // 属性系统
    int (*fnSetColor)(UIInstance, UIControlHandle, const char*, UIColor) = nullptr;
    int (*fnSetStateColor)(UIInstance, UIControlHandle, const char*, UIStateColor) = nullptr;
    int (*fnSetBool)(UIInstance, UIControlHandle, const char*, int) = nullptr;
    int (*fnSetInt)(UIInstance, UIControlHandle, const char*, int) = nullptr;
    int (*fnSetFloat)(UIInstance, UIControlHandle, const char*, float) = nullptr;
    int (*fnSetString)(UIInstance, UIControlHandle, const char*, const char*) = nullptr;
    int (*fnSetEnum)(UIInstance, UIControlHandle, const char*, const char*) = nullptr;
    int (*fnSetPtr)(UIInstance, UIControlHandle, const char*, void*) = nullptr;
    int (*fnGetColor)(UIInstance, UIControlHandle, const char*, UIColor*) = nullptr;
    int (*fnGetStateColor)(UIInstance, UIControlHandle, const char*, UIStateColor*) = nullptr;
    int (*fnGetBool)(UIInstance, UIControlHandle, const char*, int*) = nullptr;
    int (*fnGetInt)(UIInstance, UIControlHandle, const char*, int*) = nullptr;
    int (*fnGetFloat)(UIInstance, UIControlHandle, const char*, float*) = nullptr;
    int (*fnGetString)(UIInstance, UIControlHandle, const char*, char*, int) = nullptr;
    int (*fnGetEnum)(UIInstance, UIControlHandle, const char*, char*, int) = nullptr;
    int (*fnGetPtr)(UIInstance, UIControlHandle, const char*, void**) = nullptr;
    int (*fnGetControlType)(UIInstance, UIControlHandle, char*, int) = nullptr;

    // 回调
    int (*fnSetCallback)(UIInstance, UIControlHandle, const char*, UIEventCallback, void*) = nullptr;

    // 核心 DLL 句柄（进程级驻留，不主动卸载）
    void* coreHandle = nullptr;
    bool loaded = false;
};

// 进程级函数指针表（静态单例，首次访问加载）
Api& Get();

// 加载核心 DLL 并全量解析 C ABI 导出。重复调用幂等。
// dllName 缺省 "UICornerstone.dll"（exe 同目录 / 系统搜索）。
bool LoadCore(const char* dllName);

// 加载后端插件 DLL（UIBackend_<backend>.dll）。
// searchPath 非空 → 显式路径拼接；否则交给系统搜索（exe 同目录）。
// 返回回调表 + DLL 句柄（失败 {nullptr, nullptr}）。
std::pair<UIBackendCallbacks*, void*> LoadBackend(
    const std::string& searchPath, const std::string& backend);

// 便捷引用（调用点书写更短）：Dyn::API().fnProcessEvents(inst)
inline Api& API() { return Get(); }

} // namespace Dyn
} // namespace UICornerstone

#endif // UICORNERSTONE_BINDING_DYNAMIC_API_H
