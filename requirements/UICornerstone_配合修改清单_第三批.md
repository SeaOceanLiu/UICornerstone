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
3. **P1-4（HandleHitTest 注释勘误）**：一行注释修正，随批顺手。
4. **P1-5/6（手册）**：随 Wheel_Support 实施批落地（§8 已列）。
5. clip 穿透：待观察。

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

> **P0-30 复核补充（2026-09-23 二次评审）**：设计"基类仅切 Hover"不完整——未自实现按下的控件 pressed 色槽仍不可达，四态可达目标只完成一半。请引擎补充：基类 onMouseDown/Up 对称切 Pressed（分发条件与点击一致）；非交互控件（isContainsPoint=false）的 pressed 不可达属语义正确（设计期经状态预览/程序化 setState）。详见 Temp/PopupFactories_HoverChain_ImageButton_Design_复核意见.md §1 修正段。

## 追加（2026-09-24 P0-26 落地后第二轮联测——schema 声明与实现对齐）

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-32 | 高 | **schema 声明与实现对齐：输入控件收回 shadow 声明** | §3 矩阵决策"输入控件不加阴影"（edit-box/text-area/combo-box/numeric-up-down shadow ✗）但 §4.2 full 清单误含——schema 声明了 shadow 而运行时无实现，设计器按声明生成无效行。**收回 4 类型 shadow 声明**（设计器 def 驱动自动消失）。原则重申：声明=实现，未实现的不要声明 |
| P0-33 | 高 | **hover/pressed 链路第二批** | ①slider pressed 不生效；②progressbar background.hover/pressed、border.pressed 不生效；③menu-bar background.hover/pressed 不生效（BAR_HOVER/BAR_ACTIVE 映射链）；④Animation(LuotiAni) hover/pressed 均不生效（疑 update override 未调基类，对照 Actor hover 已生效）；⑤ColorPicker pressed 不生效 |
| P0-34 | 高 | **ColorPicker 文字转发错位 + 弹层生命期** | ①colors.text 四态转发错位：normal/hover 不生效、disabled 色被当作 normal 生效（槽位映射 bug——closedLabel 转发）；②删除已弹窗的 ColorPicker 时弹窗未一并关闭（popupPool 生命期随主体删除） |
| P0-35 | 中 | **ProgressBar 视觉三处** | ①文本在手柄放大后不居中（文本定位未跟随值变化）；②text 仅 normal 生效（hover/pressed/disabled 转发缺）；③shadow 无颜色生效（enable/offset 已生效——色值转发缺） |
| P0-36 | 中 | **NUD text.disabled 不生效** | colors.text.disabled 转发链核查 |
| P0-37 | 低 | **ContextMenu resize 颜色区块跟随** | 大小改变时颜色区块（menu panel/条目背景）未一并改变尺寸 |

> StatusBar/TabControl 文字相关用例依赖结构化编辑（设计器 P1 已列）；background/border 已验证正常。
> Image pressed 不生效为预期语义（isContainsPoint=false 非交互控件不收按下），非缺陷。

## 追加（2026-09-27 视觉测试第三轮——引擎清单）

| # | 优先级 | 问题 | 说明 |
|---|---|---|---|
| P0-38 | 中 | **ProgressBar textLabel 态同步** | text.hover 只在 label 自身 hotRect 内生效——应随 ProgressBar 整体状态（对照 §3.6 Slider setState 同步 valueLabel 模式） |
| P0-39 | 中 | **Slider 三处** | ①shadow.offset.* 读回缺省 0，实际语义应为 2.0（缺省值对齐）；②valueLabel hover 无法获得滑块 hover 状态（只能自身 hotRect——setState 同步链实测未通）；③text/text-shadow 的 disabled 不生效 |
| P0-40 | 中 | **ScrollBar 两处** | ①厚度无法被外观手柄（HandleControl）拖动调整；②轨道 background 色无法呈现（疑被滑块遮挡）——建议增加滑块（thumb）色键 |
| P0-41 | 低 | **WinFrame 关闭按钮边框** | 窗框 border（整体窗框）波及右上关闭按钮——关闭按钮应保持无边框 |
| P0-42 | 中 | **Splitter 背景色不受控** | colors.background 设置后视觉无变化（自绘把手/分隔线覆盖背景？） |
| P0-43 | 中 | **ListView border 不生效** | 控件级 colors.border 三态声明但视觉无变化（beforeDraw/afterDraw 绘制链核查） |
| P0-44 | 高 | **MenuBar background.hover/disabled 不生效且无法读回** | P0-33③ 实施回炉：对象路径映射/读回组装实测未通 |
| P0-45 | 中 | **ContextMenu 无动态行** | schema 无 context-menu def（defs.contains 失败动态区空）——请补 def（ContextMenu 继承 Popup，能力矩阵两态）或提供属性面板映射方案 |
| P0-46 | 低 | **font 运行时接口形态确认** | 设计器已生成 font 字体名 string 行（写回 SetString("font")）——请确认引擎分发形态（SetString vs setEnumProperty）并对齐；读回（GetString）同步 |

| P0-47 | 中 | **font 读回两处** | ①Slider：getEnumProperty(kFont)/getStringProperty(kFont) 读回缺失（写回转发已通——读回无）；②WinFrame：getStringProperty(kFont) 返回错位（实测返回 title 的 caption 内容而非字体名——实现串写） |

| P0-48 | 中 | **ComboBox selection-changed 经 Binding 未触发** | 设计器属性面板 font ComboBox：AddItem/selected-index 读写正常，但用户下拉选择后 kEventSelectionChanged 回调（Binding SetCallback 注册）未触发。交互链完整（列表项 MouseDown→selectItem→fireCCallback(Selection)——ComboBox.cpp:534/908-913），setCallbackProperty 支持该事件（:1051）——疑 fireCCallback C 通道与 Binding 桥（Selection 载荷→Event）衔接问题，请引擎自查并提供测试用例 |

| P0-48 扩展 | 高 | **font 设置后控件文字字体不变** | 设计器写回链全通（SetString(kFont)→Enum 别名→读回 match=1，如 Muyao-Softbrush），但控件文字视觉不变。请引擎：①核查 setEnumProperty(kFont) 的字体切换链（枚举名解析→Font 对象→caption Label 字体更新→重绘）；②提供**合法字体枚举名清单**或查询 API（设计器 ComboBox 选项需与引擎枚举对齐——当前清单为设计器 registerFonts 的 6 个字体资源 stem，疑与引擎枚举表不一致） |

| P0-47 扩展 | 中 | **kFont 未设置时读回实际生效的缺省字体名** | 现状：font 未显式设置时 GetString(kFont) 返回空（属性系统回显设置值），设计器无法得知控件实际字体，被迫造"(缺省)"虚拟项。请引擎：kFont 读回在未显式设置时返回**控件实际生效的缺省字体名**（构造字体 → 名字）。设计器随批：删"(缺省)"项，读回实际名直接选中 |

| P0-49 | 中 | **text-shadow per-state 读回失败（hover/pressed）** | 设计器第一轮视觉反馈（漏列，此补）：Button/Label 的 text-shadow 槽 normal/disabled 读回正常，**hover/pressed 读回失败**（caption-label 直控回读链）——写入视觉生效、读回不回。请引擎核查 getTextShadowStateColor 的 per-state 读回分发（text-shadow.hover/.pressed 键），并排查其它 StateColor 组（text/border/background）是否存在同样的 per-state 读回缺失 |

## 新设计任务：容器类子控件编辑（design/07 已定稿）

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| E-4（P0-50） | 高 | **Binding 补 AddChild/RemoveChild 容器挂载** | 设计器容器子控件放置必需：`Control AddChild(Control& parent, Control& child)`（引擎 parent->addControl）/`bool RemoveChild(Control& parent, Control& child)`（摘除不销毁或销毁语义请定）；Binding 封装 + C ABI（如需脚本通道）。设计器 07 设计 P1-a（容器内放置）依赖此能力 |

设计文档：`CornerstoneDesigner/design/07_容器类子控件编辑设计.md`（已定稿——放置双通道/WinFrame 严格 ClientPanel/分期 P1-a→P2）

## 追加（2026-10-02 浮层遮挡子视口——引擎清单）

**背景（设计器实测）**：画布工具栏的 ComboBox（cb_gridcell/tb_align 等）打开下拉后，列表被中央画布（子视口）整片盖住。根因：主实例 `Render()` 时浮层已画（Popup 挂主实例 BENCH 顶层，子列表末尾），随后 `vp->Render()` 在画布区域覆盖；调换渲染顺序不可行——主实例 bench 根背景不透明（`ConstDef::DEFAULT_NORMAL_COLOR = (23,23,24,255)`，见 `Capture_API_Design.md` §像素级覆盖语义）+ root 面板不透明（#202020），先渲画布再渲主实例会把画布整片盖掉。事件侧同病：`findViewportByCoord`（src/UICornerstoneAPI.cpp:277-286）无条件把落在子视口 rect 内的鼠标事件路由给子视口——浮层压在画布上时，点击下拉项进不了主实例 bench（点不中、外点也不关）。

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-52① | 高 | **浮层渲染通道（子视口之上）** | 新增 `UICornerstone_RenderOverlays(UIInstance)`（Binding `RenderOverlays()`）：只绘制本实例 BENCH 顶层可见浮层控件（ControlType ∈ {Popup, ConfirmPopup, Dialog, MenuPanel}，含其子树），仍按实例 viewport 裁剪。宿主帧序变为 `Clear → owner.Render → vp.Render → owner.RenderOverlays → Present`；基准 `Render` 行为不变（浮层随主渲染绘制一次，overlay pass 重绘于子视口之上；浮层背景不透明，重绘无叠色问题）。设计器随批：App.cpp 渲染循环加一行 + Binding 同步 |
| P0-52② | 高 | **浮层优先的事件路由** | 鼠标路由（src/UICornerstoneAPI.cpp:737）在 `findViewportByCoord` 之前先判 owner 顶层是否存在可见浮层：存在 → 鼠标事件优先发 owner bench（浮层 watcher 处理命中/外点关闭；是否穿透由引擎设计定），否则再按坐标路由子视口。否则浮层压画布时下拉项点不中、外点关闭失效 |

> 备选设计（由引擎定）：bench 内联子视口渲染（子视口内容并入主实例绘制序、浮层自然置顶）或浮层延迟渲染通道；目标语义一致——浮层恒在所有子视口之上且可交互。设计器侧集成改动最小者优先。

> **状态（2026-10-02）**：引擎已实施并同步 subModules（含路由探针与 owner 路径 activeViewport 回收，复核附项落实）；设计器 App.cpp 已接线 `RenderOverlays`；构建零警告、冒烟 clean、矩阵/strict 全绿、视觉验证通过（下拉浮层完整显示可点选/外点关闭/画布交互恢复）。本批闭环。

