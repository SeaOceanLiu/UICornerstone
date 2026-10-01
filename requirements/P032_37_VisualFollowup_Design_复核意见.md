# P032_37_VisualFollowup_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-27
- 对象：`design/P032_37_VisualFollowup_Design.md`（P0-32~37 + 用户 2026-09-27 补充 12 项，状态：待评审）
- 结论：**通过，放行实施**（§5 已确认项无补充；附 2 个小确认点见 §4）

## 1. 分项评审

### 问题确认与根因（§1）✓

源码逐格审计（P0-32~37 全确认 + 7 项新发现）根因明确。**P0-36 调整采纳**：输入族 text 收窄为单态（输入框文字四态语义牵强，用户决策正确）。

### 能力矩阵修正版（§2）✓

- **态子集哲学贯彻**："态子集列出即声明=实现"——每控件能力边界精确到组与态（win-frame 两态、输入族三态+单态、progress 三态、dialog 族两态、menu 统一三态）。
- **"（运行时）"维度清晰**：实现保留但 schema 不声明（面向布局作者；程序化 setState 场景不受限）——与三层合一原则兼容。
- **list/tree 控件级收窄 + item 级新增**：控件级 bg/border 三态（text/shadow 收掉，header 走 item）；item 级全四态（本批最大新增）——层次正确。
- menu-bar 统一三态（pressed/active 不对外，内部缺省）——语义干净。

### 关键设计决策（§3）✓

- **P0-28 能力用法固化**：`close-on-click-outside`/`close-on-esc` 键（不新增实例级双轨）✓。
- **P0-30 方案 A**：StateColor 显式设置标志 + 未设态回退 normal + 历史缺省显式化（Button/Label/CheckBox 构造时写入原缺省值，视觉不变对冲）+ 基类 onMouseEnter→Hover/onMouseLeave→Normal/onMouseDown→Pressed/onMouseUp→恢复（复核修正的 pressed 对称切态已含 ✓）。EditBox 链调基类（m_mouseInside 保留）✓。已自实现控件保留 override ✓。
- **P0-31**：移除清单（C ABI/binding+DynamicApi/解析降级 Warn 一版本/常量保留降级识别/测试改造/文档 27→26）；AnimatedButton 保留 ✓。
- **win-frame 两态语义分离**（§3.3）：bg→ClientPanel（内容区底色）/border→整体窗框（border-visible 联动）；pressed/disabled 取 normal（schema 仅两态）✓。
- **item 级四态**（§3.4）：键设计完整（色四态对象+单色兼容/shadow 开关偏移/border-visible）；存储（TreeNode 扩展/RowStyle，cell>row>控件级优先级）；渲染按行状态；item-id 寻址（TreeView 既有/ListView 新增）；默认回退控件级 ✓。
- **Slider 态同步**（§3.6）：valueLabel setClickable(false)+setState 跟随滑块——"label 不自行检测"语义正确 ✓。
- **ColorPicker 闭合态**（§3.7）：swatch 恒 m_color（色块语义）/底板四态/解除恒 Disabled/弹窗回收+setRect 重定位 ✓。
- **手册唯一权威**（§7）：declarative-syntax 6.4 为矩阵唯一来源、properties 7.1.1 指针化——**避免双份矩阵漂移，好设计** ✓。

### schema 设计（§4）✓

新 def（state-color-2/colors-winframe/colors-input/colors-3full/colors-3state/colors-bg3/colors-basic2）+ colors-menu 统一三态——与矩阵一一对应；item 级键为运行时键不入 schema（与 item-font 一致）✓。

## 2. §5 已确认项

2026-09-27 讨论定案（page-background 转发所有页+自设优先、win-frame bg 仅 ClientPanel、list/tree 控件级 text 运行时键保留、MenuBar 三态、ListView 优先级 cell>item>控件级）——**设计器确认无补充**。

## 3. 设计器随批（引擎实施同步后）

1. **态子集自动适配**：resolveColorGroups 的 kStateSets 补 `state-color-2` 映射（normal/hover）——新 def（colors-winframe/input/3full/3state/bg3/basic2/menu）经 $ref 解析自动生效，无代码改动；
2. **输入族 shadow 行自动消失**（P0-32 收回声明，def 驱动）✓；
3. **font 移出 skip 名单**：button/check-box/win-frame 的 font 运行时分发补齐（§3.2）——动态区生成 font 行（string 写回，引擎分发联测对齐）；
4. **tPopup/tConfirm/tDialog/tCtx**：编辑态常显键已接线 ✓（P0-28 用法，上轮完成）；
5. **联测**：各控件态子集与矩阵一致、item 级键不出现在动态区（运行时键）、font 行写回、win-frame 两态视觉（bg→内容区/border→窗框）。

## 4. 两个小确认点（不阻塞放行）

1. **colors-basic def 去留**：原 colors-basic（sc4）被 colors-3state（sc3d）替代后，原 def 是删除还是保留给未来四态 bg/border 控件？（实现细节，实施时定即可）
2. **tab-control 的 `page-background`/`selected-text` 动态行形态**：StateColor 对象键在动态区的呈现（colors 式展开 or 暂不生成）——建议本批暂不生成（等 def 类型与结构化编辑推进），联测后定。

## 5. 放行

**结论：放行实施**。本批是能力矩阵的精确化落地（声明=实现的全面贯彻）+ item 级四态的首个大新增，验收 13 项覆盖完整。实施完成后同步 subModules，设计器按 §3 随批（工作量小：态子集自动适配 + font skip 移出 + 联测）。
