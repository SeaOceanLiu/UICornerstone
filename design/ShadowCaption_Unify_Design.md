# ShadowCaption_Unify_Design — 文本/阴影配置统一（CheckBox 补齐 + Button/CheckBox/WinFrame 对齐 Label）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-09-21）——**#9**（P1 CheckBox caption 字符串分发）、**#10**（P1 CheckBox `text-shadow-enable`）、**#12**（P0 shadow 配置 schema/运行时统一）
> 状态：**已放行，已实施**（复核：`CornerstoneDesigner/Temp/ClipChildren_ShadowCaption_复核意见.md`，2026-09-21 通过；**方案修正：`text-shadow-enable` 别名删除**）
> **实施结果（2026-09-21）**：
> 1. **别名删除完成**：`kTextShadowEnable`/`kJsonEnableTextShadow` 常量删除；Button/CheckBox 统一 `kShadow`；布局迁移（all_controls.json → `shadow.enabled`）；测试迁移（test_p0_getter → "shadow"）；文档全量迁移（button/checkbox/winframe/index/properties/declarative-syntax/API_Mapping_Table）——全库零残留。
> 2. **实现修正（重要）**：原设计"单一事实来源 = 内部 Label"在实现中不可行——**CheckBox caption 随 `recreate()`（setRect/attach 路径）重建**、Button caption 经 `setCaptionLabel` 替换 → 仅存 Label 会丢失解析期状态。修正为 **宿主字段存储 + 内部 Label 为渲染载体**（与 Button 既有 `m_enableTextShadow` 模式一致）：Button/CheckBox 宿主增 `m_shadowOffset`（CheckBox 另有 `m_enableTextShadow`），(重)建/替换 caption 时应用；WinFrame titleLabel 不重建，直接转发。
> 3. **Parser**：新增 `applyShadowDecl()` 辅助（parseCommonProperties 统一调用 + parseWinFrame 显式调用——win-frame 不走 common）；parseButton 移除旧 `enableTextShadow` 死分支。
> 4. **schema**：`$defs/shadow` 抽取，label 改 $ref + button/check-box/win-frame 新增 $ref；button `text-shadow-enable` 删除；`validate_layout --strict` 三布局 PASS。
> 5. **测试**：test_colorfixes 扩展（#9 caption 往返/#10 CheckBox shadow/三控件 kShadow+offset/JSON shadow 解析×3）**37 项全 PASS**；test_p0_getter PASS；14 项回归全绿。
> 背景：#9/#10 为类型级缺口补齐；#12 为统一方案（落地后设计器删除按类型分派的"统一三行"绕行段，恢复 schema 声明式展开）

---

## 1. 问题与目标

| # | 现象 | 目标 |
|---|---|---|
| #9 | 运行时 `SetString("caption", v)` 写 CheckBox 无效（无分发；仅工厂/JSON 直控内部 Label） | CheckBox 补 `setStringProperty/getStringProperty("caption")`（与 Button 对齐） |
| #10 | CheckBox 有 `textShadow.*` 色槽但**无阴影开关**（caption 的 shadow 恒 false） | CheckBox 支持 `text-shadow-enable`（与 Button 对齐）+ schema 补键 |
| #12 | shadow 配置三类（Button/CheckBox/WinFrame）**无统一 schema/运行时口径**——设计器按类型硬编码三条路径（button：text-shadow-enable + caption 直控 offset；win-frame：title 直控；check-box 待 #10）；Button 另有 `text-shadow-enable` 与 `shadow.enabled` 双语义冗余 | schema：button/check-box/win-frame 补 `shadow` 对象（与 label 一致）；运行时：三类 `kShadow`/`kShadowOffsetX/Y` 转发内部 Label——应用/工具零类型知识 |

## 2. 现状核实（源码事实）

| 机制 | 位置 | 状态 |
|---|---|---|
| Label 统一键 | `kShadow`（Label.cpp:611 set / :685 get）、`kShadowOffsetX/Y`（:629-630 set / :702-703 get） | ✅ 已有（单一目标实现） |
| Label C++ 访问器 | `m_shadowEnabled` / `m_shadowOffset`（Label.h:51/55，**无公开 getter**） | 需补（#12 读回） |
| Button 现状 | `kTextShadowEnable` set/get ✓（Button.cpp:379/384 → `setTextShadowEnable`，:297-302 设 `m_enableTextShadow` + caption `setShadow`）；**shadow offset 无转发**（设计器经 caption-label 直控） | #12 补 kShadow 别名 + offset 转发 |
| CheckBox 现状 | 无 `setStringProperty/getStringProperty`（#9）；无 `kTextShadowEnable`/`kShadow`（#10/#12）；caption 构建 `.setShadow(false)`（CheckBox.cpp:66 区域） | 补 #9/#10/#12 |
| WinFrame 现状 | `kTitle` set/get ✓（WinFrame.cpp:453-455/481）；title Label 无 shadow 转发 | #12 补 |
| schema | `label.shadow` 对象（`{enabled, offset:{x,y}}`）✓；`button.text-shadow-enable` ✓（无 shadow）；`check-box`/`win-frame` 两者皆无 | #10/#12 补 |
| LayoutParser | `parseLabel` 解析 shadow（:413-415）；`parseCommonProperties`（含 button/check-box/win-frame 共用）**无 shadow 解析** | #12 补通用解析 |

