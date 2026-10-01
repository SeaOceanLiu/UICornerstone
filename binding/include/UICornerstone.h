// UICornerstone C++ Binding — 主类入口
// 许可证 MIT。仅依赖 include/UICornerstoneAPI.h 的 C ABI，不引用核心库内部头。
#ifndef UICORNERSTONE_BINDING_UICORNERSTONE_H
#define UICORNERSTONE_BINDING_UICORNERSTONE_H

#include <string>
#include <memory>
#include <functional>
#include <utility>
#include <vector>
#include "UICornerstoneAPI.h"

// 窗口标志（语义命名）。与核心 include/Window.h 的 UIWindowFlags 值保持
// 一致（Binding 独立发布、不引用核心头，故在此复刻，供 C++ Binding 用户
// 以 UIWindowFlags::Fullscreen 等命名而非硬编码数值）。
// 值引用 UICornerstoneAPI.h 的 UICORN_WINDOW_FLAG_* 宏（与核心 Window.h 同源，
// 消除两处定义漂移风险）
namespace UIWindowFlags {
    constexpr uint32_t None       = UICORN_WINDOW_FLAG_NONE;
    constexpr uint32_t Fullscreen = UICORN_WINDOW_FLAG_FULLSCREEN; // SDL_WINDOW_FULLSCREEN
    constexpr uint32_t Resizable  = UICORN_WINDOW_FLAG_RESIZABLE;  // SDL_WINDOW_RESIZABLE
    constexpr uint32_t Vsync      = UICORN_WINDOW_FLAG_VSYNC;      // 应用层保留位（raylib 创建期有效）
}

class Control;
class Event;

namespace UICornerstone {

// 内部实现（pimpl）。定义见 src/Impl.h。
struct Impl;

class UICornerstone {
public:
    struct Config {
        std::string backend = "sdl3";             // "sdl3" | "sfml" | "raylib"
        std::string backendSearchPath;            // 后端 DLL 搜索目录（空=exe 同目录/系统搜索）
        std::string coreLibraryDir;               // 核心 DLL 所在目录（空=exe 同目录/系统搜索）
        std::string resourceRoot;                 // 资源根路径（空 → 核心默认 exe 目录/assets，任何 cwd 均可运行）
        std::string windowTitle    = "UICornerstone";
        int windowWidth  = 1024;
        int windowHeight = 768;
        uint32_t windowFlags = 0;                 // UIWindowFlags 位组合（WithWindowFlags(UIWindowFlags::Fullscreen) 等）

        Config& WithBackend(const std::string& name)          { backend = name; return *this; }
        Config& WithBackendSearchPath(const std::string& p)   { backendSearchPath = p; return *this; }
        Config& WithCoreLibraryDir(const std::string& p)      { coreLibraryDir = p; return *this; }
        Config& WithResourceRoot(const std::string& r)        { resourceRoot = r; return *this; }
        Config& WithWindow(const std::string& t, int w, int h)
            { windowTitle = t; windowWidth = w; windowHeight = h; return *this; }
        Config& WithWindowFlags(uint32_t f)                   { windowFlags = f; return *this; }
    };

    // 通过 Config 创建（核心/后端 DLL 均经 LoadLibrary 纯动态加载，见 §5.15）
    static std::unique_ptr<UICornerstone> Create(const Config& config);
    // 通过回调查表创建（不管理后端生命周期）
    static std::unique_ptr<UICornerstone> Create(const UIBackendCallbacks* callbacks,
                                                 const Config& config = Config{});

    ~UICornerstone();
    UICornerstone(const UICornerstone&) = delete;
    UICornerstone& operator=(const UICornerstone&) = delete;

    // ── 实例句柄 ──
    UIInstance Handle() const;

    // ── Hosted 模式 ──
    using FrameCallback  = std::function<void(double deltaTime)>;
    using RenderCallback = std::function<void()>;
    int Run(FrameCallback update, RenderCallback onRender = nullptr);