## 追加（2026-10-02 StatusBar 文本更新——引擎清单）

**背景（设计器实测）**：主窗体状态栏左段（statusHint）初始"就绪"、设计器启动时改为"放置目标：画布根"——新文本与中段"未选中控件"**重叠**（放置一个控件后才恢复）。根因：`StatusBar::updateStatusItemText`（src/StatusBar.cpp:46-50）只改 `item.text`，**不触发 `relayout()`**——段 hitRect 宽度仍按旧文本（"就绪"）计算，长文本溢出压到相邻段；`addStatusItem`/`removeStatusItem` 均会 relayout，唯文本更新遗漏（字体首次就绪的 draw 内自愈重排才偶然修正）。

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-53 | 中 | **updateStatusItemText 应触发 relayout** | 段宽随文本变化：`updateStatusItemText` 内设置文本后补 `relayout()`（与 add/remove 语义对齐）。设计器过渡：过渡期以"临时段增删"强制 relayout（同帧完成、无视觉影响），引擎修复同步后删除过渡代码 |
| P0-54 | 中 | **字体路径约定两套不一致（内存 provider 下部分控件无文字）** | Label 走相对路径（`provider->readFile("fonts/…")`）；StatusBar/ListView/Menu/TabControl/TreeView 走绝对路径（`ConstDef::pathPrefix.string() + "/" + rel` = `GetBasePath()+"assets" + "/" + rel`）。视口内存 provider 为精确键匹配——后者在子视口（设计器画布）读不到已按相对键注册的字体 → 状态栏段/列表列头/菜单/页签/树节点**无文字**。建议：`MemoryResourceProvider::readFile` 增加前缀剥离回退（绝对路径以 `ConstDef::pathPrefix` 开头时，去前缀重试相对键）；或统一为 Label 的相对路径约定。设计器过渡：字体按相对+绝对两种键双注册 |
| P0-55 | 中 | **StatusBar 分段着色（用户需求）** | `StatusItem` 无颜色字段（段文字统一用控件级 text 四态色）——无法分段着色。需求：① per-segment **text 色**（至少 normal，建议四态）；② per-segment **背景色**（VSCode 风格彩色段）。建议：StatusItem 增字段 + API `StatusBarSetItemTextColor(bar, id, rgba[, state])` / `StatusBarSetItemBackgroundColor(bar, id, rgba)` + JSON items 键扩展（`text-color`/`background-color`）+ Binding 封装。设计器随批：行式编辑扩展（如 `文本|#RRGGBB`），模型持有颜色 |
| P0-56 | 中 | **ListView 表头背景色与文字阴影（用户问询）** | 现状：表头背景 `m_headerBgColor{45,45,52}`、控件级表头文字色 `m_headerTextColor{200,200,205}` 均为内部常量（无属性/API/schema 键）；`HeaderStyle`（per-column：textColor/fontName/fontSize）无阴影字段（`ListViewSetCellShadow` 仅单元格）。需求：① 表头**背景色**（控件级 + 可选 per-column）；② 表头**文字阴影**（色 + 偏移，per-column 可选）；③ 控件级表头文字色暴露（当前仅 per-column API `ListViewSetColumnHeaderStyle`）。schema 键建议 `header-background`/`header-text`/`header-shadow`；设计器随批扩展 columns 行式格式 |
| P0-57 | 中 | **StatusBar 运行期 font-size 不生效（缺陷）** | `StatusBar::setFontSize`（StatusBar.cpp:127-129）仅 `relayout()`，**未失效缓存字体 `m_font`**——`relayout→ensureFont` 早退，字号/测量宽度均按旧字号。对照：TreeView::setFontSize（m_font.reset()+m_nodeFonts.clear()+ensureFont）、TabControl::setFontSize（m_font.reset()+ensureFont+relayout）均正确。修复：StatusBar::setFontSize 补 `m_font.reset()`（+ relayout）；建议顺带审计同类控件（MenuBar/MenuPanel 等）的 setFontSize 缓存失效 |
| P0-58 | 低 | **StatusBar 段级字号/文字阴影（用户问询，可选）** | 现状：字号/阴影仅控件级（P0-55 段色已列）。需求（对齐 ListView per-column 能力）：`StatusItem` 增 `fontSize`（0=继承控件级）与 `shadowColor/offset`（可选）；API/JSON items 键（`font-size`/`text-shadow`/`text-shadow-offset-x/y`）+ 绘制分派。若引擎认为需求弱可后置 |
| P0-59 | 中 | **运行时布局模式切换 + 锚点读回（用户需求：控件固定在窗体某一边）** | 现状：锚点解析期完整（`layout.type=anchor` + 子控件 `anchor`/`anchorOffset`）；运行期子控件级写入已有（`Panel::setEnumProperty("anchor")` / `setFloatProperty("anchor-offset-x/y")`，配合父容器 `SetString("child-id", 子控件id)` 定位）——但：① **容器布局模式无运行时分发**（`layout` 键未实现，`setLayoutEngine` 仅解析期）→ 运行期无法把容器切到 anchor 布局，锚定不生效；② `Panel` 无 `getEnum/getFloat` 覆写 → 锚点**无读回**。需求：① `SetEnum("layout", "h-flow|v-flow|anchor|grid")`（+GetEnum）运行时切换布局引擎并重排；② `GetEnum("anchor")`/`GetFloat("anchor-offset-x/y")` 读回（配合 child-id）；③ schema：`anchor`/`anchorOffset` 从 panel def 下放 common（供设计器枚举行生成）。设计器随批：属性面板"布局模式"下拉（容器）+ "锚定"下拉（9 锚点 + 4 拉伸：top/bottom/left/right-stretch）+ 偏移行；预览验证贴边 |
| P0-60 | 高 | **锚点 setter 触发重排 + applyAnchor 稀疏语义（P0-59 实测缺陷）** | 设计器实测（P0-59 联测）：① 设完 `anchor`/`anchor-offset-x/y` **不触发重排**——仅切换布局/父 resize 才应用，用户设"底拉伸"后控件仍停在旧位置（实测"底拉伸跑到顶部"）；② `AnchorLayout::applyAnchor` 对**未设锚点**的子控件按默认 `top-left` 处理——切换布局瞬间把全部子控件塌到左上堆叠（实测"切 anchor 后选不中 Panel 内控件"；设计器已加模型同步过渡，但塌左上本身不合直觉）。需求：① `setEnumProperty(kChildAnchor)` / `setFloatProperty(kChildAnchorOffsetX/Y)` 更新 map 后**触发 `reflowChildren()`**（立即应用）；② `applyAnchor` 改**稀疏语义**：`anchorProps` 未命中的子控件**保持现有 rect**（跳过），仅管理显式锚定者（与 per-column 稀疏样式家族一致）。设计器过渡：`forceReflow`（父 SetRect 同值强制 reflow）+ `syncModelsFromEngine`——引擎修复同步后删 `forceReflow` |
| P0-61 | 中 | **根级锚定（Bench 布局引擎支持）——顶层控件锚定画布/窗体边（用户拍板走 Bench）** | 用户问询：顶层控件（StatusBar 等）直接锚定画布底部。Bench 继承 Panel、天然有 `m_layoutEngine`/`m_anchorItemProps`/`reflowChildren`，路线成立；实际障碍三处：① **设计器拿不到 bench 句柄**（无 `GetRoot`/根 child-anchor API）→ 顶层控件的 `child-id`+`anchor` 无处可写；② `Bench::resized`（Bench.cpp:123-130）仅 `Panel::resized`+`recomputeViewportTransform`，**不触发 `reflowChildren()`** → 视口/窗口 resize 时锚定不重排；③ off 模式 `SetCanvasSize`（UICornerstoneAPI.cpp:639-652）仅记录不即时应用（需下一次 recompute 才置 rect）。需求：① 暴露根句柄（`UICornerstone_GetRoot(instance)` → UIControlHandle；Binding `Root()`；子视口实例返回其 bench）或等效实例级根锚点 API；② `Bench::resized` 存在布局引擎时补 `reflowChildren()`；③ `SetCanvasSize` off 模式即时应用（setRect + recompute）。前提：P0-60 稀疏语义（已放行——根上的 overlay：网格/参考线/框选/手柄不被塌）。设计器随批：顶层控件锚定行父级解析支持"根"；根尺寸=可见逻辑区（`contentW/zoom`，窗口/分割条/缩放变化时同步）→ 锚定目标=可见画布底/边；首次设置顶层锚定时自动确保根为 anchor 布局；删 `forceReflow` 过渡 |

> **状态（2026-10-03）**：**P0-53~P0-61 全部闭环**——引擎均已实施并同步 subModules；设计器随批完成（删 P0-53/P0-54/P0-60 三处过渡、结构化编辑、段色/段级字号阴影、表头全样式、布局模式/锚定/偏移、设计画布尺寸、平移/滚动条/鼠标坐标、顶层根锚定、`SetControlId` 子视口实例注册修复）。本批无待引擎项。

## 追加（2026-10-03 单元格/树行着色——引擎清单）

**背景（设计器实测/用户问询）**：结构化编辑（ListView rows / TreeView items）已可用，但单元格/树行着色能力盘点：① ListView `CellStyle`（ListView.h:42-51）**有 textColor 字段但 ABI 仅暴露 `SetCellStyle`（bg+fontSize）与 `SetCellShadow`（阴影色+偏移）——单元格文字色无 setter**；② TreeView `TreeNode`（TreeView.h:18-43）**有 item 级四态 bg/text/border + textShadow + hasStyle 字段，但绑定/C ABI 无任何运行时着色 API**（仅 AddNode/RemoveNode/SetNodeLabel/UserData）。需求如下（设计器随批将扩展行式格式：单元格/节点样式字段 + x-color 之外的色键）：

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-62① | 中 | **ListView 单元格文字色 API** | `CellStyle.textColor` 已有字段无 setter：新增 `UICornerstone_ListViewSetCellTextColor(inst, lv, row, col, r,g,b,a)`（或 `ListViewSetCellStyle` 扩展变体——建议新增独立 API 保 ABI 兼容）+ Binding 封装；设计器随批：rows 行式格式扩展单元格文字色 |
| P0-62② | 中 | **TreeView 节点级着色 API** | `TreeNode` 字段齐备（bg/text/border 四态 + textShadow + hasStyle）但无入口：新增 `TreeViewSetNodeTextColor(tree, id, rgba[, state])` / `TreeViewSetNodeBackgroundColor(tree, id, rgba)` / `TreeViewSetNodeShadow(tree, id, rgba, ox, oy)`（设置任一 → hasStyle=true；稀疏——未设继承控件级）+ JSON items 键（`text-color`/`background-color`/`text-shadow`/`text-shadow-offset-x/y`，与 ListView 表头键同族）+ Binding 封装；设计器随批：items 行式格式扩展节点样式字段 |

