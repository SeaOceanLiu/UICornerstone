# Dialog 通用弹窗控件设计文档

## 1. 概述

通用弹窗控件，三层层级结构：**Popup（无按钮）→ ConfirmPopup（有确定按钮）→ Dialog（确定+取消）**。每一层在前一层基础上添加语义和控件，调用方只依赖所需层级。

### 1.1 设计目标

- 从 ColorPicker 弹窗中提取通用逻辑，消除重复代码
- 三层层级，上层依赖下层，下层不感知上层
- 统一的 `show()` / `close()` 生命周期管理
- 自定义内容区域（任意 Control 树）
- 按钮文字、位置、大小完全可定制
- 通过回调机制将结果返回给调用方，兼顾 `DialogResult` 枚举查询

---

## 2. 层级设计

### 2.1 层级关系

```
Popup (基类，无按钮)
  ↑ 继承
ConfirmPopup (添加确定按钮)
  ↑ 继承
Dialog (添加取消按钮)
```

### 2.2 各层级功能矩阵

| 功能 | Popup | ConfirmPopup | Dialog |
|------|-------|-------------|--------|
| show()/close() 生命周期 | ✔ | ✔ | ✔ |
| 添加到 BENCH 顶层 | ✔ | ✔ | ✔ |
| 自定义内容区域 | ✔ | ✔ | ✔ |
| ESC 行为 | close() | close() | onCancelAction()→close() |
| 外部点击行为 | close() | close() | onCancelAction()→close() |
| Focus Boundary | ✔ | ✔ | ✔ |
| 定位（居中/锚定/绝对） | ✔ | ✔ | ✔ |
| 确定按钮 | ❌ | ✔ | ✔ |
| 取消按钮 | ❌ | ❌ | ✔ |
| Enter → 确定 | ❌ | ✔ | ✔ |
| 按钮文字/位置/大小定制 | ❌ | ✔ | ✔ |
| 结果回调 | ✔ onClose | ✔ onConfirm | ✔ onConfirm + onCancel |
| DialogResult 查询 | ✔ | ✔ | ✔ |

---

## 3. 类设计

### 3.1 返回结果

```cpp
enum class DialogResult {
    None,        // 弹窗尚未关闭
    Confirmed,   // 用户确认
    Cancelled    // 用户取消
};
```

### 3.2 Popup（基类）

```cpp
class Popup : public Panel {
public:
    using OnCloseHandler = std::function<void(shared_ptr<Popup>, DialogResult)>;

private:
    // ── 内容 ──
    shared_ptr<ControlImpl> m_content;

    // ── 定位 ──
    enum class AnchorMode { Absolute, Centered, Anchored };
    AnchorMode m_anchorMode = AnchorMode::Centered;
    Control* m_anchorControl = nullptr;
    SRect m_anchorOffset;

    // ── 行为 ──
    bool m_closeOnClickOutside = true;
    bool m_closeOnEsc = true;
    bool m_watcherRegistered = false;
    DialogResult m_result = DialogResult::None;

    // ── 回调 ──
    OnCloseHandler m_onClose;

    // ── 内部 ──
    void registerWatcher();
    SRect computeTargetRect();

protected:
    // 子类可重写这些方法改变 ESC/外部点击行为
    virtual void onEscPressed();
    virtual void onOutsideClicked();

public:
    Popup(Control* parent, SRect rect,
          float xScale = 1.0f, float yScale = 1.0f);
    ~Popup();
    void create() override;

    // ── 生命周期 ──
    virtual void show();
    virtual void close(DialogResult result = DialogResult::Cancelled);
    bool isPopupVisible() const { return getVisible() && m_result != DialogResult::None; }
    DialogResult getResult() const { return m_result; }

    // ── 内容 ──
    void setContent(shared_ptr<ControlImpl> content);
    shared_ptr<ControlImpl> getContent() const { return m_content; }

    // ── 定位 ──
    void setCentered();
    void setAnchored(Control* anchor, const SRect& offset = {0, 0, 0, 0});
    void setAbsolute(const SRect& rect);

    // ── 行为 ──
    void setCloseOnClickOutside(bool v);
    void setCloseOnEsc(bool v);

    // ── 回调 ──
    void setOnClose(OnCloseHandler handler);

    // ── 事件 ──
    bool handleEvent(shared_ptr<Event> event) override;
    bool beforeEventHandlingWatcher(shared_ptr<Event> event);
};
```