    // ── Embedded 模式 ──
    bool ProcessEvents();   // 返回是否处理了至少一个事件（多实例主循环调度用）
    void Update(double deltaTime);
    void Render();
    void Clear();
    void Present();
    bool IsQuitRequested() const;
    // 后端能力位（UICORN_BACKEND_CAP_*，按位与）：调用方据此决定行为——
    // 例如多实例双窗口渲染仅在 MULTI_WINDOW 能力下执行（单窗口架构后端
    // 如 raylib 非首个实例为 headless，渲染会串扰到主实例窗口）。
    uint32_t GetBackendCapabilities() const;
    void Shutdown();

    // ── 运行期窗口 API（见设计 §21）──
    // 窗口尺寸查询（headless 实例返回 0×0）；false = 无窗口实例。
    bool GetWindowSize(float& w, float& h) const;
    // 请求窗口 resize（实现经后端 Window::setSize，尺寸变化经 WindowResize
    // 事件回流并触发 SetWindowResizeCallback）。false = headless/无窗口。
    bool SetWindowSize(float w, float h);
    // 原生窗口句柄（嵌入/hosted 场景）；无窗口返回 nullptr。
    void* GetNativeWindowHandle() const;
    using WindowResizeCallback = std::function<void(int width, int height)>;
    // 窗口尺寸变化回调（ProcessEvents 处理 WindowResize 时分发，替代帧内轮询）。
    void SetWindowResizeCallback(WindowResizeCallback callback);

    // ── 子视口 ──
    std::unique_ptr<UICornerstone> CreateViewport(float x, float y, float w, float h);

    // ── 资源路径（仅影响 Binding 侧路径解析；核心库 config 已固化）──
    void SetResourceRoot(const std::string& path);
    std::string GetResourceRoot() const;
    std::string ResolveResource(const std::string& relativePath) const;

    // ── 内存资源注册表（MemoryResourceProvider，懒创建 + 自动挂载）──
    // 拷贝注册：引擎内部复制 data，调用方可立即释放。需在 Create 之后调用。
    bool RegisterResource(const std::string& name, const void* data, size_t len);
    // 零拷贝注册：引擎不复制、仅引用 data（调用方须保持缓冲有效直至实例销毁；
    // 析构/覆盖时经 freeFn 回调释放，freeFn 空 → 默认 free）。
    bool AdoptResource(const std::string& name, void* data, size_t len,
                       std::function<void(void*)> freeFn = nullptr);

    // ── 布局 ──
    bool LoadLayout(const std::string& jsonContent);
    bool LoadLayoutFromFile(const std::string& filePath);
    Control FindControl(const std::string& id);
    // 给编程式创建的控件设置 id（注册到实例 id 表，供 FindControl 查询）。
    // id 空串 → 移除该控件名下全部 id。返回 true 成功。
    bool SetControlId(Control ctl, const std::string& id);
    // 将裸句柄（如 GetPtr("item-leading-control") 返回值）包装为 Control 代理。
    // 句柄须属于本实例；重复包装共享同一代理状态（生命周期/有效性追踪）。
    Control FromHandle(UIControlHandle handle);

