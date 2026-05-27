# 配置文件化布局系统 — 设计文档

## 1. 概述

当前所有 UI 控件的创建、定位、属性设置均通过 C++ 硬编码完成（参见各测试文件如 `test_label.cpp`、`test_checkbox.cpp`）。这种方式存在布局与逻辑耦合、无法热更新、复用性差、无法对接可视化编辑器等问题。

本系统旨在通过 JSON 配置文件描述 UI 布局，运行时由 LayoutParser 解析并自动构建控件树，实现布局与代码逻辑的解耦。

## 2. 总体架构

```
布局配置文件 (layouts/*.json)
        ↓  (文件读取)
   LayoutParser (解析器)
    ├─ parseLayoutFile(path)   — 从文件加载
    ├─ parseLayout(string)     — 从字符串加载
    ├─ 控件工厂
    │   └─ parseControl(json, parent) → 根据 type 分派
    │        ├─ parseLabel    → make_shared<Label>
    │        ├─ parseButton   → make_shared<Button>
    │        ├─ parseEditBox  → make_shared<EditBox>
    │        └─ parsePanel    → make_shared<Panel>
    ├─ 属性解析工具集
    │   ├─ parseRect / parseMargin / parseColor
    │   ├─ parseStateColor / parseFont
    │   ├─ parseCommonProperties
    │   └─ parseEvents
    ├─ 事件绑定
    │   ├─ registerHandler(name, func)  — 注册处理器
    │   └─ findControlById(id)          — 查找控件
    └─ 内部状态
        ├─ m_controlsById     — ID → 控件映射
        ├─ m_handlers         — handler 名称 → 函数映射
        ├─ m_currentLineNo    — 当前解析的 JSON 行号 (nlohmann/json 的 get_token_start_location)
        └─ m_currentJsonPath  — 当前解析的 JSON 路径 (如 "controls[2].children[1].rect")
             ↓
       控件树 (Control Tree)
        ├─ Panel (顶层容器)
        │   ├─ Label
        │   ├─ Button
        │   ├─ EditBox
        │   └─ Panel (子容器)
        │       └─ Label
        └─ SDL 渲染循环 (不变)
```

## 3. 阶段规划

### Phase 1 — 基础解析与绝对定位


| 功能                  | 说明                                              |
| --------------------- | ------------------------------------------------- |
| LayoutParser 基本框架 | 解析 JSON 入口，递归构建控件树                    |
| 绝对定位              | JSON 中`rect` 字段 → `SRect`，直接 `setRect()`   |
| 控件工厂              | 按`type` 字符串创建 Label/Button/EditBox/Panel    |
| 颜色解析              | 支持`"#RRGGBBAA"` 和 `{"r","g","b","a"}` 两种格式 |
| 事件绑定方式1         | JSON 声明 +`registerHandler()` 自动绑定           |
| 事件绑定方式2         | `findControlById()` 获取引用后手动绑定            |
| 支持类型              | Label, Button, EditBox, Panel                     |
| 跳过特性              | Button 跳过 Actor/LuotiAni（后续 Phase 加入）     |

### Phase 2 — 高级布局

- HFlow / VFlow 流式布局引擎
- Anchor 锚点布局引擎
- Grid 网格布局引擎
- 百分比尺寸支持（如 `"w": "50%"`）
- 嵌套布局（容器内子控件使用不同布局类型）
- 窗口 resize → 布局引擎重新计算

### Phase 3 — 高级特性

- 热加载（文件变化监听 → 自动重建控件树）
- 全局主题系统（theme 继承与覆盖）
- 数据绑定（控件属性绑定到数据源）
- 全控件类型支持（CheckBox, ProgressBar, ScrollBar, Menu, Dialog, TextArea, Actor, LuotiAni, TextArea）

## 4. 配置文件格式设计

### 4.1 JSON Schema 顶层结构

```jsonc
{
    // ===== 元信息 =====
    "version": "1.0",                         // 配置文件版本，用于兼容性检测

    // ===== 全局视口 (Phase 2 全量启用) =====
    "viewport": { "width": 1920, "height": 1080 },

    // ===== 全局主题 (Phase 3 启用继承) =====
    "theme": {
        "fonts": {
            "default": "HarmonyOS_Sans_SC_Regular",
            "size": 16,
            "style": "NORMAL"
        },
        "colors": {
            "background": "#282828FF",
            "text":       "#FFFFFFFF",
            "border":     "#808080FF",
            "textShadow": "#00000000"
        }
    },

    // ===== 控件树根节点 =====
    "controls": [ /* 控件定义数组 */ ]
}
```

### 4.2 通用控件属性

每个控件对象支持以下通用属性：


| 字段       | 类型   | 必填   | 默认值                                    | 说明                                                                                              |
| ---------- | ------ | ------ | ----------------------------------------- | ------------------------------------------------------------------------------------------------- |
| `type`     | string | **是** | —                                        | 控件类型标识符，受支持的值见 4.5                                                                  |
| `id`       | string | 否     | `""`                                      | 全局唯一标识（字符串），用于`findControlById` 和事件绑定。与 C++ 侧 `setId(int)` 的关系见第 11 节 |
| `rect`     | object | **是** | —                                        | `{"x", "y", "w", "h"}` 绝对定位 (Phase 1)                                                         |
| `scale`    | object | 否     | `{"x": 1.0, "y": 1.0}`                    | 本地缩放倍数                                                                                      |
| `visible`  | bool   | 否     | `true`                                    | 是否可见                                                                                          |
| `enabled`  | bool   | 否     | `true`                                    | 是否启用                                                                                          |
| `margin`   | object | 否     | `{"left":0,"top":0,"right":0,"bottom":0}` | 边距                                                                                              |
| `colors`   | object | 否     | 使用 ConstDef 默认值                      | 状态颜色集                                                                                        |
| `events`   | object | 否     | `{}`                                      | 事件绑定声明                                                                                      |
| `children` | array  | 否     | `[]`                                      | 子控件列表（仅容器类型支持）                                                                      |

### 4.3 颜色表示

#### 格式 1：十六进制字符串

十六进制颜色字符串包含 `#` 前缀 + 十六进制数字，支持三种长度：


| 格式        | 总字符数 | 示例          | 说明                 | 结果 SDL_Color  |
| ----------- | -------- | ------------- | -------------------- | --------------- |
| `#RGB`      | 4 字符   | `"#F00"`      | 每位重复 →`#RRGGBB` | `{255,0,0,255}` |
| `#RRGGBB`   | 7 字符   | `"#FF0000"`   | 标准 RGB             | `{255,0,0,255}` |
| `#RRGGBBAA` | 9 字符   | `"#FF000080"` | RGB + Alpha          | `{255,0,0,128}` |

- 必须以 `#` 开头
- 不区分大小写（`"#ff0000"` 等同于 `"#FF0000"`）

#### 格式 2：对象格式

```jsonc
{ "r": 255, "g": 0, "b": 0 }           // a 省略时默认为 255
{ "r": 255, "g": 0, "b": 0, "a": 128 } // 完整格式
```

- `r`, `g`, `b`, `a` 取值范围 0–255
- `a` 可选，默认 255

#### 例外处理

- JSON 中该字段为 `null` 或缺失 → 使用系统默认值
- 格式无法解析 → SDL_Log 警告 + 使用系统默认值（不崩溃）

### 4.4 状态颜色结构

**四种颜色类型**：


| 类型         | 对应 ConstDef 默认值                  | 作用           |
| ------------ | ------------------------------------- | -------------- |
| `background` | `DEFAULT_NORMAL_COLOR` 等             | 控件背景填充色 |
| `border`     | `DEFAULT_BORDER_NORMAL_COLOR` 等      | 控件边框色     |
| `text`       | `DEFAULT_TEXT_NORMAL_COLOR` 等        | 文字颜色       |
| `textShadow` | `DEFAULT_TEXT_SHADOW_NORMAL_COLOR` 等 | 文字阴影颜色   |

