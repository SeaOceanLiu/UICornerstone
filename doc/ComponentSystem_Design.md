# 组件系统（Component System）设计文档

## 1. 概述

组件系统允许在 JSON 布局中通过组合现有控件（Panel、EditBox、Button 等）定义可复用的 UI 组件模板，并在多处按需实例化，支持属性注入和事件重映射。

### 1.1 目标

- **复用**：将一组控件打包为模板，一次定义、多处引用
- **定制**：实例化时注入属性（占位文本、按钮文字等）
- **交互**：组件内部事件能够映射到外部 handler
- **渐进**：完全基于现有 `LayoutParser` + 已有控件，不改动 C++ 控件类

### 1.2 非目标

- 不支持组件继承（`extends`），未来可按需添加
- 不支持组件间通信（props 向下，events 向上，单向数据流即可）

---

## 2. JSON 语法设计

### 2.1 根级 `"components"` 区块

```json
{
  "components": {
    "<组件名>": {
      "props": {
        "<属性名>": { "type": "string|number|bool", "default": <默认值> }
      },
      "template": { <控件定义> }
    }
  },

  "layouts": [ ... ]
}
```


| 字段              | 类型    | 必填 | 说明                                                          |
| ----------------- | ------- | ---- | ------------------------------------------------------------- |
| `components`      | object  | 否   | 组件定义集合，key 为组件名                                    |
| `props`           | object  | 否   | 声明组件对外暴露的可注入属性                                  |
| `props.*.type`    | string  | 是   | 属性类型：`string` / `number` / `bool`                        |
| `props.*.default` | 同 type | 否   | 缺省时的默认值                                                |
| `template`        | object  | 是   | 控件定义（`type` + `children` 等），**type 不能是已有控件名** |

### 2.2 模板内插值语法

模板内使用 `{{propName}}` 引用 props 中的值，支持在以下字段中使用：

- `caption`
- `placeholder`
- `text`
- `rect` 中的 `x` / `y` / `w` / `h` 字符串值

### 2.3 事件重映射

组件内部用约定前缀 `_comp_` 标记内部事件名，实例化时通过实例的 `events` 映射到外部 handler。

```json
// 模板
"events": { "onClick": "_comp_onSearch" }

// 实例化
{
  "type": "SearchBar",
  "events": { "onSearch": "handleSearch" }
}
```

映射规则：`_comp_onSearch` → 查找实例 events 中 key 为 `onSearch` 的 handler。

### 2.4 实例化引用

在 `layouts` 的任意 `children` 节点中，`type` 值为已注册的组件名即触发实例化：

```json
{
  "type": "SearchBar",
  "id": "mySearch",
  "rect": { "x": 50, "y": 50, "w": 400, "h": 40 },
  "placeholder": "Type keyword...",
  "buttonText": "Search",
  "events": { "onSearch": "handleSearch" }
}
```


| 字段     | 说明                                                |
| -------- | --------------------------------------------------- |
| `type`   | 组件名（已在`components` 中注册）                   |
| `id`     | 实例 ID，注入到根 Panel                             |
| `rect`   | 尺寸，注入到根 Panel                                |
| 其他     | 同名 props 值（如`placeholder`），覆盖模板 defaults |
| `events` | 事件重映射表，key = 去掉`_comp_` 前缀的内部事件名   |

---

## 3. 处理流程

### 3.1 总体步骤

```
LayoutParser::parseLayoutFile()
├── parseComponents()         ← 新增：注册所有组件模板
└── parseLayouts()
    └── parseControl()
        ├── 已知控件 → parsePanel/parseButton/...
        └── 未知 type → 查 m_components
                 └── instantiateComponent()  ← 新增：实例化组件
```

### 3.2 parseComponents()

```
parseComponents(j)
  │
  ├─ 校验 j 包含 "components" 且为 object
  │
  ├─ 遍历每个 entry:
  │    ├─ 组件名不能与已有控件 type (Button/Panel/EditBox/...) 重复
  │    ├─ template 必须存在且为 object
  │    ├─ props 验证每个 entry 有 type 字段
  │    ├─ 记录模板在原始文件中的行号范围 → m_componentSourceLines
  │    └─ 存入 m_components[name] = { props, template }
  │
  └─ 全部注册完成后，二次遍历检测模板内部引用的组件名是否已注册
       ├─ 未注册 → logWarn
       └─ 已注册 → 记录依赖关系用于循环引用检测
```