## 3. 架构选择与关键设计决策

### 3.1 运行时：统一转发内部 Label（决策：单一事实来源，宿主零新状态）

| 属性键 | Button | CheckBox | WinFrame |
|---|---|---|---|
| `kShadow`（bool，主键） | → `setTextShadowEnable(v)`（与 `kTextShadowEnable` **同目标**；两者互为别名） | → `m_caption->setShadow(v)`（`kTextShadowEnable` 同为别名，#10） | → `m_titleLabel->setShadow(v)` |
| `kTextShadowEnable`（别名，兼容/对齐） | 既有 ✓ | 新增（同 `kShadow` 目标，#10） | —（不提供） |
| `kShadowOffsetX/Y`（float） | → `m_caption->setShadowOffset({x,y})`（保留另一分量） | → `m_caption->setShadowOffset(...)` | → `m_titleLabel->setShadowOffset(...)` |
| 读回（get*） | 内部 Label 为唯一来源 | 同 | 同 |

决策点：
- **单一事实来源 = 内部 Label**：宿主不复制 shadow 状态（避免双态失同步）；`kShadow` 与 `kTextShadowEnable` 读写同一目标（无冲突）。
- **读回经 Label 访问器**：Label 补两个公开 getter（`bool isShadowEnabled() const` / `SPoint getShadowOffset() const`，内联）——宿主转发读；不引入宿主字段。
- **caption/title 为空的边界**：无内部 Label 时（如空文本 Button）shadow 键 set/get 返回 0（不崩）；有 caption 后自动可用。
- **不引入 `shadow` × 控件级冲突**：Button 现有 `m_enableTextShadow` 字段保留（`setTextShadowEnable` 内部仍维护，GetBool `text-shadow-enable` 读该字段；`kShadow` 读同一字段）——行为等价。

### 3.2 schema：抽取 `$defs/shadow` 单一来源（决策：四段统一 $ref）

```json
"shadow": {
  "type": "object",
  "description": "阴影 {enabled, offset:{x,y}}",
  "properties": {
    "enabled": { "type": "boolean", "x-default": "false" },
    "offset": { "type": "object",
      "properties": { "x": {"type":"number","x-default":"1"}, "y": {"type":"number","x-default":"1"} },
      "additionalProperties": false }
  },
  "additionalProperties": false
}
```

- `label.shadow` 由内联改为 `$ref`（语义等价）；`button`/`check-box`/`win-frame` def 增加 `"shadow": { "$ref": "#/$defs/shadow" }`。
- **Button `text-shadow-enable` 保留**（既有布局兼容；运行时别名）。
- **check-box 是否同加 `text-shadow-enable`**：引擎建议**仅加 `shadow`**（统一主键；避免面板出现双开关）；`text-shadow-enable` 运行时支持为别名（直接 API/旧布局可用）——**待评审确认**。

### 3.3 LayoutParser：通用 `shadow` 解析（决策：parseCommonProperties 一处，四类型自动生效）

```cpp
// parseCommonProperties 末尾（applyFontDecl 前后均可）
if (j.contains(PropertyNames::kShadow) && j[PropertyNames::kShadow].is_object()) {
    const json& sh = j[PropertyNames::kShadow];
    if (sh.contains(PropertyNames::kEnabled) && sh[PropertyNames::kEnabled].is_boolean())
        ctrl->setBoolProperty(PropertyNames::kShadow, sh[PropertyNames::kEnabled].get<bool>() ? 1 : 0);
    if (sh.contains(PropertyNames::kJsonOffset) && sh[PropertyNames::kJsonOffset].is_object()) {
        float ox = ..., oy = ...;
        ctrl->setFloatProperty(kShadowOffsetX, ox); ctrl->setFloatProperty(kShadowOffsetY, oy);
    }
}
```

- 经属性系统转发 → 声明了 `shadow` 的类型（label/button/check-box/win-frame）按 §3.1 目标生效；未声明者 schema 不放行（validate 保证）。
- `parseLabel` 既有 shadow 分支**保留**（幂等；避免动已发布解析路径）——评审可决定后续清理。

### 3.4 #9：CheckBox caption 字符串分发（决策：属性分发 + 布局自动刷新）

