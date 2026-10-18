## Session History

### 2026-10-03 ~ 10-18: 设计器协作批链 P0-62~P0-101（属性/拖拽/页签/菜单/OS 拖放；配合清单第三批全程闭环）（Complete）

**范围**：覆盖 P0-62~P0-101（2026-10-03 起的设计器协作批链；此前 09 月批次记录暂缺）。流程：设计器清单 → 引擎分析 → 设计文档（待评审）→ 设计器复核（归档 `requirements/*_复核意见.md`）→ 实施 → 探针/全量回归 → make_release → subModules 同步 → 清单镜像。

**批 P0-62~P0-65（commit `bd9d517`）**：item 着色与字体名（ListView cell 字体名/大小、item 字体覆盖）、统一行式跟随（hover 高亮跨行同步）、hover 彩色与叠加语义、hover-only 修复（`CellStyle.hasBg` 显式位 + 门控 `hasBg && alpha>0`；`TreeNode.hasHoverBg` 与 bgMask 解耦、显式 hover 独立绘制）。

**批 P0-66~P0-88（commit `4a97783`，99 文件）**：ListView JSON 缺口（columns header-font-*、rows 对象 cell 解析）；auto-scroll（TextArea，缺省 false）；MenuBarRemoveMenu / TabRemovePage（保活池 + TabSetPage 双挂载修复）；子视口弹层与资源管线（弹层 bench 局部定位、resourceProviders 目录挂载、shadow-radius、colors-menu 四态、动画 provider 描述、Bench 透明+viewportBackground、MemoryResourceProvider filesystem 回退、provider 回退继承、LayoutParser::adoptContext）；菜单交互闭环（悬停残留修复、菜单项 id 注册、active-index/hovered-index 只读）；输入通路与校验硬化（跨子视口鼠标捕获、keyboardTarget 焦点路由、`validateControl` 全构建归属校验 + `Debug_ValidateHandle`、EditBox/TextArea read-only、ComboBox 吞输入修复）；只读/解绑（空回调=解绑防 bad_function_call）；logMissingHandler 诊断。

**批 P0-89~P0-94（commit `fa68932`）**：Tab 五色键 + `TabGetItemRect`/`TabMovePage`/`hovered-tab` + **TabSetPage UAF 修复**（先强引用再 detachPoolTake）；tab JSON 颜色解析 + schema 6 键（selected-text/page-background）；**通用拖拽框架 Phase 1+2**（`DragManager`/`DragDrop.h` 钩子/9 键/6 事件/语义载荷与落点/内置换序/公开 `TreeViewMoveNode`/`ListViewMoveRow`）；**`fireCCallback` virtual 继承句柄修复**（`static_cast<Control*>`，影响全部既有回调）；用户手册新增 5.7 拖拽框架页 + 导航重排。

**批 P0-96~P0-97（commit `1812159`）**：reorderable 自含（门控 `isDraggable||isReorderable`、载荷 CONTROL 判据、`findDropTarget` 自目标回退、tab 单轴落点）；ghost 字体 provider 口径（`loadFontFromMemoryWithText` + 码点计宽 + 失败日志带路径）。

**批 P0-98~P0-101（工作区，未提交）**：菜单动态化（Normal 无条件关链+click、ActionThunk 动作回调实例上下文、`MenuPanelRemoveItem`、setCaption 重算）；菜单屏幕钳制（子菜单翻转/收边 + 顶部兜底、顶层收边）；OS 文件拖放（`EventType::DropFile` + `SetDropFileCallback` + `UI_EVENT_DROP_FILE` 合成）；**UIEvent 桥缺口修复（P0-101）**——`bridge_pollEvent` 原 `default: return 0` 静默丢弃 DropFile（合成通路不过桥、极具迷惑性）→ 指针式桥接（超长路径不截断）+ `CallbackInputBackend`/`uiEventToEvent` 转换 + `PushUIEvent` 深拷贝 + MainWindow Mode 1 分发。

**本批链重点问题记录（已修复，详见各设计文档）**：① `fireCCallback` 首参句柄（ControlImpl virtual 继承偏移 0x488）→ 宿主回调内调 CABI 段错误；② ComboBox 非编辑态无条件吞 TextInput（P0-84 内联输入失效链根因）；③ TabSetPage UAF；④ 设计器手柄跨实例越界（`validateControl` 全构建校验立功）；⑤ P0-96 实施中发现 hint→索引摘除位移未补偿（tree 同父 BEFORE/AFTER 偏移、tab 槽位换算）；⑥ P0-101 UIEvent 桥无 DropFile 转换（真实 OS 拖放丢失；DLL md5 核验无法发现）；⑦ 菜单点击门控（动态项回调死+菜单不关）；⑧ 菜单悬停残留（P0-82）。

**流程与铁律**：AGENTS.md 增测试运行约定（auto=5/10 + shell timeout + build/test/Debug + `testing_scripts/run_regression.sh`）与**文件夹访问范围铁律**（仅两项目目录；设计器除 subModules 外严格只读，唯一写入=发布同步）；用户手册多轮刷新（5.7 拖拽框架、tab-*、read-only、viewport、provider、菜单钳制/动态化、debugging 调试 API 等）；设计文档 P089~P101 全量状态/问题/实施记录 + P093_094 §7 开发参考；API_Mapping_Table 增近期批次补充表。

**验证基线**：全量回归 51 OK + 仅 4 项预存失败（test_menu_cabi / test_multiviewport / test_multiviewport_cabi / test_treeview_cabi）；各批探针全过（P0-93/94 8/8、P0-96/97 8/8、P0-98/99/100 9/9、P0-101 5/5）；Debug/Release 零警告；发布同步 171 文件（2026-10-18）。

**相关文件**：src/（UICornerstoneAPI/Menu/TabControl/TreeView/ListView/ControlBase/CallbackAdapters/MainWindow/LayoutParser/backend/BackendBridge.h/backend/sdl3/InputBackend.cpp）、include/（UICornerstoneAPI/PropertyNames/DragDrop/EventTypes/StateMachine/UIContext/Menu/TabControl/TreeView/ListView）、binding/（UICornerstone/Control/DynamicApi/Impl）、design/P089~P101 设计文档、requirements/（清单镜像 + P0xx_复核意见 归档）、docs/（advanced/drag-drop 5.7、appendix/*、controls/menu/tabcontrol、schema）、AGENTS.md、testing_scripts/run_regression.sh。


### 2026-08-30（晚）: 属性常量独立列 + 多属性拆行 + 枚举值速查（方案 C）

**用户反馈修正**：不以括号内联常量；属性表新增「属性常量」列；多属性同行改为每属性一行。

**实现（Temp/build_constmap.py 权威映射 + 转换管线）**：
- 权威映射：`strcmp(prop, PropertyNames::kX)` / `==` 分发上下文（239 条）+ kEvent* 事件族补充
  （269 键）；属性表（表头含 `<th>属性</th>`）内：去括号注 → 首 td 多 `<code>` 拆行为每属性一行
  → 常量列（无分发映射 → `—`，仅 caption-size 两处（有意 JSON-only））
- 键名纠错：tree-expand→expand(kTreeExpand)、tree-collapse→collapse(kTreeCollapse)、
  current-index→currentIndex(kJsonCurrentIndex)、textarea alignment→align(kAlign)
- 源码配套：PropertyNames 补 kViewportScaleMode；Bench.cpp 改用常量
- 7.3 枚举值速查（66 组，生成式：值常量族扫描+在用标注旧名/未使用）；22 个控件页
  属性表后加「枚举值见 7.3」引用；capi 8.1.1 后端能力位宏表（5 宏）
- 二次核验：503 项常量列 0 误映射；仅 expand 双常量上下文例外（Label kExpand /
  TreeView 命令式 kTreeExpand，均为正确使用）

**相关文件**：docs/controls/*.html(23)、docs/appendix/{properties,capi}.html、
include/PropertyNames.h(kViewportScaleMode)、src/Bench.cpp、
Temp/{build_constmap.py, table2col 管线}、design/guidelines/history.md


### 2026-08-30（文档补注）: 用户手册属性表补 PropertyNames.h 常量名（482 处）+ 源码交叉核验

**改造**：脚本（Temp/build_constmap.py + Temp/annotate_docs.py）解析 PropertyNames.h 生成
key→常量映射（直名优先：key 的 PascalCase 即 kXxx 才标注，子键/歧义不猜测——202 条直名），
按"属性表首个 <td> 内全部 <code>键</code>"逐表补注格式 `<code>key</code> (<code>kXxx</code>)`：
- docs/appendix/properties.html（+256）与全部 23 个控件页（+226）
- 后端能力（GetBackendCapabilities 宏）：不在 PropertyNames 域，另行说明待补

**源码交叉核验（两轮）**：
- L1：常量字符串出现于控件源/基类/通用集 —— 0 问题
- L2：`PropertyNames::kXxx` 出现在属性分发上下文 —— 0 问题
- 全库级：仅 kCaptionSize 无任何源码引用——经核为**有意的 JSON-only 键**
  （Button 页已注明"仅布局 JSON 生效"；properties 表同步补注保持口径）

**相关文件**：docs/appendix/properties.html、docs/controls/*.html（23 页）、
Temp/{build_constmap.py, annotate_docs.py, verify_doc_keys.py, constmap.json}（保留供再生成）。


### 2026-08-30（补）: Actor/LuotiAni/Material 守卫补齐（独立控件族）

**修正豁免误判**：Actor/LuotiAni 为独立控件（Material : ControlImpl 基类），本轮补：
- `Material::draw(void)` / `Actor::draw(void)`（无参控件入口）加 `m_visible` 守卫
  （LuotiAni 无 draw(void) 覆写 → 经 Material 入口；其 `update()` 已有守卫）
- 带参重载（draw(x,y,alpha)/draw(frameNo,x,y,alpha)）**保留为显式合成原语**：
  供离线渲染/编辑器主动绘制（加守卫会破坏该用法）
- handleEvent：三者无覆写 → 基类（含 enable+visible）；isContainsPoint 显式 false（纯显示不命中）
- 回归：test_animation/test_aniviewer/test_image/test_luotiani exit=0
- 相关文件：src/Material.cpp、src/Actor.cpp


### 2026-08-30: 全库 visible/enable 守卫系统性补齐（draw/handleEvent/update 覆写）

**背景**：TabControl 页面放 ListView 切换不隐藏 → 根因 `ListView::draw` 覆写缺 `m_visible` 守卫；
用户指出「beforeDraw 内检查无效，不影响 draw 自身逻辑」→ 全库系统扫描全部
draw/handleEvent/update/beforeDraw/afterDraw 覆写的入口守卫（脚本 Temp/scan_guards.py 分类）。

**补齐清单**（入口守卫，不再依赖 beforeDraw）：
- draw 补 visible：ListView、Shape、TabControl、NumericUpDown、ComboBox、HandleControl、StatusBar（固化）
- handleEvent 补 enable+visible：Popup、ConfirmPopup、ColorPicker、HandleControl
- update 补 enable：EditBox（ComboBox/TextArea/NumericUpDown 继承受益）、ComboBox、TextArea、NumericUpDown
- 语义豁免：Actor/LuotiAni/Material 为非控件演员（父绘制链已守卫）；draw 不要求 enable
  （disabled 仍需绘制灰显）；Dialog::handleEvent 经 ConfirmPopup 链式守卫成立
- 回归：test_tabcontrol（+ListView 页切换用例 24/24）、listview/shape/dialog/numericupdown/combobox exit=0

**相关文件**：src/{ListView, Shape, TabControl, NumericUpDown, ComboBox, TextArea, EditBox,
StatusBar, ColorPicker, HandleControl, Dialog}.cpp、test/test_tabcontrol.cpp、Temp/scan_guards.py（保留）。


### 2026-08-29（晚三）: 设计文档与用户手册核对（字体键/继承 + schema 一致性修正）

**设计文档**：
- `ControlBase_Design.md` 4.4.1「字体上下文（JSON 字体链）」：3 个 API + 继承规则
- `LayoutSystem_Design.md` §6.4 追加「通用字体键与父链继承」说明与优先级（控件显式 > 父链 > Theme）
- `JSON_Schema_Design.md` 语义类型述补 `font-spec`（{name,size,style}）与 common font/fontSize

**用户手册**：
- `docs/declarative-ui.html` 与 `docs/appendix/declarative-syntax.html`：「通用字体键（v1.1.1）」说明（font/fontSize + 父链继承）
- `docs/appendix/properties.html` 7.1 通用属性表补 font/fontSize 两行
- `docs/controls/treeview.html` JSON 示例加 font 键 + note（含继承）

**schema 一致性修正（盘点时发现）**：combo-box/edit-box/progress-bar/slider/text-area 的 `font` 原引用枚举 `font-name`（与实际对象 JSON 不符）→ 全改 `font-spec`；`font-spec` 补 `style`（label 组合语义 oneOf array|string）。validate_layout strict 全 PASS。

**相关文件**：design/ControlBase_Design.md、design/LayoutSystem_Design.md、design/JSON_Schema_Design.md、docs/{declarative-ui.html, appendix/declarative-syntax.html, appendix/properties.html, controls/treeview.html}、docs/schema/declarative-ui.schema.json（+release/tools、designer subModules 同步）。


### 2026-08-29（晚二）: JSON 字体键通用化 + 父链继承（v1.1.1 增）

**背景**：CornerstoneDesigner 反馈 tree-view 字体设置被忽略（schema 未声明 font/font-size/fontSize 键，validate 报滞后；parseTreeView 也不读面板级字体）。

**1. 通用字体键**：`parseCommonProperties` 末尾统一 `applyFontDecl`：读取 `font{name,size}` / `fontSize` → 写入 ControlImpl 字体上下文（新 public API：setFontContext/getFontContext*/hasExplicitFont）+ 动态应用（Label/EditBox(含 ComboBox/TextArea)/ProgressBar/TreeView/TabControl/StatusBar/MenuBar 的 setFontSize 与 setFont）。

**2. 父链继承**：parseLayout 末尾 `resolveFontInheritance` DFS：无显式声明的控件沿父链采用最近显式字体（应用 setter + context 记录）；根或全未声明时保持主题默认。

**3. schema**：新增 `font-spec` 定义（{name,size}）；`common` 增加 font/fontSize 两键（全控件树生效）；menu-bar 的 font 改 $ref font-spec（原枚举与对象不符）；validate_layout strict 全 PASS（含 designer main_layout）。

**4. 测试**：test_label 新增 JSON 字体用例（继承父 20 / 覆盖 24 / 便捷键 fontSize 16 → 全过）；test_treeview/test_layout/test_menu/test_splitter 回归全部 exit=0。

**相关文件**：include/ControlBase.h（字体上下文 API）、include/LayoutParser.h、src/LayoutParser.cpp（applyFontDecl/ApplyFontToControl/resolveFontInheritance）、docs/schema/declarative-ui.schema.json、test/test_label.cpp、docs/declarative-ui.html（通用字体键说明）、docs/schema 同步 release/tools 与 CornerstoneDesigner subModules。


### 2026-08-29: Splitter 拖拽超动根因（双重累加）+ 三后端回归（Complete）

**用户痛症**：手动拖动任意蓝绿分条"移动距离比鼠标大、不跟手"（测试断言却一直 1:1）。
**排查链**（逐层实证）：合成事件 1:1 vs 手动 log 缺失 → DIAG 上限耗尽 → SP-HIT/SP-DRAG/SP-VS
全链路打印 → 真实命中 jSp1(310,500) → [SP-LAYOUT] 显示每帧 delta 与面板增量一致，
但 SP-VS 显示 sp 逻辑位移 296 vs 鼠标 27 → 逐帧拆分（首帧 +1 / 第二帧 +2 … 第十二帧 +12）
发现：**target = 当前宽(已含上帧 delta) + 累计 delta → 双重累加滚雪球**（13 帧累计 +74 =
鼠标 +13 的 5.7 倍）。
**修复**：startDrag 记录拖拽起始前段/后段宽（m_dragStartSegW/m_dragStartRearW），
updateEngineDrag 三式全部改"固定基准 + 累计 delta"；坐标换算统一 mapViewportToCanvas
两次差。**回归**：新增"分段拖拽 10×1px → A=210（无累加）"用例（三后端 0 失败 =
sdl3/sfml/raylib 各 22+ 断言全过）。临时 DIAG 全部移除。
**文档**：Splitter_Design §8.4 更新固定基准公式与禁止项。
**相关文件**：src/Splitter.cpp / include/Splitter.h（m_dragStartSegW/RearW）、
test/test_splitter.cpp（分段拖拽+恢复用例+取消 panel 颜色保留）、design/Splitter_Design.md§8.4。


### 2026-08-28（晚）: Splitter × 布局引擎彻底融合（引擎模式/双式拖拽/手柄模式/跟手换算）

**问题**（CornerstoneDesigner 推进反馈）：① JSON splitter 极简配置解析后卡死（引擎 reflow↔Splitter 联动递归环：applySplitRatio 清 m_lastRect + 引擎↔面板互写）；② 多 Splitter 共享弹性面板冲突（applySplitRatio 独占父容器假设）；③ 群拖拽不跟手/移动比例不对（缩放模式 delta÷scale 双因子）。

**1. 引擎分段模型（Splitter_Design §8 新增）**：H/V flow 引擎中 Splitter 成为一等元素——FlowElementWidth（thickness 流）、FlowGapAfter（splitter 前后 gap=0）、任意数量 splitter 链式累计位置；LayoutEngine 基类补 getLastFlexUnit。

**2. 两式拖拽语义**：前段固定→改前段固定宽（左界）；前段弹性&后段固定→改后段固定宽（右界，弹性自动补偿）；双弹性→降级前段锁定。多 splitter 天然链式（每次 reflow 全链重算）。

**3. 手柄模式**：无 linked / 无引擎容器也可拖（自身移动 + ratio 上报），CornerstoneDesigner 早期绕行方案正式化；驱动 guard 三模式互斥（引擎 > linked 自动 > 手柄）。

**4. 跟手换算修复**：updateDrag 坐标改用 mapViewportToCanvas 两次求差（视口像素→画布逻辑，含根/视口缩放全链），移除「delta÷父 scale 双因子」；linked 自动/引擎/手柄三模式统一，zoom 2x 下 40px=20 逻辑验证。

**5. 防重入**：applySplitRatio 加 m_applyingRatio guard（修复卡死主因）；Splitter::setRect 引擎模式不再触发 apply。

**6. 测试（test_splitter 扩展）**：引擎三栏（+37.5 非整 1:1、往返、链式）、四栏三分隔条（双弹性降级/式2）、嵌套（弹性面板内 v-flow 两条水平 splitter）、JSON 声明式（不卡死回归）、手柄模式、zoom 2x 跟手——三后端（sdl3/sfml/raylib）全过 0 失败。

**相关文件**：include/LayoutEngine.h、src/LayoutEngine.cpp（分段+flexUnit）、include/Splitter.h、src/Splitter.cpp（isEngineManaged/updateEngineDrag/两式/手柄/guard/map 换算）、include/Panel.h（getChildFlowWeight）、test/test_splitter.cpp（+5 用例函数）、design/Splitter_Design.md（§8+§8.9）、design/guidelines/history.md（本条）。


### 2026-08-28（下午）: 运行期窗口 API（Size 查询/设置 + Resize 回调）+ CornerstoneDesigner v-flow 问题根治（Complete）

**背景**：CornerstoneDesigner 的 App::SdlApi 动态解析 SDL3.dll 三函数（GetWindowSize/SetWindowSize/GetWindows）做窗口 resize 同步——绕过后端抽象（跨后端失效、headless 语义丢失）；设计评审定案：统一提供运行期窗口 API（§21 文档先行）。

**1. 文档先行**：`design/BackendAbstraction_Design.md` §21（动机/API 清单/三后端差异表/能力位/验证）——sdl3=SDL_SetWindowSize、sfml=setSize、raylib=SetWindowSize（headless 空操作+句柄 NULL）。

**2. 核心+后端**：`Window` 抽象加 `virtual setSize(w,h)`（sdl3/sfml/raylib 覆写；raylib headless guard）；`UIBackendCallbacks` Window 组加 `setWindowSize`（可选）→ `BackendBridge.h` `bridge_setWindowSize` + 三后端 cb 注册；`CallbackWindow::setSize` 转发（插件模式损坏点：此前无协议入口）。

**3. C ABI**：`GetWindowSize / SetWindowSize / GetNativeWindowHandle / SetWindowResizeCallback`（4 函数）；`UIContext` 加回调存储；统一 `DispatchWindowResize`（pumpInstanceEvents+注入队列两通路）并在 MainWindow 分发；能力位 `UICORN_BACKEND_CAP_WINDOW_SET_SIZE (1u<<4)` 三后端声明。

**4. Binding**：4 方法（GetWindowSize/SetWindowSize/GetNativeWindowHandle/SetWindowResizeCallback<std::function>，C thunk+Impl 持 shared_ptr 保活）+ DynamicApi RESOLVE×4。

**5. 测试**：`test_window.cpp`（新，10 断言）：初值 1200×800、cap 位、nativeHandle、SetWindowSize 900×700 读回（±1）、注入 WindowResize→回调≥1 且值对、取消后不再触发；三后端 PASS（sdl3/sfml 10/10、raylib 9/9 无 MULTI_WINDOW 断言）；test_menu/test_dialog/test_multi_instance 回归 exit=0。

**6. 文档**：capi.html 8.1 与 binding.html 速查表补 4+1 条目（v1.1.1 增）。

**相关文件**：include/Window.h、include/UICornerstoneAPI.h、include/UIContext.h、src/UICornerstoneAPI.cpp、src/MainWindow.cpp、src/CallbackAdapters.h/.cpp、src/backend/BackendBridge.h、src/backend/{sdl3,sfml,raylib}/Window.cpp/BackendPlugin.cpp、binding/src/(DynamicApi.h/.cpp、Impl.h、UICornerstone.cpp)、binding/include/UICornerstone.h、test/test_window.cpp、test/CMakeLists.txt、design/BackendAbstraction_Design.md（§21）、docs/appendix/{capi,binding}.html、design/guidelines/history.md（本条）。

### 2026-08-27: MenuBar 容器布局 + 下拉弹出置顶 + 全局关闭（v1.1.1 增强，Complete）

**背景**：CornerstoneDesigner（v1.1.1）开发中发现：menu-bar 原为"不挂控件树、顶层独立"控件，不参与 v-flow 布局——v-flow 内放 menu-bar 不被正确重排。用户拍板方案 A：容器内自动手动定位 + 弹出面板挂顶层（缩放与 Popup 一致）+ 全局点击/ESC 关闭；文档随后刷新。

**1. 自动手动定位（容器内模式）**：`MenuBar::setParent` 父非 Bench（挂入 panel/v-flow 等布局引擎容器）→ 自动 `m_manualPosition=true`，放弃 `layoutEntries` 全宽贴顶重置，rect 完全由布局引擎驱动（JSON 无需显式 manual-position）。

**2. 下拉弹出置顶（Popup 同款浮层模型）**：`openMenu→attachMenuPanel` 将 MenuPanel 摘离 MenuBar 父链、挂 BENCH 顶层（子列表末尾=绘制/事件最上，不被 v-flow 兄弟遮挡）；坐标=MenuBar 绘制矩形+hitRect×复合缩放；`recalculateSize` 在挂树后执行（÷面板复合），显示尺寸=逻辑×面板自身复合（解析路径面板与栏同 scale、视觉不变）；`detachMenuPanel` 关闭时摘回父链。`MenuBar::refreshScaleWith` 跳过已挂 Bench 面板（防双重缩放）；`MenuBar::draw` 挂顶层面板不再手动绘制（Bench 负责）仅离线退路手绘；析构经 `UIContext::isActive` 防护后 closeAllMenus（修复退出期析构 segv）。

**3. 全局关闭（单菜单交互语义）**：MenuBar 打开时注册 beforeEventHandlingWatcher（KeyDown+MouseDown，同 Popup），关闭注销：点击栏/面板（含子菜单）内放行；其它任何位置（其它 MenuBar/控件/空白）先 `exitMenuMode` 再放行（目标控件随后打开自身菜单）；KeyDown ESC 关闭并吸收。不依赖"事件恰好流到 MenuBar"。

**4. EventQueue 两个潜在 bug 修复**：`notifyBefore/AfterEventHandlingWatchers` 遍历期间 watcher 回调注销自身 → ①回调内锁重入同一互斥锁（死锁）；②遍历中改 vector（迭代器失效 UB）。改为"锁内快照拷贝 + 锁外执行回调"。

**5. 测试（test_menu.cpp）**：v-flow 用例——自动 manual、随流定位 (8,8)/宽384、后续项不重叠、弹出挂 Bench 顶层+子列表末尾（未被遮挡）、watcher 全链路（pushEvent→eventLoopEntry）点击其它 MenuBar 先关后开、ESC、关闭摘回；既有 2 乘断言语义按 Popup 语义更新（显示=逻辑×面板复合）。三后端（sdl3/sfml/raylib）各 2 轮 exit=0 全过；test_dialog/test_contextmenu/test_layout 回归 exit=0。

**6. 文档刷新**：`design/Menu_Design.md`（2.2 布局模式、类成员 m_manualPosition、3.1/3.3 ESC 实施状态、3.5 新增弹出置顶与全局关闭）；`docs/controls/menu.html`（4.14 说明布局模式、manual-position 属性补充、4.14.4 JSON v-flow 容器内示例、4.14.8 新增小节、4.14.7 方法表补 set/getManualPosition）；`docs/appendix/declarative-syntax.html`（menu-bar 标题与说明）。README.md 无需变更。

**相关文件**：src/Menu.cpp / include/Menu.h（attach/detachMenuPanel、register/unregisterMenuWatcher、beforeEventHandlingWatcher、setParent 自动 manual、~MenuBar、refreshScaleWith、draw）、src/EventQueue.cpp（watcher 快照遍历）、test/test_menu.cpp（v-flow 用例+watcher 全链路断言）。

### 2026-08-25: 五控件批量实施（ListView/StatusBar/ContextMenu/TabControl/Shape）+ 视觉验收 + 焦点两层作用域 + 缩放体系 + 验收收尾（Complete）

**背景**：用户指示串行实施 5 个新控件（全部完成后统一验收质量）；随后逐轮视觉验收反馈修正、编程规范修正、按 ListView_Design 格式重写设计文档、按 AGENTS 新增控件验收清单（14 项）完成验收收尾。

**1. 四控件实施（四层接入：核心类/LayoutParser JSON/C ABI/C++ 类即 Binding）**：

- **ListView**（13b3b3c）：multi/single 双模式（single=ListBox 替代）、列头排序（stable_sort+自定义比较器+排序后选中按 id 跟随）、列宽拖拽（±4 命中+minColumnWidth 钳制）、双向滚动（可见行窗口化+列头同步）、行/列/单元格三级 leadingControl+cellStyle 稀疏样式、Ctrl 多选、键盘循环；19+28 断言。
- **StatusBar**（221438f）：VSCode 蓝底左右对齐段（text/icon/onClick/MenuPanel 弹窗），弹窗直接挂载目标段 MenuPanel（共享复用，弃复制方案）+ 无 onClick 项补 no-op onClick 触发 closeMenuChain 自动关闭；20 断言。
- **ContextMenu**（28e4476）：ContextMenu : Popup + MenuPanel（零新增核心逻辑）；打开三路径（纯 API/控件右键 hook/JSON 控件级 contextMenu 键）；点项包装 onClick 用户回调后自动 close；isContainsPoint 含子菜单防外点误判；18→19 断言。
- **TabControl**（7fd7343）：四方向一体自绘（指示条贴内侧缘）、页面挂子树切换、键盘导航（方向键循环+Home/End）、removeTab 钳制、onTabChange；40 断言。
- **Shape**（会话前已存在）：本轮补多图元组合（primitives：单控件拼合图形，C++/JSON/CABI 三层）、colors.background（setBackgroundStateColor 自动取消透明）、空心圆描边闭合修复（drawEllipse 的 strokePath closed 参数误传 false）。

**2. 视觉验收逐轮修正（用户截图反馈驱动，像素级定位）**：

- StatusBar 文字不渲染：ensureFont 从未被调用 + `!data || !data->empty()` 条件写反；字体首次就绪后 relayout 自愈估算宽度。
- StatusBar 弹窗不可见：子控件 getDrawRect 会复合父偏移，绝对坐标定位实际画到屏幕外 → 改父相对坐标 + 复用 item.menuPanel（原空面板未装项）。
- ContextMenu 真实右键不弹出：SDL3 InputBackend 裸 static_cast 鼠标键（SDL 1=左/2=中/3=右）与 MouseButton 枚举（Right=2）错位 → mapSdlButton 显式翻译；修正 EventTypes.h 误导注释。
- TabControl 多实例键盘一起动作：KeyDown 无焦点门控 → 仅 getFocused() 响应 + 点击页签 focusControl(this)。
- ContextMenu 双框线（底/右 1px 错位）：诊断色定位法（品红/青）锁定 = Popup::create() 无条件重置 chrome（transparent=false/borderVisible=true）冲掉 ctor 抑制，默认边框(83,83,90)与 MenuPanel 圆角描边(69,69,69)在底/右错开 1px → ContextMenu 重写 create()+applyChromeSuppression() 三点防御（ctor/create/show-open 后）。
- StatusBar leadingControl 偏低：像素实测 Label 行偏移为常量 ≈+8（基线特性）→ 对齐基准回归槽内几何居中（内容无关），测试图标改 Shape 几何内容；StatusBar 命中补 Y 轴二维判定。
- ListView 图标被行背景覆盖：ControlImpl::draw()（子控件）原在 draw() 开头调用 → 移至 popClipRect 后。

**3. 缩放体系（三层规则，2.0x 对照验收）**：布局本地化（relayout 与 scale 无关）/ 自绘 = drawRect 原点+local×scale（字体按 fontSize×scaleXX 加载）/ 命中 = 屏幕坐标二维逆变换。五控件全量修正（原自绘误用 m_rect）+ 各测试补 normal vs 2.0x 对照行。leadingControl 特例：未挂树无父复合 → setRect 用绝对坐标；未挂树控件须显式 setContext+create 使 RenderDevice 就绪。

**4. 焦点两层作用域（用户拍板，FocusSystem_Design §4.3 精确化）**：TabControl = focus boundary 且**边界容器自身归父作用域**（findFocusScope 自父级起查）——Tab=层内循环（页内控件间 / 页签条+外部）、Ctrl+Tab=层切换（页内↔页签条，focusNextScope/PrevScope 层规则重写）；Bench 为根边界；FocusManager 新增 isEffectivelyVisible（任一祖先隐藏即不可达，修隐藏页控件可被 Tab 聚焦缺口）；setFocusBoundary 落地注册 m_boundaries。测试时序坑：合成 KeyDown 必须置 mod=KeyMod::None（事件未零化，垃圾 mod 被 Bench 误判 Shift→focusPrev）；焦点断言与 Tab 派发同帧（窗口后台化真实失焦事件会清焦）。

**5. 编程规范修正（用户指示）**：五控件魔鬼数字全部具名化（文件级 constexpr k 前缀，kScrollbarW 先例——TabControl 7 色+5 系数、StatusBar 3 色+4 系数、ListView 10 项、ContextMenu 视口兜底）；无用导入清理（StatusBar algorithm/cmath、Shape cmath、TabControl Bench.h/MainWindow.h、ContextMenu Bench.h）。

**6. 设计文档按 ListView_Design 格式重写（6018021）**：StatusBar/ContextMenu/TabControl 三份统一为 ListView 格式（修订注记版本史/现状调研行号/架构决策对比表/缩放三层/焦点系统/属性四层矩阵/JSON/CABI/Binding/文件清单/测试策略/决策点/边界）；修正 StatusBar 文档 JSON 键错误（实际 camelCase fontSize/itemHeight、align:right）；Shape_Design.md 多图元/背景色/缩放回写仍待下轮（唯一遗留）。

**7. 验收收尾（d6a9d3e，AGENTS 14 项清单）**：PropertyNames 补 kJsonContextMenu/kJsonCells/kJsonControl/kJsonOnClick + LayoutParser 裸字面量全常量化；JSON Schema 新增 status-bar/tab-control $defs + shape primitives + list-view columns.icon + common.contextMenu（JSON 无 BOM），tools 重构建 validate_layout --strict PASS；layouts/all_controls.json 追加房子组合/状态栏/选项卡样例；README 控件清单 20+；用户手册 docs/controls 新增 5 页（checkbox 骨架同款）+ nav.js 4.22-4.26；API_Mapping_Table 新增 §22-26 五节（原 §22 顺延 §27）+ 工厂 25→30；**三后端回归**：sdl3/sfml/raylib 五控件测试全绿（shape 17 / listview 19+28 / statusbar 20 / contextmenu 19 / tabcontrol 40）。

**8. 手册导航修复（338989c + d1d0066）**：nav.js 追加条目缺尾逗号致整站导航失效（JS 语法错误）→ 补逗号 + node --check；首页 docs/index.html TOC 补 4.22-4.26；新控件条目全文档**字母序**统一（ContextMenu/ListView/Shape/StatusBar/TabControl，手册页编号 4.22-4.26 重排、API_Mapping §22-26 重排）；全站 367 href 逐一定位 0 断链；controls/index.html 继承树与 declarative-ui.html 控件类型速查补五控件。

**9. 关键教训**：① 源文件必须 UTF-8 with BOM（MSVC 按 GBK 误析中文注释致类定义崩溃，本会话 Shape/TabControl/测试文件多次踩坑）；② 子控件 getDrawRect 复合父偏移——挂树子控件用父相对坐标、未挂树辅助控件用绝对坐标，二者不可混淆；③ 事件结构体未零化（mod 垃圾位）与 printf %d 读 float 都会造成"假数据"误导排查；④ 改 JS/JSON 等格式敏感文件必须整文件语法校验（nav.js 缺逗号、JSON BOM 两次教训）；⑤ 诊断色定位法（品红/青临时着色）对"框线来源"类问题高效；⑥ 自动化测试窗口后台化会收到真实失焦事件——焦点断言须与按键派发同帧。

**验证**：sdl3/sfml/raylib 三后端五控件测试全绿（shape 17+7CABI / listview 19+28CABI / statusbar 20 / contextmenu 19 / tabcontrol 40）；validate_layout --strict PASS；全站手册 367 链接 0 断链；node --check nav.js 通过。

**相关文件**：include/{Shape,ListView,StatusBar,ContextMenu,TabControl,Menu,FocusManager,ControlBase,PropertyNames,UICornerstoneAPI,EventTypes}.h、src/{Shape,ListView,StatusBar,ContextMenu,TabControl,LayoutParser,UICornerstoneAPI,FocusManager,ControlBase,Bench,RenderDevice}.cpp、src/backend/sdl3/InputBackend.cpp、test/test_{shape,listview,statusbar,contextmenu,tabcontrol}.cpp、test/CMakeLists.txt、layouts/all_controls.json、docs/schema/declarative-ui.schema.json、docs/controls/{5 新页}.html、docs/assets/nav.js、docs/{index,declarative-ui}.html、design/{Shape,StatusBar,ContextMenu,TabControl}_Design.md、design/API_Mapping_Table.md、README.md。

---