**每种颜色类型的四状态结构**：

```jsonc
"background": {
    "normal":   "#404040FF",
    "hover":    "#505050FF",
    "pressed":  "#303030FF",
    "disabled": "#808080FF"
}
```

- 任一状态缺失 → 使用该类型的系统默认值（`ConstDef::DEFAULT_XXX_COLOR`）
- 整个 `background` 缺失 → 跳过，不调用 `setBackgroundStateColor()`，保持控件默认
- 四状态可以不全部提供

### 4.5 各控件特有属性

#### Label

```jsonc
{
    "type": "Label",
    "caption": "Hello World\n第二行",              // \n 换行
    "alignment": "TOP_LEFT",                       // 见下方 AlignmentMode 枚举值
    "font": {
        "name": "HarmonyOS_Sans_SC_Regular",      // FontName 枚举字符串
        "size": 16,                                // 字体大小 (px)
        "style": "NORMAL"                          // TTF_FontStyleFlags: NORMAL / BOLD / ITALIC / UNDERLINE / STRIKETHROUGH
    },
    "shadow": {
        "enabled": true,                           // 是否显示阴影
        "offset": { "x": 1, "y": 1 }              // 阴影偏移
    },
    "lineHeight": 0,                               // 0 表示使用字体默认行高
    "lineSpacingRatio": 0.2,                       // 行间距比例 (行高的百分比)
    "enableExpand": false,                         // true: 自动扩展高度适应内容; false: 截断
    "debugDraw": false                             // 调试模式显示热区矩形
}
```

**AlignmentMode 枚举字符串对应表**：


| JSON 字符串       | C++ 枚举值                        |
| ----------------- | --------------------------------- |
| `"TOP_LEFT"`      | `AlignmentMode::AM_TOP_LEFT`      |
| `"TOP_CENTER"`    | `AlignmentMode::AM_TOP_CENTER`    |
| `"TOP_RIGHT"`     | `AlignmentMode::AM_TOP_RIGHT`     |
| `"MID_LEFT"`      | `AlignmentMode::AM_MID_LEFT`      |
| `"CENTER"`        | `AlignmentMode::AM_CENTER`        |
| `"MID_RIGHT"`     | `AlignmentMode::AM_MID_RIGHT`     |
| `"BOTTOM_LEFT"`   | `AlignmentMode::AM_BOTTOM_LEFT`   |
| `"BOTTOM_CENTER"` | `AlignmentMode::AM_BOTTOM_CENTER` |
| `"BOTTOM_RIGHT"`  | `AlignmentMode::AM_BOTTOM_RIGHT`  |

**FontName 枚举字符串对应表**（节选常用，完整见 `ResourceLoader.h`）：


| JSON 字符串                   | C++ 枚举值                            |
| ----------------------------- | ------------------------------------- |
| `"HarmonyOS_Sans_SC_Regular"` | `FontName::HarmonyOS_Sans_SC_Regular` |
| `"HarmonyOS_Sans_SC_Bold"`    | `FontName::HarmonyOS_Sans_SC_Bold`    |
| `"HarmonyOS_Sans_SC_Light"`   | `FontName::HarmonyOS_Sans_SC_Light`   |
| `"MapleMono_NF_CN_Regular"`   | `FontName::MapleMono_NF_CN_Regular`   |
| `"Muyao_Softbrush"`           | `FontName::Muyao_Softbrush`           |

#### Button (Phase 1 跳过 Actor/LuotiAni)

Button 内部使用 `Label`（`m_caption`）来渲染文字，因此 Button 的 JSON 提供两种方式来配置标题文字：简单方式和嵌入方式。

**简单方式（Phase 1 使用）**：

```jsonc
{
    "type": "Button",
    "caption": "Click Me",
    "captionSize": 16,                             // 文字大小，默认 16
    "enableTextShadow": false                      // 是否开启文字阴影
}
```

**嵌入方式（Phase 2 加入）**：

```jsonc
{
    "type": "Button",
    "captionLabel": {
        "caption": "Click Me",
        "font": { "size": 16, "style": "BOLD" },
        "alignment": "CENTER",
        "shadow": { "enabled": true, "offset": { "x": 1, "y": 1 } },
        "colors": {
            "text": { "normal": "#FFFFFFFF", "hover": "#FFFF00FF", "pressed": "#C0C0C0FF" },
            "textShadow": { "normal": "#00000080" }
        }
    }
}
```

两种方式的处理规则：

- `captionLabel` 存在时，Button 解析器使用嵌入的 Label 配置创建标题，忽略顶层的 `caption`/`captionSize`/`enableTextShadow`
- `captionLabel` 不存在时，回退到简单方式，使用 `caption`（默认 `""`）、`captionSize`（默认 `16`）、`enableTextShadow`（默认 `false`）
- 这是为了兼顾简单场景和精确控制场景

#### EditBox

```jsonc
{
    "type": "EditBox",
    "text": "",                                    // 初始文本
    "placeholder": "Please input...",              // 占位提示文字
    "passwordMode": false,                         // 密码模式
    "passwordChar": "*",                           // 密码掩码字符
    "font": {
        "name": "HarmonyOS_Sans_SC_Regular",
        "size": 16
    },
    "alignment": "MID_LEFT",                        // 文字对齐方式
    "margin": { "left": 4, "top": 4, "right": 4, "bottom": 4 }  // 内边距
}
```

#### Panel

```jsonc
{
    "type": "Panel",
    "transparent": false,                           // 是否透明 (不绘制背景)
    "borderVisible": true,                          // 是否显示边框
    "bgColor": "#404040FF",                         // 背景色 (单色, 非状态色)
    "borderColor": "#808080FF",                     // 边框色 (单色, 非状态色)
    "children": [ /* ... */ ]                       // 子控件
}
```

- `bgColor` 和 `borderColor` 是单色，不是状态颜色（Panel 本身没有交互状态）
- `children` 支持嵌套任意控件类型

### 4.6 完整示例配置

见 `layouts/test_layout.json`（第 9 节）。

### 4.7 事件绑定

#### 方式 1 — JSON 声明 + registerHandler（自动绑定）

配置文件中声明事件：

```jsonc
{
    "id": "submitBtn",
    "type": "Button",
    "events": {
        "onClick": "handleSubmitClick"     // handler 名称字符串
    }
}
```

C++ 代码注册处理器：

```cpp
parser.registerHandler("handleSubmitClick", [](shared_ptr<Control> ctrl) {
    auto btn = dynamic_pointer_cast<Button>(ctrl);
    if (btn) {
        SDL_Log("Button '%s' clicked!", btn->getCaption().c_str());
    }
});
```

自动绑定规则：

- 解析事件时，若 `events` 中的 handler 名称已在 `m_handlers` 中注册，则自动调用对应的 `setOnXxx()` 进行绑定
- 若 handler 未注册，则静默跳过（不报错），以便逐步迁移
- 自动绑定内部通过 `dynamic_pointer_cast` 检查控件类型是否匹配

#### 方式 2 — ID 查找 + 手动绑定

```cpp
// 加载布局
shared_ptr<Control> root = parser.parseLayoutFile("layouts/test_layout.json");
BENCH->addControl(root);

// 通过 ID 查找控件
auto btn = dynamic_pointer_cast<Button>(parser.findControlById("submitBtn"));
if (btn) {
    btn->setOnClick([](shared_ptr<Button> b) {
        SDL_Log("Button clicked via manual binding!");
    });
}

auto editBox = dynamic_pointer_cast<EditBox>(parser.findControlById("nameEdit"));
if (editBox) {
    editBox->setOnTextChanged([](string text) {
        SDL_Log("Text changed: %s", text.c_str());
    });
}
```

