# P082_MenuBarHoverState_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-09
- 对象：`design/P082_MenuBarHoverState_Design.md`（P0-82，状态：待评审）
- 结论：**通过，放行实施**（根因定位与设计器一致；附 **1 处代码修正** + 1 项依赖注明）

## 1. 评审要点

### §1 问题确认——✓（逐环核实一致）
- ① Move 双模式差异（:1609-1619）✓；② 场景 A（switch 成功 → `m_hoveredIndex` 残留 0 → 悬停底色）✓；③ 场景 B（面板先行消费 → `m_activeIndex` 残留 → 激活底色）✓；④ 无 enter/leave 清理 ✓；⑤ MenuPanel 同类风险 ✓；⑥ 读回缺口 ✓——与设计器复现现象两条路径均吻合。

### §2.1 Move 分支——方向 ✓，**代码须修正一处**
- `m_hoveredIndex = index` 双模式均执行 ✓（-1 自然清除，正解）；菜单模式下先更新再 `switchMenu` ✓；
- **修正（要求）**：拟案中 `return true;` **无条件**（注释"菜单模式保持消费"与代码不符）——将改变非菜单模式的 Move 消费语义：原实现非菜单模式仅在 `drawRect.contains` 时消费（:1623），栏外 Move 落到 `return false` 放行（兄弟控件悬停依赖此链路 ✗）；无条件 true 会让栏吞掉全部 Move → **兄弟控件 hover 回归风险**。应保持：
  ```cpp
  if (event->m_type == EventType::MouseMove) {
      m_hoveredIndex = index;
      if (m_menuMode) {
          if (index >= 0 && index != m_activeIndex) switchMenu(index);
          return true;                    // 仅菜单模式消费（原语义）
      }
      // 非菜单模式：落入原 drawRect.contains 判定（保持放行/消费原语义）
  }
  ```

### §2.2 `onMouseLeave` 覆写——✓
- 基类切态 + 清 per-item 悬停（不退出菜单模式）✓；与 2.1 双保险（场景 B 兜底）✓；移入面板时源标题保留激活色、去悬停 ✓（正确观感）。

### §2.3 MenuPanel 移出清 item 悬停——✓（附依赖注明）
- **依赖**：面板 handleEvent 的**子菜单优先消费顺序**（:957 `m_openSubMenu->handleEvent` 先行）必须保持——指针移入已展开子菜单时由子菜单消费 Move，父面板不会走"未命中 → `setHoveredIndex(-1)`"，否则 `setHoveredIndex(-1)` 的 `setOpenSubMenu(nullptr)`（:600）会误关子菜单 ✗；请测试矩阵补一条：**父项悬停展开子菜单 → 指针移入子菜单 → 子菜单保持展开且父项高亮保持**（已有 d 类断言，请明确含子菜单跨面板移动）。

### §2.4 读回补齐——✓（Q-2 同意）
- `getHoveredIndex()` + `getIntProperty("hovered-index")` **只读**（写回不暴露，维持状态机单一驱动）✓——探针/设计器状态显示需要；与 MenuPanel 现有读写并存无冲突。

### §2.5 不改动——✓
- 展开语义（移出栏不自动关闭、点外部/Esc 关闭）保持标准行为 ✓（Q-4 确认）；绘制色源/面板消费链不动 ✓。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| Q-1 | Move 双模式更新 + leave 覆写双保险 | **确认**（按 §2.1 修正后）；leave 覆写为场景 B 必需兜底 |
| Q-2 | `hovered-index` 只读读回 | **建议补** ✓（探针断言 a-f 均依赖；设计器状态显示亦受益） |
| Q-3 | MenuPanel 移出清悬停同批 | **确认**（附 §2.3 子菜单依赖注明与补测） |
| Q-4 | 展开后移出栏不自动关闭保持标准行为 | **确认**（用户对"移开即收起"的疑问已澄清为语义定夺——维持标准） |

## 3. 设计器联测计划（同步后）

1. 用户复现步骤：点击"文件"展开 → 移向"帮助" → "文件"恢复常态、"帮助"激活 ✓；
2. 像素差分：修复前后残留底色消失；菜单模式移出栏/移入面板/移入子菜单各态；
3. 回归：全量 auto=1/2/3 + strict + 矩阵（设计器无代码改动，纯引擎修复）。

## 4. 放行

**结论：放行实施**。根因双路径定位准确、修法与测试矩阵完备；仅需修正 §2.1 `return true` 的条件化并注明 §2.3 子菜单依赖。