### 2026-08-17: TreeView 行前置控件容器 + 逐 Item 字体 + GetControlType 机制重构（Complete）

**背景**：TreeView 增强设计（v7）实施收尾：Item 行前置控件容器（图片/CheckBox 等）、逐 Item 字体（JSON + CABI），并重构控件类型查询机制。

**1. 行控件布局迭代（v4→v6，用户逐轮拍板）**：槽起点 = 原文本起点 labelX（= arrowX + arrowGap，局部 20）；槽高 = 行内文字高度 fontH（无字体回退 rowH），行内垂直居中；槽宽 = slotH × ratio（优先纹理宽高比，退化 1:1）；文字 = 槽右缘 + leadingGap。局部坐标 `localX=(slotStartX-cr.left)/scaleX`、`localY=(y+(scaledRowH-slotH*scaleY)/2-cr.top)/scaleY`（禁止 setRect 绝对坐标，双重偏移同款）。test_treeview 14/14 断言三后端通过。

**2. JSON item 增强**：`leadingControl`（复用 parseControl type 分发，parent=nullptr，缺 rect 自动补 `{0,0,0,0}` 占位）/ `leadingGap` / `font`（枚举）/ `size`（int，0=继承）；ENH_JSON 用例 16/16 断言三后端通过。图片纹理加载根因：测试传 `string` 精确匹配 Actor 资源 ID 重载（loadFromResource），文件路径须 `fs::path`（loadFromFile）——框架无 bug。

**3. CABI item 级属性（item-id 定位模式）**：`SetString("item-id")` 定位 → Set/Get Float/Int/Enum/Ptr；test_treeview_cabi LAYOUT_JSON 增强（n2 check-box / n3 粗体 18 / n6 leadingGap 12）4 组断言 DLL 全过。**运行时挂树修复**：create() 之后 `SetPtr("item-leading-control")` 原本不挂树（syncRowControls 只在 rebuildFlatRows 调用），现 SetPtr 成功后立即 syncRowControls（addControl 幂等安全重入），**传 NULL 解除容器并摘树**。

**4. GetControlType 机制重构（用户决策）**：dynamic_cast 链（22 级，每次查询全链 RTTI）→ **基类 `m_ctlType` 枚举成员方案**——`enum class ControlType`（25 值，与 JSON type 一一对应）+ 子类构造函数体内 `m_ctlType = ControlType::X`（Actor 5 构造含拷贝构造、LuotiAni 内联构造均覆盖）+ 基类 `getControlType()` 直接返回（O(1)）；CABI 内 switch 枚举→`PropertyNames::kControlType*` 常量（**禁止字面量**）。新增 `kControlTypeMenuItem`/`kControlTypeMenuPanel` 常量（仅查询返回，JSON 无）。坑：MenuItem 私有 `MenuItemType m_type` 阴影基类成员 → 基类成员命名 `m_ctlType`；Splitter 初始化列表裸聚合 `{0,0}` 被脚本误当函数体（手工修正）。

**5. C++ Binding**：`Control::GetType()`（经 CABI）+ `UICornerstone::FromHandle()`（裸句柄包装代理，此前无法包装）；item 属性走通用 Set*/Get* 通道。

**6. 文档**：docs/controls/treeview.html（item 属性表 + item-leading-control 运行时挂载/解除 + 4.21.6 样例）、docs/appendix/properties.html、docs/appendix/declarative-syntax.html（6.3 排序 19 块 + item 增强 + SetPtr 运行时挂载）、docs/appendix/binding.html（11.5.1 + FromHandle/GetType）、design/TreeView_Enhancement_Analysis.md（v7）、design/CABI_Property_Design.md（§8.2 类型查询机制 + Ptr 表）、本条目。

**验证**：test_treeview 30/30 PASS（14 增强 + 16 json enh）、test_treeview_cabi 全过（含 GetControlType: check-box / tree-view）、静态库/DLL/binding 编译零错误零警告。

**相关文件**：include/ControlBase.h（ControlType 枚举 + m_ctlType + getControlType）、include/TreeView.h、src/TreeView.cpp（布局 v6 + item 属性 + syncRowControls 运行时挂载）、src/LayoutParser.cpp（parseItems 四键）、include/PropertyNames.h（item 常量 + JSON 键 + 类型常量）、src/UICornerstoneAPI.cpp（GetControlType switch）、24 控件构造函数（m_ctlType 设置）、binding/（GetType/FromHandle/DynamicApi）、test/test_treeview.cpp、test/test_treeview_cabi.cpp。

### 2026-08-14: ResourceProvider 全链路实施 + Label 任意字体（Complete）

**背景**：设计定稿（commit 42e19b4，含 AGENTS.md 提交/推送规则）后按 §7 实施清单逐项落地：内存资源加载（初始化前堆内存 → C ABI 注册 → 挂载实例）+ JSON 集中配置（顶层 resourceProviders 挂载点 + provider: 引用分流），并扩展 Label 任意字体加载。

**1. MemoryResourceProvider（src/ResourceProvider.cpp / include/ResourceProvider.h）**：pimpl 实现；两种注册模式——`registerMemory`（拷贝，调用方可立即释放）/ `adoptMemory`（零拷贝引用：引擎不复制，调用方保持 buffer 有效直至销毁/覆盖，析构/覆盖时经 freeFn 回调释放，freeFn 可为 NULL → 默认 free）；`mountPath` 懒建文件系统 delegate（首次 readFile 读入并缓存）；`readFile`/`exists` 自动剥离 `provider:` 前缀（PropertyNames::kProviderPrefix）。

**2. C ABI 四函数 + bridge（include/UICornerstoneAPI.h / src/backend/BackendBridge.h / 三后端 BackendPlugin.cpp）**：`createMemoryResourceProvider` / `memoryProviderRegister` / `memoryProviderAdopt` / `setResourceProvider` 追加函数表尾，三后端注册（sed 应用 + grep 各 1 处验证）；`bridge_setResourceProvider` 写 `UIContext::resourceProvider`（ControlBase 级联传播）。

**3. 入口分流（字符串层，非 fs::path）**：`Actor::loadFromFile` / `LuotiAni::loadFromFile` 首行 `rfind("provider:",0)` → `loadFromResource(substr(9))`；`LuotiAni::loadAniDesc(fs::path)` 兜底分流；**关键教训——MSVC 将 `fs::path("provider:xxx")` 判为相对路径**，上游 `is_relative()` 拼接 basePath 会污染前缀导致分流失效，故全部 6 处拼接点（布局 path / CreateAnimation / CreateAnimatedButton / 属性 animation / 状态图 / 内嵌 la）先判前缀豁免。

**4. 两阶段补读**：布局 parse 阶段控件未挂树（provider 未就绪）→ `LuotiAni::loadFromResource` 记忆 `m_resourceId`、`setRenderDevice` 时补读描述再 prepare；Label `font-resource` 记忆 `m_fontResourceId`、`create()` 补读。

**5. LoadLayout 顶层挂载（src/UICornerstoneAPI.cpp）**：`resourceProviders` 数组（camelCase 键 `name`/`path`）→ `mountPath(name, path, resourceRoot)`，json::parse 失败静默交 parseLayout 报告。

**6. Label 任意字体三形态**：`font`（枚举 28 种）< `font-file`（任意路径，覆盖 m_fontFile，相对 resourceRoot 经 provider 查询，支持 provider: 前缀）< `font-resource`（内存 ID）；互斥覆盖（m_fontResourceId / m_fontFile 互清）；JSON 键 `fontFile` / `fontResource`（parseLabel 白名单新增）；`loadFromFile(fs::path)` 新重载。

**7. C++ Binding（binding/）**：`RegisterResource` / `AdoptResource`（须在 Create 后调用，懒创建内部 provider + 自动 setResourceProvider）；std::function 不能直作 C 函数指针 → 全局 `g_adoptFreeFns` + `adoptFreeTrampoline`（调用时移除条目）；析构先 DestroyInstance 再销毁 provider。

**8. 测试（test/test_resourceprovider_cabi.cpp，三后端运行通过）**：SDL3/SFML/Raylib 各 6 项核心断言全绿——5 资源（cross_up/cross_down png、MapleMono ttf、bombBlock.jsonc、marker.svg）堆内存注册（4 register + 1 adopt 零拷贝）、`resourceRoot="_nonexistent_dir_"` 零磁盘初始化、布局三引用语法（actors providerName 对象式 + 字符串式 + animation path provider:）、工厂 provider: 三引用、GetRect 32×32 证明纹理来自内存、负用例存活、adopt freeFn 恰一次计数；fontFile 前缀形态经 `fontFile:"provider:maple-font"` label 验证。期间修复：测试文件缺 UTF-8 BOM 导致 MSVC 936 代码页中文注释吞声明行（已补 BOM）。

**9. Release 构建**：三后端 Release（build/{sdl3,sfml,raylib}_dll --config Release）→ `release/` 目录收集 UICornerstone.dll + 三后端插件 + 运行时 DLL + assets 资源。

**10. 文档**：ResourceProvider_Design.md（adopt 零拷贝契约修正、前缀拼接豁免、fontResource/fontFile JSON 键、动画 JSON 内层图片 src 原串注册、§7 清单状态、§8 风险）；docs/declarative-ui.html 新增 3.4.7 资源提供者；docs/controls/label.html 属性表 + C ABI 样例；docs/cpp-binding.html 新增 3.1.4；docs/property-system.html 3.3.2 常量示例。提交 99b2764（实施）+ f659681（Enhancements_Analysis.md）。

**11. 教训**：① DLL 模式测试 exe 目录的 UICornerstone.dll 由 POST_BUILD 拷贝，只重编核心库不重编 test 目标会导致 exe 目录 DLL 陈旧（vtable/分流行为"假象"）——重编 test 目标或核对 DLL 时间戳；② adopt 语义一旦定稿（调用方保持有效），测试与文档须同步修正，避免"释放后渲染"矛盾断言。

---

### 2026-08-13: ViewportScale 设计收尾——剩余项逐项实施（Complete）

**背景**：用户要求按上轮列出的未实施项清单逐项谨慎实施（字号重建剩余控件、属性系统接入、逆变换公开、缓存统计、双线性开关、可见性过滤、三项核对）。

**1. 字号重建补全（§4.5 全部文本控件落实）**：

- `TreeView::refreshScaleWith`：字号随复合重建（setFontSize 同语义）；行高独立不随字号；
- `Slider::refreshScaleWith`：tickFont 按 `m_tickLabelFontSize×getScaleXX()` 重建 + 刻度文本重排；`ensureTickFont` 复用已读字体数据（避免 refresh 重复 IO）；新增 `getTickFont` 访问器；
- `MenuBar/MenuPanel::refreshScaleWith`：共享字体重建 + 布局重排（layoutEntries/recalculateSize）；下拉面板与子菜单不在 m_children，手动传播复合缩放（与 setContext 传播对齐）；新增 `getFont` 访问器；
- NumericUpDown 继承 EditBox 自动覆盖；CheckBox caption/ProgressBar 文本为树成员 Label 自动覆盖；ScrollBar/ColorPicker 无自绘文本——全部文本控件链路闭合。

**2. 属性系统接入（§4.7）**：`Bench::setEnumProperty("viewport-scale-mode", "off|fit|stretch")` 覆写（非法值/未知属性拒绝并透传基类），运行时切换即重算根变换、子树字号/布局自动重建。

**3. 公开 API（§4.8）**：`ControlBase::mapViewportToCanvas`（`mapToDrawPoint` 逆变换：窗口→画布，除复合零保护），高层命中反查可用。

**4. 字号缓存统计**：`TextRenderer::getFontCacheEntryCount`——经排查确认前端 `CallbackTextRenderer`（后端字体走 C ABI 句柄、无缓存透传），改为 Callback 前端计数（loadFont* 成功累加），零 C ABI 侵入；期间排查并排除跨 DLL vtable 错位假象（`typeid = CallbackTextRenderer`）。

**5. LuotiAni 帧位图双线性过滤开关**：`LuotiAni::setFrameFilter` + `RenderDevice::setTextureFilter`（SDL3 `SDL_SetTextureScaleMode`、SFML `setSmooth`、raylib `SetTextureFilter`——raylib 无 `rlSetTextureFilter` 报错改用高层 API），纹理创建处应用。

**6. 字号重建可见性过滤（§8 风险落地）**：Label/EditBox 不可见时 `m_fontScaleDirty` 延后重建，可见帧 `update()` 补重建（TextArea 经继承 + 行高懒检测自动生效）。

**7. 三项核对（关闭，无代码）**：Cursor 系统光标 OS 托管无缩放语义（P3 关闭）；GraphTool.cpp 无 viewport/画布坐标直接引用（坐标全走标准 Control API）；全站 viewport 引用 grep 复查（Bench 根变换入口/LayoutParser/MainWindow/UICornerstoneAPI/UIContext 兜底/弹层本地反查），无遗漏散点。

**8. 测试**：test_viewport_scale 新增 Ta（属性 5 断言）/Tb（TreeView 3）/Tc（Slider 2）/Td（Menu 4）/Te（可见性过滤 3）/Tf（逆变换+过滤 4）/Tg（统计+开关 3）→ **81/81 PASS**；核心 40 项全量回归 rc=0（auto=5）；SDL3/SFML/Raylib 三后端 test_viewport_scale 编译通过（raylib 过滤 API 修正一次）。

**9. 文档**：ViewportScale_Design.md §4.5（实施状态全量）/§4.7（属性已实施）/§4.8（核对结论+统计）/§5 阶段 3（完成标注）/§6（Ta-Tg 行）/§7 附录 A（6 行新条目）/§8（可见性过滤+ grep 复查已实施）/§9 待办 3（核对关闭）全部同步；断言数 57→81。

**10. 教训**：测试运行先检查 test/Debug 下 DLL 时间戳（build.md 已注明）——本会话因 UICornerstone.dll/UIBackend_sdl3.dll 未拷贝、源码未重编导致的 vtable 缺失假象浪费多轮。

---

### 2026-08-13: 弹层随根变换缩放 + 视口背景色 + 编辑类控件缩放修复（Complete）

**背景**：用户明确"Dialog/Popup 本就应可缩放（ColorPicker 缩放与 Swatch 绑定）"，要求撤销设计 §4.2.4 的"弹层物理像素面板"机制；随后实测报告 stretch 下 TextArea 字体高度错乱、字号不随比例及时更新、ScrollBar 厚度/滚动范围错误、行高贴着、选择背景不匹配字高。

**1. 弹层随根变换缩放（撤销物理像素机制）**：

- `Dialog.cpp` `Popup::setParent`：基类快照后向 `m_children` 逐子 `refreshScaleWith(m_xxScale, m_yyScale)`——弹层复合 = 布局 × 父复合，内部已建控件同步；
- 删除 `Popup::refreshScaleWith` 空覆写；`open()` 改为**先 `BENCH->addControl` 挂树（复合生效）再 `computeTargetRect`**；
- `computeTargetRect` 钳制换算除根复合（`hiX = (vp.left+localW-bx)/bsx - m_rect.width*sx/bsx`），数学对任意弹层复合自洽；
- ComboBox/ColorPicker 坐标反查公式无需改动（尺寸/位移同式）；T6 契约改断言：fit 0.64 下弹层 DR=(416,320,192,128)（随画布缩小居中）；
- 设计文档 §4.2.2/§4.2.4/C2/T6/附录 7 同步修订。

**2. 视口背景色**：

- `UIContext.h` `viewportBackground`（默认透明不填充）+ `#include "SColor.h"`；`UICornerstone_Render` clip 内按 alpha 填充；
- C ABI `UICornerstone_SetViewportBackgroundColor(r,g,b,a)` + binding 全链 + 样例 B 视口演示；T8x 3 断言。

**3. 编辑类控件缩放修复**（stretch 独立轴）：

- `EditBox::refreshScaleWith`：复合变化 → `loadFontInternal()` 重建字号（字号=fontSize×sx），resize 后即时生效（原需焦点等路径碰巧触发）；
- `EditBox` 垂直居中/选择条/光标高度改 `getFontHeight`（真字高，非 fontSize×scaleY 近似）；margin 水平/垂直分量分离；
- `TextArea::refreshScaleWith`：基类（字号重建）后行高自适应 + `rebuildLines()` + 滚动范围重算；
- **行高自适应**：默认 `m_lineHeight = getFontHeight/getScaleYY()`（本地），stretch 下行距屏幕=真字高、字迹不重叠；`setLineHeight` 定制优先；`update()` 懒检测字体指针变化覆盖 setFont/setFontSize/setText 路径；
- TextArea draw/命中 `relY` 垂直分量改 `getScaleYY()`；选择条/光标高用 fontHeight；`getVisibleLines`/`scrollToBottom` 统一本地单位（与 updateVScrollBar 同式）；
- `ScrollBar`：垂直轴 top/height 与命中/拖拽反查用 `getScaleYY()`（水平 `getScaleXX()`）；
- 测试：T9x（字号重建/滚动范围 clamp=3844 off↔stretch 一致）6 断言 + T9y（行高自适应）4 断言 → 57/57 PASS；核心 40 测试 rc=0；binding 6 样例 rc=0。

**4. 字宽限制（决策，未实施）**：字形按标量字号（sx）重建，宽高等比；三后端能力不对称（raylib `DrawTextEx` spacing / SFML `setScale(x,1)` / SDL3 `TTF_DrawRendererText` 皆无），"字宽系数"需先补 SDL3 后端或走核心层布局换算——用户确认暂不实施。

---

### 2026-08-12: 缩放测试三件套（JSON/C ABI/sample）+ 样例统一 auto=<秒> + 动画按钮接线（Complete）

**背景**：交付缩放对照组三测试（test_scale_json / test_scale_cabi / sample_scale，btn x=60 / img x=580 / ani x=1100，1x y=80、2x y=560）；用户要求所有 Sample 自动测试向标准测试对齐（统一命令行 `auto=<秒>`，废弃 UICORN_AUTO 环境变量方案）。

**1. CreateAnimatedButton 全链路接线**：

- C ABI：`src/UICornerstoneAPI.cpp:1149` `UICornerstone_CreateAnimatedButton(instance, jsoncPath, x, y, w, h, xScale, yScale)`——Button 按 xScale/yScale 构造（参数作用于按钮本身），内嵌 LuotiAni `(btn.get(), 1.0f, 1.0f)`（不挂树、不响应鼠标、rect 铺满按钮局部坐标），loadFromFile + prepare，失败返回 nullptr（异常边界）；jsoncPath 可 NULL（纯按钮）。
- Binding：`binding/src/DynamicApi.h` fnCreateAnimatedButton + `DynamicApi.cpp` RESOLVE + `binding/src/UICornerstone.cpp` UI_FACTORY + `binding/include/UICornerstone.h` 声明 `Control CreateAnimatedButton(jsoncPath, x, y, w, h, xScale=1, yScale=1)`。

**2. 布局内嵌动画（JSON `"type":"button"` + `"luotiAni"`）**：

- LayoutParser parseButton 内嵌分支（LayoutSystem_Design.md §5）：相对路径拼 `Platform::GetBasePath()`（同 "animation" 类型）；LuotiAni 构造 scale 恒 1.0；**不显式 prepare/play**——挂树后 `Button::setRenderDevice` → `LuotiAni::setRenderDevice` 延迟 prepare（`!m_isPrepared && m_totalFrames > 0` 守卫，异常捕获 logWarn）。
- JSON 布局文件：节点由 `"type":"animation"` + `path` 改为 `"type":"button"` + `luotiAni`。

**3. 点击 CLICK 修复链**（三测试自动注入点击验证通过）：

- `Button::setBoolProperty` 转发 `"playing"` 给 m_luotiAni；`Button::setRenderDevice` override 转发设备（Button.cpp:344-348、Button.h 声明）。
- `ControlImpl::setRenderDevice` 移除 `if (m_renderDevice == device) return;` 早退——rootPanel 先经 getRenderDevice 缓存设备后，addControl 传播会被早退阻断，子控件（含内嵌动画）永远收不到设备、永不 prepare（ControlBase.cpp:506 注释）。
- 测试静态链接 UICornerstone lib：只重建 DLL 无效，必须重建 exe 目标（构建流程关键点）。

**4. 双重缩放修复**：

- 根因：setParent 复合缩放 `m_xxScale = m_xScale * parent->getScaleXX()`——LayoutParser 内嵌分支与 CreateAnimatedButton 曾传按钮 scale 给 LuotiAni 构造，2x 按钮 → 4x 内容。
- 修复：两处构造恒传 1.0f；插桩验证 `scale=(2,2) drawRect=(1100,560,440,440)`（2x 内容 440 正确）。

**5. 样例统一 auto=<秒>（废弃 UICORN_AUTO）**：

- 新建 `binding/samples/auto_args.h`：`ucorn_sample::AutoTimer`——`parse(argc, argv, extraPrefixes, extraCount)` 任意顺序识别 `auto=<秒>`（未知参数 WARN 后忽略），`expired()` **惰性计时**（首次检查时刻为起点，窗口/后端创建耗时不计入自动时长）；文件须 UTF-8 BOM（无 BOM 时 MSVC 按 GBK 读中文注释 → C2059）。
- 五个样例（sample_scale / sample_cpp_embed / sample_cpp_hosted / sample_cpp_multiinstance / sample_cpp_multiview）全部替换 `getenv("UICORN_AUTO")` 与 240 帧上限：`if (autoSeconds > 0)` 门控注入、`autoTimer.expired()` 退出；`backend=` 参数保留任意顺序。
- 验证：`auto=3` 全 rc=0（163-176 帧）；`auto=2 backend=sdl3` 乱序 OK；`foo=1` WARN；`auto=3` + raylib 后端缺 DLL（Create failed，非回归）。
- 文档同步：CppBinding_Design.md / CppBinding_UserManual.md §15.2 / guidelines/testing.md §4 / guidelines/history.md 2257 行 / README.md——样例统一 `auto=<秒>`；UICORN_AUTO 仅存于旧历史记录（37/58/2243/2271、BackendAbstraction:2096）作为存档。

**6. 文档刷新（2026-08-12）**：

- UICornerstone_DLL_Design.md：动画三件套（CreateActor/CreateAnimation/CreateAnimatedButton）原型 + 语义注释。
- LayoutSystem_Design.md：luotiAni 实现语义（基路径、1.0 构造、延迟 prepare、SetBool("playing") 启动）+ parseButton 流程图。
- Button_Design.md：内嵌动画集成语义（setRenderDevice 转发 / playing 透传 / 缩放约定）。
- ControlBase_Design.md：setRenderDevice 无早退传播语义。
- LuotiAni_Design.md：§6.6 延迟 prepare、§6.7 布局接入由"不做"转"已实施"；LuotiAni_DevGuide.md：测试矩阵补 test_scale_json/cabi。
- CppBinding_Design.md：§7.5 sample_scale、主类 CreateAnimatedButton 声明、工厂映射表；CppBinding_UserManual.md：§4.3 CreateAnimatedButton、工厂清单、构建目标补 sample_scale。
- 测试三件套全绿：json/cabi `PASS: 布局 rect 与缩放系数无关` + CLICK 注入（`[scale-json] CLICK [1x]ani (event=click)`、`[scale-cabi] CLICK … (count=1)`）；sample_scale 7 项 rect PASS；回归 test_animation / test_luotiani（须在 test/Debug 目录运行）/ test_image 全绿。

---

### 2026-08-08（午夜）: 后端能力位机制 + raylib 单窗口架构多实例 headless 化 + sfml 焦点事件修复（Complete）

**背景**：raylib 后端多实例双窗口测试人工验证时两窗口内容交替闪动；顺带暴露 sfml 焦点环切换失效（点按失去焦点后无法切回）。

**1. sfml 焦点事件转换修复（焦点环切换失效）**：

- 根因：sfml InputBackend 未转换 FocusLost/FocusGained 事件（sdl3/raylib 均有转换），窗口失焦后焦点状态不更新，键盘焦点环（Tab）无法切回。
- 修复：`src/backend/sfml/InputBackend.cpp` 补 `event.focusEvent.focused = false/true` 转换。视觉验证 OK。

**2. raylib 多实例双窗口闪动根因定位（单窗口架构限制）**：

- 根因：`subModules/raylib/lib/raylib.dll` 为**单窗口架构**——raylib CORE 全局状态只跟踪最近一次 InitWindow 的窗口，第二个 InitWindow 直接覆盖第一个；且 DLL 不导出 glfw 符号、无源码，多实例渲染全部画到同一窗口（A/B 内容交替闪动）。
- 用户否决方案：Win32 辅窗口方案（Windows 专属，跨平台要重写三套）；raylib 源码 patch 方案（绑定 raylib 内部状态，升级成本高）。
- 采纳方案：**能力限制**——声明后端能力位，调用方按能力决定行为；同时为后续原生 GPU 后端（GL/GLFW/DirectX/Vulkan）架构预留能力声明机制。

**3. 能力位机制（跨平台架构预留）**：

- `include/UICornerstoneAPI.h`：`UICORN_BACKEND_CAP_MULTI_WINDOW(1<<0)/RENDER_TARGET(1<<1)/CLIP_RECT(1<<2)/READBACK(1<<3)` 四个宏；`UIBackendCallbacks` 末尾加 `uint32_t capabilities`；导出 `UICornerstone_GetBackendCapabilities(UIInstance)`。
- `include/BackendPlugin.h`：`BackendAPI` 末尾加 `uint32_t capabilities`。
- `src/BackendManager.cpp`：静态 api 路径（`api.capabilities`）与回调表路径（`callbacks->capabilities`）均保存，`BackendManager::capabilities()` 查询。
- 三后端 BackendPlugin.cpp（g_xxxBackend 结构体 + cb.capabilities 两处）均设能力位：raylib = RENDER_TARGET|CLIP_RECT|READBACK（**无 MULTI_WINDOW**）；sdl3/sfml = 四能力全有。

**4. raylib headless 化（单窗口架构多实例适配）**：

- `src/backend/raylib/Window.cpp`：`static int s_windowCount` + `m_hasOwnWindow`——仅首个实例 InitWindow（防覆盖 CORE 全局窗口状态），后续实例不建窗口；getSize/getPosition/getDisplayWidth/Height/getDpiScale/setTitle/getMousePosition/setResizable 全部 `if (!m_hasOwnWindow)` 守卫；析构仅在 `m_hasOwnWindow && IsWindowReady()` 时 CloseWindow。
- `include/Window.h`：新增 `virtual bool isHeadless() const { return false; }`；raylib 覆写返回 `!m_hasOwnWindow`（为 headless 实例预留）。
- `src/backend/raylib/InputBackend.cpp`：`m_hasWindow`（构造时 `window ? !window->isHeadless() : false`）；pollEvent/setClipboardText/getClipboardText/getModState/newFrame（跳过 PollInputEvents）全部守卫，防串扰并抢先消费主实例事件。

**5. Binding 与测试/样例适配**：

- binding：`GetBackendCapabilities()` 封装（DynamicApi.h/cpp + UICornerstone.h/cpp），调用方按能力位决定行为。
- `test/test_multiinstance_visual_cabi.cpp` / `test/test_multi_instance_cabi.cpp`：RESOLVE(GetBackendCapabilities)；仅 MULTI_WINDOW 能力下对第二实例渲染/交换，否则渲染冒烟 SKIP 打印；测试头部打印后端能力信息（人工模式提示单窗口限制）。
- `binding/samples/sample_cpp_multiinstance.cpp`：第二实例 Clear/Render/Present 包在 `GetBackendCapabilities() & UICORN_BACKEND_CAP_MULTI_WINDOW` 条件内。

**验证**：三后端全量 test `auto=3` exit=0；4 binding 样例 UICORN_AUTO=1 exit=0（multiinstance 样例双窗口交互断言全过）；raylib 多实例测试 auto=3 ALL PASS（渲染冒烟 SKIP）+ 人工模式主实例窗口显示正常无闪动。

**相关文件**：include/UICornerstoneAPI.h、include/BackendPlugin.h、include/Window.h、src/BackendManager.cpp、src/UICornerstoneAPI.cpp、src/backend/{sdl3,sfml,raylib}/BackendPlugin.cpp、src/backend/raylib/Window.cpp、src/backend/raylib/InputBackend.cpp、src/backend/sfml/InputBackend.cpp、binding/src/DynamicApi.{h,cpp}、binding/include/UICornerstone.h、binding/src/UICornerstone.cpp、binding/samples/sample_cpp_multiinstance.cpp、test/test_multiinstance_visual_cabi.cpp、test/test_multi_instance_cabi.cpp。

### 2026-08-08（深夜）: 视觉测试 JSON 布局改造 + LoadLayout 多平级控件悬垂修复（Complete）

**背景**：两个 CABI 视觉测试改用 JSON 创建后暴露 LoadLayout 的一个 bug（测试崩溃 + 控件不显示）。

**1. LoadLayout 多平级控件悬垂修复（核心库 bug）**：

- 根因：`parseLayout` 只返回最后一个控件（`root = ctrl` 覆盖），`UICornerstone_LoadLayout` 只把 root 挂到 bench，其余平级控件既未挂载也不持有所有权，**随 parser 析构销毁，FindControl 返回悬垂句柄**；Debug API `IsControlHovered/Focused` 访问时崩溃（同一对象地址被释放）。
- 修复：LoadLayout 遍历 controlsById，将**无父控件**也挂到 bench 保持生命期与可见性（容器内子控件有父，跳过）。

**2. CABI 视觉测试改造（遵循测试规范 JSON 创建）**：

- 控件创建方式：LoadLayout + FindControl + events.onClick 绑定 RegisterAction；JSON 属性：editbox 文本用 "text"、button/label 用 "caption"、editbox 的 JSON type 为 "edit-box"。
- 多窗口：按钮点击 → GetString 读本窗口 EditBox 文本 → SetString 到**对方窗口** Label（跨实例内容传递）；auto 模式断言对方窗口内容 == 本窗口输入（"Message: hello from A"）。
- 多视口：按钮点击 → GetString 读本视口 EditBox 文本 → CreateDialog + CreateLabel(内容) + AddChildControl，弹窗显示自己视口输入的内容；auto 模式断言弹窗内 Label 文本 == 本视口 EditBox 文本 + 弹窗视口内居中（右下视口 Popup bug 回归）。
- 新增动态加载函数指针：GetString/SetString/RegisterAction/LoadLayout/FindControl/CreateLabel/AddChildControl。
- 排查结论：`ControlImpl` 是虚拟继承（virtual public Control），void*→Control* 直转须确认存储的是 Control 子对象地址——本链路（shared_ptr<Control> 存储 + reinterpret_cast）一致，无偏移问题；崩溃确认为悬垂句柄。

**3. 回归**：三后端（SDL3/SFML/raylib）DLL 全量 test exit=0 + 4 binding 样例（UICORN_AUTO=1）exit=0；两个视觉测试三后端 auto=3 全部 ALL PASS；人工模式窗口驻留正常；LoadLayoutFromFile 委托 LoadLayout 同享修复。


### 2026-06-01: Phase 1 — SColor Unification

**Changes**:

- `include/SColor.h`: Created standalone SColor class with `constexpr` constructors (float/int/rgba), color operations (brighter/darker/blend), `toSDLColor()`/`toSDLFColor()` bridge methods
- `include/GraphTool.h`: Removed inline SColor class, added `#include "SColor.h"` and `using SColor = ::SColor;` in GraphTool namespace
- `include/ControlBase.h`: StateColor members → SColor, all virtual setters/getters → SColor
- `include/ConstDef.h`: All `static const SDL_Color` → `static const SColor`
- `include/Menu.h`: SDL_Color member vars → SColor
- `include/CheckBox.h`, `include/ProgressBar.h`, `include/Panel.h`, `include/Theme.h`, `include/LayoutParser.h`: All SDL_Color in public API → SColor
- `include/GraphOperaAdapt2d.h`: Replaced duplicate SColor class with `#include "SColor.h"`
- All `src/*.cpp`: Drawing code updated to use `redByte()/greenByte()/blueByte()/alphaByte()` accessors on SColor; brace init → function-style constructors; removed unnecessary `.toSDLColor()` calls where receiving interfaces now take SColor
- `test/test_graphtool.cpp`: Fixed ambiguous SColor constructor call

**Status**: All 9 tests build successfully

### 2026-06-01: Phase 2 — RenderDevice Abstraction (Infrastructure + GraphTool + Control Migration)

**Core Infrastructure**:

- `include/RenderDevice.h`: Abstract interface with ~25 pure virtual methods covering state, primitives, textures, render targets, and frame ops. Includes convenience `drawTriangle()`/`drawQuad()` methods for common patterns. Forward-declares `SDL_Renderer` only in factory function.
- `src/RenderDevice.cpp`: `SDL3RenderDevice` concrete implementation. Converts all abstract types (`SColor`, `SRect`, `Vertex`) to SDL3 equivalents internally. Factory `CreateSDL3RenderDevice()` returns heap-allocated instance.
- `include/MainWindow.h`: Added `RenderDevice* m_renderDevice` member, `getRenderDevice()` accessor, `GET_RENDERDEVICE` macro. `MainWindow` constructor calls `CreateSDL3RenderDevice(m_renderer)`. Added destructor for cleanup.
- `include/ControlBase.h`: Added `getRenderDevice()`/`setRenderDevice()` to `Control` interface and `ControlImpl` implementation.
- `src/ControlBase.cpp`: Implemented `getRenderDevice()` with parent/game fallback chain; `setRenderDevice()` propagates to children; `inheritRenderer()` also inherits render device.
- `CMakeLists.txt`: Added `src/RenderDevice.cpp` to library sources.

**GraphTool Migration**:

- `include/GraphTool.h`: Constructor changed from `SDL_Renderer*` to `RenderDevice*`. `SDL_Texture*` → `void*` in `drawImage()`. Removed `#include "SDL3/SDL.h"`, now includes `RenderDevice.h`. Removed `SDL_Renderer*` from transform stack.
- `src/GraphTool.cpp`: Complete rewrite of all 1452 lines. All 17 `SDL_SetRenderDrawColor` → `setDrawColor(SColor)`. All `SDL_RenderFillRect`/`SDL_RenderRect`/`SDL_RenderLine`/`SDL_RenderPoint` → `fillRect`/`drawRect`/`drawLine`/`drawPoint`. All 16 `SDL_RenderGeometry`+`SDL_Vertex` arrays → `drawTriangle()`/`drawQuad()` calls. All `SDL_RenderTexture` → `drawTexture()`. All clip rect → `setClipRect()`/`clearClipRect()`. No SDL3 headers included.

**Test Migration**:

- All 10 test files updated: `SDL_SetRenderDrawColor`/`SDL_RenderClear`/`SDL_RenderPresent` → `RenderDevice` equivalents. `DrawingContext(renderer)` → `DrawingContext(getRenderDevice())`.

**Full Control Migration (54 SDL calls → RenderDevice)**:

