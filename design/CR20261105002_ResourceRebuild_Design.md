# CR20261105002 — 控件构造与主题显式回放重复资源重建（分析 + 设计）

> 状态：**已关闭（2026-11-06；设计器联测通过）**——实际方案为等值/replay 跳过 + Label 解析期延迟 recreate，未引入通用 LayoutBuildBatch/ThemeApplyBatch；性能目标达成（2026-11-05；按二次复核补齐 B5~B7）
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261105002）
> 关联：CR20261105001（启动性能）、CR20261102001/002（主题/content-padding）

## 1. 现象与基线

设计器 Release 启动约 4.92s：

| 阶段 | 耗时 |
|------|------|
| `LoadLayout(main_layout.json)` | 约 3.81s |
| `ApplyTheme(designer_theme.json)` | 约 0.91s |
| JSON 解析 | 约 1.3ms |
| `setTheme` | <1ms |

设计器插桩结论：

- LoadLayout 中控件树解析/构造约 **2.9–3.2s**；Label 约 40–54ms（构造约 10ms、旧主题字体/颜色约 10ms、通用属性约 10–21ms、caption/布局约 10–12ms），Combo/TextArea 约 80ms。
- ApplyTheme base 规则约 0.76–0.80s，其中 `replayExplicit` 占绝大部分；CSS `applyValue` 接近 0ms；Label/Button 约 9–10ms，Combo/Edit 约 19ms，MenuBar 约 50ms。
- `main_layout.json` 的显式 `font {name,size}` 在主题应用时被重复回放。

结论：瓶颈不是 JSON 解析，而是 **setter 触发的 create/recreate、字体/文本资源释放与重新加载，以及主题显式回放的等值写入**。

## 2. 代码路径与根因

### 2.1 构造阶段多次 recreate

以 `LayoutParser::parseLabel` 为例，当前顺序为：

1. `m_theme.applyCommonColors` / `applyFont`；
2. `parseCommonProperties`；
3. `setCaption`；
4. `setAlignmentMode`；
5. 显式 `font` / `font-size` / `font-style`；
6. `create()`。

`Label::setFont`、`setFontSize`、`setCaption`、`setAlignmentMode`、`setMargin` 等 setter 会调用 `recreate()`（`src/Label.cpp:494`、`src/Label.cpp:504`、`src/Label.cpp:512` 等）；`recreate()` 在 `!m_isCreated` 时会直接 `create()`，导致构造期就提前加载字体/文本，随后每个属性又重复 `releaseTexts`/`create`。

Button 的 `setCaption`（`src/Button.cpp:345`）每次删除并重建内部 caption Label；ComboBox/TextArea 继承 EditBox 的 `setFont`/`setFontSize`/`setText`（`src/EditBox.cpp:569`、`src/EditBox.cpp:654`）每次调用 `loadFontInternal`。

### 2.2 setter 缺少等值短路

- `Label::setFont` 无 `m_fontName`/字体文件相等判断，仍 `releaseFont` + `recreate`；
- `Label::setAlignmentMode`、`setMargin`、`setEnableExpand` 等无等值判断；
- `EditBox::setFont`/`setFontSize`/`setText` 无等值判断；
- `Button::setCaption` 无 `caption == m_captionText` 短路，重复构建内部 Label；
- `ComboBox` 的 `setPlaceholder` 等属性也可能重复写入。

### 2.3 `replayExplicit` 无等值短路

`ThemeRuntime::replayExplicit`（`src/ThemeStyle.cpp:630` 附近）逐键 `setColorProperty`/`setStringProperty`/`setFloatProperty`，不比较当前值。`ApplyTheme` 回放显式 `font`/`font-size`/`content-padding` 时，即使运行时值未变，仍触发 Label/EditBox 的 `recreate`/`loadFontInternal`。这是 ApplyTheme 0.76–0.80s 的主要原因。

### 2.4 字体/文本资源生命周期

