# CR20261116001 — 通用内嵌控件 Schema 指引与递归属性行（引擎侧分析 + 设计）

> 状态：**closed（2026-11-19 设计器最终联调通过；设计器/引擎两项均 closed）**
> 来源：`D:\GitSpace\CornerstoneDesigner\Temp\UICornerstone_配合修改清单_2026-11.md` → CR20261116001
> 关联：设计器设计文档 `11_级构型JSON属性动态行设计.md` §5.8 / §8 / §9；`PropertyNames.h`、`docs/schema/declarative-ui.schema.json`、`src/LayoutParser.cpp`、`Button` / `CheckBox`
> 边界：本文档只覆盖引擎侧（schema 注解、parser、持久化契约、测试与手册）；设计器侧通用对象行/递归行 UI 不在本文范围。

## 1. 背景与目标

设计器属性面板目前只按顶层标量属性生成行，宿主控件内部的内嵌控件（如 Button/CheckBox 的 `caption-label`）无法递归出子字段。CR20261116001 已确认契约：**属性级 `x-embedded-control` 注解放在引擎 schema**，设计器据此懒加载目标 `$defs` 并递归生成行。

引擎侧目标：

1. 在 `button.caption-label` 上补 `x-embedded-control` 注解；
2. 为 `check-box` 增加 `caption-label` 属性并补同款注解，使 strict validator 接受导出键；
3. 抽出 Button/CheckBox 共用的内嵌 Label JSON 解析，补齐 `shadow.blur`、`line-height`、`line-spacing-ratio`、`font.style`、`alignment`、`colors` 等字段；
4. 明确宿主转发键与内嵌键的边界（`shadow.blur` 为规范键，运行时写 `text-shadow-blur`；顶层 `text-shadow-blur` 仅 standalone Label 兼容别名，内嵌组 `exclude`）；
5. 不改变控件运行时语义，不新增 C ABI/Binding。

### 1.1 非目标

- 不实现设计器递归行 UI / 懒加载 / 缓存；
- 不补 WinFrame `title-label`（V1.5）；
- 不补 Dialog/ConfirmPopup `confirm-button`/`cancel-button`、ComboBox/TreeView 内部件、Splitter 引用、MenuItem/cellControls/leadingControl；
- 不新增引擎 JSON 导出器（引擎只解析；持久化由设计器模型负责）；
- 不改变 `x-*` 校验规则，不为设计器新增 schema 展开 API。

## 2. 现状与证据

| 项 | 现状 |
|---|---|
| schema `button.caption-label` | `{ "type": "object", "description": "自定义标题 Label 配置" }`，无 `properties`/`x-embedded-control`；strict 下内部键任意 |
| schema `check-box` | 无 `caption-label`，`additionalProperties:false` → 导出该键 strict 报错 |
| `parseButton` | 只解析 `caption` / `font{name,size,style}` / `alignment` / `shadow{enabled,offset}` / `colors{text,text-shadow}`；**不解析** `shadow.blur`、`line-height`、`line-spacing-ratio` |
| `parseCheckBox` | **完全未解析** `caption-label`；仅 `caption` / `font-size` / 颜色 / shadow（宿主级 `applyShadowDecl`） |
| 运行时句柄 | `Button::getPtrProperty` / `CheckBox::getPtrProperty` 均支持 `caption-label` → 内部 Label |
| CheckBox 生命周期 | `createCaption()` 按宿主字段重建；`recreate()`（`setRect` 等触发）前 `captureCaptionState()` 只快照文本/字号/各态色/shadow enabled+offset，**不包含** line-height / line-spacing-ratio / shadow.blur / alignment / font.style |
| 宿主转发 | Button/CheckBox 的 `setFloatProperty` 不处理 `text-shadow-blur`；`setIntProperty` 不处理 `line-height`；`setFloatProperty` 不处理 `line-spacing-ratio` |

结论：schema 与 parser 均需补齐；CheckBox 还需要宿主侧持久字段，否则 recreate 后内嵌 Label 专有字段丢失。

## 3. 契约摘要（引擎侧落地口径）

`caption-label` 的 `x-embedded-control`（Button/CheckBox 共用同一份）：

```json
"x-embedded-control": {
  "schema": "label",
  "handle": "caption-label",
  "persist": "caption-label",
  "group": "内嵌文本",
  "hostForwarded": [
    "caption", "font", "font-size", "colors",
    "shadow.enabled", "shadow.offset.x", "shadow.offset.y"
  ],
  "include": [],
  "exclude": ["text-shadow-blur"],
  "propertyMap": {
    "alignment": "align",
    "shadow.blur": "text-shadow-blur"
  },
  "lazy": true,
  "owned": true,
  "serializable": true
}
```