### 3.3 ConfirmPopup（确定按钮）

```cpp
class ConfirmPopup : public Popup {
public:
    using OnConfirmHandler = std::function<void(shared_ptr<ConfirmPopup>)>;

private:
    // ── 确定按钮 ──
    shared_ptr<Button> m_btnConfirm;
    string m_btnConfirmText = u8"确定";
    SRect m_btnConfirmRect;               // 未缩放，空=使用默认布局
    bool m_showConfirmButton = true;

    // ── 回调 ──
    OnConfirmHandler m_onConfirm;

protected:
    void createConfirmButton();
    void layoutContent();
    void onEscPressed() override;          // ESC→close而非cancel（无取消语义）
    virtual void onConfirmAction();

public:
    ConfirmPopup(Control* parent, SRect rect,
                 float xScale = 1.0f, float yScale = 1.0f);
    void create() override;
    void show() override;

    // ── 按钮定制 ──
    void setConfirmButtonVisible(bool v);
    void setConfirmButtonText(const string& text);
    // 设置按钮在弹窗客户区内的未缩放矩形（相对弹窗左上角的偏移）
    // 调用 setConfirmButtonRect({w-90, h-36, 80, 28}) 将按钮放在右下
    void setConfirmButtonRect(SRect rect);
    // 获取按钮指针以便更精细的定制（颜色、字体等）
    shared_ptr<Button> getConfirmButton() { return m_btnConfirm; }

    // ── 回调 ──
    void setOnConfirm(OnConfirmHandler handler);

    bool handleEvent(shared_ptr<Event> event) override;
};
```

### 3.4 Dialog（确定+取消）

```cpp
class Dialog : public ConfirmPopup {
public:
    using OnCancelHandler = std::function<void(shared_ptr<Dialog>)>;

private:
    // ── 取消按钮 ──
    shared_ptr<Button> m_btnCancel;
    string m_btnCancelText = u8"取消";
    SRect m_btnCancelRect;                // 未缩放，空=使用默认布局

    // ── 回调 ──
    OnCancelHandler m_onCancel;

protected:
    void createCancelButton();
    void layoutContent();                  // 覆盖 ConfirmPopup 布局
    void onEscPressed() override;          // ESC→触发取消回调
    virtual void onCancelAction();

public:
    Dialog(Control* parent, SRect rect,
           float xScale = 1.0f, float yScale = 1.0f);
    void create() override;
    void show() override;

    // ── 按钮定制 ──
    void setCancelButtonText(const string& text);
    void setCancelButtonRect(SRect rect);
    shared_ptr<Button> getCancelButton() { return m_btnCancel; }

    // ── 回调 ──
    void setOnCancel(OnCancelHandler handler);

    bool handleEvent(shared_ptr<Event> event) override;
};
```

---

## 4. 生命周期

### 4.1 Popup 构造

```
Popup 构造
├── Panel(parent, rect, scale)
├── setTransparent(true)        ← 背景由内容控件负责
├── setBorderVisible(true)
├── setNormalStateBGColor(bg)
└── setVisible(false)           ← 初始隐藏
```

### 4.2 show()

```
show()  [virtual]
├── computeTargetRect()           ← 计算屏幕绝对坐标
├── setRect(rect)
├── if (!m_watcherRegistered)
│   ├── addBeforeEventHandlingWatcher(MouseDown, this)
│   ├── addBeforeEventHandlingWatcher(KeyDown, this)
│   └── m_watcherRegistered = true
├── BENCH->addControl(this)       ← 添加到 BENCH 顶层
├── setVisible(true)
├── GET_FOCUSMANAGER->registerBoundary(this)
├── m_result = DialogResult::None
├── focusFirstContent()
└── m_onClose 回调不触发（弹窗未关闭）
```

### 4.3 close(result)

