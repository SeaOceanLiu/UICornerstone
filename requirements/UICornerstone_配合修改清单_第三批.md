# UICornerstone 配合修改清单（第三批：滚轮语义 + 文档勘误）

- 提出方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-19
- 前置：Wheel_Support_Design（Panel mouse-wheel + ScrollBar 滚轮）已实施并同步，设计器已迁移验证。本批为集成验证中发现的**行控件 wheel 语义缺陷**与**用户手册/文档勘误汇总**。

## P0 —— 行控件滚轮消费语义缺陷（阻塞容器滚轮体验）

### 背景

`Wheel_Support_Design` 落地后，Panel 容器级 `mouse-wheel` 回调（子控件未消费时 fire）工作正常。但实测放置 ComboBox（动态行 = 5×NumericUpDown + 2×TextArea + CheckBox/label）后，**滚轮滚动面板完全失效**——滚轮命中行控件时被消费，命中概率约 80%。

### 1. TextArea：无可滚内容时也应消费 wheel

- **现象**：滚轮落在 TextArea 行上，文本内容**不足一屏（无可滚空间）**，滚轮被消费但无任何视觉效果，容器滚动也不触发。
- **根因**：`TextArea::handleEvent` 的 MouseWheel 分支（TextArea.cpp:775-780）仅做 `isContainsPoint` 判定即消费，**不检查可滚性**。
- **建议**：`if (getMaxScrollY() <= 0) return false;`（内容未超视口时透传给容器）；或仅在内置 vScrollBar 有滚动余量时消费。

### 2. NumericUpDown：未聚焦时不应消费 wheel

- **现象**：滚轮落在 NumericUpDown 行上，**未聚焦**状态也消费 wheel 并**改变控件数值**——用户意图是滚动面板，结果数值被意外修改（语义错乱 + 面板不滚）。
- **根因**：`NumericUpDown::handleEvent` 消费 wheel 不检查焦点态。
- **建议**：wheel 消费加 `getFocused()` 前置条件（聚焦态滚轮改值为标准交互，保留；未聚焦透传给容器）。CheckBox 的既有语义（仅 click/check-changed，无 wheel）可作参照。

### 3. ⚠️ Panel mouse-wheel 回调对嵌套面板不可达（实测，需引擎调查）

- **现象**：Wheel_Support 落地后实测——放置 ComboBox（动态行 9：5×NumericUpDown + 2×TextArea + CheckBox/label），滚轮在属性面板**任意位置**（含行间空隙、label 行——不在任何消费型子控件上）**全程无一次**触发 `mouse-wheel` 回调（设计器回调日志零输出）。非命中率问题。
- **引擎设计**（Wheel_Support §3.1）：`Panel::handleEvent` = `ControlImpl::handleEvent`（子控件递归，未消费返回 false）→ wheel 命中面板 → fire。**按此设计行间空隙必然 fire**——实测未发生。
- **疑点**：事件路由链（bench → root → middleArea → propertyPanel → **propertyTabs(tab-control)** → tabProperty → propDynamic，7 层嵌套）中途某层对 `MouseWheel` 拦截或未转发；或主实例事件循环（`TopControl::eventLoopEntry` ← `Bench::inputControl → triggerEvent → pushEventIntoQueue`）对 wheel 的分发验证。
- **引擎侧定位建议**：
  1. 对照测试：**单层 Panel**（bench 直接子）vs **深层嵌套 Panel**（tab-control/page 链）各挂 mouse-wheel 回调——若单层触发、嵌套不触发 → 定位中间层拦截；
  2. 检查 `TabControl::handleEvent` 对 MouseWheel 的处理（页切换逻辑是否吞 wheel）；
  3. 确认 `eventLoopEntry` 对主实例（非子视口）wheel 的完整分发路径。
- **影响**：设计器动态区滚轮（Win32 钩子绕行无法回收）；任何深层嵌套容器的滚轮需求。

### 3.x 消费语义通则（建议写入控件开发规范）

- **可滚容器类**（TextArea/ListView/TreeView）：内容超视口才消费 wheel；无滚动空间透传。
- **数值输入类**（NumericUpDown/Slider）：聚焦/hover+聚焦才消费 wheel；未聚焦透传。
- 其余控件不消费 wheel（透传给容器 Panel/ScrollBar）。