> **P0-62 状态（2026-10-03）**：**已闭环**——引擎已实施（`ListViewSetCellTextColor` + `CellStyle.hasTextColor` 稀疏修复；`TreeViewSetNodeTextColor/BackgroundColor/Shadow`，bg/阴影四态同色、text 走 mask 回退——复核建议已采纳）并同步；设计器随批完成（rows 单元格着色、tree 节点着色、统一行式格式）。

## 追加（2026-10-03 统一行式格式联测——引擎清单 P0-63）

**背景**：用户实测统一结构化格式（`@` 属性 / `|` 分隔 / 属性序 背景色→字体色→字号→字体名→阴影色→偏移x→偏移y）后提出：① 单元格背景设置后 hover/选中无视觉反馈；② 字体名支持（各控件）。设计器侧已实现统一格式（字体名：tree 节点走通用链即时生效；cells/columns/statusbar 模型保留待 API）。

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-63① | 中高 | **单元格背景与 hover/选中反馈（体验缺陷）** | `ListView` 单元格背景绘制于行高亮**之上**（ListView.cpp:676-680「覆盖高亮之上，差异着色可见」）→ 设了单元格背景的行 hover/选中**完全无反馈**。需求：引擎设计（如高亮以半透明叠加于 cell bg 之上；或 hover/selected 时对 cell bg 做亮度调制）——保证 per-cell 着色与交互反馈并存 |
| P0-63② | 中 | **ListView 单元格字体名 API** | `CellStyle.fontName` 有字段无 API（`SetCellStyle` 仅 bg+fontSize、`SetCellTextColor` 仅色）→ 新增 `ListViewSetCellFontName(lv, row, col, name)`（或 `SetCellStyle` 扩展变体保 ABI）+ Binding；设计器随批接线（行式格式字体名已解析入模型） |
| P0-63③ | 中 | **ListView 列头字体名 API** | `HeaderStyle.fontName` 有字段无 API（`SetColumnHeaderStyle` 仅 color+fontSize）→ 新增 `ListViewSetColumnHeaderFontName(lv, col, name)`（或扩展）+ Binding；设计器随批接线 |
| P0-63④ | 中 | **TreeView 逐节点字体：名称单独设置不生效（缺陷）+ 专用 API（便利）** | ① 缺陷：`getNodeFont`（TreeView.cpp:111）`if (!node \|\| node->fontSize <= 0) return m_font;`——**仅设字体名（fontSize=0）永不生效**。建议：fontSize<=0 时以控件级 `m_fontSize` 驱动逐节点字体（名称生效、字号随控件级）；设计器已加过渡（名称单独设置时读控件级字号写入节点），引擎修复后删；② 便利：专用 `TreeViewSetNodeFont(tree, id, name, size)` 免共享 `item-id` 状态误写 |
| P0-63⑤ | 低 | **StatusBar 段级字体名（可选）** | 段结构（StatusBar.h:31-42）无 fontName 字段 → 需引擎加字段 + API/JSON 键；设计器行式格式已解析入模型（`font-name`），引擎支持后接线。若认为需求弱可后置/拒绝 |
| P0-63⑥ | 中 | **ComboBox 逐项样式（用户问询）** | `ComboBoxItem`（ComboBox.h:16-20）仅 label/value/disabled——无逐项样式。需求：逐项 背景色/字体色/字号/字体名/阴影/偏移（对齐统一属性序）+ API（如 `ComboBoxSetItemStyle`，按 index/id）+ JSON items 键 + 下拉列表绘制分派（注意 hover/选中态与逐项背景的叠加——同 P0-63① 家族）。引擎评估范围/优先级 |
| P0-63⑦ | 中 | **字体枚举不一致：schema 28 token vs FontName 枚举 6** | `declarative-ui.schema.json` fontName enum 列 28 个 token，但 `FontName` 枚举（ConstDef.h:42-49）仅 6 项，`FontNameFromString` 对**其余 22 个 token 静默回退 regular**（如 `harmonyos-sans-sc-bold`/`-black`/`-light`/`-medium`、maplemono 斜体族等）——设计器属性面板字体下拉（schema 驱动）同样受影响（选 22 个无效字体无任何提示）。建议：引擎补字体实现（FontName 扩展）或 schema enum 收窄到实际支持的 6 项（+文档标注）【**已更正：引擎核实 schema 实际 6 项；本项系设计器基于过期 Temp 副本误报——无需处理**】 |

> **P0-63 状态（2026-10-04）**：**已闭环**——引擎实施并同步（① 半透明高亮叠加；②③ hasFontName 稀疏 + 字体名 API；④ getNodeFont 门槛修复 + 专用 API；⑤ 段级字体名；⑦ 更正）；设计器随批完成（rows/columns/statusbar 字体名接线、tree 过渡删除）。段阴影经 C ABI 像素探针复核（同步 DLL：red_before=0 → red_after=67，绘制正常）。

## 追加（2026-10-04 hover 体验——引擎清单 P0-64）

**背景**：P0-63 联测后用户实测/建议：① TreeView 节点设背景后 hover 无反馈（同 P0-63① 家族，引擎未覆盖 TreeView）；② 建议 ListView、TreeView、ComboBox、StatusBar、Menu 支持设置鼠标 Hover 时的彩色体验（逐项 hover 态颜色可配）。

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-64① | 中高 | **TreeView 节点背景与 hover/选中反馈（缺陷）** | TreeView.cpp:234-243：`if (rowNode->hasBgStyle) { fill bg } else if (selected) … else if (hover) …`——节点设背景后选中/hover 高亮被**完全替换**（无反馈）。建议与 ListView P0-63① 同机制：hasBgStyle 且 hover/selected → 半透明高亮叠加（复用 alpha 常量）；disabled 不叠加 |
| P0-64② | 中 | **逐项/控件级 hover 彩色体验（用户建议，跨控件）** | 需求：ListView、TreeView、ComboBox、StatusBar、Menu 的 hover（+选中）态颜色可配。现状盘点：TreeView 控件级 `setHoverColor` 内部有（无属性/API 通道待确认）；ListView `m_hoverColor` 私有；StatusBar `kHoverColor` 常量（不可配）；ComboBox 控件级 item-hover/selected/disabled 色已具备（配置通道待确认）；Menu 待引擎盘点。建议引擎设计统一方案：① 控件级 hover/selected 色可配（属性/API）；② **per-item hover 覆盖**（稀疏/mask 回退家族）：ListView cell（单色 → 四态或 hover 字段）、TreeView node（bgColor 已四态——API 加 `state` 参数即可）、StatusBar 段（背景单色 → state 参数；文字色已四态）、ComboBox 逐项（P0-63⑥ 暂缓项——用户本次点名，请重新评估最小子集 `text-color`+`background-color`）、Menu 项；③ 与 P0-63① 叠加机制协同（显式设 hover 色 → 用显式色；未设 → 叠加/控件级回退）。设计器随批：行式格式扩展 hover 色字段（统一属性序追加，如 悬停背景色/悬停字体色） |

### P0-64 增补（2026-10-04 用户拍板——撤销 P064 设计的"暂缓"）

**背景**：P064 设计将"ListView cell / StatusBar 段显式 hover 色"列为暂缓（控件级通道 + 叠加已覆盖体验）；**用户明确要求逐项显式 hover 色——撤销暂缓，纳入本批**。

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-64③ | 中 | **ListView 单元格显式 hover 背景色** | `CellStyle` 增 hover 背景字段 + 掩码（稀疏家族，对齐 `hasTextColor` 形态）：建议 `SColor hoverBgColor; bool hasHoverBg = false;`（selected 态可选——引擎评估 `selectedBgColor/hasSelectedBg`）。API（保 ABI，独立函数或带 state 变体——引擎定形）：如 `ListViewSetCellHoverBackgroundColor(lv,row,col,rgba)`。绘制：显式 hover 态 → 用显式色（不叠加）；未设 → P0-63① 叠加（现有行为）；selected 同理（若纳入）。JSON rows cells 键：`hover-background-color`（或 `background-color` 对象四态——引擎定形）+ Binding 封装 |
| P0-64④ | 中 | **StatusBar 段级显式 hover 背景色** | `StatusItem` 增 hover 背景字段 + 掩码：建议 `SColor hoverBackground; bool hasHoverBackground = false;`。API：`StatusBarSetItemHoverBackgroundColor(bar,id,rgba)`（或现有 `SetItemBackgroundColor` 带 state 变体——引擎定形）。绘制：显式 hover 态 → 用显式色；未设 → 控件级 `m_hoverColor` 叠加（P0-64 本批机制）。JSON items 键：`hover-background-color`（或 `background-color` 对象四态）+ Binding 封装 |

**设计器随批（格式定案）**：行式格式新增键前缀 **`hb#`（悬停背景色）**——TreeView 节点 / ListView 单元格 / StatusBar 段按各引擎支持面接线；**`ht#`（悬停字体色）暂不加**（用户拍板：一个 hover 前缀即可，后续按需）。验收建议：像素探针（设 `hb#` 后 hover 显式色、未设走叠加）+ C ABI 断言（字段/掩码）+ 回归。

## 追加（2026-10-04 P0-64 联测缺陷——引擎清单 P0-65）

**背景**：P0-64 已实施并同步；设计器随批（`hb#`）联测发现 **"仅设 hover 背景"（无常态背景）时常态落默认色**——两处：

| # | 优先级 | 缺陷 | 说明 |
|---|---|---|---|
| P0-65① | 中高 | **ListView 单元格 hover-only 常态填黑** | `setCellHoverBackgroundColor`（ListView.cpp:426-431）创建的 CellStyle **默认 `bgColor = SColor()`（α=1.0）**；draw 常态门槛 `hasBase = csC.bgColor.alpha() > 0`（:700）→ **为真 → 常态填黑**（设计注释"无常态 bg 也可用"未成立）。修复建议：CellStyle 增显式位（如 `bool hasBg = false;`，仅 `setCellStyle`/ABI 置位），draw 改判 `csC.hasBg`（显式透明仍可不填） |
| P0-65② | 中高 | **TreeView 节点 hover-only 常态落默认深色** | `setNodeHoverBackgroundColor`（TreeView.cpp:931-938）置 `hasBgStyle=true` + `bgMask|=2`；draw base fill（:240-244）**不看掩码** → 常态 `resolveStateColor(bgColor, Normal)` = StateColor 默认 normal（DEFAULT_NORMAL_COLOR 深色）→ **常态深色**。修复建议：base fill 按状态掩码判定——`hasBgStyle && (bgMask == 0 || (bgMask & stateBit(bgSt)))`（bgMask==0=单色全态 API；位命中=显式态；未命中不填充 → 常态透明、hover 显式） |