- `ControlBase.cpp`: `drawBackground()`/`drawBorder()` — 4 SDL calls → `getRenderDevice()->setDrawColor()`/`fillRect()`/`drawRect()`
- `Bench.cpp`: Loading progress bar — 4 SDL calls → `GET_RENDERDEVICE->setDrawColor()`/`fillRect()`/`drawRect()`
- `Label.cpp`: Debug drawing — 4 SDL calls → `GET_RENDERDEVICE->setDrawColor()`/`drawRect()`
- `ScrollBar.cpp`: Track/thumb drawing — 4 SDL calls → `GET_RENDERDEVICE->setDrawColor()`/`fillRect()`
- `ProgressBar.cpp`: Background/progress fill — 4 SDL calls → `GET_RENDERDEVICE->setDrawColor()`/`fillRect()`
- `EditBox.cpp`: Clip rect, selection fill, cursor — 6 SDL calls → `setClipRect()`/`clearClipRect()`/`setDrawColor()`/`fillRect()`
- `TextArea.cpp`: Clip rect, selection fill (with blend), cursor — 6 SDL calls → `setClipRect()`/`clearClipRect()`/`setBlendMode(BlendMode::Blend)`/`setDrawColor()`/`fillRect()`
- `Menu.cpp`: MenuItem hover, checkmark, separator, MenuPanel items, MenuBar bg/items/border — 12 SDL calls → `GET_RENDERDEVICE->setDrawColor()`/`fillRect()`/`drawLine()`
- `MenuPanel::drawShadow()`: Removed unused `getRenderer()` null-check
- `CheckBox.cpp`: Removed 4 unused `getRenderer()` null-checks
- `Actor.cpp`: 4 `SDL_RenderTexture` → `getRenderDevice()->drawTexture(m_texture, ...)`
- `Material.cpp`: 1 `SDL_RenderTexture` → `getRenderDevice()->drawTexture(m_texture, ...)`

**Interface Addition**: Added `BlendMode` enum + `setBlendMode()` to `RenderDevice` interface and `SDL3RenderDevice` impl (for TextArea selection alpha blending)

**Status**: All 10 tests build successfully. Phase 2 core migration complete.

### 2026-06-01: Phase 3 — Texture/Surface Abstraction (Complete)

**Core Types**:

- `include/Texture.h`: New abstract `Texture` class — `width()`, `height()`, `setBlendMode()`, `setAlphaMod()`, `getBlendMode()`, `getAlphaMod()`
- `include/Surface.h`: New abstract `Surface` class — pixel ops (`getPixel`/`setPixel`), `blit()` (scaled + unscaled), `createTexture(RenderDevice*)`, `rotate()`, factory statics (`create`/`loadFromFile`/`loadFromMemory`)
- `include/RenderDevice.h`: Replaced `void*` texture API with `Texture*`/`SharedTexture`; added `createTextureFromSurface(Surface*)`, `SharedTexture`/`SharedSurface` type aliases
- `src/RenderDevice.cpp`: Added `SDL3Texture` (wraps `SDL_Texture*`), `SDL3Surface` (wraps `SDL_Surface*`, implements pixel/format/blit/rotate ops), all `Surface` factory statics; `SDL3RenderDevice` updated to use `Texture*`/`SharedTexture`

**`m_texture` Migration (SDL_Texture* → SharedTexture)**:

- `include/ControlBase.h`: `SDL_Texture* m_texture` → `SharedTexture m_texture`
- `include/Actor.h`: `getTexture()` returns `Texture*`, `setTexture()` takes `SharedTexture`
- `src/Material.cpp`: Removed `SDL_DestroyTexture`; `SDL_SetTextureBlendMode`/`SDL_SetTextureAlphaMod` → `m_texture->setBlendMode()`/`m_texture->setAlphaMod()`
- `src/Actor.cpp`: Uses `surface->createTexture(device)` instead of bridge; texture size via `m_texture->width()`/`height()`
- `include/GraphTool.h`: `drawImage()` takes `Texture*` instead of `void*`

**`m_surface` Migration (SDL_Surface* → SharedSurface)**:

- `include/ControlBase.h`: `SDL_Surface* m_surface` → `SharedSurface m_surface`
- `src/Actor.cpp`: `loadFromFile()`/`loadFromResource()` use `Surface::loadFromFile()`/`loadFromMemory()`; `setParent()` uses `m_surface->createTexture()`
- `ControlBase.cpp`: Copy/assign semantics unchanged (shared_ptr handles refcounting)
- Added `Surface::rotate()` for GPU-accelerated rotation (render-to-texture + readback, inside `SDL3Surface`)

**`LuotiAni.h` Full Migration (~180 SDL calls)**:

- Total rewrite: 1341-line header-only animation engine
- `SDL_Surface*` → `SharedSurface` in `OpData`, `m_frameSurfaces`, all helpers
- `SDL_BlendMode` → `BlendMode` enum; `SDL_GetTicks()` → `std::chrono::steady_clock`
- `SDL_Log`/`SDL_GetError()` → `printf`; `Uint64`/`Uint8` → standard types
- `SDL_CreateSurface`/`SDL_BlitSurfaceScaled`/`SDL_SetSurfaceAlphaMod`/`SDL_SetSurfaceBlendMode` → `Surface::create()`/`blit()`/`setAlphaMod()`/`setBlendMode()`
- `IMG_Load_IO` → `Surface::loadFromMemory()`
- Removed dead code: `normalRotateSurface`, `matrixRotateSurface`, `gpuRotateSurface`
- GPU rotation → `Surface::rotate(angle, getRenderDevice())`
- `loadFromStream(SDL_IOStream*)` replaced with `parseJsonDesc()` using `std::ifstream`
- Pixel helpers simplified for RGBA8888 format

**Bridge Removal**:

- Removed `createTextureFromSDLSurface` and `getNativeRenderer` from `RenderDevice` (no longer used)
- Removed `loadTextureFromSurface(SDL_Surface*)` from `Actor` (no longer used)
- Removed `struct SDL_Renderer`/`struct SDL_Surface` forward declarations from `RenderDevice.h`

**Animation Fixes**:

- Frame actors in `LuotiAni::prepare()` now get non-zero rect via `frame->setRect()`
- `LuotiAni::setRect()` propagates unconditionally to frame actors (removed `m_isPrepared` guard)
- `Button::setLuotiAni()` syncs button rect to LuotiAni
- `Button::setRect()` propagates to `m_luotiAni`

**Testing**:

- Added 2x scaled LuotiAni Button test (`g_button6` in `test_button.cpp`)
- All 10 tests build successfully

### 2026-06-02: Phase 4 — Remove Umbrella `#include <SDL3/SDL.h>` from Non-Backend Headers

**Umbrella include completely removed from 8 headers**:

- `Menu.h`: No SDL types used — removed entirely
- `StateMachine.h`: `SDL_Log()` → `printf()` — removed entirely
- `Material.h`: `Uint8`/`SDL_ALPHA_OPAQUE` → `uint8_t`/`255` — replaced with `<cstdint>`
- `Actor.h`: `Uint8`/`SDL_ALPHA_OPAQUE` → `uint8_t`/`255` — removed entirely
- `Utility.h`: `SDL_FRect`/`SDL_Rect`/`SDL_FPoint` bridge methods → `#include <SDL3/SDL_rect.h>`
- `SColor.h`: `SDL_Color`/`SDL_FColor` bridge methods → `#include <SDL3/SDL_pixels.h>`
- `ControlBase.h`: `SDL_Renderer*` member/API → `#include <SDL3/SDL_render.h>`
- `EditBox.h`: Removed umbrella, kept `SDL3/SDL_keyboard.h` and `SDL3/SDL_clipboard.h`

**Backend headers with umbrella preserved** (inherently SDL-dependent):

- `MainWindow.h`: Header-only window/renderer manager — keeps `#include <SDL3/SDL.h>`
- `ResourceLoader.h`: Narrowed to `SDL3/SDL_thread.h` + `SDL3/SDL_iostream.h`; `.cpp` gets umbrella

**Other improvements**:

- `RenderDevice.h`: `struct SDL_Renderer;` forward declaration instead of any SDL include
- `ResourceLoader.cpp`: Added explicit `#include <SDL3/SDL.h>` (was getting it transitively)

**Status**: All 10 tests build successfully. Only `MainWindow.h` retains the umbrella `#include <SDL3/SDL.h>` (as a intentional backend dependency).

### 2026-06-02: Phase 5 — Font/TextRenderer Abstraction (Complete)

**Core Infrastructure**:

- `include/Font.h` — abstract `Font` class with `getSize()`, `SharedFont` alias
- `include/TextRenderer.h` — abstract `TextRenderer` interface with `loadFont`, `loadFontFromMemory`, `measureText`, `getFontHeight`, `drawText` (plain + wrapWidth), factory `CreateSDL3TextRenderer(RenderDevice*)`
- `src/TextRenderer.cpp` — `SDL3Font` wraps `TTF_Font*`, `SDL3TextRenderer` wraps `TTF_TextEngine*`; factory calls `TTF_Init()` and `TTF_Quit()`
- `RenderDevice.h/cpp` — added `getNativeHandle() -> void*` for backend bridge
- `MainWindow.h` — added `TextRenderer* m_textRenderer`, creation/destruction, `getTextRenderer()`, `GET_TEXTRENDERER` macro
- `ControlBase.h/cpp` — added `TextRenderer* m_textRenderer`, `getTextRenderer()`, `setTextRenderer()` with parent propagation, inherit in `inheritRenderer()`
- `CMakeLists.txt` — added `src/TextRenderer.cpp`

**Full Control Migration (Label + EditBox + TextArea)**:

- `Label.h/cpp`: Removed `#include <SDL3_ttf/SDL_ttf.h>`, replaced `TTF_Font*`/`TTF_TextEngine*`/`vector<TTF_Text*>` with `SharedFont` + `TextRenderer`; `TTF_FontStyleFlags` → `int`; added `computeLineOffsets()`; `SDL_Cursor` forward-declared
- `EditBox.h/cpp`: Removed `#include <SDL3_ttf/SDL_ttf.h>`, replaced `TTF_Font*`/`TTF_TextEngine*`/`TTF_Text*` with `SharedFont` + `TextRenderer`; removed `createTextEngine()`/`createTextObjects()`/`destroyTextObjects()`/`recreateTextObjects()`; added `loadFontInternal()`
- `TextArea.h/cpp`: Removed `TTF_TextEngine* m_textEngine` member; replaced all `TTF_CreateText`/`TTF_GetTextSize`/`TTF_DrawRendererText`/`TTF_DestroyText` with `getTextRenderer()->measureText()`/`drawText()`
- `LayoutParser.h/cpp`: `parseFontStyle()` return type `TTF_FontStyleFlags` → `int`; removed `TTF_STYLE_*` literal dependencies (use plain int values)

**Test Cleanup**:

- Removed `TTF_Init()`/`TTF_Quit()` calls from all 10 test files (TextRenderer internally manages TTF lifecycle)

**Status**: All 10 tests build successfully. No non-backend header includes `SDL3_ttf/SDL_ttf.h` or umbrella `SDL3/SDL.h`.

### 2026-06-03: Phase 7 — TTF_Text* Caching in Label Layer

**Problem**: `TTF_CreateText()` was called on every `measureText()` / `drawText()` invocation, creating and destroying `TTF_Text*` objects each frame. This is both wasteful and fragile — if `TTF_Text*` creation fails or gets corrupted, all subsequent TTF operations hang/crash.

**Changes**:

- `include/TextRenderer.h`: Added cache-friendly methods — `createText(Font*, const string&)` → `void*`, `destroyText(void*)`, `measureText(void*)`, `drawText(void*, ...)`. Original `measureText(Font*, const string&)` / `drawText(Font*, ...)` kept for backward compatibility (used by EditBox/TextArea for one‑off temp text objects).
- `src/TextRenderer.cpp`: Implemented new methods. `createText` calls `TTF_CreateText()`, `destroyText` calls `TTF_DestroyText()`, `measureText(void*)` calls `TTF_GetTextSize()` on the cached object, `drawText(void*, ...)` calls `TTF_SetTextColor()` + `TTF_DrawRendererText()` on the cached object.
- `include/Label.h`: Added `std::vector<void*> m_cachedTexts` member, `releaseTexts()` method.
- `src/Label.cpp`:
  - `Label::~Label()` / `Label::recreate()` → calls `releaseTexts()` to destroy cached `TTF_Text*` objects.
  - `Label::create()` → after loading font and creating multiline text, creates one `TTF_Text*` per line via `renderer->createText()`.
  - `Label::computeLineOffsets()` → uses cached texts for line width measurement; if `truncateLine()` modifies a line, the cached text is recreated in‑place.
  - `Label::draw()` → draws each line using the cached `TTF_Text*` instead of creating/destroying per frame.

**Tests verified**: All 10 tests build successfully. `test_button.exe` and `test_label.exe` run without crashes (both contain Chinese captions with scaled/2x buttons).

**Status**: All 10 tests build successfully.

### 2026-06-03: Phase 8 — ResourceProvider Abstraction (Complete)

**Problem**: Code directly depended on `ResourceLoader` singleton for file I/O, mixing resource bundle loading with filesystem access. Adding filesystem-only resources (e.g., animation JSON) required awkward workarounds.

**Core Infrastructure**:

- `include/ResourceProvider.h`: Abstract `ResourceProvider` interface — `readFile()` → `shared_ptr<vector<char>>`, `openFileStream()` → `SDL_IOStream*`, `exists()` → `bool`. Factory `createFilesystem(basePath)`.
- `src/ResourceProvider.cpp`: `FilesystemResourceProvider` implementation using `fopen`/`fread`.
- `include/ControlBase.h`: Added `ResourceProvider* m_resourceProvider` member, `getResourceProvider()`/`setResourceProvider()` interface and implementation.
- `src/ControlBase.cpp`: `setResourceProvider()` propagates to children; `inheritRenderer()` also inherits resource provider.
- `include/MainWindow.h`: Added `ResourceProvider* m_resourceProvider` member, creation/destruction, `getResourceProvider()`, `GET_RESOURCEPROVIDER` macro.

**Font Data Lifetime Fix**:

- Root cause: `TTF_OpenFontIO(stream, true)` may reference font data lazily; when `shared_ptr<vector<char>>` went out of scope after `loadFromResource`/`loadFontInternal`, the font referenced freed memory → crash in `TTF_GetTextSize`.
- `include/Label.h`: Added `shared_ptr<vector<char>> m_fontData` member.
- `src/Label.cpp`: `loadFromResource()` stores font data in `m_fontData` instead of a local variable; `releaseFont()` also resets `m_fontData`.
- `include/EditBox.h`: Added `shared_ptr<vector<char>> m_fontData` member.
- `src/EditBox.cpp`: `loadFontInternal()` stores font data in `m_fontData` instead of a local variable.

**Status**: All 10 tests build successfully. `test_button.exe` runs without crash (was crashing before the lifetime fix).

### 2026-06-04: Phase 9 — Remove ResourceLoader (Complete)

**Changes**:

- **`ConstDef.h/cpp`**: Moved `FontName` enum and `fontFiles` map from `ResourceLoader.h`
- **`include/Label.h`**, **`include/Bench.h`**, **`src/EditBox.cpp`**: Removed `#include "ResourceLoader.h"`
- **`src/WinFrame.cpp`**: Inlined RID string constants (cross_up/cross_over/cross_down PNGs)
- **`test/test_button.cpp`**: Inlined RID rotateBtn_jsonc path, removed `detachLoadingThread()` call
- **All test files**: Removed `detachLoadingThread()` calls (no longer needed without async loading thread)
- **`src/Bench.cpp`**: Simplified — loading is now instantaneous (no more async resource bundle loading), removed progress bar drawing code no longer applicable
- **`CMakeLists.txt`**: Removed `ResourceLoader.cpp` from build
- **`include/ResourceLoader.h`**, **`src/ResourceLoader.cpp`**: **Deleted entirely** — old resource bundle system is gone

**Status**: All 10 tests build and run successfully.

### 2026-06-04: Phase 10 — SDL Cursor Abstraction (Complete)

**Problem**: `Label.h` and `WinFrame.h` directly used `SDL_Cursor*` and called `SDL_CreateSystemCursor`/`SDL_SetCursor`/etc. directly.

**Changes**:

- **`include/Cursor.h`**: New abstract `Cursor` class with `SystemCursorType` enum (20 cursor types), static factories `createSystem()`, `getDefault()`, and `setCurrent()`.
- **`src/Cursor.cpp`**: `SDLCursor` implementation wrapping `SDL_Cursor*` with owned/unowned semantics. Maps `SystemCursorType` → `SDL_SystemCursor` for `SDL_CreateSystemCursor`. `getDefault()` returns a static singleton.
- **`include/Label.h`**: Replaced `SDL_Cursor* m_hoverCursor/m_defaultCursor` with `Cursor*`, removed `struct SDL_Cursor;` forward declaration.
- **`src/Label.cpp`**: All `SDL_CreateSystemCursor`/`SDL_GetCursor`/`SDL_SetCursor`/`SDL_DestroyCursor` → `Cursor::createSystem`/`Cursor::getDefault`/`Cursor::setCurrent`/`delete`.
- **`include/WinFrame.h`**: Replaced 5 `SDL_Cursor*` members with `Cursor*`.
- **`src/WinFrame.cpp`**: All cursor creation/setting/destruction migrated to `Cursor` API.
- **`CMakeLists.txt`**: Added `src/Cursor.cpp`.

**Non-backend headers now SDL_Cursor-free**: `Label.h` and `WinFrame.h` no longer reference any SDL cursor types.

**Status**: All 10 tests build and run successfully.

### 2026-06-04: Phase 11 — Remove SDL_Renderer* from ControlBase (Complete)

**Problem**: After Phase 2 introduced `RenderDevice` abstraction, `getRenderer()`/`setRenderer(SDL_Renderer*)` in `ControlBase.h` was dead code — no drawing code used it anymore — but it kept `#include <SDL3/SDL_render.h>` as a hard dependency in the primary control base header.

**Changes**:

- **`include/ControlBase.h`**: Removed `#include <SDL3/SDL_render.h>`, removed `SDL_Renderer *getRenderer/setRenderer` from `Control` interface and `ControlImpl` member/overrides; removed `SDL_Renderer *m_renderer` member
- **`src/ControlBase.cpp`**: Removed `getRenderer()`/`setRenderer()` implementations (8 lines, 25 lines respectively); removed `m_renderer` propagation from `addControl()` and `inheritRenderer()`; removed `m_renderer` from constructors/copy/assign
- **`include/MainWindow.h`**: Removed `#define GET_RENDERER` macro (unused everywhere)
- **`include/ConstDef.h`**: Replaced `SDL_WINDOWPOS_CENTERED`/`SDL_WINDOW_RESIZABLE`/`SDL_WINDOW_HIGH_PIXEL_DENSITY` with hex literal equivalents, removing hidden SDL macro dependency
- **`include/EditBox.h`**: Removed `#include <SDL3/SDL_keyboard.h>` (no SDL keyboard types used in header)
- **`include/Bench.h`** / **`src/Bench.cpp`**: Removed dead `drawCenteredRectangle(SDL_Renderer*)` method
- **`test/test_label.cpp`**: `BENCH->getRenderer()` → `MAINWIN->getRenderer()`

**Result**: `ControlBase.h` no longer has any direct SDL include or SDL type in its public API.

### 2026-06-04: Design Documents Update

**BackendAbstraction_Design.md** updates:

- Progress table (§1.4): Added Phases 7-11 (TTF_Text caching, ResourceProvider, Remove ResourceLoader, SDL Cursor, Remove SDL_Renderer)
- New sections (§10-§14): Detailed design records for Phases 7-11
- §7.3 (current status): Updated to reflect Phase 11 completion; added ResourceProvider/Cursor to ready interfaces
- SFML/raylib sections renumbered to Phase 12/13 and moved to §15-§16
- Summary table (§17) and execution order (§18) updated with all 13 phases
- Note added about numbering divergence between plan and execution

**ResourceLoader_Design.md** updates:

- Marked as deprecated (⚠️), noting replacement by ResourceProvider
- Kept original content for historical reference

### 2026-06-05: Phase 6 — MainWindow Cleanup + InputBackend Standalone + SDL-free MainWindow.h + AppCallbacks Migration (Complete)

**Note**: Phase 6 in the design doc covers Window abstraction, InputBackend, BackendPlugin, Event system, and AppCallbacks. BackendPlugin was previously completed; this session focused on the remaining Window/InputBackend cleanup and all-test migration.

**Infrastructure Changes**:

- **`src/InputBackend.cpp`** (new): Extracted `SDL3InputBackend` from `src/Window.cpp` into its own file; factory uses `window->nativeHandle()` instead of SDL-specific cast
- **`src/Window.cpp`**: Removed `SDL3InputBackend` class and `CreateSDL3InputBackend` factory (moved to InputBackend.cpp)
- **`include/MainWindow.h`**: Removed `#include <SDL3/SDL.h>`, `SDL_Renderer* m_renderer`, `SDL_DisplayID m_displayId`, `getWindow()` (SDL_Window*), `getRenderer()` (SDL_Renderer*); replaced direct SDL calls with `Window` abstract methods; replaced `handleWindowEvent(SDL_WindowEvent&)` with `onWindowResized(int,int)` / `onWindowMoved(int,int)`; renamed `getWindowObject()` → `getWindow()`; now SDL-free
- **`include/Bench.h`**: Replaced `SDL_AppResult` with `int` (removed SDL type from header)
- **`src/Bench.cpp`**: `SDL_APP_CONTINUE` → `0`; `SDL_AppResult` → `int`
- **`src/EditBox.cpp`**: Added explicit `#include <SDL3/SDL_keyboard.h>` and `<SDL3/SDL_keycode.h>` (lost transitive include from MainWindow.h)
- **`src/TextArea.cpp`**: Same SDL keyboard include fix
- **`src/Actor.cpp`**, **`src/CheckBox.cpp`**, **`src/ControlBase.cpp`**, **`src/Label.cpp`**, **`src/LayoutParser.cpp`**, **`src/ProgressBar.cpp`**: Added explicit `#include <SDL3/SDL.h>` (lost transitive include)
- **`include/MainWindow.h`**: Added `run(AppCallbacks*)` for owned-loop mode, plus tick-based `init/processEvents/update/render/shutdown` API
- **`src/MainWindow.cpp`** (new): Implements `run()` — polls InputBackend, handles WindowClose/Resize/Move, dispatches old-style to BENCH, notifies AppCallbacks via `onUpdate()`/`onRender()`/`onEvent()`
- **`src/BackendPlugin.cpp`**: Moved `SDL_Init` + `SDL_SetAppMetadata` from test files into `BackendManager::initialize()`
- **`CMakeLists.txt`**: Added `src/InputBackend.cpp`, `src/MainWindow.cpp`; added `_HAS_STD_BYTE=0` for Windows SDK compatibility

**AppCallbacks & Event Infrastructure**:

- **`include/AppCallbacks.h`** (new): Abstract interface — `onInit()`, `onEvent(const Event&)` (optional, default empty), `onUpdate()`, `onRender()`, `onQuit()`
- **`include/EventTypes.h`**: Added `WindowMoved` event type + `EventWindowMoved` struct; enum now has MouseMove/MouseDown/MouseUp/MouseWheel/KeyDown/KeyUp/TextInput/WindowResize/WindowMoved/WindowClose/FocusGained/FocusLost
- **`include/InputBackend.h`**: Added `pollEvent(Event&)` abstract method — polls SDL events, populates both new (EventType+union) and old (EventName+std::any) fields for dual-run backward compatibility
- **`src/InputBackend.cpp`**: Full `SDL3InputBackend::pollEvent()` implementation handling all 10 SDL event types (mouse, keyboard, text, wheel, window)
- **`include/StateMachine.h`**: Added default `Event` constructor, `EventWindowMoved` in union, `copyUnion()` handles WindowMoved, `<memory>` include

**All 10 Test Files Migrated (SDL callbacks → AppCallbacks)**:

- **`test/test_button.cpp`**: Uses **Mode 1** (owned loop) — `MAINWIN->run(&app)`. Migrated in previous session.
- **The other 9 test files** use **Mode 2** (tick-based API) with SDL callbacks preserved. SDL provides the main loop; the SDL callbacks delegate to MainWindow's Mode 2 lifecycle methods:
  ```
  // SDL_AppInit → g_app.onInit() or MAINWIN->init(&g_app)
  // SDL_AppEvent → unchanged (dispatches to BENCH directly)
  // SDL_AppIterate → MAINWIN->update(&g_app) + MAINWIN->render(&g_app)
  // SDL_AppQuit → MAINWIN->shutdown(&g_app)
  ```
- Each test file adds an AppCallbacks subclass (`LabelApp`, `EditBoxApp`, etc.) with `onInit()`/`onUpdate()`/`onRender()`/`onQuit()`, and a static `g_app` instance. The SDL_App* functions call into these AppCallbacks methods via MainWindow's Mode 2 API.
- **`test/test_graphtool.cpp`**: Custom `SColor(0.941f)` background; no BENCH, uses `DrawingContext` directly; Space/Escape handling stays in `SDL_AppEvent`; timer auto-stepping in `onUpdate()`
- **`test/test_progressbar.cpp`**: Animation logic (`SDL_GetTicks()`) in `onUpdate()`
- **`test/test_layout_advanced.cpp`**: `g_reloader.poll()` in `onUpdate()`
- **`test/test_menu.cpp`**: `testGraphToolInitialize()` in `onInit()`
- **`test/test_editbox.cpp`**: `GET_INPUTBACKEND->startTextInput()` in `onInit()`
- `#define SDL_MAIN_USE_CALLBACKS 1` and `#include <SDL3/SDL_main.h>` are **preserved** in all 9 files (SDL callback mode remains active)

**Result**: Phase 6 complete. All 10 tests build and run successfully. Both Mode 1 (`run()`) and Mode 2 (tick-based API called from within SDL callbacks) are validated.

### 2026-06-05: Phase 12 — Event System Migration (Controls → Union API)

**Changes**: Migrated all 8 control handleEvent() implementations from old Event API (`EventName` + `std::any` + `any_cast`) to new union-based API (`EventType` + union fields):

- **EditBox.cpp**: 7 handlers migrated — MOUSE_LBUTTON_DOWN, MOUSE_MOVING, MOUSE_LBUTTON_UP, TEXT_INPUT, KEY_DOWN, KEY_UP, ON_FOCUS watcher (ON_FOCUS remains old API as custom internal event)
- **TextArea.cpp**: 12 handlers migrated — TEXT_INPUT, KEY_DOWN (×3), mouse scrollbar delegation, MOUSE_LBUTTON_DOWN, MOUSE_WHEEL, MOUSE_MOVING, MOUSE_LBUTTON_UP, KEY_DOWN navigation
- **ScrollBar.cpp**: 3 handlers — MOUSE_LBUTTON_DOWN, MOUSE_LBUTTON_UP, MOUSE_MOVING
- **CheckBox.cpp**: 1 handler — position event check → EventType switch
- **Button.cpp**: 1 handler — position event check + FINGER event fallback
- **Label.cpp**: 1 handler — position event check + FINGER event fallback
- **Menu.cpp (3 classes)**: MenuItem, MenuPanel, MenuBar — all position event checks migrated
- **WinFrame.cpp**: 1 handler — comprehensive hasPos + event type checks migrated

**Key changes per control**:

- Removed all `std::any_cast<shared_ptr<SPoint>>` / `std::any` dependencies
- Removed all `try { ... } catch (...) { }` blocks from event handlers
- Mouse events use `event->mousePos` (for MouseMove) or `event->mouseButton` (for MouseDown/Up)
- Keyboard events use `event->keyEvent` directly
- TextInput uses `event->textInput.text` (char[32])
- MouseWheel uses `event->mouseWheel`
- FINGER_* events (custom, no EventType) retain old API fallback in Button/Label

**Status**: All 10 tests build successfully.

### 2026-06-05: Phase 12 Fix — Event(EventName, any) 构造自动映射

**Bug**: Mode 2（SDL 回调）测试文件通过 `Event(EventName, any)` 旧构造函数创建事件，该构造将 `m_type` 硬编码为 `EventType::None`，导致所有控件的新 API 类型检查（`event->m_type == EventType::MouseDown` 等）失败。

**Root cause**: `StateMachine.h` 中 `Event` 类仅有 `EventName` 前向声明，无法在构造函数体内访问枚举值做映射。

**Fix**:

- `StateMachine.h`: `Event(EventName, any)` 构造声明移至类外（无函数体）
- `EventQueue.h`: 提供内联构造体，完整实现 `EventName → EventType` 映射 + union 字段填充（MouseMove/MouseDown/MouseUp/MouseWheel/TextInput/KeyDown/KeyUp）
- `EventTypes.h`: 移入旧数据结构（`KeyEventData/TextInputEventData/FocusEventData/MouseWheelEventData`）+ `<string>`
- `EditBox.h/TextArea.h`: 删除重复数据结构定义；恢复意外删除的 `#include "Label.h"`（提供 AlignmentMode）

**结果**: 无需修改任何测试文件，旧构造函数自动为新旧双 API 填充数据。所有 10 测试事件响应正常。

### 2026-06-05: Phase 13 — SFML Backend + Remove SDL from Core Sources (In Progress)

**Problem**: Core source files (`src/*.cpp`) still used SDL API calls directly (`SDL_Log`, `SDL_GetTicks`, `SDL_GetBasePath`, `SDL_GetMouseState`, etc.), preventing SFML backend compilation.

**Platform Abstractions**:

- `include/PlatformUtils.h` (new): `Platform::Log()` (printf-based), `Platform::GetTicks()` (std::chrono), `Platform::GetBasePath()` (Win32 GetModuleFileNameA)
- `include/Window.h`: Added `getMousePosition(float& x, float& y)` — returns mouse coords relative to window
- `include/InputBackend.h`: Added `getModState()` — returns keyboard modifier state as int

**Core File SDL Removal (17 files)**:

- `src/ConstDef.cpp`: `SDL_GetBasePath()` → `Platform::GetBasePath()`, `SDL_Log()` → `Platform::Log()`
- `src/ControlBase.cpp`: `SDL_GetTicks()` → `Platform::GetTicks()`
- `src/Bench.cpp`: `SDL_GetTicks()` → `Platform::GetTicks()`
- `src/Label.cpp`: `SDL_GetTicks()` → `Platform::GetTicks()`
- `src/Actor.cpp`: Removed SDL include; `Platform::Log()` for errors
- `src/CheckBox.cpp`: Removed SDL include
- `src/LayoutParser.cpp`: Removed SDL include
- `src/ProgressBar.cpp`: Removed SDL include; added `#define NOMINMAX` before includes
- `src/HotReloader.cpp`: `SDL_GetTicks()` → `Platform::GetTicks()`, `SDL_Log()` → `Platform::Log()`
- `src/EditBox.cpp`: Added `#define NOMINMAX` before includes
- `src/TextArea.cpp`: Added `#define NOMINMAX` before includes
- `include/LuotiAni.h`: `fopen`/`fread` → `FILE*` I/O instead of SDL I/O; removed `SDL_IOFromFile`/`SDL_ReadIO`
- `include/ResourceProvider.h`: Removed `SDL_IOStream*` return type and parameter
- `src/ResourceProvider.cpp`: Removed `SDL_IOStream*` dependency
- `include/SColor.h`: Added `NOMINMAX` guard before SDL `windows.h` conflict
- `include/Utility.h`: `SDL_Log()` in `assertMsg` → `Platform::Log()`
- `src/BackendManager.cpp`: `#ifdef` guards around backend-specific `extern BackendAPI`
- `include/InputBackend.h`: Restored SDL3 factory declarations under backend namespace guards

**SFML Backend Implementation**:

- `src/backend/sfml/Window.cpp`: `getMousePosition()` returns `sf::Mouse::getPosition(*m_window)`
- `src/backend/sfml/InputBackend.cpp`: `getModState()` returns `sf::Keyboard::isKeyPressed()` for Ctrl/Alt/Shift
- `src/backend/sfml/RenderDevice.cpp`: Full SFML render device (415 lines) — draws primitives via sf::RenderWindow/OpenGL; Surface via sf::Image; Texture via sf::Texture

**SVG Rasterization (nanosvg)**:

- **Problem**: LuotiAni animation engine uses SVG as native image format. SFML's `sf::Image` doesn't support SVG, so `Surface::loadFromMemory` called with SVG data printed `Failed to load image from memory` to `sf::err()` and returned a SharedSurface wrapping a 0x0 image, silently corrupting animation data.
- **Fix**: Added bundled nanosvg (from SDL3_image's source tree) to SFML backend (`src/backend/sfml/nanosvg.h`, `nanosvgrast.h`). `Surface::loadFromMemory` now detects SVG data by magic bytes (`<?xml`, `<svg`, `<!DOCTYPE`) and rasterizes it via `nsvgParse`/`nsvgRasterize` to RGBA pixels, creating a valid `sf::Image`. Non-SVG paths use the original `sf::Image` constructor with try-catch + 0x0 dimension check.
- `src/backend/sfml/RenderDevice.cpp`: Added `#include <cstdlib>`, `<cmath>`, `<string.h>`, `<stdlib.h>`, `<math.h>` for nanosvg; replaced `Surface::loadFromMemory` with SVG-aware implementation

**Build Results**:

- SDL3 backend: All 10 test executables compile and run (`test_button` verified)
- SFML backend: `UICornerstone.lib` compiles; all 10 test executables compile and link; `test_button` runs without errors (no more `Failed to load image from memory`), animation buttons render via nanosvg SVG rasterization

### 2026-06-06: Phase 13 Fixes — 4 SFML Visual/Performance Issues (Complete)

**Problem**: User reported 4 SFML-specific issues after test restoration:

1. test_editbox: lag after typing
2. test_graphtool: missing middle black in test group 3
3. test_layout_advanced: resize mispositions controls
4. test_progressbar: animation slower

**Changes (14 files)**:

**SFML Window.cpp:84**: Removed `setVerticalSyncEnabled(true)` — matches SDL3 behavior (no vsync), improves frame rate in tests that don't need it.

**SFML RenderDevice.cpp**: Fixed `drawRect(filled=true)` with SBrush::NoPen — now draws pen outline even in filled mode. `fillRect` issues: changed from `sf::VertexBuffer` to per-rectangle `sf::RectangleShape` (avoids primitive batch overflow). `drawTexture` now works with non-power-of-two textures + `sf::Quads`. Added `fillTriangle` implementation via `sf::ConvexShape`. Removed `sf::PrimitiveType::TriangleFan` approach for `drawTriangle`.

**SFML TextRenderer.cpp**: Fixed `drawText(void*, wrapWidth)` crash — null-check cached `m_textObj`. Fixed `drawText(Font*, text, x, y, color)` — now uses cached `sf::Text` (global static map, keyed by text string + font size) instead of creating/destroying per frame. Fixed alignment offsets.

**SFML InputBackend.cpp**: Fixed `getModState()` — now returns `KeyMod::None` explicitly (was returning uninitialized uint16_t). Fixed mouse wheel delta sign (SFML delta is typically -1/+1 vs SDL3 expects -120/+120).

**SFML Window.cpp**: Fixed resize event not using `sf::View` → viewport mismatch. Now calls `m_window->setView(sf::View(sf::FloatRect(0, 0, width, height)))` on resize to match SDL3 auto-viewport behavior.

**SFML BackendPlugin.cpp**: Added `sf::ContextSettings` with `sf::StencilMask` for stencil support (required by `sf::ConvexShape`/`sf::RectangleShape` rendering path).

**SFML Clipboard/TextInput**: Moved clipboard code after `SFML/Window.hpp` to avoid Windows.h macro pollution. Text input state now uses boolean toggle.

**Test files**: Changed all 10 tests from `BENCH->setOnInitial(...)` to direct initialization call (no longer needed without SDL callback mode). Removed SDL_App* callbacks, `SDL_MAIN_HANDLED`, `SDL_main` headers. All tests now use `MAINWIN->run(&app)` (Mode 1) with `main()` as entry point. Maintains `AppCallbacks` pattern.

**Bench.cpp**: Fixed `ControlBase.cpp` missing `break` in Disabled switch case. Fixed `EventQueue.h` mouse-button mapping (default case now maps to `MouseButton::Left` instead of falling through).

**Remaining Issues**: SFML `setVerticalSyncEnabled` was removed to match SDL3 frame rate. If animations still appear slower on SFML, the issue is likely `sf::sleep(sf::milliseconds(1))` in Window.cpp event polling — SDL3 does not sleep between poll cycles.

**Known Issues**:

- NOMINMAX required in source files because SDL3 headers in `subModules/` pull in `windows.h` transitively via public include dir

### 2026-06-07: Raylib InputBackend — Infinite MouseDown Loop Fix (Complete)

**Problem**: Raylib's `IsMouseButtonPressed()` returned `true` every time `pollEvent()` was called because the phase machine reset to `Keyboard` on every `GetTime()` advancement (which advanced between calls within the same frame). This cleared `m_consumedMouseButtons`, causing the same press to be detected again → infinite `MouseDown` event flood.

**Root cause**: `GetTime()` in `pollEvent()` advanced by microseconds between consecutive calls within `processEvents()`'s `while (pollEvent(event))` loop, restarting the phase machine and clearing consumed state each time.

**Fix (4 files)**:

- `include/InputBackend.h`: Added `virtual void newFrame() {}` — signals start of a new frame's event processing. Default empty so SDL3/SFML backends need no changes.
- `src/backend/raylib/InputBackend.cpp`:
  - `newFrame()` override: Resets `m_phase` to `Keyboard`, `m_consumedMouseButtons` to `0`, `m_keyConsumed`/`m_charConsumed` to `false`.
  - Modified `GetTime()` check in `pollEvent()` to NOT reset `m_consumedMouseButtons` — it now only resets `m_keyConsumed`/`m_charConsumed` (for multi-key-per-frame processing). `m_consumedMouseButtons` persists across all `pollEvent()` calls within a render frame.
- `src/MainWindow.cpp`: `processEvents()` calls `inputBackend->newFrame()` before entering the `while (pollEvent(event))` loop.
- Removed all debug `printf` traces from `MainWindow.cpp` and `RenderDevice.cpp`.

**Result**: `test_button` runs without infinite loop. No more `MouseDown` event flooding.

**Status**: Raylib backend builds and runs. No other backends affected.

### 2026-06-07: Phase 14 C2 — Raylib drawTriangle/drawQuad CCW Winding Fix

**Problem**: test_graphtool only showed the red thin line (width=1, uses `DrawLineEx`). All thick lines (width > 1, via `drawQuad` → `DrawTriangle`) and triangle-based fills (filled rounded rects/ellipses/arcs/polygons via `drawTriangle`) were invisible.

**Root cause**: raylib's `DrawTriangle(v1, v2, v3, color)` requires CCW (counter-clockwise) winding and does NOT flip CW triangles internally. The initial `drawTriangle` and `drawQuad` implementations in `src/backend/raylib/RenderDevice.cpp` had the winding logic inverted:

- In y-down coordinates: `signed_area > 0` → CW, `signed_area < 0` → CCW
- Bug: when `area < 0` (CCW), the code **swapped** the last two vertices making the triangle CW; when `area >= 0` (CW), it passed the triangle as-is (still CW)
- Result: **every triangle** was passed as CW to `DrawTriangle` and culled by OpenGL backface culling

**Fix** (`RaylibRenderDevice::drawTriangle` and `drawQuad`):

- CCW input (`area < 0`): pass vertices **as-is** → `DrawTriangle(v0, v1, v2)`
- CW input (`area >= 0`): **swap** last two vertices → `DrawTriangle(v0, v2, v1)`

**Impact**: All triangle-based rendering now works correctly in the raylib backend (thick lines, rounded rects, ellipses, arcs, polygons, styled drawing, checkmarks, menu backgrounds).

**Status**: All 10 tests build and run. SDL3/SFML backends unaffected (they handle winding internally).

### 2026-06-08: Phase 14 D1 — Raylib Resize Freeze & Font Size Fix (Complete)

**Problem (resize freeze)**: Dragging the window border to resize and releasing the mouse caused the raylib backend to freeze. Root cause traced through multiple iterations:

**Root cause chain**:

1. `EndDrawing()` (called inside `present()`) calls `PollInputEvents()` internally.
2. `InputBackend::newFrame()` also called `PollInputEvents()` → double‑polling consumed the `IsMouseButtonPressed/Released` one‑shot state, breaking mouse clicks.
3. Removing `EndDrawing()` from `present()` fixed double‑polling but the resize freeze returned.
4. **Real freeze cause**: during/after a resize drag, `IsWindowResized()` returned TRUE **500+ times** in a single `processEvents()` call, flooding the event loop with duplicate `WindowResize` events of the same dimensions and preventing rendering.
5. After the drag, the `MOUSE_UP` arriving in the same event batch triggered a control handler that froze.

**Fixes (6 files)**:


| File                                          | Change                                                                                                                                                                                                                                                                      |
| --------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/backend/raylib/TextRenderer.cpp`         | Font cache (`unordered_map<FontCacheKey, weak_ptr<RaylibFont>>`). Load font at `size * 96/72` to match SDL3_ttf point→pixel conversion. `getFontHeight` uses `MeasureTextEx("Ay")`. `wrapWidth` scaled by 96/72.                                                           |
| `src/backend/raylib/InputBackend.cpp`         | `newFrame()` calls `PollInputEvents()` once per frame. Phase order changed to `Keyboard → CharInput → **WindowEvents** → MouseButton → MouseMove → MouseWheel → Done`. `m_resizeDetected` flag suppresses `MOUSE_UP` when a `WindowResize` arrives in the same batch. |
| `src/backend/raylib/RenderDevice.cpp`         | `present()` no longer calls `EndDrawing()` (avoids internal `PollInputEvents`). Manually flushes batch + `SwapScreenBuffer()`. 60 Hz `WaitTime` frame limiter.                                                                                                             |
| `src/backend/raylib/Window.cpp`               | Removed`SetTargetFPS(60)` (moved to RenderDevice). Added `SetTraceLogLevel(LOG_WARNING)`. Fixed flag‑bit mapping (`0x00000020` → `FLAG_WINDOW_RESIZABLE`).                                                                                                                |
| `include/MainWindow.h` + `src/MainWindow.cpp` | WindowResize dedup by`(w, h)` — skips events with same dimensions as last processed. 500‑event safety guard in `processEvents`.                                                                                                                                           |

**Key insights**:

- `IsWindowResized()` in the bundled raylib returns TRUE repeatedly for the same dimensions (the flag is set by a GLFW callback but not cleared by the first `IsWindowResized()` call in this raylib build).
- `EndDrawing()` must not be called in `present()` because its internal `PollInputEvents()` collides with the one in `newFrame()`.
- Resize drag produces `MOUSE_UP` that confuses controls when arriving without a visible `MOUSE_DOWN` (the press was on the window border / NC area).

**Status**: All 10 raylib tests build and run. Resize is smooth, mouse clicks work, close works. SDL3 backend unaffected.

## Raylib Backend Notes

- `EndDrawing()` is never called — `present()` manually flushes + swaps
- `PollInputEvents()` is called only in `InputBackend::newFrame()`, once per frame
- `SetTargetFPS` is not used — frame rate limited by `WaitTime` in `RenderDevice::present()`
- Fonts loaded at `size * 96/72` and cached by `(data, size, cpHash)`

### 2026-06-09: Phase 15 — CheckBox/Label Performance Optimization (Complete)

**Problem**: `test_checkbox` took ~48s to initialize 16 checkboxes. Root cause was an O(n²) recreate cascade amplified by missing font cache in SDL3 backend.

**Root cause chain**:

1. `Bench::addControl()` → `resolveChildPercentages()` iterated ALL children, calling `setRect` on every existing child.
2. `CheckBox::setRect()` called `recreate()` unconditionally (no dirty-rect check).
3. Each CheckBox recreate destroyed and re-created its Label caption, including font reloading.
4. SDL3 TextRenderer had **no font cache** — every `loadFontFromMemory()` called `TTF_OpenFontIO()`, taking ~800ms per call.
5. With 16 checkboxes, the cascade produced 120 redundant recreates × 3 font loads = 360+ font opens.

**Changes (5 files)**:


| File                                  | Change                                                                                                                                                                                                                    |
| ------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/backend/sdl3/TextRenderer.cpp`   | Added font cache (`m_fontCache` keyed by `(contentHash, size)`, `m_pathCache` by `(path, size)`). Eliminated redundant `TTF_OpenFontIO` calls.                                                                            |
| `include/CheckBox.h`                  | Added`bool m_layoutDone = false` member.                                                                                                                                                                                  |
| `src/CheckBox.cpp`                    | `setRect` dirty-rect check. `recreate()` resets `m_layoutDone` to `false`. `create()` sets `m_layoutDone = true` after layout. `createCaption()` callback checks `m_layoutDone` before calling `adjustSpaceAssignment()`. |
| `src/Label.cpp`                       | `setRect` dirty-rect early return. `setParent` dirty-parent early return.                                                                                                                                                 |
| `src/backend/raylib/TextRenderer.cpp` | Removed debug`printf` and `Platform::GetTicks()` timing (leftover from earlier session — only removed unused variables, font cache was already present).                                                                 |

**Debug instrumentation removed from production sources**:

- `Label.cpp`: `g_recreateDepth` thread_local + `[LABEL_RECREATE]` printf
- `CheckBox.cpp`: `[CHECKBOX_RECREATE]` timing printf
- `SDL3 TextRenderer.cpp`: `[FONT_LOAD]` + `[FONT_HIT]` timing printf
- `Raylib TextRenderer.cpp`: `[FONT_HIT]` + `[FONT_RELOAD]` timing printf
- All unused `Platform::GetTicks()` / timing variables removed

**Results**:


| Backend | Before | After |
| ------- | ------ | ----- |
| SDL3    | ~48s   | ~7.2s |
| SFML    | —     | ~6.9s |
| Raylib  | —     | ~4.6s |

All 10 tests build and run on all 3 backends. ~6.5× speedup on SDL3.

**Remaining cost**: ~80 `TTF_CreateText` calls (~25ms each) during the 16-checkbox initialization. Further optimization would require caching `TTF_Text*` objects across recreates (risky — text content can change).

### 2026-06-12: Phase 15 Bugfix Round 1 — 6 Fixes Across All Backends

**Fix 1 — EditBox ON_FOCUS event union corruption (all backends)**:

- Root cause: `Event` class in `include/StateMachine.h` had `customInt` and `customPtr` in the same union. When `EditBox::setFocused()` set both fields, the second assignment (`customPtr = this`) overwrote the first (`customInt = ON_FOCUS`), so `beforeEventHandlingWatcher` could never match `EventName::ON_FOCUS` and never called `setFocused(false)` on the previously-focused EditBox.
- Fix: Moved `customInt` and `customPtr` out of the union as regular members. Updated all constructors, copy/move operators, and `copyUnion()` (removed `case EventType::Custom`).

**Fix 2 — WinFrame title bar drag area (all backends)**:

- Problem: Title bar drag (Step 4) covered the full title bar height at any X. Left edge dragging was possible on non-resizable WinFrames.
- Fix: Added `localMouse.x >= m_edgeMargin` and `localMouse.y >= m_edgeMargin` checks to restrict drag to the interior of the title bar, excluding resize edge zones and the close button area.

**Fix 3 — Raylib Chinese TextInput shows "?"**:

- Problem: `RaylibTextRenderer::loadFontFromMemory()` only loaded ASCII codepoints (0x20–0x7E) on initial font load. `EditBox::insertText()` modified `m_text` but never called `loadFontInternal()`, so the TextRenderer's lazy expansion (via `loadFontFromMemoryWithText`) was never triggered with the new CJK codepoints.
- Fix: Added `loadFontInternal()` call in `EditBox::insertText()` to reload the font with the updated text's codepoints via `loadFontFromMemoryWithText()`. Also added `ensureFontCodepoints()` lazy expansion in the raylib TextRenderer's `drawText()`/`measureText()` for the cached `void*` text path used by Label.

**Fix 4 — Raylib EditBox direction key repeat**:

- Problem: `m_repeatKey` was reset to 0 in `newFrame()`, losing the key repeat state between frames. Holding Left/Right arrow keys did not generate repeated `KeyDown` events.
- Fix: Removed `m_repeatKey = 0;` from `newFrame()` so the repeat state persists across frames. The `IsKeyDown(m_repeatKey)` check ensures stale keys are ignored. Key repeat timing: 350ms initial delay / 50ms repeat interval.

**Fix 5 — Remove DebugTrace**:

- Removed `#include "DebugTrace.h"` from `include/MainWindow.h`. Removed two `DEBUG_STREAM` calls in `onWindowResized()` and `onWindowMoved()`.

**Fix 6 — Add test name to window title**:

- Added `MainWindow::setTitle()` delegating to `Window::setTitle()`.
- Added `MAINWIN->setTitle("test_xxx");` as first line in each of the 10 test files' `onInit()` method.

**Verification**: All 10 tests build and run on all 3 backends (SDL3, SFML, Raylib).

### 2026-06-12: R2-R4 — UICornerstone C ABI Implementation (Compile Complete)

**New files**:

- `include/UICornerstoneAPI.h`: 公有 C ABI 头文件 — `UIBackendCallbacks` 回调查表（7 类回调约 40 个函数指针）、`UIControlHandle`/`UIRenderDeviceHandle` 等 10+ 个不透明句柄、所有 `UICornerstone_*` 函数声明
- `src/CallbackAdapters.h` + `.cpp`: 5 个 Adapter 类（CallbackWindow/RenderDevice/InputBackend/TextRenderer/ResourceProvider）将回调查表委托为现有的 C++ 抽象接口
- `src/UICornerstoneAPI.cpp`: C ABI 实现 — `Init` / `Shutdown` / `SetViewport` / `ProcessEvents` / `Update` / `Render` / `IsQuitRequested` + 6 个控件工厂（Button/Label/CheckBox/EditBox/ProgressBar/Panel）+ 通用控件操作 + `LoadLayout`/`FindControl`/`RegisterAction` 骨架

**编译修复**:

- SRect 成员名：所有 `.x`/`.y`/`.w`/`.h` → `.left`/`.top`/`.width`/`.height`
- CheckBox 文本：通过 `getCaption()->setCaption(text)` 设置
- `Button::setOnClick` lambda：参数包装为 `shared_ptr<Button>`
- `LayoutParser` 接口：使用 `parseLayout()` 而非 `parse()`
- 添加 `#include "StateMachine.h"` 到 CallbackAdapters.cpp 以使用完整 Event 类型
- 所有 4 个新文件保存为 UTF-8 with BOM 解决 MSVC C4819

**验证**：

- `UICornerstone.lib` 编译 0 错误
- 全部 10 个 SDL3 测试编译通过（无回归）

### 2026-06-12: R5 — JSON 布局 C ABI 包装 (Complete)

**`UICornerstone_LoadLayout` 实现**：

- 注册所有 `g_actions` 中的回调到 LayoutParser（通过 `registerHandler` 适配 `UIActionCallback` → `function<void(shared_ptr<Control>)>`）
- 调用 `parser.parseLayout()` 解析 JSON
- 成功后添加根控件到 `BENCH`（`BENCH->addControl(root)`）
- 添加所有 MenuBar 到 `BENCH`
- 遍历 `parser.getAllControlIds()`，将每个控件注册到 `g_controlsById`（使 `UICornerstone_FindControl` 可用）

**`UICornerstone_Render` 修正**：

- 移除 `clear()` 和 `present()` — 调用者负责管理全帧生命周期
- 仅做 `setClipRect(g_viewport)` → `BENCH->draw()` → `clearClipRect()`

**验证**：编译通过，全部 10 个 SDL3 测试无回归。

### 2026-06-24: Slider 刻度线 + 刻度标签（Complete）

**问题**：水平滑块的刻度线应该只在轨道下方绘制（非居中），且应带有刻度数值标签。

**变更**（6 文件）：


| 文件 | 变更 |
|------|------|
| `include/ConstDef.h` | 添加 `SLIDER_TICK_INTERVAL/LENGTH/COLOR` 常量 |
| `src/ConstDef.cpp` | 默认值：interval=0，length=8，color=灰(100,100,100) |
| `include/Slider.h` | 添加 `m_tickInterval/length/color` 成员 + `m_tickFont/m_tickFontData/m_tickLabelFontSize` + `setTickInterval/Length/Color` + Builder 方法 |
| `src/Slider.cpp` | `draw()` 中：水平滑块在轨道下方绘制竖线 + 数值标签；垂直滑块在轨道左侧绘制横线 + 数值标签；懒加载 `m_tickFont`（通过 ResourceProvider + 预加载字体数据） |
| `src/LayoutParser.cpp` | 添加 `"tick": {interval, length, color}` JSON 解析 |
| `test/test_slider.cpp` | 添加 Slider9：范围 0-100，tickInterval=10，tickLength=8，浅灰刻度 |

**刻度绘制逻辑**：
- 水平：刻度线从轨道底部向下延伸 `tickLength`，标签在刻度线下方 2px 居中
- 垂直：刻度线从轨道左侧向左延伸 `tickLength`，标签在刻度线左侧 2px 居中
- tickInterval=0（默认）时不绘制刻度
- 刻度标签使用 `m_labelFont` 字体 + `m_tickLabelFontSize`（默认 10px），懒加载缓存

**构建验证**：三后端（SDL3/SFML/Raylib）全部编译通过。test_slider 运行正常，9 个滑块（含刻度滑块）初始化 + 3 秒运行无崩溃。

### 2026-06-21: HandleControl 光标 + 视觉优化 (Complete)

**问题**：SDL3 后端 `SDL_SetCursor` 不持久，Windows `WM_SETCURSOR` 消息在每帧
复位光标。WinFrame 使用 Win32 `SetCursor` 绕过该问题，但 HandleControl 的
`setResizeCursor` 通过 `Cursor::setCurrent` 调用 `SDL_SetCursor`，导致光标
反馈始终不可见。

**Bug 1 — 光标不出现**：

- Root cause: `SDL_SetCursor` 在此 SDL3 fork 中不能持久化，Windows 的
  `WM_SETCURSOR` 消息在每帧鼠标移动时复位光标为默认
- Fix: `HandleControl::setResizeCursor()` 在 `#ifdef _WIN32` 路径下直接调用
  Win32 `SetCursor(LoadCursor(...))`，匹配 WinFrame 做法
- 添加中心十字星背景，以及十字星拖拽/缩放和释放时光标保持和恢复

**Bug 2 — Move 手柄区域太小难以命中**：

- `updateHandleAreas` 中 Move 手柄只有中心 8×8 像素区域
- 用户反馈后恢复原始设计（只在小方块和十字星上触发光标变化）

**Bug 3 — 拖拽中光标恢复为默认**：

- 按下鼠标启动拖拽/缩放后，后续 MouseMove 事件被拖拽逻辑消费，
  `setResizeCursor` 未被调用
- Fix: 在 `m_resizing`/`m_dragging` 分支的 MouseMove 中也调用 `setResizeCursor`

**Bug 4 — 按下鼠标瞬间光标恢复为默认**：

- MouseDown 中 `startDrag/startResize` 之后没有立即设置光标
- Fix: MouseDown 分支末尾加 `setResizeCursor(ht)`

**视觉优化**：

- 8 个角/边手柄保持白底蓝边方块
- 中心十字改为蓝边白底十字（5px 蓝线 + 3px 白线 + 两端各延 1px）
- 修复十字星下方重叠绘制方块的问题（循环中跳过 HandleType::Move）

**变更文件**：


| 文件                          | 变更                                                                                         |
| ----------------------------- | -------------------------------------------------------------------------------------------- |
| `src/HandleControl.cpp`       | Win32 SetCursor 路径；拖拽中光标保持；按下瞬间光标；十字星蓝边白底；移除方块循环中 Move 绘制 |
| `design/HandleControl_Design.md` | 更新 drawMoveHandle、事件流程、光标 Win32 说明、移除 beforeDraw、m_handleAreas 固定数组      |

**验证**：test_handlecontrol 三后端（SDL3/SFML/Raylib）构建通过。

**问题**：test_fromsource_sfml 和 test_fromsource_raylib 日志中出现 `Cursor::createSystem: no backend factory registered`，光标功能完全失效。

**根因**：SFML 和 Raylib 后端使用**直接方法覆盖**模式（在各自 `Cursor.cpp` 中定义 `Cursor::createSystem()`/`getDefault()`/`setCurrent()` 成员函数），而非 SDL3 所用的**工厂注册**模式（`RegisterSDL3CursorFactories()` → `Cursor::registerFactories()`）。在 fromsource/DLL 模式下，`UICornerstone.dll` 中的 `src/Cursor.cpp`（工厂指针模式）无法获取后端实现。

**修复**（5 文件）：

- `src/backend/sfml/Cursor.cpp`：`Cursor::createSystem/getDefault/setCurrent` 改为静态工厂函数 `sfmlCreateSystemCursor/sfmlGetDefaultCursor/sfmlSetCurrentCursor`；新增 `RegisterSFMLCursorFactories()`
- `src/backend/raylib/Cursor.cpp`：同上（`raylib*` 前缀）；新增 `RegisterRaylibCursorFactories()`
- `src/backend/sfml/BackendPlugin.cpp`：`GetUIBackendCallbacks()` 中添加 `RegisterSFMLCursorFactories()` 调用
- `src/backend/raylib/BackendPlugin.cpp`：同上
- `src/BackendManager.cpp`：SFML/Raylib 静态链接路径添加光标工厂注册

**SFML 默认光标 Bug**：`sfmlGetDefaultCursor()` 返回空构造的 `SFMLCursor`（`m_hasCursor=false`），`sfmlSetCurrentCursor()` 中 `get()==nullptr` 跳过 `setMouseCursor`，导致光标设成手指后无法恢复箭头。修复为初始化为真实 Arrow 光标。

**验证**：三后端（SDL3/SFML/Raylib）静态 + DLL 模式均编译通过。fromsource 测试时间戳更新为 2026-06-21。

### 2026-06-20: sample_loadlibrary 零导入库重构 + 4 samples 全部完成

**问题**：sample_loadlibrary 在 `UICORNERSTONE_BUILD_SHARED` 定义下链接 `UICornerstone_dll.lib`，导致符号解析走 `dllimport` —— `Surface::registerFactories` 等 `CORE_API` 函数的函数体在 DLL 而非 exe 中。当 exe 提供自己的 `registerFactories` 时出现 LNK2001（多重定义）。

**Fix**：

- sample_loadlibrary 不再链接 `UICornerstone_dll.lib`
- 不定义 `UICORNERSTONE_BUILD_SHARED`，`CORE_API` 为空宏 → 无 `dllimport`
- 内联 3 个 Core 符号：`Surface::registerFactories`（no-op）、`Cursor::registerFactories`（no-op）、`ResourceProvider::createFilesystem`（FilesystemResourceProvider 完整实现）
- Cursor 工厂 stub 产生 cosmetic 警告但功能正确（Label 空指针优雅处理）

**所有 4 个 Sample 最终验证**：


| Sample              | 模式        | 后端编译                 | 核心加载            | 零导入库      | 验证         |
| ------------------- | ----------- | ------------------------ | ------------------- | ------------- | ------------ |
| hello_uicornerstone | JSON 布局   | `UICornerstone.lib` 静态 | 无 DLL              | ✅            | build/run ✔ |
| sample_programmatic | C ABI 工厂  | `UICornerstone.lib` 静态 | 无 DLL              | ✅            | build/run ✔ |
| sample_fromsource   | ILT 隐式    | CMake 独立 TU            | `UICornerstone.dll` | ❌ (需导入库) | build/run ✔ |
| sample_loadlibrary  | LoadLibrary | `#include` 同一 TU       | `UICornerstone.dll` | ✅            | build/run ✔ |

**设计文档更新**：

- `design/Sample_Design.md`: §3 新增 loadlibrary 架构图；§7 关键差异说明改为零导入库方式（3 个 Core 符号内联）
- AGENTS.md: 本次 session 记录
- `design/Build_Guide.md` 已在前序 session 更新

### 2026-06-20: hello_uicornerstone sample 实现

**变更**：

- `samples/hello_uicornerstone/hello_uicornerstone.c`: 纯 C 示例（~50 行），Button 点击 → Label 计数，内联 JSON 布局，完全静态链接
- `samples/hello_uicornerstone/CMakeLists.txt`: 单目标 CMake，POST_BUILD 复制 DLL + assets
- `samples/CMakeLists.txt`: 新目录 CMake，转发到子目录
- `CMakeLists.txt`: 添加 `add_subdirectory(samples)`
- `design/Sample_Design.md`: §2 新增后端选择说明（SDK3/SFML/Raylib 对比表），§7 更新实现状态
- `design/Sample_Design.md`: §7 sample_programmatic 标记为"✅ 已实现"
- `design/Build_Guide.md`: 添加 sample_programmatic 到测试表
- `samples/sample_programmatic/sample_programmatic.c`: 纯 C 示例（~45 行），编程式控件创建代替 JSON 布局
- `samples/sample_programmatic/CMakeLists.txt`: 单目标 CMake，输出到 `build/sample/sample_programmatic/<backend>/Debug/`
- `design/Build_Guide.md`: 添加 hello_uicornerstone 到测试表、输出目录、独立构建说明

**验证**：`hello_uicornerstone.exe` 编译通过，启动后 2 秒存活正常，输出显示静态 InitFromPlugin 回退路径正常工作。

### 2026-06-20: SFML/Raylib 静态+DLL 双构建目录 + test_fromsource 改名

**变更**：

- SFML/Raylib 改为与 SDL3 一致：`build/sfml`=静态、`build/sfml_dll`=DLL、`build/raylib`=静态、`build/raylib_dll`=DLL
- `test/test_fromsource.cpp` → `test/test_fromsource_sdl3.cpp`，CMake target 同步改名
- 从静态构建目录中清理出旧 DLL 模式残留文件（`UICornerstone.dll`/`UIBackend_*.dll` 等）
- AGENTS.md 和 `design/UICornerstone_DLL_Design.md` 中引用同步更新

**构建目录结构**：


| 目录               | 模式 |     fromsource 测试     |
| ------------------ | ---- | :----------------------: |
| `build/sdl3`       | 静态 |            —            |
| `build/sdl3_dll`   | DLL  |  `test_fromsource_sdl3`  |
| `build/sfml`       | 静态 |            —            |
| `build/sfml_dll`   | DLL  |  `test_fromsource_sfml`  |
| `build/raylib`     | 静态 |            —            |
| `build/raylib_dll` | DLL  | `test_fromsource_raylib` |

**验证**：6 个构建目录的所有测试 exe + DLL 时间戳均为 2026-06-20。

### 2026-06-20: sample_fromsource — 混合集成（核心 DLL + 后端源码）

**变更**：

- `samples/sample_fromsource/sample_fromsource.c`: 纯 C 示例（~55 行），Button 点击 → Label 计数，`GetUIBackendCallbacks()` + `UICornerstone_Init(callbacks)` 模式
- `samples/sample_fromsource/CMakeLists.txt`: 仅 `UICORNERSTONE_BUILD_DLL=ON` 下构建；将 6 个后端源文件编译进 exe（Window/RenderDevice/TextRenderer/InputBackend/Cursor/BackendPlugin）；链接 `UICornerstone_dll`（导入库，ILT 隐式加载 UICornerstone.dll）
- `samples/CMakeLists.txt`: 添加 `add_subdirectory(sample_fromsource)`
- `design/Sample_Design.md`: §3 新增 fromsource 架构说明；新增 §6 sample_fromsource 代码解读；§7 CMake 章节更新 fromsource CMake；§8 文件清单更新；§9 后续扩展标记为 ✅
- `design/Build_Guide.md`: 添加 sample_fromsource 到测试表、输出目录、独立构建说明、fromsource 节

**关键架构**：exe 272KB（仅有后端源码），UICornerstone.dll 3.2MB（核心控件）。通过 ILT（Import Library Thunk）在启动时自动加载 `UICornerstone.dll`，无需 `LoadLibrary` + `GetProcAddress`。

**验证**：`sample_fromsource.exe` 编译通过，启动后 2 秒存活正常，输出显示 `GetUIBackendCallbacks ready` + `initialized from callback table`。

### 2026-06-20: sample_loadlibrary — 显式 LoadLibrary + #include 后端源码

**变更**：

- `samples/sample_loadlibrary/sample_loadlibrary.cpp`: 纯 C++ 示例（~80 行），Button 点击 → Label 计数，`LoadLibraryA("UICornerstone.dll")` + `GetProcAddress` 解析全部 C ABI 函数；`#include` 6 个后端 .cpp 文件编译入同一 TU；`main()` 帧循环
- `samples/sample_loadlibrary/CMakeLists.txt`: 仅 `UICORNERSTONE_BUILD_DLL=ON` 下构建；后端通过 `#include` 而非独立 TU 编译；链接 `UICornerstone_dll` 导入库（仅用于注册符号，C ABI 全部走函数指针）
- `samples/CMakeLists.txt`: 添加 `add_subdirectory(sample_loadlibrary)`
- `src/UICornerstoneAPI.cpp`: 修复预存在拼写错误 `initifalize` → `initialize`
- `design/Sample_Design.md`: §3 新增 loadlibrary 架构说明；新增 §7 sample_loadlibrary 代码解读（含与 fromsource 对比表）；§8-10 重编号；文件清单和后续扩展表更新
- `design/Build_Guide.md`: 添加 sample_loadlibrary 到测试表、输出目录、独立构建说明、fromsource 节

**与 sample_fromsource 的架构差异**：


| 维度       | sample_fromsource | sample_loadlibrary        |
| ---------- | ----------------- | ------------------------- |
| DLL 加载   | ILT 隐式          | `LoadLibrary` 显式        |
| C ABI 调用 | 直接符号链接      | `GetProcAddress` 函数指针 |
| 后端编译   | CMake 独立 TU     | `#include .cpp` 同一 TU   |

**验证**：`sample_loadlibrary.exe` 272KB 编译通过，启动后 2 秒存活正常，输出显示 `loaded UICornerstone.dll` + `GetUIBackendCallbacks ready` + `initialized from callback table`。

### 2026-06-19: 14 份设计文档批量更新 (Complete)

**更新列表**：


| 文档                        | 主要变更                                                                                                                                                                 |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| ControlBase_Design.md       | 移除`SDL_Renderer*`/`setRenderer`/`getRenderer`；添加 `RenderDevice*`/`TextRenderer*`/`ResourceProvider*` 抽象成员；`addControl`/`setParent` 改用 `inheritRenderer` 传播 |
| Button_Design.md            | handleEvent 重写为 union API（Phase 12）；`SDL_Color` → `SColor` 常量；移除 `setRenderer`                                                                               |
| CheckBox_Design.md          | handleEvent union API；`SDL_Color` → `SColor`；添加 `setRect` 脏矩形优化（Phase 15）                                                                                    |
| EditBox_Design.md           | `TTF_Font*`/`TTF_Text*` → `SharedFont`/`TextRenderer*`；移除 `recreateTextObjects`/`setRenderer`；添加 `m_fontData`/`loadFontInternal`                                  |
| TextArea_Design.md          | 移除`TTF_TextEngine*`；移除 `setRenderer`；handleEvent union API；`rebuildLines`/`updateVScrollBar` 匹配实际源码                                                         |
| ProgressBar_Design.md       | `SDL_Color` → `SColor`；移除 `setRenderer`                                                                                                                              |
| ScrollBar_Design.md         | handleEvent union API（移除`std::any_cast`/`try-catch`）；移除 `setRenderer`                                                                                             |
| WinFrame_Design.md          | `SDL_Cursor*` → `Cursor*`；`ResourceLoader::RID_*` → 内联路径；添加向量 X 叠加层 `WinFrame::draw()`；handleEvent union API                                             |
| Menu_Design.md              | `SDL_Log` → `Platform::Log`；`SDL_Event`/`SDL_PushEvent` → 通用退出机制；`SDL_Color` → `SColor`                                                                       |
| GraphTool_Design.md         | 全面去 SDL3：架构图/代码`SDL_Renderer` → `RenderDevice`；`SColor` 独立类；`drawTriangle`/`drawQuad` 替代 `SDL_RenderGeometry`                                           |
| LayoutSystem_Design.md      | 移除`#include <SDL3/SDL.h>`；`SDL_Log` → `Platform::Log`；`parseFontStyle` 返回 `int`；FontName 迁至 ConstDef.h                                                         |
| ComponentSystem_Design.md   | 最小变更：`SDL_Log` → `Platform::Log`                                                                                                                                   |
| UICornerstone_DLL_Design.md | 状态从"实施中"→"已完成"；新增 §9.4 Fromsource 集成模式（架构/工厂注册/修复表/回调查表扩展）；版本历史扩充至 1.11                                                       |

**验证**：所有 15 份设计文档（`*_Design.md`）均反映当前代码库状态。Label_Design.md 已在 2026-06-05 session 中更新，ResourceLoader_Design.md 已标为废弃，无需重复修改。

### 2026-06-19: 13 份设计文档批量更新 (Complete)

**更新列表**：


| 文档                      | 主要变更                                                                                                                                                                 |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| ControlBase_Design.md     | 移除`SDL_Renderer*`/`setRenderer`/`getRenderer`；添加 `RenderDevice*`/`TextRenderer*`/`ResourceProvider*` 抽象成员；`addControl`/`setParent` 改用 `inheritRenderer` 传播 |
| Button_Design.md          | handleEvent 重写为 union API（Phase 12）；`SDL_Color` → `SColor` 常量；移除 `setRenderer`                                                                               |
| CheckBox_Design.md        | handleEvent union API；`SDL_Color` → `SColor`；添加 `setRect` 脏矩形优化（Phase 15）                                                                                    |
| EditBox_Design.md         | `TTF_Font*`/`TTF_Text*` → `SharedFont`/`TextRenderer*`；移除 `recreateTextObjects`/`setRenderer`；添加 `m_fontData`/`loadFontInternal`                                  |
| TextArea_Design.md        | 移除`TTF_TextEngine*`；移除 `setRenderer`；handleEvent union API；`rebuildLines`/`updateVScrollBar` 匹配实际源码                                                         |
| ProgressBar_Design.md     | `SDL_Color` → `SColor`；移除 `setRenderer`                                                                                                                              |
| ScrollBar_Design.md       | handleEvent union API（移除`std::any_cast`/`try-catch`）；移除 `setRenderer`                                                                                             |
| WinFrame_Design.md        | `SDL_Cursor*` → `Cursor*`；`ResourceLoader::RID_*` → 内联路径；添加向量 X 叠加层 `WinFrame::draw()`；handleEvent union API                                             |
| Menu_Design.md            | `SDL_Log` → `Platform::Log`；`SDL_Event`/`SDL_PushEvent` → 通用退出机制；`SDL_Color` → `SColor`                                                                       |
| GraphTool_Design.md       | 全面去 SDL3：架构图/代码`SDL_Renderer` → `RenderDevice`；`SColor` 独立类；`drawTriangle`/`drawQuad` 替代 `SDL_RenderGeometry`                                           |
| LayoutSystem_Design.md    | 移除`#include <SDL3/SDL.h>`；`SDL_Log` → `Platform::Log`；`parseFontStyle` 返回 `int`；FontName 迁至 ConstDef.h                                                         |
| ComponentSystem_Design.md | 最小变更：`SDL_Log` → `Platform::Log`                                                                                                                                   |

**验证**：所有 14 份设计文档（`*_Design.md`）+ `ResourceLoader_Design.md`（已废弃）+ `BackendAbstraction_Design.md`（已在前序 session 更新）均反映当前代码库状态。Label_Design.md 已在 2026-06-05 session 中更新，无需重复修改。

### 2026-06-19: Raylib fromsource 纹理不可见根因排查 + 修复

**根因（双重）**：

1. **`BackendBridge.h:bridge_drawTexture` `nullptr` src 被转义为零 SRect**：当调用者（如`Actor::draw`）传递 `nullptr` src（表示"使用完整纹理"）时，`bridge_drawTexture` 创建默认 `SRect()`（全零），然后将**非空指针**传给 `RaylibRenderDevice::drawTexture`。导致 `src=(0,0,0,0)` → `DrawTexturePro` 绘制空输出。
2. **`RaylibRenderDevice::drawTexture` 使用 `rlPushMatrix + rlScalef + DrawTextureEx`**：该写法在 fromsource/DLL 桥接模式下因 OpenGL 矩阵状态跨 DLL 边界损坏而无效，纹理始终不可见；用 `DrawTexturePro` 替代后正常工作。

**诊断过程**：

- 确认非纹理控件（CheckBox、EditBox、Panel背景、Label文字）全部可见 → 问题在纹理路径
- `DrawTexturePro` 在诊断新鲜创建的纹理上工作 → 问题不是 `DrawTexturePro` 本身
- `LoadImageFromTexture` 读回原生纹理 GPU 数据成功且有有效不透明像素 → 数据未损坏
- 在 (300,10) 绘制原生纹理可见 → 纹理和 `DrawTexturePro` 都正常
- 比较原始调用 `src=(0,0,0,0)` 与诊断调用 `nSrc=(0,0,w,h)` → 定位到 `src` 为零

**修复（2 个文件）**：


| 文件                                  | 变更                                                                        |
| ------------------------------------- | --------------------------------------------------------------------------- |
| `src/backend/BackendBridge.h`         | `bridge_drawTexture` 中 `nullptr` src 传入 `nullptr` 而非 `&zeroRect`       |
| `src/backend/raylib/RenderDevice.cpp` | `drawTexture` 从 `rlPushMatrix+rScalef+DrawTextureEx` 改为 `DrawTexturePro` |

**验证**：raylib fromsource ImageButton PNG、LuotiAni 动画、WinFrame 关闭按钮 X 全部可见。SDL3 全部 10 测试无回归。

### 2026-06-19: SFML fromsource 纹理不可见修复（Actor::setParent + sf::Sprite）

**问题**：`test_fromsource_sfml.exe` 中 ImageButton 和 Animation Button 的 PNG 纹理不可见。OpenGL handle 有效、位置正确、`sf::Sprite` 被调用，但画面空白。

**根因（双重）**：

1. **`Actor::setParent` 覆盖纹理**（`src/Actor.cpp:125-140`）：`setParent` 在 `m_texture` 已存在时仍调用 `m_surface->createTexture()` 覆盖 `m_texture`。初始纹理在 `loadFromFile()` 中创建，`Button::setNormalStateActor()` 调用 `setParent()` 时创建第二个纹理覆盖第一个（第一个被销毁，OpenGL handle 回收）。
2. **`sf::VertexArray` + `RenderStates` 纹理绑定问题**：`sf::VertexArray(TriangleStrip)` + `states.texture = nativeTex` 在特定 OpenGL 状态组合下不可见。改 `sf::Sprite(*nativeTex)` 后正确。

**修复（2 个文件）**：


| 文件                                | 变更                                                                   |
| ----------------------------------- | ---------------------------------------------------------------------- |
| `src/Actor.cpp`                     | `setParent` 添加 `&& !m_texture` 避免覆盖已有纹理                      |
| `src/backend/sfml/RenderDevice.cpp` | `drawTexture` 加入 `setActive(true)` + `sf::Sprite` 替代 `VertexArray` |

**验证**：`test_fromsource_sfml.exe` 纹理和动画全部可见。SDL3 全部 10 测试无回归。

### 2026-06-19: SFML 事件响应慢修复（Label::recreate 字体磁盘 I/O 瓶颈）

**问题**：`test_fromsource_sfml.exe` 点击响应延迟大（数秒），SFML 后端事件处理完全不可用。

**根因**：`Label::recreate()` 每帧调用 `releaseFont()` 重置 `m_font`/`m_fontData`，迫使后续 `create()` → `loadFromResource()` 从磁盘读取 ~5-10MB HarmonyOS 字体文件并通过桥接链 `DLL→EXE→DLL` 反复传递，产生 15-44ms 的停顿。

**瓶颈链**：

```
uiSetText(g_prgStatus, "Progress: XX.X%")
  → UICornerstone_SetText (DLL)
    → Label::setCaption()
      → recreate()
        → releaseFont()     ← 释放 m_font / m_fontData
        → create()
          → loadFromResource()
            → provider->readFile()  ← 磁盘 I/O! ~5-10MB
            → loadFontFromMemoryBridge  ← DLL→EXE→DLL 桥接
      → computeLineOffsets()
        → measureText * 3   ← 每次桥接
```

**性能数据**：


| 指标                   | 修复前  | 修复后  |
| ---------------------- | ------- | ------- |
| Update (BENCH->update) | 15-44ms | 0.2-1ms |
| Labels (setCaption)    | 15-32ms | 0.4-6ms |
| 总帧时间               | 33-75ms | 1.5-6ms |
| FPS                    | 13-31   | 170-670 |

**修复（3 个文件）**：


| 文件                                        | 变更                                                                     |
| ------------------------------------------- | ------------------------------------------------------------------------ |
| `src/Label.cpp:recreate()`                  | 移除`releaseFont()`，字体在文本/对齐/边距变化时无需重载                  |
| `src/Label.cpp:setFont()` / `setFontSize()` | 添加`releaseFont()`，只有字体名称或大小变化时才释放字体                  |
| `src/Label.cpp:loadFromResource()`          | 添加`if (m_font) return;` 缓存命中提前返回，避免重复 Provider 读取和桥接 |

**验证**：`test_fromsource_sfml.exe` 运行流畅，FPS 稳定 300-500。SDL3 全部 10 测试无回归。
`setFramerateLimit(120)` 已移除（非正确修复方向）。

### 2026-06-18: Raylib `DrawTexturePro` DLL 桥接不可见修复

**问题**：raylib fromsource 测试（`test_fromsource_raylib`）在桥接模式下所有纹理不可见。`DrawTexturePro` 经桥接从 `UICornerstone.dll` 调用到 exe 中的 `RaylibRenderDevice::drawTexture`，日志输出正确的纹理 ID 和 dst rect，但画面上无任何纹理。

**根因**：未知。`DrawTexturePro` 和 `rlBegin/rlEnd`（`RL_TRIANGLES`）在 fromsource/bridge 模式下始终不可见。`DrawTextureV` 在 `raylib.h` 中定义为 `static inline`（编译到 exe TU），`DrawTexturePro` 来自预编译 `raylib.lib`——函数跨库调用路径可能是关键差异。透明热身（alpha=0）也无效。

**修复**：完全绕过 `DrawTexturePro` 和 `rlBegin/rlEnd`，改用 `rlPushMatrix + rlScalef + DrawTextureEx` 组合：

```
rlPushMatrix();
rlTranslatef(dst.x, dst.y, 0);
rlScalef(dst.w/src.w, dst.h/src.h, 1.0f);
rlTranslatef(-src.x, -src.y, 0);
DrawTextureEx(nativeTex, {0,0}, 0, 1.0f, WHITE);
rlPopMatrix();
```

`DrawTextureEx`（scale=1，原生大小）在桥接路径下正常工作；通过矩阵变换实现定位 + 非均匀拉伸，等价于 `DrawTexturePro` 的行为。

**关键发现**：

- `static inline DrawTextureV` → `DrawTextureEx`（raylib.lib）→ `DrawTexturePro`（raylib.lib）链路上，inline 函数编译到 exe，库函数在 raylib.lib——跨库调用路径可能触发了 raylib 内部渲染批处理状态 bug。
- `DrawTexturePro` 单独调用（含 `rlDrawRenderBatchActive` + `rlSetTexture` + `printf`/`fflush` 等各种变体）皆不可见
- `rlBegin(RL_TRIANGLES)` + `rlTexCoord2f` + `rlVertex2f` 低级 API → 同样不可见
- 透明热身（`DrawTextureV` + alpha=0）无效——必须真正绘制（alpha > 0）才能初始化渲染管线的 OpenGL 纹理绑定状态
- `drawTextureRotated` 补了缺失的 `EndBlendMode()` guard

**文件变更**：


| 文件                                  | 变更                                                                                                                                     |
| ------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `src/backend/raylib/RenderDevice.cpp` | `drawTexture()` 和 `drawTextureRotated()` 改用 `rlPushMatrix + rlScalef + DrawTextureEx`；`drawTextureRotated` 补 `EndBlendMode()` guard |

### 2026-06-16: Fix 4 fromsource 测试 Bug（Actor fallback + Raylib 字体 in-place reload）

**修复 1 — SFML/Raylib WinFrame 关闭按钮 X 不显示**：

- Root cause: `Actor::loadFromFile()` 只调用 `Surface::loadFromFile()` 加载 PNG。在 fromsource（callback bridge）路径下，Surface 工厂函数（`RegisterSFMLSurfaceFactories`/`RegisterRaylibSurfaceFactories`）注册到 DLL，但 Actor 在 DLL 内通过桥接调用 `Surface::loadFromFile()` 时工厂返回 nullptr，导致图片加载失败。
- Fix: `Actor::loadFromFile()` 在 `Surface::loadFromFile()` 返回 nullptr 后，回退调用 `getRenderDevice()->createTextureFromFile(path)`。该虚方法在 backends 和 CallbackBridge 中均已实现，可绕过 Surface 工厂直接由后端加载纹理。
- 同时所有三个后端的 BackendPlugin.cpp 已连接 `createTextureFromFile` 回调到 `bridge_createTextureFromFile`。

**修复 2 — Raylib 中文显示"?"**：

- Root cause: `RaylibTextRenderer::ensureFontCodepoints()` 懒加载扩展 CJK 码点时创建新的 `shared_ptr<RaylibFont>` 替换字体缓存。但 bridge 中的 `UIFontHandle`（`shared_ptr<Font>*` 堆分配句柄）仍指向旧对象，导致后续 `drawText` 通过 bridge 调用时使用只有 ASCII 码点的旧字体。
- Fix: `RaylibFont` 新增 `reload(rlFont)` 方法，原地卸载旧 `rlFont` 并替换为新字体的 `rlFont`，不改变对象身份。`loadOrCreate()` 和 `ensureFontCodepoints()` 中的字体重载路径改为调用 `reload()` 而非创建新 `shared_ptr`。

**修复 3 — SFML 事件响应慢**（部分处理）：

- `Window.cpp` 中已添加 `setVerticalSyncEnabled(false)`（前次 session 完成）
- 当前 session 未发现其他导致响应慢的原因（`pollEvent` 使用非阻塞 API，帧循环无休眠等待）

**修复 4 — Raylib EditBox/TextArea 中文输入**（同 Root cause 为修复 2）：

- 中文输入通过 `TextInput` 事件输入 UTF-8 文本，EditBox 内部调用 `loadFontInternal()` 时使用 `loadFontFromMemory()`（仅 ASCII）。在 `insertText()` 中 EditBox 重新加载字体（未使用 ensureFontCodepoints）。修复 2 已修复字体句柄一致性问题。

**文件变更**：


| 文件                                  | 变更                                                                             |
| ------------------------------------- | -------------------------------------------------------------------------------- |
| `src/Actor.cpp`                       | `loadFromFile()` 添加 `createTextureFromFile` 回退                               |
| `src/backend/raylib/TextRenderer.cpp` | `RaylibFont::reload()` 方法 + `loadOrCreate`/`ensureFontCodepoints` 改为原地重载 |

**已知问题**：

- SFML 和 Raylib 标准测试（静态链接 `UICornerstone.lib`）存在 `Surface::create/loadFromFile/loadFromMemory` 重复定义错误（`Surface.cpp` 和 `RenderDevice.cpp` 均定义）。这是预存在问题（自 Phase R10b 核心/后端 DLL 拆分后），不影响 DLL 模式或 SDL3 静态模式。
- SFML fromsource 在部分环境下事件响应仍可能偏慢（vsync 关闭后未完全解决）。

**验证**：

- SDL3 全部 10 个标准测试编译成功 ✅
- SDL3 fromsource 测试编译成功 ✅
- SFML fromsource 测试编译成功 ✅
- Raylib fromsource 测试编译成功 ✅

### 2026-06-16: WinFrame 关闭按钮 X 不可见修复（Raylib 混合模式 + 向量 X 叠加）

**问题**：test_fromsource_raylib WinFrame 关闭按钮 X 不显示，只看到灰色/红色方块。原因是 PNG 纹理在 raylib 后端的渲染有问题。

**根本原因**：

1. **RaylibTexture::setBlendMode** 只存储混合模式到 `m_blendMode` 成员，但从未实际调用 `BeginBlendMode()` 应用到 OpenGL 状态。SDL3 的 `SDL_SetTextureBlendMode` 直接作用于纹理对象，透明通道正确处理；而 raylib 的纹理的 alpha 从未正确生效。

**修复（2 个文件）**：


| 文件                                      | 变更                                                                                                                                       |
| ----------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------ |
| `src/backend/raylib/RenderDevice.cpp`     | `drawTexture()` 和 `drawTextureRotated()` 在调用 `DrawTexturePro` 前读取纹理的 `getBlendMode()` 并调用 `BeginBlendMode()`                  |
| `src/WinFrame.cpp` + `include/WinFrame.h` | 新增`WinFrame::draw()` override，在 `Panel::draw()` 后使用 `RenderDevice::drawLine()` 绘制向量 X 叠加层，作为跨后端回退方案确保 X 永远可见 |

**具体改动**：

- `drawTexture/drawTextureRotated`: 根据纹理的 blend mode 调用 `BeginBlendMode(BLEND_ALPHA)`/`BeginBlendMode(BLEND_ADDITIVE)`/`BeginBlendMode(BLEND_MULTIPLIED)`。之前 `setBlendMode` 只是存储值，实际绘制时从未应用。
- `WinFrame::draw()`: 调用 `Panel::draw()` 后，获取关闭按钮的 `getDrawRect()` 计算中心位置，用 3 条平行 `drawLine` 绘制两条对角线（共 6 条线），颜色根据 closeButton 的 state 变化：Normal=浅灰(200,200,200)、Hover=白(255,255,255)、Pressed=黄(255,255,100)

**验证**：

- SDL3: 全部 13 个目标（10 标准测试 + DLL + lib + fromsource + test_api）编译通过 ✅
- SFML: `test_fromsource_sfml.exe` 编译通过 ✅（标准测试有预存在的 Surface 重复符号问题）
- Raylib: `test_fromsource_raylib.exe` 编译通过 ✅（标准测试有预存在的 Surface 重复符号问题）

### 2026-06-15: 三后端 fromsource 架构切换（Separate TU 编译）

**问题**：SFML 的 `<SFML/Graphics.hpp>` 引入 `<windows.h>`，在 `#include` 模式（backend .cpp 被 test `.cpp` include）下宏污染导致编译失败。

**架构变更**：

- 所有 `test_fromsource*.cpp` 不再 `#include` backend .cpp 文件，改为 CMake 的 `add_executable` 添加为独立翻译单元：
  ```cmake
  add_executable(${target} ${source_file} ${FROMSOURCE_BACKEND_SOURCES})
  ```
- `FROMSOURCE_BACKEND_SOURCES` 收集 `Window.cpp`/`RenderDevice.cpp`/`TextRenderer.cpp`/`InputBackend.cpp`/`Cursor.cpp`/`BackendPlugin.cpp`
- `FROMSOURCE_BACKEND_LIBS` 收集各后端第三方 lib（SDL3: SDL3.lib+SDL3_ttf.lib+SDL3_image.lib / SFML: sfml-*.lib+opengl32.lib / raylib: raylib.lib+winmm.lib）

**Three fromsource files**:

- `test/test_fromsource_sdl3.cpp` — SDL3 backend (SDL callback mode, 复用 SDL3 窗口)
- `test/test_fromsource_sfml.cpp` — SFML backend (main() + LoadLibrary + GetUIBackendCallbacks)
- `test/test_fromsource_raylib.cpp` — Raylib backend (同上)

**关键修复**：

- SFML 中文 `u8"读取TextArea文本内容"` 导致 MSVC C2059 → 改为英文 `"Read TextArea Content"`
- SDL3 的 `SDLKeycodeToKeyCode`/`SDLKeymodToKeyMod` 返回类型改为 `KeyCode`/`KeyMod`（匹配 InputBackend.h 声明）
- SDL3 需 `#include "UICornerstoneAPI.h"` + `#include "EventTypes.h"` 获取 UIEvent/KeyCode 定义
- Raylib 需 `winmm.lib`（timeBeginPeriod/timeEndPeriod）
- `struct UIBackendCallbacks*` → `UIBackendCallbacks*`（避免与 typedef 冲突）

**Cursor 工厂未注册**（cosmetic 警告）：

- fromsource 路径下 Cursor 工厂未通过 `registerFactories` 注册，Label 创建时输出 `Cursor::createSystem: no backend factory registered`
- 功能不受影响（仅缺光标反馈）

**验证**：SDL3/SFML/Raylib 三后端均编译 + 链接 + 运行成功。test_fromsource_sdl3 / test_fromsource_sfml / test_fromsource_raylib 全部通过。

### 2026-06-15: fromsource 四 bug 修复（Surface 工厂 + newFrame + vsync）

**Bug 1 — SFML WinFrame 关闭按钮 X / Hover / Press 图片不显示**：

- Root cause: `Actor::loadFromFile()` 调用 `Surface::loadFromFile()`（在 UICornerstone.dll 内），走工厂函数指针 `g_loadFileFn`。在 callback init 路径下工厂未注册 → `g_loadFileFn == nullptr` → 返回 nullptr
- Fix: 在 `GetUIBackendCallbacks()` 中调用 `RegisterSFMLSurfaceFactories()` / `RegisterRaylibSurfaceFactories()`，将后端的 Surface 工厂函数通过 `Surface::registerFactories()` 注册到 UICornerstone.dll
- Added `RegisterSFMLSurfaceFactories()` to `src/backend/sfml/RenderDevice.cpp`
- Added `RegisterRaylibSurfaceFactories()` to `src/backend/raylib/RenderDevice.cpp`

**Bug 2 — SFML 事件响应慢**：

- 怀疑是 SFML 默认 vsync 开启导致 `display()` 阻塞到下一次垂直同步
- Fix: 在 `Window.cpp` 创建 `sf::RenderWindow` 后显式调用 `setVerticalSyncEnabled(false)`

**Bug 3 — Raylib 窗体无法响应事件（标题栏"未响应"）**：

- Root cause: `PollInputEvents()` 从未被调用。在 callback init 路径下 `g_inputBackend` 是 `CallbackInputBackend`，其 `newFrame()` 使用基类默认空实现
- Fix: 在 `UIBackendCallbacks` 结构体中新增 `void (*newFrame)(UIInputBackendHandle)` 成员；`CallbackInputBackend::newFrame()` 通过该回调委托；raylib BackendPlugin 设置 `cb.newFrame = bridge_newFrame`（桥接到 `RaylibInputBackend::newFrame()` → `PollInputEvents()`）
- 不影响 SDL3/SFML（`newFrame` 为 NULL 时跳过，基类空实现已满足需求）

**Bug 4 — Raylib 中文显示"?"**：

- raylib TextRenderer 已有 `ensureFontCodepoints()` 懒加载机制：初始只加载 ASCII 码点（0x20-0x7E），绘制/测量含中文文本时检测缺失码点并自动重载字体
- 该机制通过 bridge 路径（`drawText(Font*, string, ...)` → `ensureFontCodepoints`）也应正常工作

**文件变更**：


| 文件                                   | 变更                                                   |
| -------------------------------------- | ------------------------------------------------------ |
| `include/UICornerstoneAPI.h`           | 在`UIBackendCallbacks` 中新增 `newFrame` 回调函数指针  |
| `src/backend/BackendBridge.h`          | 新增`bridge_newFrame()` 桥接函数                       |
| `src/CallbackAdapters.h`               | `CallbackInputBackend` 新增 `newFrame()` 覆盖          |
| `src/CallbackAdapters.cpp`             | 实现`CallbackInputBackend::newFrame()` → 委托到回调表 |
| `src/backend/sfml/RenderDevice.cpp`    | 新增`RegisterSFMLSurfaceFactories()`                   |
| `src/backend/raylib/RenderDevice.cpp`  | 新增`RegisterRaylibSurfaceFactories()`                 |
| `src/backend/sfml/BackendPlugin.cpp`   | 注册表面工厂 +`cb.newFrame = bridge_newFrame`          |
| `src/backend/raylib/BackendPlugin.cpp` | 注册表面工厂 +`cb.newFrame = bridge_newFrame`          |
| `src/backend/sdl3/BackendPlugin.cpp`   | 设置`cb.newFrame = bridge_newFrame`（一致但不必要）    |
| `src/backend/sfml/Window.cpp`          | 创建窗口后调用`setVerticalSyncEnabled(false)`          |

**验证**：SDL3/SFML/Raylib 三后端 fromsource 全部编译通过 + 运行通过。

### 2026-06-15: RGBA8888 像素格式根因排查 + DLL 桥接验证

### 2026-06-15: Phase 16 — RGBA8888 像素格式根因排查 + DLL 桥接验证

**问题**：test_fromsource 中 WinFrame 关闭按钮的黑色 X 不可见（仅深灰色背景），怀疑 DLL 桥接存在问题。

**Root cause**：

- `Surface::create(32, 32)` 创建 RGBA8888 surface。在 little-endian x86 上，字节顺序为 A(LSB), B, G, R(MSB)
- 代码中 `0xFF000000` 实际上是 **R=255, G=0, B=0, A=0** → 透明红色，而非期望的不透明黑色
- 修复后用 `0x000000FF` (R=0,G=0,B=0,A=255) = 不透明黑；`0x505050FF` (R=80,G=80,B=80,A=255) = 不透明深灰

**DLL 桥接验证**：

- 通过 `GetUIBackendCallbacks` 桥接的 `drawTexture` 正确工作（SDL_RenderTexture 经桥接到 SDL3RenderDevice）
- PNG 文件加载和程序化表面两种纹理来源均通过桥接正常工作
- Bench 的 `getRenderDevice()` 通过 `GET_RENDERDEVICE`（`BackendManager::instance()->renderDevice()`）正确获取，无空指针问题
- Hover/Press 状态图片也可见（证明三个状态 Actor 的纹理绘制全部正常）

**关键教训**：

- 初次看到"没有黑 X"是因为黑 X (0,0,0) 在深灰背景 (80,80,80) 上确实容易被忽略，而非 DLL 桥接问题
- 所有后续"sdl3/dll/backend bridge"方向的排查都是误判

**验证**：test_fromsource 运行正常，所有控件可见，点击/悬停交互正常。

### 2026-06-14: test_fromsource — 单文件编译 + 窗口复用 + 控件可见性 + 事件注入

**单文件编译**（CMakeLists.txt）：

- 去掉 `BACKEND_SRC` glob，`test_fromsource.cpp` 通过 `#include` 引入后端 `.cpp` 文件（Window.cpp / RenderDevice.cpp / TextRenderer.cpp / InputBackend.cpp / Cursor.cpp / **BackendPlugin.cpp**）
- 添加 `src/backend/` 到 include 路径供 `BackendBridge.h` 查找

**BackendPlugin.cpp 复用**（取代内联拷贝）：

- `BackendPlugin.cpp` 新增 `#ifdef UICORNERSTONE_REUSE_SDL_WINDOW` 条件编译路径
- 复用路径：`sdl3CreateWindow` 用 `new SDL3Window(g_reuseWindow, g_reuseRenderer)`（不创建新窗口）；`sdl3Init`/`sdl3Destroy` 为空操作
- 默认路径保持原装不动（`CreateSDL3Window` + `SDL_Init`/`SDL_Quit`），不影响现有 10 个测试 + test_api
- `SDL3Backend_SetReuseWindow(g_window, g_renderer)` 在 SDL_AppInit 中 `SDL_CreateWindowAndRenderer` 后调用
- `test_fromsource.cpp` 中的 136 行内联拷贝替换为 `#define UICORNERSTONE_REUSE_SDL_WINDOW` + `#include "../../src/backend/sdl3/BackendPlugin.cpp"`

**窗口复用**：

- `SDL_AppQuit` 调用 `UICornerstone_Shutdown` 清理（`~SDL3Window` 销毁 SDL 句柄）

**控件可见性修复**（UICornerstoneAPI.cpp）：

- 所有 6 个工厂函数新增 `ctl->create()` 调用（Builder/LayoutParser 模式需要显式 create，控制器才真正初始化）
- 所有工厂新增 `ctl->setVisible(true)`
- 原来 Label 的 `m_isCreated` 初始为 `false` → `recreate()` 中的 `if(!m_isCreated) return;` 跳过所有初始化
- 工厂新增 `setFont(FontName::HarmonyOS_Sans_SC_Regular)` 确保 Label 有默认字体
- 新增 `UICornerstone_SetBGColor` API，同时设置 Normal/Hover/Pressed 三态背景色（hover = brighter(0.3), pressed = darker(0.3)）

**事件注入机制**（UICornerstoneAPI.h/.cpp）：

- 新增 `UICornerstone_PushUIEvent(const UIEvent*)` API + 内部 `std::queue<UIEvent>` 队列
- `UICornerstone_ProcessEvents` 改为先处理队列事件，再后备轮询 InputBackend
- 解决 SDL_MAIN_USE_CALLBACKS 模式下 `SDL_PollEvent` 不返回事件的问题

**test_fromsource 事件桥接**：

- `sdlEventToUIEvent()` 函数将 SDL_Event 转为 UIEvent（data buffer 格式）
- `SDL_AppEvent` 中调用 `uiPushUIEvent(&ue)` 注入
- 添加 `UICornerstone_SetOnClick` 回调指针，点击按钮输出 `"Button clicked!"`

**测试验证**：

- SDL3 静态/DLL 全部 11 个目标（10 测试 + test_fromsource）编译通过
- test_fromsource 运行：Button/Label 可见，Hover 变色正常，点击输出 Button clicked!

### 2026-06-13: test_fromsource — DLL 动态加载 + 后端源码编译 (Complete)

**问题**：之前的 `test_fromsource` 使用 `test_api` 模式（`UICornerstone_InitFromPlugin` + `UIBackend_sdl3.dll` + JSON 布局），未满足"只使用 UICornerstone.dll，后端/控件从源码编译，无声明式 UI"的需求。

**设计原则**：

- `UICornerstone.dll` 是唯一的 DLL 依赖（含控件 + C ABI 实现）
- 后端（SDL3 RenderDevice/Window/InputBackend/TextRenderer/Cursor）从源码编译进 exe
- 控件通过 C ABI 工厂函数编程式创建（`UICornerstone_CreateButton/Label/CheckBox/EditBox/ProgressBar`）
- 无 JSON 布局，无 `UIBackend_sdl3.dll` 依赖

**架构图**：

```
test_fromsource.exe
  ├── 动态加载: LoadLibrary("UICornerstone.dll")
  │     → GetProcAddress 解析所有 C ABI 函数指针
  │     → UICornerstone_Init(callbacks) 传入回调查表
  ├── 源码编译 (src/backend/sdl3/*.cpp):
  │     → BackendPlugin.cpp (GetUIBackendCallbacks)
  │     → RenderDevice.cpp, Window.cpp, InputBackend.cpp
  │     → TextRenderer.cpp, Cursor.cpp
  ├── 控件工厂: UICornerstone*.dll C ABI 函数
  └── 帧循环: ProcessEvents → Update → Clear → Render → Present
```

**编译策略**：

- 链接 `UICornerstone_dll`（import lib）供后端源码解析 `Surface::registerFactories` 等 `CORE_API dllimport` 符号
- `UICORNERSTONE_BUILD_SHARED=1` 作用于整个 target，后端源码正确从 DLL 导入符号
- test_fromsource.cpp 尽管 sees dllimport 声明，但只通过 GetProcAddress 函数指针调用，无直接函数引用 → 无 LNK2001
- `test/CMakeLists.txt` + `test_fromsource.cpp`：全新实现，不依赖 `test_api.c` 代码
- 180 帧后自动退出（非手动关闭窗口）

**验证**：

```
=== test_fromsource: UICornerstone.dll + backend from source ===
OK: loaded UICornerstone.dll           # LoadLibrary 成功
SDL3: GetUIBackendCallbacks ready       # 后端源码编译
BackendManager: initialized from callback table  # C ABI Init
Controls: 5/5 created                   # Button/Label/CheckBox/EditBox/ProgressBar
Frame loop...
Done, 180 frames                        # 帧循环正常完成
```

### 2026-06-13: R10b — UICornerstone.dll 拆分：核心 DLL + 后端插件 DLL

**DLL 拆分方案**：

- `UICornerstone.dll`（原名 `UICornerstone_dll.dll`，3.2MB）：只含 CORE_SOURCES（控件、布局、C ABI），不再包含后端实现
- `UIBackend_sdl3.dll`（268KB）：独立的后端插件 DLL，包含 BACKEND_SOURCES（RenderDevice/TextRenderer/Window/InputBackend/Cursor + BackendPlugin），通过 `LoadLibrary` 动态加载
- 静态 `UICornerstone.lib` 保持不变（CORE_SOURCES + BACKEND_SOURCES），10 个现有测试无回归

**关键技术变更**：

- `CMakeLists.txt`：`UICornerstone_dll` target 移除 BACKEND_SOURCES，`OUTPUT_NAME` 设为 `UICornerstone`；新增 `UIBackend_sdl3` SHARED target（连接 UICornerstone_dll 导入库）
- `Surface.h/cpp`、`Cursor.h/cpp`：静态工厂方法（`create`/`loadFromFile`/`loadFromMemory` / `createSystem`/`getDefault`/`setCurrent`）从后端文件迁移到核心，使用 `registerFactories` 委托机制；`CORE_API` 宏控制跨 DLL 导出
- `BackendPlugin.cpp`（三后端）：`GetUIBackendCallbacks` 用 `BACKEND_PLUGIN_EXPORT` 导出；内部调用 `RegisterSDL3SurfaceFactories()` / `RegisterSDL3CursorFactories()` 注册工厂
- `BackendManager.cpp`：`initialize(string)` 路径守卫 `#if !UICORNERSTONE_BUILD_SHARED`，静态链接时也注册工厂
- `UICornerstoneAPI.cpp`：`InitFromPlugin` 移除静态回退，纯 `LoadLibrary` 路径
- `test/CMakeLists.txt`：test_api 额外复制 `UIBackend_sdl3.dll` 到输出目录

**验证**：SDL3 静态/DLL 全部编译通过。test_api 输出显示 `UICornerstone: loaded UIBackend_sdl3.dll`，确认纯动态加载路径。

### 2026-06-13: R9 Bugfix Round — C ABI 事件循环 + 稠密图元修复

**Bug 1 — UI_EVENT_BUTTON 宏偏移错误 (所有点击交互失效)**：

- Root cause: `#define UI_EVENT_BUTTON(ev) (*(int*)(ev)->data)` 从 `data[0]` 读取，但 `bridge_pollEvent` 把鼠标按键值写到 `data[8]`
- 结果：`event.mouseButton.button` 始终是垃圾值，`== MouseButton::Left` 永假 → Button/CheckBox/EditBox 全部无视点击
- 修复：`UI_EVENT_BUTTON(ev)` → `(*(int*)((ev)->data + 8))`

**Bug 2 — 键盘修饰键丢失 (Ctrl-C/V/X/Shift-Arrow 无效)**：

- Root cause: `bridge_pollEvent` 只存 keycode 到 UIEvent，跳过 `key.mod`；`CallbackInputBackend::pollEvent` 硬编码 `KeyMod::None`
- 修复：`bridge_pollEvent` 在 `data[4..5]` 写入 `uint16_t` modifier；新增 `UI_EVENT_KEY_MOD(ev)` 宏；`CallbackInputBackend` 读取并传给 `EventKey`

**Bug 3 — CallbackRenderDevice::drawTriangle/drawQuad 画轮廓而非实心**：

- Root cause: 两个方法都用 `drawLine` 画 3/4 条轮廓线
- 结果：CheckBox 框框和钩钩变成空心线，EditBox/TextArea 选择不可见，所有粗线渲染异常
- 修复链：
  - `UIBackendCallbacks` 末尾新增 `fillTriangle`/`fillQuad` 函数指针
  - `BackendBridge.h` 新增 `bridge_fillTriangle`/`bridge_fillQuad` 委托到原生 `RenderDevice::drawTriangle/drawQuad`（实心）
  - 3 个 `BackendPlugin.cpp` 全部接通新回调
  - `CallbackRenderDevice` 优先调 `fillTriangle`/`fillQuad`，退化为 2 个三角形拼 Quad

**Bug 4 — 剪贴板 stub (Ctrl-C/V 无反应)**：

- Root cause: `CallbackInputBackend::setClipboardText/getClipboardText` 是空实现
- 修复：`UIBackendCallbacks` 新增 `setClipboardText`/`getClipboardText`；`BackendBridge.h` 新增 bridge；3 个 BackendPlugin 接通；Adapter 委托到原生 `InputBackend`

**其他 stub 检查**：`Texture::setBlendMode/setAlphaMod`、`createTextureFromSurface/createRenderTexture`、`setRenderTarget/resetRenderTarget/readPixels` 均为 C ABI 未使用功能，无害暂不改。

**test_api.c 增强**：

- 布局改为 3 行：Button/Label/CheckBox(行0) → EditBox/ProgressBar/Panel(行1) → TextArea(行2)
- 提示标签高度 12→16px；控件间隙 4→8px
- 新增 `hint_textarea`、`ta_demo` ID，共 18 个 ID 全数验证
- 帧循环改为 `while (!UICornerstone_IsQuitRequested())` 等待用户关闭窗口
- 新增 `UICornerstone_Clear`/`UICornerstone_Present` API

**验证**：全部 11 个目标（10 测试 + test_api）SDL3 编译通过。test_api 运行 ALL PASS。

### 2026-06-13: R7 — GetUIBackendCallbacks 三后端实现 (Complete)

**Files**:

- `src/backend/BackendBridge.h` (new, 250 行): 桥接函数头文件，将 `UIBackendCallbacks` 回调查表委托为 C++ 抽象接口（RenderDevice/InputBackend/TextRenderer/Window/ResourceProvider 五个模块全部虚方法已实现）
- `src/backend/sdl3/BackendPlugin.cpp`: 添加 `GetUIBackendCallbacks()` — 用 BackendBridge 初始化 `UIBackendCallbacks` 结构体
- `src/backend/sfml/BackendPlugin.cpp`: 同上
- `src/backend/raylib/BackendPlugin.cpp`: 同上

**`UICornerstone_InitFromPlugin` 双路径**：先 `LoadLibraryA("UIBackend_sdl3.dll")` + `GetProcAddress` 动态加载，失败后回退 `extern "C" GetUIBackendCallbacks()` 静态链接

**桥接策略**：通过 `reinterpret_cast` 将 `void*` 句柄转为 C++ 抽象接口指针，调用对应虚方法（避免每后端单独实现原生调用）

**纹理/字体所有权**：Texture 和 Font 通过堆上分配的 `shared_ptr` 管理，`destroy` 时 `delete` 释放

**验证**：SDK3/SFML/Raylib 三个后端均编译通过，`InitFromPlugin("sdl3")` 静态链接路径验证通过

### 2026-06-13: R8 — CMake DLL 模式 (Complete)

**`UICORNERSTONE_BUILD_DLL` 选项** (默认 OFF):

- `UICornerstone` 目标保持 STATIC（向后兼容）
- `UICornerstone_dll` 独立 SHARED 目标
- `UICORNERSTONE_API` 导出宏 (`__declspec(dllexport/dllimport)`)
- 三后端 DLL 模式编译通过

**"编译两次"策略**：UICornerstone 编译为静态库后，DLL 目标单独编译。以编译时间换 consumer 零感知、CMake 结构清晰、导出控制精确。

**验证**：6 配置全量编译通过（SDK3/SFML/Raylib × 静态/DLL）

### 2026-06-13: R10 — 构建验证（Complete）

**6 配置验证结果**：


| 后端   | 静态 (UICornerstone.lib)      | DLL (UICornerstone_dll.dll)  |
| ------ | ----------------------------- | ---------------------------- |
| SDL3   | 10/10 测试通过                | 3.3MB DLL + 10/10 测试通过   |
| SFML   | 10/10 测试通过                | 3.5MB DLL + 测试通过         |
| Raylib | 10/10 测试通过 (LNK4098 已知) | 4.7MB DLL + test_button 通过 |

### 2026-06-13: R9 — 纯 C ABI 测试 (Complete)

**test_api.c**：`test/test_api.c`，只包含 `UICornerstoneAPI.h`，编译为 C（`.c` 扩展名）。

显示所有 6 种静态控件（Button/Label/CheckBox/EditBox/ProgressBar/Panel）的视觉测试：

1. `InitFromPlugin("sdl3")` 创建原生窗口
2. `SetViewport` 设置 800×480 视口
3. `RegisterAction` + `LoadLayout` 从 JSON 布局加载 3×2 网格
4. `FindControl` 验证 9 个控件 ID 全部存在
5. `SetText` 设置 CheckBox 标题和 EditBox 文本
6. 帧循环：`ProcessEvents` → `Update` → `Render`，用户关闭窗口退出
7. `Shutdown`

**Virtual Inheritance 指针调整 Bug**：

- Root cause: `ControlImpl` 使用 `virtual public Control`（`include/ControlBase.h:226`），虚拟继承导致 `Control*` 子对象地址与派生类对象地址不同
- 工厂函数原先 `reinterpret_cast<UIControlHandle>(buttonPtr)` 存储的是派生类地址
- 通用操作 `static_cast<Control*>(voidPtr)` 不做指针调整（`static_cast` 从 `void*` 到 `T*` 相当于 `reinterpret_cast`）
- 修复：所有工厂函数改为 `reinterpret_cast<UIControlHandle>(static_cast<Control*>(ctl))`，确保存储 `Control*` 地址
- 影响：6 种控件工厂全部修复，`UICornerstone_CreateButton/Label/CheckBox/EditBox/ProgressBar/Panel`

**验证**：SDL3 静态/DLL、SFML 静态、Raylib 静态全部编译通过，test_api 在所有配置上 ALL PASS。

### 2026-06-12: R6 — BackendManager 回调表初始化 (Complete)

**BackendManager 改造**：

- 新增 `BackendManager::initialize(const UIBackendCallbacks* callbacks)` 方法
- 从回调查表创建 CallbackAdapter 实例（Window/RenderDevice/TextRenderer/InputBackend）
- 适配器通过标准访问器对外暴露，与内置后端路径共存
- `BackendPlugin.h` 包含 `UICornerstoneAPI.h` 以使用 `UIBackendCallbacks` 类型

**UICornerstone_Init/Shutdown 重构**：

- `Init` 委托 BackendManager 创建适配器，不再直接管理
**验证**：编译通过，全部 10 个 SDL3 测试无回归。

### 2026-06-30: Slider 控件优化（脏矩形/键盘重复/Value Label 跟随/字体懒加载）

**变更（2 文件）**：

| 文件 | 变更 |
|------|------|
| `include/Slider.h` | 新增 `m_lastRect`（脏矩形追踪）、`m_repeatKey/m_repeatStartTime/m_repeatNextTime`（键盘重复）、`m_tickFontAttempted`（字体加载状态）；新增 `ensureTickFont()`/`repositionValueLabel()`/`handleKeyRepeat()` 私有方法 |
| `src/Slider.cpp` | 见下方优化项 |

**5 项优化**：

1. **setRect 脏矩形检查**：`setRect()` 添加 `if (rect == m_lastRect) return;`，避免重复重定位 value label，减少 Label::setRect 调用链
2. **键盘按键重复**：`handleKeyRepeat()` 在 `update()` 中每帧检测，350ms 初始延迟 + 50ms 重复间隔；`handleEvent()` KeyDown 启动追踪，KeyUp/FocusLost 停止
3. **Value label 跟随 thumb**：`repositionValueLabel()` 将水平滑块标签居中于 thumb 上方（clamp 到滑块宽度），垂直滑块标签居中于 thumb 右侧（clamp 到滑块高度）；`updateValueLabel()` 末尾自动调用
4. **Tick 字体懒加载**：`ensureTickFont()` 单次加载 + `m_tickFontAttempted` 防止重复尝试；`draw()` 中移除了重复的字体加载代码
5. **draw() 去重**：水平/垂直分支中 Tick 字体加载代码抽离为 `ensureTickFont()`，消除 20 行重复

**细节改进**：
- `setShowValueLabel()` 在动态创建 label 后调用 `updateValueLabel()` 确保标题正确
- `m_lastRect` 在构造列表中初始化为空 SRect
- 键盘重复支持 Left/Right/Up/Down/PageUp/PageDown（Home/End 不重复，单次跳转）

**验证**：SDL3/SFML/Raylib 三后端全部编译通过。test_slider 运行正常，15 个滑块初始化 + 帧循环稳定。

### 2026-07-01: test_slider 布局间距 + raylib InputBackend KeyUp 修复

**变更（3 文件）**：

| 文件 | 变更 |
|------|------|
| `test/test_slider.cpp` | 左列 11 水平滑块 Y 重算（40~695），右列 4 垂直滑块 X 均匀分布（580~910），value label / tick label 全部留足间隙 |
| `src/backend/raylib/InputBackend.cpp` | 新增 `fillKeyUpEvent()` 和 Keyboard phase 中 `IsKeyUp()` 检测，按键释放时发送 `KeyUp` 事件 |
| `design/Slider_Design.md` | §8 优化历史 + §10 关键实现注意事项更新 |

**Raylib KeyUp 根因**：raylib 使用 `GetKeyPressed()` / `IsKeyDown()` 但无释放 API，`Keyboard` phase 从未生成 `KeyUp` 事件 → Slider 的 `m_repeatKey` 永不归零 → `handleKeyRepeat()` 无限重复无法停止。

**验证**：SDL3/SFML/Raylib 三后端全部编译通过。test_slider 运行正常。

### 2026-07-07: Focus 环 3 层对比优化 + 全设计文档更新

**焦点环变更**：

| 文件 | 变更 |
|------|------|
| `src/ControlBase.cpp` | `drawFocusRing()` 改为 3 层：黑(inset 0, alpha 150) + 白(inset 1, alpha 150) + 用户颜色(inset 2)，保证任何背景色下至少一条线可见 |
| `include/ControlBase.h` | `m_focusRingAlwaysVisible = true`（默认） |
| `src/backend/sfml/InputBackend.cpp` | 添加 `unicode < 0x20 \|\| unicode == 0x7F` 控制字符过滤，防止 Tab 注入为文本输入 |
| `test/test_button.cpp` | 图片路径相对化（`assets/images/*.png` 替代硬编码绝对路径） |
| `test/test_winframe.cpp` | 添加跨 WinFrame 焦点测试控件 |
| 全部设计文档 | 同步更新焦点描述（3 层环、setFocusable(true)、m_focusRingAlwaysVisible、作用域边界等） |

**验证**：6 构建配置 + 全部 samples 编译通过。test_fromsource_xxx 三后端焦点环可见。

### 2026-07-11: ColorPicker 控件完整实现 + C ABI + JSON + 三后端验证

**ColorPicker 实现**（~2100 行源码）：

- 核心控件：`ColorPicker.h`/`ColorPicker.cpp` — 继承 Panel，闭合状态（色块 + 十六进制文本）+ 弹窗状态（预设色面板/Hex 输入/RGB+A 滑块/确定取消）
- 内部类 `PresetCell`：继承 ControlImpl，带选中/非选中状态色块绘制
- `ColorPickerBuilder`：Builder 模式工厂，支持全部视觉属性设置

**交互逻辑**：
- 点击色块切换弹窗；预设色点击选中不关闭；Hex 输入/Slider 三向同步
- 确定（`onOK`）提交颜色触发回调；取消（`onCancel`）恢复初始颜色
- Enter/ESC 键通过 `BeforeEventHandlingWatcher` 拦截，ESC=取消，Enter=确定
- `m_ignoreKeyEvent` 标志防止关闭弹窗后同一 Enter 事件重新打开
- 弹窗为 `FocusBoundary`，Tab 在弹窗内部循环，不越过边界
- 外部点击通过 watcher 检测关闭弹窗

**视觉属性**：
- `m_swatchSize` — 闭合状态色块大小（`setClosedSwatchSize(float)`)
- `m_closedFontSize` — Hex 文本字号（`setClosedFontSize(int)`）
- `m_closedTextColor` — Hex 文本颜色（`setClosedTextColor(SColor)`）
- `m_popupBGColor` — 弹窗背景色（`setPopupBGColor(SColor)`）