    // ── 控件工厂 ──
    Control CreateButton(const std::string& text, float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateLabel(const std::string& text, float fontSize, float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateCheckBox(const std::string& text, float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateEditBox(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateProgressBar(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateSlider(float x, float y, float w, float h, float min, float max, float value, float xScale = 1.0f, float yScale = 1.0f);
    Control CreatePanel(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateTextArea(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateWinFrame(const std::string& title, float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateComboBox(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateColorPicker(float x, float y, float w, float h, const std::string& color, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateNumericUpDown(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateSplitter(float x, float y, float w, float h, int orientation, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateImage(const std::string& image, float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateAnimation(const std::string& jsoncPath, float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateAnimatedButton(const std::string& jsoncPath, float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateDialog(const std::string& confirmText, const std::string& cancelText,
                         float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreatePopup(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);   // P0-27d（不自动 open）
    Control CreateConfirmPopup(const std::string& confirmText,
                               float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);

    // ── 菜单族 / 滚动条 / 树 / 句柄 ──
    Control CreateMenuBar(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateMenuPanel(float xScale = 1.0f, float yScale = 1.0f);
    Control CreateMenuItem(const std::string& caption, int type, float xScale = 1.0f, float yScale = 1.0f);
    void MenuBarAddMenu(Control& bar, const std::string& caption, Control& panel);
    void MenuPanelAddItem(Control& panel, Control& item);
    void MenuPanelAddSeparator(Control& panel);
    void MenuItemSetSubMenu(Control& item, Control& panel);
    Control CreateScrollBar(float x, float y, float w, float h, int orientation, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateTreeView(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    // Shape 形状控件（参数经 Control::Set* 属性接口；SetPoints/MapToDrawPoint 走专用方法）
    Control CreateShape(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateContextMenu(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateStatusBar(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    Control CreateTabControl(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);

    // ── StatusBar 段操作 ──
    bool StatusBarAddItem(Control& bar, const std::string& id, const std::string& text, bool rightAlign = false);
    bool StatusBarSetItemText(Control& bar, const std::string& id, const std::string& text);
    bool StatusBarRemoveItem(Control& bar, const std::string& id);
    bool StatusBarSetItemIcon(Control& bar, const std::string& id, Control& iconControl);
    bool StatusBarSetItemMenu(Control& bar, const std::string& id, Control& menuPanel);

    // ── ContextMenu 组装/显示 ──
    bool ContextMenuAddItem(Control& menu, const std::string& caption, const std::string& shortcut = "");
    bool ContextMenuAddSeparator(Control& menu);
    bool ContextMenuShow(Control& menu, float x, float y);
    bool ContextMenuClose(Control& menu);

    // ── TabControl 页操作 ──
    int  TabAddPage(Control& tab, const std::string& title);
    bool TabSetPage(Control& tab, int index, Control& page);
    bool TabSetTitle(Control& tab, int index, const std::string& title);
    bool TabSetTabLeadingControl(Control& tab, int index, Control& ctl);
    void ShapeSetPoints(Control& sh, const std::vector<std::pair<float, float>>& pts); // 本地像素
    std::pair<float, float> ShapeMapToDrawPoint(Control& sh, float lx, float ly);      // 本地 → 全局
    // 多图元（组合图形）：与 C ABI 一一对应（覆盖层：选中框/控点/对齐线，动态改点集）
    int  ShapeAddPrimitive(Control& sh, const std::string& type, float x, float y, float w, float h);
    void ShapeClearPrimitives(Control& sh);
    void ShapeSetPrimitiveColor(Control& sh, int idx, const char* prop, UIColor value);   // prop: "fill"/"stroke"
    void ShapeSetPrimitiveFloat(Control& sh, int idx, const char* prop, float value);     // "line-width"/"radius"/"ring-width"
    void ShapeSetPrimitivePoints(Control& sh, int idx, const std::vector<std::pair<float,float>>& pts);
    // Image/Actor 源矩形（纹理子区域；对 stretch 与 tile 生效；清除传 x<0）
    void ImageSetSourceRect(Control& img, float x, float y, float w, float h);

    // ── TreeView 节点操作 ──
    // parentId 空串 = 插入为根节点；返回 true 成功 / false 失败
    bool TreeViewAddNode(Control& tree, const std::string& parentId, const std::string& id,
                         const std::string& label, bool expanded = false);
    bool TreeViewRemoveNode(Control& tree, const std::string& id);
    bool TreeViewSetNodeLabel(Control& tree, const std::string& id, const std::string& label);
    bool TreeViewSetNodeUserData(Control& tree, const std::string& id, void* userData);
    bool TreeViewSelectNode(Control& tree, const std::string& id);
    void TreeViewClearSelection(Control& tree);
    bool TreeViewExpandNode(Control& tree, const std::string& id);
    bool TreeViewCollapseNode(Control& tree, const std::string& id);
    void TreeViewExpandAll(Control& tree);
    void TreeViewCollapseAll(Control& tree);
    void TreeViewClearItems(Control& tree);
    std::string TreeViewGetSelectedId(Control& tree);

    // ── EditBox / TextArea 文本操作 ──
    void EditBoxSelectAll(Control& ctl);
    void EditBoxSetSelection(Control& ctl, int start, int end);
    void EditBoxClearSelection(Control& ctl);
    bool EditBoxHasSelection(Control& ctl);
    int  EditBoxGetCursorPosition(Control& ctl);
    void EditBoxCopy(Control& ctl);
    void EditBoxCut(Control& ctl);
    void EditBoxPaste(Control& ctl);
    void EditBoxDeleteSelectedText(Control& ctl);

    // ── NumericUpDown 数值操作 ──
    void NumericUpDownStep(Control& ctl, int dir);

    // ── ComboBox 选项操作 ──
    bool ComboBoxAddItem(Control& ctl, const std::string& label, const std::string& value, bool disabled = false);
    bool ComboBoxRemoveItem(Control& ctl, int index);
    bool ComboBoxClearItems(Control& ctl);
    int  ComboBoxGetItemCount(Control& ctl);

    // ── ListView 列表控件（属性走 Control::Set*；数据走专用方法）──
    Control CreateListView(float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);
    bool ListViewAddRow(Control& lv, const std::string& id, const std::vector<std::string>& cells = {});
    bool ListViewRemoveRow(Control& lv, int index);
    bool ListViewSetCellText(Control& lv, int row, int col, const std::string& text);
    std::string ListViewGetCellText(Control& lv, int row, int col);
    int  ListViewAddColumn(Control& lv, const std::string& title, float width, bool sortable = false);
    bool ListViewSetColumnWidth(Control& lv, int index, float width);
    bool ListViewInsertRow(Control& lv, int index, const std::string& id, const std::vector<std::string>& cells = {});
    bool ListViewSetRowCells(Control& lv, int index, const std::vector<std::string>& cells);
    int  ListViewInsertColumn(Control& lv, int index, const std::string& title, float width, bool sortable = false);
    bool ListViewRemoveColumn(Control& lv, int index);
    bool ListViewSetRowLeadingControl(Control& lv, int index, Control& iconControl);
    bool ListViewSetCellLeadingControl(Control& lv, int row, int col, Control& ctl);
    bool ListViewSetCellStyle(Control& lv, int row, int col, UIColor bg, int fontSize);
    bool ListViewSetColumnHeaderStyle(Control& lv, int colIndex, UIColor color, int fontSize);
    bool ListViewSetColumnIcon(Control& lv, int colIndex, Control& iconControl);
    bool ListViewSetColumnSorter(Control& lv, int colIndex, ListViewSortFn cmp, void* userData = nullptr);
    bool ListViewSetColumnValues(Control& lv, int colIndex, const std::vector<std::string>& values);
    bool ListViewSetCellShadow(Control& lv, int row, int col, UIColor color, float offsetX = 1.0f, float offsetY = 1.0f);

    // ── LuotiAni 动画操作 ──
    bool AnimationPrepare(Control& ctl, int startFrame = 0);
    bool AnimationSetFrameFilter(Control& ctl, bool bilinear);

    // ── 截图读回（像素级测试辅助；需后端 READBACK 能力位）──
    bool CaptureRect(float x, float y, float w, float h, uint8_t* outPixels, int* outW, int* outH) const;
    bool CaptureViewport(uint8_t* out, int* w, int* h) const;
    bool CaptureBench(uint8_t* out, int* w, int* h) const;
    bool CaptureControl(Control& ctl, uint8_t* out, int* w, int* h) const;
    // 将 RGBA8888 像素缓冲保存为 BMP 文件（与实例无关，线程安全）
    static bool SavePixelsToFile(const uint8_t* pixels, int w, int h, const std::string& filePath);

    // ── 字体枚举清单（P0-48 扩展②，静态；核心库未加载时返回 0/空）──
    static int GetFontCount();
    static std::string GetFontName(int index);
    static std::vector<std::string> GetFontNames();

    Control CreateHandleControl(Control target, float x, float y, float w, float h, float xScale = 1.0f, float yScale = 1.0f);

    // ── HandleControl 集成（第二批）──
    // 切换手柄附加目标；target 无效/默认构造 = detach（自移出父容器 + 恢复光标）。
    bool SetHandleTarget(Control handle, Control target);
    // 手柄命中查询（窗口屏幕坐标，与 UIEvent mousePos 同参照系）。命中返回 true 并写 outHandleType
    // （None=0、Move=1、NW=2、N=3、NE=4、E=5、SE=6、S=7、SW=8、W=9）。
    bool HandleHitTest(Control handle, float x, float y, int& outHandleType);
    // 拖拽/移动几何过滤回调（吸附/对齐线扩展点）。filter==nullptr 清除。
    // 回调签名复用 C ABI UIHandleRectFilter（target 为 UIControlHandle，局部坐标 io 修正，返回 1 用修正值）。
    bool SetHandleRectFilter(Control handle, UIHandleRectFilter filter, void* userData);
    // Move 手柄可见性（策略开关）。
    bool SetHandleMoveVisible(Control handle, bool show);

    // ── 视口 ──
    void SetViewport(float x, float y, float w, float h);
    UIRect GetViewport() const;
    bool SetViewportBackgroundColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);  // RGBA8888，默认透明=不填充

    // ── 视口缩放 ──
    // mode: 0=off（画布跟随窗口） 1=fit（等比居中） 2=stretch（拉伸铺满）
    bool SetViewportScaleMode(int mode);
    int GetViewportScaleMode() const;
    bool SetCanvasSize(float w, float h);   // 显式基准画布（fit/stretch 适配基准）
    bool GetViewportScale(float& sx, float& sy) const;
    // 实例缩放（画布语义）：逻辑 rect 不变，视觉/字号按比例缩放；off 分支尊重
    bool SetInstanceScale(float xScale, float yScale);
    float GetInstanceScaleX() const;
    float GetInstanceScaleY() const;
    bool SetViewportAnchor(float ax, float ay);

    // ── 事件注入 ──
    void PushEvent(const UIEvent& event);
    void PushMouseButton(int button, float x, float y, bool down);
    void PushMouseMove(float x, float y);
    void PushMouseWheel(float dx, float dy, float x, float y);
    void PushKey(int keyCode, uint16_t mod, bool down);
    void PushTextInput(const std::string& text);

    // ── JSON 布局动作注册 ──
    using ActionCallback = std::function<void(Control)>;
    void RegisterAction(const std::string& name, ActionCallback callback);

    // ── 后端配置 ──
    bool SetBackendConfig(const char* key, const char* value);
    bool SetBackendConfigBool(const char* key, bool value);
    bool GetBackendConfigBool(const char* key, bool& out) const;

    // ── 错误查询 ──
    const std::string& GetLastError() const;

    // ── Debug 辅助 ──
    static int  DebugGetAliveCount();
    static UIInstance DebugGetAliveInstance(int index);
    static UIInstance DebugGetActiveViewport(UIInstance instance);
    static bool DebugIsControlFocused(UIInstance instance, UIControlHandle control);

private:
    explicit UICornerstone(UIInstance instance, bool ownsInstance);
    Control MakeControl(UIControlHandle h);
    friend class ::Control;

    std::unique_ptr<Impl> m_impl;
};

} // namespace UICornerstone

#endif