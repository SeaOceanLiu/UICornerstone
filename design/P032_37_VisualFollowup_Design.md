# P032_37_VisualFollowup_Design — 第二轮视觉联测问题确认 + 能力矩阵修正（P0-32 ~ P0-37 + 用户补充要求）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-09-24 第二轮联测 + 2026-09-27 用户补充要求）
> 前置：P0-30 复核补充（基类 pressed 对称切态）已随上批实施；P0-26 矩阵为基础逐格审计
> 状态：**已放行，已实施**（复核：`requirements/P032_37_VisualFollowup_Design_复核意见.md`，2026-09-27 通过 + 2 小确认）
> **实施记录（2026-09-27）**：
> 1. **schema**：新增 `state-color-2`/`colors-winframe`/`colors-input`/`colors-3full`/`colors-3state`/`colors-bg3`/`colors-basic2`，`colors-menu` 统一三态；删除旧 `colors-basic`/`colors-basic1`；输入族 `shadow` 收回（P0-32）；`validate --strict` 三布局 + schema-only 全 PASS。
> 2. **运行时**：LuotiAni::update 链基类（P0-33④）；Slider/ColorPicker/ScrollBar/StatusBar/TabControl/WinFrame 点击路径补 pressed 链；ColorPicker 关闭态 Label 解除禁用 + setState 同步 + 析构回收弹窗 + setRect 重定位（P0-34）；ContextMenu::setRect 传播 panel（P0-37）；ProgressBar 文本三态快照/状态同步/矩形跟随/阴影单态键（P0-35）；MenuBar/MenuPanel `setBackgroundStateColor/setBorderStateColor` 对象路径映射（三态 N/H/D）+ 禁用背景/边框成员 + 绘制态解析（P0-33③，菜单统一三态）；WinFrame bg→ClientPanel / border→整体 / font 名 + font-size 转发；Button/CheckBox font 名分发 + ApplyFontToControl 补三型分支；TabControl `page-background` 四态转发所有页。
> 3. **item 级四态**（最大新增）：TreeView（TreeNode 四态色 + shadow/开关/偏移 + border-visible + disabled + hasStyle）与 ListView（RowStyle + item-id 寻址 + findRowById + 四态绘制）——键：`item-background/item-border/item-text/item-text-shadow`（对象四态，text-shadow 兼容单色）+ `item-shadow/item-shadow-offset-x/y/item-border-visible/item-disabled`；优先级 cell > item > 控件级；pressed 行跟踪（MouseDown/Up）。
> 4. **测试**：临时探针全绿（item 对象态读写 + 行底像素、page-background 像素、menu 对象路径像素、winframe bg/font）；全量回归仅 3 项预存失败（menu_cabi/treeview_cabi/multiviewport）。schema/tools ctest 保持全绿。
> 5. **文档**：`declarative-syntax.html` 新增 **6.3.1 视觉能力矩阵（按控件类型）**（含 item 级键与六条说明），标题 27→26；`properties.html` 7.1.1 改为指向 6.3.1 的指针；treeview.html 补 8 个 item 级键行；listview.html 新增 item（行级）属性小节。
> 6. **复核小确认（§4）**：colors-basic 已删除（无引用）；`page-background`/`selected-text` 动态行暂不生成（待 def 类型与结构化编辑推进）。

---

## 1. 问题确认（源码逐格审计结论）

