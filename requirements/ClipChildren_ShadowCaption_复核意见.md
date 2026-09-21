# 两设计复核意见（ClipChildren_Nested_Design + ShadowCaption_Unify_Design）

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-21
- 对象：`design/ClipChildren_Nested_Design.md`（clip 嵌套相交修复）与 `design/ShadowCaption_Unify_Design.md`（文字/阴影配置统一）
- 结论：**两份均通过，放行实施**（待审核问题全部确认，附少量补充）

---

## 一、ClipChildren_Nested_Design（P0-11 clip 嵌套相交修复）

### 评审要点

| 项 | 评价 |
|---|---|
| **根因链** | **四处后端 pushClipRect 均“替换而非相交”**（SDL_SetRenderClipRect/glScissor/BeginScissorMode/setClipRect）——滚出行内 EditBox/NUD 的自内容 push（rect 在容器外）**覆盖了容器 clip** → 溢出。与设计器截图完全吻合 ✓✓✓ |
| **修复方案** | 设备层 pushClipRect **与栈顶相交**、栈内保存相交后 rect（pop 恢复天然正确）——**设备层集中修复**，Panel/EditBox/ComboBox/嵌套容器全部路径获益 ✓ |
| **空相交** | SDL SetClipRect 对 0 尺寸**禁用裁剪**（SDL 语义陷阱）→ “渲染目标外 1×1”等价手段 + P0-11-2 像素用例逐一验证——细节到位 ✓ |
| **被否决备选** | 核心层双栈/调用方自行相交/仅 Panel 特判/设计器兜底——否决理由均成立 ✓ |

### 答复与确认

- 相交语义（嵌套取交集）✓；空相交实现路径 ✓；测试载体（test_capture_cabi 像素扩展）✓
- **修复后设计器删除“超界隐藏 -40 容差”兜底、恢复纯偏移滚动** ✓ 确认（联测范围含此）

### 补充（不阻塞）

- **首层 push（栈空）clamp 到视口保持现状** ✓ 已明确。
- 性能（4 次 min/max）可忽略 ✓。

---

## 二、ShadowCaption_Unify_Design（#9/#10/#12 统一）✓

### 评审要点

| 项 | 评价 |
|---|---|
| **单一事实来源 = 内部 Label** | 宿主零新状态（避免双态失同步）；`kShadow` 为主键（`kTextShadowEnable` **删除**——使用方核查：仅引擎内部，无外部依赖）✓ |
| **读回经 Label 访问器** | isShadowEnabled/getShadowOffset 两个内联 getter（宿主转发读），不引宿主字段 ✓ |
| **schema $defs/shadow 抽取** | label 内联改 $ref（语义等价）+ 三段新增——单一来源 ✓ |
| **LayoutParser 通用 shadow 解析** | parseCommonProperties 一处、四类型自动生效；parseLabel 既有分支保留（幂等）✓ |
| **Button 单色 setter** | kShadow（主）替代 kTextShadowEnable（**别名删除**——使用方核查：仅引擎 test_p0_getter.c（上批新建）与 layouts/all_controls.json（演示布局），**无外部/设计器布局依赖**，同批迁移 `shadow.enabled`）✓ |
| **check-box 不入 schema text-shadow-enable** | 避免面板双开关 ✓（**运行时别名亦删除**——同上数据核查，无外部依赖） |

### 答复（§7）

| # | 问题 | 答复 |
|---|---|---|
| 1 | shadow 主键（text-shadow-enable **删除**；Button/CheckBox 布局同批迁移 shadow.enabled） | **确认删除**（使用方数据核查：无外部依赖） |
| 2 | schema 抽 $defs/shadow（label 改 $ref + 三段新增） | **确认** |
| 3 | Label 访问器（isShadowEnabled/getShadowOffset 内联） | **确认** |
| 4 | Parser 通用化（parseCommonProperties 统一 + parseLabel 保留幂等） | **确认** |
| 5 | CheckBox 另补 C++ setCaption(string) 方法 | **按需**——属性分发已覆盖设计器需求；C++ API 对称性可选，不强求 |

### 补充确认（不阻塞）

- **WinFrame 的 title 键**：win-frame 布局的标题键为 `title`（非 text/caption）——§3.3 的通用 shadow 解析经属性系统转发，win-frame 声明 `shadow` 后 setBoolProperty(kShadow) → 转发 title Label ✓（设计器动态区 `title` 行与此无冲突——一个管标题文本、一个管阴影）。
- **设计器迁移确认**（§8 已列）：恢复通用 shadow 展开（schema 驱动）、删类型分派绕行段 ✓——届时全类型（button/check-box/win-frame/label）的 shadow 配置零类型知识。

---

## 三、放行与联测

**两份均放行实施**。同步 subModules 后设计器随批：

1. **clip**：删除“超界隐藏 -40 容差”兜底 → 纯偏移滚动；联测滚动溢出消失（部分滚出/完全滚出/嵌套）。
2. **shadow/caption**：
   - CheckBox：文本行（caption 分发生效）、text/textShadow 各态色槽（shadow 主键生效）、hover/pressed/disabled 视觉（状态联动已实施 ✓）；
   - Button/WinFrame：shadow 三行（统一段改通用展开——schema 驱动）；
   - Button 旧键 `text-shadow-enable` 失效为预期（别名删除）；
   - 回归：既有全绿。

**文档闭环**：颜色组补全的 05/history 更新在上述联测通过后一并收尾。