```cpp
int CheckBox::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kCaption) == 0) {
        if (m_caption) { m_caption->setCaption(value ? value : ""); return 1; }   // Label onPropertyChanged → setBoxSize/adjustSpaceAssignment 自动刷新
        return 0;
    }
    return ControlImpl::setStringProperty(prop, value);
}
int CheckBox::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kCaption) == 0) {
        if (m_caption) { out = m_caption->getCaption().c_str(); return 1; }
        return 0;
    }
    return ControlImpl::getStringProperty(prop, out);
}
```

- 布局刷新链路：CheckBox 的 caption Label 已挂 `setOnPropertyChanged`（createCaption，:65-68 区域）→ `setBoxSize` + `adjustSpaceAssignment` 自动执行 ✓。
- 同型排查：WinFrame `kTitle` 已有 ✓；Button `kCaption` 已有 ✓；ComboBox/NUD（自绘/自身文本）✓ —— 无其他缺口。

## 4. API 设计

- 核心库：
  - Label.h：`bool isShadowEnabled() const` / `SPoint getShadowOffset() const`（内联）。
  - Button.cpp：`setBoolProperty/getBoolProperty` 增 `kShadow` 别名；`setFloatProperty/getFloatProperty` 增 `kShadowOffsetX/Y` 转发。
  - CheckBox.h/.cpp：`setStringProperty/getStringProperty`（#9）；`setBoolProperty/getBoolProperty` 增 `kShadow` + `kTextShadowEnable`（#10/#12）；`setFloatProperty/getFloatProperty` 增 offset 转发。
  - WinFrame.cpp：`setBoolProperty/getBoolProperty` + `setFloatProperty/getFloatProperty` 增 `kShadow`/offset 转发。
  - LayoutParser.cpp：parseCommonProperties 通用 shadow 解析（§3.3）。
- schema：`$defs/shadow` 抽取 + 四段引用（§3.2）。
- C ABI / Binding：**零新增**（通用 Set/Get 键即通）。

## 5. 实现要点与验收

| 验收 | 方法 |
|---|---|
| #9-1 CheckBox caption 往返 | 内部：`setStringProperty("caption")` → `getStringProperty("caption")` 一致；caption 变化后 box 尺寸/布局刷新（宽高断言） |
| #10-1 CheckBox 阴影开关 | `SetBool("text-shadow-enable",1)` → caption `isShadowEnabled()==true`；读回一致 |
| #12-1 三控件 shadow 主键 | 各 `SetBool("shadow",1)` → 内部 Label isShadowEnabled ✓；读回一致；Button 的 `kShadow` 与 `kTextShadowEnable` 互查一致（别名） |
| #12-2 offset 转发 | 各 `SetFloat("shadow-offset-x/y")` → 内部 Label getShadowOffset 分量正确（保留另一分量）；读回一致 |
| #12-3 JSON 通用解析 | LayoutParser：button/check-box/win-frame 布局含 `"shadow":{"enabled":true,"offset":{"x":2,"y":3}}` → 运行时断言内部 Label 状态；`validate_layout --strict` PASS |
| #12-4 schema 一致性 | 四段 `shadow` 同构（$ref）；label 行为不变（回归） |
| 回归 | test_colorfixes / test_button / test_checkbox / test_winframe / test_layout / test_colorpicker 全绿；键一致性（属性键==JSON 键）保持 |

## 6. 影响面与风险

- **别名双键**（`shadow` / `text-shadow-enable`）读写同一目标：无失同步；文档注明主键与别名。
- Button 既有 `text-shadow-enable` 布局/调用零回归（字段与路径保留）。
- 空 caption/title 边界：键返回 0（不崩）；典型使用（有文本）不受影响。
- schema 抽 `$defs/shadow`：label 由内联改 $ref——校验语义等价（validate + 布局回归验证）。
- 三后端无涉（纯逻辑/配置转发）。

## 7. 待审核问题

1. **统一主键**：`shadow` 为主键（`text-shadow-enable` 为运行时别名，Button 保留、CheckBox 支持但不入 schema）——确认？（避免面板双开关）
2. **schema 抽 `$defs/shadow`**（label 改 $ref + 三段新增）——确认？
3. **Label 访问器新增**（`isShadowEnabled`/`getShadowOffset` 两个内联 getter，供宿主转发读）——确认？
4. **Parser 通用化**：parseCommonProperties 统一解析 `shadow`；parseLabel 既有分支保留（幂等）——确认？
5. **CheckBox 是否另补 C++ `setCaption(string)` 方法**（#9 仅要求属性分发；C++ API 对称性可选）——按需确认。

## 8. 待提交配套改动（实施时随批）

- schema（`$defs/shadow` + 四段）+ `validate_layout --strict` 全布局验证。
- 测试扩展（§5）+ 全量回归；设计文档状态标注；复核意见归档。
- 手册：button/checkbox/winframe 章节补 `shadow`/`shadow-offset-x/y` 键说明（含别名注记）；`properties.html` 三段补行；`API_Mapping_Table` 补行。
- make_release 同步（设计器随后恢复通用 shadow 展开、删除类型分派绕行段）。