| # | 报告 | 核实 | 根因 |
|---|---|---|---|
| P0-32 | 输入控件 shadow 声明与实现不符 | 确认 | §4.2 下放清单误含（违背矩阵） |
| P0-33① | slider pressed 不生效 | 确认 | `Slider::handleEvent` 自处理点击不链基类 |
| P0-33② | progressbar 背景/边框态不生效 | 机制完整 | update/handleEvent/beforeDraw 链均在；疑 fill 覆盖观感 + border 默认不可见（探针定位） |
| P0-33③ | menu 背景态不生效 | 确认 | 专用成员只有单态键路径；StateColor 对象路径未映射 |
| P0-33④ | animation hover/pressed 不生效 | 确认 | `LuotiAni::update` 未链 `ControlImpl::update()` |
| P0-33⑤ | colorpicker pressed 不生效 | 确认 | 自处理点击不链基类 |
| P0-34① | colorpicker 文字四态错位 | 确认 | 关闭态 Label `setEnable(false)` → 恒 Disabled（disabled 色被当 normal） |
| P0-34② | colorpicker 弹窗残留 | 确认 | `~ColorPicker()` 为空，弹窗挂 BENCH 未回收 |
| P0-35① | progressbar 文本不居中（尺寸变化） | 确认 | label 矩形仅在创建时设置，`updateTextLabel` 只更新 caption |
| P0-35② | progressbar text 仅 normal | 确认 | label 无状态同步 + 重建覆盖四态 |
| P0-35③ | progressbar shadow 无颜色 | 确认 | 单态键 `text-shadow` 未转发 + 重建覆盖 |
| P0-36 | NUD text.disabled 不生效 | 确认 | `EditBox::draw` 恒 `getNormal()`（**用户要求改为：输入族 text 仅 normal，schema 收窄，不实现四态**） |
| P0-37 | ContextMenu resize 面板不跟随 | 确认 | 无 `setRect` override |
| 新发现 | ScrollBar hover/pressed 不可达 | 确认 | `ScrollBar::update(void){}` 空 override + 点击自消费 |
| 新发现 | Splitter pressed 不可达 | 确认 | 点击自消费不链基类 |
| 新发现 | Slider text 四态不生效 | 确认 | valueLabel 无状态同步（label 恒 Normal） |
| 新发现 | button/check-box/win-frame font 名未分发 | 确认 | 无 kFont 分发、ApplyFontToControl 不含 |
| 新发现 | win-frame font-size 未分发 | 确认 | 无 kFontSize 分发 |

## 2. 能力矩阵（修正版，按用户 2026-09-27 要求）

图例：**✓** 已实现且可达；**＋** 本批新增/补齐；态子集列出即"声明=实现"；"（运行时）"= 实现保留但 schema 不声明。

| 控件组 | background | border | text | text-shadow | shadow 开关+偏移 | font / font-size |
|---|---|---|---|---|---|---|
| label / button / check-box | 四态 ✓ | 四态 ✓ | 四态 ✓ | 四态 ✓ | ✓ | ✓ / ✓（+ button/check-box font 名 ＋） |
| win-frame | **两态（N/H）＋**（作用于窗体内 ClientPanel） | **两态（N/H）＋**（作用于**整体窗框**） | 四态 ✓ | 四态 ✓ | ✓ | **font 名 ＋ / font-size ＋** |
| edit-box / text-area / combo-box / numeric-up-down | **三态（N/H/D）**（运行时四态） | **三态（N/H/D）**（运行时四态） | **单态（N）**（schema 收窄） | ⊘（收回） | ⊘（收回，P0-32） | ✓ / ✓ |
| list-view / tree-view（控件级） | **三态（N/H/D）＋** | **三态（N/H/D）＋** | **⊘（收掉，header 亦走 item）** | ⊘ | ⊘ | ✓ / ✓ |
| **list-view / tree-view（item 级）＋** | **四态** | **四态** | **四态** | **四态（色）+ 开关/偏移** | **✓（item 级）** | 既有 item-font/item-font-size ✓ |
| progress-bar | **三态（N/H/D）** | **三态（N/H/D）** | **三态（N/H/D）＋**（状态同步 + 快照） | **三态 ＋**（单态键转发） | ✓ | ✓ / ✓ |
| status-bar | 四态 ✓ | 四态 ✓ | 四态 ✓（pressed 可达性 ＋） | 四态 ✓（同上） | ✓ | ✓ / ✓ |
| tab-control | 四态 ✓ | 四态 ✓ | 四态 ✓（pressed 可达性 ＋） | 四态 ✓（同上） | ✓ | ✓ / ✓ |
| tab-control 页 panel | **`page-background` 四态转发 ＋**（见 §3.5） | ⊘ | — | — | — | — |
| slider | 四态 ✓（pressed 链 ＋） | 四态 ✓（同上） | **四态 ＋**（滑块态同步至 valueLabel；label 不自行检测） | **四态 ＋**（同同步） | ✓ | ✓ / ✓ |
| color-picker（闭合态） | **四态 ＋** | **四态 ＋** | **四态 ＋**（解除恒 Disabled） | **四态 ＋** | ✓ | ✓ / ✓ |
| menu-bar | **三态（N/H/D）＋**（对象路径映射） | **三态（N/H/D）＋** | 三态（N/H/D）✓（对象路径 ＋） | 三态（N/H/D）✓（对象路径 ＋） | ✓ | ✓ / ✓ |
| panel / shape?（见 §5-1） / image / animation | **三态（N/H/D）**（运行时四态） | **三态（N/H/D）**（运行时四态） | ⊘ | ⊘ | ⊘ | ⊘ |
| splitter | **三态（N/H/D）**（运行时四态） | ⊘ | ⊘ | ⊘ | ⊘ | ⊘ |
| scroll-bar | 四态 ✓（hover/pressed 可达性 ＋） | ⊘ | ⊘ | ⊘ | ⊘ | ⊘ |
| dialog / popup / confirm-popup | **两态（N/H）＋** | **两态（N/H）＋** | ⊘ | ⊘ | ⊘ | ⊘ |