两种方式可以混合使用，互不冲突。

### 4.8 事件类型 (Phase 1)


| 控件    | JSON 事件名     | C++ 绑定方法         | 回调签名                   |
| ------- | --------------- | -------------------- | -------------------------- |
| Button  | `onClick`       | `setOnClick()`       | `void(shared_ptr<Button>)` |
| EditBox | `onTextChanged` | `setOnTextChanged()` | `void(string)`             |
| EditBox | `onEnter`       | `setOnEnter()`       | `void()`                   |

## 5. LayoutParser 类设计

### 5.1 头文件完整定义

```cpp
// include/LayoutParser.h
#ifndef LayoutParserH
#define LayoutParserH

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <SDL3/SDL.h>
#include "nlohmann/json.hpp"
#include "ControlBase.h"
#include "Label.h"
#include "Button.h"
#include "EditBox.h"
#include "Panel.h"

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;

class LayoutParser {
public:
    LayoutParser();
    ~LayoutParser() = default;

    // ========== 布局加载 ==========

    /**
     * @brief 从 JSON 字符串解析布局
     * @param jsonContent JSON 字符串
     * @return 顶层控件（通常为 Panel），nullptr 表示解析失败
     */
    shared_ptr<Control> parseLayout(const string& jsonContent);

    /**
     * @brief 从 JSON 文件解析布局
     * @param jsonPath 配置文件路径
     * @return 顶层控件，nullptr 表示解析失败
     */
    shared_ptr<Control> parseLayoutFile(const fs::path& jsonPath);

    // ========== 事件绑定方式 2：ID 查找 ==========

    /**
     * @brief 通过 ID 查找已解析的控件
     * @param id 控件 ID（JSON 中 id 字段）
     * @return 控件指针，未找到时返回 nullptr
     */
    shared_ptr<Control> findControlById(const string& id);

    /**
     * @brief 获取所有已解析控件的 ID 列表
     * @return ID 字符串集合
     */
    vector<string> getAllControlIds() const;

    // ========== 事件绑定方式 1：注册处理器 ==========

    /**
     * @brief 注册事件处理器（解析时自动绑定）
     * @param name JSON events 中引用的处理器名称
     * @param handler 处理函数，接收 Control 基类指针
     */
    void registerHandler(const string& name,
                         function<void(shared_ptr<Control>)> handler);

    /**
     * @brief 移除已注册的处理器
     * @param name 处理器名称
     */
    void unregisterHandler(const string& name);

    /**
     * @brief 移除所有已注册的处理器
     */
    void clearHandlers();

    // ========== 状态管理 ==========

    /**
     * @brief 清除已解析的控件树和 ID 映射（不清除处理器）
     */
    void clear();

    /**
     * @brief 完全重置（清除控件树和所有处理器）
     */
    void reset();

private:
    // ========== 内部状态 ==========
    unordered_map<string, shared_ptr<Control>> m_controlsById;
    unordered_map<string, function<void(shared_ptr<Control>)>> m_handlers;

    // ========== 错误追踪 ==========

    /**
     * @brief 当前解析的 JSON 行号（用于错误定位）
     * 在 parseLayout 和 parseLayoutFile 开始时设置
     */
    int m_currentLineNo;

    /**
     * @brief 当前解析的 JSON Path（如 "controls[2].children[1].rect"）
     * 用于精确定位出错控件
     */
    string m_currentJsonPath;

    /**
     * @brief 日志辅助宏 — 在 SDL_Log 中注入行号和路径信息
     *
     * 使用示例：
     *   PARSE_ERROR("type field is missing at %s", m_currentJsonPath.c_str());
     *   // 输出示例：
     *   [LayoutParser] [Line 42] [controls[0].children[2]] ERROR: type field is missing
     */
    // 具体实现见 LogContext 辅助函数

    /**
     * @brief 输出带上下文的错误日志（严重级别）
     * @param message 错误描述
     */
    void logError(const string& message) const;
    /**
     * @brief 输出带上下文的警告日志
     * @param message 警告描述
     */
    void logWarn(const string& message) const;

    /**
     * @brief 更新当前解析路径（进入子节点时调用）
     * @param path 子路径，如 "controls[2]" 或 "rect"
     */
    void pushJsonPath(const string& segment);
    void popJsonPath();

    // ========== 控件工厂（按 type 分发） ==========

    /**
     * @brief 通用控件解析入口
     * @param j JSON 对象
     * @param parent 父控件指针
     * @return 创建的控件，nullptr 表示解析失败
     */
    shared_ptr<Control> parseControl(const json& j, Control* parent);

    // ========== 各控件专用解析函数 ==========

    shared_ptr<Label>    parseLabel(const json& j, Control* parent);
    shared_ptr<Button>   parseButton(const json& j, Control* parent);
    shared_ptr<EditBox>  parseEditBox(const json& j, Control* parent);
    shared_ptr<Panel>    parsePanel(const json& j, Control* parent);

    // ========== 通用属性解析工具 ==========

    /**
     * @brief 解析通用属性并设置到控件
     * 包括: rect, scale, margin, visible, enabled, colors
     */
    void parseCommonProperties(shared_ptr<ControlImpl> ctrl, const json& j);

    /**
     * @brief 解析事件绑定并自动绑定
     */
    void parseEvents(shared_ptr<ControlImpl> ctrl, const json& j);

    /**
     * @brief 解析子控件列表（递归）
     */
    void parseChildren(shared_ptr<Control> container, const json& j);

    // ========== 基础类型解析 ==========

    /**
     * @brief 解析 rect 对象 {"x","y","w","h"}
     */
    SRect parseRect(const json& j);

    /**
     * @brief 解析 margin 对象 {"left","top","right","bottom"}
     */
    Margin parseMargin(const json& j);

    /**
     * @brief 解析颜色值，支持两种格式
     * 格式1: "#RRGGBB" / "#RRGGBBAA"
     * 格式2: {"r","g","b","a"}
     * @return 解析后的 SDL_Color，解析失败返回默认值
     */
    SDL_Color parseColor(const json& j);

    /**
     * @brief 解析状态颜色（4 状态）
     * @param j 颜色对象（含 normal/hover/pressed/disabled 子字段）
     * @param type 颜色类型（用于获取系统默认值）
     */
    StateColor parseStateColor(const json& j, StateColor::Type type);

    /**
     * @brief 解析字体设置
     * @return FontName 枚举值，解析失败返回默认字体
     */
    FontName parseFontName(const string& name);

    /**
     * @brief 解析对齐模式字符串
     * @return AlignmentMode，解析失败返回默认值
     */
    AlignmentMode parseAlignment(const string& align);

    /**
     * @brief 解析字体样式字符串
     * @return TTF_FontStyleFlags，解析失败返回 NORMAL
     */
    TTF_FontStyleFlags parseFontStyle(const string& style);
};

#endif
```

### 5.2 解析流程详细说明

#### parseLayoutFile 完整流程

```
parseLayoutFile(path)
  │
  ├─ 1. 检查文件是否存在
  │     ├─ 不存在 → SDL_LogError("[LayoutParser] ERROR: Layout file not found: <path>")
  │     └─ return nullptr
  │
  ├─ 2. 读取文件到 string
  │     ├─ 读取失败 → SDL_LogError("[LayoutParser] ERROR: Failed to read layout file: <path>")
  │     └─ return nullptr
  │
  └─ 3. 调用 parseLayout(jsonString)
        └─ 解析时所有行号从第 1 行开始计数
```

#### parseLayout 完整流程