```
close(result)  [virtual]
├── m_result = result
├── setVisible(false)
├── BENCH->removeControl(this)
├── GET_FOCUSMANAGER->unregisterBoundary(this)
├── 焦点回到父控件
└── 触发 m_onClose(this, result)
```

> 不主动 `removeBeforeEventHandlingWatcher`：watcher 可能在同一个 EventQueue 时序中被调用，移除会导致 mutex 死锁。watcher 内部检查 `!getVisible()` 直接 return。

### 4.4 层级创建时序

```
ConfirmPopup::create()
├── Popup::create()
├── if (m_showConfirmButton) createConfirmButton()
└── layoutContent()

Dialog::create()
├── ConfirmPopup::create()
├── if (m_showConfirmButton) createCancelButton()
└── layoutContent()              ← 覆盖 ConfirmPopup 的布局
```

```
ConfirmPopup::show()
├── Popup::show()                 ← 计算位置 + 添加到 BENCH
└── layoutContent()               ← 确保按钮位置正确

Dialog::show()
├── Popup::show()
└── layoutContent()
```

---

## 5. 交互逻辑

### 5.1 Popup

```
handleEvent(event)
├── MouseWheel
│   ├── 点在弹窗内部 → 转发到子控件（从后向前遍历）
│   └── 点在弹窗外部 → return false（不落到 Panel::handleEvent，防止从弹窗外滚动）
├── 其他 → Panel::handleEvent(event)

beforeEventHandlingWatcher(event)
├── !getVisible() → return false
├── KeyDown + Escape + m_closeOnEsc → onEscPressed()
├── MouseDown + m_closeOnClickOutside
│   ├── 点在弹窗内部 → return false
│   ├── 点在锚定控件内部 → return false
│   └── onOutsideClicked()
└── return false

onEscPressed()      [virtual]     ← Popup 实现
└── close(DialogResult::Cancelled)

onOutsideClicked()  [virtual]     ← Popup 实现
└── close(DialogResult::Cancelled)
```

### 5.2 ConfirmPopup

```
onEscPressed()      [override]    ← ConfirmPopup 实现
└── close(DialogResult::Cancelled)   ← 无取消语义，等同关闭

onConfirmAction()   [virtual]
├── 触发 m_onConfirm(this)
└── close(DialogResult::Confirmed)

handleEvent(event)
├── m_ignoreKeyEvent 检查
├── KeyDown + Enter + showConfirmButton
│   ├── m_ignoreKeyEvent = true
│   └── onConfirmAction()
├── 其他 → Popup::handleEvent(event)
```

### 5.3 Dialog

```
onEscPressed()      [override]    ← Dialog 实现
└── onCancelAction()

onCancelAction()    [virtual]
├── 触发 m_onCancel(this)
└── close(DialogResult::Cancelled)

handleEvent(event)
├── 继承 ConfirmPopup::handleEvent ← Enter 已在 ConfirmPopup 中处理
└── 其他 → ConfirmPopup::handleEvent(event)
```

---

## 6. 结果返回机制

### 6.1 回调方式（主要）

```cpp
// 调用方创建弹窗并设置回调
auto popup = make_shared<Dialog>(nullptr, SRect(0, 0, 300, 200));
popup->setOnConfirm([](shared_ptr<ConfirmPopup> sender) {
    // 读取自定义内容控件的数据
    auto colorPicker = dynamic_pointer_cast<ColorPicker>(sender->getContent());
    if (colorPicker) {
        SColor chosen = colorPicker->getColor();
        // ... 处理选中的颜色
    }
});
popup->setOnCancel([](shared_ptr<Dialog> sender) {
    // 取消时恢复或什么都不做
});
popup->show();
```

### 6.2 主动查询方式（辅助）

```cpp
popup->show();
// 在后续帧循环中轮询
if (popup->getResult() == DialogResult::Confirmed) {
    // 读取内容
}
```

### 6.3 数据传递建议

调用方可通过 `getContent()` 获取弹窗内的内容控件，再 `dynamic_cast` 到具体类型读写数据。这不是 Dialog 的责任——控件树本身就是数据容器。

---

## 7. 按钮定制

### 7.1 默认布局