引擎侧需保证：

- `schema:"label"` 能解析到本地 `$defs.label`；
- `handle` 与 `GetPtr("caption-label")` 一致；
- `persist` 与 JSON 属性名一致，strict validator 接受 `caption-label`（含内部字段）；
- `hostForwarded` 中列出的模型路径，引擎按宿主键处理（不额外生成内嵌行由设计器负责）；
- `propertyMap` 的两条映射在引擎解析路径成立：`alignment` → 内嵌 Label 的 `align` 语义，`shadow.blur` → 内嵌 Label `text-shadow-blur`；
- `exclude:["text-shadow-blur"]` 表示内嵌组不再暴露顶层兼容别名；引擎解析时 `caption-label.text-shadow-blur` 视为非法/忽略并诊断（见 §4.5）。

## 4. 设计

### 4.1 Schema 变更

1. `$defs.button.properties.caption-label`：保留 `type:"object"`，补 `description` 与 `x-embedded-control`（§3 内容）。
2. `$defs.check-box.properties`：新增 `caption-label`，结构与注解同 Button。
3. 不收紧 `caption-label` 的 `properties`/`additionalProperties`：
   - 现有布局可能带 parser 未识别的键，收紧会导致 strict 新失败；
   - 内嵌字段集合由 `$defs.label` + `exclude` 决定，设计器不使用 `caption-label` 自身 properties；
   - 保持向前兼容，未来新增内嵌字段不需要再改 schema。
4. `$defs.label` 不变（已有 `line-height` / `line-spacing-ratio` / `shadow.blur` / `text-shadow-blur`）。
5. `x-*` 为 annotation，`validate_layout` 的实例键校验不受影响；`caption-label` 已在 `PropertyNames.h`（`kCaptionLabel`）。

### 4.2 Parser 抽取：`applyEmbeddedLabelConfig`

新增 `LayoutParser` 私有方法：

```cpp
// 将 caption-label JSON 应用到内嵌 Label；hostShadowOut 非空时回传宿主级 shadow 配置
void applyEmbeddedLabelConfig(Label& label, const json& cl,
                              bool* hostShadowEnabled, SPoint* hostShadowOffset);
```

解析字段（与 standalone Label 对齐，但仅限内嵌允许集）：

| JSON 字段 | 应用 |
|---|---|
| `caption` | `label.setCaption` |
| `font.name` / `font.size` / `font.style` | `setFont` / `setFontSize` / `SetFontStyle` |
| `alignment` | `setAlignmentMode(parseAlignment(...))`（运行时键 `align`） |
| `shadow.enabled` / `shadow.offset.x/y` | 应用到内嵌 Label，并回传宿主（Button/CheckBox 宿主字段优先，见 §4.3） |
| `shadow.blur` | `label.setFloatProperty(kTextShadowBlur, v)`（规范键；运行时 `text-shadow-blur`） |
| `colors.text` / `colors.text-shadow` | 逐键 `applyColorGroupPerKey` 同现有 Button 逻辑 |
| `line-height` | `label.setLineHeight(v)` |
| `line-spacing-ratio` | `label.setLineSpacingRatio(v)` |

不解析（V1 排除）：`text-shadow-blur`（内嵌组别名，见 §4.5）、`font-file` / `font-resource`、`offset`、`expand`、`clickable`、`debug-draw`、`events`、`size`、`rect`、`scale`、`padding`、`context-menu`、`children` 等。

Button 路径：

- 保留现有 `LabelBuilder` 主题默认（字体名/字号来自 `kThemeCatButton`）；
- 原内联解析块替换为 `applyEmbeddedLabelConfig(*label, cl, ...)`；
- `btn->setCaptionLabel(label)` 后，宿主 `shadow.enabled/offset` 仍由宿主字段覆盖（保持既有“button 级配置优先”）。

CheckBox 路径：

- 新增宿主字段与 `applyCaptionLabelConfig(const json&)`（见 §4.3），在 `create()` 前解析；`createCaption()` 应用；
- 或解析期直接调用 `checkBox->getCaption()`（已 create）后 `applyEmbeddedLabelConfig`，再由 `captureCaptionState()` 快照宿主字段——两条路径等价，实施时统一走宿主字段，避免 create 时序差异。