## P1 —— 用户手册 / 文档勘误汇总

| # | 项 | 说明 |
|---|---|---|
| 4 | **HandleHitTest 坐标域勘误** | `UICornerstone_HandleHitTest` 注释"子视口场景 = SDL 窗口坐标 - 视口偏移"**有误**。实测 `m_handleAreas` 基于 `targetToScreen() = getDrawRect()`（父链递归累加，视口根 rect 为窗口全局位置）→ **应传 SDL 窗口坐标原样**。按注释减偏移恒 miss（设计器实测 ret=0）。请修正注释，并建议在引擎文档全局澄清 `getDrawRect`/`mousePos`/`mouseWheel.x,y` 的参照系约定（递归累加至窗口全局域）。 |
| 5 | **mouse-wheel 事件文档** | Wheel_Support_Design §8 已列（Panel/ScrollBar 手册 + API_Mapping_Table + 事件速查），含载荷契约：`floatVal = scrollY`（+1 向上 / -1 向下，SDL3 原生透传）；坐标域由引擎分发保证。实施时请随批落地。 |
| 6 | **GetRect vs GetDrawRect 域说明** | `UICornerstone_GetRect` 返回**直接父局部坐标**（m_rect 原值），`getDrawRect` 为递归累加的窗口全局域——两者语义差异建议在 C ABI 文档/手册明确标注（集成方两度踩坑：HandleHitTest 全局域、容器位置判定局部域）。 |

## P0 追加（2026-09-20，颜色组补全发现）—— ColorPicker 构建成本过高

### 4. ColorPicker::create() 弹窗子树全量预构建（~3.5s/实例）

- **现象**：设计器属性面板需 15 个颜色槽（colors.background/border/text/textShadow × 4 态）。ColorPicker 直建实测 **~3.5s/个**（15 个 50+ 秒，选中控件即长时间卡顿，不可接受）。
- **根因**：`ColorPicker::create()`（ColorPicker.cpp:82-124）一次性构建**完整弹窗子树**：Dialog + 确定/取消按钮 + 预设网格（COLORPICKER_PRESET_COLS×ROWS ≈ 80 格）+ HexInput + RGBA 4 滑块 + 预览——数十子控件（表单控件创建+字体加载均摊 ~100ms+）。
- **设计器临时方案**（已实施）：行先放占位 Button（背景=当前色+hex 文本），点击才创建 ColorPicker 惰性替换——但**首次点击某槽仍卡 3.5s**，且面板色槽多时体验仍差。
- **建议**（任选，倾向 a）：
  - **a. 弹窗子树惰性化**：`create()` 只建关闭态（色块/箭头），**首次 `openPopup()` 时才构建弹窗内容**（Dialog/预设/滑块/hex）——单实例构建成本从"选中即付"变为"首次打开付"，且可进一步懒到"首次打开"后缓存；
  - **b. 预设网格惰性**：`recreatePopupContent` 的 `createPresetGrid`（80 格）延后到弹窗首次可见时；
  - **c. 字体/控件创建共享缓存**（长期）：与"字体预热"项合并——控件创建均摊成本是全局问题（NUD/TextArea 首建 400ms 同源）。
- **收益**：设计器可回退占位方案直接用 ColorPicker；所有含 ColorPicker 的应用受益。
- **设计器终态与回收清单**（引擎惰性化落地后执行）：
  1. 删除手工复刻的关闭态占位三件套（Panel swatch + Label hex + 透明 Button 点击层）与点击替换逻辑——形态复刻依赖引擎内部参数（swatch 16/gap 4/hex 格式），引擎改版即失配；
  2. 行内**直接 `CreateColorPicker`**（关闭态秒建、形态天然正确、点击弹窗自带）；
  3. 若同时提供 openPopup 公开 API，可进一步评估"共享单实例"变体（当前不需要，直建已够）。

## P0 追加（2026-09-20，颜色组联测发现）—— 内部 caption Label 的暴露方式（方案改定）

### 5. Button 内部 caption Label 直控接口（原“单色 setter 补同步”方案的替代）

