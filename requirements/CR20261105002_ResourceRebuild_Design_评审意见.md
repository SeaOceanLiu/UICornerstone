# CR20261105002_ResourceRebuild_Design 评审意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-05
- 对象：`D:/GitSpace/UIControls/design/CR20261105002_ResourceRebuild_Design.md`（待评审）
- 结论：**有条件通过（方向正确；实施前需补齐 4 个关键语义）**

## 1. 结论认可

- 根因判断与设计器插桩一致：JSON 解析约 1.3ms，LoadLayout 主要耗在控件构造/setter 重建；ApplyTheme base 约 0.76s，CSS `applyValue` 接近 0，显式回放占主导。
- “等值短路 → 批处理 → 字体/文本复用 → 两阶段创建”的 P0/P1 顺序合理，风险从低到高。
- `TextRenderer` 确实已有 content-hash + size 字体缓存（SDL3/raylib，`src/backend/*/TextRenderer.cpp`），字体复用方向正确；但不能把缓存存在当成正确性保证。
- 目标值 `LoadLayout ≤2.8s / ApplyTheme ≤0.4s / 启动 ≤3.5s` 方向合理；首帧 <1s 仍由独立 Launcher 负责，不属于本 CR。

## 2. 实施前必须补齐

### B1. 批处理必须覆盖直接 `create()`，不能只拦 `recreate()`

当前 `LayoutParser` 的 `parseLabel` 等函数在结尾直接调用 `label->create()`；很多控件类型也有同样的显式 create。若 `LayoutBuildBatch` 只拦截 `recreate()`/setter：

- 解析期仍会提前创建字体/文本；
- 批处理 end 后再 create 仍重复；
- P0 收益可能远低于估算。

设计需明确：

- `create()` 也在 batch 中只记录 pending（或由 parser 删除这些调用，统一 end flush）；
- `recreate()` 触发的 create 合并；
- 直接 create 与 `adoptContext`/父子挂载的顺序；
- exception/early return 时 flush 或回滚策略。

### B2. 等值短路必须检查资源有效性

例如 `Label::setFont` 当前会 `releaseFont + recreate`；如果只比较 `m_fontName`：

- 控件构造时字体名相同但 `m_font == nullptr`；
- 主题/上下文重建后字体已释放；
- 此时短路会导致字体永远不加载。

同样问题存在于 `setFontSize`（已有等值判断但资源可能为空）、`Button::setCaption`（内部 caption Label 可能尚未创建）、`EditBox::setFont/setText`（字体/文本对象尚未加载）。

设计要求：

- 短路条件同时检查“值相同 **且** 资源有效/已创建”；
- 若资源未加载，只跳过“释放/重建”中的不必要部分，不能跳过首次加载；
- `replayExplicit` 的等值跳过也必须遵守同一规则。

### B3. `EditBox::setText` 等值短路必须保留光标/选区语义

`EditBox::setText` 当前即使文本相同也会：

- 将光标移到末尾；
- 清除选区；
- 重新加载字体/更新偏移。

若直接按文本相等返回，会改变运行时 API 的可观察行为。设计需要明确：

- 文本相同但光标/选区需要复位时，仍执行状态更新；
- 只跳字体/文本资源重建；
- 或者限定等值短路仅用于 LayoutParser 的初始属性设置，不改变公开 `SetText` 语义。

### B4. 批处理与显式回放需要明确的“最终值”顺序

`replayExplicit` 的等值比较若发生在 batch 中，必须基于**已写入的最新属性值**，而不是待 flush 的旧值；同时要覆盖：

- font / font-size / font-style；
- colors 四态（normal/hover/pressed/disabled）；
- content-padding、shadow offset、corner-radius；
- `setStateColor` 四通道的整体/逐通道比较；
- getter 不可用或语义不明确的键必须回退为“执行写入”，不能擅自跳过。

还需明确 batch 内 `onPropertyChanged`/CCallback/布局回写的最终一次触发，避免“只合并资源重建、吞掉最终回调”。

## 3. 其它建议

1. **批处理作用域**：`ApplyTheme`、`SetThemeVariant`、`RefreshResponsiveTheme`、`ClearThemeOverrides`、classes 变更、LayoutParser 都需要明确的 begin/end 入口；当前设计只列了前三项，建议补 `ClearThemeOverrides`/classes。
2. **batching 不能在 `applyControl` 之外留下全局状态**；建议 per-instance、可嵌套、RAII，异常退出不留下悬挂 batch。
3. **父→子 flush**：明确 create/reflow/字体加载三类 pending 的先后；避免父容器未完成布局时子控件已按旧 rect 排版。
4. **两阶段创建**风险最高；建议只作为 P1，并先保证 P0 的等值/批处理通过视觉回归。
5. **字体文本缓存**：确认 `ResourceProvider` 字体读取是否真的缓存；若没有，P1 需补读盘缓存，否则“复用”只减少后端字体创建，仍可能重复读文件。
6. **诊断**：`[Perf]` 建议同时统计每个控件的 create/recreate 次数及 equal-skip 命中数，验收时可直接证明重建次数下降。
7. **目标值**：建议按多次 Release 中位数验收；不要以单次 4.92s 的启动总值为唯一目标，且 Launcher 的首帧目标与本 CR 无关。

## 4. 必要验收矩阵

- 构造期：同字体/同字号但 `m_font` 未加载、caption 内部 Label 未建、EditBox 文本对象未建，均能正常首次加载。
- `SetText`：相同文本且光标/选区不同时，光标/选区语义符合 API 约定。
- `ApplyTheme`：显式 font/colors/padding 仍优先；等值跳过不改变最终视觉；重复 ApplyTheme 稳定。
- 空主题 no-op、变体/media/container、popup/menu、classes/ClearThemeOverrides 零回归。
- LayoutParser 直接 create 的批处理：每个控件最终 create 次数符合预期；父容器 reflow 后子控件尺寸正确。
- onPropertyChanged/CCallback：批处理后最终回调至少触发一次且状态为最终值。
- 字体缓存：重复 create/destroy、切换项目、预览实例、多字体/多字号无泄漏。
- 回归：Debug/Release `auto=2`（272/0）、`auto=3`、`validate_layout --strict`、`test_cr_theme`；同机 Release 阶段计时。

## 5. 放行结论

**有条件通过**。方向与目标合理，但 B1（直接 create 的批处理）、B2（资源有效性）、B3（setText 光标语义）、B4（最终值/回调顺序）必须在实施前写入设计并补测试；完成后再进入 P0。P1 的字体/文本复用和两阶段创建可在 P0 回归通过后推进。
