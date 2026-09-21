#ifndef UICONTEXT_H
#define UICONTEXT_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <memory>
#include <vector>
#include <utility>
#include <mutex>
#include "UICornerstoneAPI.h"
#include "SColor.h"
#include "Utility.h"

class Window;
class RenderDevice;
class InputBackend;
class TextRenderer;
class ResourceProvider;
class MainWindow;
class Bench;
class EventQueue;
class DataContext;
class FocusManager;
class Popup;
class BackendManager;
class Control;

// 实例上下文：聚合一个 UI 实例（窗口或视口）的全部状态。
// UIInstance = UIContext*（见 UICornerstoneAPI.h）。
struct UIContext {
    // ── 层级关系 ──
    UIContext* owner = nullptr;        // 拥有后端的父实例，nullptr = 自己是 owner
    bool ownsBackend = true;           // false = 共享 owner 的后端
    std::vector<UIContext*> children;  // 子视口列表（CreateViewport 注册）

    // ── Backend 资源 ──
    // 当 ownsBackend == false 时以下指针从 owner 继承
    const UIBackendCallbacks* callbacks = nullptr;
    BackendManager* backendManager = nullptr;
    Window* window = nullptr;
    RenderDevice* renderDevice = nullptr;
    InputBackend* inputBackend = nullptr;
    TextRenderer* textRenderer = nullptr;
    ResourceProvider* resourceProvider = nullptr;

    // ── 实例配置 ──
    std::string windowTitle;   // 窗口标题，空 → "UICornerstone"
    int windowWidth = 0;       // 0 → 默认 1024
    int windowHeight = 0;      // 0 → 默认 768
    uint32_t windowFlags = 0;  // 跨后端统一窗口标志（UIWindowFlags，见 Window.h）
    std::string resourceRoot;  // 资源根目录，空 → ConstDef::pathPrefix

    // ── 视口状态 ──
    bool initialized = false;
    bool quit = false;
    bool destroying = false;   // DestroyInstance 期间置位，防回调重入
    SRect viewport{0, 0, 1024, 768};
    // 视口背景色（Render 前填充视口区域；默认透明 = 不填充，透出外层 Clear 色）
    SColor viewportBackground = SColor(0, 0, 0, 0);
    // 显式基准画布（视口缩放适配基准）：0 → 跟随窗口（viewport）；>0 → SetCanvasSize 显式声明
    float canvasWidth = 0.0f;
    float canvasHeight = 0.0f;
    UIInstance activeViewport = nullptr;  // 仅 owner 使用，当前焦点视口；nullptr = 无子视口或焦点在 owner 树
    FocusManager* focusManager = nullptr;  // 每实例独立焦点管理

    // ── 原单例（由 UIContext 持有） ──
    Bench* bench = nullptr;
    MainWindow* mainWindow = nullptr;
    EventQueue* eventQueue = nullptr;
    DataContext* dataContext = nullptr;

    // ── 动作与控件查找 ──
    std::unordered_map<std::string, std::pair<UIActionCallback, void*>> actions;
    // HandleControl 实例级保留：C ABI 裸句柄场景下对象所有权在引擎（detach 移出容器不销毁，
    // 设计器可对同一句柄反复 attach/detach）。随实例销毁级联释放。
    std::vector<std::shared_ptr<class Control>> handleControls;
    std::unordered_map<std::string, UIControlHandle> controlsById;

    // ── 窗口 resize 用户回调（运行期窗口 API，§21）──
    UIWindowResizeCallback windowResizeCb = nullptr;
    void* windowResizeCbUserData = nullptr;

    // ── 注入事件队列 ──
    // 跨线程（scheduleAutoQuit 的定时线程 → UICornerstone_PushUIEvent）与
    // 主线程（帧循环消费）访问，必须互斥保护。
    mutable std::mutex queuedEventsMutex;
    std::queue<UIEvent> queuedEvents;

