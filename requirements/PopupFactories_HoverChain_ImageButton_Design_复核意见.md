# PopupFactories_HoverChain_ImageButton_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-23
- 对象：`design/PopupFactories_HoverChain_ImageButton_Design.md`（P0-27d/28/30/31，状态：待评审）
- 结论：**通过，放行实施**（方案 A 推荐、六问全部确认；设计器随批见 §3）

## 1. 分项评审

### P0-27d Popup/ConfirmPopup 工厂（§3.1）✓

- 语义与 CreateDialog 对齐、**不自动 open**（通用浮层按需打开；CreateDialog 保持自动 open 向后兼容）✓。
- `popupPool` 生命期对齐、显隐经既有 `visible` 属性（open/close 通道已核实存在）✓。
- 纯增量（新增 C ABI 2 + Binding 2 + DynamicApi 2）✓。

### P0-28 编辑态常显（§3.2）✓ —— 重要发现：能力已存在

- **`close-on-click-outside` + `close-on-esc` + `visible` 已构成完整能力**（Dialog.cpp:437 门控核实；ContextMenu 继承 Popup 未 override，能力自动继承）——P0-28 无需引擎新代码，文档/用法固化即可 ✓。
- 不引入实例级双轨（避免语义重复）✓；"画布全局抑制"留 §7-2 另立 ✓。
- **设计器随批**：创建 Dialog/ContextMenu/Popup 后 `SetBool("close-on-click-outside", 0)` + `SetBool("close-on-esc", 0)`（§3 用法 1）。

### P0-30 hover/pressed 链路（§3.3 方案 A）✓ —— 推荐

**关键洞察确认**：
- 基类 `onMouseEnter/Leave` 空实现（我此前 grep 误读为"默认切态"——引擎核实正确）→ 仅 Button/CheckBox/Label 自实现；
- **缺省态色陷阱**：全控件切 hover 而不改语义 → 未设色控件 hover 变蓝（ConstDef 缺省）不可接受——"历史缺省显式化"是方案 A 成立的前提 ✓；
- hover 检测（update 的 drawRect.contains）与 isContainsPoint 解耦——Actor/LuotiAni 可达检测，无需改命中语义 ✓。

**方案 A vs B**：**推荐 A**——"未显式设置的状态回退 normal"是更合理的语义（设了 hover 色才有 hover 效果），一次基类切态全控件收益；方案 B 16 控件逐个改且未来新控件仍需逐个。风险对冲充分（历史缺省显式化 + 像素回归 + state 读写语义同步）。

**Pressed 保持各控件自实现**（基类仅切 Hover）——**复核修正（2026-09-23 二次评审）**：该点设计不完整。仅切 Hover 则未自实现按下的控件（Actor/LuotiAni/ColorPicker/NUD/ListView 等）的 **pressed 色槽依然不可达**，P0-30"四态真实可达"目标只完成一半。**补充建议**：
- 基类 `onMouseDown/Up` 对称切 Pressed（分发条件与点击一致——可点击控件才收按下事件，避免误触发行为）；自实现控件（Button/CheckBox/menu）的 override 与基类对齐或调用基类；
- 非交互控件（isContainsPoint=false，如 Actor/LuotiAni/Label）的 pressed 不可达属语义正确（不可点击即无按下态）——设计期预览走**状态预览功能**（P1，强制 setState 渲染）或程序化 setState；
- schema 的 pressed 态声明保留（程序化 setState 可达），设计器色槽后续可标注"交互态需预览"。
- **请引擎在实施时补充此维度**，否则 pressed 对多数控件仍是"声明但不可达"。

### P0-31 ImageButton 移除（§3.4）✓

- 移除清单完整（C ABI/binding+DynamicApi/解析分支/常量/测试/文档）；AnimatedButton 保留 ✓。
- **解析降级 Warn 一个版本**（image-button→button + 迁移提示）——采纳我清单的兼容建议 ✓。
- `kControlTypeImageButton` 常量本批保留仅用于降级识别、随下批删——合理 ✓。
- schema 无 def ✓；设计器已同步清理（工具项/映射/分支）✓。

## 2. §7 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | CreatePopup 不自动 open | **确认** |
| 2 | P0-28 控件属性落地、不引入实例级双轨 | **确认** |
| 3 | P0-30 方案 A（含历史缺省显式化） | **确认，推荐 A** |
| 4 | 基类仅切 Hover、Pressed 保持自实现 | **确认** |
| 5 | image-button 降级 Warn 保留一个版本 | **确认**（推荐） |
| 6 | 测试改"四态图 Button"等价用例 | **确认**（推荐） |

## 3. 设计器随批（引擎实施同步后）

1. **编辑态常显接线**：createViewportControl 的 tDialog/tCtx 分支创建后补 `SetBool("close-on-click-outside", 0)` + `SetBool("close-on-esc", 0)`（替换/叠加现有逻辑）；
2. **tPopup/tConfirm 真实接入**：CreatePopup（不自动 open → 创建后 `SetBool("visible",1)` 编辑态显示）/CreateConfirmPopup + 同款常显键；工具项去"(占位)"标注；
3. **联测**：hover 全控件像素（鼠标移入变色——验收 3 清单）、Dialog/ContextMenu/Popup 编辑态常显、image-button 布局降级 Warn、无回归。

## 4. 放行

**结论：放行实施**。四项需求（新工厂 ×2、能力用法固化、状态语义增强、类型移除）边界清晰、风险对冲充分。实施完成后同步 subModules，设计器按 §3 随批。