### 3.3 instantiateComponent()

```
instantiateComponent(name, instanceJ, parent)
  │
  ├─ 检测循环引用：name 是否已在 m_instantiationStack 中？
  │    ├─ 是 → logWarn + return nullptr
  │    └─ 否 → m_instantiationStack.push_back(name)
  │
  ├─ 从 m_components 获取模板定义 + m_componentSourceLines 获取定义行号
  │
  ├─ pushJsonPath("component:" + name + " [line " + lineStart + "-" + lineEnd + "]")
  │
  ├─ 深拷贝 template JSON
  │
  ├─ 替换所有 {{propName}} 为 instanceJ 中的值（或 props.default）
  │   ├─ 属性不存在且无 default → logWarn + 保留原文
  │   └─ 类型转换：string→number/bool 按 props.type
  │
  ├─ 注入实例属性：id → 根节点、rect → 根节点
  │
  ├─ 事件重映射（详见 3.4）
  │
  ├─ ID 前缀化（详见 3.5）
  │
  ├─ 走正常解析路径 parseControl(modifiedTemplate, parent)
  │   ├─ 解析时遇到嵌套组件 → 递归调用 instantiateComponent()
  │   └─ 解析过程通过 m_currentJsonPath 带双路径上下文
  │
  └─ popJsonPath() + m_instantiationStack.pop_back()
```

### 3.4 事件重映射详细规则

1. 深拷贝后递归遍历整个模板 JSON 树
2. 对每个包含 `events` 的节点：
   - 遍历 `events` 的每个 entry
   - 如果 value 以 `_comp_` 开头：
     - 截取前缀后的事件名 key：`_comp_onSearch` → `onSearch`
     - 在 `instanceJ["events"]` 中查找 `onSearch`
     - 找到则替换为该 handler 名；找不到则保留 `_comp_onSearch`
3. 实例的 `events` 本身不会被注入到根节点 event，只做映射查找表

### 3.5 ID 前缀化规则

- 模板中显式声明的 `id`（如 `"input"`, `"btn"`）→ 改为 `"<实例id>__input"`, `"<实例id>__btn"`
- 如果实例没有 `id`，则生成一个唯一前缀：`"_comp_<uuid>__"`
- 确保全局 ID 唯一，支持 `findControlById` 查找

---

## 4. 数据类型与约束

### 4.1 props.type 转换规则


| type     | JSON 类型 | 注入示例  | 说明                                           |
| -------- | --------- | --------- | ---------------------------------------------- |
| `string` | string    | `"hello"` | 直接替换                                       |
| `number` | number    | `42`      | 转字符串后替换`{{prop}}`，或用于 rect 数值字段 |
| `bool`   | bool      | `true`    | 转`"true"` / `"false"` 替换                    |

### 4.2 约束

- **模板不能自引用**（A 引用 B，B 引用 A）→ 解析时检测循环，抛出错误
- **组件名大小写敏感**，首字母大写惯例（如 `SearchBar`），不与现有控件名冲突
- **props 声明必须在模板使用前定义**，未声明的 props 在注入时给出 warning 并跳过
- **仅支持一层插值**：`{{prop}}`，不支持 `{{prop.nested}}` 或表达式
- **嵌套组件**：模板的 `children` 中可引用其他已注册组件（通过 `instantiateComponent` 递归实现）
- **实例缺省尺寸**：若实例未提供 `rect`，将 `w` 和 `h` 的百分比传递为父容器可用尺寸带入计算

---

## 5. 错误处理

### 5.1 错误场景

| 场景                                         | 行为                                     |
| -------------------------------------------- | ---------------------------------------- |
| 组件名与已有控件 type 冲突                   | `logWarn` + 跳过注册                     |
| 模板中`{{unknownProp}}` 未声明               | `logWarn` + 保留原文 `{{unknownProp}}`   |
| 实例未提供必要 props（无 default）           | `logWarn` + 保留原文 `{{propName}}`      |
| 事件映射`_comp_onXxx` 在实例 events 中找不到 | `logWarn` + 保留 `_comp_onXxx`           |
| 循环引用（A↔B）                             | `logWarn` + 中断解析（该组件不可用）     |
| 未注册的 type 且非已知控件                   | 走现有未知 type 逻辑（`logWarn` + 跳过） |