- **现象**：`SetColor("text-shadow", c)` / `SetColor("text", c)` 写 Button 后**视觉无变化**（阴影色/文字色不变）。
- **根因**：Button 无 `setColorProperty` override → 基类单色 setter 只改 Button 字段，不同步内部 caption Label（4 态版 `setTextShadowStateColor` 有同步、单色版缺失）。
- **方案改定（应用直控内部 Label，替代“补同步”）**：
  - Button 已有 C++ `getCaptionLabel()`（Button.h:55）——**补 `getPtrProperty("caption-label")` 分发**（或专用 C ABI `UICornerstone_GetCaptionLabel`）+ binding 包装；
  - 应用经句柄**直控 Label 标准属性**（SetColor/SetStateColor/字体/对齐——Label 自绘用自身字段，无转发问题）。
- **优点**：引擎零行为变化（只加查询接口）；应用获得 caption 全族属性控制（不限颜色）；schema 已公开 `caption-label` 键（抽象契约成立）。
- **同型排查**：CheckBox/ComboBox/NUD 等含内部文本子控件的控件的对应暴露（按需）。
- **设计器侧已绕行**：text/textShadow 色槽写回走 `GetStateColor`/`SetStateColor` 读改写（4 态版路径有同步）；b 落地后改直控 caption。

## P0 追加（2026-09-20，截图实锤）—— LoadLayout ColorPicker 关闭态出现双 hex Label 重叠

### 6. 外观区 color-picker（LoadLayout 创建）的关闭态 hex 文本双写重叠

- **现象**（截图 `CornerstoneDesigner/Temp/RGB值重叠.png`）：属性面板外观区"颜色"行的 ColorPicker 关闭态，**两个 hex 文本叠加**：蓝色 `#4A90D9FF`（color-picker 的 JSON 初始色，恒定）与 浅色 hex（**当前选中控件背景色**，随选中变化）。左侧 16×16 色块显示正常（当前背景色）。**取消选中（动态区清空）后重叠消失**。
- **排除**：外观区 JSON 仅一个 color-picker（prop_color）；设计器未在外观区创建任何控件；动态区控件位置与外观区不重叠（区域隔离已验证）。
- **时间相关**：惯性化（弹窗惰性化）批之前未观察到——**疑似与该批改动相关**（create() 拆分后关闭态构建路径变化），请引擎排查：
  1. `createClosedStateControls` 是否可能被调用两次（create 拆分后某路径重复进入）；
  2. `LayoutParser` 对 color-picker 的解析是否额外创建 hex 显示控件；
  3. `m_closedLabel` 的 shared_ptr 生命周期（removeControl 后残留渲染）。
- **影响**：外观区颜色控件文本不可读（设计器主路径）；任何 LoadLayout 创建的 ColorPicker 均可能复现。

## P0 追加（2026-09-21，caption 直控联测）—— Button 状态不同步内部 caption

### 7. Button hover/pressed/disabled 状态不联动内部 caption Label

- **现象**：经 `caption-label` 直控改 caption 的 `text.hover`/`text.pressed`/`text.disabled`/`text-shadow.*` 各态色——**字段写入成功（回读正确）但视觉永不显示**。
- **根因**：`ControlImpl::setState`（ControlBase.cpp:676-678）与 `setEnable`（:506-509）**只改自身 m_state**；Button hover（Button.cpp:179/211）/pressed（:159/167）/disabled 时 **m_caption 的 state 不随之改变** → caption 恒 Normal 态 → Label::draw 用 `m_textColor.getNormal()` → 非 normal 态字段永不参与绘制。
- **建议**：`Button::setState`/`setEnable` override（或基类状态变更回调）联动 `m_caption->setState(state)`；同型排查 CheckBox 等含内部文本子控件的控件。
- **设计器侧**：无绕行（字段可写、态不可达）——normal 态已可用（`20202b5` 后 text.normal/textShadow.normal ✓）。

## P0 追加（2026-09-21，各态联测发现）—— CheckBox/WinFrame 补 caption-label GetPtr 分发

### 8. `getPtrProperty("caption-label")` 同型补齐（ButtonCaptionState_Design 的配套缺口）