### 4.3 CheckBox 宿主字段与 recreate 稳定性

新增私有字段：

```cpp
bool  m_captionStyleValid = false;   // 是否收到 caption-label 配置（区分默认）
int   m_captionLineHeight = 0;       // 0 = Label 默认
float m_captionLineSpacingRatio = -1.0f; // <0 = Label 默认
float m_captionShadowBlur = 0.0f;
int   m_captionFontStyle = 0;
bool  m_captionAlignValid = false;
AlignmentMode m_captionAlignment = AlignmentMode::AM_MID_LEFT;
```

- `createCaption()`：在现有 builder 链后按 `m_captionStyleValid` 应用上述字段（`setLineHeight` / `setLineSpacingRatio` / `setFloatProperty(kTextShadowBlur)` / `SetFontStyle` / `setAlignmentMode`）。
- `captureCaptionState()`：补充从 `m_caption` 快照上述字段（设计器经 `GetPtr("caption-label")` 直写 Label 后也能在 recreate 保留）。
- `applyCaptionLabelConfig(const json&)`：解析并写入字段；`m_caption` 存在时同步应用。
- 不新增公开 C ABI/Binding；`GetPtr` 句柄不变。

Button 不需要宿主快照：现有实现不重建 caption（`setRect`/`setContentPadding` 只更新 rect）。

### 4.4 属性转发与持久化边界

| 模型路径（`caption-label.` 下） | 引擎落点 | 设计器行为 |
|---|---|---|
| `caption` / `font` / `font-size` / `colors` / `shadow.enabled` / `shadow.offset.x/y` | 宿主键（Button/CheckBox 既有转发） | 不在内嵌组重复生成（`hostForwarded`） |
| `shadow.blur` | 内嵌 Label `text-shadow-blur` | 内嵌组生成行；模型持久化 `caption-label.shadow.blur` |
| `line-height` / `line-spacing-ratio` | 内嵌 Label | 内嵌组生成行；模型持久化同名键 |
| `alignment` | 内嵌 Label `align` | 内嵌组生成行；模型持久化 `caption-label.alignment` |
| `colors.text` / `colors.text-shadow` | 宿主 + 内嵌双同步 | `hostForwarded` 已覆盖，不重复 |
| `text-shadow-blur`（顶层别名） | standalone Label 兼容；内嵌组 `exclude` | 内嵌组不生成；引擎忽略并诊断 |

优先级（同一 JSON 同时出现宿主与内嵌 shadow 时）：

1. 先应用顶层 `shadow`（`parseCommonProperties` → `applyShadowDecl`）；
2. 再应用 `caption-label.shadow.enabled/offset` 到宿主字段（内嵌显式值覆盖顶层）；
3. `shadow.blur` 只写内嵌 Label，不影响宿主 `box-shadow`。

### 4.5 兼容与诊断

- 内嵌组出现 `text-shadow-blur`：忽略，`Platform::Log` 一次性提示“请使用 shadow.blur（standalone Label 兼容别名不适用于内嵌组）”；不影响其他字段解析。
- `caption-label` 非对象：忽略（与现有 Button 行为一致），不报错。
- CheckBox 未 create 时写入：只落宿主字段，`createCaption()` 统一应用。
- 空 `caption-label:{}`：等价于主题默认，不创建额外状态。

### 4.6 与设计器递归行的接口面

引擎只提供：

- schema 注解（设计器读取）；
- `GetPtr("caption-label")` → Label 句柄（已有）；
- 标准属性读写（已有 Label 属性系统）。

引擎不提供行生成、字段过滤、缓存或展开 API。

## 5. 测试计划

新增 `test/test_cr_embedded_label.cpp` + `layouts/cr_embedded_label.json`，注册到 `test/CMakeLists.txt` 并纳入 `layouts/` POST_BUILD 复制条件。