```
┌─────────────────────────────────┐
│  ┌─────────────────────────┐    │
│  │     内容区域             │    │
│  │                         │    │
│  └─────────────────────────┘    │
│         ┌──────┐ ┌──────┐      │
│         │确定  │ │取消  │      │
│         └──────┘ └──────┘      │
└─────────────────────────────────┘
```

- 弹窗宽度 `rect.w`，高度 `rect.h`
- 内容区域：`(padding, padding)` 到 `(w - padding, h - padding - btnArea)`
- 按钮区域高度：`btnH + padding`
- 按钮位置：确定在右下角左侧，取消在确定右侧，间距 `btnGap`

### 7.2 自定义位置

调用方可完全覆盖默认布局：

```cpp
// 将确定按钮放在右上角
dlg->setConfirmButtonRect({w - 90, 10, 80, 28});
// 将取消按钮放在左上角
dlg->setCancelButtonRect({10, 10, 80, 28});
```

也可以通过 `getConfirmButton()` 获取按钮指针进行更细粒度的定制：

```cpp
auto btn = dlg->getConfirmButton();
btn->setNormalStateBGColor(SColor(0, 120, 200, 255));
btn->setFontSize(14);
```

### 7.3 按钮可见性

```cpp
dlg->setConfirmButtonVisible(false);  // 仅在特定条件下显示按钮
```

---

## 8. 样式

| 属性 | 默认值 | 说明 |
|------|--------|------|
| 背景色 | `SColor(48, 48, 48, 255)` | #303030 |
| 边框色 | `SColor(100, 100, 100, 255)` | 1px |
| 内边距 | 10px | 内容区域距离边框 |
| 确定按钮文字 | "确定" | ConfirmPopup |
| 取消按钮文字 | "取消" | Dialog |
| 按钮默认位置 | 右下 | 确定在左，取消在右 |
| 按钮默认尺寸 | 80×28 | 可通过 setRect 覆盖 |

---

## 9. 使用示例

### 9.1 Popup（无按钮，纯信息展示）

```cpp
auto info = make_shared<Popup>(nullptr, SRect(0, 0, 250, 120));
info->setCentered();
auto label = LabelBuilder(nullptr, SRect(10, 10, 230, 100))
    .setCaption(u8"加载完成！\n文件数量: 42")
    .setAlignmentMode(AM_MID_CENTER)
    .build();
info->setContent(label);
info->setOnClose([](shared_ptr<Popup> p, DialogResult r) {
    // 用户按 ESC 或点击外部
});
info->show();
```

### 9.2 ConfirmPopup（仅确定按钮）

```cpp
auto confirm = make_shared<ConfirmPopup>(nullptr, SRect(0, 0, 300, 150));
confirm->setCentered();
confirm->setContent(make_shared<SomeSettingsPanel>());
confirm->setConfirmButtonText(u8"应用");
confirm->setOnConfirm([](shared_ptr<ConfirmPopup> sender) {
    auto panel = dynamic_pointer_cast<SomeSettingsPanel>(sender->getContent());
    panel->apply();
});
confirm->show();
```

### 9.3 Dialog（确定+取消）

```cpp
auto dlg = make_shared<Dialog>(nullptr, SRect(0, 0, 300, 200));
dlg->setCentered();
dlg->setContent(myColorPicker);
dlg->setConfirmButtonText(u8"选择");
dlg->setCancelButtonText(u8"放弃");
dlg->setOnConfirm([](shared_ptr<ConfirmPopup> sender) {
    auto cp = dynamic_pointer_cast<ColorPicker>(sender->getContent());
    chosenColor = cp->getColor();
});
dlg->show();
```

---

## 10. JSON 布局

### 10.1 JSON 结构

Dialog/ConfirmPopup/Popup 支持在 JSON 布局中声明：定义弹窗的结构和内容，默认隐藏。通过事件系统或 `UICornerstone_FindControl` 找到后调用 `show()`。