## 3. 实施范围

### 3.1 前轮确认项（P0-32 ~ P0-37）
- **P0-32**：schema 收回输入族 `shadow`（text-shadow 一并 ⊘）。
- **P0-33①⑤**：Slider / ColorPicker 点击路径补 `applyPressState`（含 MouseUp 复位）。
- **P0-33③**：MenuBar/MenuPanel 补 `setBackgroundStateColor`/`setBorderStateColor`（专用成员映射 + bar→panel 传播 + 读回组装）；成员扩至四态（新增 disabled 背景/边框成员；hover/pressed 复用 hoverBg/activeBg 语义）。
- **P0-33④**：`LuotiAni::update` 链 `ControlImpl::update()`（可见守卫后）。
- **P0-34①**：关闭态 Label 解除禁用 + `ColorPicker::setState/setEnable` 同步（swatch 与 label 均跟随）；swatch 色块保持 m_color 四态同值（显示正确）。
- **P0-34②**：`~ColorPicker()` 关闭并回收弹窗；`setRect` 时弹窗可见则重定位。
- **P0-35①②③**：ProgressBar —— label 矩形随尺寸同步；持 `StateColor m_textStateColor` 快照（setTextStateColor/单态键维护）并在 `createTextLabel` 重建时应用；`setState` 同步 label；`setColorProperty` 补 text-shadow 族单态键转发。
- **P0-36（调整）**：**改为 schema 收窄**（输入族 text 单态，见 §2），不实现四态解析。
- **P0-37**：`ContextMenu::setRect` override → `m_menuPanel->setRect(0,0,w,h)` + 重算。

### 3.2 字体补齐（用户 1、2）
- Button/CheckBox：`setEnumProperty/getEnumProperty(kFont)` → caption Label `setFont`；`ApplyFontToControl` 补 Button/CheckBox 分支。
- WinFrame：`kFont`/`kFontSize`（Int/Float）→ `m_titleLabel`；`ApplyFontToControl` 补 WinFrame 分支（font-size → title）。
- word：`font` 对象 JSON 路径经 ApplyFontToControl 生效。

### 3.3 WinFrame 背景/边框两态（用户 2 + 2026-09-27 修正）
- 语义：`colors.background`（两态 N/H）**作用于窗体内 ClientPanel**（内容区底色；标题栏装饰保持专用键）；`colors.border`（两态 N/H）**作用于整体窗框**（WinFrame 自身外框边框，经 Panel 绘制路径，非 clientPanel 边框）。
- 实现：WinFrame override `setBackgroundStateColor`（+单态键）→ `m_clientPanel`；`setBorderStateColor`（+单态键）→ 自身（基类存储 + `border-visible` 联动）；hover 由 WinFrame 自身状态驱动（鼠标在窗框内）。
- pressed/disabled 取 normal（schema 仅两态）。

### 3.4 item 级四态（ListView/TreeView，用户 4）——本批最大新增
- **键设计**（item-id 寻址；TreeView 复用 item-id 模式，ListView 新增 item-id 寻址——已确认）：
  - 色（四态）：`item-background` / `item-border` / `item-text` / `item-text-shadow`（对象形式；`item-text-shadow` 由现单色升级为四态对象并**兼容单色字符串**）；
  - shadow：`item-shadow`（Bool）/`item-shadow-offset-x` / `item-shadow-offset-y`；
  - border：`item-border-visible`（Bool）。