| # | 用例 | 断言 |
|---|---|---|
| E1 | Button `caption-label` 全字段 | `GetPtr` 句柄非空；caption/font(name,size,style)/alignment/shadow.enabled+offset+blur/colors/line-height/line-spacing-ratio 均生效 |
| E2 | CheckBox `caption-label` 全字段 | 同上；`getCaption()` 与 `GetPtr` 同一对象 |
| E3 | CheckBox recreate 稳定性 | 解析后 `setRect` 触发 recreate；上述字段（尤其 blur/line-height/line-spacing-ratio/alignment/font.style）保持 |
| E4 | 设计器直写句柄 | `GetPtr` 后直接 `SetFloat("text-shadow-blur")` / `SetInt("line-height")` / `SetEnum("align")`；recreate 后保持 |
| E5 | 宿主/内嵌 shadow 优先级 | 顶层 `shadow{enabled:false,offset:1,1}` + `caption-label.shadow{enabled:true,offset:3,4}` → 宿主字段为 true/3,4；`blur` 只影响内嵌 Label |
| E6 | strict schema | 含 `check-box.caption-label` 的布局 `--strict` 通过；`caption-label` 内未知键仍按现有宽松行为接受 |
| E7 | 别名诊断 | `caption-label.text-shadow-blur` 不生效、不崩溃，其他字段正常 |
| E8 | 无回归 | Button/CheckBox 顶层 `caption`/`font-size`/`shadow`/`colors` 既有行为不变；`test_colorfixes`、`test_p0_getter` 通过 |
| E9 | 三后端 | SDL3 / SFML / raylib 均通过；C ABI/Binding 无新增符号调用 |

同时跑：`validate_layout layouts/*.json --strict`、SDL3 全量回归（仅允许 4 项历史失败）。

## 6. 用户手册与文档更新计划

| 文档 | 更新 |
|---|---|
| `docs/controls/button.html` | `caption-label` 支持字段表、`shadow.blur`/`line-height`/`line-spacing-ratio` 示例、与宿主 `shadow` 的优先级 |
| `docs/controls/checkbox.html` | 新增 `caption-label` 章节（此前未文档化） |
| `docs/appendix/declarative-syntax.html` | `caption-label` 内嵌控件说明 + `x-embedded-control` 注解用途（面向设计器/高级用户） |
| `docs/appendix/properties.html` | Button/CheckBox 行补内嵌键说明（仅列支持键） |
| `docs/schema/declarative-ui.schema.json` | 注解 + CheckBox 属性（本 CR 核心交付） |
| `README.md` | 控件特性行补“Button/CheckBox 内嵌 caption Label 可编辑/持久化”与新增测试 |
| `design/API_Mapping_Table.md` | 增 CR20261116001 行（无新增 C ABI/Binding） |

截图：`caption-label` 属于文本/字体视觉属性，需按用户手册截图要求补 `button-caption-label.png`、`checkbox-caption-label.png`（默认 vs 修改 font/line-height/shadow.blur），由可复现布局生成；无视觉变化的字段不配图。

## 7. 风险与回退

| 风险 | 处理 |
|---|---|
| CheckBox recreate 丢内嵌字段 | §4.3 宿主字段 + capture 快照；E3/E4 覆盖 |
| schema 收紧导致旧布局 strict 失败 | `caption-label` 保持开放对象，不设 `additionalProperties:false` |
| Button 宿主 shadow 覆盖内嵌配置 | 明确“宿主字段优先”，文档 + E5 |
| 内嵌别名键双重语义 | `exclude` + 忽略诊断；E7 |
| 设计器直写 Label 绕过宿主字段 | capture 从 `m_caption` 快照；E4 |
| 三后端字体度量差异 | E1/E2 断言属性值而非像素尺寸；截图用统一主题 |
| C ABI/Binding 误改 | 无新增符号；E9 回归 |

## 8. 验收对照

| CR 验收项 | 设计落点 |
|---|---|
| Button `caption-label` schema 可递归 | §4.1 |
| CheckBox `caption-label` strict 接受 | §4.1 / E6 |
| `shadow.blur` / `line-height` / `line-spacing-ratio` 可编辑并持久化 | §4.2 / §4.4 / E1~E3 |
| 设计器 `GetPtr` 直写可回放 | §4.3 / E4 |
| 宿主转发不重复、不冲突 | §4.4 / E5 |
| 无新增 C ABI/Binding | §4.3 / E9 |
| 保存/打开/导出（设计器侧） | `persist`/`serializable` 契约 + strict（E6），联调由设计器验证 |
| 三后端与全量回归 | §5 |

## 9. 实施顺序

1. schema：Button 注解 + CheckBox `caption-label`；
2. `applyEmbeddedLabelConfig` 抽取，Button 改造；
3. CheckBox 宿主字段 + `createCaption`/`captureCaptionState`/`applyCaptionLabelConfig`；
4. 新测试 + 布局样本 + CMake；
5. 用户手册/schema 校验/README/API 映射；
6. 三后端验证 → 设计器联调 → 关闭 CR 引擎项。

