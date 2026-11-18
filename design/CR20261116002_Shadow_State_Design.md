# CR20261116002 — 柔光阴影声明式状态与主题 :hover 原生支持（引擎侧分析 + 设计）

> 状态：**closed（2026-11-17 设计器视觉验证通过；设计器/引擎两项均 closed）**
> 来源：`D:\GitSpace\CornerstoneDesigner\Temp\UICornerstone_配合修改清单_2026-11.md` → CR20261116002
> 关联：CR20261112001（通用 box-shadow 柔光）、`ControlBase.h/.cpp`、`ThemeStyle.h/.cpp`、`LayoutParser.cpp`、`ShadowRenderer`、`docs/theme.html`
> 边界：本文档只做引擎侧分析与设计；评审通过后方可实施。

## 1. 现状与证据

`box-shadow` 目前只有 normal 规格：

- `ControlImpl` 保存单份 `m_boxShadowColor/OffsetX/OffsetY/Radius/Spread` 与 `*Set` 标志：`include/ControlBase.h:347-357`；
- 绘制只读取这一份：`src/ControlBase.cpp:284-302`（`drawBoxShadowForRect`）；
- JSON 解析只有 `box-shadow` 顶层五字段：`src/LayoutParser.cpp:3143-3152`；
- 主题 `box-shadow` 只写 normal：`src/ThemeStyle.cpp:527-542`；
- 主题状态规则（`:hover`/`:pressed`/`:disabled`）只对 **颜色键** 加状态后缀（`src/ThemeStyle.cpp:468-479`、`stateSuffix()`）：`box-shadow` 对象不是颜色键，所以写在 `:hover` 里仍落到 normal；
- 主题 `:selected` 复用 pressed 后缀（`stateSuffix` 已实现）；
- 显式覆盖登记 `captureExplicitFromJson` 未采集 `box-shadow`（`src/ThemeStyle.cpp:636-689`），主题重放无法保护显式阴影；
- 阴影缓存键已包含生效 color/blur/spread/corner/scale + generation（`src/ShadowRenderer.cpp:505-514`），offset 在绘制期应用；纹理键不需要状态名。

结论：缺少的是 **按状态存储 + 状态键路由 + 主题状态映射 + 显式采集**，绘制与缓存主体可复用。

## 2. 目标与非目标

### 2.1 目标

1. `box-shadow` 支持 `hover` / `pressed` / `disabled` 三个状态规格；
2. 状态对象为部分覆盖，缺省字段继承 normal；
3. 状态 `color` alpha=0 表示该状态明确关闭阴影；
4. 主题 `:hover` / `:pressed` / `:disabled` 的 `box-shadow` 落到对应状态；`:selected` 沿用 pressed；
5. 运行时状态键沿用 `.hover` / `.pressed` / `.disabled` 后缀；
6. 显式 JSON 状态 > 主题状态 > 显式 JSON normal > 主题 normal；
7. 不改变布局、命中、内容裁剪与绘制顺序；状态切换只重绘；
8. 不新增 C ABI/Binding 函数。

### 2.2 非目标

- `text-shadow` / Label 字形柔光状态（Q7：另立 CR）；
- focus/selected 新 `ControlState`（V1 仅现有三态 + normal）；
- 方案 B 的独立 `hover-box-shadow` 键；
- 阴影形状/描边扩展、多层 box-shadow 数组；
- 设计器属性面板嵌套行（schema 就绪后由设计器实施）。

## 3. 引擎对 Q1~Q7 的结论

| 问题 | 结论 |
|---|---|
| Q1 方案 A/B | **采用方案 A**：`box-shadow{hover{},pressed{},disabled{}}` 嵌套对象；schema/主题/设计器同一结构；不引入独立键 |
| Q2 状态集合 | **V1 = hover / pressed / disabled**；不新增 focus/selected 状态；`:selected` 按现有规则映射 pressed |
| Q3 部分继承 + alpha=0 | **接受**；按字段回落 normal；状态 color 显式 alpha=0 关闭该状态阴影（normal alpha=0 时状态仍可开启） |
| Q4 主题映射 | **选择器状态 → 对应状态**：`:hover` 的 `box-shadow` 写 hover，不要求主题再嵌套；状态规则内出现嵌套状态键 → 忽略 + 一次性诊断 |
| Q5 运行时键 | **`.hover/.pressed/.disabled` 后缀**（与背景/边框/文字色一致）；`Get*` 返回**生效值**（状态未设置时回退 normal），文档明确 |
| Q6 缓存/失效 | 缓存键**不需要状态名**：生效 color/blur/spread/corner/scale 已足以区分；主题切换已有 generation；状态切换只改生效规格 → 换键/重绘。映射入口在 `ThemeRuntime`，生效解析在 `ControlImpl` |
| Q7 text-shadow | **不纳入本 CR**；Label 字形柔光状态（颜色/offset/blur 四态）另立 CR 评估 |

