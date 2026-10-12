# P076_ColorMerge_BenchTransparency_Design — colors 逐键合并修正 / Bench 透明与视口背景 / 面板阴影禁用

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-08 追加：P0-76、P0-77、P0-77①）
> 前置：P0-72/P0-74 批已实施并同步（P0-74④ 合并语义为本批修正对象）
> 状态：**已放行，已实施**（2026-10-08；Q-1/2/3/4 全部确认）
> 复核：`requirements/P076_ColorMerge_BenchTransparency_Design_复核意见.md`（通过）
> 实施备注：① **逐键合并**：`LayoutParser::applyColorGroupPerKey`（字符串=normal；对象仅写显式键；子类 `setColorProperty` override 可达——MenuBar pressed 修复）替代 `mergeStateColor` 整态写回；特化路径（winframe 标题栏/文本、button caption-label）同款逐键；② **单态转发补齐**：Button 8 个 + CheckBox 10 个（整态+8 单态）override（base + caption label 双同步）；③ `Bench::initial` 去除强制不透明 + 主实例创建缺省 `viewportBackground=DEFAULT_NORMAL_COLOR`（子视口保持透明）；④ `drawShadow` radius<=0 跳过（r#0 禁用投影）；验证：**P0-76/77 OK**（menubar `background.pressed` JSON=(18,52,86) 读回；checkbox base+label text.hover 双同步；默认底 (23,23,24,255)+bench transparent=1）+ P0-74 探针复跑全过；⑤ `test_overlay_viewport_cabi` 像素前提同步（子视口显式底色）12/0；回归仅 4 项预存（设计器可删 `ReplayMenuBarColorsTo`）

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-76 | P0-74④ 合并经整态 setter 写回 → 子类影子成员丢失（pressed 等） | **确认** | P0-74④ `mergeStateColor` 读回后经 `setBackgroundStateColor` 等整态 setter 写回；`MenuBar`/`MenuPanel` 颜色为**子类影子成员**（include/Menu.h:354-358），其整态 setter 仅覆盖 normal/hover/disabled（Menu.cpp:1260-1272），**pressed 丢弃**（`MenuBar::setColorProperty(kStatePressed)→m_activeBgColor` 不可达）→ `colors.background.pressed` 解析后丢失（用户问题 2）；`StatusBar`（m_textColor 影子）等同族风险 |
| P0-77 | Bench 默认不透明 → `viewport.background` 不可见 + alpha=0 透明失效 | **确认** | `Bench::initial()`（src/Bench.cpp:10）`setTransparent(false)` 覆盖构造 `setTransparent(true)`（:31）→ bench 绘制不透明底 (23,23,24)，盖住 `UICornerstone_Render` 的视口背景填充（API:927-930）→ 任意 `viewport.background` 不可见、游戏叠加（alpha=0 透出底层帧）不成立 |
| P0-77① | `r#0` 阴影无法去掉 | **确认** | `drawShadow()` 恒由 `draw()` 调用（Menu.cpp:892）；`m_shadowRadius` 仅作 blur（:864-884），radius=0 仍绘制"无模糊带偏移"阴影；`m_shadowEnabled`（kShadow）为**文本阴影**开关（P0-26），与面板投影无关 |

## 2. 修改方案

### 2.1 P0-76 合并改逐键 `setColorProperty`（子类 override 可达）+ 单态转发补齐
- Parser：弃用 `mergeStateColor`+整态写回，改 **逐键写**：
  - 字符串值 → 组基础键（normal）；
  - 对象 → 按组映射 `normal/hover/pressed/disabled` → `ctrl->setColorProperty(键, 色)`（组映射表同 P0-74 文档；MenuBar pressed → `m_activeBgColor` 等子类 override 天然可达）；
  - 未给键不写 → 天然合并（保留现值）。
- **转发语义补齐（P0-74 复核要求）**：单态写不再经整态 setter，需为需要转发的控件补单态转发：
  - `Button`：`setTextNormal/Hover/Pressed/DisabledStateColor` + `setTextShadowNormal/Hover/Pressed/DisabledStateColor`（8 个 override）→ 基类 + `m_caption`（与整态 setter 同口径）；
  - `CheckBox`：同款（m_caption）逐项核对后补；
  - `WinFrame`：`setColorProperty` 已转发标题文本键（WinFrame.cpp:455-462）✓；标题栏/标题文本特化路径改为对 `titleBar`/`titleLabel` **逐键** setColorProperty（label/panel 自身成员即真值）；
  - 特化路径（button caption-label）改为对 `btn` 逐键（转发后 base+label 双同步，读回对称保持）。
- 移除 `mergeStateColor`（P0-74④ 引入）或降级为未用；`parseStateColor` 字符串分支保留。
- 验证：P0-74 既有探针（sb 仅 hover 保持蓝 / btn 部分合并+读回 / wf 基线）+ 新增 **MenuBar `colors.background.pressed` 读回**（修复点）；设计器删除 `ReplayMenuBarColorsTo` 过渡。

### 2.2 P0-77 Bench 透明与视口背景层次（引擎定夺项，建议如下）
- `Bench::initial()` 去除 `setTransparent(false)`（保持构造透明意图）；
- **默认外观保持**：主实例（ownsBackend）创建时 `viewportBackground` 缺省设为不透明深色 `(23,23,24,255)`（原 bench 缺省底）；子视口缺省保持透明（`(0,0,0,0)`，画布透出 owner 内容——与设计器画布语义一致）；`viewport.background` 显式覆盖；alpha=0 → 不填充（游戏帧透出）；
- 提供 bench 直连能力（设计器 Bench 编辑）：`GetRoot`（CABI:713）/Binding `Root()`（:127）**已暴露**——文档补充说明；如需 bench 透明开关/背景 API 可后续（Q-3）；
- 兼容风险：窗口默认清屏由"bench 自绘"改为"视口背景填充"，需全量像素回归（现有探针/样例）。

### 2.3 P0-77① 阴影禁用
- `drawShadow()` 开头：`if (m_shadowRadius <= 0.0f) return;`（r#0 → 不绘制投影；默认 8 不变）；
- `kShadow`（文本阴影）语义保持不动；文档注明 `shadow-radius=0` 为禁用投影（Q-4）。

## 3. 测试计划
- **76**：新探针——MenuBar `colors.background.pressed` JSON → `GetColor("background.pressed")` == 设定值；StatusBar 同款（text pressed）；P0-74 探针复跑全过；Button caption-label 转发读回（base+label）；
- **77**：像素探针——主实例默认底色 (23,23,24)；`viewport.background="#C0C0C0FF"` 生效；`#00000000` → 视口不填充（清屏保留底层）；子视口透明画布语义不变；全量像素回归；
- **77①**：像素探针——`shadow-radius=0` 无投影像素、默认 8 有投影；
- 全量回归（仅 4 项预存）+ 设计器联测（删 ReplayMenuBarColorsTo 后复测）。

## 4. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 76：逐键写回方案确认（替代整态写回；转发由单态 override 补齐） | 确认（MenuBar pressed 修复的必需路径） |
| Q-2 | 76：CheckBox 是否需要同款单态转发（设计器列名） | 按核对结果补（无 label 转发需求则仅 Button） |
| Q-3 | 77：主实例默认 `viewportBackground=(23,23,24,255)`（外观保持）vs 完全透明默认 | 前者（外观零变化）；如需 bench 透明/背景 API 另议 |
| Q-4 | 77①：禁用语义取 `radius<=0`（kShadow 保持文本阴影语义） | 确认 |
