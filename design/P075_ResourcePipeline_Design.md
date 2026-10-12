# P075_ResourcePipeline_Design — viewport.id / items hover 键 / 顶层 provider 挂载 / 动画描述 provider 加载

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-08 追加：P0-74⑥、P0-75、P0-78、P0-79）
> 前置：P0-72/P0-74 批已实施并同步
> 状态：**已放行，已实施**（2026-10-08；四问确认；Q-5 保留回退+日志；含 src provider 补充）
> 复核：`requirements/P075_ResourcePipeline_Design_复核意见.md`（通过）
> 实施备注：① `viewport.id` → `controlsById[bench]` 注册（schema 描述）；② tree/statusbar items 独立 `hover-background-color`（tree：`bgColor.setHover`+`hasHoverBg` 解耦；status：`setStatusItemHoverBackgroundColor`）+ schema/docs；③ `MemoryResourceProvider::setFilesystemFallback` + readFile/exists 末尾 delegate 回退（精确→目录前缀→assets 剥离→回退）+ `MainWindow` 改 MemoryResourceProvider（顶层挂载生效）；④ `LuotiAni::loadAniDesc` provider 优先 + fopen 回退（覆盖延迟补读）；⑤ **补充（用户要求）**：描述内层 `src:"provider:marker"` 内存取图链路固化——`getImageFromResource` 已支持，本批补测试；显式 provider 引用未命中时 `Platform::Log` 回退日志（Q-5，描述与帧两处）；验证：**P0-75 batch OK**（viewport.id==GetRoot / tree+status hb / provider:m1 挂载 32x32 / delegate 回退 32x32 / 描述经 provider total-frames=60 + loop/playing=1）；回归仅 4 项预存；`test_overlay_viewport_cabi` 因 P0-77（子视口 bench 透明）同步修正像素差分前提（子视口显式底色）后 12/0

---

## 1. 问题确认（源码核实）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-74⑥ | `viewport.id` 被忽略 | **确认** | 布局解析仅 width/height/scale-mode/background（src/LayoutParser.cpp:53-84）；`UICornerstone_SetControlId`（API:1151）+ `controlsById`（:183）机制齐备，bench 从未注册 → 宿主 `FindControl("canvasId")` 不可达 |
| P0-75 | tree/statusbar items 无独立 `hover-background-color` | **确认** | status-bar items（LayoutParser.cpp:1534-1545）仅 `background-color` 字符串/对象（hover 需嵌对象）；tree-view items（:2481-2510）同款；ListView cells（:738）与菜单项（P0-72④新增）已支持独立键 → 同族不一致；设计器 hb# 行式/模型以独立键表达 → 预览/JSON 加载被忽略（编辑态经 CABI 正常） |
| P0-78 | 顶层实例 `resourceProviders` 挂载恒跳过 | **确认** | `UICornerstone_LoadLayout`（API:1047）`dynamic_cast<MemoryResourceProvider*>`；顶层实例 provider 来自 MainWindow（src/MainWindow.cpp:12 `createFilesystem(resourceRoot)`）→ 恒失败；且内存 provider 对未注册键**无文件系统回退**（`delegate` 仅由 mountPath 懒建、只服务挂载键，src/ResourceProvider.cpp:104-111/188-205） |
| P0-79 | `provider:` 动画描述 fopen 直读 | **确认** | `LuotiAni::loadFromFile` 正确分流（src/LuotiAni.cpp:150-165）→ `loadFromResource` → `loadAniDesc(resourceId)` → **fopen 直读**（:167-200）；帧加载 `getImageFromResource` 有 provider 路径 ✓，描述加载没有 ✗ → provider:xxx.jsonc 解析失败（延迟补读 :558 同样走 fopen） |

## 2. 修改方案

### 2.1 P0-74⑥ `viewport.id`
- 解析块（viewport）增：`id`（string）→ `m_viewportTarget->controlsById[id] = reinterpret_cast<UIControlHandle>(m_viewportTarget->bench)`；
- schema `viewport` 描述补 `id`（画布代码引用，`FindControl` 可定位根画布）；
- 备注：`GetRoot`（CABI:713）+ Binding `Root()`（binding:127）已暴露——设计器 Bench 编辑可直连（P0-77 建议同步提供 bench 透明/背景 API，见 P076 文档）。

### 2.2 P0-75 items 独立 `hover-background-color`
- status-bar items（:1534 块后）：`kHoverBackgroundColor`（"hover-background-color"，`parseSingleColor`）→ `setStatusItemHoverBackgroundColor(id, c)`；
- tree-view items（:2481 块后）：`kHoverBackgroundColor` → `bgColor.setHover(c)` + `hasHoverBg = true`（与专用 API `TreeViewSetNodeHoverBackgroundColor` 同语义：不置 bgMask/hasBgStyle、不影响常态背景；P0-65② 解耦）；
- schema：status-bar items 描述/属性表 + tree-view items 描述补该键；docs（statusbar/treeview/declarative-syntax）同步；
- 与对象形态并存：对象 `{normal,hover}` 仍生效（现状），独立键优先/等价（Q-1）。

