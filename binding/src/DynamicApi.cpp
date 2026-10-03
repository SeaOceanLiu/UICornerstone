// UICornerstone C++ Binding — 动态 API 层实现（纯 LoadLibrary 模式）
// 许可证 MIT。所有 C ABI 导出均经 GetProcAddress 运行时解析，
// 不链接 UICornerstone_dll.lib；核心 DLL 进程级驻留（不主动 FreeLibrary）。
#include "DynamicApi.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#endif

namespace UICornerstone {
namespace Dyn {

// ============================================================
// GetProcAddress 解析（C4191 不安全转换警告：经 void* 槽位写入）
// 成员统一 fn 前缀（token paste）：windows.h 的 UNICODE 条件宏
// （CreateDialog→CreateDialogA/W 等）只匹配完整 token，fnCreateDialog
// 与之无冲突，故无需 #undef。
// 导出名经 #name 字符串化（# 阻止参数宏展开）→ 恒为 "UICornerstone_<name>"。
// ============================================================
#ifdef _WIN32
#define RESOLVE(name) \
    do { \
        FARPROC p = ::GetProcAddress(h, "UICornerstone_" #name); \
        if (p) { void** slot = reinterpret_cast<void**>(&api.fn##name); *slot = reinterpret_cast<void*>(p); } \
    } while (0)
#else
#define RESOLVE(name) (void)0
#endif

static Api& LoadInto(Api& api, void* handle) {
#ifdef _WIN32
    HMODULE h = static_cast<HMODULE>(handle);

    RESOLVE(CreateInstance);
    RESOLVE(DestroyInstance);
    RESOLVE(CreateInstanceFromPlugin);

    RESOLVE(SetBackendConfig);
    RESOLVE(SetBackendConfigInt);
    RESOLVE(SetBackendConfigBool);
    RESOLVE(GetBackendConfig);
    RESOLVE(GetBackendConfigInt);
    RESOLVE(GetBackendConfigBool);

    RESOLVE(CreateViewport);
    RESOLVE(SetViewport);
    RESOLVE(SetViewportBackgroundColor);
    RESOLVE(GetViewport);

    RESOLVE(SetViewportScaleMode);
    RESOLVE(GetViewportScaleMode);
    RESOLVE(SetCanvasSize);
    RESOLVE(GetViewportScale);
    RESOLVE(SetViewportAnchor);

    RESOLVE(GetWindowSize);
    RESOLVE(SetWindowSize);
    RESOLVE(GetNativeWindowHandle);
    RESOLVE(SetWindowResizeCallback);

    RESOLVE(ProcessEvents);
    RESOLVE(Update);
    RESOLVE(PushUIEvent);
    RESOLVE(Render);
    RESOLVE(RenderOverlays);
    RESOLVE(Clear);
    RESOLVE(Present);
    RESOLVE(IsQuitRequested);
    RESOLVE(GetBackendCapabilities);

    RESOLVE(Debug_GetAliveCount);
    RESOLVE(Debug_GetAliveInstance);
    RESOLVE(Debug_GetActiveViewport);
    RESOLVE(Debug_IsControlFocused);

    RESOLVE(LoadLayout);
    RESOLVE(LoadLayoutFromFile);
    RESOLVE(FindControl);
    RESOLVE(SetControlId);
    RESOLVE(RegisterAction);

    RESOLVE(CreateButton);
    RESOLVE(CreateLabel);
    RESOLVE(CreateCheckBox);
    RESOLVE(CreateEditBox);
    RESOLVE(CreateProgressBar);
    RESOLVE(CreateSlider);
    RESOLVE(CreatePanel);
    RESOLVE(CreateTextArea);
    RESOLVE(CreateWinFrame);
    RESOLVE(CreateMenuBar);
    RESOLVE(CreateMenuPanel);
    RESOLVE(CreateMenuItem);
    RESOLVE(MenuBarAddMenu);
    RESOLVE(MenuPanelAddItem);
    RESOLVE(MenuPanelAddSeparator);
    RESOLVE(MenuItemSetSubMenu);
    RESOLVE(CreateColorPicker);
    RESOLVE(CreateNumericUpDown);
    RESOLVE(CreateComboBox);
    RESOLVE(CreateSplitter);
    RESOLVE(CreateScrollBar);
    RESOLVE(CreateTreeView);
    RESOLVE(CreateShape);
    RESOLVE(CreateListView);
    RESOLVE(ListViewAddRow);
    RESOLVE(ListViewRemoveRow);
    RESOLVE(ListViewSetCellText);
    RESOLVE(ListViewGetCellText);
    RESOLVE(ListViewAddColumn);
    RESOLVE(ListViewSetColumnWidth);
    RESOLVE(ListViewSetCellLeadingControl);
    RESOLVE(CreateStatusBar);
    RESOLVE(CreateContextMenu);
    RESOLVE(CreateTabControl);
    RESOLVE(StatusBarAddItem);
    RESOLVE(StatusBarSetItemText);
    RESOLVE(StatusBarSetItemTextColor);
    RESOLVE(StatusBarSetItemBackgroundColor);
    RESOLVE(StatusBarSetItemFontSize);
    RESOLVE(StatusBarSetItemTextShadow);
    RESOLVE(StatusBarRemoveItem);
    RESOLVE(ListViewInsertRow);
    RESOLVE(ListViewInsertColumn);
    RESOLVE(ListViewRemoveColumn);
    RESOLVE(ListViewSetRowCells);
    RESOLVE(ListViewSetRowLeadingControl);
    RESOLVE(ListViewSetCellStyle);
    RESOLVE(ListViewSetColumnHeaderStyle);
    RESOLVE(ListViewSetColumnHeaderBackground);
    RESOLVE(ListViewSetColumnHeaderShadow);
    RESOLVE(ListViewSetColumnIcon);
    RESOLVE(ListViewSetColumnSorter);
    RESOLVE(ListViewSetColumnValues);
    RESOLVE(ListViewSetCellShadow);
    RESOLVE(StatusBarSetItemIcon);
    RESOLVE(StatusBarSetItemMenu);
    RESOLVE(ContextMenuAddItem);
    RESOLVE(ContextMenuAddSeparator);
    RESOLVE(ContextMenuShow);
    RESOLVE(ContextMenuClose);
    RESOLVE(TabAddPage);
    RESOLVE(TabSetPage);
    RESOLVE(TabSetTitle);
    RESOLVE(TabSetTabLeadingControl);
    RESOLVE(ShapeSetPoints);
    RESOLVE(ShapeMapToDrawPoint);
    RESOLVE(ShapeAddPrimitive);
    RESOLVE(ShapeSetPrimitiveColor);
    RESOLVE(ShapeSetPrimitiveFloat);
    RESOLVE(ShapeSetPrimitivePoints);
    RESOLVE(ShapeClearPrimitives);
    RESOLVE(ActorSetSourceRect);
    RESOLVE(SetInstanceScale);
    RESOLVE(GetInstanceScale);
    RESOLVE(TreeViewAddNode);
    RESOLVE(TreeViewRemoveNode);
    RESOLVE(TreeViewSetNodeLabel);
    RESOLVE(TreeViewSetNodeUserData);
    RESOLVE(TreeViewSelectNode);
    RESOLVE(TreeViewClearSelection);
    RESOLVE(TreeViewExpandNode);
    RESOLVE(TreeViewCollapseNode);
    RESOLVE(TreeViewExpandAll);
    RESOLVE(TreeViewCollapseAll);
    RESOLVE(TreeViewClearItems);
    RESOLVE(TreeViewGetSelectedId);
    RESOLVE(EditBoxSelectAll);
    RESOLVE(EditBoxSetSelection);
    RESOLVE(EditBoxClearSelection);
    RESOLVE(EditBoxHasSelection);
    RESOLVE(EditBoxGetCursorPosition);
    RESOLVE(EditBoxCopy);
    RESOLVE(EditBoxCut);
    RESOLVE(EditBoxPaste);
    RESOLVE(EditBoxDeleteSelectedText);
    RESOLVE(NumericUpDownStep);
    RESOLVE(ComboBoxAddItem);
    RESOLVE(ComboBoxRemoveItem);
    RESOLVE(ComboBoxClearItems);
    RESOLVE(ComboBoxGetItemCount);
    RESOLVE(CreateHandleControl);
    RESOLVE(SetHandleTarget);
    RESOLVE(HandleHitTest);
    RESOLVE(SetHandleRectFilter);
    RESOLVE(SetHandleMoveVisible);
    RESOLVE(CreateImage);
    RESOLVE(CreateAnimation);
    RESOLVE(CreateAnimatedButton);
    RESOLVE(CreateDialog);
    RESOLVE(CreatePopup);
    RESOLVE(CreateConfirmPopup);

    RESOLVE(SetRect);
    RESOLVE(GetRect);
    RESOLVE(AddChildControl);
    RESOLVE(GetRoot);
    RESOLVE(RemoveChild);
    RESOLVE(DestroyControl);
    RESOLVE(GetControlId);
    RESOLVE(AnimationPrepare);
    RESOLVE(AnimationSetFrameFilter);
    RESOLVE(CaptureRect);
    RESOLVE(CaptureViewport);
    RESOLVE(CaptureBench);
    RESOLVE(CaptureControl);
    RESOLVE(SavePixelsToFile);
    RESOLVE(GetFontCount);
    RESOLVE(GetFontName);

    RESOLVE(SetColor);
    RESOLVE(SetStateColor);
    RESOLVE(SetBool);
    RESOLVE(SetInt);
    RESOLVE(SetFloat);
    RESOLVE(SetString);
    RESOLVE(SetEnum);
    RESOLVE(SetPtr);
    RESOLVE(GetColor);
    RESOLVE(GetStateColor);
    RESOLVE(GetBool);
    RESOLVE(GetInt);
    RESOLVE(GetFloat);
    RESOLVE(GetString);
    RESOLVE(GetEnum);
    RESOLVE(GetPtr);
    RESOLVE(GetControlType);

    RESOLVE(SetCallback);
#else
    (void)api; (void)handle;
#endif
    return api;
}

// ============================================================
// 进程级单例
// ============================================================
static Api& Instance() {
    static Api api;
    return api;
}

Api& Get() {
    return Instance();
}

bool LoadCore(const char* dllName) {
    Api& api = Instance();
    if (api.loaded) return true;

#ifdef _WIN32
    HMODULE h = ::LoadLibraryA(dllName);
    if (!h) return false;
    LoadInto(api, h);
    if (!api.fnCreateInstance) {   // 关键函数必须解析成功
        ::FreeLibrary(h);
        return false;
    }
    api.coreHandle = h;
    api.loaded = true;
    return true;
#else
    (void)dllName;
    return false;
#endif
}

std::pair<UIBackendCallbacks*, void*> LoadBackend(
    const std::string& searchPath, const std::string& backend)
{
#ifdef _WIN32
    std::string dllName = "UIBackend_" + backend + ".dll";
    std::string dllPath = searchPath.empty() ? dllName : (searchPath + "/" + dllName);
    HMODULE h = ::LoadLibraryA(dllPath.c_str());
    if (!h) return {nullptr, nullptr};
    auto fn = reinterpret_cast<UIBackendCallbacks* (*)(void)>(::GetProcAddress(h, "GetUIBackendCallbacks"));
    if (!fn) { ::FreeLibrary(h); return {nullptr, nullptr}; }
    return {fn(), reinterpret_cast<void*>(h)};
#else
    (void)searchPath; (void)backend;
    return {nullptr, nullptr};
#endif
}

#undef RESOLVE

} // namespace Dyn
} // namespace UICornerstone