- **现象**：Button 的 `text.*`/`textShadow.*` 各态色经 caption 直控生效（getPtr 可达）；**CheckBox 与 WinFrame 的对应色槽不生效**——`GetPtr("caption-label")` 失败 → 应用兜底写自身字段（对内部文本无效果）。
- **根因**：P0-5 实施时仅覆盖 `Button::getPtrProperty`（CheckBox 当时答复"按需"、WinFrame 为后续扩展纳入）——**句柄暴露缺口**。
- **建议**（统一键，应用无感知类型差异）：
  - `CheckBox::getPtrProperty("caption-label")` → `m_caption`；
  - `WinFrame::getPtrProperty("caption-label")` → `m_titleLabel`。
- **配套**：ButtonCaptionState_Design 的状态联动已覆盖三类 ✓（引擎已实施）——仅差句柄暴露即可全链生效。
- 设计器侧：回调统一经 `GetPtr("caption-label")`（无需改动代码）。

## P1 追加（2026-09-21，文本联测）—— CheckBox/TabControl 等枚举-字符串键补充

### 9. CheckBox 缺 `setStringProperty/getStringProperty("caption")` 分发

- **现象**：运行时设置 CheckBox 文本（属性面板 text 行 / `SetString("caption", v)`）无效——CheckBox 无 caption 属性分发（仅工厂构造时 `getCaption()->setCaption(text)` 可设）。
- **设计器已绕行**：经 `GetPtr("caption-label")` 直控内部 Label（caption 读写完整）——引擎修复后可按需简化。
- **建议**：CheckBox 补 `setStringProperty("caption")`（→ m_caption->setCaption）/`getStringProperty("caption")`（→ getCaption()->getCaption），与 Button 对齐；同型排查其余含内部 caption 而文本不可运行时设置的控件。
- **备注**：WinFrame 的 hover/pressed 无状态机（`setState` 无调用点，窗口语义）——title 的非 normal 态色**不可达属设计决定**（disabled 经 setEnable 可达）；设计器 win-frame 的 text.hover/pressed 槽位属"无态语义"（可接受，面板不特殊标注）。

### 10. CheckBox 缺 `text-shadow-enable`（阴影不可开启）

- **现象**：CheckBox 有 `textShadow.*` 色槽（schema common）但**无阴影开关**——CheckBox.cpp 无 `kTextShadowEnable` 分发、schema check-box def 无该键（caption Label 的 shadowEnabled 恒 false）→ 色槽可设置但阴影永不显示。
- **建议**：CheckBox 补 `setBoolProperty(kTextShadowEnable)`（→ m_caption->setShadow + m_enableTextShadow 字段，与 Button 对齐）+ schema check-box def 补 `text-shadow-enable`；同型排查（含内部 caption 且 common 有 text-shadow 的控件）。
- 设计器侧：待引擎补齐后色槽即全链可用（当前属"有槽无开关"，已可接受）。

## P0 升级（2026-09-21，截图复现）—— clip-children 滚动溢出

### 11. clip-children 未裁剪滚动出容器的子控件（复现——ClipChildren_Nested_Design）

- **现象**（截图 `CornerstoneDesigner/Temp/Panel clip问题.png`）：动态属性区（propDynamic，clip-children=true）滚动后，上方滚出容器的行内容（截图示例：font-size 行的 NUD 值 "14"）**未裁剪**，叠绘在容器上方的固定区（"行为"标题处）。
- **机制**：滚动使行局部 y 为负（跑出容器顶部）→ 行控件（NUD/TextArea 等）的绘制超出容器 rect；`Panel::draw` 的 `pushClipRect(getDrawRect())` **对该穿透未生效**。
- **疑点方向**：嵌套 clip 语义——行内子控件（EditBox/NUD/TextArea）自身也 pushClipRect（自内容裁剪，其 rect 亦在容器外）——clipStack 的相交/覆盖语义（`applyClipRect`）在该嵌套路径下可能失效；或绘制原语（文本/自绘）未全部经由 clip。
- **设计器现状**：`applyDynScroll` 超界隐藏容差 -40（保留半可见行）——收紧到 vy≥0 可止溢出但造成"行整跳"（滚动 3px 首行即消失），体验更差，故**保留容差、等引擎修复**。
- **升级理由**：滚动容器基础体验；修复后设计器可删除"超界隐藏"兜底（纯偏移滚动）。