**设计器过渡（已加，引擎修复后删）**：① rows：hover-only（无常态 bg/字号）先 `SetCellStyle(透明,0)` 再 hover 色；② tree：hover-only 先 `SetNodeBackgroundColor(透明)` 再 hover 色。StatusBar 段无此问题（`hasBackground` 独立位）✓。

## 追加（2026-10-04 导入/导出 JSON 缺口——引擎清单 P0-66）

**背景**：设计器计划支持 打开/保存（`.csd` 项目）+ 导入/导出（引擎布局 JSON）。逐键核对 `LayoutParser.cpp` 后确认两项**必须补**（用户拍板：列表头多为设计后不变更——列头样式必须无损往返；单元格样式同样需要）。

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-66① | 中高 | **ListView columns JSON 补列头字号/字体名解析** | 现状：parseListView columns（LayoutParser.cpp:659-685）只读 title/width/sortable/`header-text`/`header-background`/`header-shadow`(+偏移)——**`header-size`（列头字号）与 `header-font-name`（列头字体名）无 JSON 入口**（`HeaderStyle.fontSize/fontName/hasFontName` 字段齐备，仅缺解析）。需求：columns 元素补 `header-size`（number → `HeaderStyle.fontSize`）+ `header-font-name`（string → `fontName` + `hasFontName`）；与 API 面（`SetColumnHeaderStyle`/`SetColumnHeaderFontName`）语义一致 |
| P0-66② | 中高 | **ListView rows JSON 补单元格样式解析（对象 cell）** | 现状：parseListView rows 的 cells **只读字符串**（`c.is_string() ? c.get<string>() : string()`——**对象 cell 被读成空串**）；CellStyle（背景/悬停背景/文字色/字号/字体名/阴影+偏移）**无 JSON 入口**（styles 仅 API）。需求：cells 元素支持对象形态 `{text, background-color, hover-background-color（或 background-color 对象四态）, text-color, font-size, font-name, text-shadow, text-shadow-offset-x/y}` → CellStyle（hasBg/hasHoverBg/hasTextColor/hasFontName/fontSize/shadowEnabled）；**字符串形态保持兼容**；键形态建议与 statusbar/tree items 家族一致（引擎定形） |

**备注（设计器侧，无需引擎动作）**：① 设计器 columns 模型键 `header-color` 与引擎 `header-text` 不一致——导出映射转换（或随导出实现把模型键对齐为 `header-text`）；② **画布尺寸经顶层 `viewport` 键**（`{width,height[,scale-mode]}` → `canvasWidth/Height` + bench rect）进 JSON ✓（引擎已支持）。

## 追加（2026-10-05 动态区 TextArea 光标跟随——引擎清单 P0-68）

**背景（设计器实测，用户报告）**：动态区（属性面板）TextArea 中光标移动出可视区后**不自动滚动让光标可见**（上移/编辑中部文本尤甚）。

**根因（源码核实）**：`TextArea::m_autoScroll` **缺省 true 且无属性/API 可关**（TextArea.cpp:16；头文件仅成员无 setter）；`ensureCursorVisible`（:1362/:1382）在 `autoScroll=true` 时**恒滚到底**（日志式自动跟随）——编辑式"光标跟随"逻辑（:1366-1379）被覆盖。

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-68 | 中高 | **TextArea 暴露 `auto-scroll`（bool）属性** | 语义：`true`=恒滚到底（日志式自动跟随）；`false`=光标跟随（编辑式——`ensureCursorVisible` 正常逻辑：光标上/下移出可视区即滚动使其可见）。实现：`setBoolProperty/getBoolProperty("auto-scroll")` + Binding + JSON 键 + schema；**缺省建议改 `false`**（编辑式为 TextArea 通用语义；日志式显式开 true）——若引擎顾虑兼容可保持缺省 true，由设计器显式设 false（请引擎定形）。设计器随批：动态区全部 TextArea（普通文本行 + 结构化 items/columns/rows 行）创建后 `SetBool("auto-scroll", false)` |

> **P0-68 状态（2026-10-05）**：**已闭环**——引擎实施并同步（`auto-scroll` setBool/getBool + **缺省改 false** 编辑式，按复核 Q-1）；设计器随批完成（动态区两类 TextArea + 工具箱放置 tArea 显式 `SetBool("auto-scroll", false)`——双保险不依赖缺省）。构建零警告、冒烟/自测 18/0。

## 追加（2026-10-05 内容类属性编辑缺口——引擎清单 P0-69）

**背景**：设计器内容类属性编辑器扩展（用户确认：MenuBar `menus` 格式可行、TabControl `tabs` 可行（每页控件由设计器自管页 Panel）、Shape `points` 先行、ContextMenu 等引擎）。逐项核对发现**移除类 API 缺口**（重编辑时需移除旧内容再重建）：

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-69① | 中 | **MenuBar 菜单移除 API 暴露** | `MenuBar::removeMenu(caption)` 内部已有（Menu.h:273）但 C ABI/Binding 未暴露 → 结构化 `menus` 编辑（重编辑移除旧菜单再重建）无法实现。需求：`UICornerstone_MenuBarRemoveMenu(inst, bar, caption)`（或 ClearMenus）+ Binding `MenuBarRemoveMenu` |
| P0-69② | 中 | **TabControl 页移除 API 暴露** | `TabControl::removeTab(index)` 内部已有（TabControl.h:34）但未暴露 → `tabs` 编辑（删页）无法实现。需求：`UICornerstone_TabRemovePage(inst, tab, index)` + Binding `TabRemovePage` |
| P0-69③ | 低 | （可选）MenuPanel 清空/移除项 API | 设计器每菜单新建 panel 可规避；引擎若提供 `MenuPanelClearItems` 更省（可选，不阻塞） |

> **P0-69 状态（2026-10-06）**：**已闭环**——引擎实施并同步（MenuBarRemoveMenu/TabRemovePage + §3.4 TabSetPage 双挂载修复）；设计器随批完成：**MenuBar menus 编辑器**（`>` 层级/@快捷键/-- 分隔）+ **TabControl tabs 编辑器 + 页 Panel 方案 A**（页内放置/页 Panel 可选中/树"页 N·标题"/序列化 `tabs[i].page.children`）；自测扩展 24/0（菜单/页签结构往返）。过程修复：放置记账 parentId（TabControl→当前页 Panel）、syncTabs 悬垂指针、页模型 extended null、重复挂载、画布控件操作实例统一（33 处 m_ui→m_viewport）。

## 追加（2026-10-05 联测——引擎清单 P0-70）

**背景**：P0-68/资源面板联测（用户实测）：

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-70① | 中高 | **TextArea 输入路径补纵向光标跟随** | 用户实测：方向键移动光标会自动滚动跟随，但**输入内容（含回车）不触发滚动**。源码核实：TextInput 分支（TextArea.cpp:645-652）`insertText(filtered); ensureCursorHorizontalVisible();` ——**缺 `ensureCursorVisible()`**（方向键/回车路径均已有调用）→ 打字换行后光标移出可视区不跟随。修复：TextInput 分支补 `ensureCursorVisible()`（顺带请核对回车路径实测不跟随的原因——代码路径已有调用，疑被后续 `updateVScrollBar` 覆盖或顺序问题） |
| P0-70② | 中高 | **resourceProviders 目录挂载语义（或确认单文件语义）** | 设计器资源面板按用户 UX 实现为"名称 + **目录**选择"（SHBrowseForFolder）；但引擎语义（ResourceProvider_Design §4.1）= `name → 单文件路径`（引用 `provider:<name>`，readFile 精确键匹配 `paths.find(name)`——**不支持目录/子路径**）→ 目录条目 `provider:main` 会读到目录路径失败。建议二选一：**(a) 引擎扩展目录挂载**——`name → dir` + `provider:name/rel.ext` 前缀解析（最长挂载名匹配 → delegate->readFile(dir + "/" + rest) + 懒加载缓存）；**(b) 保持单文件语义** → 设计器改为文件选择对话框（每条目=一个资源文件）。设计器倾向 (a)（与用户"选目录"诉求一致，列本项） |
| P0-70③ | 低 | （可选）ListView 长文本查看体验 | 资源列表路径列宽受限（240 面板），引擎无列宽拖拽/单元格 hover tooltip → 设计器过渡：**选中行全路径显示于状态栏** + 列宽加宽（80/160）。引擎可后置（列宽拖拽/hover tooltip），不阻塞 |

