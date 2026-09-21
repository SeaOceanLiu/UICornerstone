# WinFrameShadow_CheckBoxCaptionFix_Design — WinFrame 文本阴影转发 + CheckBox caption 残留/文本持久化修复

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-09-21）——**#13**（WinFrame textShadow 色/偏移缺口）、**#15**（CheckBox releaseCaption 顺序缺陷）、**#16**（CheckBox caption 字符串分发——**上批已实施**，本文档确认）
> 状态：**已放行，已实施**（复核：`CornerstoneDesigner/Temp/WinFrameShadow_CheckBoxCaptionFix_Design_复核意见.md`，2026-09-21 通过 + 2 项补充覆盖）
> **实施结果（2026-09-21）**：
> 1. **#15 顺序修复**：`releaseCaption` 改为"保存 old → reset → removeControl(old)"；**实测**：修复前 `#15-1` FAIL（recreate 后 caption Label == 2 残留）→ 修复后 PASS（== 1）。
> 2. **caption 状态宿主持久化（统一快照方案）**：新增 `captureCaptionState()`（recreate 释放前快照：文本/字号/各态色/shadow+offset）→ `createCaption` 重建时应用；`setStringProperty` 同步 `m_captionText`；**覆盖复核 §3.1（CreateCheckBox 工厂路由）与 §3.2（直控各态色持久）**——工厂/parser/Builder/直控全路径 recreate 后不丢。
> 3. **路由修正**：CreateCheckBox 工厂（text → setStringProperty）、CheckBoxBuilder::setCaptionText、parseCheckBox（文本/字号 → 宿主字段）——不再绕过宿主。
> 4. **#13 WinFrame**：`setColorProperty` 补 textShadow 四态 + text 三态（hover/pressed/disabled）转发 title Label；`getColorProperty` 补对应读回；offset 上批回归 ✓。
> 5. **#16 确认闭环**（上批已实施）。
> 6. **测试**：test_colorfixes 46 项全 PASS（#15-1/2、§3.2 直控色持久、#13 四态/三态 roundtrip、offset 回归）；test_p0_getter C ABI 新增（工厂文本 recreate 后 "T1"、直控色 recreate 后 rgba=0,255,0）；11 项回归全绿。

---

## 1. 问题与目标

| # | 现象 | 目标 |
|---|---|---|
| #13 | WinFrame `textShadow.*` 色槽设置无效（视觉不变）；`shadowOffset.x/y` 无效 | title Label 的 textShadow 四态单色 + text 四态单色转发；offset 转发（上批已实施，核实） |
| #15 | **拖动 CheckBox**（setRect → recreate）后旧 caption Label **残留渲染树**（旧文字+旧样式恒显）+ 新空 caption 叠加（两层文字/阴影）；取消选中后消失 | `releaseCaption` 顺序修复（removeControl 用有效指针）；caption 文本/字号**宿主持久化**（recreate 后不丢/不重影） |
| #16 | 运行时 `SetString("caption", v)` 无效 | **上批已实施**（setStringProperty/getStringProperty + onPropertyChanged 布局刷新）——本文档确认，无需再改 |

## 2. 现状核实（源码事实）

| 机制 | 位置 | 状态 |
|---|---|---|
| **#15 顺序缺陷** | `CheckBox::releaseCaption`（CheckBox.cpp:42-48）：`m_caption.reset(); m_caption = nullptr; removeControl(m_caption);` → **`removeControl(nullptr)` 无效**（旧 Label 留在 children → 残留渲染） | **缺陷确认**（对照 `Button::setCaption` removeControl 先于 reset ✓） |
| recreate 触发 | `CheckBox::setRect`/attach 等 → `CheckBox::recreate()`（:136-153）：releaseCaption + create → createCaption 重建 | 拖动即触发（设计器现象吻合） |
| caption 文本存储 | `createCaption`（:55+）`.setCaption("")` —— **无宿主存储**；parser（LayoutParser.cpp:1840）/Builder（CheckBox.cpp:627-630）直写 label | recreate 后文本丢失（上批 diag 实测空文本） |
| caption 字号 | `createCaption` 用 `effectiveCaptionSize()`（宿主 `m_captionSize` ✓）；但 parser:1842/1846 直写 label font（不经 `setCaptionSize`） | recreate 后回落（同源） |
| **#13 色转发** | `WinFrame::setColorProperty`（WinFrame.cpp:435-441）仅 kWinFrameBG/kWinFrameBorder/kTitleBarBG/kTitleText；**无 kTextShadow/kTextShadowHover/Pressed/Disabled**（对照 `setTitleTextColor` 同步模式 ✓，:407-412） | **缺口确认** |
| #13 offset | `WinFrame::setFloatProperty` kShadowOffsetX/Y → titleLabel（**上批已实施**，:456+；get 同 :495） | 已实施（本批核实回归） |
| #13 读回 | `WinFrame::getColorProperty`（:471-477）无 textShadow 读回 | 需补 |
| WinFrame 标题重建自查 | WinFrame 无 releaseCaption/titleLabel 重建路径（titleLabel 构造期建、不重建） | ✓ 无同型缺陷 |
| ColorPicker 同型自查 | `recreateClosedState` remove 先于 reset ✓ | ✓ 无缺陷 |

## 3. 架构选择与关键设计决策

### 3.1 #15：releaseCaption 顺序修复 + caption 文本/字号宿主持久化

```cpp
// ① 顺序修复（无泄漏变体）
void CheckBox::releaseCaption(void) {
    if (m_caption != nullptr) {
        auto old = m_caption;          // 先持有效指针
        m_caption = nullptr;
        removeControl(old);            // 有效移除（此前传 nullptr 无效 → 残留渲染树）
    }
}

// ② 文本宿主持久化（createCaption 应用）
//   setStringProperty(kCaption) → m_captionText = value + label->setCaption
//   createCaption：.setCaption(m_captionText)（替代 ""）
//   getStringProperty：读 label 并回填 m_captionText（稳定指针）
```