```
parseLayout(jsonString)
  │
  ├─ 1. json::parse(jsonString) — 使用 try/catch 捕获 parse_error
  │     ├─ 成功 → 继续
  │     ├─ 失败 → 从异常 e.byte 通过 byteOffsetToLineNo 转为行号
  │     │       → SDL_LogError("[LayoutParser] [Line %d] JSON parse error: %s",
  │     │                      lineNo, e.what())
  │     │       → return nullptr
  │
  ├─ 2. 初始化追踪状态
  │     ├─ m_currentLineNo = 1
  │     ├─ m_currentJsonPath = "root"
  │
  ├─ 3. 检查 controls 是否存在且为数组
  │     └─ 缺失/非数组 → logError("'controls' array is required") → return nullptr
  │
  ├─ 4. 遍历 controls 数组（索引 i = 0, 1, 2...）
  │     └─ 对每个元素调用 parseControl(j, parent=nullptr, index=i)
  │
  ├─ 5. 如果 controls 数组为空
  │     └─ logWarn("'controls' array is empty")
  │
  └─ 6. 返回解析后的根控件
```

#### parseControl 完整流程

```
parseControl(j, parent, index)
  │
  ├─ 1. 构建路径标识：如 "controls[2]"
  │     └─ pushJsonPath("controls[" + to_string(index) + "]")
  │
  ├─ 2. 检查 j 中是否包含 "type" 字段
  │     └─ 缺失 → logError("'type' field is required") → popJsonPath → return nullptr
  │
  ├─ 3. 读取 type 字符串 → 分发到对应解析函数
  │     ├─ "Label"     → parseLabel(j, parent)
  │     ├─ "Button"    → parseButton(j, parent)
  │     ├─ "EditBox"   → parseEditBox(j, parent)
  │     ├─ "Panel"     → parsePanel(j, parent)
  │     └─ 其他        → logWarn("unknown control type \"" + type + "\", skipping") → popJsonPath → return nullptr
  │
  ├─ 4. 解析完成 → popJsonPath → return 控件
```

#### parseLabel 完整流程

```
parseLabel(j, parent)
  │
  ├─ 1. pushJsonPath("rect")
  │     └─ 解析 rect → 若缺失则 logError + return nullptr
  │     └─ popJsonPath
  │
  ├─ 2. 解析 scale (可选，默认为 1.0)
  │
  ├─ 3. 创建 Label(make_shared<Label>(parent, rect, xScale, yScale))
  │     └─ 创建失败 → logError("failed to create Label") → return nullptr
  │
  ├─ 4. 设置通用属性
  │     └─ parseCommonProperties(label, j)
  │
  ├─ 5. 解析 Label 特有属性（每个字段解析前 push/pop 路径）
  │     ├─ pushJsonPath("font") → 解析字体 → popJsonPath
  │     ├─ pushJsonPath("shadow") → 解析阴影 → popJsonPath
  │     └─ 其他简单字段直接解析
  │
  ├─ 6. 解析事件绑定
  │     └─ parseEvents(label, j)
  │
  ├─ 7. 注册 ID（若有）
  │
  ├─ 8. 调用 label->create()
  │
  └─ 9. return label
```

#### parseButton 完整流程

```
parseButton(j, parent)
  │
  ├─ 1. 解析 rect + scale → 创建 Button
  │
  ├─ 2. parseCommonProperties (通用属性)
  │
  ├─ 3. 解析 Button 特有属性
  │     ├─ j["caption"]        → setCaption
  │     ├─ j["captionSize"]    → setCaptionSize
  │     └─ j["enableTextShadow"] → setTextShadowEnable
  │
  ├─ 4. parseEvents
  │     └─ j["events"]["onClick"] → btn->setOnClick(lambda)
  │
  ├─ 5. 注册 ID
  │
  ├─ 6. btn->create()
  │
  └─ 7. return btn
```

注意：Phase 1 的 Button 不会解析 actors 和 luotiAni 字段，即使 JSON 中包含这些字段也会忽略。

#### parseEditBox 完整流程

```
parseEditBox(j, parent)
  │
  ├─ 1. 解析 rect + scale → 创建 EditBox
  │
  ├─ 2. parseCommonProperties
  │
  ├─ 3. 解析 EditBox 特有属性
  │     ├─ j["text"]           → setText
  │     ├─ j["placeholder"]    → setPlaceholder
  │     ├─ j["passwordMode"]   → setPasswordMode
  │     ├─ j["passwordChar"]   → setPasswordChar
  │     ├─ j["font"]["name"]   → setFont
  │     ├─ j["font"]["size"]   → setFontSize
  │     ├─ j["alignment"]      → setAlignmentMode
  │     └─ j["margin"]         → setMargin
  │
  ├─ 4. parseEvents
  │     ├─ j["events"]["onTextChanged"] → editBox->setOnTextChanged(lambda)
  │     └─ j["events"]["onEnter"]       → editBox->setOnEnter(lambda)
  │
  ├─ 5. 注册 ID
  │
  ├─ 6. editBox->create()
  │
  └─ 7. return editBox
```

#### parsePanel 完整流程

```
parsePanel(j, parent)
  │
  ├─ 1. 解析 rect + scale → 创建 Panel
  │
  ├─ 2. parseCommonProperties
  │
  ├─ 3. 解析 Panel 特有属性
  │     ├─ j["transparent"]    → setTransparent
  │     ├─ j["borderVisible"]  → (Panel 没有 setBorderVisible, 通过 setBorderStateColor 控制)
  │     ├─ j["bgColor"]        → setBackgroundStateColor(single color for normal state)
  │     └─ j["borderColor"]    → setBorderStateColor(single color for normal state)
  │
  ├─ 4. （Panel 没有事件）
  │
  ├─ 5. 注册 ID
  │
  ├─ 6. 解析子控件
  │     └─ parseChildren(panel, j)
  │         ├─ j["children"] → 遍历数组
  │         └─ 每个子元素 → parseControl(child, panel) → panel->addControl(result)
  │
  ├─ 7. panel->create()
  │
  └─ 8. return panel
```

#### parseEvents 详细逻辑

```cpp
void LayoutParser::parseEvents(shared_ptr<ControlImpl> ctrl, const json& j) {
    if (!j.contains("events") || j["events"].is_null()) return;

    const json& events = j["events"];

    // Button: onClick
    if (auto btn = dynamic_pointer_cast<Button>(ctrl)) {
        if (events.contains("onClick") && events["onClick"].is_string()) {
            string handlerName = events["onClick"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                btn->setOnClick([this, handlerName](shared_ptr<Button> sender) {
                    m_handlers[handlerName](sender);
                });
            }
        }
    }

    // EditBox: onTextChanged, onEnter
    if (auto editBox = dynamic_pointer_cast<EditBox>(ctrl)) {
        if (events.contains("onTextChanged") && events["onTextChanged"].is_string()) {
            string handlerName = events["onTextChanged"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                editBox->setOnTextChanged([this, handlerName](string text) {
                    // 暂不支持传递 string，后续可扩展
                    m_handlers[handlerName](nullptr);
                });
            }
        }

        if (events.contains("onEnter") && events["onEnter"].is_string()) {
            string handlerName = events["onEnter"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                editBox->setOnEnter([this, handlerName]() {
                    m_handlers[handlerName](nullptr);
                });
            }
        }
    }
}
```

### 5.3 工具函数实现细节

#### parseColor 实现逻辑