## 4. 设计

### 4.1 数据模型

`ControlImpl` 新增内部状态结构（private，不进入公开 API）：

```cpp
struct BoxShadowStateValues {
    SColor color{0,0,0,0};
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float radius = 0.0f;
    float spread = 0.0f;
    bool colorSet = false;
    bool offsetXSet = false;
    bool offsetYSet = false;
    bool radiusSet = false;
    bool spreadSet = false;
};
BoxShadowStateValues m_boxShadowState[3];   // Hover / Pressed / Disabled
uint32_t m_boxShadowThemeStateMask = 0;     // 主题写入位（3 状态 × 5 字段）
```

- normal 继续使用现有 `m_boxShadowColor/OffsetX/...` 与 `*Set`，避免迁移风险；
- `stateIndex(ControlState)`：Hover→0、Pressed→1、Disabled→2、Normal→-1；
- 生效解析（绘制前）：

```cpp
SColor color = st.colorSet ? st.color : m_boxShadowColor;
float  ox    = st.offsetXSet ? st.offsetX : m_boxShadowOffsetX;
... // y / radius / spread 同
if (color.alpha() <= 0.0f) return;   // 显式 alpha=0 或 normal 亦无 → 不绘制
```

### 4.2 JSON 解析

`parseCommonProperties` 的 `box-shadow` 块改为调用 `applyBoxShadowObject(ctrl, sh, -1)`：

1. 先按现有五字段写 normal；
2. 若存在 `hover`/`pressed`/`disabled` 对象，逐字段写对应状态槽（只写出现字段，置 `*Set=true`）；
3. 空状态对象 → 全继承 normal；
4. 状态对象只接受 `color/offset-x/offset-y/blur/spread`；未知键忽略并由 schema strict 拒绝；
5. 范围沿用 CR20261112001：`blur 0~64`、`spread>=0`、offset 可负；schema 继续承担边界校验；
6. `color` 使用 `SColor::fromHex`；alpha=0 合法并保留（`colorSet=true`）。

### 4.3 运行时属性键

新增 `PropertyNames`（15 个常量）：

```
box-shadow-color.hover      box-shadow-color.pressed      box-shadow-color.disabled
box-shadow-offset-x.hover   box-shadow-offset-x.pressed   box-shadow-offset-x.disabled
box-shadow-offset-y.hover   box-shadow-offset-y.pressed   box-shadow-offset-y.disabled
box-shadow-radius.hover     box-shadow-radius.pressed     box-shadow-radius.disabled
box-shadow-spread.hover     box-shadow-spread.pressed     box-shadow-spread.disabled
```

- `setColorProperty`：识别颜色状态键 → 写状态槽；
- `setFloatProperty`：识别 offset/radius/spread 状态键 → 写状态槽；
- `getColorProperty` / `getFloatProperty`：状态键返回**生效值**（set ? 状态值 : normal）；
- 无状态键写入时行为与旧版完全一致；
- B6：`Get*` 只返回生效值，**无法区分该状态是否显式设置**；设计器判断“是否已自定义”应读模型 JSON（`persist` 路径），编辑时显示继承值、写入后即固化为该状态显式值；不新增查询 API；
- C ABI/Binding 无新增函数；现有 `SetColor/SetFloat/GetColor/GetFloat` 传状态键即可；
- 运行期写入经 `recordExplicit*` 登记，主题重放按状态键保护（见 §4.6）。

### 4.4 主题状态映射

1. `ThemeRuntime::mergeRules`：状态规则中除颜色键外，**把 `box-shadow` 与其拆散键也加状态后缀**：
   - `box-shadow` → `box-shadow.hover`（pressed/selected→`.pressed`、disabled→`.disabled`）；
   - `box-shadow-offset-x/-y/-radius/-spread/-color` 同理；
2. `applyValue`：
   - `box-shadow`：解析对象，先写 normal，再按对象内嵌 `hover/pressed/disabled` 写状态槽（与 JSON 同语义）；
   - `box-shadow.<state>`：解析对象为**该状态的部分覆盖**，只写状态槽；出现嵌套状态键 → 忽略 + 一次性诊断；
   - `box-shadow-color.hover` 等：沿用现有颜色状态键分支；
   - `box-shadow-offset-x/-y/-radius/-spread` 的 `.hover/.pressed/.disabled` 键必须先于现有“含 `.hover` → 按颜色解析”分支识别并走 `setFloatProperty`，否则会被颜色分支吞掉；
