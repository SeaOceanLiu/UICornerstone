# P074_PreviewFix_Design — 预览联测（viewport 背景 / tree hover·selected / scale-mode 核实 / colors 逐键合并【高】/ statusbar hover 来源）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-07 追加：P0-74）
> 前置：P0-72 同批（④ 与本批 ④ 交叉：schema pressed 依赖逐键合并）
> 状态：**已放行，已实施**（2026-10-07；含增补：①alpha=0 透明、②读回补齐、④保转发语义、⑤①+②同批）
> 复核：`requirements/P074_PreviewFix_Design_复核意见.md`（通过，含增补要求）
> 实施备注：① `viewport.background` 解析（parseColor，alpha=0 不填充语义）＋schema 描述；② `TreeView::getColorProperty` 补 `hover`/`selected` 读回 + JSON 解析（`parseSingleColor`）+ schema；③ 非缺口（CABI/Binding/文档已齐）；④ **逐键合并**：parser `mergeStateColor`（逐键 `getColorProperty` 读当前 → JSON 显式键覆盖 → 整态 `setXxxStateColor` 写回，保 Button/WinFrame 转发）；字符串色=normal 修复；`parseSingleColor` 用于 slider 单色键；winframe 标题栏/文本、button caption-label 同批；MenuPanel/MenuBar/StatusBar 读回补齐（shadow 四态等）；⑤ statusbar `hover` JSON 键 + 段回退链（item hb → `background.hover`（显式标记 m_bgHoverExplicit）→ hover → 缺省）+ schema；验证：**P0-74④ MERGE OK**（panel 字符串色 / sb 仅 hover 保持 VSCode 蓝 / btn label 部分合并 / wf 基线保持）+ **P0-74 1/2/5 OK**；回归仅 4 项预存

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-74① | `viewport` JSON 缺背景色 | **确认** | 解析仅 width/height/scale-mode（src/LayoutParser.cpp:53-80）；实例背景仅 C ABI `SetViewportBackgroundColor`（src/UICornerstoneAPI.cpp:665；渲染清屏 :927）→ 导出布局不携带画布底色（预览黑底） |
| P0-74② | TreeView schema 缺 `hover`/`selected` | **确认** | 运行期颜色键齐备（src/TreeView.cpp:992-993 `kTreeHover/kTreeSelected`，读写）；LayoutParser 无解析、schema tree-view 无声明。**注意**：list-view 的 schema `hover` 是 **bool**（kHoverHighlight="hover"，行悬停开关），与 tree-view 的 hover **颜色**同名异义（不同 def 合法，见 Q-1） |
| P0-74③ | 缩放模式运行时 API | **非缺口** | `UICornerstone_SetViewportScaleMode(inst, int)` / `GetViewportScaleMode`（include/UICornerstoneAPI.h:271-272；实现 src/UICornerstoneAPI.cpp:672-684）+ Binding `SetViewportScaleMode(int)/GetViewportScaleMode()`（binding/include/UICornerstone.h:327-328）+ 文档（appendix/capi.html:46、binding.html:312）**均已暴露**——设计器直接接线（0=off/1=fit/2=stretch）；无需引擎改动 |
| P0-74④ | `colors` 部分对象整体替换（StatusBar 背景变黑，**高**） | **确认（含附带发现）** | `parseCommonProperties`（LayoutParser.cpp:2793-2810）对每组合 `setXxxStateColor(parseStateColor(...))`；`parseStateColor`（:3408-3437）**新建** StateColor——构造缺省（include/ControlBase.h:42-59，Background=(23,23,24,255) 黑）覆盖控件现值 → 仅给 hover 时 normal 变黑。**附带发现**：`parseStateColor` 无字符串分支 → `"colors": {"background": "#..."}`（layouts/all_controls.json:433、test_layout.json:487/508、test_layout_advanced.json:79/109）解析为全缺省（字符串色静默失效）——同根因一并修 |
| P0-74⑤ | 状态栏悬停色来源不明/不可配 | **确认** | 段悬停链（src/StatusBar.cpp:285-301）：item 显式 hb → 有段 bg 叠加 `m_hoverColor`@0.35 → 无 bg 实色 `m_hoverColor`；`m_hoverColor` 仅 `SetColor("hover")` 可设（:430；缺省蓝）；`colors.background.hover` 只作用于条自身背景态；schema/JSON 无 `hover` 键 |