## P0 追加（2026-09-21）—— 文字阴影配置引擎统一（设计器绕行的根除方案）

### 12. shadow 配置的 schema/运行时统一（Button/CheckBox/WinFrame 对齐 Label——ShadowCaption_Unify_Design）

- **背景**：设计器已以**类型分派**在面板层拼出统一的三行（shadow.enabled/offset.x/offset.y）：label 走自身 `shadow`/`shadow-offset-*`；button 走 `text-shadow-enable`+caption 直控 offset；win-frame 走 title 直控；check-box 待 P1-10。**属应用层绕行**（每类型硬编码路径；button 另有 `text-shadow-enable` 与 `shadow.enabled` 两个语义重复的开关）。
- **建议（引擎统一，设计器随后删除绕行段）**：
  1. **schema**：`button`/`check-box`/`win-frame` def 补 `shadow` 对象键（结构与 `label` 的 shadow 一致：`{enabled, offset:{x,y}}`）；
  2. **运行时**：三类补 `setBoolProperty(kShadow)`（转发内部 Label：Button/CheckBox→caption、WinFrame→titleLabel）与 `setFloatProperty(kShadowOffsetX/Y)`（同转发）；
  3. **删除 `text-shadow-enable` 别名**（使用方核查：仅引擎 `test_p0_getter.c`（上批新建）与 `layouts/all_controls.json`（演示布局）——**无外部/设计器布局依赖**（设计器 main_layout.json 零使用）——同批迁移为 `shadow.enabled`；文档同步）。
- **受益**：影子配置按 schema 声明式驱动；应用/后续工具零类型知识；消除语义重复开关。

## P0 追加（2026-09-21，WinFrame 阴影联测）—— WinFrame 文本阴影色/偏移缺口

### 13. WinFrame 缺 textShadow 色与 shadowOffset 转发（title Label 不同步）

- **现象**：WinFrame 的 `textShadow.*` 色槽与 `shadowOffset.x/y` NUD **设置无效**（视觉不变）；`text.normal` 与 `shadow.enabled` 正常。
- **根因**（源码核实）：
  1. `WinFrame::setColorProperty`（WinFrame.cpp:435-441）仅分发 `kWinFrameBG/kWinFrameBorder/kTitleBarBG/kTitleText`——**无 `kTextShadow`** → 基类字段（WinFrame 自身）被改，但 title Label 的 textShadow 不同步；
  2. `WinFrame::setFloatProperty` **无 `kShadowOffsetX/Y`** 分发 → title Label 的 shadowOffset 不可达。
- **对照**（既有同步模式）：`setTitleTextColor`（:407-412）同步 `m_titleLabel->setTextNormalStateColor` ✓——**单色 textShadow 版缺失**。
- **建议**：
  1. `WinFrame::setColorProperty` 补 `kTextShadow` → `m_titleLabel->setTextShadowNormalStateColor`（对照 setTitleTextColor 模式）；
  2. `WinFrame::setFloatProperty` 补 `kShadowOffsetX/Y` → `m_titleLabel->setShadowOffset`。
- **验收**：`SetColor("text-shadow", red)` → title Label 阴影色变红（GetColor 往返 + 视觉）；`SetFloat("shadow-offset-x", 5)` → getShadowOffset().x == 5。
- **同型**：kTextShadowHover/Pressed/Disabled 与 kShadowOffsetY 同批（枚举完备）。
- **设计器侧**：无绕行（纯转发缺失）——text.normal 与 shadow.enabled 可用，其余槽位待本项。

## P0 追加（2026-09-21，截图+日志实锤）—— CheckBox::releaseCaption 顺序缺陷致旧 caption 残留

### 15. CheckBox::releaseCaption 的 removeControl 晚于 reset（传 nullptr 无效）——旧 caption Label 残留渲染树

- **现象**：**拖动 CheckBox**（SetRect → CheckBox::setRect → recreate()）后，**内部 caption Label 残留渲染树**（旧文字+旧样式恒显）+ createCaption 空文本新 caption 叠加——视觉“两层文字/两层阴影”；**取消选中后残留消失**（行销毁级联）。
- **根因**（CheckBox.cpp:43-46）：
  ```cpp
  if (m_caption != nullptr) {
      m_caption.reset();          // 先置空（shared_ptr 释放）
      m_caption = nullptr;
      removeControl(m_caption);   // remove(nullptr)——无效调用
  }
  ```