3. 主题普通规则与状态规则合并时，键已区分（`box-shadow` vs `box-shadow.hover`），不会互相覆盖；
4. B8：`:selected` 是**主题伪类别名**，不是运行时状态：`stateSuffix("selected")` → `.pressed`，与 `:pressed` 写同一组槽位；两者按既有 specificity/order 合并，运行时仍只有 Normal/Hover/Pressed/Disabled，selected 与 hover/pressed 的优先级即 pressed 状态；文档需明确“等同 pressed”；
5. 主题变量 `var(--x)` 在 `applyValue` 前已解析，对象内颜色变量沿用现有字符串解析。

### 4.5 生效状态与绘制

- 状态优先级：Disabled > Pressed > Hover > Normal（`m_state` 已由输入层决定）；
- 生效规格按 §4.1 逐字段回落；
- 绘制仍走 `ShadowRenderer::drawRectShadow`，顺序不变（背景外/内容下）；
- 状态切换只改 `m_state`，不触发 `setRect`/布局/命中；下一帧绘制使用新规格；
- 内容裁剪/命中不受阴影影响（既有行为）。

### 4.6 显式与主题优先级

`ThemeRuntime::captureExplicitFromJson` 增加 `box-shadow` 采集：

- normal 五字段 → `box-shadow-color` / `box-shadow-offset-x/y` / `box-shadow-radius` / `box-shadow-spread`；
- `hover/pressed/disabled` 内出现字段 → 对应 `.hover/.pressed/.disabled` 键；
- 只采集 JSON 显式出现的字段，未出现字段不登记。

`replayExplicit` 复用现有颜色/数字通道；补充规则：

- B5：`explicitValueMatches` 对 `box-shadow-*.hover/.pressed/.disabled` 先检查对应状态槽的 `*Set`；**未设置直接返回 false**（强制 replay），再比较值。否则显式值恰好等于 normal 时会被误判为已匹配而跳过；需要 `ControlImpl` 暴露内部查询（如 `bool isBoxShadowStateSet(ControlState, const char* field)`，仅 C++ 内部，无 C ABI）；
- normal 键只写 normal 槽，不会覆盖状态槽，满足“normal 不覆盖 current state”。

优先级链（从高到低）：显式 JSON 状态 → 主题状态 → 显式 JSON normal → 主题 normal。

### 4.7 缓存与失效

- 纹理键继续使用生效 color/blur/spread/corner/scale + generation（`ShadowRenderer.cpp:505`）；
- 状态名不进入键：两个状态生效规格相同则复用纹理，视觉等价；
- 状态切换：规格变化 → 新键；未变化 → 命中已有纹理；
- 主题切换/DPI/尺寸/圆角变化：沿用现有 generation/键字段失效；
- offset 在绘制期应用（既有），不入纹理键；
- 不新增独立失效 API。

### 4.8 兼容性

- 未声明状态字段的布局/主题：normal 行为不变；
- 旧主题 `:hover` 中的 `box-shadow` 从“落到 normal”改为“落到 hover”：属新增能力，旧主题若同时声明 normal 与 `:hover`，升级后会按状态生效；
- 应用在 `onMouseEnter/Leave` 手改 normal 键的做法仍可用；同时声明状态规格时，状态规格优先（文档说明）；
- 拆散键继续表示 normal；
- B9：normal 阴影沿用主题属性累积语义（B 未声明的 normal 字段保留 A 的值）；状态槽随主题重算清空，避免残留；
- 主题状态里写 `box-shadow-color` 的旧行为（已支持 `.hover` 颜色键）不变。

### 4.9 主题状态生命周期（B1/B1a/B9/B12）

#### 4.9.1 掩码归属（B1a/B10）

掩码**放在 `ControlImpl`**（`uint32_t m_boxShadowThemeStateMask`，3 状态 × 5 字段位），不放进 `ThemeRuntime`：

- 生命周期随控件：销毁/移除自动消失，无悬垂指针；
- `ThemeRuntime` 通过内部方法置位/取走/清位，**不新增数据成员** → `ThemeRuntime` 布局不变（B10 关闭）；
- `ControlImpl` 本就因状态槽改变布局，多一个 `uint32_t` 不额外扩大 ABI 影响面。

内部方法（private + `friend class ThemeRuntime`，无 C ABI）：

```cpp
void markBoxShadowThemeState(uint32_t bits);   // 主题写状态槽时置位
uint32_t takeBoxShadowThemeMask();             // 取走并清零
void clearThemeBoxShadowStates(uint32_t bits); // 按位清状态槽 + *Set
```

#### 4.9.2 清理时机与遍历

