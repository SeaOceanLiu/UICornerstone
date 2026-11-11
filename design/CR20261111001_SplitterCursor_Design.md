# CR20261111001 — Splitter 移出后仍保持双向缩放光标（分析 + 设计）

> 状态：**已关闭（2026-11-11；设计器联调通过）**
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261111001）
> 关联：Splitter hover/cursor、ControlImpl::update、Cursor owner 仲裁、拖拽 watcher

## 1. 问题与根因

设计器主布局 Splitter：鼠标从左侧进入后光标变为左右双向箭头，继续向右移入右侧面板，光标仍保留双向箭头；预期离开 Splitter 命中区后恢复默认。

`Splitter` 当前只有：

- `handleEvent(MouseMove)`：仅在 Splitter 命中时 `updateCursor(true)`；
- 鼠标移出后，父 Panel 不再把 MouseMove 派发给 Splitter；
- `ControlImpl::update()` 的 hover 检测会调用 `onMouseLeave()`，但 Splitter 未覆写，因此没有恢复默认；
- `endDrag()` 未按释放位置更新/恢复光标，拖到外部松开也会残留。

这是 cursor ownership 生命周期问题，不是绘制/命中问题。

## 2. 目标与范围

- 鼠标离开 Splitter 命中区后恢复默认箭头；再次进入重新显示 resize 光标；
- 拖拽到 Splitter 外部释放后，按最终鼠标位置恢复/保持正确光标；
- 不影响拖拽中的 resize 光标（拖拽期间即使指针暂时离开，仍保持 resize，直到释放）；
- 使用现有 `Cursor::setCurrentWithOwner/restoreDefault` owner 仲裁，不覆盖其它控件的光标；
- 不新增 C ABI/Binding/JSON/schema API；
- 不改变 Splitter 布局、比例、双击/键盘语义。

## 3. 方案设计

### 3.1 覆写 hover 入口

`Splitter` 增加：

```cpp
void onMouseEnter(float x, float y) override;
void onMouseLeave(float x, float y) override;
```

实现：

```cpp
void Splitter::onMouseEnter(float x, float y) {
    ControlImpl::onMouseEnter(x, y);
    if (m_enable && m_visible && !m_dragging) {
        m_hovered = true;
        updateCursor(true);
    }
}

void Splitter::onMouseLeave(float x, float y) {
    ControlImpl::onMouseLeave(x, y);
    m_hovered = false;
    if (!m_dragging) {
        Cursor::restoreDefault(getThis().get());   // owner 仲裁
    }
}
```

- 进入即可显示 resize，不依赖首个 MouseMove；
- 离开时只有默认光标属于本 Splitter（`restoreDefault` 内部检查 owner）才恢复；
- 拖拽期间不恢复，由 watcher/endDrag 管理。

### 3.2 endDrag 按最终位置收尾

将 `endDrag()` 改为携带释放坐标：

```cpp
void endDrag(float x, float y);
```

调用点：

- `handleEvent` 的 MouseUp → `endDrag(event->mouseButton.x, ...)`；
- `beforeEventHandlingWatcher` 的 MouseUp / 拖拽中 MouseDown → 使用事件坐标。

结束逻辑：

```cpp
m_dragging = false;
// ...既有 linked/engine 比例收尾...
const bool inside = isContainsPoint(x, y);
m_hovered = inside;
updateCursor(inside);   // inside → resize；outside → restoreDefault(owner)
```

- 拖到外部释放 → 默认箭头；在 Splitter 上释放 → 保持 resize；
- engine-managed / linked 两种早退路径也必须执行光标收尾（把收尾提到早退之前或统一 helper）；
- 不改变 watcher 的消费语义（MouseUp 仍返回 true，避免穿透）。

### 3.3 owner 仲裁

- `Cursor::setCurrentWithOwner(resize, this)` 记录 owner；
- 只有 owner 匹配时 `restoreDefault` 才真正恢复；相邻 Splitter/自定义光标控件不会互相覆盖；
- `onMouseLeave` 不直接 `Cursor::setCurrent(default)`，必须走 `restoreDefault(getThis().get())`。

## 4. 边界与风险

| 场景 | 处理 |
|---|---|
| 拖拽期间指针离开 Splitter | 保持 resize；不因 onMouseLeave 恢复 |
| 外部释放 | endDrag 按最终坐标恢复默认 |
| 在 Splitter 上释放 | 保持 resize |
| 相邻 Splitter | owner 仲裁；只有当前 owner 恢复 |
| 自定义光标控件 | 不覆盖其 owner；本控件只恢复自己 |
| disabled/hidden | 不进入 hover 分支；若禁用时仍持有光标，后续显式变更/离开路径负责；本 CR 不扩展禁用路径 |
| 多实例/子视口 | owner 指针按实例控件唯一；沿用现有 Cursor 全局 owner |

## 5. 测试/验收

### 5.1 自动化/探针

1. 模拟 MouseMove 进入 Splitter → `Cursor` 为 resize、owner=Splitter；
2. MouseMove 移到右侧面板 → `restoreDefault` 后 owner 不再为 Splitter，光标为默认；
3. 再次进入 → resize 恢复；
4. 按住左键拖拽，移出后 MouseUp → 默认光标；
5. 在 Splitter 上 MouseUp → 保持 resize；
6. 两个 Splitter 相邻切换 → 只有当前 owner 设置/恢复；
7. 与 Label/Button 自定义光标切换不互相覆盖；
8. 普通 Splitter 布局/比例/双击/键盘测试不回归。

测试建议：新增 `test_cr_splitter_cursor`（或扩展 `test_splitter`），用 `Cursor::getCurrentOwner()` 断言 owner，不依赖真实屏幕截图；手工补设计器主布局视觉回归。

### 5.2 手工

- splitter1/splitter2 从左/右进入、穿过、移出；
- 拖拽到外部释放；
- 快速来回移动/多显示器/缩放窗口；
- 与设计器其它自定义光标控件相邻切换。

## 6. 文档/交付

- 无新增公共 API/JSON/schema；用户手册无需新增章节；
- 可在 Splitter 注释说明 hover/cursor owner 生命周期；
- 回归 `test_splitter`、`auto=2`、`auto=3`、`strict`，Release 同步。

## 7. 实施顺序

1. Splitter onMouseEnter/onMouseLeave；
2. endDrag 坐标参数与所有调用点收尾；
3. cursor owner 测试；
4. 全量回归、Release 同步、设计器联调。

## 修订记录

- 2026-11-11 初稿：定位 onMouseLeave 缺失与 endDrag 未收尾；采用 hover 覆写 + endDrag 最终坐标恢复；保持 owner 仲裁与拖拽语义；待评审。

## 8. 实施记录（2026-11-11）

- Splitter 覆写 `onMouseEnter/onMouseLeave`；离开/禁用/隐藏恢复 owner 光标，拖拽期间不提前恢复。
- `endDrag(x,y)` 按最终坐标恢复/保持光标，engine/linked 早退路径先收尾。
- 新增 `test_cr_splitter_cursor`：覆盖 `ControlImpl::update()` hover 路径、禁用/隐藏清理、外部拖拽释放；全量回归 **61 OK + 4 项预存失败**。
- Release 已同步，DLL SHA256 `94bf92b9de3aa0b6fcdbea85ec202e0311e62566bcf3d6432c1beb3482419421`。
- 联调（2026-11-11）：94bf92b9 版本实测通过；auto=2 297/0、auto=3 REPORT-OK、strict PASS；CR 设计器/引擎均 closed。
