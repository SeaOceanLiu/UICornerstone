// UICornerstone C++ Binding — 控件代理类
// 许可证 MIT。仅依赖 C ABI 头。
#ifndef UICORNERSTONE_BINDING_CONTROL_H
#define UICORNERSTONE_BINDING_CONTROL_H

#include <string>
#include <memory>
#include <functional>
#include "UICornerstoneAPI.h"

class Event;

// 共享状态（§5.7.1）：句柄有效性追踪。Control 拷贝共享同一状态。
struct ControlState {
    UIInstance instance = nullptr;
    UIControlHandle handle = nullptr;
    void* ownerImpl = nullptr;      // UICornerstone::Impl*（回调 userData 注册表）
    bool alive = true;
};

class Control {
public:
    Control() = default;
    explicit Control(std::shared_ptr<ControlState> state);

    bool IsValid() const;
    UIControlHandle Handle() const { return m_state ? m_state->handle : nullptr; }

    // ── 属性设置（一一对应 C ABI：UICornerstone_Set*）──
    void SetColor(const char* prop, UIColor value);
    void SetStateColor(const char* prop, UIStateColor value);
    void SetBool(const char* prop, bool value);
    void SetInt(const char* prop, int value);
    void SetFloat(const char* prop, float value);
    void SetString(const char* prop, const std::string& value);
    void SetEnum(const char* prop, const std::string& value);
    void SetPtr(const char* prop, void* value);

    // ── 属性读取（一一对应 C ABI：UICornerstone_Get*）──
    UIColor      GetColor(const char* prop) const;
    UIStateColor GetStateColor(const char* prop) const;
    bool         GetBool(const char* prop) const;
    int          GetInt(const char* prop) const;
    float        GetFloat(const char* prop) const;
    std::string  GetString(const char* prop) const;
    std::string  GetEnum(const char* prop) const;
    void*        GetPtr(const char* prop) const;

    // ── 回调（类型安全 lambda 桥接）──
    using EventCallback = std::function<void(const Event&)>;
    void SetCallback(const char* event, EventCallback callback);

    // ── 控件操作（一一对应 C ABI：UICornerstone_*Control / SetRect 等）──
    void SetRect(float x, float y, float w, float h);
    UIRect GetRect() const;
    // P0-50：挂载子控件（目标 = this，须为 Panel；WinFrame 请用其 client-panel 代理）。
    // 返回 child；任意旧父自动摘除；非 Panel/自身/祖先环静默无操作。
    Control AddChild(Control child);
    // P0-50：从本控件摘除 child（不销毁，保活可再挂）；要求 child 当前父 == this；返回是否成功。
    bool RemoveChild(Control child);
    void Destroy();
    std::string GetId() const;

    // 控件运行时类型（"check-box" / "image" / "tree-view" ...，与 JSON "type" 值一致）
    std::string GetType() const;

private:
    std::shared_ptr<ControlState> m_state;

    // C 回调 thunk（静态，经 userData 指向 shared_ptr<EventCallback>）
    static void CallbackThunk(UIControlHandle ctl, const UIEventData* event, void* userData);
};

#endif