- 只在**主题真正变化**时清理：`setTheme` 成功替换样式表后由 `applyAll/applyControl` 执行；`UICornerstone_ApplyTheme` 对空/无线数据提前返回（`UICornerstoneAPI.cpp:1686-1687`），因此“空主题”不会触发清理，也不应触发——沿用既有 no-op 契约；
- 清理遍历与主题应用遍历**分开**：先对整棵子树（不因 `isThemeEligible()` 跳过）取走掩码并清槽，再执行 `applySet`；未被主题写过的控件掩码为 0，遍历开销可忽略；
- `applyControl`（单控件）同样先清该控件掩码，再应用；
- `clearOverrides`（CR2 定案）：清除显式登记后，**清全部状态槽 + 掩码**，由调用方（`UICornerstone_ClearThemeOverrides` → `applyThemeAllRoots`）立即按当前主题重算。这样显式状态消失、主题状态恢复；主题无状态时回退 normal。该行为已同步到 S13c；
- 控件移出树/销毁：掩码成员随对象销毁；重新挂载后由 `applyControl` 先清后应用；
- `theme-eligible=false`：若曾被主题写入，清理遍历仍会清（不跳过）；之后不再接受主题写入。

#### 4.9.3 normal 槽的生命周期（B9）

normal 阴影沿用**既有主题属性累积语义**，不纳入掩码清理：

- 主题应用是“属性写入”而非整表重置：主题 B 未声明某属性时，主题 A 写入的值保留（与 background/text/padding 等现有行为一致）；
- 因此 B 无 normal 声明 → 保留 A 的 normal，**hover 回退的是 A 的 normal，而不是“无阴影”**；B 只有部分 normal 字段 → 只覆盖声明字段，其余保留 A；
- 如需整体移除 normal 阴影，显式写 `box-shadow.color` alpha=0 或应用完整覆盖的主题；
- 状态槽不同：它们是“按主题重算的派生状态”，主题替换后必须清空重算，避免 A 的状态残留（B1）；
- **B12 结论**：掩码只跟踪 3×5 个状态槽位，不跟踪 normal；normal 不参与清理。

#### 4.9.4 `ApplyTheme` 空主题语义（B1a 补充）

- `UICornerstone_ApplyTheme("{}")`、仅变量/无线数据主题：**no-op**，不替换样式表、不触发清理、不回到“无阴影”；
- 当前主题写入的状态槽保持不变；normal 也不变；
- 需要清理/覆盖状态时：应用**有实际主题数据**的新主题，或显式写 `box-shadow.color` alpha=0 / 对应状态键；
- “空主题 no-op” ≠ “清空阴影”，文档与测试需明确区分；
- `ClearThemeOverrides`：清显式登记 + 全部状态槽/掩码，随后按当前主题重算（显式消失、主题状态恢复；无主题状态则回退 normal）。

#### 4.9.5 replay 顺序

清理 → `applySet`（base + variant + media/container）→ `replayExplicit`。显式 JSON/运行期状态经 B5 强制写回；normal 显式本就由既有 replay 处理。

### 4.10 声明应用顺序（B2/B3）

`applyOne` 的 `merged` 是 `unordered_map`，必须固定应用顺序。按类别排序后依次应用（后者覆盖前者）：

| 顺序 | 类别 | 说明 |
|---|---|---|
| 1 | 普通非阴影声明 | 与顺序无关，保持现状 |
| 2 | `box-shadow` 对象（normal） | 含对象内嵌 `hover/pressed/disabled` |
| 3 | 普通拆散阴影键 | `box-shadow-color/-offset-x/-offset-y/-radius/-spread` |
| 4 | 状态对象键 | `box-shadow.hover/.pressed/.disabled` |
| 5 | 状态拆散键 | `box-shadow-*.hover/.pressed/.disabled` |

结论：

- B2：**状态选择器始终胜出**于普通规则内嵌状态；同一状态对象后应用；
- B3：**拆散键优先于对象**（同一状态、同一层级），标量声明视为更具体的覆盖；
- 排序后即使 `unordered_map` 遍历顺序变化，结果确定；
- B13：排序只决定**类别间**先后；同一类别内同一键仍按既有 specificity/order 合并（`mergeRules` 已处理），不会因排序覆盖更高优先级规则；需补“同类别多规则”测试；
- 普通规则内嵌状态与状态选择器同时存在时不禁止，但状态选择器结果确定优先；如需诊断可在文档说明。

### 4.11 控件支持矩阵（B4）

