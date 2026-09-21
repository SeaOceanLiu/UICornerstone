# ClipChildren_Nested_Design — 嵌套裁剪相交修复（clip-children 滚动溢出）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md` §P0 升级（2026-09-21，P0-11：截图复现——滚动容器内容叠绘到容器外）
> 状态：**已放行，已实施**（复核：`CornerstoneDesigner/Temp/ClipChildren_ShadowCaption_复核意见.md`，2026-09-21 通过）
> **实施结果与实测数据（test_capture_cabi 像素用例）**：
> - 四处 pushClipRect 均已改"与栈顶相交"（栈存相交后 rect）；空相交：SDL 用渲染目标外 1×1（0 尺寸会禁用裁剪），SFML/raylib 0 尺寸天然无绘制，插件路径同语义转发。
> - **修复前**（两层回退 replace）：`FAIL×3`（部分越出泄漏/完全越出泄漏/嵌套内层逃逸）+ 容器内有内容 PASS；**修复后**：4 项全 PASS。
> - 回归：capture/listview/treeview/textarea/editbox/combobox/layout×2/colorfixes/colorpicker/handlecontrol 全绿；三后端编译 0 错误。

---

## 1. 问题与目标

| # | 现象 | 目标 |
|---|---|---|
| P0-11 | `clip-children=true` 容器（如动态属性区）滚动后，**滚出容器的行内容未裁剪**，叠绘在容器上方固定区（截图：font-size 行 NUD 值 "14" 绘到"行为"标题处） | 嵌套裁剪按**相交**语义生效：子控件（含其自身内容裁剪）的任何绘制均不超出祖先 clip-children 容器；修复后设计器删除"超界隐藏 -40 容差"兜底，恢复纯偏移滚动 |

## 2. 现状核实（源码事实）

| 机制 | 位置 | 状态 |
|---|---|---|
| 容器裁剪 | `Panel::draw`（Panel.cpp:21-26）：`m_clipChildren` → `pushClipRect(getDrawRect())` → 子递归 → `popClipRect()`；注释称"clipStack 支持嵌套 Panel" | 已存在 |
| 子控件自内容裁剪 | `EditBox::draw`（EditBox.cpp:273）、`ComboBox`（ComboBox.cpp:851）等自身 `pushClipRect(内容 rect)` | 已存在 |
| **clip 栈语义** | **四处实现均为"替换"而非"相交"**： | **缺陷点** |
| | ① SDL3 `pushClipRect`（backend/sdl3/RenderDevice.cpp:105-108）→ `applyClipRect(rect)` = `SDL_SetRenderClipRect(rect)`（替换；:367-371） | |
| | ② SFML `pushClipRect`（backend/sfml/RenderDevice.cpp:337-345）→ `applyClipRect(rect)` = `glScissor(rect)`（替换；:286-290） | |
| | ③ raylib `pushClipRect`（backend/raylib/RenderDevice.cpp:412-421）→ `EndScissorMode()` + `BeginScissorMode(rect)`（替换） | |
| | ④ 插件路径 `CallbackRenderDevice::pushClipRect`（CallbackAdapters.cpp:94-97）→ `setClipRect(rect)`（替换；栈仅用于 pop 恢复） | |
| 绘制原语 | 文本/图元均经后端渲染器绘制，**遵循渲染器当前 clip**（SDL clip / GL scissor / raylib scissor） | 无独立绕过（无需逐原语排查） |

**根因链**：
1. 容器（clip-children）push 容器 rect（如 y∈[100,500]）；
2. 滚出行（局部 y 为负，如 y∈[-10,40]）内的 EditBox/NUD 绘制时 push **自身内容 rect**（在容器外）；
3. 后端 `pushClipRect` **直接替换**当前 clip → 容器 clip 丢失 → 行内容（文本等）绘制到容器外 ✓ 与截图完全吻合（叠绘在容器上方固定区）；
4. 无自内容裁剪的子控件不触发（仅受容器 clip 约束）——故表现为"部分控件（NUD/EditBox/TextArea 等）泄漏"。

**旁证**：嵌套 Panel（两个 clip-children 容器相交）同样因替换语义而裁剪错误（内层绕过外层约束）——本修复一并解决。

## 3. 架构选择与关键设计决策

### 3.1 修复（决策：设备层 pushClipRect 改为"与栈顶相交"，四处统一）

```cpp
// 新增共享工具（GraphTool.h 的 SRect 成员，供四处复用）
SRect intersected(const SRect& o) const {
    float l = std::max(left,   o.left);
    float t = std::max(top,    o.top);
    float r = std::min(right(),  o.right());
    float b = std::min(bottom(), o.bottom());
    if (r <= l || b <= t) return SRect(l, t, 0.0f, 0.0f);   // 空相交
    return SRect(l, t, r - l, b - t);
}

// 四处 pushClipRect 统一改法（以 SDL3 为例）
void pushClipRect(const SRect& rect) override {
    SRect r = m_clipStack.empty() ? rect : m_clipStack.back().intersected(rect);
    m_clipStack.push_back(r);          // 栈内保存"相交后"rect（pop 恢复即正确）
    applyClipRect(r);
}
```

