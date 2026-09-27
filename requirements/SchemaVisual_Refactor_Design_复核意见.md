# SchemaVisual_Refactor_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-23
- 对象：`design/SchemaVisual_Refactor_Design.md`（P0-26，状态：待评审；另确认 P0-24/P0-25 已落地）
- 结论：**通过，放行实施**（§8 全部问题已在讨论定案中确认，无剩余；设计器随批 4 项见 §3）

## 1. 分项评审

### 能力矩阵（§3）✓

- 12 组分类（full/menu/basic/basic-1/bg-only）边界清晰，粒度到 colors 子组与态子集（menu 三态/dialog 单态）。
- **image/animation 获得背景/边框绘制（补齐而非否定）**——比我方此前"无背景色概念"的判断更准确：基类字段与状态机本就存在，仅绘制链未接（beforeDraw/afterDraw 两行）。设计器意义：图片/动画可配装饰边框。
- 决策②③合理：menu 角色统一覆盖（粗粒度简单，细粒度角色定制可留未来需求）；dialog/popup 模态无 hover 语义仅 normal 单态。
- 决策"输入控件不加阴影"（edit-box/text-area/combo-box/numeric-up-down）✓ 观感合理。

### schema 结构（§4）✓

- 态子集 def（state-color-4/3d/3p/1）+ 组合 def（colors-full/basic/bg/menu）+ `additionalProperties:false` 严格化 ✓。
- **顶层 `text-shadow` 死键删除**（解析层只读 colors.text-shadow，4 处引用均在组内核实）✓——设计器未使用顶层键，无影响。
- common 保留真通用键（id/flow-weight/enabled/context-menu/events/margin/rect/scale/visible）✓。
- 不设过渡（决策⑦）：strict Error / 非 strict Warn 既有机制已够 ✓——引擎 3 布局逐键核对 + **设计器 main_layout.json 自查**（随批）。

### 运行时补齐（§5）✓

- 渲染助手抽取（Label 重构为调用方 + 像素回归）最大化复用 ✓。
- Actor/LuotiAni 两行补齐四态背景/边框（经基类状态机）✓。
- ColorPicker/Slider 通用键转发（closed-*/label* 保留兼容）✓。
- MenuBar 映射表明确（normal→BAR_BG/PANEL_BG、hover→…、pressed→BAR_ACTIVE_BG、item 按下沿用 hover 文档注明）✓。
- **ListView 方案 A（新增 ABI 纯增量）**——方案 B（扩签名）因动态解析（GetProcAddress）破坏性已否决 ✓ 判断正确。
- **单色决策有规则支撑**："text 无四态则 shadow 无四态"——TreeView/ListView 文本均为单色 SColor（行号核实）✓。

### P0-24/P0-25 落地确认 ✓

- P0-24（path 别名）：设计器 `path→animation` 特判映射将随批删除（键一致性恢复）。
- P0-25（空路径创建）：设计器维持缺省路径方案（更稳，不折腾）。

## 2. §8 待审核问题

全部已于 2026-09-23 讨论定案（①selected-text 专用键、②单色 shadow 键、③menu 背景边框可定制、④dialog 单态 JSON、⑤border-visible 下放范围）——**设计器确认无补充**。

## 3. 设计器随批（引擎实施同步后）

1. **删除 noVisual 补缺排除**：common 无视觉键后补缺排除失去意义，纯 schema 驱动达成。
2. **colors 展开按 def 态子集生成**：解析类型 def 的 colors $ref 组合（colors-full→四态/colors-menu→background 3 态+text 3 态+border 单态/basic-1→normal 单态），替换 kStates 硬编码四态——避免 menu/dialog 生成不可达槽。
3. **main_layout.json strict 自查**：设计器布局对照新声明集排键。
4. **删除 path→animation 特判**（P0-24 别名生效）；win-frame 的 text.hover/pressed 过滤保留（视觉语义，与 schema 无关）。
5. **联测**：image/animation 背景边框（新能力）、各补齐项视觉（slider label 四态/menu 文本色/statusbar/tabcontrol/progressbar shadow/list-tree item shadow）、态子集正确性、TabControl selected-text 行。

## 4. 放行

**结论：放行实施**。这是一次高质量的三层合一重构（schema 声明=布局合法=视觉有效），能力矩阵与补齐范围经充分讨论定案。实施完成后同步 subModules，设计器按 §3 随批进入纯 schema 驱动形态。
