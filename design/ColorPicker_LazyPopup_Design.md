# ColorPicker_LazyPopup_Design — ColorPicker 弹窗子树惰性构建

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md` §P0 追加（2026-09-20，颜色组补全发现：ColorPicker 构建成本 ~3.5s/实例，15 颜色槽 50+ 秒卡顿）
> 状态：**已放行，已实施**（复核：`CornerstoneDesigner/Temp/ColorPicker_LazyPopup_Design_复核意见.md`，2026-09-20 通过——5 项确认 + 2 项实施注意）
> 实测数据（Temp/temp_cp_lazy 探针，全绿）：**T1** 15 实例关闭态直建 875ms（avg 58ms/实例；此前 ~3.5s/个 ≈ 52.5s，**~60x**）；**T2** 首次打开 844ms（含惰性构建）/ 二次打开 **0ms**（缓存复用）；**T3** 键盘路径不崩；**T4** 构建前 setter（color/preset-cols/rows/closed-font-size/popup-bg）全部生效且打开不崩
> 实施勘误：`openPopup` 实为 **private**（设计 §2 写"已公开"有误——交互经 handleEvent 点击/键盘入口，见 §3.3；设计器确认不需要 C ABI OpenPopup）；另发现 `getStringProperty("color")` 无分发（set 有、get 无）——既有 getter 缺口，不在本批范围，列后续 getter 补齐参考

---

## 1. 问题与目标

设计器属性面板需 15 个颜色槽（colors.background/border/text/textShadow × 4 态）。直建 ColorPicker 实测 **~3.5s/个**（15 个 50+ 秒，选中控件即长时间卡顿）。设计器当前以"占位 Button + 点击惰性替换"绕行——**首次点击仍卡 3.5s**，且形态复刻依赖引擎内部参数（易失配）。

根因：`ColorPicker::create()` 一次性构建**完整弹窗子树**（Dialog + 确定/取消按钮 + 预设网格 ~80 格 + HexInput + RGBA 4 滑块 + 预览/哈希标签）——数十子控件，含字体加载（均摊 ~100ms+/控件）。

目标：弹窗子树**首次 `openPopup()` 时才构建**（关闭态秒建），设计器删除占位方案、行内直建 ColorPicker。

## 2. 现状核实（源码事实）

| 机制 | 位置 | 状态 |
|---|---|---|
| `create()` 全量构建 | ColorPicker.cpp:82-124：Panel::create → **Dialog 构建**（~20 配置 + `create()`：确认/取消按钮 + layoutContent）→ setVisible(false) → createClosedStateControls → **recreatePopupContent** | 成本 ~3.5s 来源 |
| `recreatePopupContent()` | :121-138：removeAllControls + recreateButtons + 预设网格（PresetCell×cols×rows ≈80，Panel 无字体）+ 预览（Panel+Label）+ HexInput（EditBox）+ 4 Slider（R/G/B/A）+ layoutButtons | 字体型控件（EditBox/Slider/Label/Button）为成本大头 |
| 关闭态 | `createClosedStateControls()`（:150-176）：swatch Panel + hex Label | 轻量（1 字体），保留于 create() |
| `m_dialog` 引用 | 全文件 **49 处**；build/open/**close/handleEvent/setters**；多数已有 `if (!m_dialog)` guard | **唯一未守卫**：:198 `handleEvent` KeyDown 路径 `!m_dialog->getVisible()`——惰性化后空指针 |
| 打开路径 | `openPopup()`（:272-289）：computePopupRect → setAbsolute → layoutButtons → syncUIFromColor → `m_dialog->open()`（Popup::open 挂 BENCH，Dialog.cpp:143）→ focus hexInput | 构建插槽点 |
| 切换/查询 | `togglePopup()`（:296）：`m_dialog && getVisible()` 分支；`isPopupVisible()`（:305）：`m_dialog && getVisible()` | 已 guard |
| setters 交互 | setPresetColors（:500 `m_dialog && visible` → recreate）、setPresetLayout（:514 `if (m_dialog)` → recreate）、setColor/syncUIFromColor（成员 null-guard） | 构建前调用应只写成员，build 时生效 |
| 属性设置 | setColorProperty（:628 popup-bg → setPopupBGColor 等） | 同上 |
| 公开度 | `openPopup/closePopup/togglePopup/isPopupVisible` 已公开（ColorPicker.h:73-76）；C ABI 仅 CreateColorPicker + 通用属性键 | 无新增 API 需求 |

## 3. 架构选择与关键设计决策

### 3.1 方案选择（决策：a 弹窗子树整体惰性；b/c 处理见 §3.4）

| 方案 | 内容 | 评估 |
|---|---|---|
| **a（采纳）** | `create()` 只建关闭态；首次 `openPopup()` 时 `ensurePopupBuilt()` 全量构建弹窗子树并缓存 | 单实例成本"选中即付"→"首次打开付"；改动集中（create 拆分 + 1 处解引用修正）；设计器占位方案可回收 |
| b | 预设网格进一步延迟到弹窗首次可见 | **不做**：80 格为 Panel（无字体），非成本大头；增加状态复杂度 |
| c | 字体/控件创建共享缓存（长期） | **backlog**：与 P1-7 字体懒加载预热合并（全局问题） |

首次打开仍有 ~3.5s 一次的构建（字体加载为硬成本）——设计器已确认接受（"首次打开付"即其建议 a 的原始收益）；彻底消除需 c（backlog）。

### 3.2 惰性构建实现（决策：`m_dialog == nullptr` 即未构建标志 + `ensurePopupBuilt()`）

```cpp
// ColorPicker::create()（修改后）
void ColorPicker::create() {
    if (m_isCreated) return;
    if (GET_CONTEXT == nullptr) return;
    Panel::create();
    setTransparent(true);
    setBorderVisible(false);
    createClosedStateControls();          // 关闭态（swatch + hex Label）：保留，秒建
    // 弹窗子树（Dialog/按钮/预设/滑块/hex）延迟到首次 openPopup()
}