`Label::m_fontData` 是控件级缓存；`releaseFont()` 只释放 `m_font`，但 `recreate` 仍释放并重建文本对象。后端 `TextRenderer` 有字体缓存（SDL3/raylib 按内容 hash + size），但“重复触发加载路径”仍产生对象创建、字体度量、文本缓存失效等成本。

### 2.5 缺少批处理边界

解析阶段和主题应用阶段没有统一批处理：
- 每个 setter 可触发 `recreate`；
- `Panel`/容器 reflow 可能被逐个属性触发；
- `ApplyTheme` 无“应用完成后再统一刷新”的边界。

## 3. 优化设计

### 3.1 P0：等值短路 + 显式回放跳过

1. **Label**
   - `setFont`：字体名/文件未变且 `m_font != nullptr`（资源有效）时返回；若同名但字体未加载/已释放，仍需走首次加载/重建；
   - `setFontSize` 已有等值判断，补 `m_font` 有效性检查：等值但资源为空时必须加载；
   - `setAlignmentMode`/`setMargin`/`setEnableExpand`/行高/行距等加入等值判断；
   - `setCaption` 保留等值判断；初始 caption 空时避免无意义 recreate。
2. **EditBox/TextArea/ComboBox**
   - `setFont`、`setFontSize` 等值且字体资源有效时跳过；
   - `setText` 的等值短路**只用于 LayoutParser 初始化/主题回放**，不得改变公开 `SetText` 语义；若文本相同但调用方需要光标/选区复位，仍执行光标/选区/偏移更新，仅跳字体/文本资源重建；
   - `setPlaceholder`/`setAlignmentMode` 等值短路。
3. **Button**
   - `setCaption` 仅当 `caption == m_captionText` **且内部 caption Label 已存在**时短路；caption 未建立时仍需创建；
   - `setCaptionSize` 等值短路，但内部 Label 未建时不跳过首次构建。
4. **主题显式回放**
   - `replayExplicit` 对每个键先读取**当前最终值**（batch 中已写入的最新值）再比较；相同且资源有效则跳过；
   - 覆盖 font、font-size、font-style、content-padding、shadow-offset、corner-radius；
   - 颜色按四态逐通道比较；`SetStateColor` 传入的整组/单通道均先比较；
   - font-style 等 getter 不明确的键**回退为执行写入**，不擅自跳过；
   - 不在 `replayExplicit` 内直接触发 recreate；差异写入交由批处理合并。

### 3.2 P0：解析/主题应用批处理

增加内部 RAII 批处理，不新增公开 API：

```
ThemeApplyBatch / LayoutBuildBatch
  begin: 禁止 create/recreate/reflow/字体释放立即执行，记录 pending
  end:   一次 flush（每控件最多一次 create/recreate；容器一次 reflow）
```

- **B1：直接 create 也必须纳入**。`parseLabel`/`parseButton` 等解析函数结尾的显式 `create()` 不能在批处理内立即执行；要么 parser 删除这些调用、统一由 end flush，要么 `create()` 在 batch 内只记录 pending。
- `recreate()` 触发的 create 与直接 create 合并为同一 pending；`adoptContext`/父子挂载顺序保持“先挂树、后 create”。
- `LayoutParser` 在整棵布局解析开始时 `begin`，解析结束、统一 `create`/reflow 前 `end`；
- `ApplyTheme`/`SetThemeVariant`/`RefreshResponsiveTheme`/`ClearThemeOverrides`/classes 变更均在遍历前 `begin`、遍历后 `end`；
- `Label::recreate`、`EditBox::loadFontInternal`、`Label::setCaption` 等入口在批处理中只记录 dirty，不立即执行；
- 同一控件多次属性写入合并为一次最终重建；
- flush 顺序：**父→子**，先完成容器 create/reflow，再创建子控件字体/文本；`create`、`reflow`、字体加载三类 pending 按此依赖顺序执行；
- **异常/提前 return**：RAII 析构必须 flush 或显式回滚 pending；不能让控件停留在 `m_isCreated=false`/pending 状态；
- batch 为 **per-instance、可嵌套、仅 UI 线程**，不保留跨调用全局状态；嵌套时最外层 flush。