- **存储**：TreeView→TreeNode 扩展字段（StateColor×4 + shadow 开关/偏移 + borderVisible）；ListView→行级样式结构（新增 RowStyle，CellStyle 优先级更高：cell > row > 控件级）。
- **渲染**：行/节点绘制按自身状态（hover 行=鼠标所在行；pressed=按下行；disabled=行/节点禁用）解析四态并绘制 bg→border→text(两遍 shadow)。
- **C ABI/Binding**：通用属性通道 + item-id 寻址（TreeView 既有 `item-id`；ListView 新增 `item-id` 键与行 id 映射）+ 必要专用 setter（`ListViewSetRowStyle`? 见 §5-4）；Binding 包装。
- **默认**：item 级未设置 → 控件级（List/Vue 现行为不变）。

### 3.5 TabControl `page-background` 四态转发（用户 5，2026-09-27 确认）
- 新增 `page-background`（StateColor 四态，作用于页 panel 背景，无需边框）；
- 实现：TabControl 存储并转发至**所有页 panel**（切换后仍生效）；页 panel **显式自设** `colors.background` 时优先（应用显式 > 转发）；
- 页 panel 自设四态背景本就可用（Panel），转发键提供统一控制入口。

### 3.6 Slider 态同步（用户 6）
- `Slider::setState` override → `m_valueLabel->setState(state)`（滑块/控件态驱动）；
- valueLabel `setClickable(false)`（不自行检测 hover/pressed）；
- bg/border 四态：补 pressed 链（§3.1①）。

### 3.7 ColorPicker 闭合态四态（用户 7，2026-09-27 确认）
- 关闭态 = swatch（色块）+ hex Label：
  - `colors.background`/`border` 四态 → **封闭态底板**（ColorPicker 自身 Panel 表面 + 外框；P0-27a 设色自动取消透明）；
  - **swatch 色块内容恒 m_color**（不随 colors.background）；
  - text/text-shadow 四态 → 关闭态 Label（状态同步，解除恒 Disabled）；
  - 弹窗回收（§3.1 P0-34②）。

### 3.8 MenuBar 背景/边框三态（用户 8 + 2026-09-27 修正：N/H/D）
- colors.background/border 为**三态（normal/hover/disabled）**，映射 bar/panel/item 角色：
  - normal → bar/panel 底色、item 底色；hover → bar 悬停项/item 悬停底色；disabled → 禁用项底色/边框（新增禁用成员）；
  - **pressed/active 不对外**：菜单展开激活视觉沿用内部缺省（`BAR_ACTIVE_BG`），不新增公开态；
- text/text-shadow 已是三态（N/H/D）✓，与背景/边框一致；
- MenuItem 级样式待设计器支持添加 MenuItem 后再分析（本批不展开）。

### 3.9 其余状态收窄/补齐（用户 3、9、10、11、12）
- 输入族：text 单态、bg/border 三态声明（运行时四态保留）；shadow 收回。
- panel / shape? / image / animation：bg/border 三态声明（运行时四态；pressed 不声明）。
- splitter：bg 三态声明。
- scroll-bar：bg 四态保持；**可达性补齐**（`update()` 链基类 + 点击链 `applyPressState`）。
- dialog / popup / confirm-popup：bg/border 两态（新增 `state-color-2` def）；**pressed 不处理不声明**（点击应传给窗体内控件或触发关闭——现关闭逻辑已有）；disabled 语义不适用。

## 4. schema 设计（def 调整）

