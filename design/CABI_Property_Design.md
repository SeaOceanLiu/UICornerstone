# C ABI 属性系统设计

> 对应 Phase 16i | 编制 2026-07-26 | 状态: **已实现（Phases 1-6 完成；Phase 7 — Ptr 属性类型 + 控件特有 Ptr/Float/String/Enum/Bool 扩展已完成）**

## 目录

1. [动机](#1-动机)
2. [问题背景](#2-问题背景)
3. [备选方案](#3-备选方案)
4. [最终决策：结构化多态](#4-最终决策结构化多态)
5. [详细设计](#5-详细设计)
   - [5.7 Getter C ABI](#57-getter-c-abi)
   - [5.8 Callback C ABI](#58-callback-c-abi)
   - [5.9 属性名注册表](#59-属性名注册表)
   - [5.10 枚举值字符串管理](#510-枚举值字符串管理)
6. [属性命名约定](#6-属性命名约定)
   - [6.8 Enum 属性表](#68-enum-属性表)
   - [6.9 Callback 事件表](#69-callback-事件表)
7. [实施清单](#7-实施清单)
   - [测试策略](#测试策略)
8. [扩展指南](#8-扩展指南)

---

## 1. 动机

C ABI 层最初为每个控件的颜色属性暴露独立的导出函数（如 `UICornerstone_SetBGColor`、`UICornerstone_TreeViewSetSelectedColor`），每个自定义颜色需要写一个函数声明 + 一个实现。随着控件数量增加，这种方式导致大量重复代码：


| 控件     | 特有颜色属性数                                                                | 函数数 |
| -------- | ----------------------------------------------------------------------------- | ------ |
| TreeView | 5 个 (bg/border/hover/selected/text)                                          | 5      |
| Slider   | 7 个 (track/trackFill/thumb/thumbBorder/thumbHover/tick/label)                | 7      |
| ComboBox | 7 个 (arrow/arrowHover/itemSelected/itemHover/itemDisabled/listBg/listBorder) | 7      |
| CheckBox | 4 个 (check/cross/indeterminate/boxBorder)                                    | 4      |
| 累计     | ~23 个                                                                        | ~23    |

**目标**：提供一个统一、可扩展的 C ABI 入口，减少导出函数数量，同时支持任意控件的任意属性。

---

## 2. 问题背景

### 2.1 现有的颜色体系

代码库中存在两套并行的颜色存储机制：


| 机制            | 基类          | 存放位置                                                            | 特点                                                 |
| --------------- | ------------- | ------------------------------------------------------------------- | ---------------------------------------------------- |
| **StateColor**  | `ControlImpl` | `m_bgColor` / `m_borderColor` / `m_textColor` / `m_textShadowColor` | 4 态 (Normal/Hover/Pressed/Disabled)，通过虚方法设置 |
| **简单 SColor** | 各控件独立    | 控件自定义成员（如`TreeView::m_selectedColor`）                     | 单态，通过控件特有方法设置                           |

### 2.2 现有的 C ABI 颜色函数


| 函数                                                   | 范围        | 类型                       |
| ------------------------------------------------------ | ----------- | -------------------------- |
| `UICornerstone_SetBGColor(ctl, r,g,b,a)`               | 全控件      | StateColor（仅 Normal 态） |
| `UICornerstone_TreeViewSetBgColor(ctl, r,g,b,a)`       | 仅 TreeView | SColor                     |
| `UICornerstone_TreeViewSetSelectedColor(ctl, r,g,b,a)` | 仅 TreeView | SColor                     |
| ...                                                    | 仅某控件    | SColor                     |

### 2.3 关键数据

全代码库颜色属性分布（详见 [控件颜色调研](#附录控件颜色属性分布)）：

- **4 态 StateColor 通用属性**：background, border, text, text-shadow（ControlImpl 提供）
- **单色控件特有属性**：Slider(7), ComboBox(7), TreeView(5), CheckBox(4), ProgressBar(1), Splitter(3色合一), ColorPicker(2), NumericUpDown(3色合一)
- **总特有属性数**：约 20+

---

## 3. 备选方案

### 3.1 方案 A：全局枚举 + 单入口

```c
typedef enum {
    UIC_PROP_BACKGROUND,
    UIC_PROP_BORDER,
    UIC_PROP_TEXT,
    UIC_PROP_SELECTED,
    UIC_PROP_HOVER,
    UIC_PROP_TRACK,
    UIC_PROP_TRACK_FILL,
    // ... 不断增长
} UIColorProp;

void UICornerstone_SetColor(UIControlHandle ctl, UIColorProp prop, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
```


| 维度              | 评价                                                                   |
| ----------------- | ---------------------------------------------------------------------- |
| 入口数            | **1 个**                                                               |
| 枚举膨胀          | 必须包含所有控件特有属性 → 30+ 枚举值，成为大杂烩                     |
| 语义模糊          | `HOVER` 在 TreeView 是"行悬停色"，在 ComboBox 是"箭头悬停色"，含义不同 |
| StateColor 不兼容 | 基类用 4 态，但枚举方案只能表达单色                                    |
| 二进制稳定性      | 枚举值不可重排/插入，新值只能 append，否则破坏 ABI                     |

### 3.2 方案 B：每控件独立导出

```c
void UICornerstone_SliderSetTrackColor(UIControlHandle ctl, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void UICornerstone_ComboBoxSetItemSelectedColor(UIControlHandle ctl, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
// ... 每个属性一个函数
```


| 维度       | 评价                                       |
| ---------- | ------------------------------------------ |
| 入口数     | **~28 个**                                 |
| 自文档     | 函数名即文档，含义精确                     |
| 签名灵活   | Splitter 的 3 色同设可用独立签名           |
| 扩展成本高 | 每新增属性 = 1 个新导出函数（声明 + 实现） |
| 无枚举耦合 | 互不影响                                   |

### 3.3 方案 C：每控件聚合枚举 + 单入口

```c
typedef enum { UIC_TREEVIEW_COLOR_SELECTED, UIC_TREEVIEW_COLOR_HOVER, ... } UIC_TreeViewColorProp;
void UICornerstone_TreeViewSetColor(UIControlHandle ctl, UIC_TreeViewColorProp prop, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
```


| 维度                    | 评价                                                                                                    |
| ----------------------- | ------------------------------------------------------------------------------------------------------- |
| 入口数                  | **~6 个**（每控件 1 个）                                                                                |
| 枚举域化                | 不互相污染                                                                                              |
| TreeView bg/border 歧义 | 同时存在通用`SetBGColor`（StateColor 路径）+ TreeView 自身 `SetColor(..., BG)`（SColor 路径），效果不同 |

### 3.4 方案 D：全局字符串 + 虚方法分发

```c
int UICornerstone_SetColor(UIControlHandle ctl, const char* prop, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
```

Base Control 添加虚方法，各控件 override：

```cpp
// Control
virtual int setColorProperty(const char* prop, SColor color) { return 0; }

// ControlImpl
int ControlImpl::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, "background") == 0) { setNormalStateBGColor(color); return 1; }
    if (strcmp(prop, "border") == 0)     { setNormalStateBDColor(color); return 1; }
    if (strcmp(prop, "text") == 0)       { setTextNormalStateColor(color); return 1; }
    return 0;
}

// TreeView
int TreeView::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, "selected") == 0) { setSelectedColor(color); return 1; }
    if (strcmp(prop, "hover") == 0)    { setHoverColor(color);    return 1; }
    return ControlImpl::setColorProperty(prop, color); // fallback
}
```


| 维度             | 评价                                      |
| ---------------- | ----------------------------------------- |
| 入口数           | **1 个**（仅颜色）                        |
| 虚方法分发       | 零 dynamic_cast，纯虚函数，性能开销可忽略 |
| 字符串运行时错误 | 拼错属性名 → 返回 0，需调用方检查返回值  |
| StateColor 4 态  | 无法表达，字符串只能设 Normal 态          |

### 3.5 方案 E：变参 + 字符串

```c
int UICornerstone_SetProperty(UIControlHandle ctl, const char* prop, const char* type, ...);
```

```c
// 单色
UICornerstone_SetProperty(ctl, "selected", "color", 255, 0, 0, 255);
// StateColor 4 态（16 个参数）
UICornerstone_SetProperty(ctl, "background", "state", 200,200,200,255, 180,180,180,255, 160,160,160,255, 220,220,220,255);
// 整数
UICornerstone_SetProperty(ctl, "font-size", "int", 16);
```


| 问题            | 表现                                                |
| --------------- | --------------------------------------------------- |
| C 变参提升      | `uint8_t` 自动提升为 `int`，易出错                  |
| StateColor 爆炸 | 4 态 × 4 通道 = 16 个 int 参数，不可维护           |
| 无类型安全      | `"color"` 拼成 `"colo"` → 静默乱解析 → 运行时崩溃 |
| 无 IDE 提示     | 16 个参数，无法记住顺序                             |

### 3.6 方案 F（最终）：结构化多态

```c
typedef struct { uint8_t r, g, b, a; } UIColor;

typedef struct {
    UIColor normal;
    UIColor hover;
    UIColor pressed;
    UIColor disabled;
} UIStateColor;

// 5 个类型安全入口，每个支持字符串属性名
int UICornerstone_SetColor(     UIControlHandle ctl, const char* prop, UIColor       value);
int UICornerstone_SetStateColor( UIControlHandle ctl, const char* prop, UIStateColor  value);
int UICornerstone_SetInt(       UIControlHandle ctl, const char* prop, int           value);
int UICornerstone_SetFloat(     UIControlHandle ctl, const char* prop, float         value);
int UICornerstone_SetString(    UIControlHandle ctl, const char* prop, const char*   value);
```


| 维度            | 评价                             |
| --------------- | -------------------------------- |
| 入口数          | **6 个**（覆盖所有值类型）       |
| 类型安全        | struct 成员名自解释，无变参      |
| StateColor 友好 | 原生结构体支持，命名成员无歧义   |
| 扩展性          | 新属性加一行`strcmp`，不改变 ABI |
| IDE 可发现      | struct 成员在 VS 等 IDE 中有补全 |

---

### 3.7 方案 G：Union 单入口

```c
typedef union {
    UIColor      color;
    UIStateColor stateColor;
    int          i;
    float        f;
    const char*  str;
} UPropValue;

int UICornerstone_SetProp(UIControlHandle ctl, const char* prop, UPropValue value);

// 调用
UICornerstone_SetProp(ctl, "selected",  (UPropValue){.color = {255,0,0,255}});
UICornerstone_SetProp(ctl, "font-size", (UPropValue){.i     = 16});
```


| 问题                     | 表现                                                                                                            |
| ------------------------ | --------------------------------------------------------------------------------------------------------------- |
| **union 不携带类型信息** | 实现层无法判断该读哪个成员。try-all 有崩溃风险：`const char*` 占 8 字节，读 float 的 4 字节当成指针 → 非法地址 |
| **加 type tag 则更啰嗦** | `SetProp(ctl, "x", {TYPE_COLOR, {.color=...}})` — 类型写了 2 次，不如独立函数简洁                              |
| **入口数**               | 1 个，但代价是安全性或冗余度                                                                                    |

### 3.8 方案 H：Struct 泛化入口

```c
int UICornerstone_SetStruct(UIControlHandle ctl, const char* prop, const void* data, int dataSize);

UIRange range = {0.0f, 100.0f};
UICornerstone_SetStruct(ctl, "range", &range, sizeof(range));
```


| 问题                   | 表现                                                                                |
| ---------------------- | ----------------------------------------------------------------------------------- |
| **void* 绕过类型系统** | 编译期零检查，运行期靠 size 校验，不足                                              |
| **struct 对齐风险**    | `#pragma pack` 不一致导致跨 ABI 边界崩溃                                            |
| **受益面极窄**         | 仅 7 个复合属性（`setRange`、`setPresetLayout`、`setMinSize` 等），占总属性数 < 10% |
| **拆分方案已足够**     | 多调 1-3 次 C ABI 函数在初始化路径上无感知                                          |

**结论**：Union 和 Struct 都为少数场景引入了不成正比的风险和复杂度，不采纳。

---

## 4. 最终决策：结构化多态

### 4.1 理由

1. **类型安全**：5 个显式函数替代变参，参数通过 struct 成员命名
2. **StateColor 友好**：`UIStateColor` 的 `.normal/.hover/.pressed/.disabled` 成员自解释，避免 16 参数混乱
3. **虚方法分发**：每值类型一个虚方法，零 `dynamic_cast`，O(1) 分发
4. **属性名全局统一字符串**：不占用枚举 ABI 槽位，不共享枚举域，互不污染
5. **结构体 ABI 稳定**：`UIColor`/`UIStateColor` 是简单 POD 结构，成员位置固定

### 4.2 对比总结


| 维度           | A:全局枚举 | B:独立函数 | C:控件枚举 | D:字符串   | E:变参   | **F:结构化(选)** | G:Union      | H:Struct  |
| -------------- | ---------- | ---------- | ---------- | ---------- | -------- | ---------------- | ------------ | --------- |
| 函数数         | 1          | 28+        | ~6         | 1          | 1        | **6**            | 1            | 1         |
| 类型安全       | ✅         | ✅         | ✅         | ⚠️       | ❌       | **✅**           | ❌           | ❌        |
| StateColor 4态 | ❌         | ✅         | ❌         | ❌         | ❌       | **✅**           | ❌           | ❌        |
| Bool 支持      | ❌         | ✅         | ❌         | ⚠️       | ❌       | **✅**           | ❌           | ❌        |
| 无枚举耦合     | —         | ✅         | ⚠️       | ✅         | ✅       | **✅**           | ✅           | ✅        |
| 签名灵活       | ❌         | ✅         | ❌         | ✅         | ⚠️     | **✅**           | ⚠️         | ⚠️      |
| IDE 补全       | ✅         | ✅         | ✅         | ❌(字符串) | ❌       | **⚠️(struct)** | ⚠️(struct) | ❌(void*) |
| 新属性扩展     | 改枚举     | 加函数     | 改枚举     | 加strcmp   | 加strcmp | **加strcmp**     | 加strcmp     | 加strcmp  |

---

## 5. 详细设计

### 5.1 架构

```
Setter C ABI (9 入口)                  Getter C ABI (8 入口)
    │                                        │
    ▼                                        ▼
Control::set*Property(prop, val)      Control::get*Property(prop, &out)
    │                                        │
    ├─ Property（颜色/数值/文本/枚举/指针）    ├─ Property 读取
    └─ Callback（事件绑定）

值类型: Color | StateColor | Int | Float | Bool | String | Enum | Ptr | Callback
                                                    └── Ptr：指针属性（void*）
```

### 5.2 C ABI 类型定义

```c
// include/UICornerstoneAPI.h

/* ============ 属性系统类型 ============ */
typedef struct { uint8_t r, g, b, a; } UIColor;

typedef struct {
    UIColor normal;
    UIColor hover;
    UIColor pressed;
    UIColor disabled;
} UIStateColor;

/* ============ 属性系统入口 ============ */
// 设置单色属性（例如 TreeView 的 "selected"、"hover"）
// prop 属性名，value 颜色值
// 返回 1 成功，0 不识别
int UICornerstone_SetColor(UIControlHandle ctl, const char* prop, UIColor value);

// 设置 4 态颜色属性（例如全控件通用的 "background"、"border"）
// prop 属性名，value 四态颜色
int UICornerstone_SetStateColor(UIControlHandle ctl, const char* prop, UIStateColor value);

// 设置布尔属性（例如 "visible"、"enabled"）
// value: 0=假, 非0=真
int UICornerstone_SetBool(UIControlHandle ctl, const char* prop, int value);

// 设置整数属性（例如 "font-size"）
int UICornerstone_SetInt(UIControlHandle ctl, const char* prop, int value);

// 设置浮点属性（例如 "row-height", "indent-width"）
int UICornerstone_SetFloat(UIControlHandle ctl, const char* prop, float value);

// 设置字符串属性（例如 "text"、"font"）
int UICornerstone_SetString(UIControlHandle ctl, const char* prop, const char* value);

// 设置枚举属性（例如 "style"、"align"）
// value 为枚举值名称的字符串，如 "Vertical"、"Checked"
// 不区分大小写
int UICornerstone_SetEnum(UIControlHandle ctl, const char* prop, const char* value);

// 设置指针属性（例如 Popup 的 "content"、Splitter 的 "first-linked"/"second-linked"）
// value 为 UIControlHandle（即 Control*），调用方保证生命周期
int UICornerstone_SetPtr(UIControlHandle ctl, const char* prop, void* value);

/* ── Getter ── */
// ...（GetColor/GetStateColor/GetBool/GetInt/GetFloat/GetString/GetEnum）

// 读取指针属性。out 返回属性值，返回 1 成功，0 不识别
int UICornerstone_GetPtr(UIControlHandle ctl, const char* prop, void** out);

/* ============ 回调系统 ============ */
typedef void (*UIEventCallback)(UIControlHandle ctl, const UIEventData* event, void* userData);

typedef struct {
    const char* eventName;
    union {
        float           floatVal;
        double          doubleVal;
        int             intVal;
        const char*     strVal;
        struct { int idx; const char* val; } selection;
        struct { int row; int col; int asc; } grid;
    } data;
} UIEventData;

// 绑定事件回调
// 所有控件的事件（click、value-changed、text-changed 等）统一走此入口
int UICornerstone_SetCallback(UIControlHandle ctl, const char* event, UIEventCallback cb, void* userData);
```

### 5.3 虚方法接口

```cpp
// include/ControlBase.h (Control 类新增)

virtual int setColorProperty(const char* prop, SColor color) { return 0; }
virtual int setStateColorProperty(const char* prop, StateColor stateColor) { return 0; }
virtual int setBoolProperty(const char* prop, int value) { return 0; }
virtual int setIntProperty(const char* prop, int value) { return 0; }
virtual int setFloatProperty(const char* prop, float value) { return 0; }
virtual int setStringProperty(const char* prop, const char* value) { return 0; }
virtual int setEnumProperty(const char* prop, const char* value) { return 0; }
virtual int setPtrProperty(const char* prop, void* value) { return 0; }

// 回调绑定：event 事件名（不包含 "on" 前缀），如 "click"、"value-changed"
// cb C 回调函数指针，userData 透传给 cb
virtual int setCallbackProperty(const char* event, UIEventCallback cb, void* userData) { return 0; }
```

### 5.4 ControlImpl 实现通用属性

```cpp
// src/ControlBase.cpp

int ControlImpl::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, "background") == 0)         { setNormalStateBGColor(color);  return 1; }
    if (strcmp(prop, "background.hover") == 0)   { setHoverStateBGColor(color);   return 1; }
    if (strcmp(prop, "background.pressed") == 0) { setPressedStateBGColor(color); return 1; }
    if (strcmp(prop, "background.disabled") == 0){ setDisabledStateBGColor(color);return 1; }
    if (strcmp(prop, "border") == 0)             { setNormalStateBDColor(color);  return 1; }
    if (strcmp(prop, "border.hover") == 0)       { setHoverStateBDColor(color);   return 1; }
    if (strcmp(prop, "border.pressed") == 0)     { setPressedStateBDColor(color); return 1; }
    if (strcmp(prop, "border.disabled") == 0)    { setDisabledStateBDColor(color);return 1; }
    if (strcmp(prop, "text") == 0)               { setTextNormalStateColor(color);return 1; }
    if (strcmp(prop, "text.hover") == 0)         { setTextHoverStateColor(color); return 1; }
    if (strcmp(prop, "text.pressed") == 0)       { setTextPressedStateColor(color);return 1; }
    if (strcmp(prop, "text.disabled") == 0)      { setTextDisabledStateColor(color);return 1; }
    if (strcmp(prop, "text-shadow") == 0)        { setTextShadowNormalStateColor(color);return 1; }
    // ...
    return 0;
}

int ControlImpl::setStateColorProperty(const char* prop, StateColor sc) {
    if (strcmp(prop, "background") == 0) { setBackgroundStateColor(sc); return 1; }
    if (strcmp(prop, "border") == 0)     { setBorderStateColor(sc);     return 1; }
    if (strcmp(prop, "text") == 0)       { setTextStateColor(sc);       return 1; }
    if (strcmp(prop, "text-shadow") == 0){ setTextShadowStateColor(sc); return 1; }
    return 0;
}

int ControlImpl::setBoolProperty(const char* prop, int value) {
    bool b = value != 0;
    if (strcmp(prop, "visible") == 0)         { setVisible(b); return 1; }
    if (strcmp(prop, "enabled") == 0)         { setEnable(b);  return 1; }
    if (strcmp(prop, "transparent") == 0)     { setTransparent(b); return 1; }
    if (strcmp(prop, "border-visible") == 0)  { setBorderVisible(b); return 1; }
    return 0;
}
```

### 5.5 C ABI 实现

```cpp
// src/UICornerstoneAPI.cpp

int UICornerstone_SetColor(UIControlHandle ctl, const char* prop, UIColor value) {
    auto* c = static_cast<Control*>(ctl);
    if (!c || !prop) return 0;
    return c->setColorProperty(prop, SColor(value.r, value.g, value.b, value.a));
}

int UICornerstone_SetStateColor(UIControlHandle ctl, const char* prop, UIStateColor value) {
    auto* c = static_cast<Control*>(ctl);
    if (!c || !prop) return 0;
    StateColor sc(value.normal, value.hover, value.pressed, value.disabled);
    return c->setStateColorProperty(prop, sc);
}
// ...其余类似（SetInt、SetFloat、SetString、SetEnum）

int UICornerstone_SetBool(UIControlHandle ctl, const char* prop, int value) {
    auto* c = static_cast<Control*>(ctl);
    if (!c || !prop) return 0;
    return c->setBoolProperty(prop, value);
}

int UICornerstone_SetEnum(UIControlHandle ctl, const char* prop, const char* value) {
    auto* c = static_cast<Control*>(ctl);
    if (!c || !prop || !value) return 0;
    return c->setEnumProperty(prop, value);
}

int UICornerstone_SetCallback(UIControlHandle ctl, const char* event, UIEventCallback cb, void* userData) {
    auto* c = static_cast<Control*>(ctl);
    if (!c || !event) return 0;
    return c->setCallbackProperty(event, cb, userData);
}
```

### 5.6 TreeView 覆写

```cpp
// src/TreeView.cpp

int TreeView::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, "selected") == 0) { setSelectedColor(color); return 1; }
    if (strcmp(prop, "hover") == 0)    { setHoverColor(color);    return 1; }
    if (strcmp(prop, "background") == 0){ setBgColor(color);      return 1; }
    if (strcmp(prop, "border") == 0)   { setBorderColor(color);   return 1; }
    if (strcmp(prop, "text") == 0)     { setTextColor(color);     return 1; }
    // fallback: 用基类的 StateColor Normal 态
    return ControlImpl::setColorProperty(prop, color);
}
```

---

### 5.7 Getter C ABI

Getter 与 Setter 对称：6 个虚方法 + 6 个 C ABI 函数。返回值表示属性是否被识别（1=成功，0=未识别），值通过输出参数返回。

#### 虚方法接口

```cpp
// include/ControlBase.h (Control 类新增)

virtual int getColorProperty(const char* prop, SColor& out) { return 0; }
virtual int getStateColorProperty(const char* prop, StateColor& out) { return 0; }
virtual int getBoolProperty(const char* prop, int& out) { return 0; }
virtual int getIntProperty(const char* prop, int& out) { return 0; }
virtual int getFloatProperty(const char* prop, float& out) { return 0; }
virtual int getStringProperty(const char* prop, const char*& out) { return 0; }
virtual int getEnumProperty(const char* prop, const char*& out) { return 0; }
virtual int getPtrProperty(const char* prop, void*& out) { return 0; }
```

#### C ABI 函数

```c
// include/UICornerstoneAPI.h

int UICornerstone_GetColor(UIControlHandle ctl, const char* prop, UIColor* out);
int UICornerstone_GetStateColor(UIControlHandle ctl, const char* prop, UIStateColor* out);
int UICornerstone_GetBool(UIControlHandle ctl, const char* prop, int* out);
int UICornerstone_GetInt(UIControlHandle ctl, const char* prop, int* out);
int UICornerstone_GetFloat(UIControlHandle ctl, const char* prop, float* out);
int UICornerstone_GetString(UIControlHandle ctl, const char* prop, char* out, int maxLen);
int UICornerstone_GetEnum(UIControlHandle ctl, const char* prop, char* out, int maxLen);
int UICornerstone_GetPtr(UIControlHandle ctl, const char* prop, void** out);
```

String 使用 `char* out + maxLen` 模式，避免静态 buffer 或 malloc 生命周期问题。

#### ControlImpl 默认实现

Setter 在写入时同时保存当前值，Getter 直接从内存读取（例如 `getNormalStateBGColor`）。若属性不存在返回 0。

```cpp
int ControlImpl::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, "background") == 0)          { out = getNormalStateBGColor();  return 1; }
    if (strcmp(prop, "background.hover") == 0)    { out = getHoverStateBGColor();   return 1; }
    if (strcmp(prop, "background.pressed") == 0)  { out = getPressedStateBGColor(); return 1; }
    if (strcmp(prop, "background.disabled") == 0) { out = getDisabledStateBGColor();return 1; }
    if (strcmp(prop, "border") == 0)              { out = getNormalStateBDColor();  return 1; }
    // ...
    return 0;
}
```

#### C ABI 实现

```cpp
// src/UICornerstoneAPI.cpp

int UICornerstone_GetColor(UIControlHandle ctl, const char* prop, UIColor* out) {
    auto* c = static_cast<Control*>(ctl);
    if (!c || !prop || !out) return 0;
    SColor s;
    if (!c->getColorProperty(prop, s)) return 0;
    out->r = s.getRed(); out->g = s.getGreen();
    out->b = s.getBlue(); out->a = s.getAlpha();
    return 1;
}
// ...其余类似（GetStateColor、GetBool、GetInt、GetFloat、GetString、GetEnum）
```

---

### 5.8 Callback C ABI

回调（事件）与属性不同：属性设值后一直有效，回调注册后等待触发。回调走独立的 C ABI 入口 `SetCallback`，与 Set/Get Property 并列。

#### 数据结构

```c
typedef void (*UIEventCallback)(UIControlHandle ctl, const UIEventData* event, void* userData);

typedef struct {
    const char* eventName;  // "click"、"value-changed" 等
    union {
        float           floatVal;
        double          doubleVal;
        int             intVal;
        const char*     strVal;   // 临时指针，回调内立即用
        struct { int idx; const char* val; } selection;
    } data;
} UIEventData;
```

`eventName` 使 C 方可在同一个回调函数中通过 `strcmp(event->eventName, "click")` 区分不同事件。

#### 虚方法

```cpp
// include/ControlBase.h
virtual int setCallbackProperty(const char* event, UIEventCallback cb, void* userData) { return 0; }
```

每个控件 override 此方法，将 C ABI 回调桥接到 C++ `std::function`：

```cpp
// Button.cpp
int Button::setCallbackProperty(const char* event, UIEventCallback cb, void* userData) {
    if (strcmp(event, "click") == 0) {
        setOnClick([this, cb, userData](auto self) {
            UIEventData ev{};
            ev.eventName = "click";
            cb((UIControlHandle)this, &ev, userData);
        });
        return 1;
    }
    return ControlImpl::setCallbackProperty(event, cb, userData);
}
```

#### 带数据的回调桥接

```cpp
// Slider.cpp
int Slider::setCallbackProperty(const char* event, UIEventCallback cb, void* userData) {
    if (strcmp(event, "click") == 0) { /* ... */ }
    if (strcmp(event, "value-changed") == 0) {
        setOnValueChanged([this, cb, userData](auto self, float val) {
            UIEventData ev{};
            ev.eventName = "value-changed";
            ev.data.floatVal = val;
            cb((UIControlHandle)this, &ev, userData);
        });
        return 1;
    }
    return ControlImpl::setCallbackProperty(event, cb, userData);
}
```

C 方处理：

```c
void myHandler(UIControlHandle ctl, const UIEventData* event, void* userData) {
    if (strcmp(event->eventName, "click") == 0) {
        // 按钮被点击
    } else if (strcmp(event->eventName, "value-changed") == 0) {
        float newVal = event->data.floatVal;
        // ...
    }
}
// UICornerstone_SetCallback(ctl, "value-changed", myHandler, NULL);
```

#### 设计要点


| 点                       | 说明                                                                                                                                                                                                                    |
| ------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 一个回调函数处理所有事件 | 通过`event->eventName` 区分，避免每个事件一个导出函数                                                                                                                                                                   |
| 不丢失事件数据           | `UIEventData` 的 union 覆盖所有回调参数类型                                                                                                                                                                             |
| 字符串参数为临时指针     | `strVal` 指向 C++ 侧临时 buffer，回调返回后失效，必须立即使用或拷贝                                                                                                                                                     |
| 事件名字典               | 事件名统一 kebab-case，不加 "on" 前缀：`"click"`、`"value-changed"`、`"text-changed"`、`"selection-changed"`、`"check-changed"`、`"confirm"`、`"cancel"`、`"close"`、`"enter"`、`"select"`、`"expand"`、`"collapse"` 等 |
| 替换范围                 | 替换现有的`UICornerstone_SetOnClick` 等 7 个 `UIActionCallback` 导出和 2 个带数据回调导出                                                                                                                               |

#### 性能分析

**注册时**：`setCallbackProperty` 内逐条 `strcmp` 匹配事件名，一次性开销，不关键。

**触发时（关键路径）**：C++ 回调触发 → 构造 `UIEventData` → 直接调用绑定的 C 函数指针。**零 strcmp、零查找**——每个 C 回调只对应一个事件，handler 无需区分事件名：

```c
void myValueChangedHandler(UIControlHandle ctl, const UIEventData* event, void* userData) {
    float newVal = event->data.floatVal;  // 直接读，无需 strcmp
    // ...
}
// UICornerstone_SetCallback(ctl, "value-changed", myValueChangedHandler, NULL);
```

`eventName` 字段仅用于通用路由器（一个回调注册多个事件时做分发）或调试日志，非通用路由器场景下 C 方忽略即可。

对比：专有导出方案在触发时同样是直接调用 C 函数指针，零 strcmp。统一 `SetCallback` 的最坏路径（通用路由器 + 20 个事件）每次触发 1 次 strcmp，实测 < 100ns，60fps 下 6μs/秒，无感。

#### 字符串比较 vs Hash Map 的选择

`set*Property` 和 `setCallbackProperty` 均使用线性 `strcmp` 链进行属性名匹配。Hash Map（`std::unordered_map`）方案曾被考虑但未采用：


| 维度              | strcmp 线性链                       | Hash Map                                    |
| ----------------- | ----------------------------------- | ------------------------------------------- |
| **注册/调用耗时** | 最坏 O(n)（n < 16），实测 <200ns    | O(1) 均摊，但哈希计算 + 表查找 ≈ 50-100ns  |
| **无初始化开销**  | ✅ 零额外内存，零构造               | ❌ 需构造 map，分配内存                     |
| **调用点可读性**  | ✅ 同一函数内连续 if-else，一目了然 | ❌ 字符串藏在远处理论表中，调用点只看到 map |
| **调试友好**      | ✅ 断点打在 strcmp 即可拦截任意属性 | ❌ 需在 map 查找回调中加断点                |
| **分支预测**      | ❌ 线性链对 CPU 分支预测不友好      | ✅ 哈希跳转更可预测                         |

**决策**：对于属性解析路径——一次 setup 调用，非热循环——线性 strcmp 链的绝对耗时（<200ns）远低于人类感知阈值。Hash Map 的内存开销和抽象泄漏得不偿失。若未来 profiler 证明某条解析路径成为帧循环热点，再局部替换为 `unordered_map`。

#### 未来优化（profile-driven）

若 profiler 证实在通用路由器场景下 strcmp 成热点：

```c
typedef struct {
    const char* eventName;
    int         eventId;    // C 方缓存后走 switch
    union { /* ... */ } data;
} UIEventData;
```

初始注册时 C 方通过 `strcmp` 查一次 `eventId`，后续触发直接 `switch (eventId)`。未成为热点前不引入。

---

### 5.9 属性名注册表

属性名字符串分散在各控件 `set*Property` 的 `strcmp` 链中，难以检索重复和冲突。统一管理方式：

```cpp
// include/PropertyNames.h

#pragma once

// 命名规范：k{PropertyName}，全小写 kebab-case 映射
namespace PropertyNames {

// ── 通用（全控件）──
inline constexpr const char* kBackground      = "background";
inline constexpr const char* kBorder          = "border";
inline constexpr const char* kText            = "text";
inline constexpr const char* kTextShadow      = "text-shadow";
inline constexpr const char* kVisible         = "visible";
inline constexpr const char* kEnabled         = "enabled";
inline constexpr const char* kTransparent     = "transparent";
inline constexpr const char* kBorderVisible   = "border-visible";

// ── 数值 ──
inline constexpr const char* kFontSize        = "font-size";
inline constexpr const char* kLineHeight      = "line-height";
inline constexpr const char* kScrollX         = "scroll-x";
inline constexpr const char* kScrollY         = "scroll-y";
inline constexpr const char* kStep            = "step";
inline constexpr const char* kValue           = "value";
inline constexpr const char* kDecimals        = "decimals";
inline constexpr const char* kRangeMin        = "range-min";
inline constexpr const char* kRangeMax        = "range-max";
inline constexpr const char* kLabelFontSize   = "label-font-size";
inline constexpr const char* kSelectedIndex   = "selected-index";
inline constexpr const char* kButtonHeight    = "button-height";

// ── 枚举 ──
inline constexpr const char* kAlign           = "align";
inline constexpr const char* kFont            = "font";
inline constexpr const char* kStyle           = "style";
inline constexpr const char* kOrientation     = "orientation";
inline constexpr const char* kCheckState      = "check-state";

// ── 事件 ──
inline constexpr const char* kEventClick      = "click";
inline constexpr const char* kEventValueChanged = "value-changed";
inline constexpr const char* kEventTextChanged = "text-changed";

} // namespace PropertyNames
```

#### 使用方式

```cpp
// 原：if (strcmp(prop, "font-size") == 0)
// 改：if (strcmp(prop, PropertyNames::kFontSize) == 0)
```

#### 好处


| 方面         | 说明                                                   |
| ------------ | ------------------------------------------------------ |
| **防 typo**  | 编译期检查，拼错常量名报 undefined                     |
| **IDE 补全** | 输入`PropertyNames::k` 即可看到所有属性名              |
| **查重**     | `rg "k[A-Z]" include/PropertyNames.h` 一眼看出同名常量 |
| **全局检索** | `rg "PropertyNames::kBackground"` 找到所有使用点       |
| **文档同步** | 头文件本身就是属性名清单，可直接生成 §6 表格          |

#### 冲突处理

同名属性用于不同控件时，在常量名上加控件前缀：

```cpp
inline constexpr const char* kCheckBoxStyle   = "style";   // CheckBox: classic/cross/circle
inline constexpr const char* kProgressBarStyle = "style";   // ProgressBar: horizontal/vertical
```

`kCheckBoxStyle` 和 `kProgressBarStyle` 值相同但名字不同，IDE 检索 `"style"` 仍能发现两条定义，做冲突判断。

### 5.10 枚举值字符串管理

`SetEnum` 接口传入枚举值字符串（如 `"horizontal"`、`"checked"`），实现层需将其映射为 C++ 枚举值。策略：

**原则**：枚举值字符串的映射函数与枚举定义放在同一文件，不由 `PropertyNames.h` 管理。

```cpp
// Slider.h — 紧挨着 enum class SliderStyle
inline SliderStyle SliderStyleFromString(const char* s) {
    if (_stricmp(s, "horizontal") == 0) return SliderStyle::Horizontal;
    if (_stricmp(s, "vertical")   == 0) return SliderStyle::Vertical;
    return SliderStyle::Horizontal; // default
}
```


| 角度         | 评价                                                                    |
| ------------ | ----------------------------------------------------------------------- |
| **职责归位** | 属性名常量归`PropertyNames.h`，枚举值映射归枚举所在文件，两个维度不耦合 |
| **改不漏**   | 增删枚举值，顺手更新同文件的 FromString 函数，不担心跨文件遗漏          |
| **查询方便** | `rg "SliderStyleFromString"` 即可找到所有调用点                         |
| **命名统一** | `{EnumName}FromString` 模式，IDE 输入 `FromString` 即得补全             |

FontName（28 值）延续现有 `FontNameFromString` 函数，模式一致。

---

## 6. 属性命名约定

### 6.1 命名规则

```
{property-name}[.{state}]
```

- 属性名全小写，多词用连字符 `-`
- 枚举值字符串（控件类型、对齐、布局类型、字体样式、BlendMode、绑定模式等）同样全小写 kebab-case，多词用连字符 `-`（如 `"edit-box"`、`"top-stretch"`、`"additive-premultiplied"`；解析侧不区分大小写，读写两侧保持同一小写风格）
- 4 态后缀 (仅用于 `SetColor` 单色入口设置 StateColor 的某态)：
  - `.normal`（默认，可不写）
  - `.hover`
  - `.pressed`
  - `.disabled`

### 6.2 通用属性表


| 属性名                  | 值类型     | C ABI 入口              | 范围   |
| ----------------------- | ---------- | ----------------------- | ------ |
| `"background"`          | StateColor | `SetStateColor`         | 全控件 |
| `"background"`          | Color      | `SetColor` → Normal 态 | 全控件 |
| `"background.hover"`    | Color      | `SetColor`              | 全控件 |
| `"background.pressed"`  | Color      | `SetColor`              | 全控件 |
| `"background.disabled"` | Color      | `SetColor`              | 全控件 |
| `"border"`              | StateColor | `SetStateColor`         | 全控件 |
| `"border"`              | Color      | `SetColor` → Normal 态 | 全控件 |
| `"border.hover"`        | Color      | `SetColor`              | 全控件 |
| `"border.pressed"`      | Color      | `SetColor`              | 全控件 |
| `"border.disabled"`     | Color      | `SetColor`              | 全控件 |
| `"text"`                | StateColor | `SetStateColor`         | 全控件 |
| `"text"`                | Color      | `SetColor` → Normal 态 | 全控件 |
| `"text.hover"`          | Color      | `SetColor`              | 全控件 |
| `"text.pressed"`        | Color      | `SetColor`              | 全控件 |
| `"text.disabled"`       | Color      | `SetColor`              | 全控件 |
| `"text-shadow"`         | StateColor | `SetStateColor`         | 全控件 |
| `"text-shadow"`         | Color      | `SetColor` → Normal 态 | 全控件 |

### 6.3 控件特有属性表


| 控件          | 属性名               | 值类型 | 说明               |
| ------------- | -------------------- | ------ | ------------------ |
| TreeView      | `"selected"`         | Color  | 选中行背景色       |
| TreeView      | `"hover"`            | Color  | 悬停行背景色       |
| Slider        | `"track"`            | Color  | 轨道色             |
| Slider        | `"track-fill"`       | Color  | 轨道填充色         |
| Slider        | `"thumb"`            | Color  | 滑块色             |
| Slider        | `"thumb-border"`     | Color  | 滑块边框色         |
| Slider        | `"thumb-hover"`      | Color  | 滑块悬停色         |
| Slider        | `"tick"`             | Color  | 刻度线色           |
| Slider        | `"label"`            | Color  | 标签色             |
| ComboBox      | `"arrow"`            | Color  | 箭头色             |
| ComboBox      | `"arrow-hover"`      | Color  | 箭头悬停色         |
| ComboBox      | `"item-selected"`    | Color  | 列表选中项色       |
| ComboBox      | `"item-hover"`       | Color  | 列表悬停项色       |
| ComboBox      | `"item-disabled"`    | Color  | 列表禁选项色       |
| ComboBox      | `"list-bg"`          | Color  | 列表背景色         |
| ComboBox      | `"list-border"`      | Color  | 列表边框色         |
| CheckBox      | `"check"`            | Color  | 勾选标记色         |
| CheckBox      | `"cross"`            | Color  | 叉号标记色         |
| CheckBox      | `"indeterminate"`    | Color  | 不确定态标记色     |
| CheckBox      | `"box-border"`       | Color  | 复选框边框色       |
| ProgressBar   | `"progress"`         | Color  | 进度条填充色       |
| Splitter      | `"line"`             | Color  | 分割线色（Normal） |
| Splitter      | `"line-hover"`       | Color  | 分割线悬停色       |
| Splitter      | `"line-drag"`        | Color  | 分割线拖拽色       |
| ColorPicker   | `"closed-text"`      | Color  | 关闭态文字色       |
| ColorPicker   | `"popup-bg"`         | Color  | 弹窗背景色         |
| WinFrame      | `"win-frame-bg"`     | Color  | WinFrame 背景色    |
| WinFrame      | `"win-frame-border"` | Color  | WinFrame 边框色    |
| WinFrame      | `"title-bar-bg"`     | Color  | 标题栏背景色       |
| WinFrame      | `"title-text"`       | Color  | 标题栏文字色       |
| NumericUpDown | `"arrow"`            | Color  | 箭头色（Normal）   |
| NumericUpDown | `"arrow-hover"`      | Color  | 箭头悬停色         |
| NumericUpDown | `"arrow-pressed"`    | Color  | 箭头按下色         |

### 6.4 Bool 属性表

> `setBoolProperty` 桥接状态：`✅` = 已通过 C ABI `SetBool` 可用，`❌` = 仅有 C++ setter，未桥接
>
>
> | 控件          | 属性名                     | setter/说明                                     | setBoolProperty  | getBoolProperty |
> | ------------- | -------------------------- | ----------------------------------------------- | --------------- |--------|
> | 全控件        | `"visible"`                | `setVisible(bool)`                              | ✅              | ✅ |
> | 全控件        | `"enabled"`                | `setEnable(bool)`                               | ✅              | ✅ |
> | 全控件        | `"transparent"`            | `setTransparent(bool)`                          | ✅              | ✅ |
> | 全控件        | `"border-visible"`         | `setBorderVisible(bool)`                        | ✅              | ✅ |
> | Button        | `"text-shadow-enable"`     | `setTextShadowEnable(bool)`                     | ✅                 | ❌ |
> | Label         | `"shadow"`                 | `setShadow(bool)`                               | ✅                 | ✅ |
> | Label         | `"expand"`                 | `setEnableExpand(bool)`                         | ✅                 | ✅ |
> | Label         | `"clickable"`              | `setClickable(bool)`                            | ✅                 | ✅ |
> | EditBox       | `"password-mode"`          | `setPasswordMode(bool)`                         | ✅                 | ✅ |
> | TextArea      | `"word-wrap"`              | `setWordWrap(bool)`                             | ✅                 | ✅ |
> | CheckBox      | `"checked"`                | `setCheckState(Checked/Unchecked)`              | ✅                 | ✅ |
| CheckBox      | `"tri-state"`              | `setTriStateEnabled(bool)`                      | ✅                 | ✅ |
> | Slider        | `"reverse"`                | `setReverse(bool)`                              | ✅                 | ✅ |
> | Slider        | `"show-value-label"`       | `setShowValueLabel(bool)`                       | ✅                 | ✅ |
> | ComboBox      | `"cycle-enabled"`          | `setCycleEnabled(bool)`                         | ✅                 | ✅ |
> | TreeView      | `"cycle-navigation"`       | `setCycleNavigation(bool)`                      | ✅                 | ✅ |
> | TreeView      | `"default-expand"`         | `setDefaultExpand(bool)`                        | ✅                 | ✅ |
> | TreeView      | `"expand-all"`             | 展开全部节点，忽略 value                        | ✅                 | ❌ |
> | TreeView      | `"collapse-all"`           | 折叠全部节点，忽略 value                        | ✅                 | ❌ |
> | WinFrame      | `"resizable"`              | `setResizable(bool)`                            | ✅                 | ✅ |
> | Splitter      | `"horizontal"`             | `setOrientation(bool) — true=水平, false=垂直` | ✅                 | ✅ |
> | Dialog        | `"close-on-click-outside"` | `setCloseOnClickOutside(bool)`                  | ✅                 | ✅ |
> | Dialog        | `"close-on-esc"`           | `setCloseOnEsc(bool)`                           | ✅                 | ✅ |
> | ConfirmPopup  | `"confirm-visible"`        | `setConfirmButtonVisible(bool)`                 | ✅                 | ❌ |
> | NumericUpDown | `"read-only"`              | `setReadOnly(bool)`                             | ✅                 | ✅ |
> | MenuItem      | `"checked"`                | `setChecked(bool)`                             | ✅                 | ✅ |
> | Image        | `"match-parent-rect"`      | `setMatchParentRect(bool)` — 开启后 w/h 被父尺寸覆盖（Image_Design §6.1） | ✅ | ✅ |

### 6.5 Int 属性表

> `setIntProperty` 桥接状态：`✅` = 已通过 C ABI `SetInt` 可用，`❌` = 仅有 C++ setter，未桥接
>
>
> | 控件          | 属性名                | setter/说明                   | setIntProperty  | getIntProperty |
> | ------------- | --------------------- | ----------------------------- | -------------- |--------|
> | Label         | `"font-size"`         | `setFontSize(int)`            | ✅                | ✅ |
> | Label         | `"line-height"`       | `setLineHeight(int)`          | ✅                | ✅ |
> | EditBox       | `"font-size"`         | `setFontSize(int)`            | ✅                | ✅ |
> | TextArea      | `"line-height"`       | `setLineHeight(int)`          | ✅                | ✅ |
> | TextArea      | `"scroll-x"`          | `setScrollX(int)`             | ✅                | ✅ |
> | TextArea      | `"scroll-y"`          | `setScrollY(int)`             | ✅                | ✅ |
> | ProgressBar   | `"font-size"`         | `setFontSize(int)`            | ✅                | ✅ |
> | Slider        | `"label-font-size"`   | `setLabelFontSize(int)`       | ✅                | ✅ |
> | ColorPicker   | `"preset-cols"`       | `setPresetLayout(cols, rows)` | ✅                | ✅ |
> | ColorPicker   | `"preset-rows"`       | `setPresetLayout(cols, rows)` | ✅                | ✅ |
> | ColorPicker   | `"closed-font-size"`  | `setClosedFontSize(int)`      | ✅                | ✅ |
> | ComboBox      | `"max-visible-items"` | `setMaxVisibleItems(int)`     | ✅                | ✅ |
> | ComboBox      | `"selected-index"`    | `setSelectedIndex(int)`       | ✅                | ✅ |
> | MenuPanel     | `"hovered-index"`     | `setHoveredIndex(int)`        | ✅                | ✅ |
> | NumericUpDown | `"decimals"`          | `setDecimals(int)`            | ✅                | ✅ |
> | TreeView      | `"font-size"`         | `setFontSize(int)`            | ✅                | ✅ |
> | Image        | `"alpha"`            | `setAlpha(int)`（0-255，默认 255）            | ✅                | ✅ |

### 6.6 Float 属性表

> `setFloatProperty` 桥接状态：`✅` = 已通过 C ABI `SetFloat` 可用，`❌` = 仅有 C++ setter，未桥接
>
>
> | 控件          | 属性名                  | setter/说明                      | setFloatProperty  | getFloatProperty |
> | ------------- | ----------------------- | -------------------------------- | ---------------- |--------|
> | Label         | `"line-spacing-ratio"`  | `setLineSpacingRatio(float)`     | ✅                  | ✅ |
> | CheckBox      | `"size-ratio"`          | `setSizeRatio(float)`            | ✅                  | ✅ |
> | Slider        | `"step"`                | `setStep(float)`                 | ✅                  | ✅ |
> | Slider        | `"value"`               | `setValue(float)`                | ✅                  | ✅ |
> | Slider        | `"track-thickness"`     | `setTrackThickness(float)`       | ✅                  | ✅ |
> | Slider        | `"thumb-size"`          | `setThumbSize(float)`            | ✅                  | ✅ |
> | Slider        | `"tick-interval"`       | `setTickInterval(float)`         | ✅                  | ✅ |
> | Slider        | `"tick-length"`         | `setTickLength(float)`           | ✅                  | ✅ |
> | Slider        | `"label-gap"`           | `setLabelGap(float)`             | ✅                  | ✅ |
> | Slider        | `"range-min"`           | `setRange(min, max) — 拆自双参` | ✅                  | ✅ |
> | Slider        | `"range-max"`           | `setRange(min, max) — 拆自双参` | ✅                  | ✅ |
> | ProgressBar   | `"animation-speed"`     | `setAnimationSpeed(float)`       | ✅                  | ✅ |
> | ProgressBar   | `"value"`               | `setValue(float)`                | ✅                  | ✅ |
> | ProgressBar   | `"range-min"`           | `setRange(min, max) — 拆自双参` | ✅                  | ✅ |
> | ProgressBar   | `"range-max"`           | `setRange(min, max) — 拆自双参` | ✅                  | ✅ |
> | TextArea      | `"scrollbar-thickness"` | `setScrollBarThickness(float)`   | ✅                  | ✅ |
> | ScrollBar     | `"value"`               | `setValue(float)`                | ✅                  | ✅ |
> | ScrollBar     | `"page-size"`           | `setPageSize(float)`             | ✅                  | ✅ |
> | ScrollBar     | `"step-size"`           | `setStepSize(float)`             | ✅                  | ✅ |
> | ScrollBar     | `"thickness"`           | `setThickness(float)`            | ✅                  | ✅ |
> | ScrollBar     | `"range-min"`           | `setRange(min, max) — 拆自双参` | ✅                  | ✅ |
> | ScrollBar     | `"range-max"`           | `setRange(min, max) — 拆自双参` | ✅                  | ✅ |
> | WinFrame      | `"edge-margin"`         | `setEdgeMargin(float)`           | ✅                  | ✅ |
> | MenuBar       | `"bar-height"`          | `setBarHeight(float)`            | ✅                  | ✅ |
> | MenuBar       | `"item-height-ratio"`   | `setItemHeightRatio(float)`      | ✅                  | ✅ |
> | MenuBar       | `"font-size"`           | `setFontSize(float)`             | ✅                  | ✅ |
> | ComboBox      | `"arrow-width"`         | `setArrowWidth(float)`           | ✅                  | ✅ |
> | ComboBox      | `"item-height"`         | `setItemHeight(float)`           | ✅                  | ✅ |
> | Dialog        | `"button-height"`       | `setButtonHeight(float)`         | ✅                  | ✅ |
> | Dialog        | `"button-gap"`          | `setButtonGap(float)`            | ✅                  | ✅ |
> | Dialog        | `"padding"`             | `setPadding(float)`              | ✅                  | ✅ |
> | TreeView      | `"indent-width"`        | `setIndentWidth(float)`          | ✅                  | ✅ |
> | TreeView      | `"row-height"`          | `setRowHeight(float)`            | ✅                  | ✅ |
> | TreeView      | `"line-spacing"`        | `setLineSpacing(float)`          | ✅                  | ✅ |
> | TreeView      | `"arrow-gap"`           | `setArrowGap(float)`             | ✅                  | ✅ |
> | Splitter      | `"thickness"`           | `setThickness(float)`            | ✅                  | ✅ |
> | Splitter      | `"ratio"`               | `setSplitRatio(float)`           | ✅                  | ✅ |
> | Splitter      | `"first-min"`           | `m_minFirst`（SetFloat 直接设） | ✅                  | ✅ |
> | Splitter      | `"second-min"`          | `m_minSecond`（SetFloat 直接设）| ✅                  | ✅ |
> | ColorPicker   | `"closed-swatch-size"`  | `setClosedSwatchSize(float)`     | ✅                  | ✅ |
> | NumericUpDown | `"value"`               | `setValue(double)`               | ✅                  | ✅ |
> | NumericUpDown | `"step"`                | `setStep(double)`                | ✅                  | ✅ |
> | NumericUpDown | `"page-step"`           | `setPageStep(double)`            | ✅                  | ✅ |
> | NumericUpDown | `"button-width"`        | `setButtonWidth(float)`          | ✅                  | ✅ |
> | NumericUpDown | `"range-min"`           | `setRange(min, max) — 拆自双参` | ✅                  | ✅ |
> | NumericUpDown | `"range-max"`           | `setRange(min, max) — 拆自双参` | ✅                  | ✅ |

### 6.7 String 属性表

> `setStringProperty` 桥接状态：`✅` = 已通过 C ABI `SetString` 可用，`❌` = 仅有 C++ setter，未桥接
>
>
> | 控件        | 属性名             | setter/说明                    | setStringProperty               | getStringProperty |
> | ----------- | ------------------ | ------------------------------ | ------------------------------ |--------|
> | Button      | `"caption"`        | `setCaption(string)`           | ✅                                | ❌ |
> | Button      | `"animation"`      | JSONC 动画文件路径，加载 LuotiAni 并播放 | ✅ | ❌ |
> | Label       | `"caption"`        | `setCaption(string)`           | ✅                                | ✅ |
> | EditBox     | `"text"`           | `setText(string)`              | ✅                                | ✅ |
> | EditBox     | `"placeholder"`    | `setPlaceholder(string)`       | ✅                                | ✅ |
> | ComboBox    | `"placeholder"`    | `setPlaceholder(string)`       | ✅(继承自 EditBox)                | ❌ |
> | ComboBox    | `"selected-value"` | `setSelectedValue(string)`     | ❌                                | ✅ |
> | ComboBox    | `"items"`          | JSON 字符串格式：`[{"label":"..","value":".."}]` | ✅ | ❌ |
> | ProgressBar | `"custom-text"`    | `setCustomText(string)`        | ✅                                | ✅ |
> | Slider      | `"label-format"`   | `setLabelFormat(string)`       | ✅                                | ✅ |
> | MenuItem    | `"caption"`        | `setCaption(string)`           | ✅                                | ✅ |
> | TreeView    | `"expand"`         | 按 nodeId 展开单个节点         | ✅                                | ❌ |
> | TreeView    | `"collapse"`       | 按 nodeId 折叠单个节点         | ✅                                | ❌ |
> | MenuItem    | `"shortcut"`       | `setShortcut(string)`          | ✅                                | ✅ |
> | WinFrame    | `"title"`          | `setTitle(const string&)`      | ✅                                | ❌ |
> | Dialog      | `"confirm-text"`   | `setConfirmButtonText(string)` | ✅(通过`SetConfirmButtonText`)    | ✅ |
> | Dialog      | `"cancel-text"`    | `setCancelButtonText(string)`  | ✅(通过`SetCancelButtonText`)     | ✅ |
> | ColorPicker | `"color"`          | `setColor(string) — hex`      | ✅                                | ❌ |
> | Image      | `"image"`          | `setStringProperty → loadFromFile（相对路径经基路径拼接）` | ✅ | ❌（只写不读，fs::path string() 悬垂） |
> | Image      | `"image-resource"` | `setStringProperty → loadFromResource` | ✅ | ❌（只写不读） |

### 6.8 Enum 属性表

> 使用 `SetEnum` 接口，传枚举值名称字符串，不区分大小写。`setEnumProperty` 桥接状态：`✅` = 已通过 C ABI `SetEnum` 可用，`❌` = 仅有 C++ setter，未桥接
>
>
> | 控件        | 属性名             | setter/说明 | setEnumProperty | 枚举值                                                                                                                                               | getEnumProperty |
> | ----------- | ------------------ | ----------- | --------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------  | -------- |
> | Label       | `"align"`          | 对齐模式    | ✅              | `am-top-left`, `am-mid-left`, `am-bottom-left`, `am-top-right`, `am-mid-right`, `am-bottom-right`, `am-top-center`, `am-center`, `am-bottom-center` | ✅ |
> | EditBox     | `"align"`          | 对齐模式    | ✅              | 同上 | ✅ |
> | ProgressBar | `"align"`          | 对齐模式    | ✅              | 同上 | ✅ |
> | CheckBox    | `"check-state"`    | 勾选状态    | ✅              | `unchecked`, `checked`, `indeterminate` | ✅ |
> | CheckBox    | `"style"`          | 复选框样式  | ✅              | `classic`, `cross`, `circle` | ✅ |
> | CheckBox    | `"layout"`         | 文字布局    | ✅              | `text-right`, `text-left` | ✅ |
> | CheckBox    | `"vertical-align"` | 垂直对齐    | ✅              | `center`, `top`, `bottom` | ✅ |
> | ProgressBar | `"style"`          | 进度条方向  | ✅              | `horizontal`, `vertical` | ✅ |
> | ProgressBar | `"text-mode"`      | 文字模式    | ✅              | `none`, `percent`, `custom` | ✅ |
> | ProgressBar | `"font"`           | 字体名      | ✅              | 28 字体 | ✅ |
> | Slider      | `"style"`          | 滑块方向    | ✅              | `horizontal`, `vertical` | ✅ |
> | Slider      | `"label-font"`     | 标签字体名  | ✅              | 28 字体 | ✅ |
> | ScrollBar   | `"orientation"`    | 滚动条方向  | ✅              | `vertical`, `horizontal` | ✅ |
> | Label       | `"font"`           | 字体名      | ✅              | 28 字体 | ✅ |
> | EditBox     | `"font"`           | 字体名      | ✅              | 28 字体 | ✅ |
> | TreeView    | `"font"`           | 字体名      | ✅              | 28 字体 | ✅ |
> | Popup       | `"centered-mode"`  | 弹窗定位模式 | ✅ | `centered` | ❌ |
> | Image       | `"scale-type"`    | 缩放方式    | ✅ | `stretch`, `fit-center`, `center-crop`, `none`（不区分大小写） | ✅ |
> | Image      | `"anchor"`         | 锚点（9 种，同 Label `align` 值域：`top-left`/`top-center`/`top-right`/`mid-left`/`center`/`mid-right`/`bottom-left`/`bottom-center`/`bottom-right`，无 `am-` 前缀） | ✅ | 同上 | ✅ |

### 6.9 Callback 事件表

> 使用 `SetCallback` 接口，事件名不加 "on" 前缀。


| 控件          | 事件名                | C++ 回调签名                                              | 事件数据                                           |
| ------------- | --------------------- | --------------------------------------------------------- | -------------------------------------------------- |
| Button        | `"click"`             | `void(shared_ptr<Button>)`                                | 无                                                 |
| Label         | `"click"`             | `void(shared_ptr<Label>)`                                 | 无                                                 |
| EditBox       | `"enter"`             | `void(shared_ptr<Control>)`                               | 无                                                 |
| EditBox       | `"text-changed"`      | `void(shared_ptr<Control>, string)`                       | `.data.strVal`                                     |
| TextArea      | `"text-changed"`      | `void(shared_ptr<Control>, string)`                       | `.data.strVal`                                     |
| Popup         | `"close"`             | `void(shared_ptr<Popup>, DialogResult)`                   | `.data.intVal`（DialogResult 映射）                |
| ConfirmPopup  | `"confirm"`           | `void(shared_ptr<ConfirmPopup>)`                          | 无                                                 |
| Dialog        | `"cancel"`            | `void(shared_ptr<Dialog>)`                                | 无                                                 |
| CheckBox      | `"check-changed"`     | `void(shared_ptr<CheckBox>, CheckState, CheckState)`      | `.data.intVal`（新 CheckState）                    |
| Slider        | `"value-changed"`     | `void(shared_ptr<Slider>, float)`                         | `.data.floatVal`                                   |
| ProgressBar   | `"value-changed"`     | `void(shared_ptr<ProgressBar>, float, float)`             | `.data.floatVal`                                   |
| ScrollBar     | `"position-changed"`  | `void(shared_ptr<ScrollBar>, float, float, float, float)` | `.data.floatVal`                                   |
| ComboBox      | `"selection-changed"` | `void(shared_ptr<ComboBox>, int, const string&)`          | `.data.selection`（`.idx` + `.val`）               |
| NumericUpDown | `"value-changed"`     | `void(shared_ptr<NumericUpDown>, double)`                 | `.data.doubleVal`                                  |
| Splitter      | `"moved"`             | `void(shared_ptr<Splitter>, float)`                       | `.data.floatVal`                                   |
| ColorPicker   | `"color-changed"`     | `void(shared_ptr<ColorPicker>, const SColor&)`            | （颜色暂不通过 C ABI 回调传回，用`GetColor` 轮询） |
| TreeView      | `"select"`            | `void(const string&)`                                     | `.data.strVal`（节点 id）                          |
| TreeView      | `"expand"`            | `void(const string&)`                                     | `.data.strVal`                                     |
| TreeView      | `"collapse"`          | `void(const string&)`                                     | `.data.strVal`                                     |
| MenuItem      | `"click"`             | `void(shared_ptr<MenuItem>)`                              | 无                                                 |

### 6.10 Ptr 属性表

> `setPtrProperty` / `getPtrProperty` 桥接状态：`✅` = 已通过 C ABI `SetPtr` / `GetPtr` 可用
>
> value 类型为 `void*`，实际为 `UIControlHandle`（即 `Control*`）。调用方保证指针在控件生命周期内有效。
>
> | 控件        | 属性名                  | setter/说明                                   | `setPtrProperty` | `getPtrProperty` |
> | ----------- | ----------------------- | --------------------------------------------- | ---------------- | ---------------- |
> | Popup       | `"content"`             | `setContent(shared_ptr<ControlImpl>)`         | ✅               | ❌ |
> | Splitter    | `"first-linked"`        | 设置首个关联控件（与 second-linked 配套）     | ✅               | ❌ |
> | Splitter    | `"second-linked"`       | 设置第二个关联控件                            | ✅               | ❌ |
> | TreeView    | `"selected-user-data"`  | 读写当前选中节点的 `userData` 指针           | ✅               | ✅ |
> | TreeView    | `"item-leading-control"`| 读写 `item-id` 定位节点的行前置控件容器句柄（借用语义，生命周期调用方保证） | ✅ | ✅ |

> TreeView item 级属性（**item-id 定位模式**：先 `SetString("item-id", id)` 定位，随后读写作用于该节点）：
> `"item-leading-gap"`（Float，容器与文本间隔，默认 6）、`"item-font-size"`（Int，0=继承）、
> `"item-font"`（Enum，逐 Item 字体，如 `"maplemono-nf-cn-regular"`）、
> `"item-id"`（String，定位/查询当前目标）。对应 JSON 键：`leadingGap` / `size` / `font` / item 内 `leadingControl` 对象（复用控件 JSON）。

---

## 7. 实施清单

### Phase 1 — Color 基类（已完成）

- [X]  Control 基类 `setColorProperty` / `setStateColorProperty` 虚方法
- [X]  ControlImpl 通用属性实现 (background/border/text/text-shadow)
- [X]  TreeView override `setColorProperty`

### Phase 2 — 控件特有 Color（已完成）

- [X]  Slider override `setColorProperty`
- [X]  ComboBox override `setColorProperty`
- [X]  CheckBox override `setColorProperty`
- [X]  ProgressBar override `setColorProperty`
- [X]  Splitter override `setColorProperty`
- [X]  WinFrame override `setColorProperty`
- [X]  ColorPicker override `setColorProperty`
- [X]  NumericUpDown override `setColorProperty`

### Phase 3 — Int/Float/Bool/String/Enum（已完成）

- Phase 2 全部 8 控件（Slider/ComboBox/CheckBox/ProgressBar/Splitter/WinFrame/ColorPicker/NumericUpDown）的 `setIntProperty` / `setFloatProperty` / `setBoolProperty` / `setStringProperty` / `setEnumProperty` override。
- **ScrollBar**: `setColorProperty`（track/thumb/thumb-hover/thumb-pressed）+ `setFloatProperty`（value/range-min/range-max/page-size/step-size/scrollbar-thickness）+ `setEnumProperty`（orientation: vertical/horizontal）
- **MenuBar**: `setFloatProperty`（item-height-ratio/value）+ `setEnumProperty`（font）

**注意 — MenuBar 静态方法改造**：
`MenuBar::setItemHeightRatio(float)` 和 `MenuBar::setFontSize(float)` 已由 `static` 改为实例方法，添加了实例成员变量，更新了现有调用点（LayoutParser.cpp、test_menu.cpp）。已实现 `setFloatProperty("item-height-ratio")` / `setFloatProperty("value")` 及 `setEnumProperty("font")`。

**注意 — `setRange(min, max)` 拆分**：
ProgressBar / Slider / ScrollBar 的 `setRange(min, max)` 双参方法拆为 `"range-min"` 和 `"range-max"` 两个属性，每个属性只设一个值。（已实现：Slider / ProgressBar / NumericUpDown / ScrollBar 按此方式处理）

- **Phase 3 扩展 — 全控件覆盖**：`Button`（bool/string）、`Label`（bool/int/float/string/enum）、`EditBox`（bool/int/string/enum）、`TextArea`（bool/int/float）、`Dialog`（bool/float/string）、`TreeView`（bool/int/float/enum）、`MenuItem`（bool/string）、`ProgressBar`（align enum 补加）— 所有通用型属性桥接完成

### Phase 4 — Getter C ABI（已完成）

- [X]  Control 基类新增 7 个 `get*Property` 虚方法（默认返回 0）
- [X]  ControlImpl 实现通用属性 Getter（从已有的 StateColor getter 读取）
- [X]  7 个 C ABI Getter 函数声明 + 实现 (`GetColor`/`GetStateColor`/`GetBool`/`GetInt`/`GetFloat`/`GetString`/`GetEnum`)
- [X]  Phase 2/3 全部 10 控件（Slider/ComboBox/CheckBox/ProgressBar/Splitter/WinFrame/ColorPicker/NumericUpDown/ScrollBar/MenuBar）实现 `get*Property` override，与对应 setter 对称

### Phase 5 — Callback C ABI（已完成）

- [X]  `UIEventData` 结构体定义 + `UIEventCallback` 类型
- [X]  Control 基类 `setCallbackProperty` 虚方法
- [X]  `UICornerstone_SetCallback` C ABI 函数声明 + 实现
- [X]  ControlImpl 新增通用回调存储 `m_cCallbacks` 映射表 + `fireCCallback()` 辅助方法
- [X]  6 控件实现 Callback 绑定：Slider(`value-changed`)、CheckBox(`check-changed`)、ComboBox(`selection-changed`)、Splitter(`position-changed`)、ScrollBar(`position-changed`)、NumericUpDown(`value-changed`)
- [X]  3 后端 (SDL3/SFML/Raylib) 全部编译通过，0 错误 0 警告

### Phase 6 — 移除旧版专用 C ABI 导出（已完成）

- [X]  将 48 个旧专用导出（`SetBGColor`/`SetText`/`SetOnClick`/`GetSliderValue`/`SetNumericUpDownValue` 等）替换为统一的属性系统 API (`SetColor`/`SetString`/`SetCallback`/`GetFloat` 等)
- [X]  保留 16 个非属性可映射的导出（`Show`/`Close`/`ExpandNode`/`SetComboItems`/`SetSplitterLinkedControls` 等）
- [X]  更新 6 个测试文件（`test_fromsource_cabi`/`test_dialog_cabi`/`test_combobox_cabi`/`test_splitter_cabi`/`test_numericupdown_cabi`/`test_treeview_cabi`）使用新 API
- [X]  从 `UICornerstoneAPI.h` 移除旧导出声明
- [X]  从 `UICornerstoneAPI.cpp` 移除旧导出实现
- [X]  3 后端 × 8 C ABI 测试目标 = 24 构建全部通过，0 错误

### Phase 7 — Ptr 属性类型 + 控件特有属性扩展（已完成）

- [X]  Control 基类新增 `setPtrProperty` / `getPtrProperty` 虚方法（默认返回 0）
- [X]  ControlImpl 默认实现（返回 0）
- [X]  C ABI `SetPtr` / `GetPtr` 函数声明（`UICornerstoneAPI.h`）+ 实现（`UICornerstoneAPI.cpp`）
- [X]  Popup override `setPtrProperty("content")` — 将 UIControlHandle 转为 shared_ptr\<ControlImpl\> 后调用 `setContent`
- [X]  Popup override `setEnumProperty("centered-mode")` — 支持 `"centered"` 模式
- [X]  Splitter override `setPtrProperty("first-linked"/"second-linked")` + 新增 `setFirstControl`/`setSecondControl` 方法，支持逐个设置关联控件
- [X]  Splitter 扩展 `setFloatProperty("first-min"/"second-min")` + getter — 替代旧 `SetSplitterMinSize`
- [X]  TreeView override `setStringProperty("expand"/"collapse")` — 委托给 `expandNode`/`collapseNode`
- [X]  TreeView 扩展 `setBoolProperty("expand-all"/"collapse-all")` — 委托给 `expandAll`/`collapseAll`
- [X]  TreeView override `getPtrProperty("selected-user-data")` — 返回选中节点 `userData` 指针
- [X]  ComboBox override `setStringProperty("items")` — JSON 解析后调用 `setItems`
- [X]  Button 扩展 `setStringProperty("animation")` — 加载 JSONC 动画文件并播放
- [X]  全库编译通过，`test_property_cabi` 76/76 测试通过

### 实现模式

```cpp
// xxx.cpp
int Slider::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, "step") == 0)             { setStep(value);             return 1; }
    if (strcmp(prop, "track-thickness") == 0)  { setTrackThickness(value);   return 1; }
    if (strcmp(prop, "thumb-size") == 0)       { setThumbSize(value);        return 1; }
    if (strcmp(prop, "tick-interval") == 0)    { setTickInterval(value);     return 1; }
    if (strcmp(prop, "tick-length") == 0)      { setTickLength(value);       return 1; }
    if (strcmp(prop, "label-gap") == 0)        { setLabelGap(value);         return 1; }
    return ControlImpl::setFloatProperty(prop, value);
}
```

---

### 测试策略

属性系统覆盖 UI 层全部控件的全部属性，需要一个独立的测试体系。设计原则：


| 原则               | 说明                                                                                                      |
| ------------------ | --------------------------------------------------------------------------------------------------------- |
| **C ABI 集成测试** | 测试以`test_*_cabi.cpp` 形式通过 DLL 编译，LoadLibrary + GetProcAddress 调用 C ABI 函数，模拟真实使用场景 |
| **Set/Get 对称性** | 对每个属性 Set 后立即 Get，验证返回值一致                                                                 |
| **非侵入**         | 不创建视觉窗口，纯逻辑验证（初版用视觉窗口+人工验，回归后改为无窗口断言）                                 |

#### 测试用例设计

```
test_property_cabi.cpp          ← 通用属性系统测试
  ├─ Null/invalid 边界
  │   ├─ SetColor(NULL, "x", val)       → 0
  │   ├─ GetColor(ctl, NULL, &out)      → 0
  │   └─ GetColor(ctl, "unknown", &out) → 0
  ├─ Set/Get 对称（ControlImpl 通用属性）
  │   ├─ SetStateColor(ctl, "background", rgba)  → 1
  │   ├─ GetStateColor(ctl, "background", &out)  → 1, out == rgba
  │   ├─ SetBool(ctl, "visible", 0)              → 1
  │   ├─ GetBool(ctl, "visible", &out)           → 1, out == 0
  │   ├─ SetInt(ctl, "font-size", 16)            → 1
  │   ├─ GetInt(ctl, "font-size", &out)          → 1, out == 16
  │   ├─ SetFloat(ctl, "step", 1.0f)             → 1
  │   ├─ GetFloat(ctl, "step", &out)             → 1, out ≈ 1.0f
  │   ├─ SetString(ctl, "caption", "hello")      → 1
  │   ├─ GetString(ctl, "caption", buf, 64)      → 1, buf == "hello"
  │   ├─ SetEnum(ctl, "style", "horizontal")     → 1
  │   └─ GetEnum(ctl, "style", buf, 64)          → 1, buf == "horizontal"
  ├─ 控件特有属性
  │   ├─ SetColor(ctl, "selected", red)         → 1  (TreeView)
  │   └─ SetColor(ctl, "track", blue)           → 1  (Slider)
  └─ 回调
      └─ SetCallback(ctl, "click", handler, ud) → 1

test_combobox_cabi.cpp          ← 已有，扩展 Set/Get 属性验证
test_numericupdown_cabi.cpp     ← 已有，扩展 Set/Get 属性验证
test_splitter_cabi.cpp          ← 已有，扩展 Set/Get 属性验证
test_treeview_cabi.cpp          ← 已有，扩展 SetColor/SetString 验证
```

#### 自动验证（无窗口模式）

对回调和数据驱动的属性（非视觉依赖），无需创建窗口：

```cpp
void* ctl = uiFindControl("myTree");
int ok;

// --- Color ---
ok = UICornerstone_SetColor(ctl, "selected", (UIColor){255,0,0,255});
assert(ok == 1);
UIColor c;
ok = UICornerstone_GetColor(ctl, "selected", &c);
assert(ok == 1 && c.r == 255 && c.g == 0 && c.b == 255);

// --- Bool ---
ok = UICornerstone_SetBool(ctl, "cycle-navigation", 1);
assert(ok == 1);
int b;
ok = UICornerstone_GetBool(ctl, "cycle-navigation", &b);
assert(ok == 1 && b == 1);

// --- String ---
ok = UICornerstone_SetString(ctl, "caption", "hello");
assert(ok == 1);
char buf[64];
ok = UICornerstone_GetString(ctl, "caption", buf, sizeof(buf));
assert(ok == 1 && strcmp(buf, "hello") == 0);
```

#### 视觉验证（窗口模式）

颜色、布局等视觉属性仍需窗口观察。策略：创建一个包含多个控件的布局，程序化逐一 Set 属性，人工目视验证。回归后在 CI 中用截图对比自动化。

#### 测试清单

- [ ]  `test_property_cabi.cpp` — 通用属性 + Set/Get 对称 + 边界条件
- [ ]  扩展现有 `test_*_cabi.cpp` — 各控件特有属性验证
- [ ]  程序化布局（非 JSON）— `UICornerstone_CreateControl` + 属性设置（若已有则复用）

---

## 8. 扩展指南

### 8.1 新增通用属性

修改 `ControlImpl::setColorProperty` / `setStateColorProperty` 加一行 `strcmp`。

### 8.2 控件类型查询（UICornerstone_GetControlType）

- **机制**：基类 `Control` 持有 `protected: ControlType m_ctlType = None`（`enum class ControlType`，
  ControlBase.h，与 JSON `"type"` 值一一对应），**子类构造函数体内设置**（如
  `m_ctlType = ControlType::Label;`），基类 `getControlType()` 直接返回——O(1)，零 dynamic_cast。
- **字符串**：CABI `UICornerstone_GetControlType` 内 `switch` 枚举转 `PropertyNames::kControlType*`
  常量（禁止字面量）；`None` 或未识别返回 0。
- **注意**：子类若已有 `m_type` 私有成员（如 MenuItem 的 `MenuItemType`），基类成员用
  `m_ctlType` 避免被派生成员阴影；新增控件只需在构造函数设置枚举 + switch 加一行。
- **特殊类型**：image-button 本质为 button → 返回 `"button"`；`"menu-item"` / `"menu-panel"` /
  `"handle-control"` 仅查询返回（JSON 无对应 type）。

### 8.2 新增控件特有属性

1. 在控件类中实现 setter 方法
2. Override `setColorProperty`（或其他类型对应虚方法）
3. 加 `strcmp` case 处理属性名，fallback 到基类

### 8.3 新增值类型

如果需要新的值类型（如 `bool`），请在 `Control` 中加对应的虚方法，C ABI 加对应的入口函数，并在实现中 fallback 到基类空实现。

---

## 附录：控件颜色属性分布


| 控件          | bg         | border     | text   | textShadow | 特有颜色                                                                     |
| ------------- | ---------- | ---------- | ------ | ---------- | ---------------------------------------------------------------------------- |
| Button        | 4态        | 4态        | 4态*   | 4态*       | —                                                                           |
| Label         | 4态        | 4态        | 4态    | 4态        | —                                                                           |
| EditBox       | 4态        | 4态        | 4态    | —         | —                                                                           |
| TextArea      | 4态        | 4态        | 4态    | —         | —                                                                           |
| CheckBox      | 4态        | 4态        | 4态    | —         | check, cross, indeterminate, boxBorder                                       |
| ProgressBar   | 4态        | 4态        | 4态    | —         | progress                                                                     |
| Slider        | 4态        | 4态        | —     | —         | track, trackFill, thumb, thumbBorder, thumbHover, tick, label                |
| ScrollBar     | 4态        | 4态        | —     | —         | —                                                                           |
| Splitter      | 4态        | 4态        | —     | —         | line(normal+hover+drag)                                                      |
| Panel         | 4态        | 4态        | —     | —         | —                                                                           |
| WinFrame      | 4态        | 4态        | —     | —         | winFrameBG, winFrameBorder, titleBarBG, titleText                            |
| ColorPicker   | 4态        | 4态        | —     | —         | closedText, popupBG                                                          |
| ComboBox      | 4态        | 4态        | 4态    | —         | arrow, arrowHover, itemSelected, itemHover, itemDisabled, listBg, listBorder |
| NumericUpDown | 4态        | 4态        | 4态    | —         | arrow(normal+hover+press)                                                    |
| MenuItem      | 4态        | —         | 4态    | —         | —                                                                           |
| MenuBar       | 4态        | —         | 4态    | —         | —                                                                           |
| TreeView      | 4态+SColor | 4态+SColor | SColor | —         | selected, hover                                                              |

> `*` = Button 重写了 `setTextStateColor` / `setTextShadowStateColor` 以同步到内部 caption Label。
> `4态` = StateColor (Normal/Hover/Pressed/Disabled)，`SColor` = 简单单色。

---

## 变更注记（2026-08-14）

Label 新增两个 String 属性（走 `Control::setStringProperty`，`GetString` 只写不读，未设置时返回空串）：

| 属性（kJson 键） | 说明 |
|------------------|------|
| `font-resource`（`kJsonFontResource`="fontResource"） | 内存字体资源 ID，经 MemoryResourceProvider 注册（`RegisterResource`/`AdoptResource`），优先级低于 `font-file` |
| `font-file`（`kJsonFontFile`="fontFile"） | 任意字体文件路径（相对 resourceRoot，或 `provider:` 前缀引用内存资源），覆盖 `font` 枚举 |

三形态互斥（详见 ResourceProvider_Design.md §5.2）：`font` 枚举 < `font-file` 路径 < `font-resource` 内存 ID。文件路径一律**在字符串层判定前缀**（`provider:`），不得经 `fs::path::is_relative` 判断——MSVC 会把带前缀的字符串当作相对路径并拼接 basePath 污染前缀。