| 控件 | 阴影绘制路径 | 状态阴影 V1 |
|---|---|---|
| 普通控件（Panel/Button/EditBox/Image/Label 等） | `ControlImpl::beforeDraw` → `drawBoxShadowForRect` | 支持四态 |
| Popup / ConfirmPopup / Dialog | 继承 Panel → `ControlImpl::beforeDraw` → `drawBoxShadowForRect` | 支持四态（运行时通常为 Normal，disabled 可生效） |
| ContextMenu | 自定义 `draw()` 跳过 Popup 背景/阴影，视觉由 MenuPanel 负责 | **不支持**；normal/状态键均只存储 + 一次性诊断（不得静默） |
| MenuBar | 自定义 `draw`，已调用 `drawBoxShadowForRect` | 支持四态（随 MenuBar 自身 state） |
| MenuPanel | 自定义 `MenuPanel::drawShadow()` | 只按 **normal** 字段判定 legacy/generic；状态键忽略 + 一次性诊断（CR1）；自身无 hover/pressed 状态切换 |
| MenuItem | 自定义 `draw`，V1 不绘制 box-shadow | 不支持；normal/状态键均只存储 + 一次性诊断（现有日志扩展覆盖状态键） |
| HandleControl | V1 不绘制 box-shadow | 不支持；设置状态键给一次性诊断，文档标注 |
| Window（非 ControlImpl 控件） | 无 `box-shadow` 属性 | 不适用 |

- 统一解析入口：`ControlImpl` 增加 protected `ShadowRenderer::ShadowSpec resolveBoxShadowSpec()`（或 `boxShadowSpecForState(ControlState)`），`drawBoxShadowForRect` 与 `MenuPanel::drawShadow` 共用；
- MenuPanel 的 legacy 回退逻辑保留：只要任意状态或 normal 显式字段存在，就走通用阴影；否则走 legacy `shadow-radius`；
- 不支持控件必须显式诊断（`Platform::Log` 一次性），避免主题 `:hover` 静默无效；ContextMenu 的 **normal 与状态键**都要诊断（不能只诊断状态键），S19 覆盖；
- 文档支持矩阵同步写入用户手册。

## 5. 影响面与 ABI

| 层 | 影响 |
|---|---|
| C++（B10） | `ControlImpl` 增加状态槽 + 状态掩码（私有成员）→ **类布局变化，DLL/头文件/C++ 使用方需同批重编译**；无 vtable 变化。掩码放在 `ControlImpl`，`ThemeRuntime` **不新增成员、布局不变**；若后续改为 ThemeRuntime 持有，则同样需同批重编译 |
| C ABI | 无新增函数；现有 `SetColor/SetFloat/Get*` 接受状态键 |
| Binding | 无新增封装；透传属性键 |
| JSON/Schema | `box-shadow` 增加三个可选状态对象；旧布局合法 |
| 主题 | 状态选择器新增 `box-shadow` 路由；普通规则可内嵌状态 |
| 设计器 | schema 就绪后按 `box-shadow.hover.*` 生成嵌套行；无需新 API |

## 6. 测试计划

新增 `test/test_cr_shadow_state.cpp` + `layouts/cr_shadow_state.json` + `layouts/theme_shadow_state.json`，注册 CMake 并纳入 layouts 复制条件。

| # | 用例 | 断言 |
|---|---|---|
| S1 | JSON normal 阴影 | 生效 color/offset/blur/spread 与旧版一致；外观回归 `test_soft_shadow` |
| S2 | 部分继承 | 仅写 `hover.blur` → hover 生效 blur 新值，color/offset/spread 来自 normal |
| S3 | alpha=0 | `hover.color` alpha=0 → hover 不绘制；`normal` alpha=0 + `hover.color` 有值 → hover 可见 |
| S4 | pressed/disabled | 各自覆盖正确；disabled 优先于 pressed/hover |
| S5 | 主题状态选择器 | 主题 normal + `:hover`/`:pressed`/`:disabled` 的 `box-shadow` 落到对应状态；`:selected` 等同 pressed |
| S6 | 主题嵌套状态 | 普通规则内嵌 `hover` 等对象生效；状态规则内嵌套键被忽略且有诊断 |
| S7 | 优先级 | 显式 JSON 状态 > 主题状态 > 显式 normal > 主题 normal；normal 不覆盖当前状态 |
| S8 | 运行时键 | C ABI `SetColor("box-shadow-color.hover")` / `SetFloat("box-shadow-radius.hover")` 生效；`Get*` 返回生效值；未设置时回退 normal |
| S9 | 布局/命中 | 状态切换前后 rect、命中、子控件布局不变 |
| S10 | 缓存 | 状态来回切换阴影正确；切主题后重建正确（generation）；无错误复用 |
| S11 | 回归 | `test_soft_shadow` 13/13；CR12002/13001 测试通过；Label/Shape/菜单阴影无回归 |
| S12 | strict | 含状态字段的布局 `--strict` 通过；未知键被 schema 拒绝 |
| S13 | B1/B11 主题切换清理 | 主题 A（hover 阴影）→ 应用主题 B（有数据、无状态）→ 状态槽清理，hover 回退生效 normal；B 只有部分 normal 字段时其余 normal 保留 A；B 无 normal 时回退 A 的 normal；再切回 A 恢复 |
| S13a | 空主题 no-op | 主题 A 有状态阴影 → `ApplyTheme("{}")` / 仅变量主题 → 状态阴影与 normal 均保持，不清空 |
| S13b | 显式覆盖/清空 | `box-shadow.color` alpha=0 关闭 normal；状态键 alpha=0 关闭对应状态；与空主题 no-op 区分 |
| S13c | ClearThemeOverrides | 主题状态 + 显式状态 → 清除后显式消失、主题状态恢复；主题状态 + 无显式 → 保持；主题无状态 + 显式 → 回退 normal |
| S20 | B13 同类别多规则 | 两条 `:hover` 规则（同键不同 specificity/order）在五级排序后仍按 specificity/order 取值 |
| S14 | B2 顺序确定 | 普通规则内嵌 hover + `:hover` 对象同时存在，多次重算结果稳定且 `:hover` 胜出 |
| S15 | B3 对象/拆散冲突 | 同状态同时写 `box-shadow.hover` 对象与 `box-shadow-color.hover`，拆散键胜出，多次重算稳定 |
| S16 | B5 显式强制 | 显式 `box-shadow-color.hover` 恰好等于 normal 且状态槽未设置 → 仍写入状态槽；normal 后续变化不影响该状态 |
| S17 | B7 缓存复用 | normal/hover 生效规格相同 → 第二次绘制命中（hit+，miss 不变）；规格不同 → miss+；切回原状态命中 |
| S18 | B8 selected | `:selected` 与 `:pressed` 写同一槽；同时存在时按 specificity/order 合并；运行时 `ControlState` 仍为 Pressed |
| S19 | B4/B4a 支持矩阵 | MenuBar 状态阴影生效；MenuPanel normal 生效、无状态切换；Popup/ConfirmPopup/Dialog 四态生效；ContextMenu/MenuItem/HandleControl 设置状态键有一次性诊断且不崩溃、不绘制 |