### 3.2a 回调与最终状态

- batch 内 `onPropertyChanged`/CCallback 延迟到 flush 后按最终值触发**至少一次**；
- 同一控件合并多次属性变更，只保留最终一次回调；不得吞掉最终值的通知；
- 回调若再次写入属性，进入下一轮 flush，禁止在 flush 遍历中修改容器子项导致迭代失效。

### 3.2b B5：create 入口清单与批次边界

实现前必须按以下入口逐项登记并覆盖；未列入 batch 的入口视为遗漏：

| 入口 | 处理 |
|------|------|
| `parseLabel/parseButton/parsePanel/parseTabControl/...` 尾部显式 `create()` | parser 在 batch 中**不直接 create**；统一 end flush，或删除调用由 flush 承担 |
| `Button` caption Label、`CheckBox`/`ProgressBar`/`ComboBox` 内部 Label/弹层构建 | 内部复合控件同样受 batch 抑制；最终一次 create |
| `setRect`/`resized`/`reflowChildren` | batch 内只登记 pending reflow；end 时父→子执行一次 |
| `adoptContext`/`AddChild`/`setParent` | 只做挂树/上下文传播，不提前 create；create 延后到 flush |
| `create()` 失败 / 控件销毁 / early return | 清除该控件 pending；不得在已销毁控件上 flush；异常析构 RAII flush 或丢弃 |

- `LayoutParser` 每个 parse 类型（Label/Button/Panel/Tab/CheckBox/ProgressBar/Combo/TextArea）最终 create 次数纳入测试。
- `create`/`recreate`/reflow 三类 pending 分开记录；flush 统一由父→子处理。

### 3.2c B6：flush 最终一致性与回调

- flush 只按**最终值**执行一次；batch 内中间值不产生 create/recreate。
- `onPropertyChanged`/CCallback：
  - 最终值与原值不同才触发一次；最终回到原值时默认不触发（需写入验收）；
  - 回调内再次写属性进入下一轮 flush，限制迭代次数（建议 4 次）超限诊断并停止，防自激；
  - 回调不得在 flush 遍历中增删当前容器子项导致迭代失效。
- `create()` 失败（provider/renderDevice 未就绪）：保留 pending 到上下文就绪后重试；不得让控件永久半创建。
- 顺序：父容器 `create`/`reflow` 完成后再创建子控件字体/文本；需要测量子控件尺寸的宿主在父 reflow 后统一测量。

### 3.2d B7：replayExplicit getter/规范化比较

- 比较必须读取 **batch 已写入的最新属性值**，而不是待 flush 的旧资源状态。
- 各属性规范化：
  - `font`：统一枚举名/字体文件路径，别名归一化后再比较；
  - `content-padding`：解析为 `Margin` 后按数值比较，忽略字符串空格/顺序；
  - 四态颜色：逐通道比较，浮点使用 epsilon；整组 `StateColor` 与逐通道写入分别判断；
  - `font-style`：仅 Label 有可靠 getter；其他控件无 getter 时必须执行写入；
  - `corner-radius`/`shadow-offset`：数值 epsilon 比较。
- **不确定就写入**：getter 不存在、返回非规范值或语义不明确时不跳过；只对有明确 getter + 规范化函数的键做 equal-skip。
- 测试覆盖 font/state colors/padding/shadow/radius 的等值跳过与不等值应用；确保不会漏掉“看起来相等但资源/状态需更新”的场景。

### 3.3 P1：字体/文本资源复用

- `ResourceProvider` 的字体文件读取已有缓存时，复用字体数据，不按控件重复读盘；
- 后端字体缓存已有 content-hash + size 缓存，避免相同字体/字号在批处理 flush 时重复创建；
- 对 Label 的文本对象按最终 caption 只创建一次，取消中间 caption 的 `createText`；
- 字体缓存引用计数/生命周期随 `TextRenderer`，控件只持有 SharedFont/SharedText。

### 3.4 P1：解析顺序与两阶段创建

在不改变可见行为的前提下：

