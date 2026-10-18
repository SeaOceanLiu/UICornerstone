// UICornerstone C++ Binding — 内部 Impl 结构（不对外，仅 .cpp 可见）
// 注意：类 UICornerstone 与命名空间同名，MSVC 不支持其嵌套类外定义
// （struct UICornerstone::Impl），故 Impl 提升为命名空间级类型。
#ifndef UICORNERSTONE_BINDING_IMPL_H
#define UICORNERSTONE_BINDING_IMPL_H

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include "UICornerstoneAPI.h"
#include "UICornerstone.h"   // 完整类定义（Impl 引用其 Config / ActionCallback）

struct ControlState;

namespace UICornerstone {

// P0-98①：动作回调载体（捕获注册时实例——动作处理器内 Control.GetId/GetString 可用）
struct ActionThunk {
    UICornerstone::ActionCallback cb;
    UIInstance inst = nullptr;
};

// 内部实现（pimpl）。类声明见 UICornerstone.h，此定义仅 .cpp 可见。
struct Impl {
    UICornerstone::Config config;
    UIInstance instance = nullptr;      // C ABI 实例句柄（唯一真源）
    bool ownsInstance = false;          // 由 Binding 创建（需析构销毁）
    bool initialized = false;
    std::string lastError;

    // 资源根（Binding 侧路径解析）
    std::string resourceRoot;

    // Memory ResourceProvider（懒创建；析构时经 destroyResourceProvider 释放）
    const UIBackendCallbacks* callbacks = nullptr;
    UIResourceProviderHandle memoryProvider = nullptr;

    // 后端插件 DLL 句柄（纯动态加载模式；实例析构时 FreeLibrary）
    // 核心 DLL 由 DynamicApi 进程级持有（不随实例卸载）
    void* dllHandle = nullptr;

    // Action 注册表（实例私有，无全局 static；P0-98①：ActionThunk 携带注册时实例）
    std::unordered_map<std::string, std::shared_ptr<ActionThunk>> actions;

    // 窗口 resize 回调（std::function + C thunk 的 userData/存活域）
    std::shared_ptr<UICornerstone::WindowResizeCallback> windowResize;  // nullptr = 未设置

    // P0-100：OS 文件拖放回调（std::function + C thunk 的 userData/存活域）
    std::shared_ptr<UICornerstone::DropFileCallback> dropFileCb;        // nullptr = 未设置

    // Control 生命周期追踪（weak 不保活）
    std::unordered_map<UIControlHandle, std::weak_ptr<ControlState>> liveControls;

    // 回调 userData 注册表（Impl 级持有，与 Control 对象生命周期解耦）
    std::unordered_map<UIControlHandle, std::vector<std::shared_ptr<void>>> callbackUserData;
};

} // namespace UICornerstone

#endif