三后端各跑一遍；SDL3 全量回归仅允许 4 项历史失败。

## 7. 用户手册与文档更新计划

| 文档 | 更新 |
|---|---|
| `docs/appendix/declarative-syntax.html` | `box-shadow` 增加 hover/pressed/disabled 语法、部分继承、alpha=0 语义 |
| `docs/theme.html` | 状态选择器 `box-shadow` 示例、`:selected` 映射、优先级说明 |
| `docs/appendix/properties.html` | 运行时状态键表（15 个）+ Get 生效值语义 + 控件支持矩阵（MenuBar 四态、MenuPanel normal、MenuItem/HandleControl 不支持并诊断） |
| `docs/controls/label.html` / `menu.html` / `contextmenu.html` | 阴影状态能力说明（无字段变化的控件明确“无视觉变化”） |
| `docs/schema/declarative-ui.schema.json` | `common.box-shadow` 增加状态对象（`$ref` 复用） |
| `README.md` | 特性行 + 新测试 |
| `design/API_Mapping_Table.md` | 增 CR20261116002 行（无新增 C ABI/Binding） |

截图：`box-shadow` 状态属视觉属性，需补 normal/hover/pressed/disabled 对比截图（统一主题/尺寸），放入 `docs/assets/controls/`。

## 8. 风险与回退

| 风险 | 处理 |
|---|---|
| 状态键与 normal 键混淆 | 15 个常量集中 `PropertyNames`；Get 生效语义文档化 |
| 主题状态对象与拆散键双重路径 | 统一走状态槽；S5/S6 覆盖 |
| 显式值恰好等于 normal 被跳过 | 状态槽未设置视为不匹配，强制 replay（§4.6） |
| 缓存错误复用 | 键含生效规格 + generation；S10 |
| C++ 类布局变化 | 同批重编译声明（§5）；C ABI 不变 |
| `text-shadow` 期望被顺带支持 | CR 明确排除 + 文档说明 |
| 旧主题 `:hover` 视觉变化 | 视为新增能力；发布说明/手册注明 |
| B1 主题切换残留 | 主题写入位掩码 + 重算前清理 + replayExplicit；S13 |
| B2/B3 顺序不确定 | 固定 5 级应用顺序；S14/S15 |
| B4 内部件静默失效 | 支持矩阵 + 统一解析入口 + 一次性诊断；S19 |
| B5 显式被误跳过 | 状态槽未设置强制不匹配；S16 |

## 9. 验收对照

