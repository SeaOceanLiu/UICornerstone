# P068_TextAreaAutoScroll_Design — TextArea `auto-scroll`（光标跟随 vs 日志跟随）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-05 追加：P0-68）
> 前置：P0-66 批、ScrollBar 拖拽捕获修复（均已实施并同步）
> 状态：**已放行，已实施**（2026-10-05；Q-1 确认缺省 false，无回落）
> 复核：`requirements/P068_TextAreaAutoScroll_Design_复核意见.md`（通过）
> 实施备注：① `PropertyNames::kAutoScroll`；② `TextArea::setAutoScroll/getAutoScroll` + set/getBoolProperty 分支 + **缺省改 false**（TextArea.cpp ctor）；③ LayoutParser `parseTextArea` 增 `auto-scroll` 解析；④ schema text-area 增键（x-default false）；⑤ 文档：textarea.html（属性表 + 示例）、appendix/properties.html、appendix/declarative-syntax.html；⑥ CABI/Binding 通用 SetBool/GetBool 直通（零新增接口）；⑦ 测试：test_property_cabi 168→**178/0**（缺省 false/JSON true 解析/set-readback）+ 行为探针 **P0-68 OK**（false 光标上移跟随到顶 0 / true 恒 168 / false setText 不滚动）；⑧ 全量回归仅 4 项预存失败；⑨ 设计器随批：动态区 + 工具箱 TextArea 显式 `SetBool("auto-scroll", false)`（双保险）

---

## 1. 问题确认（源码核实）

**报告**：动态区（属性面板）TextArea 光标移动出可视区后不自动滚动使光标可见（上移/编辑中部文本尤甚）。

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-68 | TextArea 无 `auto-scroll` 属性；光标跟随不可用 | **确认** | ① `m_autoScroll` 缺省 `true`（TextArea.cpp:16），头文件仅成员变量，**无 setter/属性/API**；② `ensureCursorVisible`（:1362）`m_autoScroll && cursorLine >= totalLines-1` → `scrollToBottom(); return`，且末尾（:1382-1384）`if (m_autoScroll) m_scrollY = maxScroll`——**true 时恒滚到底**（日志式自动跟随），编辑式光标跟随分支（:1367-1380）被完全覆盖 |

**行为细节（影响语义定义）**：
- `ensureCursorVisible` 调用点覆盖输入/退格/回车/方向导航/鼠标定位/选区等全部编辑路径（:682-1201）；
- `TextArea::insertText` 调 `ensureCursorVisible`（:1401）；`TextArea::setText` **不调用**（:1420，仅重建行 + 滚动条）——setText 追加场景不会自动到底；
- `scrollToBottom()` 为公开方法（:1224），可用于显式到底。

## 2. 修改方案

### 2.1 属性语义
- **`auto-scroll`（Bool）**：
  - `true` = 日志式——凡 `ensureCursorVisible` 均滚到底（即现状缺省行为）；
  - `false` = 编辑式——光标跟随：光标行移出可视区即滚动使其可见（`ensureCursorVisible` 正常分支 :1367-1380）；光标在末行输入/追加时仍自然可见（等效到底）。
- **缺省值（Q-1，待复核定形）**：建议改 **`false`**。
  - 依据：编辑式为 TextArea 通用语义（本次报告的缺陷正源于缺省 true）；引擎内测试/样例无依赖缺省 true 者（test_editbox / layouts / samples 均未断言自动到底）；日志式用户影响有限——`setText` 置光标于末尾且 `insertText` 走光标跟随，典型"末尾追加"仍自然到底；纯 `setText` 整体替换需显式 `scrollToBottom()` 或设 `auto-scroll=true`。
  - 回落方案：若复核顾虑严格兼容，保持缺省 `true`（零破坏），设计器侧显式 `SetBool("auto-scroll", false)`（设计器已声明愿随批设置）。

### 2.2 实现点
| # | 位置 | 内容 |
|---|---|---|
| 1 | `include/PropertyNames.h` | `kAutoScroll = "auto-scroll"` |
| 2 | `include/TextArea.h` / `src/TextArea.cpp` | `setAutoScroll(bool)` / `getAutoScroll()` + `setBoolProperty/getBoolProperty` 分支（同 `word-wrap` 款） |
| 3 | `src/LayoutParser.cpp` `parseTextArea`（:1868+） | `auto-scroll`（boolean）解析 |
| 4 | `docs/schema/declarative-ui.schema.json` text-area | `auto-scroll`（boolean，`x-default` 按 Q-1 定形） |
| 5 | `docs/controls/textarea.html` | 属性表 + JSON 示例 + C ABI 示例（通用 `SetBool`） |
| 6 | `docs/appendix/properties.html`、`docs/appendix/declarative-syntax.html` | 速查表同步 |
| 7 | C ABI / C++ Binding | **无需新增**——通用 `UICornerstone_SetBool/GetBool`（UICornerstoneAPI.h:765）与 Binding `Control::SetBool`（binding/include/Control.h:32）直通属性系统 |

### 2.3 不改动
- `ensureCursorVisible` 既有逻辑（true 分支保留现状）；
- `scrollToBottom()` 公开语义；
- 其他滚动逻辑与事件路径。

## 3. 测试计划
- **属性回归**：test_editbox.cpp TextArea 段——缺省读回（按 Q-1）、`set false → true → false` 读回；JSON 解析读回（内联或样例）。
- **行为探针**（Temp，静态）：多行文本 + 小高度 TextArea：
  - `auto-scroll=false`：光标移至首行（模拟 Ctrl+Home/方向键）→ `scroll-y=0`（光标跟随）；末尾 `insertText` → 仍到底；
  - `auto-scroll=true`：光标移至首行 → `scroll-y=max`（日志式）；
  - `setText` 替换 + false → 不自动滚动（文档化行为）。
- **全量回归** + 样例（如新增 `layouts/p068_textarea.json` 走 ctest strict）。

## 4. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 缺省值：`false`（编辑式，语义优先）vs `true`（零破坏） | **false**（理由见 2.1；回落方案可行） |
| Q-2 | 属性名 `auto-scroll` / 常量 `kAutoScroll` | 确认 |
| Q-3 | `false` 下 `setText` 整体替换不自动到底（需显式 `scrollToBottom()` 或 `auto-scroll=true`） | 确认（与编辑式语义一致） |
| Q-4 | 范围：仅属性暴露 + 缺省决策，不动其他滚动逻辑 | 确认 |