```json
{
  "type": "Dialog",
  "id": "settingsDialog",
  "rect": { "w": 400, "h": 300 },
  "centered": true,
  "closeOnEsc": true,
  "closeOnClickOutside": false,
  "confirmButton": {
    "text": "确定",
    "rect": { "x": 230, "y": 260, "w": 80, "h": 28 },
    "visible": true
  },
  "cancelButton": {
    "text": "取消",
    "rect": { "x": 320, "y": 260, "w": 80, "h": 28 },
    "visible": true
  },
  "children": [
    {
      "type": "Label",
      "id": "title",
      "rect": { "x": 10, "y": 10, "w": 380, "h": 24 },
      "caption": "设置"
    },
    {
      "type": "CheckBox",
      "id": "autoSave",
      "rect": { "x": 20, "y": 44, "w": 200, "h": 24 },
      "caption": "自动保存"
    }
  ]
}
```

### 10.2 type 支持

| JSON type | C++ 类 | 说明 |
|-----------|--------|------|
| `"Popup"` | `Popup` | 无按钮，ESC/外部点击关闭 |
| `"ConfirmPopup"` | `ConfirmPopup` | 有确定按钮，Enter 触发 |
| `"Dialog"` | `Dialog` | 确定+取消，ESC→取消回调 |

### 10.3 通用属性

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `centered` | boolean | `true` | 是否屏幕居中；`false` 则使用 `rect` 的绝对坐标 |
| `closeOnEsc` | boolean | `true` | ESC 是否关闭弹窗 |
| `closeOnClickOutside` | boolean | `true` | 点击外部是否关闭弹窗 |
| `children` | array | `[]` | 弹窗内容控件，被包装为 Panel 后通过 `setContent()` 设置 |

### 10.4 confirmButton 属性

ConfirmPopup 和 Dialog 支持：

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `text` | string | `"确定"` | 按钮文字 |
| `rect` | object | 右下角 | `{x, y, w, h}` 未缩放矩形，不指定则使用默认右下位置 |
| `visible` | boolean | `true` | 是否显示确定按钮 |

### 10.5 cancelButton 属性

仅 Dialog 支持：

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `text` | string | `"取消"` | 按钮文字 |
| `rect` | object | 右下角 | `{x, y, w, h}` 未缩放矩形，不指定则使用默认右下位置 |
| `visible` | boolean | `true` | 是否显示取消按钮 |

### 10.6 事件与回调绑定

JSON 中 Dialog 的 `events` 对象注册到 `m_handlers`，由 C ABI 或 LayoutParser 的 `registerHandler` 机制回调：

```json
{
  "type": "Dialog",
  "id": "confirmDlg",
  "events": {
    "onConfirm": "handleConfirm",
    "onCancel": "handleCancel"
  }
}
```

在 C++ 端：

```cpp
parser.registerHandler("handleConfirm", [](shared_ptr<Control> ctl) {
    auto dlg = dynamic_pointer_cast<Dialog>(ctl);
    // 处理确认
});
```

### 10.7 使用模式：JSON 定义 + 代码 show()

典型的使用方式是 JSON 定义弹窗结构，按钮点击事件触发 `show()`：

```cpp
// 在 onInit 中加载布局
parser.parseLayout(jsonContent);
auto dlg = parser.findControlById("settingsDialog");

// 按钮的 onClick 事件中 show 弹窗
auto btn = parser.findControlById("openBtn");
btn->setOnClick([dlg](shared_ptr<Button>) {
    dlg->show();
});
```

或者通过 JSON 内的 `events.onClick` 配合 LayoutParser 的 handler 机制触发：

```json
{
  "type": "Button",
  "id": "openBtn",
  "caption": "打开设置",
  "events": { "onClick": "showSettings" }
}
```

```cpp
parser.registerHandler("showSettings", [dlg](shared_ptr<Control>) {
    dlg->show();
});
```

### 10.8 C ABI 风格

```c
UIControlHandle dlg = UICornerstone_CreateDialog(NULL, 0, 0, 400, 300);
UICornerstone_SetDialogCentered(dlg, 1);
UICornerstone_SetContent(dlg, somePanel);
UICornerstone_SetConfirmButtonText(dlg, "确定");
UICornerstone_SetOnConfirm(dlg, onConfirmCallback, userData);
UICornerstone_Show(dlg);
```

---

## 11. 边界与约束

