# P071_MenuBarSubViewportPopup_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-06
- 对象：`design/P071_MenuBarSubViewportPopup_Design.md`（P0-71①②，状态：待评审）
- 结论：**通过，放行实施**（Q-1 同批修 ContextMenu；Q-2 维持不读回）

## 1. 评审要点

### P0-71① 根因链 ✓（逐环核实与我方判断一致）

- ① 弹层挂实例 bench ② 定位用全局 `getDrawRect()` ③ Off 模式视口偏移由 bench anchor 携带 ④ bench 子控件 rect 为局部域 ⑤ 偏差 = 视口原点——链路完整、主实例无差解释合理 ✓；
- **同类缺陷核查价值高**：`Popup::computeTargetRect` 已正确换算（Dialog/ConfirmPopup 无此问题）；**`ContextMenu::show` 覆盖了 Popup 的正确 target → 同根因**（设计器画布右键菜单偏移）——我方未单列，确认采纳。

### §2.1 方案 ✓

- bench 局部定位（`mapViewportToCanvas` + `/benchScale`，与 computeTargetRect 同公式）✓；主实例逐位等价（benchDraw=(0,0)、scale=1）回归保证 ✓；fit/stretch 自适应 ✓；离线分支不动 ✓。

### P0-71② ✓（§3.1 核实充分）

- 维持"资源引用不读回"（P0-21 口径 + Actor 对称）✓；§3.1 导出/加载链核实（设计器导出完全基于自身模型；应用加载读 JSON 原值；受影响仅属性面板显示——模型兜底已足）——证据链完整，**设计器侧无新增动作** ✓。

## 2. §5 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| Q-1 | ContextMenu 同根因是否同批修复？ | **确认同批**（同一根因；设计器画布内放置的 ContextMenu 在预览态右键弹层同样偏移——一并覆盖） |
| Q-2 | P0-71② 维持不读回？ | **确认维持**（§3.1 证据链成立；设计器模型兜底已足；未来"引擎侧序列化"单独立项） |
| Q-3 | test_multiviewport_cabi 增 CABI 弹层定位断言？ | 可选（静态探针已覆盖主/子视口对照；若引擎愿意补 CABI 断言更贴近设计器路径，不阻塞） |

## 3. 设计器随批

- 无代码改动（纯引擎侧）；同步后重建设计器 + 联测：
  1. 画布内 MenuBar 菜单弹层对齐（主菜单项正下方）；
  2. 主实例菜单栏（设计器自身）位置回归不变；
  3. 预览态画布内 ContextMenu 右键弹层位置（Q-1 采纳后）。

## 4. 放行

**结论：放行实施**。根因链完整（含 ContextMenu 同类缺陷发现）；方案与既有正确实现（computeTargetRect）同公式、主实例零回归；② 维持决策证据充分。