## 10. 评审意见处理（2026-11-18，设计器）

设计器结论：**有条件通过**；B1/B2 必须闭合，B3/B4 需明确规则与测试；N1~N6 为非阻塞。处理如下。

### 10.1 B1 CheckBox alignment 被 adjustSpaceAssignment 覆盖

采用方案 1（支持对齐，显式优先）：

- 新增 `m_captionAlignValid` / `m_captionAlignment`；
- `caption-label.alignment` 解析后写 `m_captionAlignment` 并置 `m_captionAlignValid=true`；
- `createCaption()` 使用该对齐；`adjustSpaceAssignment()` 仅在 `!m_captionAlignValid` 时按 `layout` 设默认 `AM_MID_LEFT/AM_MID_RIGHT`，不再无条件覆盖；
- 设计器直写 `GetPtr("caption-label")->align` 后，`captureCaptionState()` 回读 `getAlignmentMode` 并置 valid，recreate 保持。

### 10.2 B2 CheckBox font.name 在 recreate 后丢失

采用方案 1（补齐宿主快照）：

- 新增 `m_captionFontName`（默认 `HarmonyOS_Sans_SC_Regular`）与 `m_captionFontStyle`；
- `caption-label.font.name/style` 解析后同时写宿主字段并应用到 Label；`font.size` 走既有 `m_captionSize`；
- `createCaption()` 用 `m_captionFontName`（不再硬编码）；
- `captureCaptionState()` 回读 `getFontName()` / `getFontStyle()`，recreate 稳定；
- `parseCheckBox` 同步补宿主 `font.name` 解析（仅 `font-size` 的旧行为补齐）。

### 10.3 B3 顶层键与内嵌键优先级

统一为 **inner wins（内嵌显式值覆盖顶层）**：

- 解析顺序：先顶层 `caption`/`font`/`font-size`/`colors`/`shadow`，再 `caption-label` 同名字段；
- CheckBox 走宿主字段时同样先写宿主、后写内嵌；
- 覆盖 `caption`、`font.name/size/style`、`colors.text/text-shadow`、`alignment`；
- 新增 E10 覆盖三类冲突。

### 10.4 B4 `shadow.blur` 可见性

- 引擎保持**字面语义**：`shadow.blur` 只写内嵌 Label `text-shadow-blur`，不自动改变 `shadow.enabled`；
- 可见性条件与 standalone Label 一致：`shadow.enabled=true` 且 text-shadow 颜色 alpha>0（Label 默认 normal 色 alpha=120）；
- **设计器内嵌行负责**：用户编辑 `shadow.blur` 时，若尚未启用则同时写 `shadow.enabled=true`（与 standalone 的自动启用策略一致）；
- 新增 E11：仅 blur、`enabled=false` → 不可见；补 `enabled=true` → 可见。

### 10.5 非阻塞建议

| # | 处理 |
|---|---|
| N1 `font.style` UI 路径 | 保持 `hostForwarded:["font"]`；文档标注 `font.style` 仅 JSON/高级用户可用（不在 V1 内嵌行） |
| N2 开放对象诊断 | 对 `caption-label.text-shadow-blur`（内嵌别名）一次性诊断；文档标注内嵌键集合以 `$defs.label` + `exclude` 为准 |
| N3 CheckBox 字段回读 | `captureCaptionState()` 回读 font.name/style、alignment、line-height、line-spacing-ratio、text-shadow-blur |
| N4 单一解析器 | Button/CheckBox 共用 `applyEmbeddedLabelConfig` + 字段映射表；CheckBox 在 createCaption 前后统一走宿主字段 |
| N5 persist/serializable | schema 注解说明 `persist:false` 必须 `serializable:false`；V1 首批全 true |
| N6 文档一致性 | Button/CheckBox 的 `caption-label` 支持键表与 parser 实际集合逐项核对；手册同步字段表、默认值与 `font.style` 限制 |

### 10.6 测试补充

在 E1~E9 基础上增加：

- E10：顶层/内嵌优先级（caption/colors/alignment/font）；
- E11：仅 `shadow.blur`、`enabled=false/true` 的可见性；
- E12：CheckBox recreate 后 `font.name`/`alignment` 保持；
- E13：`caption-label:{}` / 非对象 / 未知内嵌键宽松接受、不崩溃；
- E14：`GetPtr` 直写 `align`/`text-shadow-blur`/`line-height` 后 recreate 保持。

## 11. 实施记录（2026-11-19）