**C ABI 新增**（UICornerstoneAPI.h/.cpp）：
- `UICornerstone_CreateColorPicker` — 工厂函数
- `UICornerstone_GetColorPickerColor` — 获取当前颜色 Hex
- `UICornerstone_SetOnColorChanged` — 设置颜色变化回调
- `UICornerstone_SetClosedSwatchSize/SetClosedFontSize/SetClosedTextColor/SetPopupBGColor` — 视觉属性设置

**LayoutParser JSON 新增**：
- `swatchSize` — 闭合色块大小
- `closedFontSize` — Hex 文本字号
- `closedTextColor` — Hex 文本颜色
- `popupBGColor` — 弹窗背景色
- 已有：`color`、`presets`、`presetLayout`、`events.onColorChanged`

**测试**：
- `test/test_colorpicker.cpp` — 5 个 ColorPicker（含 2x 缩放）+ 状态标签
- `layouts/test_layout.json` — 新增 `testColorPicker` JSON 示例
- `test_fromsource_sdl3/sfml/raylib` — 三后端 C ABI ColorPicker 验证

**设计文档更新**：
- `design/ColorPicker_Design.md` — 完整设计文档（13 节）
- `design/FocusSystem_Design.md` — ColorPicker 焦点边界
- `design/LayoutSystem_Design.md` — ColorPicker JSON 格式
- `README.md` — 14+ 控件、test_colorpicker/test_slider 列表
- `design/Build_Guide.md` — test_colorpicker/test_slider 列表
- `design/UICornerstone_DLL_Design.md` — 完整 C ABI API 清单（含 ColorPicker/Slider/WinFrame/TextArea/ImageButton 等）

