# P076_ColorMerge_BenchTransparency_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-08
- 对象：`design/P076_ColorMerge_BenchTransparency_Design.md`（P0-76 / P0-77 / P0-77①，状态：待评审）
- 结论：**通过，放行实施**（方案与设计器复核要求一致；落地后设计器回收 2 项绕过，Bench/透明/阴影三项闭环）

## 1. 评审要点

### 2.1 P0-76 合并改逐键 `setColorProperty` + 单态转发补齐——✓（Q-1 确认）
- 弃用整态写回、逐键写（未给键不写=天然合并）✓——`MenuBar::setColorProperty(kStatePressed)→m_activeBgColor` 等子类 override 天然可达 ✓（用户问题 2 修复点）；
- **单态转发补齐**（P0-74 复核要求的等价路径）✓：Button 8 个 override（base+caption 双同步）；CheckBox 同款；WinFrame 标题栏/标题文本特化路径逐键（label/panel 自身成员即真值）✓；
- **设计器读回对称性确认**：设计器写按钮/复选文本色经**内部 caption label 句柄直控**（writeColorSlotTo:1712-1719），读回同样经 label ✓——引擎单态写 base+label 双同步后，设计器 label 读回所见=新值 ✓（若只写 base 会读回旧值，双同步是必须项 ✓）；
- 特化路径（button caption-label）改对 `btn` 逐键后，设计器 `applyModelToEngine`/行写回路径（SetColor 到 btn + label 直控并存）兼容 ✓。

### 2.2 P0-77 Bench 透明与视口背景层次——✓（设计正确，Q-3 同意）
- 去除 `initial()` 的 `setTransparent(false)` ✓（构造意图）；**主实例缺省 viewportBackground=(23,23,24,255)** 保持外观零变化 ✓；**子视口缺省透明** ✓（设计器画布=子视口语义不变）；显式 `viewport.background` 覆盖 ✓；alpha=0 → 不填充（游戏帧透出）✓✓——同时满足"预览默认外观不变"与"游戏叠加"两个目标；
- 设计器侧确认：预览实例=主实例 → 缺省深色底 ✓（用户既有观感不变）；设计器画布=子视口 → 透明 ✓（纸面/工作区观感不变，瓦片底色已承载 Bench 背景 ✓）；导出 `viewport.background`（设计器缺省白 #FFFFFFFF）→ 预览白底与编辑态一致 ✓；
- Q-3：同意"主实例缺省不透明深色（外观零变化）"；bench 透明/背景 API 后续另议（`GetRoot`/`Root()` 已暴露，设计器当前无需直连——Bench 编辑经导出/预览闭环已工作）。

### 2.3 P0-77① 阴影禁用——✓（Q-4 同意）
- `drawShadow()` 开头 `m_shadowRadius <= 0 → return` ✓（`r#0`=禁用投影）；`kShadow`（文本阴影）语义不动 ✓；文档注明 ✓。
- 设计器侧：menus 行式 `r#0` 往返已支持（hasRadius 显式位 ✓）→ 引擎落地即生效 ✓。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| Q-1 | 76：逐键写回方案 | **确认**（MenuBar pressed 修复必需路径；转发由单态 override 补齐） |
| Q-2 | 76：CheckBox 是否补单态转发 | **需要**：设计器 check-box 文本色经内部 caption label 直控/读回（与 Button 同款）——不补则 JSON 加载的 text.* 读回不对称 |
| Q-3 | 77：主实例缺省 (23,23,24,255) vs 全透明 | **同意前者**（外观零变化；透明叠加经 alpha=0 显式路径已成立） |
| Q-4 | 77①：`radius<=0` 禁用 | **确认**（kShadow 保持文本阴影语义） |

## 3. 设计器联测计划（同步后）

1. 删除 `ReplayMenuBarColorsTo` → 复测菜单 pressed 经 JSON 解析生效（auto=3 探针保留）；
2. Bench：预览 `#C0C0C0FF` 像素生效（auto=3 bench bg 探针转 OK）；alpha=0 透明场景像素验证；
3. `r#0` 菜单阴影禁用像素验证；默认 8 回归；
4. Button/CheckBox 文本色 JSON 加载读回回归（base+label 对称）；P0-74 既有探针全过；
5. 全量：auto=1/2/3、strict、矩阵、全量像素回归关注主实例缺省底变化。

## 4. 放行

**结论：放行实施**。三项根因与设计器实测一致；P0-76 的逐键+单态转发是修复影子成员问题的正解；P0-77 的主/子实例分层默认是"外观零变化 + 透明叠加"的最优解；落地后设计器回收并回归。
