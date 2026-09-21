# HandleControl_CABI_Integration_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-18
- 对象：`design/HandleControl_CABI_Integration_Design.md`（状态：待评审）
- 结论：**通过，放行实施**（5 个待审核问题全部答复见 §2），附 4 项补充（不阻塞，实施时注意）

## 1. 分项评审

### P0-1 SetHandleTarget ✓

- 复用既有 `setTarget(Control*)` + `detach()`，核心层零新增 ✓；NULL 语义显式分支（setTarget 无 NULL 防护，C ABI 拦截）正确。
- **幂等快路径建议加**（`if (m_target == target) return;`）——设计器重复选中同一控件是高频路径，同意。
- 生命周期沿用 no-op deleter 模型 ✓（不引入新所有权语义）。

### P0-2 HandleHitTest ✓

- **`m_handleAreas` 仅在 draw() 重建**的坑识别关键——按下沿可能发生在首帧 draw 前，查询前强制 `updateHandleAreas` 是必要设计 ✓。
- HandleType 转 public + `hitTestHandle`/`updateHandleAreas` 公开：最小暴露 ✓。
- 性能 O(9) 每次刷新：按下沿单次调用，无碍 ✓。

### P0-3 RectFilter ✓ —— 本设计最有价值的部分

- **局部坐标（= 逻辑 rect）域选择正确**：设计器模型存逻辑 rect，吸附算法在逻辑域闭环，零转换。
- setRect 前插槽、返回 0/1 严格二值语义 ✓。
- anchor 方向（NW/W/SW）的 left/width 联动说明清晰——四值独立、以 filter 输出为准，可预期 ✓。

### P0-4 收敛为 SetHandleMoveVisible ✓

- 同意收敛（"能简单就别复杂"）：filter 落地后设计器倾向保留 Move 手柄经 filter 吸附，其余配置（size/colors/minSize）缺省可用，列 backlog。

## 2. 对 5 个待审核问题的答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | filter 坐标系 = target 局部坐标 | **确认**。设计器吸附算法在逻辑域，零转换；无需屏幕域预留 |
| 2 | 先 minSize 钳位后 filter（filter 可越过 minSize） | **确认接受**。吸附 snap 值 ≥ grid ≥ 8 与缺省 minSize 无实际冲突；"filter 最终"语义简单 |
| 3 | P0-4 收敛为 SetHandleMoveVisible 单函数 | **接受** |
| 4 | HandleType 值映射 None=0、Move=1、NW=2…W=9 | **确认**，作为稳定契约写入文档 |
| 5 | SetControlId binding 已完成；schema 裁剪/字体预热为存续 backlog | **确认**。已验证 subModules 当前 binding 含 `SetControlId` 包装（DynamicApi RESOLVE:80 + UICornerstone.h:119）——此前我方验证时 DLL 为旧版，陈述属实 |

## 3. 补充（不阻塞，实施时注意）

### 3.1 坐标域需精确定义（P0-2）

文档 §3.2 写"x, y 为窗口屏幕坐标（与 handleEvent mousePos 同域）"——建议进一步写明 mousePos 的具体参照系：**子视口局部物理坐标**（bench drawRect 域）还是窗口全局。设计器侧调用时将从 SDL 窗口坐标换算（`wx - viewportX`），两域差一个视口偏移——请在 API 注释中给出确定义并附一行换算说明。

### 3.2 filter 回调类型名统一（§4.2 vs §4.3）

C ABI 段定义 `UIHandleRectFilter`、Binding 段写 `UIRectFilterFn`——两处类型名不一致，实施时统一为 **`UIHandleRectFilter`**（并在 UICornerstoneAPI.h 公开，binding 复用）。

### 3.3 设计器 filter 实现的边角责任（NW/W/SW 拖拽）

四值独立语义下，设计器 filter 对 NW/W/SW 拖拽 snap 修改 `x` 时需**自行保持右缘**（同步调整 width 使 `x + width` 不变），否则视觉右缘漂移。这是应用侧实现细节，建议在 C ABI 注释中提示（"修改 left 时注意与 width 的联动"）。

### 3.4 HandleHitTest 与 HandleControl 可见性

查询路径 `updateHandleAreas` 强制刷新不依赖 m_visible——设计器 detach（Destroy）后不再查询即可，无需引擎处理可见性守卫。仅提示：设计器将保证只对有效（选中态）HandleControl 发起查询。

## 4. 设计器侧配合改动（实施后随批）

1. 选中切换改 `SetHandleTarget`（删除 Destroy/重建路径）；
2. 按下沿 `HandleHitTest` 精确让区（删除外扩 10px 近似；恢复本体边缘点击的移动能力）；
3. 吸附逻辑移入 filter（删除"拖拽中只同步 + 松手收敛"的轮询快写回；注意 3.3 的 NW/W/SW 右缘联动）；filter 用逻辑域 snap，模型同步仍由现有轮询承担；
4. `SetHandleMoveVisible` 按策略接线（filter 落地后倾向保留 Move 手柄）。

## 5. 放行

**结论：放行实施**（按 §4 API 设计 + 5 项确认 + 4 项补充注意）。实施完成后同步 subModules，设计器按 §4 迁移并联测：选中切换零重建、命中像素级一致、拖拽过程实时吸附（含 NW/W/SW 右缘保持）、Move 手柄开关。
