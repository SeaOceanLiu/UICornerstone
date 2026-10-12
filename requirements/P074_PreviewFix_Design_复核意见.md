# P074_PreviewFix_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-07
- 对象：`design/P074_PreviewFix_Design.md`（P0-74①~⑤，状态：待评审）
- 结论：**通过，放行实施**（④ 高优先；② 需补一项读回；④ 实现方式须保转发语义；⑤ 建议 ①+② 同批）

## 1. 评审要点

### ④ colors 逐键合并（核心）——方向 ✓，**实现方式需修正一处**

- 根因链与我方判断一致 ✓（`parseCommonProperties` 整体替换 + `parseStateColor` 新建 StateColor 覆盖现值）；
- 附带发现（`parseStateColor` 无字符串分支 → `"colors":{"background":"#..."}` 静默全缺省）核实 ✓——引擎自家 layouts（all_controls.json:433、test_layout*.json 多处）确实使用字符串色，一并修复价值高；
- **修正点（要求）**：方案"逐键 `setColorProperty`"会走 `ControlImpl` 的**单态 setter**（`setTextNormalStateColor` 等），**丢失整态 setter 的转发语义**：
  - `Button::setTextStateColor` → 内部 caption label（src/Button.cpp:293-296）；
  - `WinFrame` 标题 Label（经 `setTitleTextColor`/`setState`）；
  - 设计器侧 text/text-shadow 现走 `GetPtr(caption-label / title-label)` 直控（CanvasPane.cpp writeColorSlotTo:1712-1719）——若解析侧逐键写 ControlImpl 单态字段，**button/winframe 的 `text.*` 从 JSON 加载将失效**（相对现状回归）。
  - 建议：逐键合并**改态后经整态 setter 写回**——`StateColor sc = ctrl->getTextStateColor(); applyStates(sc, json); ctrl->setTextStateColor(sc);`（getter/setter 均为虚函数，转发保留）；或为单态 setter 在 Button/CheckBox/WinFrame 补转发（实现由引擎定夺，二选一即可）。
- 其余（未给出状态不触碰 = 天然合并；字符串→基础键=normal）✓ 与设计器写回路径完全兼容（设计器 `SetColor("background.hover")` = kStateHover="background.hover" 带点键 ✓，读回 `GetColor` 同键 ✓）。

### ② TreeView `hover`/`selected`——✓，**需补读回分支（要求）**

- 解析/schema 方案 ✓；命名 Q-1 同意保持 `hover`/`selected`（per-def 合法；设计器属性面板按当前选中类型的 def 解析，list-view 的 bool `hover`（kHoverHighlight）互不影响 ✓）；
- **要求**：`TreeView::setColorProperty` 已支持 kTreeHover/kTreeSelected（src/TreeView.cpp:992-993），但 **`getColorProperty` 无对应分支**（:1192-1208 落 `ControlImpl`，后者只认 "background.hover" 等带点键——"hover"/"selected" 读回失败）→ 设计器 ColorPicker 行回读将显示缺省色 ✗。补 `getHoverColor()/getSelectedColor()`（已有，include/TreeView.h:216-219）分支即可。
- 设计器侧零代码改动：面板对 `$ref: color`（`x-color: true` ✓）顶层键自动生成 ColorPicker 行（prop=键名 → SetColor/GetColor）✓——前提即上述读回补齐。

### ① viewport.background——✓（补充要求：透明支持）

- 方案 ✓；Q-4 同意不做 CABI 读回（设计器模型自持值；导出/预览经 JSON 闭环）；
- **补充要求（用户新增）**：游戏场景 UI 需叠加在游戏画面上——背景色必须支持 **alpha=0（透明）**，语义 = **不填充/不清屏**（保留底层游戏帧）。**渲染侧已正确**：`UICornerstone_Render` 仅 `alpha>0` 时填充视口、缺省 `SColor(0,0,0,0)` 跳过（src/UICornerstoneAPI.cpp:927-930、include/UIContext.h:61）——本项 JSON 解析须覆盖透明值（`#RRGGBB00` / `{r,g,b,a}` a=0，parseColor 已支持 alpha），schema 描述注明 alpha 语义即可；
- 设计器随批：`serializeLayoutJson` 增 `viewport.background`（含 alpha；+既有 `viewport.scale-mode` 一并纳入模型编辑）；Bench 属性行"背景色"支持透明（alpha=0/透明勾选），画布透明底以棋盘格标识（设计器视觉）；`.csd` 元数据携带窗口标题（设计器侧，无引擎需求）；预览经 `LoadLayout` 应用 ✓（透明预览窗透出后景，设计器侧评估窗口透明标志）。

### ③ scale-mode 非缺口——✓ 确认

- CABI/Binding/文档齐备核实 ✓；设计器主要经 JSON `viewport.scale-mode`（导出/预览闭环），运行时 API 备而不用（若预览刷新需即时切换可用）——不阻塞、无引擎改动。

### ⑤ statusbar hover——建议 **①+② 同批**

- 现状链核实 ✓（段悬停用控件级 `m_hoverColor`，仅 `SetColor("hover")` 可设，缺省蓝；`colors.background.hover` 仅条自身背景态）；
- 用户实际编辑的是**面板 background.hover 行**（期望=悬停反馈）——仅 ① 时其既有值仍不驱动段悬停（预览仍见蓝）✗；**②（回退链纳入 `colors.background.hover`）才对齐用户心智模型**（段显式 hb → 条 background.hover → `hover` 键 → 缺省蓝），且改动小、与 ① 不冲突；
- 若引擎坚持分批，① 先行可接受，但文档须明示三档回退链；设计器侧两键均自动出 ColorPicker 行（StatusBar 的 "hover" 读写已齐：src/StatusBar.cpp:430/:440 ✓），零代码改动。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| Q-1 | tree `hover`/`selected` 命名 | **同意保持**（与运行时键一致；per-def 合法；设计器面板按类型解析无歧义） |
| Q-2 | ④ 逐键整改范围 | 同意"通用 + winframe + button label 同批"；但见 §1④ 修正点——winframe/button 文本键必须保转发（整态 setter 写回或补转发） |
| Q-3 | ⑤ ① 或 ①+② | **建议 ①+② 同批**（② 才覆盖用户实际场景；均小改动） |
| Q-4 | ① 是否需背景色读回 CABI | 不需要（设计器模型自持；如需另议） |

## 3. 设计器随批（同步后）

1. 重建设计器 + auto=2 自测（27 项，含"导出携带 StatusBar 颜色/引擎应用/provider 路径保留"）+ auto=3 预览探针；
2. 新增回归探针：仅设 `colors.background.hover` → 预览 normal 保持构造缺省蓝（修复前=黑）；tree `hover`/`selected` 面板行读写回；`viewport.background` 预览生效；
3. Bench 编辑（背景色/大小/缩放模式/标题）随 ①+③ 落地实施（设计器侧功能）。

## 4. 放行

**结论：放行实施**（④ 优先，② 读回补齐 + ④ 转发语义为放行条件；⑤ 建议 ①+② 同批）。