### 5.2 行号追溯

组件模板在 `components` 区块中定义，实例化时深拷贝模板 JSON 并在内存中做属性注入/事件重映射。展开后的节点不在原始文件中有对应行号，因此需追溯回原始定义位置。

**实现方法**：

1. **注册时记录定义位置**：`parseComponents()` 在将模板存入 `m_components` 的同时，记录该模板在原始文件中的行号范围（起始行 + 结束行）到 `m_componentSourceLines`：
   ```cpp
   struct ComponentSource {
       string name;
       int lineStart;   // components.SearchBar 的起始行号
       int lineEnd;     // template 结尾的行号
   };
   ```

2. **实例化时绑定位置信息**：`instantiateComponent()` 在展开模板时，将每个子节点的错误上下文关联到原始定义位置。

3. **报错格式**：
   - 组件定义阶段的错误（props 校验等）：`components.SearchBar.props.buttonText: ... (line 42)`
   - 实例化阶段的错误（属性注入、事件映射）：`instantiate SearchBar at layouts[0].children[1]: ... (line 120)`
   - 组件内部模板解析错误：`` component `SearchBar` (defined at line 35) instantiated at layouts[0].children[1] (line 120): ... ``

4. **`m_currentJsonPath` 管理**：进入组件实例化时，`pushJsonPath("component:SearchBar")` + `pushJsonPath("instance:layouts[0].children[1]")`；处理完弹出。这样 `logWarn` 可以同时看到定义位置和实例位置。

> **设计原则**：所有报错信息必须引用原 `test_layout_advanced.json` 中的行号，不引用展开后的虚拟位置。

---

## 6. 完整示例

### 6.1 输入 JSON

```json
{
  "theme": { "colors": { "primary": "#0078D4FF" } },

  "components": {
    "SearchBar": {
      "props": {
        "placeholder": { "type": "string", "default": "Search..." },
        "buttonText":  { "type": "string", "default": "Go" }
      },
      "template": {
        "type": "Panel",
        "children": [
          {
            "type": "EditBox",
            "id": "input",
            "rect": { "x": 0, "y": 0, "w": "75%", "h": 40 },
            "placeholder": "{{placeholder}}",
            "events": { "onEnter": "_comp_onSearch" }
          },
          {
            "type": "Button",
            "id": "btn",
            "rect": { "x": "78%", "y": 0, "w": "22%", "h": 40 },
            "caption": "{{buttonText}}",
            "events": { "onClick": "_comp_onSearch" }
          }
        ]
      }
    }
  },

  "layouts": [
    {
      "type": "Panel",
      "children": [
        {
          "type": "Label",
          "rect": { "x": 10, "y": 10 },
          "caption": "Search Demo:"
        },
        {
          "type": "SearchBar",
          "id": "search1",
          "rect": { "x": 10, "y": 40, "w": 500, "h": 40 },
          "placeholder": "Type keyword...",
          "buttonText": "Search",
          "events": { "onSearch": "handleSearch" }
        }
      ]
    }
  ]
}
```

### 6.2 实例化后等价 JSON（开发者视角）

```json
{
  "theme": { ... },

  "layouts": [
    {
      "type": "Panel",
      "children": [
        { "type": "Label", ... },
        {
          "type": "Panel",
          "id": "search1",
          "rect": { "x": 10, "y": 40, "w": 500, "h": 40 },
          "children": [
            {
              "type": "EditBox",
              "id": "search1__input",
              "rect": { "x": 0, "y": 0, "w": "75%", "h": 40 },
              "placeholder": "Type keyword...",
              "events": { "onEnter": "handleSearch" }
            },
            {
              "type": "Button",
              "id": "search1__btn",
              "rect": { "x": "78%", "y": 0, "w": "22%", "h": 40 },
              "caption": "Search",
              "events": { "onClick": "handleSearch" }
            }
          ]
        }
      ]
    }
  ]
}
```

### 6.3 C++ handler 注册

```cpp
g_parser.registerHandler("handleSearch", [](shared_ptr<Control>) {
    SDL_Log("Search triggered!");
});
```

---

## 7. 变更清单