**路由修正（宿主字段不被绕过）**：
- `CheckBoxBuilder::setCaptionText`（:627-630）→ `m_checkBox->setStringProperty(kCaption, caption.c_str())`（宿主字段更新）；
- `parseCheckBox`（LayoutParser.cpp:1840）→ `checkBox->setStringProperty(kCaption, ...)`；
- `parseCheckBox` 字号（:1842/:1846）→ `checkBox->setCaptionSize(...)`（宿主 `m_captionSize`；recreate 后 `effectiveCaptionSize()` 正确）。

决策点：
- **宿主字段为 caption 文本/字号的持久化来源**（与上批 shadow 宿主字段同模式）——recreate 重建 label 后由 createCaption 统一应用（文本/字号/shadow/状态）；
- 顺序修复后旧 Label 彻底移除，**两层文字/阴影消失**；
- 不改变 caption 颜色来源（`m_textColor` 宿主基类字段 ✓ 已持久）。

### 3.2 #13：WinFrame 文本/阴影四态单色转发（决策：对称补齐 set/get）

```cpp
// WinFrame::setColorProperty 追加（对照 setTitleTextColor 同步模式）
if (kTextShadow)          → m_titleLabel->setTextShadowNormalStateColor(color)
if (kTextShadowHover)     → m_titleLabel->setTextShadowHoverStateColor(color)
if (kTextShadowPressed)   → ...Pressed...
if (kTextShadowDisabled)  → ...Disabled...
if (kTextHover/kTextPressed/kTextDisabled) → m_titleLabel->setText*StateColor(color)   // 同型枚举完备
// WinFrame::getColorProperty 追加读回（titleLabel->getTextShadowStateColor()/getTextStateColor() 对应态）
```

决策点：
- **四态枚举完备**（设计器"同型"要求）：textShadow 四态 + text 四态（hover/pressed/disabled；normal 由 kTitleText 承载）；
- offset 转发上批已实施——本批仅核实（`SetFloat("shadow-offset-x", 5)` → `getShadowOffset().x == 5`）；
- 状态可达性沿用 P0-7 说明（WinFrame 交互不进入 hover/pressed；色值转发本身完整、程序化 setState 可达）。

### 3.3 #16 确认（已实施，无代码变更）

上批已实现 `CheckBox::setStringProperty/getStringProperty(kCaption)`（含 `m_captionText` 稳定存储与布局自动刷新）；本批设计器清单滞后，**确认闭环**。

## 4. API 设计

- 核心库：CheckBox.cpp（releaseCaption 顺序、createCaption 文本应用、setStringProperty 更新宿主字段）；CheckBoxBuilder（setCaptionText 路由）；LayoutParser.cpp（parseCheckBox 文本/字号路由）；WinFrame.cpp（setColorProperty/getColorProperty 四态转发）。
- C ABI / Binding / schema / PropertyNames：**零变更**。

## 5. 实现要点与验收

| 验收 | 方法 |
|---|---|
| #15-1 无残留 | 内部测试：CheckBox 建 → `setRect`（触发 recreate）→ 断言 `children` 中 caption Label 数量 == 1（旧 label 已移除）；修复前 == 2 |
| #15-2 文本持久 | 建 → `setStringProperty("caption","T1")` → `setRect` → 断言 caption 文本仍 "T1"（宿主重建）；字号同理（`setCaptionSize`） |
| #15-3 JSON 场景 | LayoutParser 解析 check-box（caption+font-size）→ 模拟 recreate（setRect）→ 文本/字号保持 |
| #13-1 shadow 色 | `SetColor("text-shadow", red)` → `getColorProperty("text-shadow")` 往返 red；titleLabel 状态色字段断言（四态遍历） |
| #13-2 offset 核实 | `SetFloat("shadow-offset-x", 5)` → titleLabel `getShadowOffset().x == 5`（上批回归） |
| #16-1 确认 | 既有 test_colorfixes `#9 CheckBox caption string roundtrip` PASS（上批已覆盖） |
| 回归 | test_checkbox / test_winframe / test_colorfixes / test_layout / test_button 全绿 |

## 6. 影响面与风险

- releaseCaption 修复：旧 Label 正确移除（此前泄漏在渲染树）——纯修复；文本宿主化后 recreate 场景文本不丢（此前丢/重影）。
- parser/builder 路由改动：caption 文本/字号经属性/API（宿主更新）——行为等价 + recreate 稳定。
- WinFrame 四态转发：纯补齐（此前静默落基类字段）；不影响既有 kTitleText/offset 路径。
- 三后端无涉。

## 7. 待审核问题

1. **#15 范围**：releaseCaption 顺序 + 文本/字号宿主持久化 + parser/builder 路由——确认？（字号路由为连带修复，防"拖动后字号回落"）
2. **#13 枚举范围**：textShadow 四态 + text 三态（hover/pressed/disabled）转发 + get 读回——确认？
3. **#16 确认**：上批已实施（setString/getString + 布局刷新）——确认闭环？
4. 验证载体：test_colorfixes 扩展（#15-1/2/3 + #13-1/2）——确认？

## 8. 待提交配套改动（实施时随批）

- 测试扩展（§5）+ 全量回归；设计文档状态标注；复核意见归档。
- 手册：winframe.html 补 textShadow 四态/text 三态转发说明（若需）；make_release 同步（设计器联测：CheckBox 拖动无重影、WinFrame 色槽全链）。