# P049_TextShadowReadback_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-30
- 对象：`design/P049_TextShadowReadback_Design.md`（P0-49，状态：待评审）
- 结论：**通过，放行实施**（§4 两问全部确认）

## 1. 评审要点

### 探针矩阵（§1）✓

- **范围比报告更大**（hover/pressed/disabled 三态全缺，非仅两态）——探针矩阵（18 目标×16 单态键）系统化定位 ✓。
- **根因单点**：基类 getColorProperty 仅 normal 分支——caption-label 直控通道同缺口、**单点修复覆盖全部**（含 Button/Label/CheckBox 等全部控件）✓。
- **举一反三完成**：background/border/text 三组四态读回完整（仅 text-shadow 缺）——回应了清单的排查要求 ✓。
- **新发现**：WinFrame background.pressed/disabled 写读双路径不一致（单态键未转发+读回折叠错位）——真实缺陷一并修复 ✓。

### 修改方案（§2）✓

- **2.1 基类三分支**：最小单点修复，覆盖全控件 + caption-label 通道，与其它三组基类四态口径一致 ✓。
- **2.2 WinFrame 三步自洽**：①单态键 1:1 转发 ClientPanel（补齐）②读回取消折叠（实际四态）③对象路径折叠保留（两态缺省填充语义）——**单态键与对象路径 set→get 全部自洽**，逻辑闭环 ✓。
- **2.3 验收**：探针复扫 readFail 清零（含 WinFrame 两态语义 2 项）✓。

### 设计器侧影响

**零改动**——读回链修复后 text-shadow 四态槽回读自动正确；WinFrame 背景四态槽同受益。

## 2. §4 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | 测试落位 test_property_cabi（text-shadow 四态 + WinFrame 背景 per-state） | **确认** |
| 2 | WinFrame 对象路径折叠保留（两态缺省填充、往返自洽） | **确认** |

## 3. 放行

**结论：放行实施**。实施完成后同步 subModules，设计器联测：text-shadow 四态槽读回（Button/Label/caption-label 通道）、WinFrame 背景四态往返。
