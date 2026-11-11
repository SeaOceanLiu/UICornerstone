# CR20261105002_ResourceRebuild_Design 二次复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-05
- 对象：`D:/GitSpace/UIControls/design/CR20261105002_ResourceRebuild_Design.md`（修订版，待复核）
- 结论：**有条件通过（设计方向已闭合 B1–B4；实施前需补全 B5/B6/B7 三个执行级契约）**

## 1. 已闭合

- **B1 直接 create**：明确 parser 直接 `create()` 也必须纳入 batch；不再只拦 `recreate()`。
- **B2 资源有效性**：`setFont`/`setFontSize`/caption 内部 Label/文本对象均要求“值相同 + 资源有效”才短路。
- **B3 SetText 语义**：公开 `SetText` 保持光标/选区/偏移语义；等值短路只用于初始化/主题回放，并明确只省资源重建。
- **B4 显式回放**：比较基于 batch 最新值；覆盖四态颜色、font/style/padding/shadow/radius；getter 不明确回退写入；flush 后最终回调至少一次。
- batch 作用域补 `ClearThemeOverrides`/classes；per-instance、可嵌套、异常 flush；父→子 flush 顺序；P1 两阶段保持后置。整体方向认可。

## 2. 实施前仍需补齐

### B5. 直接 `create()` 的盘点与批次边界

设计给出“parser 删除 create 调用，或 create 在 batch 内记录 pending”两种方式，但没有列出所有入口。当前实际代码里：

- `parseLabel`/`parseButton`/`parsePanel`/`parseTabControl` 等多个 `parse*` 函数有显式 `create()`；
- `Button`/`CheckBox`/`ProgressBar` 等复合控件内部还可能有自己的 create/lazy label；
- `adoptContext`、`AddChild`、`setRect`/父链变更也可能触发 create/recreate/reflow。

实施前应给出一份**入口清单**，并明确：

- parser 结束时哪些 create 被删除、哪些由 batch flush 承担；
- 内部复合控件的 create 是否也受 batch 抑制；
- `setRect`/`resized`/`reflow` 是否纳入 pending，以及最终父→子顺序；
- `create` 失败/控制被销毁时 pending 如何清除。

否则 B1 仍可能“设计上覆盖、代码上漏口”。

### B6. flush 的最终一致性与回调语义

- batch 内属性值可能反复变化；最终 flush 只应按最终值创建/回放。
- `onPropertyChanged`/CCallback：
  - 若值最终回到原值，是否仍触发一次？设计写“至少一次”，需明确避免无意义回调；
  - 若回调再次写入属性，进入下一轮 flush；需限制迭代次数/防自激。
- `create()` 是插入子控件还是仅重建自身？provider/renderDevice 尚未就绪时 flush 失败如何处理。
- 父容器 create/reflow 与子控件字体/文本加载的依赖顺序需明确到“先父 create/reflow，还是先子字体测量”；不同控件可能不同。

### B7. `replayExplicit` 的 getter/规范化比较

设计已要求“getter 不明确则回退写入”，但还需定义：

- `font` 的 `GetString` 返回值是否规范化（别名、字体文件路径、默认字体）；
- `content-padding` 字符串空格/顺序归一化；
- 四态颜色的 per-channel 比较及浮点 epsilon；
- `font-style` 对非 Label 控件无 getter 时的回退；
- batch 尚未 flush 时读取的是“属性值”还是“资源状态”，必须统一。

这些直接决定 equal-skip 是否会把“应该更新但当前看起来相等”的样式漏掉；建议在实现中默认“不确定就写入”，只在有明确 getter + 规范化函数时跳过。

## 3. 非阻塞建议

1. **B2 的 `setFont` 判断**建议使用“字体文件/名称 + `m_font` 有效 + `m_isCreated`”三重条件，不要只看 `m_font`；字体缓存命中不等于文本对象有效。
2. **`Label::setCaption`** 也要检查文本对象/缓存有效，不仅是字符串相同。
3. **`EditBox::setText`** 初始化路径建议使用显式 `m_batchApplying`/`m_initializing` 标志，而不是在公开 `SetText` 里靠文本比较猜调用来源。
4. **`ResourceProvider` 字体文件缓存**在 P1 前先确认是否真实存在；若没有，P1 的“复用”要包含读盘缓存。
5. **性能验收**：`ApplyTheme ≤0.4s` 与 `LoadLayout ≤2.8s` 建议分别报告 equal-skip 命中数、create/recreate 次数和 flush 时间；否则无法判断收益来自哪里。
6. **首帧**：本 CR 目标 ≤3.5s，Launcher 的首帧 <1s 另算；不要把两者合并成一个指标。

## 4. 必要验收补充

- parser 直接 create 清单覆盖；每个 parse 类型（Label/Button/Panel/Tab/CheckBox/ProgressBar/Combo/TextArea）最终 create 次数符合预期。
- `setFont` 同名但未加载、`setCaption` 内部 Label 未建、`setText` 文本对象未建，均首次正常创建。
- `setRect`/`resized`/`reflow` 在 batch 内不丢最终布局；父容器先于子控件完成尺寸。
- 回调：最终值回调一次、无自激、无丢失；值回到原值时的行为有明确测试。
- `replayExplicit`：font/state colors/padding/shadow/radius 等值跳过与不等值应用分别断言；getter 不可用时必须应用。
- 字体缓存：重复 create/destroy、多实例/预览、切换项目、多字体多字号无泄漏。
- 回归：Debug/Release `auto=2`（272/0）、`auto=3`、`validate_layout --strict`、`test_cr_theme`；同机 Release 多次中位数。

## 5. 放行结论

**有条件通过**。B1–B4 的设计意图已闭合，但 B5 的 create 入口清单、B6 的 flush 最终一致性/回调、B7 的 getter 规范化仍是实施级缺口；补齐后可进入 P0。P1 两阶段创建继续保持后置。