| CR 验收项 | 设计落点 |
|---|---|
| 1 normal 与旧版一致 | S1 / §4.2 |
| 2 部分继承 | §4.1 / S2 |
| 3 alpha=0 显式关闭 | §4.1 / S3 |
| 4 主题四态映射 + selected | §4.4 / S5 |
| 5 显式/主题优先级 | §4.6 / S7 |
| 6 只重绘、缓存正确 | §4.5 / §4.7 / S9/S10 |
| 7 主题/DPI/尺寸/圆角重建 | §4.7 / S10 |
| 8 C ABI/Binding + strict | §4.3 / S8/S12 |
| 9 设计器 auto/strict、旧布局不变 | §4.8 / S12（联调由设计器验证） |
| B1 主题切换清理 | §4.9 / S13 |
| B2 顺序确定（状态选择器胜出） | §4.10 / S14 |
| B3 对象/拆散键优先级 | §4.10 / S15 |
| B4 内部件支持矩阵与诊断 | §4.11 / S19 |
| B5 显式强制 replay | §4.6 / S16 |
| B6 Get 生效值限制 | §4.3（文档说明） |
| B7 缓存复用/不复用 | §4.7 / S17 |
| B8 selected 等同 pressed | §4.4 / S18 |
| B9 normal 主题生命周期 + 空主题 no-op | §4.9.3/§4.9.4 / S13/S13a/S13b |
| B1a 掩码生命周期 | §4.9.1-4.9.2 / S13 |
| B4a ContextMenu/Popup/Dialog 矩阵 | §4.11 / S19 |
| B10 ThemeRuntime ABI | §4.9.1 / §5 |
| B11 部分 normal 场景 | S13 |
| B12 掩码范围 | §4.9.3 |
| B13 同类别 specificity/order | §4.10 / S20 |

## 10. 实施顺序

1. `PropertyNames` 15 个状态键 + `ControlImpl` 状态槽/生效解析；
2. `drawBoxShadowForRect` 改用生效规格；
3. `LayoutParser` `box-shadow` 状态解析；
4. `ThemeRuntime` 状态后缀/applyValue/显式采集与 replay；
5. schema + 新测试/布局样本 + CMake；
6. 文档/README/API 映射 + 截图；
7. 三后端验证 → 设计器联调 → 关闭引擎项。

## 11. 实施记录（2026-11-16）

### 11.1 落地文件

- `include/PropertyNames.h`：新增 15 个状态键 + 3 个主题状态对象键（`box-shadow.hover/.pressed/.disabled`）。
- `include/ControlBase.h` / `src/ControlBase.cpp`：
  - `BoxShadowStateValues m_boxShadowState[3]` + `m_boxShadowThemeStateMask`（主题写入位，随控件生命周期）；
  - `parseBoxShadowStateKey` / `boxShadowStateIndex` / `boxShadowStateBit` / `isBoxShadowStateSet` / `hasAnyBoxShadowField` / `getEffectiveBoxShadow*` / `resolveBoxShadowSpec` / `clearThemeBoxShadowStates`；
  - `setColorProperty` / `setFloatProperty` 识别状态键；`getColorProperty` / `getFloatProperty` 返回生效值；
  - `drawBoxShadowForRect` 改走 `resolveBoxShadowSpec`。
- `src/LayoutParser.cpp`：新增 `applyBoxShadowJson`，解析 normal + `hover/pressed/disabled` 部分覆盖。
- `include/ThemeStyle.h` / `src/ThemeStyle.cpp`：
  - 状态规则对 `box-shadow` 及拆散键加状态后缀；`applyValue` 支持状态对象/状态拆散键；
  - `applyOne` 固定五级应用顺序（普通 → 对象 → 普通拆散 → 状态对象 → 状态拆散）；
  - `applyAll` / `applyControl` 先清主题写入的状态槽；`clearOverrides` 清全部状态槽 + 掩码；
  - `captureExplicitFromJson` 采集 box-shadow normal + 状态；`explicitValueMatches` 状态槽未设置强制 replay；
  - `MenuPanel::drawShadow` 改用 `hasAnyBoxShadowField()`；MenuItem/HandleControl/ContextMenu 状态键诊断。
- `docs/schema/declarative-ui.schema.json`：新增 `$defs/box-shadow-state`，`common.box-shadow` 增加三状态 `$ref`。
- `test/test_cr_shadow_state.cpp` + `layouts/cr_shadow_state.json` + `test/CMakeLists.txt`；`docs/assets/controls/box-shadow-states.png`（四态截图，`shot=` 模式生成）。
- 文档：`docs/appendix/declarative-syntax.html`、`docs/appendix/properties.html`、`docs/theme.html`、`docs/controls/{label,menu,contextmenu}.html`、`README.md`、`design/API_Mapping_Table.md`。

### 11.2 验证结果

