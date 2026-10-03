// UICornerstone C++ Binding — 控件代理实现
// 许可证 MIT。仅调用 C ABI。
#include "Control.h"
#include "Event.h"
#include "Impl.h"
#include "DynamicApi.h"

#include <cstring>

namespace {
// 访问 Impl 中的回调 userData 注册表（ControlState::ownerImpl 指向 UICornerstone::Impl*）
UICornerstone::Impl* ownerOf(const ControlState& s) {
    return static_cast<UICornerstone::Impl*>(s.ownerImpl);
}
} // namespace

// ============================================================
// 构造 / 有效性
// ============================================================
Control::Control(std::shared_ptr<ControlState> state) : m_state(std::move(state)) {}

bool Control::IsValid() const {
    return m_state && m_state->alive && m_state->instance && m_state->handle;
}

// ============================================================
// 属性设置（全部经 C ABI 转发；悬挂/无效时静默忽略）
// ============================================================
void Control::SetColor(const char* prop, UIColor value) {
    if (IsValid()) UICornerstone::Dyn::API().fnSetColor(m_state->instance, m_state->handle, prop, value);
}
void Control::SetStateColor(const char* prop, UIStateColor value) {
    if (IsValid()) UICornerstone::Dyn::API().fnSetStateColor(m_state->instance, m_state->handle, prop, value);
}
void Control::SetBool(const char* prop, bool value) {
    if (IsValid()) UICornerstone::Dyn::API().fnSetBool(m_state->instance, m_state->handle, prop, value ? 1 : 0);
}
void Control::SetInt(const char* prop, int value) {
    if (IsValid()) UICornerstone::Dyn::API().fnSetInt(m_state->instance, m_state->handle, prop, value);
}
void Control::SetFloat(const char* prop, float value) {
    if (IsValid()) UICornerstone::Dyn::API().fnSetFloat(m_state->instance, m_state->handle, prop, value);
}
void Control::SetString(const char* prop, const std::string& value) {
    if (IsValid()) UICornerstone::Dyn::API().fnSetString(m_state->instance, m_state->handle, prop, value.c_str());
}
void Control::SetEnum(const char* prop, const std::string& value) {
    if (IsValid()) UICornerstone::Dyn::API().fnSetEnum(m_state->instance, m_state->handle, prop, value.c_str());
}
void Control::SetPtr(const char* prop, void* value) {
    if (IsValid()) UICornerstone::Dyn::API().fnSetPtr(m_state->instance, m_state->handle, prop, value);
}

// ============================================================
// 属性读取
// ============================================================
UIColor Control::GetColor(const char* prop) const {
    UIColor v{0, 0, 0, 255};
    if (IsValid()) UICornerstone::Dyn::API().fnGetColor(m_state->instance, m_state->handle, prop, &v);
    return v;
}
UIStateColor Control::GetStateColor(const char* prop) const {
    UIStateColor v{};
    if (IsValid()) UICornerstone::Dyn::API().fnGetStateColor(m_state->instance, m_state->handle, prop, &v);
    return v;
}
bool Control::GetBool(const char* prop) const {
    int v = 0;
    if (IsValid()) UICornerstone::Dyn::API().fnGetBool(m_state->instance, m_state->handle, prop, &v);
    return v != 0;
}
int Control::GetInt(const char* prop) const {
    int v = 0;
    if (IsValid()) UICornerstone::Dyn::API().fnGetInt(m_state->instance, m_state->handle, prop, &v);
    return v;
}
float Control::GetFloat(const char* prop) const {
    float v = 0.f;
    if (IsValid()) UICornerstone::Dyn::API().fnGetFloat(m_state->instance, m_state->handle, prop, &v);
    return v;
}
std::string Control::GetString(const char* prop) const {
    char buf[256];
    buf[0] = '\0';
    if (IsValid()) UICornerstone::Dyn::API().fnGetString(m_state->instance, m_state->handle, prop, buf, sizeof(buf));
    return std::string(buf);
}
std::string Control::GetEnum(const char* prop) const {
    char buf[64];
    buf[0] = '\0';
    if (IsValid()) UICornerstone::Dyn::API().fnGetEnum(m_state->instance, m_state->handle, prop, buf, sizeof(buf));
    return std::string(buf);
}
void* Control::GetPtr(const char* prop) const {
    void* v = nullptr;
    if (IsValid()) UICornerstone::Dyn::API().fnGetPtr(m_state->instance, m_state->handle, prop, &v);
    return v;
}

// ============================================================
// 回调桥接：C thunk → std::function
// ============================================================
void Control::CallbackThunk(UIControlHandle ctl, const UIEventData* event, void* userData) {
    auto* fn = static_cast<EventCallback*>(userData);
    Event ev(event);
    (*fn)(ev);
}

void Control::SetCallback(const char* event, EventCallback callback) {
    if (!IsValid()) return;

    auto slot = std::make_shared<EventCallback>(std::move(callback));

    // 注册到 Impl 级注册表（跨 Control 拷贝共享，与 C 端 userData 生命周期一致）
    auto* impl = ownerOf(*m_state);
    if (!impl) return;
    impl->callbackUserData[m_state->handle].push_back(slot);

    UICornerstone::Dyn::API().fnSetCallback(m_state->instance, m_state->handle, event,
                              &Control::CallbackThunk, slot.get());
}

// ============================================================
// 控件操作（一一对应 C ABI 入口）
// ============================================================
void Control::SetRect(float x, float y, float w, float h) {
    if (IsValid()) UICornerstone::Dyn::API().fnSetRect(m_state->instance, m_state->handle, x, y, w, h);
}
UIRect Control::GetRect() const {
    UIRect r{0, 0, 0, 0};
    if (IsValid()) UICornerstone::Dyn::API().fnGetRect(m_state->instance, m_state->handle, &r.x, &r.y, &r.w, &r.h);
    return r;
}
Control Control::AddChild(Control child) {
    if (IsValid() && child.IsValid())
        UICornerstone::Dyn::API().fnAddChildControl(m_state->instance, m_state->handle, child.Handle());
    return child;
}
bool Control::RemoveChild(Control child) {
    if (!IsValid() || !child.IsValid()) return false;
    if (!UICornerstone::Dyn::API().fnRemoveChild) return false;
    return UICornerstone::Dyn::API().fnRemoveChild(m_state->instance, m_state->handle, child.Handle()) != 0;
}
void Control::Destroy() {
    if (m_state && m_state->alive && m_state->instance && m_state->handle) {
        UICornerstone::Dyn::API().fnDestroyControl(m_state->instance, m_state->handle);
        m_state->alive = false;   // 悬挂检测：后续调用静默忽略
    }
}
std::string Control::GetId() const {
    if (!IsValid()) return std::string();
    const char* id = UICornerstone::Dyn::API().fnGetControlId(m_state->instance, m_state->handle);
    return id ? std::string(id) : std::string();
}

std::string Control::GetType() const {
    if (!IsValid()) return std::string();
    char buf[64] = {0};
    if (!UICornerstone::Dyn::API().fnGetControlType(m_state->instance, m_state->handle, buf, (int)sizeof(buf)))
        return std::string();
    return std::string(buf);
}