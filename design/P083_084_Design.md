# P083_084_Design — 画布选中态 EditBox 输入失效定位 / EditBox 只读属性

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-09 追加：P0-84 高、P0-83 低）
> 前置：P0-82 批已实施并同步
> 状态：**已放行，已实施**（2026-10-09；P0-83 全量；P0-84 a/b/c 硬化落地，根因继续联合定位）
> 复核：`requirements/P083_084_Design_复核意见.md`（通过；Q-4 答复 owner 创建——vp 假设未成立；a/b/c 采纳，c 为定案关键工具）
> 实施备注：① **a** `validateControl` 归属校验**全构建启用**（Release 日志+拒绝，Debug 另加断言）；② **b** 新增 `keyboardTarget`——TextInput/KeyDown/KeyUp（轮询+注入两通路）改**焦点归属优先**（owner 聚焦→owner；否则 activeViewport）；③ **c** `UICornerstone_Debug_ValidateHandle`（Release 可用）+ Binding `DebugValidateHandle`；④ **P0-83**：EditBox `m_readOnly` + set/get + setBool/getBool(kReadOnly) + 编辑守卫（TextInput/Backspace/Ctrl+V/X + cut/paste 中央守卫）、TextArea 继承守卫（TextInput/Return/V/X）+ JSON 解析（edit-box/text-area）+ schema + 文档；⑤ 验证：**P0-84c 探针**——合法拓扑（含 TabControl 页/树点击/池复用/序列）全过（p084b 早前失败系点击坐标未命中）；`Debug_ValidateHandle same=1/cross=0/vp=1` ✓；**C3 stale activeViewport + owner 聚焦 → 输入仍达 owner** ✓（b 生效）；**P0-83 探针 OK**（只读只拦编辑、选择保留、读回、JSON edit-box/text-area）；⑥ 全量构建 0 错误、回归仅 4 项预存；⑦ **根因联合定位**：设计器冷启动复现（owner 创建+owner 容器、焦点可见、输入不达）引擎在合法拓扑下未复现；请设计器将 `DebugValidateHandle` 包进属性行构建（Release 记录日志）在现网二进制取证——若为 0 即跨实例句柄误用（a 已将其变为可诊断拒绝）；若为 1 请提供最小复现流程，引擎继续深挖（候选：真实后端 TextInput 与注入差异、多窗口焦点）。

---

## 1. P0-84 定位（引擎复现 + 机制核实）

### 1.1 已核实机制
| # | 事实 | 位置 |
|---|---|---|
| ① 输入路由 | `TextInput/KeyDown → kbdTarget = activeViewport ? activeViewport : instance`（轮询与注入两通路一致） | src/UICornerstoneAPI.cpp:820-847 |
| ② 投递链 | `dispatchToBench` → `bench->inputControl` → `triggerEvent`（入该实例事件队列）→ `Update → eventLoopEntry` 弹出 → `handleEvent` 链（逆序子控件；EditBox 仅以 `m_focused` 为门槛） | API:399-402、ControlBase.cpp:792-793/1177-1205、EditBox.cpp:369-389 |
| ③ 跨实例句柄 | Debug：`validateControl` 归属校验失败 → `assert`（探针实测命中 `"control handle ... NOT owned by instance"`，API:151-155）；**Release：`#else` 分支直接返回裸指针，无任何校验** | API:151-159 |
| ④ 重挂语义 | `AddChildControl` 对非本实例句柄：Debug 断言、Release 静默 `validateControl` 通过但 `dynamic_cast<Panel*>`/`shared_from_this` 等按传入实例语义执行 → 控件实际仍属原实例树 | API:2082-2108 |

### 1.2 探针结果（Temp/temp_p084_probe，owner + 子视口）
- **A) owner 创建 + owner 面板 + owner 点击/注入**：文本 `A1` ✓（合法同实例路径正常）；
- **C) 先点子视口（activeViewport=vp）→ 点 owner EditBox → 注入 owner**：文本 `C1` ✓（鼠标路由在 owner 区按下时已清 activeViewport）；
- **C2) activeViewport=vp 且不点 owner 框，注入 owner**：输入落到 vp（owner 框保持 `C1`）——**stale activeViewport 类可复现**（真实场景：owner 焦点控件存在但 activeViewport 未清时，TextInput 全部发往子视口）；
- **B) vp 创建 EditBox → AddChildControl 到 owner 面板**：Debug 断言（非本实例句柄）。