```
state-color-2  新：{normal, hover}                       // dialog/popup/confirm-popup、win-frame bg/border
state-color-3d 既有：{normal, hover, disabled}
state-color-4  既有：{normal, hover, pressed, disabled}
state-color-1  既有：{normal}
colors-full    既有（label/button/check-box）           // 4/4/4/4
colors-winframe 新：{background: sc2, border: sc2, text: sc4, text-shadow: sc4}
colors-input   新：{background: sc3d, border: sc3d, text: sc1}   // 无 text-shadow（P0-32 收回）
colors-3full   新：{background: sc3d, border: sc3d, text: sc3d, text-shadow: sc3d}   // progress-bar（三态，无 pressed）
colors-3state  新：{background: sc3d, border: sc3d}     // panel/shape/image/animation + list/tree 控件级（无 text）
colors-bg3     新：{background: sc3d}                    // splitter
colors-bg      既有：{background: sc4}                   // scroll-bar
colors-menu    调整：{background: sc3d, border: sc3d, text: sc3d, text-shadow: sc3d}   // 统一三态（N/H/D；原 background 三态+ border 单态）
colors-basic2  新（替代 colors-basic1）：{background: sc2, border: sc2}   // dialog 族
```
- item 级键为**运行时键**（不入 schema 控件 def，与既有 item-font 一致；items 数据节点由结构化编辑扩展后另行声明）。
- `shadow` 从输入族/panel/shape/image/animation/splitter/scroll-bar/dialog 族移除；list/tree 控件级无 shadow（item 级新增）。

## 5. 已确认（2026-09-27，无剩余待审核）

- page-background 转发至**所有页** + 页显式自设优先；win-frame background 仅 ClientPanel；list/tree 控件级 text 运行时键保留；**MenuBar 三态（N/H/D，非 pressed）**；ListView 优先级 cell > item > 控件级。

## 6. 验收测试

| # | 验收 | 方式 |
|---|---|---|
| 1 | schema：态子集与矩阵一致；strict 校验全绿 | validate_layout + 矩阵核对 |
| 2 | 可达性：slider/colorpicker/splitter/scrollbar/list/tree/statusbar/tabcontrol 的 pressed 像素（自消费类经探针确认后补齐） | 探针（PushUIEvent）+ 捕获 |
| 3 | 字体：button/check-box/win-frame font 名、win-frame font-size | 读写回 + 像素 |
| 4 | win-frame 背景/边框两态 → clientPanel | 像素 |
| 5 | item 级四态（tree 节点/list 行）：bg/border/text/shadow + hover 行/pressed 行/disabled 行 | 探针像素 + item-id 读写 |
| 6 | tab 页 panel 四态背景 | 像素 |
| 7 | slider：滑块态传递 valueLabel（bg/border 四态 + label text/shadow 四态） | 像素 |
| 8 | colorpicker 闭合态四态（解除恒 Disabled）+ 弹窗回收 | 像素 + 生命周期 |
| 9 | menu bg/border 四态（对象路径 + 单键） | 读写回 + 像素 |
| 10 | progressbar（centering + text/shadow 三态色） | 像素 |
| 11 | dialog 族 bg/border 两态 | 像素 |
| 12 | 手册：`declarative-syntax.html` 6.4 视觉能力矩阵存在且与 schema/实现一致；properties.html 7.1.1 为指针不重复 | 人工核对 + 与 schema 矩阵比对 |
| 13 | 全量回归（既有预存失败除外） | 全量扫描 |

## 7. 配套改动

- schema（§4 def 调整）+ 校验。
- **用户手册：`docs/appendix/declarative-syntax.html` 新增小节 `6.4 视觉能力矩阵（按控件类型）`**（内容 = 本设计 §2 修正版矩阵 + 态子集说明 + item 级键清单）；作为矩阵**唯一权威来源**。
- `docs/appendix/properties.html`：7.1.1 旧矩阵（P0-26 版）改为**指向 6.4 的指针 + 状态色组（colors/state-color def）说明**，避免双份矩阵漂移；表内"设色自动取消透明"等注保留。
- 相关控件手册页同步：list-view/tree-view（item 级 8 键）、progress-bar（三态）、tab-control（page-background）、win-frame（bg→ClientPanel/border→整体 + font）、color-picker（闭合态四态/弹窗回收）、menu-bar（三态）。
- C ABI/Binding 速查表：ListView item-id/行样式 setter 增补（§3.4）。
- C ABI/Binding：ListView item-id + 行样式 setter（见 §5-4）；其余经通用通道。
- 探针/持久测试；requirements/ 归档；复核意见归档后 make_release 同步 CornerstoneDesigner。