### 11.1 落地文件

- `docs/schema/declarative-ui.schema.json`：`button.caption-label` 增加 `x-embedded-control`；`check-box` 新增 `caption-label` 属性与同款注解（开放对象，不收紧 additionalProperties）。
- `src/LayoutParser.cpp`：
  - Button：`caption-label` 全字段（caption/font(name,size,style)/alignment/shadow/colors/line-height/line-spacing-ratio/blur），内嵌优先；
  - CheckBox：新增 `caption-label` 解析（宿主字段 + 快照），补顶层 `font.name`；内嵌 `text-shadow-blur` 别名一次性诊断；
  - `parseFontName` 补 schema kebab 字体名（`maplemono-nf-cn-regular` 等），避免 schema 合法值被当成未知字体。
- `include/Label.h`：新增 `getFontName()`（CheckBox 快照回读；评审修正 1）。
- `include/CheckBox.h` / `src/CheckBox.cpp`：
  - `m_captionAlignValid/m_captionAlignExplicit/m_captionAlignment`；`adjustSpaceAssignment` 仅在未显式设置时取 layout 默认；
  - `m_captionFontStyleSet/m_captionFontStyle`、`m_captionLineHeight`、`m_captionLineSpacingRatio`、`m_captionShadowBlur` 宿主快照；
  - `createCaption` 使用 `m_fontName` 与快照；`captureCaptionState` 回读 font.name/style、alignment、line-height/spacing、blur；
  - **评审修正 2**：capture 不无条件置 `m_captionAlignValid`，仅“显式写入（`m_captionAlignExplicit`）或读回值 != layout 默认”时置位；
  - 新增 `setCaptionAlignment/FontStyle/LineHeight/LineSpacingRatio/TextShadowBlur`。
- 测试：`test/test_cr_embedded_label.cpp` + `layouts/cr_embedded_label.json` + CMake。

### 11.1.1 联调回归修复（2026-11-19）

设计器 GetPtr 直写 `align` 后立即读回被重置为 `mid-left`：

- 根因：Label 的 `onPropertyChanged` → `setBoxSize/adjustSpaceAssignment` 在 `m_captionAlignValid==false` 时按 layout 默认重置对齐；
- 修复：新增 `CheckBox::syncCaptionAlignmentFromLabel()`，在 caption 回调与 `adjustSpaceAssignment` 入口检查当前 Label 对齐：与 layout 默认不同（或已显式）时置 `m_captionAlignValid=true` 并同步 `m_captionAlignment`；
- 测试新增 E15：`SetEnum("align","bottom-right")` 立即读回 + `setRect` recreate 后保持；三后端 53/53。

### 11.2 验证结果

- `test_cr_embedded_label`：SDL3 / SFML / raylib 均 **53/53**（E1~E14：全字段、inner wins、CheckBox recreate、GetPtr 直写、shadow.blur 字面语义、空对象宽松）。
- 回归：SDL3 全量回归仅 4 项历史失败，无新增；`validate_layout --strict` PASS。
- 影响：CheckBox 类布局/vtable 变化，DLL/头文件/C++ 使用方需同批重编译；C ABI/Binding 无新增函数。
- Release DLL SHA256：`59e222ed90247a3234c8b2989f4ad616184620bc6cdf00d9697392cb79808ad6`；已同步设计器 `subModules/UICornerstone`。`/XD` 不能排除 `.git` 文件（submodule 的 `.git` 是 gitdir 指针文件），需用 `/XF .git`；本次同步后已恢复。

## 修订记录

- 2026-11-16 v1：引擎侧首版设计；确认 schema 开放对象策略、Button/CheckBox 共用解析、CheckBox recreate 持久字段、shadow 优先级与别名诊断；待评审。
- 2026-11-18 v2：按设计器评审闭合 B1（CheckBox alignment 显式优先）、B2（font.name 宿主快照）；明确 B3 inner wins、B4 blur 字面语义 + 设计器负责自动启用；补 E10~E14；待确认后实施。
- 2026-11-19 v3（实施）：schema/parser/CheckBox 快照与两处评审修正落地；三后端 50/50；补 §11；引擎项 closed。
- 2026-11-19 v4（联调回归）：修复 GetPtr 直写 align 被 layout 默认覆盖；E15 + 三后端 53/53；重新交付。
- 2026-11-19 v5（关闭）：设计器恢复 CheckBox alignment 断言并通过（save/open 保持）；CR 设计器/引擎两项 closed。