> **P0-70 状态（2026-10-06）**：**已闭环**——引擎实施并同步（① TextInput 防御性补 `ensureCursorVisible`（核实 insertText 已调用——我方误报更正）；② **目录挂载 (a) 采纳**：最长挂载名前缀解析 + `..`/盘符拒绝 + `\`→`/` 归一化 + 懒加载缓存）；设计器随批：资源面板**文件/目录双按钮**（`选择文件…`/`选择目录…`——两种形态并存）、`isPathLikeProp` 补 `"path"`（Animation 资源引用读回兜底，用户已验证 ✓）、动态区 TextArea 显式 `auto-scroll=false`（P0-68 随批）。

## 追加（2026-10-05 menus/资源联测——引擎清单 P0-71）

**背景**：P0-69/P0-70 联测（用户实测）：

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-71① | 中高 | **MenuBar 弹层定位：视口内坐标域错（缺陷）** | 现象：画布内 MenuBar 创建菜单后弹层位置不在主菜单项下方（设计器修复实例归属后仍偏——偏移量=画布视口原点）。根因：`MenuBar::attachMenuPanel`（Menu.cpp:1147-1170）用 `getDrawRect()`（**递归全局域**，ControlBase.cpp:640-651）设置面板 rect，但面板父级为**实例 bench**——bench 子坐标是**局部域**；主实例 bench 恒 (0,0) 无差，**子视口 bench 有窗口偏移 → 弹层整体偏移**。建议：以 bench 局部坐标定位（如菜单栏 `getRect()`（bench 局部）+ hitRect×（栏复合/bench 复合），或用既有 `mapToDrawRect`/`mapViewportToCanvas` 映射helper），并回归主实例菜单栏位置不变 |
| P0-71② | 低 | （可选/确认）LuotiAni 资源引用路径读回返回 0 | `getStringProperty("path"/"animation")`：`m_filePathStr.empty()`（**资源引用不存**）→ 返回 0（引擎既定语义"资源引用不读回"）。设计器已用**模型 extended 兜底**修复读回显示（随批：`isPathLikeProp` 补 `"path"`）——若引擎愿意让 provider 引用也可读回（存原值串）更佳；不阻塞（设计器兜底已足） |

> **P0-71 状态（2026-10-07）**：**已闭环**——引擎实施并同步（① `attachMenuPanel` 改 **bench 局部域**定位（`mapViewportToCanvas` + `/benchScale`，与 computeTargetRect 同公式；主实例逐位等价）；**ContextMenu 同根因同批修复**（Q-1）；② 维持"资源引用不读回"（§3.1 导出/加载链核实：设计器导出基于自身模型、应用加载读 JSON 原值——无新增动作）。设计器验证：重建 + 冒烟/自测 24/0/矩阵/strict 全绿；画布内菜单弹层对齐待用户真机复测（主实例回归同测）。

## 追加（2026-10-07 菜单联测——引擎清单 P0-72）

**背景**：P0-69 菜单编辑器联测（用户实测四项）——逐项源码核实后列本批：

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-72① | 中 | **菜单面板阴影半径可调（属性暴露）** | `MenuPanel::m_shadowRadius = MenuColors::PANEL_SHADOW_BLUR`（Menu.h:245）为常量初值，**无属性入口**（`setFloatProperty` 仅 `shadow-offset-x/y`，Menu.cpp:699-711）→ 用户实测"面板阴影太宽"无法调整。需求：暴露 `shadow-radius`（float，setFloat/getFloatProperty）+ Binding（通用 SetFloat 直通）+ JSON 键 + schema |
| P0-72② | 中 | **主菜单激活（pressed）背景色在 schema 层不可配** | 引擎运行时可写（`m_activeBgColor ← kStatePressed`，Menu.cpp:1289 ✓），但 schema `colors-menu`（declarative-ui.schema.json `$defs/colors-menu`）为 **state-color-3d（N/H/D 三态）** → 属性面板无 pressed 行 → "点击选中暗蓝色"不可调。建议：menu-bar 的 colors 改含 pressed 的集合（如 `state-color-4` 或 bar 专用 def——注意 colors-menu 现为 bar/panel/item 共享，panel/item 无 pressed 语义，需拆分或仅 bar 换 4 态） |
| P0-72③ | 中 | **`MenuBar::addMenu` 时向新面板传播 bar 当前颜色/字体（缺陷）** | `setColorProperty`（Menu.cpp:1298-1303）与 `setFontSize`（:1251-1253）仅向**既有** `m_entries` 面板传播；`addMenu` 新建面板不回放 → 设计器结构化重建菜单后面板回默认深灰/暗蓝（用户实测）。建议：addMenu/attach 时把 bar 当前 11 个颜色键 + font-size + font 应用到新面板。**设计器已加过渡**（syncMenus 新建面板后回放——引擎修复后删） |
| P0-72④ | 中 | **菜单项级状态颜色（用户建议，对齐 StatusBar）** | 现状 MenuPanel 颜色全部为**控件级**（N/H/D）；`item-id` 定位链仅覆盖 caption/shortcut/checked/enabled/leading-gap 等——**无 item 级颜色属性** → 无法单独控制某项（如未激活项）。建议（对齐 StatusBar 统一状态色控制）：item 级颜色属性（如 `item-text`/`item-text-hover`/`item-text-disabled`/`item-background`… 或统一 `item-colors` 对象）+ `item-id` 定位 + JSON items 键（`text-color` 字符串/三态对象——与 statusbar/tree 家族一致）+ 绘制回退链（item 显式 → 面板 → bar）+ Binding；设计器随批：menus 行格式扩展项级颜色 attrs |

## 追加（2026-10-07 预览联测——引擎清单 P0-74）

**背景**：预览独立窗体联测（用户五项反馈）——逐项探针/源码核实后，本批为剩余引擎侧缺口（设计器侧已修：预览实例窗口句柄混用、图片/动画放置默认值入模型）：

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-74① | 中 | **`viewport` JSON 键补背景色（Bench 编辑需求，含透明）** | 用户需求：点击画布空白=选中 Bench，可改 背景色/大小/缩放模式/窗口标题。现状：`viewport` 键仅 width/height/scale-mode（LayoutParser.cpp:53-74）；实例级背景色仅 C ABI `SetViewportBackgroundColor`（无 JSON 键）→ 导出后预览/应用无法携带画布底色（预览现为黑色）。需求：`viewport.background`（色，字符串）解析 → `ctx->viewportBackground`；schema viewport 描述同步。**补充（透明要求）**：游戏场景 UI 需叠加在游戏画面上——背景色必须支持 **alpha=0（透明）**，语义 = **不填充/不清屏**（保留底层游戏帧，UI 透出）。渲染侧已正确：`UICornerstone_Render` 仅 `alpha>0` 时填充视口、缺省 `SColor(0,0,0,0)` 跳过（src/UICornerstoneAPI.cpp:927-930、include/UIContext.h:61）——JSON 解析/校验只需覆盖透明值（`#RRGGBB00` / `{r,g,b,a}` a=0）并在 schema 注明 alpha 语义 |
| P0-74② | 中 | **TreeView schema 补 `hover`/`selected` 键** | 引擎运行时已有（P0-64：kTreeHover/kTreeSelected，行 hover/选中色）但 **schema tree-view def 未声明** → 设计器属性面板无对应行 → 行 hover/选中色不可编辑（对比 list-view schema 已含 `hover` ✓）。需求：tree-view def 补 `hover`/`selected`（$ref 色 def）+ JSON 解析键 + schema 描述 |
| P0-74③ | 低 | **缩放模式运行时 API（Bench 编辑需求，可选）** | 设计器 Bench 编辑"缩放模式"需运行时切换视口 scale-mode（布局期 `viewport.scale-mode` ✓；运行期仅 Bench::setViewportScaleMode 内部）——若绑定暴露（如 `SetViewportScaleMode`）设计器可直接接线；或经 SetString 属性通道（引擎定形） |
| P0-74④ | 高 | **`colors` 部分对象语义：须合并而非整体替换（用户"StatusBar 背景变黑"根因）** | 复现：布局 JSON 中状态栏 `colors: { background: { hover: "#00FF00FF" } }`（仅 hover，schema `state-color-4` 各态均非 required、部分对象合法）→ 预览实例状态栏 **背景变黑**（23,23,24），构造缺省 VSCode 蓝 (0,122,204) 丢失。根因：`parseCommonProperties`（LayoutParser.cpp:2791-2810）对每组合调用 `parseStateColor`（:3408）**构造新 StateColor**——JSON 未给出的状态取通用缺省（StateColor 构造 = DEFAULT_NORMAL_COLOR 黑），控件现值/构造缺省被整体覆盖。需求（实现方式由引擎定夺，建议）：解析时在**控件当前状态色**上合并——仅覆盖 JSON 显式给出的状态键（normal/hover/pressed/disabled），其余保持现值；`getBackgroundStateColor`/`getBorderStateColor`/`getTextStateColor`/`getTextShadowStateColor` 已有读回接口可用。范围：`parseCommonProperties` 通用路径（全控件）；button/winframe 等特化路径（:970/:1650-1664）存在同类语义，是否一并合并请引擎评估。证据：设计器 auto=3 预览探针（仅设 hover → 期望 normal 保持构造缺省蓝；当前为黑） |
| P0-74⑤ | 中 | **状态栏悬停反馈色来源澄清/可配（用户"hover 变蓝"）** | 现象：用户配置状态栏悬停色后预览仍见蓝色悬停。现状：段悬停反馈（无显式段 hover 背景时）用**控件级 `m_hoverColor`**（StatusBar.cpp:289-301；缺省蓝，仅 `SetColor("hover")` 可设）——`colors.background.hover` 只作用于条自身背景态，被段叠加覆盖；且 `hover` 键 **schema/JSON 均未声明**（LayoutParser 无 kTreeHover 解析）。需求（引擎定夺）：① schema status-bar 补 `hover` 键 + JSON 解析；或 ② 段悬停回退链改含 `colors.background.hover`（段显式 hb → 控件 background.hover → `hover` 键 → 缺省）；设计器侧将随引擎定形暴露对应属性行/行式 attr |
| P0-74⑥ | 低 | **`viewport.id` 键（画布 Bench 代码引用）** | 设计器 Bench 编辑已支持"画布 id"行并随导出携带 `viewport.id`（代码引用语义，同控件 id 行）；引擎当前忽略该键。需求：解析 `viewport.id` → `SetControlId(bench, id)`（或等价注册）→ 宿主代码可 `FindControl(id)` 定位根画布；schema viewport 描述同步 |

> **P0-74 状态（2026-10-08）**：**已闭环**（①~⑤）——引擎实施并同步（release 10-08 15:00；subModules DLL/schema/头文件一致）：① `viewport.background` 含 alpha=0 透明语义（LayoutParser.cpp:67-76）；② tree-view `hover`/`selected` 解析 + `getColorProperty` 读回（TreeView.cpp:1209）；③ 非缺口确认；④ colors **逐键合并经整态 setter 写回**（保转发——button caption/winframe title 特化路径同步整改）+ 字符串色分支；⑤ statusbar `hover` schema/解析（LayoutParser.cpp:1473）。设计器联测：auto=2 **45/0**、auto=3 四项预览探针 **OK**（部分对象语义/画布背景像素/图片/动画内容——含 provider 引用经导出挂载加载）、strict 校验 **0 error**、矩阵 **PASS**。**⑥ 为新增项（画布 id 代码引用），待引擎评估。**

## 追加（2026-10-08 预览联测二轮——引擎清单 P0-75）

**背景**：用户预览态五项复测——#1/#2/#5 已由设计器修复并探针验证（画布背景纸面范围、预览浮层通道、provider 链）；#3/#4（树/状态栏段 `hb#` 悬停背景）经源码核实为**引擎 JSON 解析缺口**：

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-75 | 中 | **tree-view / status-bar items 支持独立 `hover-background-color` 键（对齐 list-view cell / menu item）** | 现状：ListView cells（LayoutParser.cpp:738）与菜单项（:1866）均解析**独立 `hover-background-color`** 键；tree-view（:2481-2505）与 status-bar（:1534-1545）**仅支持 `background-color` 对象形态 {normal,hover}** → 设计器模型/行式格式（hb#）以独立键表达，编辑态经 CABI 正常、预览/JSON 加载该键被忽略（用户实测"编辑态 OK、预览不生效"根因）。需求：两控件 items 解析补独立键（与同族一致）+ schema 描述补该键（status-bar items 属性表当前无此键）。**设计器过渡已实施**：导出时树/状态栏项 hb# 归一为 `background-color:{normal?,hover}` 对象形态 + 导入拆回独立键（auto=2 断言覆盖）——引擎补齐独立键后可移除过渡（或两者并存均可） |

## 追加（2026-10-08 预览联测三轮——引擎清单 P0-76~78）