### 1.3 根因判定（与设计器证据全链吻合）
设计器"**由画布构建期创建的 EditBox 是坏的** + 池复用传染 + TextArea/对话框正常 + 与 (0,0) 创建无关"的最优解释：
**画布选中态下属性行 EditBox 以画布子视口实例创建**（设计器此前"统一 m_ui→m_viewport"涉及 33 处，属性面板容器仍为 owner）→ Release 构建下 `validateControl` 静默放行 → 行控件实际归属 vp 树/实例，owner 的输入通路（队列/焦点/绘制链）不达 → 输入无效；复用该池控件到控件行后把"错误实例归属"带入好场景 → 传染 ✓。
（Debug 构建会直接断言，与设计器"复测仍不能输入"不矛盾——设计器联测为 Release。）

### 1.4 引擎侧修改方案（硬化 + 路由改进，供复核定形）
- **a) Release 所有权校验（建议必做）**：`validateControl` 在 Release 也执行 `instanceHoldsControl`，失败 `UI_LOGE` + 返回 nullptr（不 assert）——把静默跨实例误用变为"可诊断拒绝"；评估兼容面（正常用法均持有，无行为变化）；
- **b) 输入路由按焦点归属（建议）**：`TextInput/KeyDown` 目标改为——owner `focusManager->getCurrentFocused()` 非空 → owner；否则 activeViewport 有聚焦控件 → vp；否则 activeViewport/owner 兜底。消除 stale activeViewport 类（C2），且不依赖鼠标路由清理时机；
- **c) 诊断 API（可选）**：`UICornerstone_Debug_GetControlInstance(handle)`（或 `Debug_ValidateHandle(inst,handle)`）供设计器行构建自检（Release 可用）；
- **d) 设计器侧确认（Q-4）**：画布选中行创建所用实例——若确为 vp，设计器改 owner 创建 + 引擎 a/b 兜底；若为 owner，请提供最小复现（哪个实例/容器/构建调用序），引擎继续深挖。
- 过渡现状：设计器"画布行只读显示 + 画布设置对话框编辑"可用，不阻塞。

## 2. P0-83 EditBox 只读属性

**现状**：EditBox 无 `read-only`（仅 NumericUpDown 有：src/NumericUpDown.cpp:291/322、LayoutParser.cpp:2378）；设计器以"伪只读"过渡。

**方案**：
- `EditBox` 增 `m_readOnly` + `setReadOnly/isReadOnly`；`setBool/getBoolProperty` 分支（`kReadOnly`）；
- 编辑守卫（保留选择/复制/滚动/光标/焦点）：TextInput 分支、Backspace/Delete/Return（多行回车）、Ctrl+V/Ctrl+X、输入法提交路径（TextArea 继承同语义）；
- JSON：edit-box def 增 `read-only`（bool，缺省 false）+ `parseEditBox` 解析；TextArea 经继承同步生效（Q-5）；
- schema edit-box 增键 + docs（editbox.html 属性表 / appendix/properties / declarative-syntax）；
- CABI/Binding 通用 SetBool/GetBool 直通（零新增接口）。

## 3. 测试计划
- **P0-84**：探针回归（A/C 合法路径、C2 路由修复后 owner 焦点优先、Release 校验：跨实例句柄被拒并记录日志）；设计器联测（画布行输入 + 控件行不受影响）；
- **P0-83**：探针——read-only 下 TextInput/退格/删除/粘贴/剪切不改文本；选择/复制/滚动/焦点正常；SetBool/GetBool 读回；JSON 解析；全量回归（仅 4 项预存）；
- 文档/schema 同步 + release/subModules。

## 4. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | Release 启用 `validateControl` 所有权校验（日志+拒绝，不 assert） | 建议启用（根治静默误用；兼容面已评估） |
| Q-2 | 输入路由改为"焦点归属优先"（owner 聚焦 → owner；否则 activeViewport） | 建议采纳（消除 C2 类） |
| Q-3 | 诊断 API（Debug_GetControlInstance / Debug_ValidateHandle）是否需要 | 可选（设计器行构建自检） |
| Q-4 | 设计器确认：画布选中行创建所用实例与容器（若 vp → 改 owner；若 owner → 提供最小复现） | 请复核答复 |
| Q-5 | read-only 范围：仅 EditBox 还是含 TextArea（继承）/是否保留选择复制 | EditBox + 继承生效；保留选择/复制/滚动（只读≠禁用） |