| 约束 | 说明 |
|------|------|
| 一次只弹一个弹窗 | 嵌套弹窗需手动关闭前一个（不禁止，但需自行管理） |
| 非阻塞 | 不提供 `exec()` — UI 框架是事件驱动的 |
| 弹窗不随父控件移动 | `show()` 时锚定到 BENCH 顶层坐标 |
| 弹窗尺寸固定 | `show()` 时确定的尺寸不变；框架不会自动裁剪超出内容，调用方应确保内容控件尺寸不超过弹窗客户区 |
| 按钮 rect 为未缩放值 | 同控件系统的坐标规则一致 |

---

## 12. 迁移计划

| 步骤 | 内容 | 影响 |
|------|------|------|
| 1 | 实现 Popup：构造 + show/close + watcher + 定位 | `include/Dialog.h` `src/Dialog.cpp` ~150 行 |
| 2 | 实现 ConfirmPopup：确定按钮 + Enter + onConfirm | 同上 ~80 行 |
| 3 | 实现 Dialog：取消按钮 + ESC→onCancel + DialogResult | 同上 ~60 行 |
| 4 | ColorPicker 迁移：m_popup 替换为 Dialog | `src/ColorPicker.cpp` 约-80 行净减少 |
| 5 | 测试 | ~200 行 |

---

## 13. 测试计划

| # | 测试项 | 验证层级 |
|---|--------|----------|
| 1 | Popup show/close 生命周期 | Popup |
| 2 | Popup ESC 关闭，外部点击关闭 | Popup |
| 3 | Popup 屏幕居中/锚定/绝对定位 | Popup |
| 4 | ConfirmPopup 确定按钮触发回调 | ConfirmPopup |
| 5 | ConfirmPopup Enter 触发确定 | ConfirmPopup |
| 6 | ConfirmPopup 按钮文字/位置/可见性定制 | ConfirmPopup |
| 7 | Dialog 确定+取消按钮 | Dialog |
| 8 | Dialog ESC→取消回调，Enter→确定回调 | Dialog |
| 9 | Dialog 外部点击→取消回调 | Dialog |
| 10 | DialogResult 状态查询 | Dialog |
| 11 | FocusBoundary Tab 不越界 | Popup |
| 12 | 2x 缩放 | Popup |
| 13 | `test_dialog_cabi` 三后端 C ABI Dialog 集成测试（LoadLibrary + JSON dialogs + 共享头文件模式） | 集成（SDL3/SFML/Raylib） |

---

## 14. 跨后端注意事项

### 14.1 `windows.h` 冲突

Raylib 后端因 `CloseWindow()`/`DrawTextExA()` 等函数名与 `<windows.h>` 中的 Win32 API 函数名冲突（均为 `extern "C"` 但签名不同），无法在同一翻译单元中同时包含 `<windows.h>` 和 `raylib.h`。

`test_dialog_cabi_shared.h` 的解决方案：

```cpp
#ifndef _WINDOWS_
// windows.h 未被包含 → 手动 extern "C" 声明所需 Win32 API
extern "C" {
    __declspec(dllimport) void* __stdcall LoadLibraryA(const char* lpLibFileName);
    __declspec(dllimport) void* __stdcall GetProcAddress(void* hModule, const char* lpProcName);
    __declspec(dllimport) int   __stdcall FreeLibrary(void* hLibModule);
}
using HMODULE = void*;
#else
// windows.h 已被后端源码包含（SDL3/SFML 路径）→ 正常 include
#include <windows.h>
#endif
```

### 14.2 Cursor 工厂注册

在 fromsource/DLL 桥接模式下，`Cursor::registerFactories()` 的静态工厂注册路径不可用（`Cursor.cpp` 属于核心 DLL，后端函数指针未注册）。`UIBackendCallbacks` 回调查表新增三个函数指针（`createSystemCursor`/`getDefaultCursor`/`setCurrentCursor`），`BackendManager::initialize(callbacks)` 调用 `Cursor::registerFactories()` 从回调表注册。三后端的 `BackendPlugin.cpp` 均在 `GetUIBackendCallbacks()` 中填充这些回调。
| 13 | `test_dialog_cabi` 三后端 C ABI Dialog 集成测试（LoadLibrary + JSON dialogs + 共享头文件模式） | 集成（SDL3/SFML/Raylib） |
