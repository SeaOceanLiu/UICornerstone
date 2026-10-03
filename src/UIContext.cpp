#include "UIContext.h"
#include "BackendPlugin.h"
#include "Bench.h"
#include "MainWindow.h"
#include "EventQueue.h"
#include "DataContext.h"
#include "FocusManager.h"
#include <atomic>
#include <cstdio>

// 全局自增实例 ID
static std::atomic<uint32_t> s_nextInstanceId{1};

// 最近实例：无 parent 归属的浮层控件（Popup/Dialog）open 时兜底解析
UIContext* UIContext::s_lastInstance = nullptr;

// 实例活跃注册表（析构守卫）：堆分配、永不析构（静态析构顺序无保证）
std::unordered_set<UIContext*>& UIContext::activeContexts() {
    static std::unordered_set<UIContext*>* s = new std::unordered_set<UIContext*>();
    return *s;
}

void UIContext::registerActive(UIContext* ctx) { activeContexts().insert(ctx); }
void UIContext::unregisterActive(UIContext* ctx) { activeContexts().erase(ctx); }
bool UIContext::isActive(UIContext* ctx) { return ctx && activeContexts().count(ctx) > 0; }

bool UIContext::initialize() {
    instanceId = s_nextInstanceId++;
    setLastInstance(this);
    registerActive(this);
    if (debugLabel.empty()) {
        debugLabel = "Instance_" + std::to_string(instanceId);
    }

    // 步骤 1: Backend（仅 owner 创建；子视口共享 owner 后端）
    if (ownsBackend) {
        backendManager = new BackendManager();
        // 窗口标题标记（仅 Debug）：追加实例标签，便于多窗口调试识别（§5.11.5）
#ifdef _DEBUG
        std::string titleStr;
        if (!windowTitle.empty()) {
            titleStr = windowTitle;
            if (!debugLabel.empty()) titleStr += " [" + debugLabel + "]";
        }
        const char* title = titleStr.empty() ? nullptr : titleStr.c_str();
#else
        const char* title = windowTitle.empty() ? nullptr : windowTitle.c_str();
#endif
        if (!backendManager->initialize(callbacks, title, windowWidth, windowHeight, windowFlags)) {
            delete backendManager;
            backendManager = nullptr;
            return false;
        }
        window = backendManager->window();
        renderDevice = backendManager->renderDevice();
        inputBackend = backendManager->inputBackend();
        textRenderer = backendManager->textRenderer();
    } else if (owner) {
        backendManager = owner->backendManager;
        window = owner->window;
        renderDevice = owner->renderDevice;
        inputBackend = owner->inputBackend;
        textRenderer = owner->textRenderer;
        resourceProvider = owner->resourceProvider;
        viewport = owner->viewport;  // 兜底；CreateViewport 已在创建时写入
    }

    // 步骤 2: EventQueue / DataContext（Bench 构造绑定 ctx->eventQueue，须先建）
    eventQueue = new EventQueue();
    dataContext = new DataContext();

    // 步骤 3: MainWindow（仅 owner 创建；子视口共享 owner 的 ResourceProvider）
    if (ownsBackend) {
        mainWindow = new MainWindow(this);
        if (!mainWindow->getResourceProvider()) {
            delete mainWindow;
            mainWindow = nullptr;
            goto rollback;
        }
        resourceProvider = mainWindow->getResourceProvider();
    }

    // 步骤 4: FocusManager + Bench（控件树根）
    focusManager = new FocusManager();
    bench = new Bench(this);
    bench->show();

    initialized = true;
    quit = false;
    return true;

rollback:
    delete dataContext;     dataContext = nullptr;
    delete eventQueue;      eventQueue = nullptr;
    if (backendManager) {
        backendManager->shutdown();
        delete backendManager;
        backendManager = nullptr;
    }
    window = nullptr;
    renderDevice = nullptr;
    inputBackend = nullptr;
    textRenderer = nullptr;
    resourceProvider = nullptr;
    return false;
}

void UIContext::destroy() {
    quit = true;
    popupPool.clear();
    menuPool.clear();
    detachedControls.clear();   // P0-50：摘除保活池随实例销毁级联释放
    controlsById.clear();
    actions.clear();
    while (true) {
        std::lock_guard<std::mutex> lock(queuedEventsMutex);
        if (queuedEvents.empty()) break;
        queuedEvents.pop();
    }

    delete dataContext;     dataContext = nullptr;
    delete eventQueue;      eventQueue = nullptr;
    delete bench;           bench = nullptr;
    delete mainWindow;      mainWindow = nullptr;
    delete focusManager;    focusManager = nullptr;

    if (ownsBackend && backendManager) {
        backendManager->shutdown();
        delete backendManager;
    }
    backendManager = nullptr;
    window = nullptr;
    renderDevice = nullptr;
    inputBackend = nullptr;
    textRenderer = nullptr;
    resourceProvider = nullptr;
    callbacks = nullptr;
    initialized = false;
    unregisterActive(this);
}