```cpp
SDL_Color LayoutParser::parseColor(const json& j, const string& fieldName) {
    SDL_Color color = {255, 255, 255, 255}; // 默认白色

    if (j.is_string()) {
        // 格式 1: "#RRGGBB" / "#RRGGBBAA" / "#RGB"
        string hex = j.get<string>();
        if (hex.empty() || hex[0] != '#') {
            logWarn(fieldName + ": invalid hex color \"" + hex + "\", using default white");
            return color;
        }
        hex = hex.substr(1); // 去掉 '#'

        if (hex.length() == 3) {
            string r(2, hex[0]), g(2, hex[1]), b(2, hex[2]);
            color.r = (uint8_t)stoi(r, nullptr, 16);
            color.g = (uint8_t)stoi(g, nullptr, 16);
            color.b = (uint8_t)stoi(b, nullptr, 16);
            color.a = 255;
        } else if (hex.length() == 6) {
            color.r = (uint8_t)stoi(hex.substr(0,2), nullptr, 16);
            color.g = (uint8_t)stoi(hex.substr(2,2), nullptr, 16);
            color.b = (uint8_t)stoi(hex.substr(4,2), nullptr, 16);
            color.a = 255;
        } else if (hex.length() == 8) {
            color.r = (uint8_t)stoi(hex.substr(0,2), nullptr, 16);
            color.g = (uint8_t)stoi(hex.substr(2,2), nullptr, 16);
            color.b = (uint8_t)stoi(hex.substr(4,2), nullptr, 16);
            color.a = (uint8_t)stoi(hex.substr(6,2), nullptr, 16);
        } else {
            logWarn(fieldName + ": invalid hex color length \"" + hex + "\", using default white");
        }
    } else if (j.is_object()) {
        // 格式 2: {"r","g","b","a"}
        color.r = j.value("r", 255);
        color.g = j.value("g", 255);
        color.b = j.value("b", 255);
        color.a = j.value("a", 255);
    }

    return color;
}
```

注意：`parseColor` 的参数增加 `fieldName` 只是其中一种实现方案。另一种方案是在调用 `parseColor` 之前和之后维护 `pushJsonPath/popJsonPath`，由 `logWarn` 自动获取当前路径。具体采用哪种方案在实现时确定，目标都是确保日志能输出 `[path] WARN: ...` 格式。

#### parseStateColor 实现逻辑

```cpp
StateColor LayoutParser::parseStateColor(const json& j, StateColor::Type type) {
    StateColor stateColor(type); // 使用系统默认值初始化

    if (j.is_null()) return stateColor;

    if (j.contains("normal"))   stateColor.setNormal(parseColor(j["normal"], "normal"));
    if (j.contains("hover"))    stateColor.setHover(parseColor(j["hover"], "hover"));
    if (j.contains("pressed"))  stateColor.setPressed(parseColor(j["pressed"], "pressed"));
    if (j.contains("disabled")) stateColor.setDisabled(parseColor(j["disabled"], "disabled"));

    return stateColor;
}
```

#### parseFontName 实现逻辑

```cpp
FontName LayoutParser::parseFontName(const string& name) {
    static const unordered_map<string, FontName> nameMap = {
        {"HarmonyOS_Sans_SC_Regular",   FontName::HarmonyOS_Sans_SC_Regular},
        {"HarmonyOS_Sans_SC_Bold",      FontName::HarmonyOS_Sans_SC_Bold},
        {"HarmonyOS_Sans_SC_Light",     FontName::HarmonyOS_Sans_SC_Light},
        {"HarmonyOS_Sans_SC_Thin",      FontName::HarmonyOS_Sans_SC_Thin},
        {"HarmonyOS_Sans_SC_Medium",    FontName::HarmonyOS_Sans_SC_Medium},
        {"HarmonyOS_Sans_SC_Black",     FontName::HarmonyOS_Sans_SC_Black},
        {"MapleMono_NF_CN_Regular",     FontName::MapleMono_NF_CN_Regular},
        {"MapleMono_NF_CN_Bold",        FontName::MapleMono_NF_CN_Bold},
        {"Muyao_Softbrush",             FontName::Muyao_Softbrush},
        // ... 其他字体
    };

    auto it = nameMap.find(name);
    if (it != nameMap.end()) return it->second;

    logWarn("unknown font name \"" + name + "\", using default");
    return FontName::HarmonyOS_Sans_SC_Regular; // 默认字体
}
```

#### parseAlignment 实现逻辑

```cpp
AlignmentMode LayoutParser::parseAlignment(const string& align) {
    static const unordered_map<string, AlignmentMode> alignMap = {
        {"TOP_LEFT",      AlignmentMode::AM_TOP_LEFT},
        {"TOP_CENTER",    AlignmentMode::AM_TOP_CENTER},
        {"TOP_RIGHT",     AlignmentMode::AM_TOP_RIGHT},
        {"MID_LEFT",      AlignmentMode::AM_MID_LEFT},
        {"CENTER",        AlignmentMode::AM_CENTER},
        {"MID_RIGHT",     AlignmentMode::AM_MID_RIGHT},
        {"BOTTOM_LEFT",   AlignmentMode::AM_BOTTOM_LEFT},
        {"BOTTOM_CENTER", AlignmentMode::AM_BOTTOM_CENTER},
        {"BOTTOM_RIGHT",  AlignmentMode::AM_BOTTOM_RIGHT},
    };

    auto it = alignMap.find(align);
    if (it != alignMap.end()) return it->second;

    logWarn("unknown alignment \"" + align + "\", using TOP_LEFT");
    return AlignmentMode::AM_TOP_LEFT;
}
```

#### parseFontStyle 实现逻辑

```cpp
TTF_FontStyleFlags LayoutParser::parseFontStyle(const string& style) {
    static const unordered_map<string, TTF_FontStyleFlags> styleMap = {
        {"NORMAL",        TTF_STYLE_NORMAL},
        {"BOLD",          TTF_STYLE_BOLD},
        {"ITALIC",        TTF_STYLE_ITALIC},
        {"UNDERLINE",     TTF_STYLE_UNDERLINE},
        {"STRIKETHROUGH", TTF_STYLE_STRIKETHROUGH},
    };

    auto it = styleMap.find(style);
    if (it != styleMap.end()) return it->second;

    return TTF_STYLE_NORMAL;
}
```

#### parseRect 实现逻辑

```cpp
SRect LayoutParser::parseRect(const json& j) {
    SRect rect;
    pushJsonPath("rect");

    if (!j.is_object()) {
        logError("'rect' must be an object with {x, y, w, h}");
        popJsonPath();
        return rect;
    }

    rect.left   = j.value("x", 0.0f);
    rect.top    = j.value("y", 0.0f);
    rect.width  = j.value("w", 0.0f);
    rect.height = j.value("h", 0.0f);

    vector<string> missing;
    if (!j.contains("x")) missing.push_back("x");
    if (!j.contains("y")) missing.push_back("y");
    if (!j.contains("w")) missing.push_back("w");
    if (!j.contains("h")) missing.push_back("h");

    if (!missing.empty()) {
        string fields;
        for (size_t i = 0; i < missing.size(); ++i) {
            if (i > 0) fields += ", ";
            fields += missing[i];
        }
        logWarn("rect missing field(s): " + fields + ", defaulting to 0");
    }

    popJsonPath();
    return rect;
}
```

#### parseMargin 实现逻辑

```cpp
Margin LayoutParser::parseMargin(const json& j) {
    Margin margin;

    if (!j.is_object()) {
        pushJsonPath("margin");
        logWarn("'margin' must be an object, using defaults");
        popJsonPath();
        return margin;
    }

    margin.left   = j.value("left",   0.0f);
    margin.top    = j.value("top",    0.0f);
    margin.right  = j.value("right",  0.0f);
    margin.bottom = j.value("bottom", 0.0f);

    return margin;
}
```

## 6. 错误处理策略

### 6.1 核心原则

所有解析错误和警告的日志输出 **必须包含**以下信息：