- 解析阶段只记录属性，不提前 `create`；
- 所有属性（主题默认、公共 colors、显式 font/colors、caption、alignment、padding）落定后统一 create；
- 对需要 `onPropertyChanged`/布局回写的控件（CheckBox、ProgressBar 等）保留一次 flush 回调，避免遗漏语义；
- 内部 caption/Label 的构建延后到宿主最终属性确定后。

### 3.5 诊断

复用 CR20261105001 的默认关闭诊断，增加：

```
[Perf] layout create=... recreate=... reflow=... inherit=...
[Perf] theme replay=... equal-skip=... apply=... flush=...
[Perf] font load=... cache-hit=... text-create=...
```

## 4. 验收

- 同一 Release `main_layout.json`/`designer_theme.json`：
  - `LoadLayout` 目标 **≤2.8s**（从 3.81s）；
  - `ApplyTheme` 目标 **≤0.4s**（从 0.91s）；
  - 启动到首帧目标 **≤3.5s**（从 4.92s）。
- 构造期：字体名/字号相同但资源未加载、caption 内部 Label 未建、EditBox 文本对象未建，均能正常首次加载。
- `SetText`：相同文本且光标/选区不同时，光标/选区语义符合公开 API；初始化路径只跳资源重建。
- 显式 font/colors/padding 仍优先；空主题 no-op；变体/media/container/popup/menu 零回归。
- 批处理直接 create：每个控件最终 create 次数符合预期；父容器 reflow 后子控件尺寸正确。
- B5 入口清单覆盖：parser 各类型、内部 caption/Label、setRect/resized/reflow、adoptContext/AddChild；失败/销毁 pending 清理。
- B6 回调：最终值回调一次；值回到原值行为明确；回调二次写入有迭代上限；create 失败可重试。
- B7 规范化：font 别名/路径、content-padding Margin、四态颜色 epsilon、font-style 无 getter 回退写入。
- 回调：onPropertyChanged/CCallback 批处理后最终值至少触发一次。
- 字体资源缓存无泄漏；重复 ApplyTheme、控件删除/重建、切换项目稳定。
- 回归：Debug/Release `auto=2`（272/0）、`auto=3`、`validate_layout --strict`、`test_cr_theme`；耗时以多次 Release 中位数判断。
- 字体缓存无泄漏：重复 create/destroy、切换项目、预览实例、多字体/多字号。
- 诊断验证：批处理后每个控件 create/recreate 次数不高于最终需要次数；显式回放 equal-skip 计数符合预期。

## 5. 风险与待确认

- 批处理期间读取 `getCaption`/字体状态可能拿到中间值；需要明确 finalize 顺序。
- `onPropertyChanged`、CCallback、布局回写可能依赖立即重建；批处理需保留最终一次回调，避免语义回归。
- `setFont` 等值短路在“字体名相同但资源尚未加载”场景必须仍触发首次加载；需区分 `m_font` 为空/未创建。
- 字体/文本缓存的生命周期与 `TextRenderer`/上下文销毁顺序需要回归。
- 若 `replayExplicit` 跳过等值写入，必须确保主题 baseline 与显式值状态一致；否则后续变体切换可能读到旧状态。

## 6. 实施顺序

1. 等值短路 + `replayExplicit` 比较（低风险，先拿 ApplyTheme 收益）；
2. 解析/主题批处理（Label/EditBox/Button，先覆盖高频控件）；
3. 字体/文本资源复用；
4. 解析两阶段创建（最后，回归面最大）。

## 修订记录

- 2026-11-05 初稿：基于设计器 Release 插桩与当前引擎 setter/replay 路径分析；待评审后实施。
- 2026-11-05 按评审修订：B1 直接 create 纳入批处理；B2 等值短路检查资源有效性；B3 SetText 保留光标/选区语义；B4 最终值/回调/四态比较；补异常/嵌套/父→子 flush。
- 2026-11-05 二次复核修订：补 B5 create 入口清单、B6 flush/回调一致性、B7 getter 规范化；补资源三重判断与性能诊断指标。

