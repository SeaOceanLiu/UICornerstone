# UICornerstone 需求：容器 reflow 一致性 / 属性读写对称性 / Panel 裁剪能力

- 提出方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-16
- 优先级：P1（三项均为设计器属性面板/动态布局的基础能力缺口，当前以 workaround 绕行）
- 性质：均为"引擎已有机制差最后一步"，无新增核心机制

## 背景

设计器属性面板按引擎 `declarative-ui.schema.json` 动态生成属性行（编程式 Create 控件 + AddChild 挂入容器），滚动/回填/布局遇到三项引擎侧缺口，被迫 workaround（行 y 手动累加、读失败回落缺省值、超界行隐藏模拟滚动）。三项修复均宜在引擎侧落地，设计器侧 workaround 届时简化或删除。

## 需求一：Panel::addControl 触发 reflow（容器行为一致性）

### 现状

- `Bench::addControl`（Bench.cpp:233）→ `Panel::addControl` → reflowChildren ✓
- `Panel::addControl`（Panel.cpp:29-31）→ 仅 `ControlImpl::addControl` 挂树，**不 reflow**

同为"往容器加控件"，Bench 排版、普通 Panel 不排——编程式 `AddChildControl` 挂入 v-flow/h-flow 容器的子控件全部叠在 (0,0)，直到某次全局重排才修正（设计器动态属性行初始位置错误的根因）。

### 需求

`Panel::addControl` 在容器含 layoutEngine 时追加 `reflowChildren()`（与 Bench::addControl 行为对齐）：

```cpp
void Panel::addControl(shared_ptr<Control> control) {
    ControlImpl::addControl(control);
    if (m_layoutEngine) reflowChildren();   // 新增：编程式挂入即排
}
```

### 验收条件

1. 编程式创建 v-flow Panel → AddChildControl 依次挂入 N 个控件 → 各控件 rect 由布局引擎正确排布（不叠 (0,0)）；
2. LoadLayout 路径回归无损（Parser 已有显式 reflow，重复调用幂等无害）；
3. 挂入后再 removeControl 的场景无布局残留。

## 需求二：属性系统读写对称性 + schema 键名桥接

### 现状 A（set/get 不对称）

放置的控件（编程式创建）经属性系统写入的值读不回来，属性面板回填只能显示缺省值：

- `Button` 无 `getFloatProperty` 重载 → `caption-size` 写成功、读失败（Button.h:58 仅有 `getCaptionSize(float)` 签名疑为笔误，应为 `float getCaptionSize() const`）；
- 类似"set 有 get 无"的类型逐一存在（Label `font-size` 在 `setIntProperty`/`getIntProperty` ✓ 对称，可作正确样例）。

### 现状 B（schema 键名 ↔ 运行时属性名差异）

`declarative-ui.schema.json` 键名（camelCase，如 `text`/`captionSize`）与运行时属性名（kebab-case，如 `caption`/`caption-size`）无官方映射：

- 大多数可由 camelCase→kebab-case 规则转换，但存在引擎简写特例：`checkColor`→`check`、`crossColor`→`cross`、`indeterminateColor`→`indeterminate`、`boxBorderColor`→`box-border`；
- 语义差异项：schema `text` 键对 label/button/check-box 运行时是 `caption`，对 edit-box/text-area 才是 `text`——应用层无法从 schema 机械推导。

### 需求

1. **getter 补齐**：逐一核对 22 类控件的 set/get 属性对称性，补缺失的 getter（优先：Button `caption-size`、各类型 `check-state` 等高频编辑项）；
2. **键名桥接（二选一，引擎定夺）**：
   - 方案 a：运行时属性系统接受 schema 键名作别名（`setProperty("captionSize", ...)` 内部转 kebab）；
   - 方案 b：导出映射表 C ABI：`UICornerstone_GetPropertyMapping(UIControlHandle ctl, int index, char* outJsonKey, char* outPropName, char* outType)`，应用层遍历。

### 验收条件

1. Button 放置后 `GetFloat("caption-size")` 返回真实字号（写 24 → 读 24）；
2. （方案 a）`SetString("captionSize"...)` 与 kebab 名等价；或（方案 b）映射表覆盖全部 schema 可编辑键；
3. 既有 JSON 布局加载回归无损。

## 需求三：Panel 子项裁剪能力（clipChildren）

### 现状

`RenderDevice` 已有 `pushClipRect/popClipRect`（RenderDevice.cpp:95-106，EditBox/ListView/TreeView 内部自裁剪均在用），但 **Panel 渲染不裁剪子项**——子控件超出容器 rect 仍然绘制。应用层做滚动容器只能"超界行隐藏"模拟（行半可见时突兀消失）。

### 需求

Panel 支持裁剪开关（默认不裁剪保持兼容）：

- 属性系统：`SetBool("clip-children", true)`（ControlImpl 或 Panel 层处理）；
- 渲染：绘制子项前 `pushClipRect(自身内容 rect)`（与 EditBox 同机制，含 clipStack 嵌套）。

### 验收条件

1. `clip-children=true` 的 Panel：子控件超出部分不绘制；`false`/未设置：行为与现状一致；
2. 嵌套裁剪（Panel 内 Panel 均开启）结果正确（clipStack 相交）；
3. 设计器滚动容器改为真滚动后：行滚动平滑、半可见行被正确裁剪。

## 优先级建议

| 项 | 设计器阻塞程度 | 建议序 |
|---|---|---|
| 二（getter/桥接） | 高——值回填全面依赖 | 1 |
| 一（Panel reflow） | 中——已 workaround | 2 |
| 三（clipChildren） | 中——窗口滚动可用但体验差 | 3 |

## 设计器侧配合改动（接口就绪后）

1. 需求一就绪：动态属性行删除手动 y 累加（恢复声明式 v-flow）；
2. 需求二就绪：删除 `textIsCaption` 特判与 font-size 回落逻辑，回填全走 `Get*`；
3. 需求三就绪：`propDynamic` 开 `clip-children`，滚动删除"超界隐藏"，改为纯偏移。

## 参考

- 前序需求：`Temp/UICornerstone_需求_实例缩放CABI.md`（画布缩放，同属属性/布局体系）
- 设计器 workaround 位置：`src/CanvasPane.cpp` 的 `rebuildDynamicRows/applyDynScroll`（提交 327b8e2）