    // ── 弹窗 / 菜单生命周期池 ──
    std::vector<std::shared_ptr<Popup>> popupPool;
    std::vector<std::shared_ptr<Control>> menuPool;

    // ── 实例内字符串缓冲（GetControlId 输出） ──
    std::string strBuf;

    // ── 调试标识 ──
    uint32_t instanceId = 0;
    std::string debugLabel;

    // ── 调试鼠标注入（仅 _DEBUG 有效，无人值守测试驱动 hover 状态） ──
    // SetMousePosition 置位后，本实例控件树的 hover 判定用注入坐标替代
    // Window::getMousePosition（真实鼠标位置在无头测试中不可控）。
    // 注入坐标为窗口坐标系下的绝对坐标，与 getDrawRect 判定一致。
    bool debugMouseOverride = false;
    float debugMouseX = 0.0f;
    float debugMouseY = 0.0f;

    // ── 生命周期（全有或全无） ──
    bool initialize();
    void destroy();

    // ── 最近实例解析（浮层控件无 parent 归属时的兜底） ──
    static UIContext* getLastInstance() { return s_lastInstance; }
    static void setLastInstance(UIContext* ctx) { s_lastInstance = ctx; }

    // ── 实例活跃注册表（析构守卫） ──
    // 静态/全局残留控件（shared_ptr 逃逸控件树）在进程退出阶段析构时，
    // 其 m_context 可能已随 DestroyInstance 释放（悬垂）。析构路径中的
    // 上下文访问（如 unregisterControl）必须通过 isActive 确认实例仍存活。
    // 实现用堆分配容器（永不析构）：跨编译单元的静态析构顺序无保证，
    // 若容器先于残留控件析构，查询即访问已销毁对象。
    static void registerActive(UIContext* ctx);
    static void unregisterActive(UIContext* ctx);
    static bool isActive(UIContext* ctx);

private:
    static UIContext* s_lastInstance;
    static std::unordered_set<UIContext*>& activeContexts();
};

// 日志辅助：实例标签前缀 + 级别（I=Info / W=Warn / E=Error）
// Debug 构建：三个级别全部输出；
// Release 构建：INFO 编译为 no-op（发布版不刷信息日志），WARN/ERROR 保留（错误仍需可见）。
// UI_LOG 为 UI_LOGI 的兼容别名（历史调用不受影响）。
#ifdef _DEBUG
#define UI_LOGP(instance, fmt, ...) \
    do { \
        if ((instance) && !(instance)->debugLabel.empty()) { \
            printf("[%s] " fmt "\n", (instance)->debugLabel.c_str(), ##__VA_ARGS__); \
        } \
    } while (0)
#define UI_LOG(instance, fmt, ...)  UI_LOGP(instance, "[INFO] "  fmt, ##__VA_ARGS__)
#define UI_LOGI(instance, fmt, ...) UI_LOGP(instance, "[INFO] "  fmt, ##__VA_ARGS__)
#define UI_LOGW(instance, fmt, ...) UI_LOGP(instance, "[WARN] "  fmt, ##__VA_ARGS__)
#define UI_LOGE(instance, fmt, ...) UI_LOGP(instance, "[ERROR] " fmt, ##__VA_ARGS__)
#else
#define UI_LOG(instance, fmt, ...)  ((void)0)
#define UI_LOGI(instance, fmt, ...) ((void)0)
#define UI_LOGW(instance, fmt, ...) \
    do { \
        if ((instance) && !(instance)->debugLabel.empty()) { \
            printf("[%s] [WARN] " fmt "\n", (instance)->debugLabel.c_str(), ##__VA_ARGS__); \
        } \
    } while (0)
#define UI_LOGE(instance, fmt, ...) \
    do { \
        if ((instance) && !(instance)->debugLabel.empty()) { \
            printf("[%s] [ERROR] " fmt "\n", (instance)->debugLabel.c_str(), ##__VA_ARGS__); \
        } \
    } while (0)
#endif

#endif // UICONTEXT_H