- **对照**：`Button::setCaption`（Button.cpp:295+）顺序正确（removeControl 先于 reset）——CheckBox 孤例。
- **建议**：removeControl 移到 reset 前（用有效指针）；或先保存 old 再 remove+reset（无泄漏变体）：
  ```cpp
  auto old = m_caption;
  m_caption = nullptr;
  removeControl(old);
  ```
- **同型**：`recreateClosedState`（ColorPicker）顺序正确 ✓；WinFrame title 重建路径请引擎自查。
- **连带**：残留旧 caption 不响应任何新设置（应用直控的是新 m_caption）——与设计器“shadow/text 设置后无效”观察吻合。
- **设计器侧**：无绕行（旧句柄不可达）——等引擎修复。**关联**：P0-5/P0-7（caption-label 暴露与状态联动）在该缺陷修复前对 CheckBox 拖动场景部分失效。
### 16. CheckBox 缺 setStringProperty/getStringProperty("caption") 分发

- **现象**：运行时 `SetString("caption", v)` 写 CheckBox 无效——无分发（仅工厂/JSON 直控内部 Label）。
- **建议**：CheckBox 补 `setStringProperty("caption")`（→ m_caption->setCaption）/`getStringProperty("caption")`（→ getCaption()->getCaption）。
- **备注**：与 15（releaseCaption 顺序缺陷）同源修复后，设计器固定区“文本”行对 check-box 即全链生效。


## 待观察（维持）

| 项 | 状态 |
|---|---|
| （clip 穿透已升级为 P0-11，见上） | — |

## 已闭环（备忘）

- Panel mouse-wheel 容器回调 + ScrollBar 滚轮：已实施已验证（设计器 Win32 钩子绕行已回收）。
- SetControlId C ABI + binding、HandleControl C ABI 四件套、font-size 统一 + caption-size 废弃、TextShadow 缺省色、schema kebab 统一：均已实施已验证。

## 优先级建议

1. **P0-3（Panel wheel 嵌套不可达调查）**：机制性缺口，本轮升级为首位。
2. **P0-1/P0-2（TextArea/NUD wheel 语义）**：直接阻塞容器滚轮体验，建议同批实施。
2. **P1-4（HandleHitTest 注释勘误）**：一行注释修正，随批顺手。
3. **P1-5/6（手册）**：随 Wheel_Support 实施批落地（§8 已列）。
4. clip 穿透：待观察。

## 追加（2026-09-22 逐控件属性检查：资源类属性）

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-17 | 高 | **Actor::setStringProperty 的 image 分发拼 basePath** | Actor.cpp:304 直传 `fs::path(value)`——相对路径（如 `textures/foo.png`）加载失败；对照 Button.cpp:441 animation 分发的 `Platform::GetBasePath()` 拼接模式（provider: 前缀不拼） |
| P0-18 | 中 | **providerName/resourceId 运行时 setter** | schema image def 暴露但引擎无 setStringProperty 分发（仅解析期键）——设计器已过滤无效槽；若补分发则设计器恢复生成 |
| P0-19 | 中 | **schema button def 补运行时资源键** | normal-image/hover-image/pressed-image/disabled-image/animation 五键（引擎 setter 已支持、schema 只有 actors/luotiAni object 解析期形式）——设计器已硬编码补行（CanvasPane.cpp Button 资源段），schema 补齐后设计器删硬编码段 |

## 追加（2026-09-22 Button 运行时状态图不显示——探针实锤）

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-20 | 高 | **运行时设置四态状态图不显示** | 双重缺陷：①`make_shared<Actor>` 直接构造（Button.cpp:459 四态图分发）m_visible 默认 false（工厂/ActorBuilder 路径有显式 setVisible，此路径无）；②ensureActor 只在 Button::create 跑一次（Button.cpp:33-36），运行时 setNormalStateActor 的新 Actor 无人 create。探针：test_button.cpp `[probe A] created=0 visible=0`。**修复建议**：setNormalState/Hover/Pressed/DisabledActor 四 setter 统一补 `actor->setVisible(true)` + `if (isCreated() && !actor->isCreated()) { actor->setParent(this); actor->create(); }`；修复后 probe A 应 created=1 visible=1（可转 CHECK 断言）。animation（luotiAni）分发同型待验证 |
| P0-21 | 中 | **四态图/animation 运行时读回** | setStringProperty 已支持写入，getStringProperty 无对应读回——设计器已 extended 绕行（输入值保持显示），引擎补读回后设计器可去掉兜底 |