1. **行号** — JSON 文件中的行号（通过 `nlohmann::json::get_token_start_location()` 获取）
2. **路径** — 当前解析的 JSON 路径（如 `controls[2].children[1].rect`）
3. **原因** — 具体的错误描述

```
输出格式：
[LayoutParser] [Line 42] [controls[0].type] ERROR: type field is required
[LayoutParser] [Line 85] [controls[1].colors.background.normal] WARN: invalid color format "#GGG", using default
```

### 6.2 行号获取机制

`nlohmann/json` 在 v3.11+ 提供了 `get_token_start_location()` 方法返回 `json::parse_event_t` 的位置信息。通过结合 `parser` 回调，可以在解析时获取每个 JSON token 的行号。

```cpp
// 在 parseLayout 中使用带位置信息的解析方式：
struct JsonContext {
    int lineNo;
    string jsonPath;
};

void LayoutParser::parseLayout(const string& jsonContent) {
    // 预扫描：将 JSON 字符串按行分割，建立 行号→行内容 的映射
    // 用于错误日志中输出上下文行
    // 实际行号在 parseControl 中通过 json[json对象].get_token_start_location()
    // 获取（需 nlohmann/json v3.11+ 支持）
    //
    // 替代方案：在解析过程中手动维护 m_currentJsonPath，
    // 并在每个 json 节点访问时通过异常 catch 获取位置
    // 或使用 json::parse 的 callback 模式捕获行号

    // === 方案选择：手动维护 + 异常位置捕捉 ===
    try {
        json j = json::parse(jsonContent);
        m_currentLineNo = 1;
        m_currentJsonPath = "root";
        parseControls(j);
    } catch (const json::parse_error& e) {
        // 从异常中提取行号
        // e.byte 是字节偏移，需要通过预扫描的行映射转为行号
        int lineNo = byteOffsetToLineNo(jsonContent, e.byte);
        SDL_LogError("[LayoutParser] [Line %d] JSON parse error: %s",
                     lineNo, e.what());
    }
}
```

所有解析函数进入子对象时调用 `pushJsonPath`，退出时调用 `popJsonPath`：

```cpp
// 在 parseControl 入口处
void LayoutParser::parseControl(const json& j, Control* parent) {
    // 检查 type 字段前先记录路径
    pushJsonPath(/* 从 j 中获取的当前对象名称或索引 */);
    // ... 解析逻辑
    popJsonPath();
}
```

### 6.3 日志辅助函数

```cpp
void LayoutParser::logError(const string& message) const {
    SDL_LogError(
        "[LayoutParser] [Line %d] [%s] ERROR: %s",
        m_currentLineNo, m_currentJsonPath.c_str(), message.c_str()
    );
}

void LayoutParser::logWarn(const string& message) const {
    SDL_LogWarn(
        "[LayoutParser] [Line %d] [%s] WARN: %s",
        m_currentLineNo, m_currentJsonPath.c_str(), message.c_str()
    );
}
```

### 6.4 pushJsonPath / popJsonPath 实现

```cpp
void LayoutParser::pushJsonPath(const string& segment) {
    if (m_currentJsonPath.empty()) {
        m_currentJsonPath = segment;
    } else {
        m_currentJsonPath += "." + segment;
    }
}

void LayoutParser::popJsonPath() {
    auto pos = m_currentJsonPath.rfind('.');
    if (pos != string::npos) {
        m_currentJsonPath = m_currentJsonPath.substr(0, pos);
    } else {
        m_currentJsonPath.clear();
    }
}
```

### 6.5 行号映射辅助函数

```cpp
// 将 JSON 字符串和字节偏移转换为行号
int byteOffsetToLineNo(const string& content, size_t byteOffset) {
    int line = 1;
    for (size_t i = 0; i < byteOffset && i < content.size(); ++i) {
        if (content[i] == '\n') line++;
    }
    return line;
}
```

### 6.6 错误分级


| 级别 | 处理方式                  | 日志格式     | 场景                                    |
| ---- | ------------------------- | ------------ | --------------------------------------- |
| 严重 | 跳过当前控件/返回 nullptr | `ERROR: ...` | 文件不存在、JSON 解析失败、关键字段缺失 |
| 警告 | 使用默认值继续解析        | `WARN: ...`  | 颜色格式错误、未知 type、未知 font name |
| 静默 | 跳过不处理                | 不输出日志   | 缺失可选字段、未注册的 handler 名称     |

### 6.7 常见错误场景与日志示例


| 错误                | 级别 | 日志示例                                                                                                                 |
| ------------------- | ---- | ------------------------------------------------------------------------------------------------------------------------ |
| 配置文件不存在      | 严重 | `[LayoutParser] ERROR: Layout file not found: layouts/nonexist.json`                                                     |
| JSON 语法错误       | 严重 | `[LayoutParser] [Line 15] JSON parse error: [json.exception.parse_error.101] syntax error at byte 320`                   |
| `controls` 字段缺失 | 严重 | `[LayoutParser] [Line 1] [root] ERROR: 'controls' array is required`                                                     |
| `type` 字段缺失     | 严重 | `[LayoutParser] [Line 42] [controls[2]] ERROR: 'type' field is required`                                                 |
| 未知的`type` 值     | 警告 | `[LayoutParser] [Line 55] [controls[3]] WARN: unknown control type "FooBar", skipping`                                   |
| 颜色格式无法解析    | 警告 | `[LayoutParser] [Line 78] [controls[1].colors.background.normal] WARN: invalid hex color "XFF0000", using default white` |
| `rect` 字段缺失     | 严重 | `[LayoutParser] [Line 90] [controls[4]] ERROR: 'rect' field is required, skipping control`                               |
| `rect` 字段值不完整 | 警告 | `[LayoutParser] [Line 95] [controls[5].rect] WARN: rect missing field "w", defaulting to 0`                              |
| 未知 handler 名称   | 静默 | 不输出日志                                                                                                               |
| 字体名称不存在      | 警告 | `[LayoutParser] [Line 33] [controls[0].font.name] WARN: unknown font name "NonExistentFont", using default`              |
| 控件创建失败        | 严重 | `[LayoutParser] [Line 50] [controls[2]] ERROR: failed to create Label control`                                           |

## 7. 模板代码与宏

### 7.1 parseCommonProperties 实现模板

```cpp
void LayoutParser::parseCommonProperties(shared_ptr<ControlImpl> ctrl, const json& j) {
    if (!ctrl) return;

    // rect
    if (j.contains("rect")) {
        ctrl->setRect(parseRect(j["rect"]));
    } else {
        // parseRect 内部有 push/pop，但若 j 不含 rect 则需自行记录
        logError("'rect' field is required, skipping control");
    }

    // scale
    if (j.contains("scale") && j["scale"].is_object()) {
        float sx = j["scale"].value("x", 1.0f);
        float sy = j["scale"].value("y", 1.0f);
        ctrl->setScaleX(sx);
        ctrl->setScaleY(sy);
    }

    // margin
    if (j.contains("margin")) {
        ctrl->setMargin(parseMargin(j["margin"]));
    }

    // visible
    ctrl->setVisible(j.value("visible", true));

    // enabled
    ctrl->setEnable(j.value("enabled", true));

    // colors
    if (j.contains("colors") && j["colors"].is_object()) {
        pushJsonPath("colors");
        const json& colors = j["colors"];
        if (colors.contains("background")) {
            ctrl->setBackgroundStateColor(
                parseStateColor(colors["background"], StateColor::Type::Background));
        }
        if (colors.contains("border")) {
            ctrl->setBorderStateColor(
                parseStateColor(colors["border"], StateColor::Type::Border));
        }
        if (colors.contains("text")) {
            ctrl->setTextStateColor(
                parseStateColor(colors["text"], StateColor::Type::Text));
        }
        if (colors.contains("textShadow")) {
            ctrl->setTextShadowStateColor(
                parseStateColor(colors["textShadow"], StateColor::Type::TextShadow));
        }
        popJsonPath(); // colors
    }
}
```