**三后端构建验证**：SDL3/SFML/Raylib 全部编译通过，test_fromsource 三后端输出 "Color: #FF6600"。

### 2026-07-11: Dialog 2 Bug Fixes — Content Render Device + Destructor Crash

**Bug 1 — Popup 内容不可见**：

- Root cause: `setContent()` 在 `create()` 之前调用，此时 Popup 尚未从父级继承 `RenderDevice`。内容控件获取到 null render device → 不可见。
- Fix: `Popup::create()` 末尾添加对 `m_content` 的 render device/text renderer/resource provider/input backend 传播。

**Bug 2 — 关闭窗口时崩溃**：

- Root cause: `Popup::~Popup()` 调用 `getThis()`（即 `shared_from_this()`），在静态析构阶段 EventQueue 已被销毁后调用危险。
- Fix: 将析构函数清空；不在 `close()` 中移除 watcher（避免 ESC/outside-click 路径的 mutex 递归 lock → UB），不可见时 watcher 检查 `getVisible()` 安全返回 false；EventQueue 静态析构时自动清理 shared_ptr。

**验证**：全部 18 个 SDL3 目标编译通过，0 error 0 warning。test_dialog 运行 15 秒无崩溃。

### 2026-07-11: ColorPicker 闭合状态透明背景修复

**Bug**：LayoutParser 和 C ABI 路径下 ColorPicker 闭合状态背景为黑色（`DEFAULT_NORMAL_COLOR`），与父容器背景不一致。

**根因**：`setTransparent(true)` 和 `setBorderVisible(false)` 仅放在 `ColorPicker::create()` 中，但三个代码路径的时序不同：
- Builder 路径：`create()` → `addControl()`（无问题）
- LayoutParser 路径：构造 → 属性解析 → `create()` → `parseChildren` 中 `addControl`（`create()` 虽已调用但构造函数阶段的 `inheritRenderer()` 可能受其他因素影响）
- C ABI 路径：构造 → `addControl()` → `create()`（`addControl` 在 `create` 之前）

**Fix**：将 `setTransparent(true)` 和 `setBorderVisible(false)` 移至 `ColorPicker` 构造函数（`src/ColorPicker.cpp:70-71`），与 Label 做法一致（`src/Label.cpp:30`）。`create()` 中保留相同调用作为防御性冗余。

**验证**：test_layout / test_fromsource 三后端全部编译通过，闭合状态背景透明。

### 2026-07-12: test_dialog_cabi — Button色块 + RGB滑块 + 预设色 via JSON Dialog

**需求**：用户希望 Dialog 用纯 JSON 定义，含 Button 色块 + R/G/B 滑块 + 预设色按钮，而非 C++ ColorPicker 组件。

**变更**（3 个文件）：

- `src/LayoutParser.cpp` `parseEvents()`: 新增 Slider `onValueChanged` JSON 事件绑定
- `src/LayoutParser.cpp` `parseCommonProperties()`: 新增 `borderVisible` 属性支持
- `test/test_dialog_cabi.cpp`: 完全重写
  - 主界面：Button 色块(可点击) + Label 显示 Hex
  - JSON Dialog (`"dialogs"` 数组)：预览色块，R/G/B 滑块(`labelFormat: "R: %.0f"`)，5 个预设色按钮
  - `onColorChange` 滑块事件：`FindControl` + `GetSliderValue` 读取 RGB → `SetBGColor` 更新预览
  - `onPreset0-4` 预设色点击：`SetSliderValue` 设置 RGB → 更新预览
  - `onColorConfirmed` 确定回调：更新主界面色块 + Hex 标签
  - 使用 `UICornerstone_GetSliderValue/SetSliderValue/SetBGColor` C ABI

**验证**：SDL3 DLL 模式编译 0 错误，运行无崩溃。标准 test_dialog 无回归。

### 2026-07-12: test_dialog_cabi 三后端 + 设计文档刷新

**Bug 修复 — OK 按钮不提交颜色**：

- 根因：`test_dialog_cabi_shared.h:204` `//` 注释覆盖了整行，`g_savedR/G/B/A` 赋值语句被注释掉从未执行
- OK 流程：`onColorConfirmed` → 赋值被注释 → `close()` → `onColorClose` → `restoreFromSaved()` → 读取旧 `g_savedR/G/B/A` → swatch 恢复为原始颜色
- 修复：赋值移到注释后新行

**Raylib windows.h 冲突修复**：

- 根因：`CloseWindow()`/`DrawTextExA()` 等 raylib 函数名与 `<windows.h>` Win32 API 函数名冲突（均为 `extern "C"` 但签名不同）
- 修复：`test_dialog_cabi_shared.h` 用 `#ifndef _WINDOWS_` 条件守卫，未包含 windows.h 时手动 `extern "C" __declspec(dllimport)` 声明 `LoadLibraryA`/`GetProcAddress`/`FreeLibrary` + `using HMODULE = void*`

**UTF-8 BOM + 中文乱码修复**：

- 4 个测试文件（`test_dialog_cabi_shared.h` + 3 个 `.cpp`）全部转换为 UTF-8 with BOM
- 所有因 GBK→UTF-8 双编码而乱码的中文注释已恢复为正确文本

**设计文档刷新**（6 文件）：

| 文档 | 主要变更 |
|------|----------|
| `design/EventSystem_Design.md` | **新建** — 事件系统完整设计文档（EventType→Event→InputBackend→EventQueue→控件分派→FocusManager→StateMachine） |
| `design/Build_Guide.md` | 测试表 + fromsource 表添加 `test_dialog_cabi` 条目 |
| `design/BackendAbstraction_Design.md` | 进度表新增 Phase 16h；§13 新增 Cursor 回调表工厂子节 |
| `design/UICornerstone_DLL_Design.md` | `UIBackendCallbacks` 新增 3 个光标工厂回调；新增 Dialog C ABI API（11 个函数）；版本历史 1.13 |
| `design/Dialog_Design.md` | 测试计划新增第 13 项（test_dialog_cabi）；新增 §14 跨后端注意事项（windows.h 冲突 + Cursor 工厂注册） |

**验证**：SDL3/SFML/Raylib 三后端 DLL 模式 test_dialog_cabi 全部编译通过，0 error，0 C4819。

### 2026-07-14: ComboBox 滚轮滚动 + Focus Tab 环重复 + 2x Popup 大小修复

**Bug 1 — 鼠标点击 ComboBox 箭头造成 Tab 环重复**：

- Root cause: `ControlImpl::setFocused()` 没有通知 `FocusManager`，导致 `FocusManager::m_currentFocused` 与控件的焦点状态不同步
- Fix: 在 `setFocused()` 末尾调用 `FocusManager::notifyControlFocused(this, byKeyboard)` 同步状态
- 注意：`FocusManager::notifyControlFocused` **不调用** `setFocused`（避免递归），只更新 `m_currentFocused`

**Bug 2 — ComboBox 弹出 Popup 中鼠标滚轮滚动不工作**（根因链复杂；需修复 3 个子问题才完全解决）：

1. **符号错误**：`ComboBoxListPanel::handleEvent` 中 `newOffset = m_scrollOffset - delta` 导致 `scrollY < 0`（向下滚动）时 `newOffset = 0 - 1 = -1` → 钳位到 0
   - Fix: `m_scrollOffset + delta`
2. **回调级联重置**：`updateScrollBar()` 调用 `setRange(0,22)` → 触发 `notifyPositionChanged` → `syncListFromScroll` → `setScrollOffset(0)`（ScrollBar 值尚未更新）；`setPageSize(8)` 同理。之后 `setValue(getScrollOffset())` 拿到被重置的 0。
   - Fix: 在 `setRange/setPageSize` 之前**保存** `int intendedOffset = m_listPanel->getScrollOffset()`，`setValue(intendedOffset)` 使用保存值
3. **ScrollBar 区域不可用**：鼠标在 ScrollBar 上时，ListPanel 的 `dr.contains(mx, my)` 返回 false（点不在 ListPanel 的 draw rect 内）。同时也应限制鼠标在 popup 区域外时不滚动。
   - Fix: `Popup::handleEvent` 拦截 `MouseWheel` 后用 `isContainsPoint` 检查鼠标是否在 Popup 区域内，仅在区域内才转发；不在区域内时直接 `return false` 阻止落到 `Panel::handleEvent`（否则子控件被重新遍历时不带区域检查）
4. **ComboBox 焦点时无位置限制**：`ComboBox::handleEvent` 中 `!isPopupOpen() && getFocused()` 的 MouseWheel 分支缺少 `isContainsPoint` 检查
   - Fix: 添加 `isContainsPoint(event->mouseWheel.x, event->mouseWheel.y)` 检查

**重现过程**：通过 EventQueue 注入含正确屏幕坐标的 MouseWheel 事件，验证 `Popup::handleEvent` → `ComboBoxListPanel::handleEvent` → `setScrollOffset` 完整路由。

**Bug 3 — 2x 缩放下 Popup 尺寸翻倍**：

- `computePopupRect()` 返回缩放后的 pw/ph，`setRect` 再用 getDrawRect 时会再次缩放 → 2×
- Fix: `computePopupRect()` 返回 `pw / sx, bestPh / sy`（未缩放值）
- ListPanel/ScrollBar 子控件使用 base `1.0f` 缩放避免复合缩放

**ComboBox API 扩展**：

- `include/ComboBox.h`: 新增 `getListPanel()`（公开访问 `m_listPanel`），`openPopupForTest()`（公开调用 `openPopup()`）
- `src/ComboBox.cpp`: `updateScrollBar()` 保存预期 offset 避免回调级联

**验证**：SDL3 18 个目标全部编译通过。SFML/Raylib 静态构建通过。

### 2026-07-21: Splitter 拖拽崩溃修复 + Raylib DLL 退出慢修复 + 编码规范整理

**Bug — Splitter 拖拽释放后运行库异常**：

- **Root cause**: `endDrag()` 从 `beforeEventHandlingWatcher` 回调内部调用 `removeBeforeEventHandlingWatcher`，此时 `EventQueue` 已持有 `m_mtxForBeforeEventHandlingWatcher` 非递归 mutex，递归 lock → 死锁 + 迭代器失效。
- **Fix**: `endDrag()` 不再移除 watcher（同 Dialog 既有模式），不拖拽时 watcher 检查 `!m_dragging` 返回 false 放行事件。`EventQueue` watcher map 改用 `weak_ptr<Control>` 存储，不延长控件生命周期，消除静态析构顺序问题。

**Bug — Raylib DLL 退出慢**：

- **Root cause**: `UICornerstone_Shutdown()` 先销毁后端（`CloseWindow` 等），`FreeLibrary` 后触发 `Bench` 静态析构时控件析构访问已释放的后端资源。
- **Fix**: `UICornerstone_Shutdown()` 在 `BackendManager::shutdown()` 前调用 `BENCH->removeAllControls()` 主动销毁控制树。

**Files changed**:

| File | Change |
|------|--------|
| `include/EventQueue.h` | watcher map: `shared_ptr<Control>` → `weak_ptr<Control>` |
| `src/EventQueue.cpp` | 6 个 watcher 函数全部更新为 `std::find_if` + `lock()` |
| `include/Splitter.h` | 删除未使用字段 `m_dragEventTypes[2]`；加 `#include "ConstDef.h"`；修复 UTF-8 BOM |
| `src/Splitter.cpp` | `endDrag()` 移除 `removeBeforeEventHandlingWatcher` 调用；魔鬼数字改用 `ConstDef::SPLITTER_*`；`setSplitRatio` 加早期返回避免相同比例重复触发回调 |
| `src/UICornerstoneAPI.cpp` | `UICornerstone_Shutdown()` 添加 `BENCH->removeAllControls()` |
| `include/ConstDef.h` | 新增 `SPLITTER_KEY_STEP`、`SPLITTER_KEY_FINE_STEP` |
| `src/ConstDef.cpp` | 新增上述常量定义 |

**Documentation updated**:
- `design/Splitter_Design.md`: 更新 §3.1/4.2/4.3.3/4.4/6/12.3 与当前实现一致
- `design/EventSystem_Design.md`: 更新 §3.4 `eventLoopEntry` 含 consumed 检查；§3.6 补充 `weak_ptr` 实现细节
- `design/UICornerstone_DLL_Design.md`: 版本历史 v1.16
- `design/guidelines/history.md`: 本次记录

### 2026-07-26: Phase 4 — Getter/Setter/Callback C ABI 实现完成

**Changes**:
- `src/UICornerstoneAPI.cpp`: 新增 `SetBool`、`SetEnum`、`SetCallback`、`GetColor`、`GetStateColor`、`GetBool`、`GetInt`、`GetFloat`、`GetString`、`GetEnum` C ABI 函数
- `src/ControlBase.cpp`: 新增 `setBoolProperty`、`setEnumProperty`、`setCallbackProperty` 及 7 个 `get*Property` 默认实现，覆盖颜色/状态颜色/可见性/启用等通用属性
- `include/ControlBase.h`: `ControlImpl` 类添加所有缺失的 Property override 声明
- 修复: `SColor` 访问方法 (`redByte()` 代替 `getRed()`)、`StateColor` 构造函数最令人费解的解析、`UIEventCallback` 类型转换
- 3 后端 (SDL3/SFML/Raylib) 全部编译通过，0 错误 0 警告

### 2026-07-26: Phase 2/5 — 8 控件属性系统重载完成

**Changes**:
- `include/PropertyNames.h`: 新增 18 个控件特有颜色常量（track/track-fill/thumb/…/arrow/popup-bg 等）
- `include/ConstDef.h` / `src/ConstDef.cpp`: 新增 `FontNameFromString`（28 个字体名 kebab-case 映射）
- 8 个控件新增 `setColorProperty` 重载，全部使用 `PropertyNames::k*` 常量代替 raw string：
  - `Slider` (track/track-fill/thumb/thumb-border/thumb-hover/tick/label)
  - `ComboBox` (arrow/arrow-hover/item-selected/item-hover/item-disabled/list-bg/list-border)
  - `CheckBox` (check/cross/indeterminate/box-border)
  - `ProgressBar` (progress/background/text)
  - `Splitter` (line/line-hover/line-drag)
  - `WinFrame` (win-frame-bg/win-frame-border/title-bar-bg/title-text)
  - `ColorPicker` (closed-text/popup-bg)
  - `NumericUpDown` (arrow/arrow-hover/arrow-pressed)
- 各控件同时实现 `setBoolProperty` / `setIntProperty` / `setFloatProperty` / `setStringProperty` / `setEnumProperty` 重载（按 §6.4~§6.8 属性表）
- 新增 FromString 函数（与枚举定义同文件，方案 B）：
  - `SliderStyleFromString`, `CheckBoxStyleFromString`, `CheckStateFromString`
  - `ProgressBarStyleFromString`, `ProgressBarTextModeFromString`
- `design/CABI_Property_Design.md`: 新增 §5.10 枚举值字符串管理章节，更新 TOC
- 3 后端编译通过，0 错误 0 警告

### 2026-07-30: C ABI 测试问题修复

**问题发现与修复**：

- `Splitter::getFloatProperty` 缺失 `"ratio"` 分支：`setFloatProperty` 有但 getter 无，导致 C ABI 读 ratio 始终 0。已在 `getFloatProperty` 补上 `PropertyNames::kRatio`。
- `CheckBox` 的 `setBoolProperty` / `getBoolProperty` 均不处理 `"checked"`：`uiSetBool(handle, "checked", 1)` 和 `uiGetBool(handle, "checked", &st)` 静默失败。已补上读写 `m_checkState`。
- 测试代码中 Label 使用 `"text"` 而非 `"caption"`：`Label::setStringProperty` 只识别 `"caption"`，`uiSetString(lbl, "text", ...)` 失败。已在 `test_splitter_cabi`、`test_treeview_cabi`、`test_fromsource_cabi` 中修正为 `"caption"`。
- WinFrame 标题使用 `"caption"` 而非 `"title"`：WinFrame 的 `setStringProperty` 只处理 `"title"`（`PropertyNames::kTitle`）。已在 `test_fromsource_cabi` 中修正。
- TreeView `"selected-user-data"` 通过 `GetPtr` 而非 `GetString`：JSON 加载将 `userData` 存为 `new std::string` → `void*`，需用 `GetPtr` 获取指针再转回 `std::string*`。已在 `test_treeview_cabi` 中修正。
- Label 遮挡关闭按钮：`test_fromsource_cabi` 中 WinFrame 内 Label 位于 (10, 10, 480, 260) 覆盖了关闭按钮区域，`ControlImpl::handleEvent` 的遮挡检查导致关闭按钮收不到点击。已下移至 (10, 35) 避开标题栏。
- 视口裁剪导致底部控件不可见：`UICornerstone_Render` 推 `g_viewport` 裁剪矩形后，位于 y=470、高度 30~32 的 Slider/NUD/ColorPicker 超出 480 视口被裁剪。已增大 `test_fromsource_cabi` 视口至 550。

**涉及文件**：`src/Splitter.cpp`、`src/CheckBox.cpp`、`test/test_fromsource_cabi.cpp`、`test/test_splitter_cabi.cpp`、`test/test_treeview_cabi.cpp`

**文档更新**：`design/CABI_Property_Design.md`（Bool 属性表补 CheckBox `"checked"`）、`design/guidelines/testing.md`（新增 C ABI 测试常见陷阱章节）

**修复示例程序**：
- `test_numericupdown_cabi`：回调中读取 `evt->data.floatVal` 而非 `doubleVal`（NumericUpDown 用 `CCallbackData::Float` 发送，`doubleVal` 始终为 0）
- `hello_uicornerstone.c`：状态标签更新用 `"caption"` 代替 `kTextContent`（`"text"`），Label 只认 `"caption"`
- `sample_fromsource.c` / `sample_programmatic.c`：回调签名补 `const UIEventData* evt` 参数以匹配 `UIEventCallback`；状态标签用 `"caption"` 代替 `kTextContent`
- `sample_loadlibrary.cpp`：`UICornerstone_SetOnClick`/`SetText`/`SetBGColor` 早已被移除（Phase 6），替换为 `SetCallback("click",…)`/`SetString("caption",…)`/`SetColor("background",…)`；回调签名匹配新的函数指针类型

**涉及文件**：`samples/hello_uicornerstone.c`、`samples/sample_fromsource/sample_fromsource.c`、`samples/sample_programmatic/sample_programmatic.c`、`samples/sample_loadlibrary/sample_loadlibrary.cpp`、`test/test_numericupdown_cabi.cpp`

**关键修正 —— `sample_loadlibrary` 结构体参数 ABI 不匹配**：
- `UISetColorFn` 错误地将 `UIColor`（4 字节结构体）拆为 4 个 `uint8_t` 参数。MSVC x64 调用约定中，4 字节结构体通过单寄存器传递，而 4 个独立 `uint8_t` 占用 4 个不同的参数位置（寄存器+栈），导致 `UICornerstone_SetColor` 读取到乱值。修复为 `typedef int (*UISetColorFn)(void*, const char*, UIColor)` 并在调用处构造 `UIColor` 临时变量。

### 2026-07-31: 焦点切换 z-order 修复 + ScrollBar/TreeView/HandleControl/Menu C ABI 补齐

**Bug — WinFrame 键盘焦点切换不提升顶层**：

- `Control` 新增 `virtual void onFocusScopeActivated()`（`include/ControlBase.h`，`ControlImpl` 默认空实现）
- `WinFrame` 覆盖为 `bringToFront()`
- `FocusManager::focusFirstInScope()`、`focusNext()`、`focusPrev()` 三处切换成功后均调用 `findFocusScope(c)->onFocusScopeActivated()`
- 覆盖两个场景：CTRL+Tab 直接切到 WinFrame；Tab 首次聚焦非顶层 WinFrame

**C ABI 补齐 —— ScrollBar / TreeView / HandleControl / Menu**：

- `UICornerstone_CreateScrollBar(x, y, w, h, orientation)`：orientation 0=垂直，非 0=水平（`ScrollBarOrientation`），属性走统一系统
- `UICornerstone_CreateTreeView(x, y, w, h)`：此前 TreeView 只能通过 layout JSON 创建
- `UICornerstone_CreateHandleControl(target, x, y, w, h)`（target 必传非 NULL）+ `HandleControl::setTarget(Control*)` 裸指针重载
- HandleControl 裸指针存活标记：C ABI 路径无法从裸指针构造 weak_ptr，新增 no-op deleter 的 `m_targetShared` 成员；`handleEvent`/`draw` 存活检查改为 `m_targetWeak.expired() && !m_targetShared`；`detach()`/析构 reset
- Menu 三件套（7 个导出）：`UICornerstone_CreateMenuBar(x,y,w,h)`、`UICornerstone_CreateMenuPanel()`、`UICornerstone_CreateMenuItem(caption, type)`（type 0=Normal/1=Separator/2=SubMenu，越界返回 nullptr）、`UICornerstone_MenuBarAddMenu(bar, caption, panel)`、`UICornerstone_MenuPanelAddItem(panel, item)`、`UICornerstone_MenuPanelAddSeparator(panel)`、`UICornerstone_MenuItemSetSubMenu(item, panel)`
- Menu 生命周期：裸指针无法恢复 shared_ptr，参照 `g_popupPool` 模式新增 `g_menuPool` 保活池（`menuPoolKeep`/`menuPoolTake`）；创建时进池，挂载进 MenuBar 链时转出所有权
- `UICornerstone_CreateMenu` 空实现（printf "not implemented"）已删除
- MenuBar/MenuPanel/MenuItem 继承自虚拟基类，向下转换必须用 `dynamic_cast`/`dynamic_pointer_cast`（MSVC C2635）
- `src/Menu.cpp:919` 残留孤立字符"对"导致语法错误（增量构建长期未暴露），已删除

**sample_loadlibrary 三后端化**：

- 原实现硬编码 `#include "../../src/backend/sdl3/*.cpp"` 导致 Raylib/SFML 构建失败
- `CMakeLists.txt` 改为按 `${_BACKEND_LOWER}` 将后端 6 个源文件编译为独立 TU（同 `sample_fromsource` 的 `_BACKEND_SOURCES` 模式）
- 源码改用 `extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);` 声明；补 `Surface.h`/`Cursor.h`/`ResourceProvider.h` include

**验证**：SDL3/SFML/Raylib 三后端 `UICornerstone.lib` + `UICornerstone_dll.dll` 全部编译通过，0 错误；三后端 `sample_loadlibrary.exe` 均运行进入帧循环；新导出符号经 pefile 验证。提交 `7c63af0` 覆盖 4 示例修复（44 文件，含 `design/CABI_MultiInstance_Design.md`/`design/CppBinding_Design.md` 新文档）。

### 2026-07-31: Menu 重构 — 去除内嵌 Label，直接绘制文本

**背景**：Menu 原实现每菜单项内嵌 3 个 Label（caption/shortcut/arrow）+ MenuBar 每项 1 个 Label，存在控件树膨胀、位置手工同步（`updateLabelPositions`/`layoutEntries` 反复 setRect）、字体大小"须在创建前设置"、箭头依赖 Nerd Font 字形（`▶`）、hover 双重维护/双重绘制、字体状态全局变量（多实例冲突）、MenuBar 非 (0,0) 时面板坐标双重偏移等问题。

