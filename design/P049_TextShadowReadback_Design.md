# P049_TextShadowReadback_Design — text-shadow per-state 读回缺失（P0-49）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-09-30 补充：P0-49）
> 前置：P0-47/48 批已实施并同步
> 状态：**已放行，已实施**（2026-09-30）
> 复核：`requirements/P049_TextShadowReadback_Design_复核意见.md`（通过，放行实施；§4 两问全部确认）
> 实施备注：① 基类 `getColorProperty` 补 text-shadow hover/pressed/disabled 三分支（单点，全控件 + caption-label 通道）；② WinFrame per-state 单态键 1:1 转发 ClientPanel（补齐 pressed/disabled）+ `getBackgroundStateColor` 取消 hover 折叠；③ `test_property_cabi` 增补 text-shadow 四态（label/button）与 WinFrame 背景 per-state（单态键 + 对象路径）断言；④ 探针矩阵 readFail 清零（18 目标 × 16 键，含 WinFrame）；⑤ 回归仅 4 项预存失败

---

## 1. 问题确认（源码核实 + 全控件矩阵探针）

| 项 | 结论 | 根因 |
|---|---|---|
| P0-49 text-shadow per-state 读回（hover/pressed） | **确认（已复现，范围比报告更大：hover/pressed/disabled 三态均缺）** | 基类 `ControlImpl::getColorProperty`（ControlBase.cpp:1068 段）对 text-shadow **仅有 normal 分支**（`kTextShadow` → `shd.getNormal()`），缺 `text-shadow.hover/pressed/disabled` 三分支；写入侧（`setColorProperty` 四键 + `setTextShadow*StateColor` 存储/内部 label 转发）完整 → "写入视觉生效、读回不回" |
| 其它 StateColor 组排查（报告要求） | **background / border / text 三组四态读回完整**（基类均有四态分支；逐控件矩阵实测一致） | 仅 text-shadow 组缺失 |
| caption-label 直控通道 | 同缺口（17 项含 `btn-caption`/`chk-caption` 全部三态 READ 失败） | Label 无 getColorProperty 覆写 → 走基类同路径 |

**探针矩阵（18 目标 × 16 单态键 set→get 一致性；`Temp/temp_p049_probe.cpp`）**：
```
CTL label            setFail=0 readFail=3 | sh.h:READ sh.p:READ sh.d:READ
CTL button           setFail=0 readFail=3 | sh.h:READ sh.p:READ sh.d:READ
CTL check-box        ...                    （同上 16 控件）
CTL win-frame        setFail=0 readFail=2 | bg.p:READ bg.d:READ   ← 两态语义（见 §2 说明）
CTL btn-caption      setFail=0 readFail=3 | sh.h:READ sh.p:READ sh.d:READ
CTL chk-caption      setFail=0 readFail=3 | sh.h:READ sh.p:READ sh.d:READ
=== total readFail=53 ===
```
- 所有目标：`text-shadow.hover/pressed/disabled` SET=1、GET=0（唯一系统性缺口）；`text-shadow`（normal）与其余三组 × 四态全部一致；
- WinFrame `background.pressed/disabled` 读回≠写入：**确认为真实缺陷**（非两态语义镜像）——① 单态键 `background.pressed/disabled` 写入未转发 ClientPanel、落入基类存储，而读回经 `getBackgroundStateColor` 覆写取 ClientPanel 的 hover 镜像 → 写入/读回双路径不一致；② 对象路径 `setBackgroundStateColor` 将 pressed/disabled 折叠为 normal，读回却折叠为 hover → 对象路径往返也自相矛盾（§2.2 一并修复）。

## 2. 修改方案

### 2.1 基类 text-shadow per-state 读回（单点修复）
`ControlImpl::getColorProperty` 在 `kTextShadow` 分支后补齐：
```cpp
if (strcmp(prop, PropertyNames::kTextShadowHover) == 0)    { out = shd.getHover();    return 1; }
if (strcmp(prop, PropertyNames::kTextShadowPressed) == 0)  { out = shd.getPressed();  return 1; }
if (strcmp(prop, PropertyNames::kTextShadowDisabled) == 0) { out = shd.getDisabled(); return 1; }
```
- 覆盖全控件（Button/Label/CheckBox/ProgressBar/Slider/… 及 caption-label 直控通道）——与 background/border/text 三组的基类四态读回口径一致；
- 写入链不动（已完整）；WinFrame 标题走自身四态读回（:554-556），不受影响。

### 2.2 WinFrame 背景 per-state 写读一致性（确认为缺陷，一并修复）
- ① `WinFrame::setColorProperty` 增补 `kStatePressed`/`kStateDisabled` 分支 → `m_clientPanel->setPressedStateBGColor / setDisabledStateBGColor`（1:1 转发，与既有 `kBackground`/`kStateHover` 口径一致）；
- ② `WinFrame::getBackgroundStateColor` 取消 pressed/disabled→hover 折叠，返回 ClientPanel 实际四态（pressed→pressed、disabled→disabled）；
- ③ 对象路径 `setBackgroundStateColor` 保留 pressed/disabled→normal 折叠（两态声明对象未提供值的缺省填充语义）——经 ② 后对象路径 set→get 往返一致（normal 读写一致）；
- 效果：单态键与对象路径的 set→get 全部自洽（探针矩阵 WinFrame bg.p/bg.d 清零）。

### 2.3 测试与验收
- `test_property_cabi` 增补：① text-shadow 四态单键 set→get 一致性断言（label/button 两组，含 `text-shadow.hover/pressed/disabled`）；② WinFrame 背景 per-state 断言（`background.pressed/disabled` 单态键 1:1 往返 + 对象路径往返一致）；
- 探针复扫：矩阵 `readFail` = **0**（WinFrame 两态语义 2 项一并清零）；
- 全量回归（仅既有 4 项预存失败）。

## 3. 配套改动

- 代码：`src/ControlBase.cpp`（getColorProperty 三分支，单文件单点）；
- 测试：`test/test_property_cabi.cpp`（per-state 读回断言）；
- 文档：无需变更（单态键语义与手册一致，本修复为"实现对齐文档"）；设计文档标注 + 复核归档 + make_release + 同步。

## 4. 待审核问题

1. **测试落位**：per-state 读回断言加入 `test_property_cabi`（text-shadow 四态 label/button 两组 + WinFrame 背景 per-state 往返）——确认？
2. **WinFrame 对象路径折叠保留**：`setBackgroundStateColor` 对象路径的 pressed/disabled→normal 折叠（两态缺省填充）保留，经 ② 后往返自洽——确认？