## 2. 修改方案

### 2.1 ① `viewport.background`
- LayoutParser 顶层 viewport 解析增 `background`（color：`#RRGGBB[AA]` 或 `{r,g,b,a}`）→ `m_viewportTarget->viewportBackground`（`parseColor` 复用）；
- schema `viewport` 描述补充；文档 declarative-ui.html/属性表同步。

### 2.2 ② TreeView `hover`/`selected`（颜色）
- LayoutParser tree-view 分支：`hover`/`selected`（`$ref color`；字符串或 {r,g,b,a}）→ `setColorProperty(kTreeHover/kTreeSelected)`；
- schema tree-view 增两键（描述"行悬停/选中底色"）；
- （List/gen 不加；与其 bool `hover` 冲突，Q-1）。

### 2.3 ③ 非缺口
- 无引擎改动；回复设计器：直接使用既有 `SetViewportScaleMode(0/1/2)`（CABI/Binding/文档齐备）。

### 2.4 ④ colors 逐键合并（核心，高）
- **弃用**通用路径的 `setXxxStateColor(parseStateColor(...))`，改为**逐键 `setColorProperty`**：

| colors 组 | normal | hover | pressed | disabled |
|---|---|---|---|---|
| `background` | kBackground | kStateHover | kStatePressed | kStateDisabled |
| `border` | kBorder | kBorderHover | kBorderPressed | kBorderDisabled |
| `text` | kText | kTextHover | kTextPressed | kTextDisabled |
| `text-shadow` | kTextShadow | kTextShadowHover | kTextShadowPressed | kTextShadowDisabled |

- **字符串值** → 组基础键（=normal）；
- 未给出的状态键**不触碰** → 天然合并（保留控件构造/主题缺省）——修复"仅 hover 变黑"与"字符串色失效"；
- **特化路径评估**：winframe 标题栏/标题文本（LayoutParser.cpp:1647-1668，控件/label 已存在）→ 逐键；button caption label 构建器（:966-976，label 未 build）→ build 后对 label 逐键；其余（List item 等解析器）逐项核对（Q-2 范围）。
- `parseStateColor` 保留供确实需要全量 StateColor 的场景（如 item 级 block），但补字符串分支（=normal）。

### 2.5 ⑤ statusbar `hover`
- **①（推荐）**：schema status-bar 增 `hover`（`$ref color`）+ LayoutParser 解析 → `setColorProperty("hover")`（m_hoverColor）；段链不变；文档说明三档回退（段显式 hb → 控件级 hover → 缺省蓝）；
- **②（可选，Q-3）**：段链纳入 `colors.background.hover`（item hb → 条 background.hover → `hover` 键 → 缺省），与 ① 不冲突。

## 3. 测试计划
- **④**：新 JSON 探针——statusbar `colors:{background:{hover:...}}` → normal 保持构造缺省蓝（修复前=黑）；panel `colors:{background:"#..."}` 字符串生效；全 layouts strict + test_image 等回归；
- **②⑤**：tree/statusbar JSON 解析 + `GetColor` 读回 + 像素探针（hover/选中行色）；
- **①**：viewport.background 解析 → 渲染清屏色（像素探针）；
- 回归全量（仅 4 项预存）。

## 4. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | tree-view `hover` 键名与 list-view bool `hover` 同名异义（per-def 合法）；是否统一改 `hover-background-color`？ | 保持 `hover`/`selected`（与运行时键一致；per-def 合法） |
| Q-2 | ④ 逐键整改范围：通用路径必改；winframe 标题栏/按钮 caption label/其余特化路径是否同批？ | 通用 + winframe + button label 同批；其余逐项核对后补 |
| Q-3 | ⑤ 采用 ①（显式 `hover` 键）或 ①+②（回退链扩展）？ | ① 先行；② 视需要再加（不阻塞） |
| Q-4 | ① 是否需要 `GetViewportBackgroundColor` CABI 读回（现仅 Set）？ | 不需要（设计器模型自持值；如需另议） |