### 7.2 parseChildren 实现模板

```cpp
void LayoutParser::parseChildren(shared_ptr<Control> container, const json& j) {
    if (!j.contains("children") || !j["children"].is_array()) return;

    pushJsonPath("children");
    const json& children = j["children"];
    for (size_t i = 0; i < children.size(); ++i) {
        auto child = parseControl(children[i], container.get(), (int)i);
        if (child) {
            auto containerImpl = dynamic_pointer_cast<ControlImpl>(container);
            if (containerImpl) {
                containerImpl->addControl(child);
            }
        }
    }
    popJsonPath(); // children
}
```

### 7.3 解析结果使用者指南

```cpp
// 使用示例
void testBenchInitialize(void) {
    LayoutParser parser;

    // 方式1：注册事件处理器
    parser.registerHandler("onSubmitClick", [](shared_ptr<Control> ctrl) {
        auto btn = dynamic_pointer_cast<Button>(ctrl);
        if (btn) {
            SDL_Log("Button '%s' clicked via auto-binding!",
                    btn->getCaption().c_str());
        }
    });

    // 加载布局
    shared_ptr<Control> root = parser.parseLayoutFile("layouts/test_layout.json");
    if (root) {
        BENCH->addControl(root);

        // 方式2：手动绑定
        auto editBox = dynamic_pointer_cast<EditBox>(
            parser.findControlById("nameEdit"));
        if (editBox) {
            editBox->setOnTextChanged([](string text) {
                SDL_Log("Text changed: %s", text.c_str());
            });
        }
    }
}
```

## 8. 构建系统变更

### 8.1 CMakeLists.txt (根目录)

在 `set(SOURCES ...)` 中添加 `src/LayoutParser.cpp`：

```cmake
set(SOURCES
    src/ControlBase.cpp
    src/Label.cpp
    src/Button.cpp
    src/EditBox.cpp
    src/TextArea.cpp
    src/CheckBox.cpp
    src/ProgressBar.cpp
    src/ScrollBar.cpp
    src/Menu.cpp
    src/Panel.cpp
    src/Dialog.cpp
    src/Actor.cpp
    src/Material.cpp
    src/LuotiAni.cpp
    src/Bench.cpp
    src/MainWindow.cpp
    src/Utility.cpp
    src/ConstDef.cpp
    src/ResourceLoader.cpp
    src/EventQueue.cpp
    src/GraphTool.cpp
    src/GraphOperaAdapt2d.cpp
    src/LayoutParser.cpp              # ← 新增
    subModules/DebugTrace/DebugTrace.cpp
)
```

### 8.2 test/CMakeLists.txt

```cmake
# 在底部添加（或与其他 test_xxx 并列的区域添加）：

add_executable(test_layout test_layout.cpp)
target_link_libraries(test_layout UIControls SDL3.lib SDL3_ttf.lib SDL3_image.lib)

# 复制 DLL - 采用和已有测试相同的 POST_BUILD 模式
add_custom_command(TARGET test_layout POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/subModules/libs/$<CONFIG>/*.dll"
        "$<TARGET_FILE_DIR:test_layout>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:test_layout>/assets"
    COMMENT "Copying DLLs and assets for test_layout"
)
```

## 9. 完整示例配置文件

文件路径：`layouts/test_layout.json`

```jsonc
{
    "version": "1.0",
    "controls": [
        {
            "type": "Panel",
            "id": "mainPanel",
            "rect": { "x": 0, "y": 0, "w": 800, "h": 600 },
            "colors": {
                "background": { "normal": "#282828FF" }
            },
            "children": [
                {
                    "type": "Label",
                    "id": "titleLabel",
                    "rect": { "x": 0, "y": 20, "w": 800, "h": 40 },
                    "caption": "布局配置系统测试",
                    "alignment": "CENTER",
                    "font": {
                        "name": "HarmonyOS_Sans_SC_Bold",
                        "size": 28
                    },
                    "colors": {
                        "text": { "normal": "#FFFFFFFF" }
                    },
                    "shadow": {
                        "enabled": true,
                        "offset": { "x": 2, "y": 2 }
                    }
                },
                {
                    "type": "Panel",
                    "id": "formPanel",
                    "rect": { "x": 50, "y": 80, "w": 400, "h": 200 },
                    "colors": {
                        "border": {
                            "normal": "#808080FF"
                        }
                    },
                    "children": [
                        {
                            "type": "Label",
                            "id": "nameLabel",
                            "rect": { "x": 20, "y": 20, "w": 80, "h": 30 },
                            "caption": "姓名：",
                            "alignment": "MID_LEFT",
                            "font": { "size": 16 }
                        },
                        {
                            "type": "EditBox",
                            "id": "nameEdit",
                            "rect": { "x": 110, "y": 20, "w": 250, "h": 30 },
                            "placeholder": "请输入姓名",
                            "font": { "size": 16 },
                            "colors": {
                                "background": { "normal": "#404040FF", "hover": "#505050FF" },
                                "border": { "normal": "#606060FF", "focused": "#909090FF" }
                            }
                        },
                        {
                            "type": "Label",
                            "id": "descLabel",
                            "rect": { "x": 20, "y": 70, "w": 360, "h": 20 },
                            "caption": "这是一个编辑框，输入文本后会触发 onTextChanged",
                            "alignment": "TOP_LEFT",
                            "font": { "size": 12 },
                            "colors": {
                                "text": { "normal": "#A0A0A0FF" }
                            }
                        },
                        {
                            "type": "Button",
                            "id": "submitBtn",
                            "rect": { "x": 280, "y": 140, "w": 100, "h": 35 },
                            "caption": "提交",
                            "captionSize": 16,
                            "colors": {
                                "background": {
                                    "normal": "#4060A0FF",
                                    "hover": "#5080C0FF",
                                    "pressed": "#305090FF"
                                },
                                "border": {
                                    "normal": "#6080C0FF"
                                },
                                "text": {
                                    "normal": "#FFFFFFFF"
                                }
                            },
                            "events": {
                                "onClick": "onSubmitClick"
                            }
                        }
                    ]
                },
                {
                    "type": "Panel",
                    "id": "previewPanel",
                    "rect": { "x": 500, "y": 80, "w": 250, "h": 200 },
                    "colors": {
                        "border": {
                            "normal": "#808080FF"
                        }
                    },
                    "children": [
                        {
                            "type": "Label",
                            "id": "previewTitle",
                            "rect": { "x": 10, "y": 10, "w": 230, "h": 25 },
                            "caption": "预览区域",
                            "alignment": "CENTER",
                            "font": { "size": 14, "style": "BOLD" }
                        },
                        {
                            "type": "Label",
                            "id": "previewContent",
                            "rect": { "x": 10, "y": 45, "w": 230, "h": 140 },
                            "caption": "姓名：\n年龄：\n备注：",
                            "alignment": "TOP_LEFT",
                            "font": { "size": 14 },
                            "colors": {
                                "text": { "normal": "#C0C0C0FF" }
                            }
                        }
                    ]
                },
                {
                    "type": "Label",
                    "id": "statusLabel",
                    "rect": { "x": 0, "y": 560, "w": 800, "h": 25 },
                    "caption": "状态：就绪",
                    "alignment": "TOP_LEFT",
                    "font": { "size": 14 },
                    "colors": {
                        "text": { "normal": "#808080FF" }
                    }
                }
            ]
        }
    ]
}
```

## 10. 测试计划

### 10.1 test_layout 测试场景