- `test_cr_shadow_state`：SDL3 / SFML / raylib 均 **77/77 通过**，覆盖 S1~S20 + CR1/CR2 + Label/Shape（JSON 四态、部分继承、alpha=0、主题状态选择器、优先级、空主题 no-op、主题切换清理、显式强制 replay、缓存复用/不复用、支持矩阵诊断、ContextMenu 诊断、MenuPanel normal 判定、ClearThemeOverrides 三种来源组合、Label/Shape 状态阴影与主题切换回退、状态 text-shadow 非颜色键忽略）。
- 回归：`test_soft_shadow` 13/13；`test_cr_theme` OK；`test_cr_contextmenu_sync` 48/48；`test_cr_contextmenu_position` 44/44；`test_colorfixes` fail=0。
- SDL3 全量回归：仅 4 项历史失败（`test_menu_cabi` bar-height、`test_multiviewport*`、`test_treeview_cabi` item-font），无新增。
- `validate_layout layouts/*.json --strict`：0 error / 0 warn。
- CR1（代码检视）：`MenuPanel::drawShadow()` 改为 `hasExplicitNormalBoxShadowField()`，状态键不再触发 legacy→generic 切换；MenuPanel 状态键一次性诊断；新增 `hasExplicitNormalBoxShadowField()` 与 S19/CR1 测试（主题 `:hover` 只写状态时 normal 标记不变）。
- Label/Shape 检查：两者走 `ControlImpl::beforeDraw → drawBoxShadowForRect`，状态阴影与主题状态切换/清理均正常；ThemeRuntime 对状态规则中的 `text-shadow-blur` / `shadow-offset-x/y` 忽略 + 一次性诊断（Q7 边界），避免静默修改 normal 文字阴影；新增 L1/L2/SH1 测试。
- CR2（代码检视）：`clearOverrides` 定案为“清显式 + 全部状态槽/掩码，随后按当前主题重算”；设计 §4.9.2/§4.9.4 与 S13c 同步更新；新增“主题状态 + 显式 → 清除后主题恢复”“无显式保持”“无主题状态回退 normal”测试。
- 影响面：`ControlImpl` 类布局变化，DLL/头文件/C++ 使用方需同批重编译；`ThemeRuntime` 无新增成员、布局不变；C ABI/Binding 无新增函数。
- Release DLL SHA256：`d047d755283d890fd32c8240d99ab775f030d0f5fc5dc9863d0e4dd3ce8d2798`；已 `robocopy /MIR` 同步设计器 `subModules/UICornerstone`（含 docs/schema/截图）。

## 修订记录

- 2026-11-16 v1：引擎侧首版设计；确认方案 A、三态范围、部分继承/alpha=0、主题状态路由、`.hover` 后缀键与 Get 生效语义、缓存不含状态名、显式采集补强；待评审。
- 2026-11-16 v2（按复核 B1~B8）：主题写入位掩码 + 重算前清理（B1）；固定 5 级声明应用顺序，状态选择器胜出、拆散键优先（B2/B3）；控件支持矩阵 + 统一生效解析入口 + 不支持诊断（B4）；显式匹配检查状态槽 `*Set`（B5）；Get 生效值限制说明（B6）；缓存复用/不复用测试（B7）；`:selected` 等同 pressed 说明（B8）。
- 2026-11-16 v3（按复核 B9/B1a/B4a/B10/B11~B13）：掩码移入 `ControlImpl`（生命周期随控件、ThemeRuntime 布局不变）；明确 normal 沿用主题累积语义不清理；清理遍历与应用遍历分离、空主题 no-op；矩阵补 Popup/ConfirmPopup/Dialog/ContextMenu；补 S13 部分 normal、S19 ContextMenu、S20 specificity/order。
- 2026-11-16 v4（设计通过）：补 `ApplyTheme("{}")`/仅变量主题 no-op 语义与 S13a；补 S13b 显式 alpha=0 清空、S13c ClearThemeOverrides 掩码；明确 hover 回退的是 A 的 normal 而非“无阴影”；去掉支持矩阵 Dialog 重复行与 §5 重复 C++ 行；确认 ContextMenu normal/状态键均诊断、掩码最终放在 `ControlImpl`。
- 2026-11-16 v5（实施）：完成状态槽/主题映射/显式采集/清理与测试；三后端 56/56；补 §11 实施记录。
- 2026-11-16 v6（代码检视 CR1/CR2）：MenuPanel 只按 normal 判定并诊断状态键；`clearOverrides` 语义定案并同步设计/S13c；测试 65/65。
- 2026-11-17 v7（Label/Shape 检查 + 截图）：Label/Shape 状态阴影验证通过；状态规则 text-shadow 非颜色键忽略 + 诊断；主题截图改浅色背景；测试 76/76。

## 12. 最终视觉验证（2026-11-17）

- 设计器验收通过：normal/hover/pressed/disabled 四态；状态字段部分覆盖/继承；状态 `color` alpha=0 显式关闭；主题 `:hover`/`:pressed`/`:disabled`/`:selected`；截图与文档复核。
- Label/Shape 检查通过；MenuPanel/MenuItem/HandleControl/ContextMenu 支持矩阵与诊断符合约定。
- `test_cr_shadow_state` 三后端 77/77；`validate_layout --strict` 通过。
- 结论：设计器/引擎两项均 closed，满足归档门禁；无代码变更。
