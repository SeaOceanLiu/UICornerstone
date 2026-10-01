# P047_48_FontSelection_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-30
- 对象：`design/P047_48_FontSelection_Design.md`（P0-47/48 系列，状态：待评审）
- 结论：**通过，放行实施**（§7 五问全部确认；设计器侧两项误报澄清与随批见 §3）

## 1. 分项评审

### 探针复现的澄清价值（§1）✓

双通道（C ABI + Binding）复测 + 像素级验证，澄清了设计器侧多项误报：
- **P0-47② WinFrame font 读回**：C/Binding 双通道均正确（初始返回缺省 'harmonyos-sans-sc-regular'、设置后读回一致、title 无串扰）——设计器上轮报告系误报（疑观察混淆），待复验。
- **P0-48 扩展① 字体视觉**：像素级确认切换生效（Button 519px/Label 595px 差异）——**与设计器用户反馈一致**（用户已确认"能看到字体变化"），该项关闭。
- **P0-48 触发语义更正**：交互（下拉点选）回调正常触发（实测 7 次）；**程序化 SetInt(selected-index) 不触发**（setSelectedIndex 只更新不 fire）——精确区分了两种路径。
- **P0-47 扩展**：除 Slider（空，同①）外全部文字控件未设置即返回有效缺省——修复 Slider 后全矩阵覆盖。

### 修改方案（§2）✓

- **2.1 Slider kFont 读回**：与 kLabelFont 等价返回 FontNameToString(m_labelFont)——全矩阵对齐 ✓。
- **2.2 setSelectedIndex 变更触发对齐**：变更检测 + 与 selectItem 同口径触发；解析期（JSON）天然不触发（回调注册前）✓；setSelectedValue 同路径 ✓。
  设计器受益确认：E refresher 的 selected-index 回填为同值（变更守卫→不触发）或幂等写回（异值触发→写回值与读回一致，无害）——无回环风险 ✓。
- **2.3 字体清单 API**：GetFontCount/GetFontName（静态、单一数据源 ConstDef 字体表）+ Binding 封装——设计器 ComboBox 选项从写死清单改为 API 动态获取（随批）✓。形态建议前者（零解析）确认。
- **schema font-name 镜像一致**（6 项）无需动 ✓。

### 待复现两项的处置（§2.5）✓

本批不实施无问题改动、请求复现信息——**严谨**。设计器侧回应见 §2。

## 2. §7 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | 程序化 setSelectedIndex 变更即触发（对齐 selectItem） | **确认** |
| 2 | 同值写入不触发（变更守卫） | **确认** |
| 3 | P0-47②/48 扩展① 待复现，确认则关闭 | **确认**——P0-48 扩展① **设计器正式关闭**（用户实测"能看到字体变化"）；P0-47② 待联测复验 WinFrame font 读回 |
| 4 | 清单 API 形态：Count/GetName（前者） | **确认** |
| 5 | 读回取控件字体成员、父链继承显示名不参与 | **确认可接受** |

## 3. 设计器随批（引擎实施同步后）

1. **font ComboBox 选项动态化**：替换写死 kFontNames → `GetFontCount/GetFontName` API（与引擎枚举单一数据源对齐）；
2. **P0-48 扩展① 关闭存档**（用户实测字体可见）；
3. **联测**：Slider font 读回（修复后）、selected-index 程序化触发、WinFrame font 读回复验（若复现按 §2.5 清单提供：句柄来源/键常量/DLL 版本）；
4. 态子集脚本（verify_matrix.py）无需变更（font-name schema 镜像未动）。

## 4. 放行

**结论：放行实施**。探针驱动的精准澄清（三项设计器误报/部分确认）+ 最小化方案（仅 Slider 读回 + 触发对齐 + 清单 API）。实施完成后同步 subModules，设计器随批联测。