**背景**：用户按实测配置（Bench 灰 #C0C0C0FF / 菜单 pressed / provider:test 引用）复测——设计器侧修复（画布纸面范围、预览浮层通道、树/状态栏 hb# 对象形态、menus r# 往返、provider 探针+字体注册绕过）已实施并探针验证；以下三项为**引擎根因**：

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-76 | 高 | **P0-74④ 合并路径回归：颜色须逐键经 `setColorProperty`（子类影子成员不可达）** | 复现：菜单栏 `colors.background.pressed` 导出正常，预览解析后读回仍为缺省（用户问题 2）。根因：P0-74④ 合并"经整态 setter 写回"（`setBackgroundStateColor` 等）——MenuBar 颜色成员为**子类影子**（include/Menu.h:354-358 m_bgColor/m_activeBgColor/m_textColor…），基类整态 setter 写基类成员，`MenuBar::setColorProperty` override（kStatePressed→m_activeBgColor）不可达 ✗；StatusBar（m_textColor 影子）等同族同样受影响。需求：合并逐键经 `setColorProperty(键, 色)`（子类 override 可达；字符串=normal）；整态 setter 仅保留确实需要整态的场景。设计器绕过：预览加载后 `ReplayMenuBarColorsTo`（逐态 SetColor 回放模型颜色）——引擎修复后移除 |
| P0-77 | 高 | **Bench 默认不透明 → `viewport.background` 不可见 + alpha=0 透明/游戏叠加失效（用户问题 1 根因）** | 复现：`viewport.background` 任意色（含 #C0C0C0FF）预览均显示 bench 缺省深色 (23,23,24)。根因：`Bench::initial()`（src/Bench.cpp:10）`setTransparent(false)` 覆盖构造的 `setTransparent(true)`（:31）→ bench 绘制自身不透明背景，盖住 `UICornerstone_Render` 的视口背景填充（API:927-930）✗；同时 **alpha=0 透明/游戏画面透出（用户原始需求）亦不成立**（bench 不透出）。需求：bench 默认透明（与构造意图一致）/或明确"视口背景填充 vs bench 背景"层次（引擎定夺）；透明叠加场景须保证 bench 不遮挡底层游戏帧。设计器侧无绕过（无法触达 bench 句柄——建议同步提供 bench 透明/背景 API 或 `GetRoot` 绑定暴露） |
| P0-77① | 中 | **菜单面板阴影禁用语义（用户问题 3）** | `r#0`（阴影半径 0）当前仍绘制"无模糊但带偏移"的阴影（Menu.cpp:864-884 中 m_shadowRadius 仅作 blur）→ 无法去掉阴影；`m_shadowEnabled`（kShadow）仅存储、未参与 drawShadow 调用判断（draw():892 恒调）。需求：明确禁用路径（建议 kShadow=false → 跳过 drawShadow；或 radius 0 → 不绘制）——引擎定夺。设计器 menus 行式已支持 `r#0` 往返（hasRadius 显式位，模型/导出保留） |
> **评审状态（2026-10-08）**：引擎已出两份设计——`design/P075_ResourcePipeline_Design.md`（P0-74⑥/P0-75/P0-78/P0-79）与 `design/P076_ColorMerge_BenchTransparency_Design.md`（P0-76/P0-77/P0-77①）——**我方复核均通过放行**（`Temp/P075_..._复核意见.md`、`Temp/P076_..._复核意见.md`）：根因核实与设计器实测一致；Q 项全部答复（P076 Q-2 明确 CheckBox 需补单态转发——设计器经内部 caption label 读回，不对称即回归）。落地后设计器回收 6 项绕过（探针/字体注册/帧注册/动画路径解析/菜单颜色回放/导出对象形态归一）+ 联测回归。
>
> **实施联测状态（2026-10-09）**：引擎实施并同步 subModules（release 10-09 07:46）——设计器重建联测：**P0-74⑥/P0-75/P0-76/P0-77/P0-77①/P0-78/P0-79 主体全部生效**（auto=3 五探针全 OK：bench bg 像素 / 菜单 pressed 经解析 / 图片 provider / 动画 provider 描述 `playing=1 loop=1 total=60`；导出切独立 hover 键 + 旧对象形态导入兼容；fonts 经 delegate 回退）。设计器已回收：探针注册、帧注册、菜单颜色回放、编辑态动画路径解析、导出对象形态归一（导入侧保留）。**联测发现两项引擎遗留**（见 P0-80/P0-81）：
>
> | # | 优先级 | 需求 | 说明 |
> |---|---|---|---|
> | P0-80 | 高 | **绑定内存 provider 替换实例 provider 时丢失文件系统回退（P0-78 场景遗漏）** | 复现：设计器启动注册网格资源（`RegisterResource`）→ 绑定 `ensureMemoryProvider` → C ABI `createMemoryResourceProvider` = `bridge: new MemoryResourceProvider`（**无 setFilesystemFallback**，BackendBridge.h:298）→ `setResourceProvider` 替换 MainWindow 的（带回退）provider → 此后实例内 `fonts/...` 等相对键资源全部读不到（Label 字体 ×71 "not found"）。需求：`setResourceProvider`（或 createMemoryResourceProvider 的调用点）在替换时继承/补齐 `ctx->resourceRoot` 文件系统回退（一行：`if (auto* mem=dynamic_cast<MemoryResourceProvider*>(h)) if (!mem->hasFallback()) mem->setFilesystemFallback(ctx->resourceRoot);`）。设计器绕过：网格注册后显式注册字体（registerFonts，已恢复） |
> | P0-81 | 高 | **多实例 LoadLayout 解析期控件 provider 作用域（全局 GET_CONTEXT 陈旧）** | 复现：设计器预览（第二/第三实例）LoadLayout → 动画 `path="provider:test/bombBlock.jsonc"` 解析期 `loadAniDesc(string)` → `getResourceProvider()` 取到**全局 GET_CONTEXT** 的 provider（非目标实例——无该实例的 mounts）→ `'test/bombBlock.jsonc' not found`；挂树后延迟补读 → `No resource provider` → `[Instance_N] layout animation prepare failed`（动画空白）。Actor 图片因两阶段（挂树后 create 才加载）侥幸避开；动画解析期即加载 ✗。需求：解析期控件 context/provider 绑定目标实例（parser 显式 `setContext(target)` 或 C ABI LoadLayout 期间作用域化全局上下文）。设计器绕过：预览布局副本 `ResolvePreviewAnimationPaths`（动画 provider 路径→文件路径，引擎修复后可删） |
>
## 追加（2026-10-09 深度联测——引擎清单 P0-84）

> **P0-83/84 设计评审（2026-10-09）**：引擎已出 `design/P083_084_Design.md`，我方复核**通过放行**（`Temp/P083_084_Design_复核意见.md`）：P0-83（EditBox read-only）直接放行；P0-84 的 a) Release 所有权校验 / b) 焦点归属路由 / c) 诊断 API（Release 可用）均建议采纳；**Q-4 答复=owner 创建**（证据链：App.cpp:392→CanvasPane.cpp:287 m_ui 为主实例；容器 App.cpp:229 propDynamic 主实例；属性区无 m_viewport->Create；Debug 无跨实例断言）→ vp 假设未成立，附"复现差异"（TabControl 页 + 控件树 + 池化 EditBox 拓扑）请引擎收紧探针继续定位；c) 为定案关键工具（设计器将在 Release 行构建自检）。 

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-84 | 高 | **画布（Bench）选中态下属性面板 EditBox 无法输入（TextInput 路由/焦点）** | **复现（用户实测）**：设计器 → 选中画布（画布空白点击或控件树"画布"节点）→ 点击属性栏"画布 id"输入框（**聚焦成功：焦点边框/光标可见** ✓）→ **输入任何字符都不显示、状态栏无提示**（text-changed 未达 ✗）。**对照**：同面板、同控件类型的控件行（如"控件 id"行）输入正常 ✓（同一次运行内）。**设计器侧已核实**：行集构建/可见/启用/位置/取值全部正确（自测读回断言 ✓）；行与控件行为同一 `CreateEditBox` 产物 + 同容器。**引擎侧候选**（源码）：①`UICornerstone` 主循环 TextInput/键盘路由 `kbdTarget = instance->activeViewport ? activeViewport : instance`（src/UICornerstoneAPI.cpp:839-847）——画布=子视口（设计器 CanvasPane 子视口），若用户先前点击过画布使 `activeViewport=画布子视口`，且其后点击 owner 区域（属性面板）**未能清除** `activeViewport`（`dispatchMouseByRouting` :341-345/:353-357 的清除分支），则 TextInput 全部发往画布子视口、owner 面板的已聚焦 EditBox 收不到 ✗；②`EditBox::setText` 程序化不发事件（:568-574，与 insertText :209-226 的差异为预期）——设计器自测无法据此验回调，仅记录。需求：引擎复现并修复（建议：TextInput/键盘路由改为"当前实例的聚焦控件归属"判定，或 owner 区域点击可靠清除 `activeViewport`；并补输入注入回归）。**复测更新（用户）**：①冷启动（不经画布点击）直选画布 → 内联输入仍失败（**排除 activeViewport**）；②**画布设置对话框可用** ✓；③**A/B 探针大发现**：画布 id 行改为复用控件 id 行的同一池控件（"edit:id"）后，**控件 id 行也随之失效** ✗ —— 即"**由画布构建期创建的 EditBox 是坏的**"，被复用后把好的场景也带坏。**线索收敛**：坏控件=**C ABI 以 (0,0) 创建 + SetRect 重定位**（画布行原实现）；好控件=**按最终位置创建**（控件行/对话框）；对照：多行 TextArea 以 (0,0) 创建后 SetRect **工作正常**（caption 行）→ 疑似 **EditBox 特有的"创建位置/重定位"输入通路问题**（点击/焦点/光标均正常，仅 TextInput 不达）。设计器已改为**按位置创建**画布行——复测**仍不能输入** ✗ → **(0,0) 创建/重定位线索一并排除** ✗。**最终收敛证据**（同一运行、同一面板、同一 `rebuildDynamicRows` 调用路径、同一 `CreateEditBox` API、同一容器）：**控件选中 → 控件 id 行（EditBox）输入正常** ✓；**画布选中 → 画布 id/标题行（EditBox）输入无效** ✗（点击/焦点/光标正常，TextInput 不达）；**画布设置对话框（浮层）输入正常** ✓；对照多行 TextArea 各场景均正常 ✓。即"**同一构建路径下、因选中对象不同而输入通路失效**"——唯一变量=选中态（控件 vs 画布/Bench）。设计器暂无进一步可排点；请引擎以输入注入 + 事件追踪（bench 选中 vs 控件选中两个场景）定位（如 FocusManager/事件队列对该状态下新建控件的处理差异）。设计器过渡：画布 id/标题内联行改**只读值显示** + "画布设置…"对话框统一编辑（对话框通道已证可用）。 |