### 2.3 P0-78 顶层 provider + delegate 回退
- `MemoryResourceProvider` 增 `void setFilesystemFallback(const std::string& basePath)`（公开；创建 delegate，幂等）；
- `readFile`/`exists` 末尾增**文件系统回退**（现有链：entries → paths 精确 → 目录前缀 → assets 前缀剥离重试 → **delegate 回退**；`name` 与 `rel` 两个键都试）——回退语义=资源未注册/未挂载时按 resourceRoot 读盘（与替换前文件系统 provider 一致）；
- `MainWindow`：`m_resourceProvider` 改为 `MemoryResourceProvider` + `setFilesystemFallback(resourceRoot/prefix)` → 顶层 `resourceProviders` 挂载（memory 型判定）生效；控件相对路径资源经回退仍可读（字体等不再依赖设计器显式注册）；
- 兼容：内存 provider 的 entries/paths 精确匹配优先；`provider:` 前缀剥离、目录挂载、assets 剥离语义不变；子视口继承（UIContext.cpp:80）不变。

### 2.4 P0-79 动画描述经 provider
- `LuotiAni::loadAniDesc`：先经 `getResourceProvider()->readFile(p)`（provider 可用时；含 provider: 剥离后的资源 ID）→ 读入 `m_pJsonFileContent` → `parseJsonDesc()`；provider 未命中/不可用 → 保留 fopen 回退（文件路径场景）；
- 覆盖两入口：`loadFromFile`（provider: 分流后）与 `loadFromResource`/延迟补读（:558，挂树后 provider 就绪）；
- 与帧加载（`getImageFromResource`）同口径；文件路径语义不变。

### 2.5 P0-79 补充（2026-10-08 用户要求）：描述内层 `"src"` 的 provider 语法
- 需求：动画 jsonc 内层帧图引用支持 `"src": "provider:marker"`（从内存 provider 加载，不落盘）；
- 现状核实：`getImageFromResource`（src/LuotiAni.cpp:86-114）已剥 `provider:` 前缀并**优先 provider->readFile** → `Surface::loadFromMemory`，失败再文件回退（`assets`/exe 基准）——内存路径**已具备**；
- 本批动作：**测试固化**（内存注册 `marker` + jsonc `src:"provider:marker"` + 描述经 provider 加载 → 帧加载成功/像素非空），确保 P0-78（顶层 provider 回退）与 P0-79（描述 provider）落地后全链路可用；文档（LuotiAni/动画章节）注明 `src` 支持 `provider:` 资源 ID 语法；
- Q-5（已定，2026-10-08 用户答复）：`provider:` 前缀未命中时**保留**文件回退（P0-23 健壮性），但**回退时输出日志**（`Platform::Log`：显式 provider 引用未命中 → 记录资源名并提示走文件回退），避免静默落盘；文档注明。

## 3. 测试计划
- **74⑥**：LoadLayout 含 `viewport.id` → `FindControl(id)` == `GetRoot`（CABI 探针断言）；schema strict；
- **75**：tree/statusbar items `hover-background-color` JSON → CABI/引擎读回（tree `hasHoverBg`+bgColor.hover；status `hasHoverBackground`+色值）+ 像素探针（hover 行色）；与对象形态并存；
- **78**：顶层实例（WithResourceRoot）LoadLayout `resourceProviders` → `provider:name/...` 图片/字体加载成功；未注册相对路径（字体）经 delegate 回退可读；目录挂载/内存注册优先不回归；
- **79**：内存 provider 注册 jsonc + `path="provider:xxx"` → `GetInt("frame-count")>0`、`loop/playing` 可设、像素非空；文件路径动画回归；
- 全量回归（仅 4 项预存）+ 设计器联测（移除对应绕过后复测）。

## 4. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 75：独立键与对象 `{normal,hover}` 同时出现时优先级 | 独立键后解析覆盖（独立键优先）；文档注明 |
| Q-2 | 78：delegate 回退顺序（assets 剥离前/后）与是否也回退绝对路径 | 剥离重试后回退（保持精确优先）；绝对路径经 `operator/` 直读（同现状） |
| Q-3 | 78：MainWindow 换 provider 是否影响 `GetResourceProvider`/用户替换 provider 的语义 | 保持 `getResourceProvider()` 返回同一对象；用户替换行为不变 |
| Q-4 | 79：provider 未命中时是否回退 fopen（文件路径键） | 保留回退（兼容文件路径动画） |
