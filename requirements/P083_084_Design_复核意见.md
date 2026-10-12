# P083_084_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-09
- 对象：`design/P083_084_Design.md`（P0-84 高 / P0-83 低，状态：待评审）
- 结论：**通过，放行实施**（P0-83 直接放行；P0-84 的 a/b/c 硬化建议采纳，但 **Q-4 答复为"owner 创建"——vp 假设未成立**，我方一致同意继续联合定位）

## 1. P0-84 复核

### §1 机制核实——✓（①-④ 与探针 A/C/C2/B 均认可）
- ① 输入路由；② 投递链；③ Release 下 `validateControl` 静默放行（Debug 断言）；④ 跨实例 AddChild 的 Release 静默语义——核实成立，属于真实可诊断性缺口。

### §1.3 根因判定——**Q-4 答复：设计器属性行 = owner 创建 + owner 容器（vp 假设未成立）**
- 实例证据：`App.cpp:392 m_canvas->Create(*m_ui, …)`（m_ui=主实例）→ `CanvasPane.cpp:287 m_ui = &ui`；
- 容器证据：`App.cpp:229 m_canvas->SetDynamicPropsContainer(m_ui->FindControl("propDynamic"))` → `CanvasPane.cpp:2608`（主实例 propDynamic）；行控件经 `takePoolRow → m_propDynamic.AddChild`；
- 创建证据：属性行一律 `m_ui->Create{EditBox,TextArea,Button,Label,…}`（属性区无 `m_viewport->Create*`；`m_viewport->` 在属性区仅 7 处 `FromHandle` 用于画布控件内部 Label，属正确用法——句柄本就属子视口）；
- **旁证**：设计器 Debug 构建同流程（auto=2 内含 `selectBench()` 建行）**无任何 `NOT owned by instance` 断言** → 该构建路径非跨实例创建。
- 结论：**我方现网代码不存在"画布行以 vp 实例创建"的路径**；vp 假设不能解释用户复现，需继续联合定位。

### 复现差异（请引擎按此收紧探针）
- 引擎探针 **A（owner+owner）通过** ✓、**C 通过** ✓、C2 为 stale-activeViewport 类；但**我方冷启动复现（不经任何画布点击）**= 纯 owner 交互：**控件树点击选中"画布" → 点击属性行 EditBox（focus 正常）→ 输入不达** ✗——**与探针 A 的表面结构相同却失败**，差异候选：①属性行位于 **TabControl 页**内（`TabControl::handleEvent:340 if (!getFocused())…` 的分支行为）；②树控件选中路径（owner 焦点迁移）；③行控件经**池复用**后重绑；④子树先历经过"选中其它控件再选画布"的序列。请引擎以**含 TabControl 页 + 控件树 + 池化 EditBox** 的拓扑复现（我方 Debug 无断言、Release 失败——先按 Release 语义验）。

### §1.4 修改方案——**a/b/c 均建议采纳（无论最终根因）**
- **a) Release 所有权校验** ✓ 必做（静默跨实例=可诊断性缺口；`UI_LOGE`+拒绝，兼容面正常用法均持有）；
- **b) 输入路由按焦点归属** ✓ 建议（消除 C2 类；注意"owner 聚焦优先"多实例场景回归——设计器预览窗与主窗并列时各实例自持焦点）；
- **c) 诊断 API（Release 可用）** ✓ **请务必提供 `Debug_ValidateHandle(inst,handle)`（或取控件归属实例）**——设计器将把它包进属性行构建（Release 亦记录日志），在**用户现网二进制**上直接判定行控件归属，一举定案；此 API 同时是长期自检手段；
- d) 我方答复如上（owner）；若引擎仍存疑，可先以 c) 在设计器侧取证。

## 2. P0-83 复核——✓ 直接放行

- 方案完整：`m_readOnly` + `setBool/getBoolProperty(kReadOnly)`；编辑守卫覆盖 TextInput/退格/删除/回车/Ctrl+V/X + 输入法提交路径；**保留选择/复制/滚动/光标/焦点**（只读≠禁用）✓；
- JSON/schema/docs/CABI 直通 ✓；TextArea 继承生效（Q-5）✓；
- 设计器随批：引擎同步后将资源弹窗路径框 + 画布值显示行/对话框字段切**真只读**并删伪只读（防回环标志一并删）。

## 3. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| Q-1 | Release 启用所有权校验 | **建议启用**（日志+拒绝，不 assert） |
| Q-2 | 输入路由按焦点归属 | **建议采纳**（补"多实例各自焦点"回归断言） |
| Q-3 | 诊断 API | **需要**（请 Release 可用；设计器行构建自检 + 现网取证） |
| Q-4 | 画布行创建实例 | **owner（主实例）**——见 §1.3 证据链；vp 假设未成立，请按"复现差异"收紧 |
| Q-5 | read-only 范围 | **EditBox + TextArea 继承**；保留选择/复制/滚动 ✓ |

## 4. 放行

**结论：放行实施**。P0-83 无异议；P0-84 的 a/b/c 作为硬化先落地（c 为定案关键工具），根因继续联合定位（我方按"复现差异"提供精确流程与 Debug/Release 对照）；落地后设计器恢复画布行内联直编 + 切真只读。