## 7. 评审响应（2026-11-05）

| 评审项 | 处理 |
|--------|------|
| B1 直接 create | 批处理明确覆盖 parser 显式 `create()`；parser 统一 end flush，create/recreate 合并；补异常/early return 策略。 |
| B2 资源有效性 | 等值短路必须同时满足“值相同 + 字体/文本资源有效/已创建”；资源为空仍走首次加载。 |
| B3 setText 光标语义 | 公开 `SetText` 语义不变；等值短路仅用于初始化/主题回放，保留光标/选区/偏移更新；仅跳字体/文本资源重建。 |
| B4 最终值/回调 | replay 比较基于 batch 最新值；覆盖 font/style/colors 四态/padding/shadow/radius；getter 不明确的键回退写入；flush 后最终回调至少一次。 |
| 批处理作用域 | 补 ApplyTheme/SetThemeVariant/RefreshResponsiveTheme/ClearThemeOverrides/classes/LayoutParser；per-instance、RAII、可嵌套、异常安全。 |
| flush 顺序 | 明确父→子、create/reflow/字体加载顺序；回调二次写入进入下一轮。 |
| 字体缓存 | 确认 ResourceProvider 读盘缓存；后端缓存只减少创建，不替代正确性/生命周期测试。 |
| 诊断/目标 | 统计 create/recreate/equal-skip；目标以多次 Release 中位数验收，Launcher 首帧不属于本 CR。 |
| 两阶段创建 | 保持 P1，P0 视觉/语义回归通过后再推进。 |

## 8. 二次复核响应（2026-11-05）

| 评审项 | 处理 |
|--------|------|
| B5 create 入口清单 | 新增 3.2b 表：parser 各类型、内部 Label/弹层、setRect/reflow、adoptContext/AddChild；create/recreate/reflow 分类 pending；失败/销毁清理。 |
| B6 flush 回调 | 新增 3.2c：只按最终值、最终不变不回调、回调迭代上限、create 失败重试、父→子顺序。 |
| B7 getter 规范化 | 新增 3.2d：font/content-padding/四态颜色/font-style 规范化；不确定就写入。 |
| B2 资源有效性 | 补充 `m_font` + `m_isCreated` + 字体文件/名称三重判断；caption 文本缓存、EditBox 文本对象同样检查。 |
| SetText 初始化 | 用显式 `m_batchApplying`/`m_initializing` 标志区分调用来源，不在公开 SetText 中靠文本相等猜。 |
| 字体文件缓存 | P1 前确认 ResourceProvider 读盘缓存；没有则把读盘缓存纳入 P1。 |
| 性能验收 | 增加 equal-skip 命中、create/recreate 次数、flush 时间三项指标；首帧与 Launcher 分开。 |
- 2026-11-06 P0 实施：Label/EditBox/Button 等值短路、`replayExplicit` 规范化跳过、Label 解析期延迟 recreate；探针 LoadLayout 约 1.76s、designer_theme 约 0.11s、空主题快速路径；全量回归基线不变。完整通用批处理（EditBox/复合控件/回调迭代）未实施。
- 2026-11-06 回归修复：Label 构造期 `setFont` 等值短路误跳过 `m_fontFile` 初始化，导致 `Label::loadFromResource: Error: '' not found`；短路条件补 `!m_fontFile.empty()`。全量测试日志已无该错误。

## 9. 交付结论（2026-11-06）

- 设计器 Release：启动约 3.11s；LoadLayout≈2.44s（≤2.8s）；ApplyTheme≈0.067s（≤0.4s）。
- 回归：Debug/Release auto=2 272/0、auto=3 REPORT-OK、strict PASS、test_cr_theme OK。
- 实际实现未新增通用 `LayoutBuildBatch/ThemeApplyBatch` 符号；采用等值短路、`replayExplicit` 规范化跳过、Label 解析期延迟 recreate 达成目标。
- 若未来需要 EditBox/复合控件/回调迭代的完整通用批处理，另行评估；不作为本次关闭条件。