### 7.1 LayoutParser 新增


| 方法                                                                       | 可见性  | 说明                                        |
| -------------------------------------------------------------------------- | ------- | ------------------------------------------- |
| `parseComponents(const json& j)`                                           | private | 解析根级`"components"`，填充 `m_components` |
| `instantiateComponent(const string& name, const json& j, Control* parent)` | private | 实例化组件模板，返回解析后的根控件          |
| `replacePlaceholders(json& node, const json& props)`                       | private | 递归替换`{{propName}}`                      |
| `remapEvents(json& node, const json& instanceEvents)`                      | private | 递归重映射`_comp_xxx` 事件                  |
| `prefixIds(json& node, const string& prefix)`                              | private | 递归前缀化子控件 ID                         |

### 7.2 LayoutParser 新增成员


| 成员                      | 类型                                   | 说明                                       |
| ------------------------- | -------------------------------------- | ------------------------------------------ |
| `m_components`            | `unordered_map<string, json>`          | 组件名 → 模板定义（已校验）               |
| `m_componentSourceLines`  | `unordered_map<string, ComponentSource>` | 组件名 → 在原始文件中的行号范围 |
| `m_instantiationStack`    | `vector<string>`                       | 递归实例化栈，检测循环引用                 |

```cpp
struct ComponentSource {
    string name;
    int lineStart;
    int lineEnd;
};
```

### 7.3 LayoutParser 修改


| 位置                              | 修改                                                                                                                                                                                    |
| --------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `LayoutParser::parseLayoutFile()` | 解析`"components"` 后调用 `parseComponents()`                                                                                                                                           |
| `LayoutParser::parseControl()`    | 遇到未知 type 时，在 logWarn/return nullptr 前先查`m_components`，找到则调用 `instantiateComponent()`                                                                                  |
| `LayoutParser::clear()`           | 新增`m_components.clear()`, `m_componentSourceLines.clear()`, `m_instantiationStack.clear()`                                                                                            |

**热加载支持**：`HotReloader` 检测到 JSON 文件变更后调用 `reloadLayout()`，该函数会依次：
1. `g_parser.clear()` — 清理所有组件注册和实例
2. `BENCH->removeAllControls()` — 清除旧控件树
3. `g_parser.parseLayoutFile(...)` — 重新解析（重新注册组件）
4. `BENCH->addControl(root)` — 重建 UI

> **注意**：组件模板本身存储在 `m_components`（C++ 内存中），无需额外文件监视。模板变更通过 JSON 文件整体重载生效，子控件 ID 重新生成，旧控件树完整替换。此流程与现有热加载机制完全一致。

### 7.4 新增文件

- 无。所有变更在 `LayoutParser.h` / `LayoutParser.cpp` 内完成

---

## 8. 设计决策记录

以下决策对应 v1 实现范围，基于架构评审讨论形成。

| # | 议题 | 决策 | 理由 |
|---|------|------|------|
| 1 | **嵌套组件** | ✅ 支持。`instantiateComponent()` 递归调用 | 模板 `children` 中可引用其他已注册组件，通过 `m_instantiationStack` 检测循环引用 |
| 2 | **百分比尺寸传递** | ✅ 实例未提供 `rect` 时，`w`/`h` 的百分比值以父容器可用尺寸带入计算 | 根 Panel 的 `rect` 优先用实例提供的值，缺省时父 Panel 通过 `resized()` 传入可用尺寸 |
| 3 | **enabled / visible 覆盖** | ⏸ 暂不特殊处理。现有解析逻辑已覆盖子控件初始状态 | 实例化时模板自带的状态属性不变，如需覆盖可在 props 中增加 `enabled`/`visible` 参数 |
| 4 | **Slot 插槽** | ⏸ 标记为**下一阶段**（v2） | 当前需求（搜索栏、工具栏、卡片等）用 props + 事件重映射已足够覆盖 |
| 5 | **热加载** | ✅ 支持。通过 JSON 文件整体重载触发 | 现有 `HotReloader` → `reloadLayout()` 流程自然覆盖组件模板更新 |
| 6 | **行号追溯** | ✅ 组件内部错误同时报告**定义位置**和**实例化位置** | `m_componentSourceLines` 记录模板在原始文件中的行号范围，`m_currentJsonPath` 叠加双路径 |