- **栈内保存相交后 rect**：`popClipRect` 现有逻辑（恢复栈顶/清空）无需改动即正确（嵌套多层逐级还原）。
- **空相交处理（实现要点）**：相交结果 w/h ≤ 0 时需保证"什么都不绘制"：
  - SDL：`SDL_SetRenderClipRect` 对 **w/h=0 会禁用裁剪**（SDL 语义）→ 空相交改用**渲染目标外 1×1**（如 `left=-2, top=-2, w=1, h=1`）或实现时实测选择等价手段；
  - SFML：`glScissor(0 或负尺寸)` 天然无绘制；
  - raylib：`BeginScissorMode(0)` 无绘制；
  - Callback：`setClipRect` 空 rect 语义由后端决定（同上口径，实现时逐一验证）。
  - 验证方式：**完全滚出行**用例（§5 P0-11-2）。
- **不相交用 clamp 到视口**：首层 push（栈空）保持现状（rect 原样）；渲染目标自动约束（SDL clamp / GL scissor）——不改动既有行为。
- **不改 push 调用方**（Panel/EditBox/ComboBox 均无感）：在设备层集中修复，所有嵌套路径获益。

### 3.2 被否决备选

| 方案 | 否决理由 |
|---|---|
| 核心层维护 clip 栈 + 交集后再下发 | 设备层已有栈（pop 恢复依赖）；双栈易不一致；改动面更大 |
| 各 push 调用方自行与祖先相交 | 调用方无祖先信息（无全局 clip 查询）；重复实现易漏 |
| 仅为 Panel clip-children 特判（限制子控件不 push） | 破坏子控件自内容裁剪；未覆盖嵌套 Panel 场景 |
| 设计器继续"超界隐藏"兜底 | 体验差（行整跳）；本需求即根除该兜底 |

## 4. API 设计

- 核心库：`GraphTool.h` SRect 新增 `intersected()`（1 处）。
- 三后端 + 插件适配：`pushClipRect` 交集化（4 处，各 ~2 行）。
- 无 C ABI / Binding / PropertyNames / schema 变更。

## 5. 实现要点与验收

| 验收 | 方法 |
|---|---|
| P0-11-1 部分滚出裁剪 | 像素测试（test_capture_cabi 扩展）：clip-children Panel + 内含自内容裁剪的 EditBox（rect 上缘越出容器）→ CaptureRect 覆盖**容器上方区域** → 断言背景色（无文本像素）；修复前该区域出现文本 |
| P0-11-2 完全滚出（空相交） | 子控件 rect 完全在容器外 → 容器内外均无其绘制（尤其容器外） |
| P0-11-3 嵌套容器 | clip Panel ⊂ clip Panel（内层部分越出外层）→ 内层内容不越外层（CaptureRect 断言外区干净） |
| P0-11-4 正常内容不回归 | 容器内正常行完整绘制（CaptureControl 断言内容存在/像素抽样）；既有 test_capture_cabi / test_listview（自身有 clip 路径）全绿 |
| P0-11-5 三后端 | 像素断言在三后端各自跑（测试框架已按后端构建）；无像素能力后端标注 skip |

测试载体：`test_capture_cabi` 扩展（既有 Capture* + BMP 校验框架，像素级断言现成）。

## 6. 影响面与风险

- **语义修复面**：所有嵌套 push 路径（Panel clip-children、EditBox 自裁剪、ComboBox、嵌套容器）——此前"替换"语义下多层级联裁剪错误；修复后严格相交（正确语义）。潜在行为变化：**此前因替换而"意外可见"的内容将按语义被裁剪**——设计器期望即如此（其兜底可删除）。
- 空相交的三后端实现差异为最大实现风险（SDL 空 rect 语义）——以 P0-11-2 用例逐一验证。
- 性能：每 push 一次相交计算（4 次 min/max）——可忽略。
- 既有单层 clip 用例（无嵌套）行为不变（栈空时原样 apply）。

## 7. 待审核问题

1. **相交语义**确认（嵌套 clip 取交集；此前为替换）？
2. **空相交处理**：实现按"渲染目标外 1×1"（SDL）与各后端等价手段，以像素用例验证——接受该实现路径（或评审指定偏好）？
3. **测试载体**：test_capture_cabi 像素扩展（部分/完全滚出/嵌套/回归）——确认？
4. 修复后设计器删除"超界隐藏 -40 容差"兜底（纯偏移滚动）——确认联测范围？

## 8. 待提交配套改动（实施时随批）

- 像素测试扩展（§5）+ 全量回归；三后端验证记录。
- 设计文档状态标注；复核意见归档；make_release 同步。
- 文档备注：`Panel::draw` 注释（"clipStack 支持嵌套 Panel"）改为"嵌套取交集"表述准确化。