**重构内容**：

- **文本直接绘制**（同 TreeView 模式）：MenuBar/MenuPanel 各自懒加载共享 `SharedFont`（`ensureFont()`，`loadFontFromMemoryWithText`，字号 = `fontSize × getScaleXX()`）；MenuItem 经 `setMenuFont` 从所属面板注入同一字体；`draw()` 用 `TextRenderer::drawText` 绘制标题/快捷键
- **箭头图形化**：子菜单箭头改用 `RenderDevice::drawTriangle`（照 TreeView::drawArrow），与勾选标记（drawLine）风格统一，不依赖字体字形
- **全局状态 → 实例成员**：删除 `MenuColors::g_menuTextSize`/`g_heightRatio`/`getItemHeight()`/`getBarHeight()`；MenuBar 持有 `m_menuTextSize`/`m_itemHeightRatio`/`m_fontName`，MenuPanel 持有 `m_fontSize`/`m_heightRatio`/`m_fontName`；`setFontSize`/`setItemHeightRatio`/`setFontName` 修改后即时重载字体并重算面板尺寸，同步所有子面板，"须在创建前设置"限制消除
- **hover 统一管理**：删除 `MenuItem::m_hovered` 与 MenuItem::draw 中 hover 背景（死代码 + 双重绘制），背景统一由 MenuPanel 按 `m_hoveredIndex` 绘制
- **坐标修正**：`MenuBar::openMenu` 原用 `getDrawRect()`（屏幕坐标）直接 `setPosition`，而面板是 MenuBar 子控件，MenuBar 不在 (0,0) 时双重偏移；改为扣除父链偏移后传相对坐标
- `MenuItem::getItemHeight()` 静态方法移除（项高度由面板 `m_fontSize × m_heightRatio` 统一计算）；`setEnumProperty("font", ...)` 从"不支持返回 0"变为完整支持
- `src/Menu.cpp` 由 1044 行精简至约 750 行

**验证**：SDL3/SFML/Raylib 三后端静态库 + `UICornerstone_dll.dll` 全部编译通过，0 错误；`test_menu.exe` 三后端运行正常（事件循环 6 秒无崩溃）。C ABI 导出（`CreateMenuBar/CreateMenuPanel/CreateMenuItem/MenuBarAddMenu/MenuPanelAddItem/MenuPanelAddSeparator/MenuItemSetSubMenu`）签名不变，`src/UICornerstoneAPI.cpp` 无需修改。

**文档**：`design/Menu_Design.md`（§2 类图同步实例成员与 setMenuFont、§3.2 生命周期重写、§4.1 字体描述、§4.2 运行时调整即时生效、§6.4 API 变更表、§7.4 注意事项）、`design/guidelines/history.md` 本次记录。

### 2026-07-31: WinFrame 焦点置顶统一修复（notifyControlFocused）

**问题**：焦点进入 WinFrame scope 时置顶的钩子只覆盖 `focusNext`/`focusPrev`/`focusFirstInScope`（对 scope 自身）三条路径；`focusControl`（鼠标/代码聚焦）、`focusFirstInScope` 聚焦 scope **后代控件**等路径缺失 → 例如 Ctrl+Tab 切到 Bench scope 时 `focusFirstInScope(Bench)` 把焦点给到 WinFrame 内控件，但该 WinFrame 不置顶（需再按一次 Tab 或 Ctrl+Tab 往返才恢复）。

**修复**（`src/FocusManager.cpp`，提交 `d80439e`）：

- `notifyControlFocused` 聚焦分支新增：`if (Control* s = findFocusScope(ctl)) s->onFocusScopeActivated();` — 任何路径下焦点进入某 scope（WinFrame）即激活（提升顶层）
- `focusNext`/`focusPrev`/`focusFirstInScope` 原有调用保留（幂等双保险）
- 主界面（Bench scope）控件聚焦时 `findFocusScope` 返回 Bench（`onFocusScopeActivated` 空实现），无误触

**实证**（sdl3 test_winframe 临时诊断代码，验证后已移除）：show 两 WinFrame 后完整序列 —— 首次 Tab → WinFrame1 置顶 ✓；Tab → 同 scope 内循环 ✓；Ctrl+Tab → WinFrame2 置顶 ✓；Ctrl+Tab 回 Bench scope → 焦点落 WinFrame1 控件 → **WinFrame1 立即置顶** ✓（修复前失败）；`focusControl(g_btn1)` 主界面按钮 → 不误触置顶 ✓。

**调查发现**（未改动，供后续参考）：

- `WinFrameBuilder::build()` 末尾 `hide()`（WinFrame.cpp:551）— WinFrame 默认隐藏，须 `show()` 后可见；`focusNextScope`/`focusPrevScope` 的 `if (!boundary->getVisible()) continue;` 会跳过隐藏的 WinFrame
- `hide()` 不递归隐藏子控件（内部 Button/EditBox 仍 `getVisible()==true`，Tab 可聚焦到隐藏 WinFrame 内控件）
- Tab 首个焦点是标题栏关闭按钮 `m_closeButton`（无 caption、parent 直接是 WinFrame，`setFocusable` 未禁，注册在 `m_controls` 首位）
- 首次 Tab（`m_currentFocused==nullptr`）时 `focusNext(nullptr)` scope 为空，遍历全部 `m_controls`，聚焦第一个可见启用控件

### 2026-07-31: CABI_MultiInstance_Design.md 全面修订（对照真实源码核验）

**背景**：多实例改造设计草案（design/CABI_MultiInstance_Design.md）存在大量虚构 API 与事实性错误。对照 include/UICornerstoneAPI.h（63 个导出）、src/UICornerstoneAPI.cpp、ControlBase.h/.cpp、Bench.h、RenderDevice.h 等源码逐节核验并全面修订。

**修正虚构 API（§5.2 重写）**：

- 删除不存在的 CreateControl/SetControlAttribute/FindControlById/AddChild+RemoveChild；替换为完整迁移表：63 个现有导出按 6 组（帧循环 9 / 布局 4 / 控件工厂 25 含菜单 5 / 通用操作 5 / 属性 16 / 回调 1）+ 4 个 Debug 辅助
- InitFromPlugin 迁移为 CreateInstanceFromPlugin（保留 LoadLibrary + GetUIBackendCallbacks 能力）
- 伪代码全部改用真实 UIEvent{type; uint8_t data[128]} + UI_EVENT_MOUSE_X/Y 等宏（无 x/y/keycode/mod 直接字段）；ench->inputControl 需 uiEventToEvent 转换（UICornerstoneAPI.cpp:223）；UI_EVENT_CLICK 不存在（枚举为原始事件 UI_EVENT_MOUSE_DOWN 等）

**修正事实错误**：

- 全局变量 13→14：补 g_menuPool（UICornerstoneAPI.cpp:63）与 static char buf[256]（:637，线程不安全，改为实例内缓冲）
- 后端静态缓存 12→15：补 createResourceProvider；**5 个 destroyXxx 回调在 UIBackendCallbacks 中已存在**（UICornerstoneAPI.h:87-153），§5.6 直接采用方案 A（原稿误以为不存在、推荐方案 B）
- §5.3 Bench：实际继承 Panel+TopControl 是控件树根（Bench.h:11），无 getRootControl()/createControlByType
- §5.4 补 m_eventQueueInstance：ControlImpl 构造绑定 EventQueue::getInstance()（ControlBase.cpp:657-659）、TopControl 构造（ControlBase.h:515）；**setContext 必须同步 m_eventQueueInstance**；析构 MAINWIN 引用（ControlBase.cpp:663）改经 m_context
- §5.13.4 UIContext 结构体补 children/activeViewport/focusManager/menuPool 字段；§5.13.5 补队列消费说明（只调 ProcessEvents(owner) 会让子视口输入积压）
- RenderDevice 为 setClipRect(const SRect&)（RenderDevice.h:25），全文 setScissor 清除
- std::erase（C++20）→ remove_if+erase（CMakeLists.txt:4 为 C++17）
- UIInstanceConfig 两处不一致统一为 structSize 版（§5.2/§5.11.1）
- §5.12 补 samples×4（hello_uicornerstone/sample_programmatic/sample_fromsource/sample_loadlibrary）+ test_fromsource_cabi 适配（后两者走 CreateInstanceFromPlugin）

**补全遗漏**：§7 新增 3 条风险（跨实例 IME 冲突、销毁期回调重入、裸指针句柄归属校验）；收益补充静态析构顺序消除（呼应 2026-07-21 Raylib DLL 慢退出）；§6 清单同步 14 全局/方案 A/samples；TOC 补 5.11-5.13。

**验证**：残留虚构引用清零；文件补 UTF-8 BOM（编辑工具会丢 BOM，与上次 history.md 教训一致）。

**未提交**（用户指示更新但不提交）。

### 2026-07-31: CABI_MultiInstance_Design.md 复核修订（第二轮，对照源码二次核验）

**背景**：上一轮修订（2026-07-31）提交后，对全部代码引用与内部一致性做二次核验，发现 7 项重大 + 7 项次要问题，已全部修订进文档（追加"复核修订（2026-07-31）"标注）。

**重大修正**：

- **buf 归属纠错**：`static char buf[256]` 位于 `UICornerstone_GetControlId` 函数体内（cpp:637），非匿名 namespace 全局、**非 GetString 缓冲**（GetString/GetEnum 用调用者缓冲 + strncpy_s，cpp:869-885）。§1/§2.1/§6 第 4 项三处"GetString 输出缓冲"表述改正；全局实为 14 项（13+补 g_menuPool），"遗漏 2 项"的数字矛盾澄清
- **UIInstanceConfig 仍未真正统一**：§5.11.1 有 debugLabel、§5.2 没有（修订说明称已统一，实际未统）。已按 §5.11.1 字段序（structSize→debugLabel→resourceRoot→windowTitle→windowWidth→windowHeight→reserved[6]）补齐 §5.2
- **setContext 类型错误**：`&ctx->eventQueue` 得 `EventQueue**`（eventQueue 是指针成员），改为 `ctx->eventQueue`
- **FocusManager 归属矛盾**：§5.3 MainWindow 示例与 §5.10 所有权模型仍把 FocusManager 挂 MainWindow，与 §5.13.4/§6 第 28-29 项（移入 UIContext）冲突，已统一
- **§5.6 重复块**： "移除静态缓存"与"静态缓存移除"两节代码完全相同，已删除后者
- **Debug 辅助数量**：§5.2 "6 个新增" vs 实际列出 4 个，改为 4 个
- **§5.13.5 ProcessEvents 重写（关键）**：原稿混写两条事件通路——轮询通路实际产出 **C++ `Event`**（`InputBackend::pollEvent(Event&)`，InputBackend.h:25），不是 UIEvent；坐标在 `mousePos/mouseButton/mouseWheel`（EventTypes.h:158-160）；`uiEventToEvent` 为两参数签名（cpp:223）；`inputControl` 收 `shared_ptr<Event>`。已按真实实现重写路由伪代码（轮询 → Event 层路由 + 坐标转换 + 直接 dispatch；注入队列 → uiEventToEvent → dispatch），并同步 §5.13.6/§6 第 24 项表述

**次要修正**：

- Render 伪代码改 `pushClipRect/popClipRect`（现实现 UICornerstoneAPI.cpp:343-345），删"恢复整窗"步骤；§5.13.2 表格同步
- **countVisibleBoundaries 断言纠错**：`m_boundaries` 仅由 WinFrame（WinFrame.cpp:85）/Dialog（Dialog.cpp:107）注册，Bench 不注册——"Bench 是首个 boundary 始终 +1"不成立；智能路由条件 `>1` 改 `>=1`（规则块/Mermaid 图/伪代码/K4 同步）
- §5.8 Mermaid Note "createRootControl" → "Bench 即控件树根"
- §5.13.4 UIContext 结构体补 `strBuf`（此前只补了 menuPool/children/activeViewport/focusManager）
- §5.12.2 测试 1 补 `#include "BackendPlugin.h"`（GetUIBackendCallbacks 声明）
- §5.12.1 "§6 第 20-22 项" → "第 20 项"
- tryViewportScopeSwitch 参数 `UIEvent&` → `Event&`，键码判断改为 `keyEvent.keyEvent.keycode/mod`（KeyCode::Tab=0x09、KeyMod::LCtrl=0x0040 已核实）

**核验确认无误的项**：UI_EVENT_MOUSE_DOWN/WHEEL 等为 UIEventType 枚举值（h:45-58）；InputBackend::newFrame() 存在（InputBackend.h:31）；UIInstanceConfig 修订版含 structSize 与 §7.2 一致；§5.9 rollback 标签逻辑正确。

**验证**：残留 setScissor/getRootControl/单参 uiEventToEvent/&ctx->eventQueue 等清零；grep 复核通过。

### 2026-07-31: 文档作者第二轮审核（second audit pass）之复核 + 4 处修正

**背景**：文档作者在 2eb8a29（第一轮修订）基础上提交 b2c87d6（second audit pass，修订 198 行）。逐条对照真实源码复核作者分析。

**作者分析正确（核验通过）**：

- static char buf[256] 在 UICornerstone_GetControlId 函数体内（UICornerstoneAPI.cpp:635-647），非全局；GetString/GetEnum 用调用者 out 缓冲 + strncpy_s（:869-884），与 buf 无关
- UIInstanceConfig 补 debugLabel（§5.11.1 定义确有该字段，字段序 structSize→debugLabel→resourceRoot→...）
- Debug 辅助 6→4（第一轮笔误）
- setContext 的 &ctx->eventQueue→ctx->eventQueue（eventQueue 是指针成员）
- Render 用 pushClipRect/popClipRect（RenderDevice.h:27-28；现实现 cpp:341-346）
- **两条事件通路**：uiEventToEvent 两参数 bool（cpp:223）；pollEvent(Event&) 直接产出 C++ Event（InputBackend.h:25）；合流到 bench->inputControl（cpp:293-333）
- m_boundaries 仅 WinFrame（WinFrame.cpp:85/352）+ Dialog（Dialog.cpp:107）注册；Bench.cpp:33 只设 m_isFocusBoundary 不注册；focusNextScope 回退引用（FocusManager.cpp:190-198）准确

**复核发现的 4 处问题（本次已修正）**：

1. 	ryViewportScopeSwitch Ctrl 判定 !(mod & KeyMod::LCtrl) 漏 RCtrl——现实现 Bench.cpp:81 为 LCtrl||RCtrl，KeyMod::Ctrl=LCtrl|RCtrl（EventTypes.h:131）。改为 !isModSet(mod, KeyMod::Ctrl)，并补 ool shift = isModSet(mod, KeyMod::Shift)（原伪代码 shift 未定义）
2. cur->focusManager.clearFocus()、owner->activeViewport->focusManager.focusFirstInScope(...) 用 . 访问指针成员（§5.13.4 定义 FocusManager*）→ 改 ->
3. **K2 与新条件自相矛盾**：双视口各单 WinFrame 时 countVisibleBoundaries>=1 → 视口内优先，不会跨视口，但 K2 原预期"跨视口切换"。K2 及测试桩已同步：首次 Ctrl+Tab 聚焦本视口 WinFrame，隐藏后（count==0）才跨视口
4. §6 第 25 项仍写 setClipRect，与 §5.13.5 的 pushClipRect/popClipRect 不一致 → 已同步

**顺带改进**：坐标写回统一用 evt.mousePos.x/y 依赖 union 布局兼容（三事件坐标均在偏移 0，EventTypes.h:158-160），已补注释说明。

**未提交**（用户指示更新但不提交）。

### 2026-07-31: 第四轮复核（third audit pass 之复核 + 深入遗漏排查，修订 12 处）

**背景**：主设计 Session 提交 81d5bed（third audit pass，4 处修正 + 1 处改进）。前 2 处遗留问题（L1607 设计依据矛盾、K2 测试桩 isFocused 断言）修订后，做全文深入排查，又发现 8 处此前三轮均遗漏的问题。

**主 Session 修正核实（全部通过）**：

- Ctrl 判定 `isModSet(mod, KeyMod::Ctrl)`：Bench.cpp:81 确为 LCtrl||RCtrl；KeyMod::Ctrl=LCtrl|RCtrl（EventTypes.h:131）；isModSet 存在（EventTypes.h:142）。**补充**：原 `!(mod & KeyMod::LCtrl)` 因 enum class 无 operator!/隐式转 bool 本就编译不过，改 isModSet 是双重正确
- `.` → `->`（FocusManager* 指针成员，§5.13.4）
- K2 与 >=1 条件自相矛盾修正（内部闭环）
- §6 第 25 项 setClipRect 残留（第二轮漏项）
- 坐标写回 union 布局注释（EventTypes.h:158-160 均以 x,y 起始）

**本次新发现并修订（8 处）**：

1. **注入通路与路由/窗口事件脱节**（关键）：§5.13.5 伪代码注入通路直接 `bench->inputControl`，未过 tryViewportScopeSwitch——K2 测试桩用 PushUIEvent 注入 Ctrl+Tab 将永远走视口内逻辑，跨视口路由测试无效；且注入通路缺 WindowClose/WindowResize 分支（现实现 cpp:302-305 有）。已补：注入通路对齐轮询通路（Close→quit、Resize→resized、KeyDown/Up 先过 tryViewportScopeSwitch）
2. **"注意"段过时**：原"只调 ProcessEvents(owner) 会让子视口输入积压"基于旧的队列推送实现；新实现 owner 直接 dispatch 到子视口 bench，与子视口 ProcessEvents 无关。已重写（积压仅适用于外部 PushUIEvent 注入且从不调子视口 ProcessEvents）；时序第 8/9 步、语义分化块同步
3. **UI_LOGI/UI_LOGW/UI_LOGE 宏错误**：`UI_LOG(ctx, "INFO", __VA_ARGS__)` 把 "INFO" 当格式串、消息内容变多余参数被丢弃。改为 `UI_LOG(ctx, "[INFO] " fmt, ##__VA_ARGS__)`
4. **LeakDetector Release 编译错误**：s_aliveInstances 仅 _DEBUG 定义（§5.11.3），LeakDetector 无条件引用 → Release 编译失败；且表格"泄漏检测 Release 保留"自相矛盾。已包 #ifdef _DEBUG + 表格改"摘除"
5. **CreateViewport 用 SRect 错误**：UICornerstoneAPI.h 是纯 C 头（仅 stdint/stddef），无 SRect——有 UIRect（h:34，布局相同）。C ABI 边界应传 UIRect；且示例/测试代码 `(SRect){...}` 是 C99 compound literal，**C++ 编译不过**。已改 `UIRect{...}` 聚合初始化（§5.13.4 示例、§5.13.7 测试、§6 第 23 项签名）
6. §5.11.5 `backend->createWindow` → `callbacks->createWindow`（UIBackendCallbacks 回调表成员）
7. findViewportByCoord 注释残留 "UI_EVENT_MOUSE_X 返回 float"（基于 UIEvent 的旧说明）→ 改为 Event 坐标字段
8. §5.13.3 瓶颈 3 / §5.13.4 结构体：瓶颈描述 setClipRect → pushClipRect/popClipRect；结构体补 pathPrefix 注释（与 §5.1 定义对齐）

**遗留提示**：注入通路路由后，K2 测试桩（PushUIEvent 注入）在实现时可验证跨视口路由；轮询通路需真实键盘事件（测试可依赖注入通路或直接调内部函数）。

### 2026-07-31: 第五轮复核（fourth audit pass 之复核 + 深入遗漏排查，修订 6 处）

**背景**：辅设计 Session 提交 9b72a85（fourth audit pass，修订 12 处：注入/路由通路统一、UI_LOG/LeakDetector 宏、UIRect for C ABI）。本 Session 逐条核实事实依据（全部通过），并做全文深入排查，又发现 4 处遗漏 + 2 处行为差异说明。

**辅设计 Session 修正核实（8 处全部通过）**：

- 注入通路对齐轮询通路（Close→quit、Resize→resized、KeyDown/Up 先过 tryViewportScopeSwitch）——且为 K2 测试桩（PushUIEvent 注入 Ctrl+Tab）生效的必要条件
- "注意"段重写 + 时序第 8/9 步（轮询直接 dispatch，与 cpp:293-333 一致）
- UI_LOGI/UI_LOGW/UI_LOGE 宏：`UI_LOG(ctx, "[INFO] " fmt, ##__VA_ARGS__)` 字符串拼接正确（原 "INFO" 当 fmt 吞消息）
- LeakDetector 包 `#ifdef _DEBUG` + 表格改"摘除"（s_aliveInstances 仅 _DEBUG 定义）
- CreateViewport 改 UIRect：UICornerstoneAPI.h:34 纯 C 结构体；SRect 是 C++ class（Utility.h:185），C ABI 按值传 C++ class 是 UB；`(SRect){...}` C99 compound literal 在 C++ 编译不过
- `backend->createWindow` → `callbacks->createWindow`（回调表成员）
- findViewportByCoord 注释 float 来源（EventTypes.h:158-160）
- §5.13.3 setClipRect → pushClipRect/popClipRect（栈实现 RenderDevice.cpp:98-110，支持嵌套；控件内部无 setClipRect 调用，只有桥接层）

**本次新发现并修订（6 处）**：

1. **addChild 直赋 m_context 绕过 setContext 同步**（关键）：§5.4 方式 2 `child->m_context = m_context` 直赋，不同步 m_eventQueueInstance（§5.4 修订说明刚强调 setContext 必须同步）→ 事件投递仍指向旧实例。已改为 `child->setContext(m_context)`
2. **destroying 标志未落地**：§7 风险 4 缓解措施要求 `instance->destroying` 置位 + C ABI 入口短路，但 §5.13.4 结构体无此字段、DestroyInstance 伪代码未置位。已补：结构体字段 + 伪代码置位/防重入 + 缓解措施明确守卫范围（ProcessEvents/Render/Update/PushUIEvent/SetCallback/Debug 辅助）+ §6 26b
3. **未命中子视口的鼠标事件被丢弃**（`if (!target) break`）：owner 是全窗口兜底视口（自身 bench），子视口未覆盖区域与 owner 控件交互失效。已改为路由给 owner 自身 bench（Bench 内部未命中控件则焦点保留，同现实现）
4. **activeViewport 初始 nullptr 与 K2 测试桩矛盾**：1796 行断言 `Debug_GetActiveViewport(win) == vp1` 在初始 nullptr（且测试桩从未点击）下必失败；且窗口启动后首次点击前键盘事件无处投递。已约定：**CreateViewport 创建首个子视口时自动设为 owner->activeViewport**
5. 注入通路键盘 fallback 行为差异说明：注入到 owner 走 owner 树（注入语义 = 显式指定投递目标），与轮询通路（activeViewport->bench）不同，已注释明示
6. 已核验排除项：§7 风险 3 IME 引用有效（startTextInput/stopTextInput 存在，InputBackend.h:15-17）；pushClipRect 栈实现支持嵌套（RenderDevice.cpp:98-110），多视口 Render 与控件内裁剪不冲突

**未提交**（用户指示更新但不提交）。

### 2026-07-31: 第六轮复核（fifth audit pass 之复核 + 深入遗漏排查，修订 6 处）

**背景**：主设计 Session 提交 6284881（fifth audit pass，6 处修正）。逐条核实（SRect 确为 class，Utility.h:185；addControl 真实实现 ControlBase.cpp:304-317）后，发现 1 处严重遗漏 + 3 处次要问题，已修订。

**主 Session 修正核实（6 处全部通过）**：

- addChild 直赋 → setContext：正确（与 §5.4 修订说明的 m_eventQueueInstance 同步一致）；但方法名 addChild 非真实 addControl，且真实实现（ControlBase.cpp:304-317）含 setParent + setRenderDevice(getRenderDevice())——多实例化须在 addControl 中同步 setContext；setRenderDevice 经宏重定义自动适配。已对齐命名并补实现骨架
- destroying 标志落地：正确（§7 风险 4 与结构体/伪代码/§6 脱节修复；顺带防 double-destroy）
- 未命中子视口 → owner 兜底：正确（鼠标兜底）
- activeViewport 首个子视口自动激活：正确且必要（否则 K2 断言失败 + countVisibleBoundaries(nullptr) 空指针）
- 注入通路 fallback 语义说明：正确
- 排除项核实（IME、pushClipRect 栈嵌套）：准确

**本次新发现并修订（4 处）**：

1. **键盘路由 nullptr 丢弃（严重）**：轮询通路 `if (!tryViewportScopeSwitch(...) && instance->activeViewport)` 在无子视口的纯多实例场景（children 空，activeViewport 恒 nullptr）**静默丢弃全部键盘事件**——多实例设计的基础场景（双窗口测试、全部现有测试改造）键盘不可用，与第五轮刚补的鼠标兜底不对称。已改：`kbdTarget = activeViewport ? activeViewport : instance`（nullptr 回退 owner 树）
2. **owner 兜底点击的 activeViewport 状态未定义**：点击 owner 区域（子视口空隙）只投递事件，不清旧视口焦点、不更新 activeViewport → 鼠标焦点（owner 树）与键盘焦点（子视口）分离。已补：owner 兜底 MouseDown/Up → 旧视口 clearFocus + activeViewport=nullptr（与 1 结合，nullptr 语义 = 焦点在 owner 树）
3. **K6 测试注入目标未注明**：注入到 win 的 Tab 按注入语义走 owner 树，测不到"vp2 内循环"。K6 已注明注入目标须为 vp2 或走轮询通路
4. **§5.4 方式 2 命名与实现骨架**：addChild → ControlImpl::addControl（真实签名 shared_ptr<Control> + setParent + setRenderDevice），补 setContext 同步点；§5.13.4 activeViewport 注释补 nullptr 语义；§6 第 24 项补键盘回退

**遗留提示**：owner 兜底点击后 activeViewport=nullptr，再点击子视口正常转移；销毁 activeViewport 的视口时 owner 需置 nullptr（§5.13.5 已有约定）。

### 2026-07-31: 第七轮复核（sixth audit pass 之复核 + 深入遗漏排查，修订 6 处）

**背景**：辅设计 Session 提交 37b52d3（sixth audit pass，5 处修正：addControl 实名化、activeViewport nullptr 语义、owner 兜底点击焦点转移、键盘 fallback 退回 owner、K6 注入目标修正）。本 Session 逐条核实源码事实（全部通过），并排查第六轮行为的连带影响，发现 1 处严重联动漏洞 + 3 处同步遗漏。

**辅设计 Session 修正核实（5 处全部通过）**：

- addControl 实名化：真实方法 `ControlImpl::addControl(shared_ptr<Control>)`（ControlBase.cpp:304-317）——含 nullptr 检查、重复检查（find）、setParent(this)、setRenderDevice(getRenderDevice())、stabilizeTopmostChildren()（ControlBase.h:326）；真实实现**无 m_context 继承**，文档伪代码为改造版（保留 setContext 同步），处理正确
- activeViewport 注释补 nullptr 语义（焦点在 owner 树）——正确
- owner 兜底点击清旧视口焦点 + activeViewport=nullptr（避免鼠标/键盘焦点分离）——正确且必要
- 键盘 fallback 退回 owner bench：原 `&& instance->activeViewport` 在纯多实例场景（children 空）静默丢弃全部键盘事件，与鼠标兜底不对称——正确（顺带修复 K1 场景）
- K6 注入目标须为 vp2（注入到 win 走 owner 树）——正确，与 §5.13.5 注入通路 fallback 语义自洽

**本次新发现并修订（6 处）**：

1. **第六轮联动漏洞（严重）**：新增"点击 owner 区域 → activeViewport=nullptr"后，多视口场景 Ctrl+Tab 可达 `countVisibleBoundaries(nullptr)` 与 `nextViewport(owner, nullptr)`——**null 解引用崩溃**（此前被单视口短路 children.size()<=1 挡住，纯多实例 children 空同样短路；第六轮使多视口下 activeViewport 首次可为 null）。已修：tryViewportScopeSwitch 判空（cur 非空才查 boundary、才 clearFocus）、防御性无子视口不消费、countVisibleBoundaries 入参判空（nullptr→0）、cur==nullptr 时切入 children.front()/back()（shift 反向）
2. **nextViewport/prevViewport 未定义**：文档仅引用无定义。已补实现（children 序列环移，nullptr 起点 = front/back）
3. **时序段落未覆盖 owner 兜底转移**：新增"点击 owner 区域 → 焦点回 owner 树"独立时序（MouseDown/Up 触发、清焦点、置 null、事件进 owner bench、后续键盘行为、再点击子视口恢复）
4. **Ctrl+Tab 规则文本/流程图边界**：规则文本与验证点补 activeViewport==null 分支（单视口+点击 owner 区域 → fallback 退回 owner 树；跨视口起点取 front/back）
5. **语义分化块第 4 步残留**："不匹配 → 视为窗口事件"与第六轮兜底行为冲突 → 同步为"dispatch 到 owner bench + MouseDown/Up 清焦点/置 null"
6. **§7 风险 2 命名残留**：addChild → addControl（第六轮实名化后风险段未同步）；§6 26b 补兜底点击清 activeViewport + 键盘 fallback 实现项

**已核验排除**：注入通路 tryViewportScopeSwitch 不受影响（注入到 vpA 时 children 空短路，注入到 owner 正常）；K1 场景（纯多实例 Ctrl+Tab）由第六轮键盘 fallback 修复后与"2 个 WinFrame 间切换"预期一致。

**未提交**（用户指示更新但不提交）。

### 2026-07-31: 第八轮复核（seventh audit pass 之复核 + 深入遗漏排查，修订 4 处）

**背景**：主设计 Session 提交 fbca905（seventh audit pass，6 处修正：第六轮联动漏洞修复、nextViewport/prevViewport 定义补齐、owner 兜底点击时序段、Ctrl+Tab 规则文本 null 分支、语义分化块同步、addControl 命名残留清理）。本 Session 逐条核实（全部通过），并沿 activeViewport==null 新语义继续排查销毁路径与可访问性，发现 1 处重要 + 3 处次要遗漏。

**主设计 Session 修正核实（6 处全部通过）**：

- 联动漏洞修复：`cur && countVisibleBoundaries(cur) >= 1`、`if (cur) clearFocus()`、countVisibleBoundaries 入参判空（nullptr→0）、cur==nullptr 时 nextViewport/prevViewport 取 front/back——修复完整，路径分析确认此前确被 children.size()<=1 短路掩盖，第六轮使多视口下 activeViewport 首次可为 null
- nextViewport/prevViewport 环移实现正确：`++it != end ? *it : front()`；`it != begin ? prev(it) : back()`（it==end 时 prev(end)==last，与 front 起点语义自洽）
- `if (!owner->activeViewport) return false;` 防御：实际不可达（children 空已被 size()<=1 短路），无害冗余，可保留
- owner 兜底点击时序段与伪代码/规则文本三方一致（MouseDown/Up 触发、清焦点、置 null、事件进 owner bench、键盘后续、再点击恢复）
- 语义分化块第 4 步、§7 风险 2、§6 26b 同步正确

**本次新发现并修订（4 处）**：

1. **DestroyInstance 销毁 activeViewport 悬空（重要）**：销毁子视口循环未处理 `activeViewport == child`——验证点"析构 active 视口时 owner 将其设为 nullptr"仅有文字约定、伪代码未实现。销毁后 owner 继续运行（多窗口场景销毁其中一个窗口的视口）→ 键盘 fallback `activeViewport ? activeViewport : instance` 解引用已 delete 的 UIContext → 崩溃。已修：循环内递归前置 `if (instance->activeViewport == child) instance->activeViewport = nullptr;`
2. **countVisibleBoundaries 访问器缺口**：伪代码直接遍历 `focusManager->m_boundaries`，但该成员为私有（FocusManager.h:40），仅有 registerBoundary/unregisterBoundary（:27/29）无 getter。已注明：须新增 `FocusManager::getVisibleBoundaryCount() const` 或友元声明
3. **Mermaid 流程图未同步 null 分支**：规则文本已补"activeViewport 非空 且"，流程图 D 条件/E1 步骤仍为旧文本。已同步（D: "activeViewport 非空 且 内可见 boundary >= 1?"；E1: "若 activeViewport 非空：clearFocus 旧视口"；E2: cur 为 null 取 front/back）
4. **K 测试无 owner 兜底场景**：新增 K8（点击 owner 区域 → activeViewport==nullptr → Debug_GetActiveViewport 返回 null、旧焦点已清、Tab 走 owner 树、Ctrl+Tab 从 children.front() 切入）——覆盖 cur==nullptr 分支的回归

**已核验排除**：DestroyInstance 中 destroying 短路仅保护销毁期间的重入，不解决销毁后的悬空访问（故遗漏 1 必须修复）；递归销毁循环中 owner->children 不被修改（child 销毁不改 owner 容器），`children.clear()` 前迭代安全；nextViewport 的 `if (!owner->activeViewport) return false;` 防御不可达但无害。

**未提交**（用户指示更新但不提交）。

### 2026-07-31: 第九轮复核（eighth audit pass 之复核 + 深入遗漏排查，修订 4 处）

**背景**：辅设计 Session 提交 eb52ffe（eighth audit pass，4 处修正：mermaid null 分支同步、countVisibleBoundaries 访问器缺口、DestroyInstance 级联销毁 activeViewport 置 null、K8 测试桩）。本 Session 逐条核实源码事实（全部通过），并沿"直接销毁子视口"路径深入分析，发现 1 处严重悬垂遗漏 + 3 处同步遗漏。

**辅设计 Session 修正核实（4 处全部通过）**：

- mermaid 流程图同步（D 条件补"activeViewport 非空 且"、E1 条件 clearFocus、E2 cur 为 null 取 front/back）——与规则文本一致
- countVisibleBoundaries 访问器缺口：`m_boundaries` 确为 FocusManager 私有成员（FocusManager.h:40），现有仅 registerBoundary/unregisterBoundary（:27/29）无 getter，`getCurrentFocused()`（:25）是唯一访问器——C ABI 层直接遍历不成立，须新增 `getVisibleBoundaryCount() const` 或友元
- DestroyInstance 级联销毁循环内 `activeViewport == child` 置 null——正确（销毁后 owner 继续运行时键盘 fallback 不再解引用已 delete 对象）
- K8 测试桩（owner 兜底点击 → activeViewport==nullptr、旧焦点已清、Tab 走 owner 树、Ctrl+Tab 从 front() 切入）——与第六/七轮行为一致

**本次新发现并修订（4 处）**：

1. **直接销毁子视口路径悬垂（严重）**：级联销毁（owner 循环）已由第八轮修复，但**直接对子视口调 DestroyInstance**（5.13.7 测试第一段正是 `DestroyInstance(vp1); DestroyInstance(vp2); DestroyInstance(win)`）时：vp1/vp2 销毁后**不摘除 owner->children**（残留悬垂指针）→ 后续 `DestroyInstance(win)` 级联遍历时对已 delete 的 vp1 调 DestroyInstance → 读 `vp1->destroying` UAF；且 owner->activeViewport 若指向 vp1 同样悬垂（第八轮置 null 只在级联循环内）。已修：DestroyInstance 尾部 `if (instance->owner)` 块——activeViewport 置 null + `cs.erase(std::remove(...))` 摘除
2. **级联销毁迭代器失效**：补摘除后，owner 级联 range-for 在迭代中 erase 子项 → 迭代器失效。已改为先拷贝 `auto snapshot = instance->children` 再遍历（子销毁时摘除安全）
3. **§6 清单未同步**：26a 补 `getVisibleBoundaryCount()` 调用说明（m_boundaries 私有）；新增 26c 实现项（FocusManager.h/.cpp 新增 `int getVisibleBoundaryCount() const`）；27 项 K1-K7 → K1-K8 + 销毁顺序补悬垂防护验证
4. **生命周期管理段注记**：文字约定"销毁时摘除"现已落地伪代码，补注记明确覆盖直接销毁路径