## 追加（2026-09-22 资源行联测反馈）

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-22 | 低 | **Actor::loadFromFile 空路径防御** | `fs::path("").is_relative()==true` → 拼 basePath 打开 `Debug\` 失败刷日志（设计器已过滤空串写回，引擎侧建议早退防御）；可选：`SetString("image","")` 卸载语义（设计器"清空"才能移除图片，当前保持旧值） |

| P0-22 补充 | 低 | **Actor::loadFromFile 失败应清 m_texture** | 设计器已放行空串写回（"清空=删除"）；Image 同 Actor 复用场景下若失败时旧纹理保留则删除无效——失败=无图语义对齐 |

| P0-23 | 高 | **LuotiAni 帧图资源文件回退** | `getImageFromResource`（LuotiAni.cpp:85-104）仅查 resourceProvider——动画 jsonc 的 `src`（如 `animations/rotateBtn/rotateBtn.svg`）在未注册资源包的宿主（设计器）必然 not found，而 svg 实际就在 jsonc 旁。**建议**：provider miss → 尝试 `basePath/<resourceId>` 文件加载（Surface::loadFromFile），仍失败再 throw——资源包优先语义不变、独立运行/设计器场景零配置即用。对照：Actor::loadFromFile 已有同款 basePath 解析 |

| P0-24 | 低 | **path 键别名统一（92 键原则收敛）** | 布局解析键 `path`（kJsonPath，LayoutParser.cpp:485）与运行时键 `animation`（kAnimation）历史上未对齐——schema（布局语法限定表）写 path、运行时只认 animation。建议：LuotiAni set/getStringProperty 加 `"path"` 别名（= kAnimation），布局键=运行时键；设计器现有特判映射（CanvasPane.cpp）在别名落地后可删 |

| P0-25 | 高 | **CreateAnimation 支持空路径创建** | 设计器 tAni 工具项接入真实 LuotiAni 控件的前置：`UICornerstone_CreateAnimation`（UICornerstoneAPI.cpp:1333）空串会 loadFromFile("") 早退 → prepare 对未加载 throw（LuotiAni.cpp:595-598）→ catch 回滚返回 nullptr。建议：`if (jsoncPath && jsoncPath[0])` 跳过 load/prepare（无路径创建=占位，path 行运行时设置加载）。设计器随后加 createViewportControl 的 tAni 分支（CreateAnimation("")，当前分支缺失落 CreatePanel——动画"占位"工具项故名） |

> **P0-25 状态更新（2026-09-22）**：设计器已采用缺省路径方案（`CreateAnimation("assets/animations/rotateBtn/rotateBtn.jsonc", ...)`——assets 必备文件）绕过空路径创建限制，**引擎侧改动可关闭**；若引擎未来支持无路径占位创建（`if (jsoncPath && jsoncPath[0])`），设计器可改回空路径。

## 追加（2026-09-22 schema 重构讨论——双方达成方向共识）

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-26 | 中 | **schema 视觉属性重构：common 下放各类型 def（按视觉适用性声明）** | colors/text-shadow/font/font-size 从 common def 下放 35 个类型 def，三层合一（声明=布局合法=视觉有效），消除引擎/设计器视觉理解差异。**粒度**：colors 子组按需（有文字控件全四组；形状/面板类仅 background/border；image/animation 全不声明），text-shadow/font/font-size 跟随"有无文字"。**兼容**：解析层不动（schema 仅校验层）；既有布局排查 + validate 未声明键 warning 过渡期。**设计器随批简化**：删除 noVisual 补缺排除/固定区显隐等全部特例，纯 schema 驱动 |

## 追加（2026-09-23 P0-26 联测反馈——A 类引擎缺口）

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-27a | 中 | **Label 背景纯色填充** | schema 已声明 background（colors-full），但 Label::draw 只调 beforeDraw（画纹理）无纯色背景填充段（对照 Button::draw 的 setStateColor 背景矩形填充）——声明与实现缺口 |
| P0-27b | 中 | **popup border / dialog 背景自查** | Popup 继承 Panel（beforeDraw/afterDraw 应有效）但实测 border 色不生效；dialog 背景疑似被画布 panel 遮挡（z 序/绘制顺序）——引擎侧自查两控件绘制链 |
| P0-27c | 低 | **progressbar 文字可达性** | def 有 text/custom-text/text-mode，但 text-mode 缺省 percent 忽略 text——需 text-mode=custom + custom-text 组合；建议 text 键设置时联动/文档说明 |

## 设计器 P1 规划（本轮分诊产出）

| 功能 | 说明 |
|---|---|
| **状态预览** | 画布控件不接收鼠标（选择/拖动拦截）→ hover/pressed 恒不可达——选中控件强制渲染指定状态（normal/hover/pressed/disabled），解决四态色设计期不可预览 |
| **结构化数据编辑** | menu items / TabControl 页签等数组结构，动态区不支持——结构化数据编辑器 |

## 追加（2026-09-23 P0-27b 结案 + 联测收尾）

- **P0-27b 结案（接受引擎不复现结论）**：设计器 dialog/popup 工具项此前为占位映射（tDialog→CreateWinFrame、tPopup→CreatePanel），用户观察的"背景被挡/border 不生效"来自占位组合语义，非引擎绘制缺陷。设计器已接入真实控件（CreateDialog/CreateStatusBar，binding 无 CreatePopup 工厂故 tPopup/tConfirm 维持占位）。
- **P0-27c 备注**：progressbar 文字需 text-mode=custom + custom-text 组合，设计器联测确认后如有体验问题再议。

| P0-27d | 低 | **binding 补 CreatePopup/CreateConfirmPopup 工厂** | 设计器 tPopup/tConfirm 仍占位（Panel）——Dialog.h 有 Popup/ConfirmPopup 类但 binding 未暴露工厂；补齐后设计器同模式接入 |

| P0-28 | 中 | **Dialog 编辑态常显** | Dialog"点击外部关闭"（dismiss）语义与设计器冲突——画布点其它控件即消失。建议：设计器场景抑制外部点击关闭（实例级标志或默认常显至 setVisible(false)）；或提供 API 供宿主关闭 dismiss 行为 |

| P0-30 | 中 | **各控件 hover/pressed 状态机链路核查** | 设计器画布确实转发鼠标（Button override onMouseEnter/Leave 显式切状态→hover/press 生效），但 Actor/LuotiAni 零鼠标处理代码（依赖基类 ControlImpl::onMouseEnter 默认链，实测 hover 色不生效）、ColorPicker/NUD 主体 hover 亦不生效。请核查：①基类默认 onMouseEnter（ControlBase.cpp:664）是否切 Hover 态、Actor/LuotiAni 未 override 时事件是否到达（hitTest 分发条件）；②ColorPicker/NUD 复合结构的主体 hover 链。目标：colors 四态对所有声明控件真实可达 |

> **P0-28 扩围（2026-09-23）**：ContextMenu 同为 dismiss 语义（设计器取消选择即隐藏）——编辑态常显的控件范围 = Dialog + ContextMenu（+未来 Popup/ConfirmPopup）。

| P0-31 | 中 | **ImageButton 全面移除** | 用户决策：Button + normal/hover/pressed/disabled-image 四态图资源行已完整覆盖图片按钮语义，ImageButton 类冗余。**移除范围**：ImageButton 类（include/src）、UICornerstone_CreateImageButton C ABI、binding CreateImageButton、LayoutParser 的 image-button 类型解析、相关测试。**注意**：①AnimatedButton（luoti 动画按钮）语义不同**保留**；②既有布局的 image-button 类型建议解析层降级为普通 Button 并 Warn（或引擎自查布局后彻底删）；③schema 无 image-button def（已核实）无需动。设计器侧已同步清理（工具项/类型映射/创建分支） |
