# CR20261105002_ResourceRebuild_Design 三次复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-05
- 对象：`D:/GitSpace/UIControls/design/CR20261105002_ResourceRebuild_Design.md`（修订版 v2）
- 结论：**通过（附三条实施约束）**

## 1. 复核结论

B1~B7 的设计意图已闭合：

- B5 已补 create/create/reflow 入口清单，内部复合控件、setRect、adoptContext/AddChild、失败/销毁均纳入；
- B6 已明确最终值一次 create、回调不改变则不触发、回调迭代上限、create 失败 pending 重试、父→子顺序；
- B7 已明确 font/content-padding/四态颜色/font-style 的规范化与“不确定就写入”；
- `SetText` 初始化路径用显式标志区分，公开 API 语义保持。

同意进入 P0 实施。

## 2. 实施约束

1. **B5 仍有两个选择**（parser 删除 create 或 create 在 batch 内 pending）。实施前必须选定一个并在实现/注释中固定；建议统一为 **create 在 batch 内 pending**，避免逐个 parse 函数删除调用造成遗漏。
2. **B6 create 失败重试**需明确触发条件（上下文就绪/下一次 flush），避免 pending 永久悬挂；控制销毁时清理 pending，不能访问悬垂指针。
3. **B7 的 equal-skip 默认策略**必须在代码中落实为“有明确 getter + 规范化函数才跳过”，不可仅凭属性字符串相等；否则字体别名、padding 字符串格式、四态颜色状态会漏更新。

## 3. 验收补充

- 选定 B5 方案后，逐个 parse 类型统计最终 create 次数；batch 内直接 create 不得提前执行。
- create 失败→上下文就绪重试→成功；失败后控件销毁无悬垂 pending。
- 回调最终值不变不触发、值变化触发一次、回调二次写入迭代上限生效。
- font 别名/路径归一化、padding Margin 比较、四态颜色 epsilon、font-style 无 getter 回退均有用例。
- `auto=2`（272/0）、`auto=3`、strict、`test_cr_theme`；同机 Release 多次中位数。