> **状态（2026-10-10）**：引擎已完成问题分析（`UIControls/design/P086_087_Design.md`）；设计器复核**通过**（`Temp/P086_087_复核意见.md`）。**引擎已实施并同步**（DLL md5 `abab828b…` 与 release 一致；binding 空回调分支先于 slot ✓）：**P0-86/P0-87 闭环**——设计器随批：`removeEventBinding` 改回空回调解绑；auto=1 探针 `miUndo valid=1 disabled=1 miRedo disabled=1 → MENU-STATE-OK`；auto=2 82/0、五探针、strict、Release 零警告。

> **状态（2026-10-10）**：引擎已完成问题分析（`UIControls/design/P088_MenuCloseOwner_Design.md`）；设计器复核**通过**（`Temp/P088_复核意见.md`：3 点非阻塞补充——2.2 兜底双段可简化 / 测试补右键上下文菜单+≥2 级嵌套子菜单 / active-index 只读钩子与 isInMenuMode 断言）。待引擎实施并同步。

## 追加（2026-10-10 菜单点击后主菜单残留激活态——引擎清单 P0-88）

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-88 | 高 | **菜单项点击后 MenuBar 状态未复位（`m_activeIndex`/`m_menuMode` 残留）** | 用户实测：点击菜单项（如 编辑→撤销）后，下拉面板收起 ✓，但**主菜单标题保持"点击态"底色**，且菜单悬停切换模式仍激活——必须再点击一次屏幕退出该状态才能正常操作。根因（引擎源码）：`MenuItem::closeMenuChain()`（Menu.cpp:250-265）**靠父链向上查找 MenuBar**；而 `MenuBar::attachMenuPanel`（:1246-1249，P0-71①）为取得根复合把**面板父级挂到 bench**（`entry.panel->setParent(bench)`）→ 面板打开态下父链为 item→panel→bench→(null)，**永远到不了 MenuBar** → `bar->closeAllMenus()` / `bar->exitMenuMode()` 不执行 ✗。次生影响：面板滞留 bench 下（未 detach，父链未还原）；`exitMenuMode` 才做的"父级还原 + `m_activeIndex=-1` + `m_menuMode=false` + `m_hoveredIndex=-1`"全部未发生；点击别处由 before-handling watcher → `exitMenuMode` 补做（即"再点一次才能操作"现象）。需求：**关闭路径可靠触达所属 MenuBar**——建议方案 A：`MenuPanel` 增 `m_ownerBar` 背指针（`MenuBar::addMenu` 时设置；子菜单面板经 `MenuItem::setSubMenu` 从父面板传播 owner）→ `closeMenuChain` 父链查找失败时以 owner 兜底执行 `closeAllMenus()+exitMenuMode()`（或直接统一改走 owner，父链保留兼容离线单测路径）；方案 B（次选）：`MenuPanel::hide/closeWithChildren` 通知 owner bar（须防 `closeAllMenus→closeWithChildren→hide→通知` 递归，例如仅在"面板原为可见 + bar 的 activeIndex 对应该面板"时通知）。验收：①点击菜单项（含子菜单项）→ 面板立即收起 **且主菜单标题立即恢复非激活态**、`m_menuMode=false`（不再需要补一次点击）；②悬停跨菜单切换（P0-72/P0-82）、子菜单链关闭、面板定位（P0-71① bench 挂载语义）不回归；③建议补回归用例：合成 MouseDown 点击项后断言 `m_activeIndex==-1 && m_menuMode==false`（可选补 `getIntProperty("active-index")` 只读钩子便于断言）。设计器侧无需改动；同步后手动复测并回归 auto=3 菜单探针。 |

## 追加（2026-10-10 菜单动态置灰——引擎清单 P0-87）

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-87 | 中 | **菜单项按 JSON `id` 注册进 id 表（`FindControl` 可达）** | 需求：设计器"编辑→撤销/重做"应随栈状态动态置灰。现状：`LayoutParser::populateMenuPanel` 已解析 item `"id"`（`setItemId`，LayoutParser.cpp:1872）但**未注册** `m_controlsById`（其余控件类型逐类注册，如 1180/1250/1350…）→ `UICornerstone_FindControl("miUndo")` 返回空。补齐后即可：`SetBool(item, "enabled", 0/1)`——**属性派发已通**（ControlBase.cpp:983 kEnabled→setEnable 虚函数，MenuItem 已重写；`MenuItem::handleEvent` 已检查 `getEnable()` 自动忽略交互；状态色 disabled 生效）。设计器侧已就绪：`layouts/main_layout.json` 菜单项已带 `"id": "miUndo"/"miRedo"` + `App::refreshMenuStates()` 按 `CanUndo/CanRedo` 变化写 enabled（未注册前 FindControl 无效=无操作，零副作用）。验证：auto=1 打印 `miUndo valid=1`；撤销栈空时菜单项灰显且点击忽略 |

## 追加（2026-10-10 事件面板联测——引擎清单 P0-86）

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-86 | 中 | **绑定层 `Control::SetCallback(event, 空回调)` 应等价解绑（防 bad_function_call 崩）** | 复现（用户实测）：打开项目选中按钮即报 VC 运行库异常。根因：绑定层 `Control::SetCallback`（binding/src/Control.cpp:109-121）对**空 `std::function`** 仍注册 `CallbackThunk` + 引擎侧回调 → 事件触发（如 click）时 thunk 调用空函数 → **`std::bad_function_call`** ✗（设计器已自修：不再用空回调解绑、改 no-op）。需求：绑定层判定空回调 → 传 `nullptr` 给 C ABI（引擎语义=解绑）或注册内部空实现；并建议 `SetCallback` 语义文档注明。设计器侧已自防（no-op 解绑），本项为绑定健壮性加固 |

## 追加（2026-10-10 深度联测——引擎清单 P0-83）

> **P0-85 状态（2026-10-10）**：**已实施并同步**（release 10-10 18:35，md5 核验一致）——`ComboBox::handleEvent` 非编辑 TextInput 改 `return getFocused()`（与复核一致）；设计器重建全量回归：auto=2 **54/0**、auto=3 五探针全 OK、越界错误 0、strict 0 error、矩阵 PASS、Debug/Release 零警告。**用户复测通过** ✓（画布行内联 id/标题直编 ✓ + 缩放模式下拉回归 ✓）——**P0-84 全链结案**（手柄跨实例修复 / a) Release 校验 / b) 焦点归属路由 / c) Debug_ValidateHandle 长期自检 / P0-85 ComboBox 门控，全部在册闭环）。
>
> **P0-85 设计评审（2026-10-10）**：引擎已出 `design/P085_ComboBoxTextInput_Design.md`——**P0-84 根因定案：`ComboBox` 非编辑模式无条件吞 TextInput**（逆序子遍历下，同容器位于聚焦 EditBox 之后的默认 ComboBox 先吞 ✗）；与设计器全部实测吻合（画布行集 ComboBox 在 EditBox 之后 ✗ / 控件行 ComboBox 在 id 行之前 ✓ / 对话框无 ComboBox ✓ / 池复用顺序改变=传染 ✓）。我方复核**通过放行**（`Temp/P085_ComboBoxTextInput_Design_复核意见.md`）：修法 `return getFocused()` 确认；仅 ComboBox 需修（同类审计一致）；测试矩阵确认；P0-84 收尾确认（a/b/c 硬化与 `Debug_ValidateHandle` 长期自检保留）。落地后画布行内联直编即通、P0-84 结案。

> **P0-84 复测更新（2026-10-10，手柄修复后）**：用户复测：**画布内联 id/标题输入仍失效** ✗；手柄拖拽/缩放正常 ✓；控件行正常 ✓。**引擎新 a) 校验立了大功**：抓到设计器侧真实越界（选中手柄经主实例创建/操作、目标为子视口控件——Release 静默容忍）→ 设计器已修正全部 5 处至 `m_viewport`，越界日志清零 ✓；**但内联输入仍失效** → 手柄非唯一根因。**收敛后的最终证据（请引擎插桩定位）**：
> - 焦点在行上（光标/焦点样式可见 ✓、行控件 owner 归属经 c) 校验 ✓、TextInput 事件未达回调——无字符、无状态提示）；
> - 唯一实测差异 = 选中态（控件行 ✓ / 画布行 ✗），两行**同容器、同创建 API、同回调结构**（代码级逐行对比无差异）；
> - **未被探针覆盖的拓扑要素**：①属性面板位于 **TabControl 页**内（`TabControl::handleEvent` 仅拦 KeyDown，源码看不应拦 TextInput，但请实测）；②画布 = **子视口** + 画布控件树（TreeView 选中路径）；③行控件经**行池**跨选中复用重绑。
> - **建议引擎**：以"owner + 子视口 + TabControl（两页）+ 页内 EditBox + TreeView 选中流"的最小拓扑做输入注入复现；或对设计器 exe 做一次临时插桩（SDL_EVENT_TEXT_INPUT → kbdTarget → dispatchToBench → 队列 → handleEvent 链 → EditBox m_focused/insertText）逐环打点。设计器过渡保持"对话框编辑 + 内联回显"。

> **P0-83/84 状态（2026-10-10）**：**已实施并同步**（release 10-10 12:39）——P0-83 EditBox `read-only`（守卫：TextInput/退格/删除/粘贴剪切 + 保留选择/复制/滚动 + JSON/schema ✓）；P0-84 三项：a) Release 所有权校验 ✓（`UI_LOGE`+拒绝）、b) 键盘/文本输入**焦点归属路由** `keyboardTarget` ✓、c) **`Debug_ValidateHandle`（Release 可用 + 绑定暴露）** ✓。**重大收获：新校验立即抓到设计器侧真实越界**——**选中手柄（HandleControl）此前用主实例 `m_ui` 创建/操作，目标却是子视口控件**（Release 静默容忍；Debug 断言）→ 设计器已修正全部 5 处到 `m_viewport`（CreateHandleControl/SetHandleRectFilter/SetHandleTarget×2/HandleHitTest）→ **越界错误清零**（auto=2/3 日志 0 条），自测 54/0、五探针全 OK、strict/矩阵 PASS、Debug/Release 零警告。该跨实例手柄状态极可能就是用户"画布选中态输入失效"的根因（手柄在 main 创建/target 在 vp，focus/输入通路不一致）；画布内联 id/标题行已恢复直编，待用户复测；另：资源弹窗路径框已切**真只读**（P0-83），画布值行随内联恢复删除只读过渡。

