# P038_46_VisualRound3_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-27
- 对象：`design/P038_46_VisualRound3_Design.md`（P0-38~46，状态：待评审）
- 结论：**通过，放行实施**（§7 七问全部确认；设计器随批工作量趋近于零，见 §3）

## 1. 分项评审

### P0-38/39②③ 内部 Label 态同步（§2.1）✓

- **"非交互 + 持续同步"决策正确**：`setClickable(false)` 使 Label 不再自管态（handleEvent 对非 clickable 直接 return）+ 每帧 `setState(getState())` 同步（幂等成本低）；保留 setState override 即时同步——hover/pressed/disabled 三态由控件统一驱动，任意鼠标位置一致。
- Slider 补 setState override（P0-32~37 §3.6 漏项承认并补齐）✓。

### P0-39① Slider offset 独立存储（§2.2）✓

成员缺省 2.0f（对齐 Label 语义）+ 读写走成员（label 有无均可读回）+ 创建时应用——**读/写/缺省三者一致**，正确。

### P0-40 ScrollBar（§2.3）✓

- ① thickness 走通用 Float 通道（垂直→width/水平→height）——零新 ABI ✓；手柄路径联测定位（§7-4）。
- ② **track/thumb/thumb-hover/thumb-pressed 色键已存在**（ScrollBar.cpp:379-382 核实）——面板映射即可、控件级 background 降级为底板语义并文档注明 ✓ 零新增成本。

### P0-41 WinFrame 关闭按钮（§2.4）✓

按评审意见定稿：仅 `setBorderVisible(false)` 消除按钮自身边框线；灰底保留、窗框边框压过按钮顶/右缘保持现状、hover/pressed 红底素材不动——最小改动 ✓。

### P0-42 Splitter（§2.5 方案 a）✓

`colors.background` 三态映射把手线色（normal/hover/新增 disabled 缺省=normal）+ **line*/line-hover/line-drag 专用键优先**（显式专用 > 通用组）✓。方案 b（缩进露背景）观感变化大不推荐——同意。
语义说明：Splitter 全自绘把手线、无底板概念，colors.background 驱动线色符合其视觉本质。

### P0-43 单态边框键自动开启（§2.6）✓

`setNormalStateBDColor` 等统一追加 `setBorderVisible(true)`——与对象路径"设色即显示"对齐，消除单态/对象不一致 ✓。
影响面：缺省无边框控件（Label/CheckBox 等）显式设 border 色后显示边框——语义一致（设色即见）；像素回归对冲 ✓。

### P0-44 MenuBar（§2.7）✓

绘制状态解析（disabled→disabledBg；hover 且无悬停项→hoverBg；悬停项时保持 normal 避免与 item hover 叠加）+ `getStateColorProperty` 读回组装（pressed 取 hover/active 语义）——**绘制与读回双修** ✓。

### P0-45 context-menu def + 解析分支（§2.8）✓

def（colors-basic2 两态 + border-visible + items?）+ 解析分支复用 make_shared<ContextMenu> 挂父保持隐藏 ✓。
**设计器零改动**：def 落地后 resolveColorGroups 自动生成两态色槽（state-color-2 映射上批已就绪）。

### P0-46 font string 别名（§2.9）✓

基类 setString/getStringProperty(kFont) 委托 Enum 通道（FontNameToString 静态串读回安全）——**全控件受益、设计器上轮的 SetString 接线零改动即通** ✓。

## 2. §7 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | P0-42 方案 a（background 三态驱动线色、line* 专用键优先） | **确认** |
| 2 | P0-43 全局"设边框色即开启 border-visible" | **确认**（含像素回归） |
| 3 | P0-45 def 两态 + 解析分支；items 随结构化编辑 | **确认**（items 本批不纳入） |
| 4 | thickness Float 形态（垂直→width/水平→height） | **确认**（手柄路径联测定位） |
| 5 | P0-41 定稿（仅 setBorderVisible(false)） | **确认** |
| 6 | P0-46 string 通道返回枚举名 | **确认** |
| 7 | P0-44 hover 填充规则（无悬停项才 hoverBg） | **确认** |

## 3. 设计器随批

1. **零代码改动项**：P0-46（SetString 接线已就绪，别名生效即通）、P0-45（def 驱动自动生成两态色槽）、P0-40②（引擎面板映射后 bg 槽经通用通道即达 track）；
2. **联测**：验收 10 项全过 + 历史回归（image 边框/动画/编辑态常显不受影响）；
3. 手册 6.3.1 矩阵补 context-menu 行后，设计器态子集脚本（Temp/verify_matrix.py）同步补该行断言。

## 4. 放行

**结论：放行实施**。九项问题的根因核实扎实（含上批设计漏项的诚实更正），方案全部最小化且复用既有能力（track/thumb 色键、Enum 通道、visible 属性）。实施完成后同步 subModules，设计器联测收尾。