**已核验排除**：快照拷贝后 `children.clear()` 保留（快照遍历销毁全部子视口，clear 确保容器清空，二者不冲突）；摘除在 `delete instance` 之前（用 instance->owner 指针，owner 此时必然存活——子销毁时 owner 未销毁；owner 自身销毁时 instance->owner 为 null 跳过）。

**未提交**（用户指示更新但不提交）。

### 2026-07-31: 第十轮复核（ninth audit pass 之复核 + 深入遗漏排查，修订 2 处）

**背景**：主设计 Session 提交 bd047d8（ninth audit pass，4 处修正：直接销毁子视口路径的悬垂修复、级联销毁快照遍历、§6 清单同步 26a/26c/27、生命周期注记）。本 Session 逐条核实（全部通过），并沿销毁/摘除新逻辑深入排查，未发现实质遗漏，仅 2 处小瑕疵。

### 2026-07-31: 第十一轮复核（tenth audit pass 之复核 + 深入遗漏排查，修订 2 处）

**背景**：辅设计 Session 提交 5b722eb（tenth audit pass，2 处修正：§6 26a/26b/26c 编号重排、5.13.7 验证点补"销毁后句柄失效"约定）。本 Session 逐条核实（全部通过），并沿重排/句柄失效语义排查，发现 1 处交叉引用遗漏 + 1 处注释微瑕。

**辅设计 Session 修正核实（2 处全部通过）**：

- §6 编号重排 26a→26b→26c（内容不变）：26c（getVisibleBoundaryCount）原插在 26a/26b 之间，编号不连续。核实重排后内容与文件一一对应
- 销毁后句柄失效约定：`destroying` 短路只保护销毁期间的重入，销毁后对象已 delete 任何入口无法保护（读 instance->destroying 本身即 UAF）——约定"未定义行为，调用者责任"正确且必要；引用 CppBinding_Design.md:838（绑定层经 FindControl 返回空间接感知）核实无误；"销毁 activeViewport 视口后焦点丢失不自动转移，点击子视口或 Ctrl+Tab front/back 切入均可恢复"自洽无遗漏

**本次新发现并修订（2 处）**：

1. **26a 内文交叉引用未随重排同步（关键）**：重排前 getVisibleBoundaryCount 是 26c，26a 内文"见 26c"正确；重排后 26c 已是 CreateViewport 项，26a 内文仍写"见 26c"→ 指向错误项。已改"见 26b"
2. **结构体 children 注释未补销毁摘除语义**：§5.13.4 `children` 注释仍只写"级联销毁"，第九轮落地的"直接销毁时摘除自身"未体现。已补

**已核验排除**：销毁后句柄失效约定与 §7 风险 5（Debug controlsById 校验）无冲突（校验前提即实例句柄有效）；混合场景（多窗口 × 多视口）各 owner 独立轮询无交叉；快照遍历 + 先置 null 再递归的顺序在回调重入下安全（activeViewport 已 null）。

**未提交**（用户指示更新但不提交）。

**主设计 Session 修正核实（4 处全部通过）**：

- 直接销毁子视口悬垂（严重）：分析正确——第八轮置 null 仅在 owner 级联循环内，直接 `DestroyInstance(vp1)`（5.13.7 测试正如此）不经过该循环：vp1 销毁后 win->children 残留悬垂指针（后续 `DestroyInstance(win)` 级联遍历读 vp1->destroying UAF）、win->activeViewport 残留悬垂（后续键盘 fallback 解引用 UAF）。尾部摘除块修复完整：
  - 摘除在 `delete instance` 之前、`destroy()` 之后（回调重入窗口由 destroying 短路保护）✅
  - 级联销毁时循环内置 null 先执行、尾部 `owner->activeViewport == instance` 条件不满足跳过——两处置 null 不重复、双路径全覆盖 ✅
  - owner 自身销毁时 instance->owner 为 null 跳过 ✅；三层嵌套（视口带子视口）递归摘除完整 ✅
  - 销毁后坐标路由遍历不到已销毁视口（children 已摘除）、键盘 fallback 走 owner（activeViewport 已 null）、销毁后重建首个子视口自动激活——均自洽 ✅
- 快照遍历：必要且正确——递归中 `erase(remove(...))` 修改原容器，快照拷贝迭代不受影响；循环后 `children.clear()` 无害（已空）✅
- §6 同步内容正确 ✅
- 生命周期注记正确 ✅

**本次新发现并修订（2 处，均为小瑕疵）**：

1. **§6 编号不连续**：26c（getVisibleBoundaryCount）插在 26a/26b 之间，编号顺序 26a→26c→26b。已重排为 26a→26b→26c（内容不变）
2. **销毁后句柄失效约定缺失**：摘除逻辑落地后"销毁后句柄不得再使用"语义需明确（`destroying` 短路只保护销毁期间的重入，销毁后任何入口均无法保护——对象已 delete）。已在 5.13.7 验证点补一条约定（未定义行为，调用者责任）

**已核验排除**：句柄失效约定核对——CppBinding_Design.md:838 仅绑定层"自动失效检测"（经 FindControl 返回空间接感知），核心库无此约定，补入 5.13.7 验证点；销毁 activeViewport 视口后焦点丢失（不自动转移）符合"置 nullptr"约定，点击子视口或 Ctrl+Tab（front/back 切入）均可恢复，自洽无遗漏。

**未提交**（用户指示更新但不提交）。

### 2026-08-04: 标准测试全量验证 + test_slider 退出段错误修复 + 测试 DLL 同步修复（Complete）

**背景**：三后端（sdl3/sfml/raylib）标准测试全部编译通过（0 error，31 个 exe 输出至 `build/{backend}/test/Debug/`），逐批运行视觉验证时发现两个问题。

**1. test_slider 退出阶段段错误（EXIT=139，三后端均复现）——已修复**：

- **现象**：`Slider test quit` / `[Instance_1] [INFO] destroyed` / `run returned 0, exiting` 全部打印完毕之后崩溃——即 main 正常返回后、全局 `shared_ptr<Slider>` 退出期析构阶段。
- **根因**：`Slider::~Slider()` → `destroyCachedTickTexts()`（src/Slider.cpp:221-229）对 `m_cachedTickTexts` 中每个句柄调 `getTextRenderer()->destroyText(t)`，但**没有实例存活守卫**。test_slider 的 17 个 Slider 由全局 `shared_ptr` 持有，`DestroyInstance` 时控件计数未归零，析构推迟到 main 返回后——此时 `BackendManager::shutdown` 已释放 textRenderer，对**悬垂 renderer 调用 destroyText** 即崩溃。
- **修复**：与 `Label::releaseTexts`（src/Label.cpp:58-68，早已有此守卫）同一模式，`destroyCachedTickTexts` 先查 `UIContext::isActive(m_context)`，实例已销毁则放弃缓存文本（进程退出回收）。
- **验证**：三后端 `test_slider` 修复后均 **EXIT=0**（修复前 SDL3/SFML 均 EXIT=139）。

**2. 标准测试 exe 的 DLL 同步缺失——已修复**：

- **现象**：SFML test_layout_advanced 拉伸窗口后画面被整体缩放——`src/backend/sfml/InputBackend.cpp` 的 resize view 修复（**辅设计开发 Session 13:31 所做**，fromsource/C ABI 模式下 CallbackWindow 不转发 `Window::onResized`，SFML 需在 Resized 事件中自行 `setView`）未生效。
- **根因**：测试实际以 DLL 模式运行（`UICORNERSTONE_BUILD_DLL=ON`），库代码编译进 `UICornerstone.dll`/`UIBackend_${backend}.dll`；但 **test/CMakeLists.txt 标准测试（UI_TEST_EXECUTABLES）foreach 块原本没有 DLL 拷贝 POST_BUILD**（仅 test_api/cabi 块有）→ test/Debug 下 DLL 长期陈旧（时间戳早于源码），view 修复未进入运行目录。
- **修复**：test/CMakeLists.txt 标准测试 WIN32 分支新增 `UICORNERSTONE_BUILD_DLL` 守卫的 POST_BUILD，拷贝 `$<TARGET_FILE_DIR:UICornerstone_dll>/UICornerstone.dll` 与 `$<TARGET_FILE_DIR:UIBackend_${_BACKEND_LOWER}>/UIBackend_${_BACKEND_LOWER}.dll`。
- **验证**：重新 cmake 配置 + 重建后 test/Debug DLL 同步为最新（时间戳对齐），test_layout_advanced EXIT=0、拉伸正常。

**其他核验**：窗口标志链路完整——`CreateInstance` → `BackendManager::initialize` → `api.createWindow(title,width,height,flags)` → `CreateSFMLWindow`（src/backend/sfml/Window.cpp:84-103，`style = Titlebar|Close`，`Resizable` 标志才加 Resize）；UIWindowFlags（include/Window.h:11-15）None=0x0 / Fullscreen=0x1 / Resizable=0x20。SDK 全量重建后 `getTextRenderer`（ControlBase.cpp:506-522）确认：缓存命中直接返回、父链回溯、末级 `UIContext::isActive` 守卫，语义与守卫模式一致。

**相关文件**：src/Slider.cpp（destroyCachedTickTexts 守卫）、test/CMakeLists.txt（标准测试 DLL 同步 POST_BUILD）、src/backend/sfml/InputBackend.cpp（view resize 修复，辅设计开发 Session 所做）。

**未提交**（本会话改动：src/Slider.cpp、test/CMakeLists.txt；InputBackend.cpp 修复与多实例文档/测试同属待提交集合）。

### 2026-08-05: C ABI 测试全套验证 + 实例归属断言修复 + raylib 关闭挂死根因修复（60/60 PASS）

**背景**：对 sdl3/sfml/raylib 三后端**标准树 + _dll 专用树**共 6 棵构建树的 10 个 C ABI 测试（dialog/combobox/numericupdown/splitter/treeview/property/fromsource/api/multi_instance/multiviewport）做逐批运行验证（WM_CLOSE 自动化关闭），发现并修复 3 个真实缺陷。

**1. test_dialog_cabi 实例归属断言（SDL3，后续 SFML 同样触发）——已修复**：

- **现象**：`uiSetFloat` 触发断言 "UICornerstone: control handle not owned by this instance"（src/UICornerstoneAPI.cpp:109）。
- **根因**：`instanceHoldsControl` 只检查 bench 树、popupPool 根、menuPool 根——**未递归检查 popup/menu 的子树**。Dialog 挂载于 popupPool，其子控件（rSlider/gSlider/bSlider/aSlider）位于 Dialog 子树内，验证失败。
- **修复**：instanceHoldsControl 对 popupPool/menuPool 条目追加 `treeContains` 递归归属校验。
- **验证**：SDK 重建后 SDL3/SFML test_dialog_cabi EXIT=0，对话框正常交互。

**2. 测试树 DLL 拷贝竞态——已修复**：

- **现象**：raylib 测试重建后 `test/Debug/UICornerstone.dll` 时间戳陈旧（早于源码）——POST_BUILD 拷贝先于核心 DLL 完成，`copy_if_different` 静默跳过。
- **修复**：test/CMakeLists.txt 标准测试与 multi/multiviewport cabi 块均补 `add_dependencies(... UICornerstone_dll UIBackend_${_BACKEND_LOWER})`，构建顺序先库后拷贝。
- **验证**：raylib test/Debug 下 DLL 时间戳与源码对齐。

**3. raylib 关闭窗口后进程挂死（三后端中唯一复现）——已修复**：

- **现象**：所有 raylib 测试对 WM_CLOSE 无响应、进程永不退出（探针统计：8807 次 newFrame vs 357891 次 CLOSE-DETECTED，即单帧内无限重放）。
- **根因**：`InputBackend::pollEvent`（src/backend/raylib/InputBackend.cpp:69-81）用 `GetTime()` 浮点秒判断"新帧"来重置事件相位——同一渲染帧内多次调用 pollEvent 时浮点值也跳动 → 相位反复重置 → `WindowClose` 事件每次 pollEvent 调用都重新发出 → `ProcessEvents` 内 `while(pollEvent)` 死循环 → quit 标志永远检查不到。**附带 bug**：关闭前 GetTime 抖动还会单帧内重复弹窗/重复触发按键。
- **修复**：移除 GetTime 相位重置块，相位仅由 `newFrame()` 每帧重置（帧级 consumed 标志同理）；同相位内连续按键由 `GetKeyPressed()==0` 自然截断，多键输入不受影响。
- **验证**：WM_CLOSE 后 raylib 测试立即正常退出（CLOSE-DETECTED 恰 1 次，`done` 完整打印）。

**4. _dll 专用树补齐 multi/multiviewport C ABI 测试**：

- 三棵 `build/{backend}_dll/` 树 CMake 配置为 08-03 旧树，缺 multi_instance_cabi/multiviewport_cabi 目标、DLL 陈旧 → 重新 cmake 配置 + 全量构建补齐。

**验证结果（全部 EXIT=0 / 全部 PASS）**：6 棵树 × 10 测试 = **60/60 通过**。其中 combobox 在早期 batch 中偶发一次 EXIT=139（环境残留），修复后稳定 done；multi_instance 在 raylib_dll 树需 ~60-120s（日志 669 行），timeout 60 曾误判 124。

**相关文件**：src/UICornerstoneAPI.cpp（instanceHoldsControl 递归）、test/CMakeLists.txt（add_dependencies）、src/backend/raylib/InputBackend.cpp（pollEvent 相位重置移除）。

**未提交**（本会话累计未提交集合：src/Slider.cpp、src/UICornerstoneAPI.cpp、src/backend/raylib/InputBackend.cpp、src/backend/sfml/InputBackend.cpp、test/CMakeLists.txt、test/test_dialog_cabi.cpp、include/UIContext.h、design/CABI_MultiInstance_Design.md、design/CppBinding_Design.md、design/guidelines/build.md、design/guidelines/history.md；未跟踪：test/test_multi_instance_cabi.cpp、test/test_multiviewport_cabi.cpp）。

### 2026-08-05: Image 图片控件化（Image_Design.md 设计审核通过 + 实施，image-control 分支 d27ccca）

**背景**：辅 Session 按 design/Image_Design.md 在 image-control 分支实施零架构改动的 Image 图片控件（`UICornerstone_CreateImage` 直接复用 Actor）。主 Session 代提交代码（d27ccca，8 文件 +501/-34），辅 Session 完成验收与文档同步。

**实施要点**：

- `src/Actor.cpp`：加载 rect 语义修正——`loadTextureFromSurface` 与 `loadFromFile` fallback 分支不再无条件重置 `m_rect`（原实现 left/top 清零 + w/h 覆盖，导致 C ABI 传入 x/y/w/h 丢失）；新增 `m_explicitSize` 标记（`setRect` 传 w/h>0 时置位）：显式 rect 保留、自然尺寸模式换图后跟随新图、match-parent-rect 覆盖 w/h（父尺寸）。
- `include/Actor.h`：`isContainsPoint` 重写返回 false（纯显示控件不参与事件命中与 covered 遮挡检测，先例 HandleControl.h:46）；`draw(void)` override 使用成员 `m_alpha`；新增 `setAlpha/getAlpha/setMatchParentRect`；属性分发 override 7 个（image/image-resource 只写不读——`fs::path::string()` 临时对象悬垂，不引入缓存成员；scale-type/anchor 枚举解析同 ProgressBar 惯例）。
- `include/PropertyNames.h`：Image 段 6 常量（`image`/`image-resource`/`scale-type`/`match-parent-rect`/`alpha`/`anchor`）。
- `include/UICornerstoneAPI.h` / `src/UICornerstoneAPI.cpp`：`UICornerstone_CreateImage(instance, image, x, y, w, h)`——image 可 NULL、w/h=0 → 纹理自然尺寸；构造 + setRect + 两阶段 loadFromFile + addControl + create。
- `test/test_image.cpp`：纯 DLL 动态加载模式（结构仿 test_multi_instance_cabi.cpp），T1-T8 全绿。

**验证**：三棵 `_dll` 树（sdl3/sfml/raylib）test_image T1-T8 全部 PASS；三棵标准树编译 0 错误 + test_button/test_winframe 回归通过（Actor rect 修正向后兼容）。

**相关文件**：include/Actor.h、src/Actor.cpp、include/PropertyNames.h、include/UICornerstoneAPI.h、src/UICornerstoneAPI.cpp、test/test_image.cpp、test/CMakeLists.txt、design/Image_Design.md（状态→已审核）。

**文档同步**（本会话补充）：design/UICornerstone_DLL_Design.md（API 清单加 CreateImage）、design/CABI_MultiInstance_Design.md（迁移表 + §6 清单 #33）、design/CABI_Property_Design.md（Bool/Int/String/Enum 四表 Image 行）。

### 2026-08-07: PropertyNames 常量统一 — 枚举值全量小写 kebab-case（Complete）

**背景**：LuotiAni/LayoutParser/Theme/后端/CABI 属性系统的硬编码字符串存在多套大小写风格（Upper 组、camelCase、小写组并存）。按 `design/CABI_Property_Design.md` §6.1（属性名/值全小写、多词用连字符）统一收口到 `include/PropertyNames.h` 单一数据源。

**约定**：

- **JSON 键名**（数据契约键）保持 cocoa camelCase：`kJsonXxx` 前缀（frameRate/bgColor/arrowWidth/onClick 等）。
- **枚举值**（控件类型、对齐、布局类型、字体样式、BlendMode、绑定模式、CheckState、textMode、scaleType 等）全量小写 kebab-case：`label`/`edit-box`/`combo-box`/`check-box`/`progress-bar`/`scroll-bar`/`win-frame`/`color-picker`/`menu-bar`/`text-area`/`confirm-popup`/`numeric-up-down`/`treeview`、`h-flow`/`v-flow`/`anchor`/`grid`、`top-left`~`bottom-center`、`top-stretch`/`bottom-stretch`/`left-stretch`/`right-stretch`/`fill`、`normal`/`bold`/`italic`/`underline`/`strikethrough`、`oneway`/`twoway`、`additive-premultiplied`/`blend-premultiplied`、`separator` 等。

**去重与删除**：

- 删除 `*Upper` 后缀常量组（kCheckState*Upper/kCheckBoxStyle*Upper/kProgressStyleVerticalUpper 等）与 `kAlignJson*` 组，统一复用既有小写常量（kCheckUnchecked/kVAlignTop/kLayoutTextRight/kStyleCross/kOrientVertical/kTextModeNone/kScaleTypeFitCenter 等）。
- 新增 `kAlignLower*` 组同时服务 LayoutParser JSON 与 LayoutEngine/CABI；`kAlignLowerCenter == kVAlignCenter`（"center"）。
- 补缺口常量：kEditable/kSelectedId/kDefaultMenuTitle/kDefaultWinFrameTitle/kJsonBorderVisible。

**替换范围**：

- `src/LayoutParser.cpp`（控件类型分派、事件键 on*→kEventKey*、菜单 menuJson/itemJson、CheckBox 主题色段 kThemeCheckbox*、CheckState 解析、parseAlignment、parseGridSize auto/fr/px、组件系统 template/props、stateColor 键、字体、颜色通道、rect 字段、knownTypes 集合）。
- `src/Theme.cpp`（parseHexColor/parseStateColor 通道键、fonts./colors. 路径拼接、default 类别）；`src/Actor.cpp`（scaleType/anchor 读写双向）；`src/ControlBase.cpp`（getColorProperty/getStateColorProperty 复合键 background.hover/pressed/disabled、text-shadow 等）；`src/ComboBox.cpp`（editable、items 子键）；`src/TreeView.cpp`（selected-id）；`src/ScrollBar.cpp`（vertical/horizontal）；`src/WinFrame.cpp`（默认标题）；`src/Panel.cpp`（Grid/Anchor）；`src/LayoutEngine.cpp`（anchor 12 值）；`src/EditBox.cpp`/`src/Label.cpp`/`src/ProgressBar.cpp`/`src/CheckBox.cpp`/`src/Slider.cpp`（对齐/方向/textMode/style/checkState 枚举读写）。
- `include/LayoutEngine.h`：getType() 四布局名 + AnchorInfo::anchor 默认值 → 常量。
- 后端：`src/UICornerstoneAPI.cpp`、`src/backend/sdl3|sfml|raylib/RenderDevice.cpp`（vsync/swap-ratio/renderer-name 键），补 PropertyNames.h include。
- 数据：`layouts/test_layout.json`、`layouts/test_layout_advanced.json` 枚举值同步为小写 kebab-case（caption 显示文本保留原样）。

**修复的编译问题**：kSelectedId 重复定义（C2086）、LayoutParser shadow 变量缺失、ProgressBar colors 块重复残留、parseRect missing 未声明、ProgressBar kTextModeNoneLower 残留引用。

**验证**：`cmake --build build --config Debug --target sample_cpp_hosted sample_cpp_embed test_layout test_layout_advanced` 0 错误；test_layout/test_layout_advanced 带 `auto=N` 参数干净退出（无参运行挂起等窗口属测试设计：TestInstance.h scheduleAutoQuit 需 `auto=<秒>`，非缺陷），JSON 全量解析与事件驱动（菜单/自动绑定/按钮点击）正常，无回归。

**相关文件**：include/PropertyNames.h（唯一数据源）、src/LayoutParser.cpp、src/Theme.cpp、src/Actor.cpp、src/ControlBase.cpp、src/ComboBox.cpp、src/TreeView.cpp、src/ScrollBar.cpp、src/WinFrame.cpp、src/Panel.cpp、src/LayoutEngine.cpp、src/EditBox.cpp、src/Label.cpp、src/ProgressBar.cpp、src/CheckBox.cpp、src/Slider.cpp、include/LayoutEngine.h、src/UICornerstoneAPI.cpp、src/backend/{sdl3,sfml,raylib}/RenderDevice.cpp、layouts/test_layout.json、layouts/test_layout_advanced.json。




### 2026-08-07: JSON type 规范统一收尾 + 重构回归修复（Complete）

**背景**：PropertyNames 常量统一后，test/*.cpp、test/test_api.c、design/*.md、layouts/*.json 的 JSON type 仍有残留大写写法（`"type": "Button"`、`"type": "Panel"`、`	ype\: \Panel\` 等多种引号转义变体），LayoutParser 不做归一化（大小写不匹配即 unknown），导致多处 LoadLayout 失败。

**本次工作**：

- **JSON type 全量统一 kebab 小写**：正则清理 test/*.cpp、test/test_api.c（双重转义变体）、binding/samples/*.cpp、layouts/*.json、design/*.md（12 个设计文档 + guidelines/testing.md）全部 type 值（含 Panel/Label/Button/EditBox/CheckBox/ProgressBar/TreeView 等）。
- **design/LayoutSystem_Design.md §4.2**：type 字段补规范声明——必须 kebab 小写（如 `"label"`/`"edit-box"`），大小写不匹配一律视为未知类型跳过，不兼容 `"Label"`/`"NumericUpDown"` 等写法。
- **修复 Actor.cpp setEnumProperty 重构回归**：anchor 枚举 9 值被错误合并进 scale-type 分支（git diff 确认重构引入），恢复独立 kAnchor 分支 → test_image 的 `uiSetEnum(inst,img,"anchor","center")` 断言通过。
- **test_multi_instance / test_multi_instance_cabi / test_treeview_cabi**：内嵌 JSON 转义引号变体未覆盖导致 LoadLayout 失败，正则统一清理后通过。

**验证**：全量测试集（含 test_api auto=5、test_aniviewer 带 jsonc 路径参数 auto=3 loop=0）全部 exit=0，无回归。

**相关文件**：src/Actor.cpp、test/test_api.c、test/test_multi_instance.cpp、test/test_multi_instance_cabi.cpp、test/test_treeview_cabi.cpp、design/LayoutSystem_Design.md、design/guidelines/testing.md 及 design/*.md 示例。

### 2026-08-08: 多实例事件隔离 + hover/焦点跨窗口修复 + C++ Binding 多实例样例（Complete）

**背景**：`sample_cpp_multiview` 右下视口 Popup 不显示；新样例 `sample_cpp_multiinstance`（双窗口双向通信）暴露出多窗口场景的系列问题：跨窗口鼠标事件串扰、窗口"未响应"（沙漏）、hover 跨窗口串扰、跨窗口焦点环并存。逐项排查修复，最终按用户伪码方案实现多实例事件泵。

**1. Popup 坐标语义统一（用户定案）**：

- `Popup::m_rect` 为**父（bench）相对本地坐标**（与普通控件一致），`computeTargetRect` 内部换算视口/bench 偏移（Centered 用 vp 尺寸本地居中，Anchored 用 `adr - bench 偏移` + 本地钳制 0..vp 尺寸）
- 曾加的 `Popup::getDrawRect` 重载（返回绝对 m_rect）按用户要求**删除**，用通用实现（叠加父偏移）
- `src/ComboBox.cpp`/`src/ColorPicker.cpp` `computePopupRect` 弹层绝对坐标 → 本地坐标转换（减 BENCH getDrawRect 偏移）
- `UICornerstone_CreateDialog` 补 `ctl->open()`（`create()` 内 setVisible(false)，不 open 永远不可见）

**2. binding Config.resourceRoot 默认改为空串**：

- 原 `"./assets"` 相对 cwd，任意 cwd 下找不到字体；空 → 核心默认 exe 目录/assets

**3. 多实例事件泵（用户定案伪码）**：

- `UICornerstone_ProcessEvents` 返回 `int`（handled ≥ 1），提取 `pumpInstanceEvents` 静态函数（while pollEvent 处理本实例事件）；`include/UICornerstoneAPI.h` 声明同步 int；binding `fnProcessEvents` 类型 int、`ProcessEvents()` 返回 bool（忽略返回值的旧调用完全兼容）
- **调用者驱动所有实例直到队列空**：样例层内层 `while (processedCount > 0) { processedCount = A.ProcessEvents()?1:0 + B.ProcessEvents()?1:0; }`
- 核心外层"遍历全部实例泵"方案撤销（`UIContext::allActive()` 成为死代码，本会话删除）

**4. sdl3 pollEvent 窗口隔离 + 未响应修复**（`src/backend/sdl3/InputBackend.cpp`）：

- **`SDL_PumpEvents()` 必须显式调用**（SDL_PeepEvents 不像 SDL_PollEvent 那样 pump 窗口消息 → 窗口 hung/沙漏）
- `SDL_PeepEvents` peek 找本窗口第一个事件 → **headOne 同 type 队头检查**（防 GETEVENT 拿错窗口同 type 事件）→ GETEVENT 消费 → **`gotEvent` 守卫**（循环结束未取到自己的事件必须 return false，不得用未初始化 sdlEvent 处理，否则永远 true → 内层 while 死循环）
- SDL_EVENT_WINDOW_FOCUS_GAINED/LOST → FocusGained/FocusLost 事件转换补全（此前落入默认被忽略）

**5. hover 跨窗口隔离**（`src/backend/sdl3/Window.cpp` + `src/ControlBase.cpp`）：

- `getMousePosition` 不能用 `SDL_GetMouseState`（返回**鼠标焦点窗口**坐标，与窗口重叠无关地串扰）→ 用 `SDL_GetGlobalMouseState` + 窗口位置/尺寸判定，鼠标不在本窗口返回 false
- `ControlImpl::update()`：`isInside = hasMouse && drawRect.contains(...)`

**6. 焦点跨实例隔离**（`src/UICornerstoneAPI.cpp`）：

- 每个实例的 FocusManager 相互独立，点击 B 窗口的 EditBox 不会清除 A 的焦点 → 利用**窗口级焦点事件**：分发 `FocusLost`（窗口失去系统焦点）→ 清除本实例（含活动子视口）焦点，保证同一时刻只有一个焦点环

**7. 新样例 `sample_cpp_multiinstance`**（binding/samples，已注册 BINDING_SAMPLES）：

- 两独立窗口实例双向通信（A 按钮→B Label，B 按钮→A Label），主循环按用户伪码内层 while 驱动，AUTO 模式双向注入验证（`[A] sent to B: hello from A` / `[B] sent to A: hello from B`）
- `sample_cpp_multiview` 内容改为 `CreateDialog` + `CreateLabel` + `dialog.AddChild(label)`（用户指令：不新增 API，`CreateDialogWithText` 已回退）

**8. 死代码清理**：`UIContext::allActive()`（include/UIContext.h + src/UIContext.cpp）无调用者，删除。

**验证**：构建 0 错误；全量回归——4 个 binding 样例（multiinstance/multiview/hosted/embed）AUTO exit=0，34 个 test（auto=N 无人值守）全部 exit=0（test_aniviewer 需 jsonc 路径参数，exit=2 为用法错误非回归）；用户手动验证：多窗口 hover 隔离、焦点环单一（点击 B EditBox → A 焦点环消失）、窗口交互正常。

**文档刷新**（实施后两轮同步）：`design/CABI_MultiInstance_Design.md`（ProcessEvents→int、语义分化表、§5.13.5 伪代码、多实例事件泵伪代码、实施状态 2026-08-08、清单 #21 改为已实施）、`design/CppBinding_Design.md`（ProcessEvents bool、resourceRoot 空串回退链路、§5.6.1 事件泵、§7.4 新样例、P16、TOC 补 7.3/7.4、samples 目录树）、`design/CppBinding_UserManual.md`（§12.1 事件泵、§12.3 CreateDialog+Label+AddChild、§3/§9 resourceRoot 默认、§15.2 改"四个样例"+构建目标）、`design/EventSystem_Design.md`（§3.2 窗口隔离+焦点事件转换、§3.3 多实例路径 pumpInstanceEvents、变更历史）、`design/BackendAbstraction_Design.md`（Phase 16i 进度表+执行清单）、`design/Dialog_Design.md`/`design/ComboBox_Design.md`/`design/ColorPicker_Design.md`（Popup 父相对坐标）、`design/UICornerstone_DLL_Design.md`（版本历史 1.18）、`design/ControlBase_Design.md`（§5.1 补 update() hover 判定+多实例语义）、`design/FocusSystem_Design.md`（§9 补窗口级焦点丢失）、`design/Tutorial.md`（§8.1 双实例示例改事件泵模式+焦点/hover 隔离说明）、`design/guidelines/history.md`（本条目）。

**相关文件**：src/UICornerstoneAPI.cpp（ProcessEvents 返回 int、pumpInstanceEvents、FocusLost 清除焦点、CreateDialog 补 open）、src/Dialog.cpp / include/Dialog.h（computeTargetRect 本地坐标）、src/ComboBox.cpp / src/ColorPicker.cpp（popup 本地坐标）、src/backend/sdl3/InputBackend.cpp（PumpEvents/PeepEvents 窗口隔离/headOne/gotEvent/FocusLost 转换）、src/backend/sdl3/Window.cpp（getMousePosition 全局坐标判定）、src/ControlBase.cpp（isInside 判定）、binding/include/UICornerstone.h（resourceRoot 空、ProcessEvents bool）、binding/src/DynamicApi.h、binding/src/UICornerstone.cpp、binding/src/DynamicApi.cpp、include/UICornerstoneAPI.h、binding/samples/sample_cpp_multiinstance.cpp（新）、binding/samples/sample_cpp_multiview.cpp、binding/CMakeLists.txt、include/UIContext.h / src/UIContext.cpp（allActive 删除）。


### 2026-08-08（傍晚）: 自动化测试参数规范 + queuedEvents 跨线程竞态修复（Complete）

**背景**：按测试规范补齐自动化参数支持（任意顺序、全测试统一）；回归中暴露 sfml/raylib 后端测试随机挂死。

**1. 自动化测试参数规范（`design/guidelines/testing.md` 新增章节）**：

- 所有标准 C++ 测试 / 集成测试 / CABI 测试必须支持 `auto=<秒>`（无人值守自动退出），参数任意顺序；无法识别的参数 WARN 后忽略。
- 例外与附加：`test_aniviewer` 的 jsonc 路径参数可出现在任意位置、缺省使用 `assets/animations/rotateBtn/rotateBtn.jsonc`；binding 样例与标准测试统一使用 `auto=<秒>`（共享解析头 `binding/samples/auto_args.h`），并支持 `backend=<后端名>` 命令行选择后端（任意顺序，缺省 sdl3）。

**2. 测试代码整改**：

- `test/TestInstance.h`：`scheduleAutoQuit` 循环扫描全部参数识别 `auto=<秒>`（原只认 argv[1]），非 auto 参数 WARN 后忽略（覆盖全部走 TestRunMain 的标准测试）。
- `test/test_aniviewer.cpp`：路径参数任意位置（非 `key=` 且非纯 0/1 的参数视为路径），缺省路径；用法文本更新。
- `test/test_api.c`、`test/test_multi_instance_cabi.cpp`、`test/test_multiviewport_cabi.cpp`、`test/test_image.cpp`：main 接受 argc/argv，任意顺序解析 auto= + WARN。
- 4 个 binding 样例（embed/hosted/multiinstance/multiview）：解析 `backend=` 传入 WithBackend，头注释说明。

**3. queuedEvents 跨线程竞态修复（核心库 bug，sfml/raylib 随机挂死）**：

- 根因：`UIContext::queuedEvents`（`std::queue`）无锁。scheduleAutoQuit 的 detach 定时线程经 `UICornerstone_PushUIEvent` push，主线程帧循环（MainWindow::run / UICornerstoneAPI 注入队列通路 / UIContext::destroy）front/pop——**并发读写 std::deque 为数据竞争（UB）**，偶发挂死/损坏。sdl3 后端帧时序恰好未撞上，sfml/raylib 回归中重现（test_animation/test_multi_instance 卡在 ALL PASS 后、test_multi_instance_cabi 卡在实例创建）。
- 修复：`UIContext` 增加 `mutable std::mutex queuedEventsMutex`，Push（UICornerstoneAPI.cpp）与全部消费点（MainWindow.cpp 帧循环、UICornerstoneAPI.cpp 注入队列通路、UIContext.cpp destroy）加锁保护。

**验证**：三后端各两轮全量回归——全部 test `auto=3` exit=0；4 样例（各树 `backend=对应后端` + sdl3 树缺省）UICORN_AUTO=1 exit=0；test_aniviewer 三后端路径任意位置 / 缺省路径均自动退出。

**相关文件**：design/guidelines/testing.md、test/TestInstance.h、test/test_aniviewer.cpp、test/test_api.c、test/test_multi_instance_cabi.cpp、test/test_multiviewport_cabi.cpp、test/test_image.cpp、binding/samples/sample_cpp_embed.cpp、binding/samples/sample_cpp_hosted.cpp、binding/samples/sample_cpp_multiinstance.cpp、binding/samples/sample_cpp_multiview.cpp、include/UIContext.h（queuedEventsMutex）、src/UICornerstoneAPI.cpp（Push 加锁）、src/MainWindow.cpp（帧循环消费加锁）、src/UIContext.cpp（destroy 加锁）。