## 追加（2026-10-09 深度联测——引擎清单 P0-83）

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-83 | 低 | **EditBox 支持只读（`read-only` 属性）** | 用户需求：资源弹窗路径显示用"只读 EditBox"（长路径内部滚动不越界、内容不可误改）。现状：schema/引擎仅 NumericUpDown 有 `read-only`（LayoutParser.cpp:2378），EditBox 无 → 设计器以"伪只读"过渡（text-changed 即回退 + 防回环标志）。需求：edit-box def 增 `read-only`（bool，缺省 false）+ 解析 + 运行时（禁编辑/光标插入，保留滚动/选择/复制）+ schema/docs 同步；设计器随批切换真只读并删伪只读 |

## 追加（2026-10-09 预览联测——引擎清单 P0-82）

**背景**：用户实测"主菜单的 `background.pressed` 色在鼠标移走后仍然保持"——设计器侧为纯引擎渲染/状态机（菜单栏无设计器干预；P0-76 逐键合并已确认生效，色值可达 `m_activeBgColor`/`m_hoverBgColor`）。我方已做源码级排查（候选机制）：

| # | 优先级 | 需求 | 说明 |
|---|---|---|---|
| P0-82 | 中 | **菜单栏项状态色残留：展开后移向相邻菜单，"来源项"仍保持底色（用户精确复现）** | **复现步骤（用户实测）**：点击"文件"展开（面板显示）→ 将鼠标从"文件"移到"帮助" → **"文件"仍保持 background.pressed 底色**（应移到"帮助"激活色、"文件"恢复常态）。**根因（源码定位）**：`MenuBar::handleEvent` 的 MouseMove 分支（Menu.cpp:1593-1601）在 `m_menuMode`（展开中）下**只做 `switchMenu`、不更新 `m_hoveredIndex`**（`m_hoveredIndex = index` 仅非菜单模式执行 ✗）——而点击前悬停"文件"已置 `m_hoveredIndex=0`，其后：①若 `switchMenu(1)` 成功：`m_activeIndex=1`（"帮助"激活 ✓）但 `m_hoveredIndex` 仍为 0 → 绘制 `else if (i==m_hoveredIndex) fill(m_hoverBgColor)`（:1530）→ **"文件"残留悬停底色** ✗；②若 switch 未达（面板/兄弟消费 Move 等）→ "文件"保持 `m_activeIndex` → **残留激活（pressed）底色** ✗——两种均与现象吻合。`MenuBar` 无 `onMouseEnter/onMouseLeave` 覆写（帧级 enter/leave 仅切 setState，不清 per-item `m_hoveredIndex` ✗）。**建议修法**：Move 分支两种模式下均更新 `m_hoveredIndex = index`（菜单模式下先更新再 switchMenu；index=-1 自然清除 ✓）——"文件"→ 非 active 且非 hovered → 恢复常态；或 draw 的 per-item hover 分支增加"bar 处于 hover 态/索引有效"守卫 ✓。**请引擎测试矩阵补**：悬停→移走、展开→移向相邻项（本复现）、展开→移出栏、展开→点外部/Esc、快速移动（`m_hoveredIndex` 读回酌情补 getter/像素断言）；**MenuPanel 项悬停同模式风险**（`hide()` 才清 :461）一并核查。设计器侧无绕过（纯引擎状态机） |

> **P0-82 状态（2026-10-09）**：**已实施并同步**（release 10-09 12:47，md5 核验一致）——实现与复核要求逐条吻合：①Move 分支双模式更新 `m_hoveredIndex` + **条件化消费**（仅菜单模式 return true；非菜单落入 drawRect 判定，兄弟 hover 链路保持）；②`MenuBar::onMouseLeave` 覆写清 per-item 悬停（不退出展开）；③MenuPanel 移出面板清 item 悬停（子菜单优先消费依赖保持）；④`getHoveredIndex()` + `hovered-index` 只读读回。设计器侧：纯引擎修复无代码改动；全量回归通过（auto=3 五探针、auto=2 45/0、auto=1、strict、矩阵、Debug/Release）。**待用户按复现步骤人工复测**（点击"文件"展开 → 移向"帮助" → 来源项恢复常态/目标项激活）。
>
> **P0-80/81 状态（2026-10-09）**：**已闭环**——引擎实施并同步 subModules（release 10-09 09:50；我方复核通过 `Temp/P080_081_ProviderScope_Design_复核意见.md`）。设计器联测：**最后两项绕过已回收**（`registerFonts`=P0-80 绑定 provider 回退继承生效；`ResolvePreviewAnimationPaths`=P0-81 解析期上下文绑定生效——动画 `provider:` 路径直通 `playing=1 loop=1 total=60`、字体零 `not found`）。**至此全部引擎绕过清零（累计 7 项）**。终验：auto=3 五探针全 OK、auto=2 **45/0**、auto=1 clean、strict **0 error**、矩阵 **PASS**、Debug/Release 零警告。
>
> **方案建议（2026-10-08，P0-76~79 整体）**：设计器绕过均为**过渡方案**（已逐项标注"引擎修复后可删"），且部分场景**绕过不可达/不覆盖导出产物**，建议引擎按批实施后由设计器统一回收：
> 1. **必需性排序**：P0-77（bench 不透明——设计器无法触达 bench 句柄；且阻塞"游戏画面透出"原始需求，**导出应用同样受影响**）> P0-79（动画 provider 描述——绕过仅救设计器预览，**导出的应用/游戏加载 provider: 动画仍空白**）≈ P0-78（顶层实例挂载——绕过依赖绑定内存 provider 懒挂载隐式行为，引擎原生加载器场景仍失败）> P0-76（菜单 pressed 合并——同类影子成员问题可能波及其它控件）。
> 2. **建议增强（一并考虑，可替代多项设计器绕过）**：① 内存 provider 增加 **delegate 文件系统回退**（未注册/未挂载键 → resourceRoot 文件系统）——可替代设计器的字体/动画帧显式注册；② 资源加载统一走 provider（动画 desc、四态图等全部资源入口与帧加载同口径）；③ 顶层实例 `MainWindow` 改 MemoryResourceProvider + filesystem delegate（挂载点天然生效）；④ 绑定暴露 `GetRoot` / bench 透明与背景 API（供设计器 Bench 编辑直连，替代视口级填充）。
> 3. **设计器回收清单**（引擎批落地后删除，自测已覆盖）：`RegisterResource` 探针、`RegisterFontsTo`（若 ①落地）、`RegisterAnimationFramesTo`（若 ①②落地）、`ResolvePreviewAnimationPaths` + 编辑态 path 解析（若 ②落地）、`ReplayMenuBarColorsTo`（P0-76 落地）。
>
| P0-79 | 高 | **LuotiAni 的 `provider:` 描述加载经 fopen 直读不走 provider → provider:xxx.jsonc 动画空白（用户"动画不显示"根因）** | 复现：动画 `path="provider:test/bombBlock.jsonc"`，LoadLayout 后动画空白：`GetBool("loop")=0`、`GetBool("playing")=0`（描述从未解析）。根因：`LuotiAni::loadFromFile` 正确分流 `provider:` → `loadFromResource`（LuotiAni.cpp:150-165），但 `loadFromResource` → `loadAniDesc(resourceId)` → **`fopen("test/bombBlock.jsonc")` 直读文件系统**（:167-200）——provider 从未被咨询（`getImageFromResource` 的帧加载有 provider 路径 ✓，**描述加载没有** ✗）→ fopen 失败 throw → 解析期 catch 后动画控件无描述（空白）。需求：`loadAniDesc` 的 `provider:`/资源 ID 路径经 provider readFile 读取（与帧加载同口径）；编辑态同受影响（SetString("path") 失败保留旧动画）。**设计器绕过已实施**：①预览布局副本 `ResolvePreviewAnimationPaths`（provider 路径→实际文件路径，导出保持可移植）；②编辑态 writeStringPropTo 对动画 path 同样解析（引擎侧），属性行显示模型 provider 原值；③帧显式注册（资产相对键/jsonc 相对键双分支，决定性用例验证）。验证：预览 `playing=1 loop=1` + 像素确证（黄色主色 5804px） |
| P0-78 | 高 | **顶层实例 `resourceProviders` 挂载恒被跳过（用户问题 4 根因）** | 复现：顶层实例（WithResourceRoot）LoadLayout 的 `resourceProviders` 挂载静默跳过 → `provider:test/xxx` 引用加载失败（编辑态正常=绑定内存 provider 懒挂载已触发；预览/顶层应用失败）。根因：`UICornerstone_LoadLayout`（API:1047）`dynamic_cast<MemoryResourceProvider*>`——顶层实例 `resourceProvider` 来自 MainWindow（src/MainWindow.cpp:12 `createFilesystem(resourceRoot)`；UIContext.cpp:80）✗ 恒失败。需求：顶层实例资源挂载点须生效（建议 MainWindow 改 MemoryResourceProvider + filesystem delegate——该设计已有；或挂载代码兼容文件系统 provider）。**附带**：内存 provider 对未注册/未挂载键无文件系统回退（字体等内置相对路径键在替换后失效）——设计器绕过：预览实例注册探针触发内存 provider + 显式注册字体（RegisterFontsTo）；引擎若补 delegate 回退更佳 |

> **P0-72 状态（2026-10-08）**：**已闭环**（①~④）——引擎实施并同步（shadow-radius（menus[i]）/ colors-menu.background 四态 / addMenu 状态回放 applyStateToPanel（Menu.cpp:1127/1133）/ item 级颜色）。设计器随批：**syncMenus 手动回放过渡已删**（引擎自动回放）；menus 行式格式扩展（菜单行 panel attrs/项级颜色 attrs/项 id 生成）列为设计器后续项（非阻塞）。

> **P0-66 状态（2026-10-05）**：**已闭环**——引擎实施并同步（① columns `header-font-size`/`header-font-name` 解析；② rows 对象 cell 解析 + 字符串兼容；③ 粒度方法修复附带回归）。设计器侧验证：C ABI 探针（仅阴影单元格 `black=0 / red(shadow)=80` ✓ 常态不填黑）；构建/冒烟/矩阵 PASS。设计器随批无需代码改动（导出映射随"导出 JSON"功能实施）。**流程注意：引擎同步 subModules 后必须先重建设计器**（build 目录 DLL 为构建期拷贝，本次探针曾误用过期副本）。

> **P0-65 状态（2026-10-04）**：**已闭环**——引擎实施并同步（① `CellStyle.hasBg` 显式位 + 门槛 `hasBg && alpha>0`；② 按复核修订实施：`TreeNode.hasHoverBg` 与 `bgMask` 解耦、显式 hover 独立绘制、合并场景（单色+hover API）常态单色不丢失）；设计器过渡已删；构建零警告、冒烟/矩阵 PASS、引擎 test_listview 21+30 断言全过。