// 新增：构建弹窗子树（原 create() 的 Dialog 段 + recreatePopupContent）
void ColorPicker::ensurePopupBuilt() {
    if (m_dialog || GET_CONTEXT == nullptr) return;
    m_dialog = make_shared<Dialog>(nullptr, SRect(0, 0, m_popupWidth, m_popupHeight), m_xScale, m_yScale);
    ...（原 :91-116 的全部配置，包括 setContext/setOnConfirm/setOnClose/closeOnClickOutside/create/setVisible(false)）
    recreatePopupContent();               // 预设/预览/hex/滑块/按钮布局（读当前成员：颜色/尺寸/preset 配置）
}

// openPopup()：入口确保已构建
void ColorPicker::openPopup() {
    ensurePopupBuilt();
    if (!m_dialog) return;
    ...（原逻辑不变）
}

// togglePopup()：分支用 isPopupVisible()，打开走 openPopup()（内部构建）
// isPopupVisible()/closePopup()：维持 m_dialog guard（未构建 = 不可见 = 关闭，语义自洽）
```

### 3.3 null-safety 全量核对（决策：仅修 1 处未守卫 + 保持既有 guard 语义）

- **`:198` 修正**（唯一未守卫解引用）：`getFocused() && !m_dialog->getVisible()` → `getFocused() && !isPopupVisible()`。
- setters（构建前）：自然只写成员（Dialog 未建，跳过 recreate/bash——`setPresetColors` 的 `m_dialog && visible`、`setPresetLayout` 的 `if (m_dialog)` 均已有 guard 且语义正确）；`ensurePopupBuilt` 的 `recreatePopupContent` 读取当前成员——**build 后值正确**。
- `syncUIFromColor()`：各成员 null-guard 完整（m_hexInput && / m_previewSwatch / m_closedSwatch）——关闭态与构建前调用安全。
- `setRect`（:141）：closed 成员 guard 已有。
- `computePopupRect/layoutButtons`：仅 open 路径（已构建）。
- 析构：现有语义不变（dialog 属 BENCH 树）。

### 3.4 backlog（写入设计文档，不在本批）

- **b 预设网格二次延迟**：不做（见 §3.1）。
- **c 字体/控件创建共享缓存**：与 P1-7 字体懒加载预热合并评估。**2026-09-20 决策：暂不实施**（收益评估——渲染层字体缓存三后端已既有（contentHash+size，插件路径经后端回调命中），真正缺失的仅是"预热"（首次冷加载 ~390ms 无法被缓存消除）；用户评估收益不大，保持 backlog、不排期）。
- **C ABI OpenPopup/ClosePopup**：设计器条件项（"若提供可评估共享单实例"）——当前不需要（关闭态秒建 + 点击自带弹出已满足），不做。

## 4. API 设计

- 核心库：`create()` 拆分 + 新增私有 `ensurePopupBuilt()`；`:198` 修正。无签名变更。
- C ABI / Binding：**零变更**（`openPopup` C++ 已公开；C ABI 创建 + 属性键已覆盖设计器需求）。

## 5. 实现要点与验收

| 验收 | 方法 |
|---|---|
| 1. 关闭态秒建 | 探针（Temp/）：创建 15 实例计时，目标 <500ms 总量（此前 50+ 秒）；断言 create 后弹窗子树未建 |
| 2. 首次打开构建并正确 | 探针：`openPopup()` 后弹窗可见、预设/滑块/hex 齐全；二次 open 零重建（复用） |
| 3. 构建前属性设置生效 | 探针：create → setColor → setPresetColors/setPresetLayout → openPopup → 断言颜色/网格正确 |
| 4. 交互回归 | `test_colorpicker` 全绿（含弹窗打开/选色/确认/取消/外点关闭） |
| 5. 无空指针回归 | 未打开弹窗时键盘（Enter/Space）路径（:198 修正点）与各 setter 调用不崩 |
| 6. 全量回归 | 既有测试全绿 |

## 6. 影响面与风险

- **首次打开一次性构建**：~3.5s 移至首次 open（设计器接受）；二次打开零成本。
- **内存**：未打开的 ColorPicker 少持有整棵弹窗树（正向收益）。
- **属性设置在构建前**：全部走成员缓存，build 时应用——需保证 build 路径覆盖全部可设置项（实现时对照 setter 清单核对）。
- **`recreateClosedState`**（size/字号变更时）不受影响（仅关闭态）。
- 多实例/子视口：ensurePopupBuilt 依赖 GET_CONTEXT（与现 create 相同约束，open 时必已挂树）。

## 7. 待审核问题

1. **方案 a 范围**：接受"首次打开付 ~3.5s、二次打开零成本"（彻底消除需 backlog c 字体缓存）——确认？
2. **b 不做**（预设网格延迟）：80 格为无字体 Panel，收益低——确认？
3. **c 列 backlog** 与 P1-7 合并——确认？
4. **C ABI OpenPopup 不做**（设计器条件项，当前不需要）——确认？
5. **量化验收阈值**：15 实例关闭态直建总量 <500ms（探针 Temp/，不入测试门禁）——阈值是否合理（可按实测调整）？

## 8. 待提交配套改动（实施时随批）

- `test_colorpicker` 回归 + Temp/ 量化探针（前后对比数据）。
- 设计文档状态标注；复核意见归档 requirements/。
- 用户手册 ColorPicker 章节补"弹窗首次打开构建"说明（如有性能注意事项）。
- make_release 同步（设计器随后回收占位方案：删除 swatch+hex+透明按钮三件套，行内直建 CreateColorPicker）。