测试文件：`test/test_layout.cpp`

结构与现有测试相同（`SDL_AppInit` → `BENCH->setOnInitial` → `SDL_AppEvent/Iterate/Quit`）。

#### 初始化函数 `testBenchInitialize` 逻辑：

```
1. 创建 LayoutParser 实例
2. 注册事件处理器
   - "onSubmitClick" → SDL_Log("Submit button clicked")
3. 调用 parseLayoutFile("layouts/test_layout.json")
4. 如果解析成功，添加到 BENCH
5. 使用 findControlById 查找各控件
   - "nameEdit" → 手动绑定 onTextChanged → SDL_Log("Text: %s", text)
   - "titleLabel" → 验证非 nullptr
   - "statusLabel" → 手动修改 caption
6. 如果解析失败 → SDL_LogError
```

#### 验证清单

- [ ]  所有控件按 `rect` 出现在正确位置
- [ ]  标题 Label 居中显示，带阴影
- [ ]  EditBox 点击后可输入文字
- [ ]  方式1：点击 Button → 控制台输出 "Submit button clicked"
- [ ]  方式2：EditBox 输入文字 → 控制台输出 "Text: ..."
- [ ]  `findControlById` 对存在的 ID 返回非空
- [ ]  `findControlById` 对不存在的 ID 返回 nullptr
- [ ]  `getAllControlIds` 返回所有 ID 列表
- [ ]  Panel 嵌套结构正确（formPanel / previewPanel）
- [ ]  两种颜色格式均正常解析
- [ ]  窗口缩放后控件跟随缩放（继承现有 scale 系统）

## 11. 入口关系与共存策略

### 11.1 当前硬编码入口

```
SDL_AppInit()
  └─ BENCH->setOnInitial(testBenchInitialize)   ← 注册初始化回调
       └─ testBenchInitialize()
            ├─ LabelBuilder(...).build()          ← 硬编码创建控件
            ├─ g_label1->setCaption("...")
            ├─ ButtonBuilder(...).build()
            ├─ g_button1->setOnClick(...)         ← 手动绑定事件
            ├─ BENCH->addControl(g_label1)        ← 逐个添加到 Bench
            └─ BENCH->addControl(g_button1)
```

每个测试文件在 `testBenchInitialize()` 中依次创建控件、设置属性、绑定事件、添加到 Bench。

### 11.2 配置文件化入口

```
SDL_AppInit()
  └─ BENCH->setOnInitial(testLayoutInitialize)   ← 注册新回调
       └─ testLayoutInitialize()
            ├─ LayoutParser parser;
            ├─ parser.registerHandler(...)         ← 注册事件处理器
            ├─ auto root = parser.parseLayoutFile(
            │       "layouts/test_layout.json")    ← 从配置文件一步加载布局
            ├─ if (root) BENCH->addControl(root)   ← 将整棵树一次性添加到 Bench
            └─ // 可选：再通过 findControlById 补充绑定
                 auto btn = parser.findControlById("submitBtn");
                 btn->setOnClick(...);
```

### 11.3 两种入口的关系

```
                    ┌──────────────────────────────────────┐
                    │          SDL_AppInit()               │
                    │        BENCH->setOnInitial(fn)       │
                    └────────────┬─────────────────────────┘
                                 │
                    ┌────────────▼─────────────────────────┐
                    │        testBenchInitialize()         │
                    │         (由开发者实现的回调)           │
                    ├──────────────────┬───────────────────┤
                    │                  │                   │
              ┌─────▼──────┐    ┌──────▼──────┐    ┌──────▼──────┐
              │ 硬编码模式   │    │ 混合模式    │    │ 纯配置文件  │
              │             │    │             │    │   模式      │
              │ LabelBuilder│    │ C++ 创建     │    │ LayoutParser│
              │ ButtonBuilde│    │ 与 Layout    │    │ .parseLayout│
              │ r           │    │ Parser 混用  │    │ File()      │
              │ .build()    │    │             │    │             │
              │ BENCH->     │    │ BENCH->     │    │ BENCH->     │
              │ addControl  │    │ addControl  │    │ addControl  │
              │ (逐个添加)   │    │ (混合添加)   │    │ (一次添加)  │
              └─────────────┘    └─────────────┘    └─────────────┘
```

### 11.4 三种模式详解

#### 模式 1：纯硬编码（现有模式）

```cpp
void testBenchInitialize() {
    g_label1 = LabelBuilder(nullptr, SRect(20, 30, 130, 30))
        .setCaption("L1: Left")
        .setAlignmentMode(AlignmentMode::AM_TOP_LEFT)
        .setBorderStateColor(redBorder)
        .build();
    BENCH->addControl(g_label1);

    g_button1 = ButtonBuilder(nullptr, SRect(20, 80, 100, 35))
        .setCaption("Click")
        .build();
    g_button1->setOnClick([](auto btn) { ... });
    BENCH->addControl(g_button1);
}
```

- 所有控件在 C++ 中逐个创建和配置
- 适用于：编写新控件时的单元测试、调试场景
- **不会因为引用了 `LayoutParser` 而受影响**

#### 模式 2：纯配置文件

```cpp
void testLayoutInitialize() {
    LayoutParser parser;

    parser.registerHandler("onSubmit", [](shared_ptr<Control> ctrl) {
        // 处理点击
    });

    auto root = parser.parseLayoutFile("layouts/test_layout.json");
    if (root) {
        BENCH->addControl(root);
    }
}
```

- 布局完全由 JSON 文件定义
- 事件处理通过 `registerHandler` + JSON 声明自动绑定
- 适用于：功能稳定的界面、多项目共享布局

#### 模式 3：混合模式（**推荐**作为迁移方案）

```cpp
void testMixedInitialize() {
    LayoutParser parser;

    // 部分控件由配置文件加载
    parser.registerHandler("onBtnClick", [](shared_ptr<Control> ctrl) { ... });
    auto panel = parser.parseLayoutFile("layouts/partial_layout.json");
    if (panel) BENCH->addControl(panel);

    // 部分控件继续硬编码（比如需要动态创建或复杂逻辑的控件）
    auto dynamicBtn = ButtonBuilder(nullptr, SRect(100, 200, 120, 35))
        .setCaption("Dynamic")
        .build();
    dynamicBtn->setOnClick([](auto btn) { /* 复杂逻辑 */ });
    BENCH->addControl(dynamicBtn);
}
```

- 配置文件和硬编码控件可以在同一个 `testBenchInitialize()` 中混用
- 适用于：渐进式迁移，将稳定部分逐步抽取到配置文件中
- 两者同时存在时，硬编码控件按添加到 Bench 的顺序排在配置控件的上方

### 11.5 迁移路径

```
Phase 1:  test_layout 纯配置文件模式         ← 新建测试，验证功能
              │
Phase 2:  test_label  混合模式               ← 将部分 Label 迁移到配置
              │         (保持原有 + 新增配置)
Phase 3:  test_checkbox 等 纯配置文件模式    ← 完全替换
```

整个迁移过程可逐步进行，随时可以退回到硬编码模式。

## 12. 向后兼容与安全性

### 12.1 向后兼容

- 现有 C++ 创建控件的方式完全不受影响
- LayoutParser 仅作为新增工具，不修改任何现有控件类
- 现有测试用例无需改动
- nlohmann/json 已是子模块，无需新增依赖

### 12.2 安全性

- 所有 `json` 访问均使用 `.value()` 或 `contains()` + 默认值的方式，不会因字段缺失而崩溃
- 错误信息通过 `SDL_LogError/SDL_LogWarn` 输出，不抛出异常
- 配置文件需放置在项目目录中（非用户可上传路径）
- 不执行配置文件中的任意代码（事件处理函数需在 C++ 侧注册）
