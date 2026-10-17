# P097_GhostFontProvider_Design — 拖拽 ghost 字体改资源 provider 口径（消除 CWD 相对路径报错）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-17 追加：P0-97 低）
> 前置：P0-93/94 通用拖拽框架已实施并同步
> 状态：**已放行，已实施**（2026-10-17；复核：`requirements/P096_097_复核意见.md`——通过，附可选优化）
> 实施备注（2026-10-17）：① provider 内存流优先（`getResourceProvider()->readFile` → `loadFontFromMemoryWithText(data,size,14,label)`——采纳复核可选优化 1：文本感知加载）+ `loadFont(path)` 兜底；② 采纳 Q-2：`loadOrCreate` 失败日志带路径（`SDL3TextRenderer::loadFont: '%s': %s`）；③ 采纳可选优化 1 附带：ghost 宽度按**码点**计数（UTF-8 中文不再偏宽）；④ 验证：P0-96/97 探针拖动期日志零 `Couldn't open`（`Temp/p096_097_out.txt`）；Debug/Release 零警告。

---

## 1. 复现与现状核实

**复现（设计器，拖页签期间）**：日志 `SDL3TextRenderer::loadFont: Couldn't open fonts/HarmonyOS_Sans_SC_Regular.ttf`；ghost 字体回落默认字体（拖动功能本身正常，非致命）。

**源码核实**：

| # | 事实 | 依据 |
|---|---|---|
| ① ghost 字体直开路径 | `tr->loadFont(it->second, 14)`（`it->second` = `ConstDef::fontFiles[...]` 相对路径） | `src/UICornerstoneAPI.cpp:1280-1282` |
| ② 回退分支按 CWD 解析 | `loadOrCreate` → `TTF_OpenFont(path.c_str())` 原样使用相对路径（CWD=exe 目录时 `fonts/…` 不存在 ✗） | `src/backend/sdl3/TextRenderer.cpp:154-169` |
| ③ 常规字体均为 provider 口径 ✓ | `provider->readFile(...)` → `loadFontFromMemory`（EditBox:79 / Label:305 / ListView:51 同款） | 同左 |
| ④ `loadFont(path)` 其余调用点 | `BackendBridge.h:235` / `CallbackAdapters.cpp:351` 为**宿主直通**（路径语义由宿主定义），不属本项 | 同左 |
| ⑤ Label `m_fontFile` | 经 `loadFromResource` → `provider->readFile` ✓（无 CWD 问题） | `src/Label.cpp:107,305` |

结论：问题**仅限 ghost 字体加载这一处引擎内部调用**；触发面=内置拖拽 ghost（其余为宿主直通路径）。

## 2. 方案

1. `DragRenderOverlay` ghost 字体改 **provider 口径**（与常规控件一致）：
   - `ResourceProvider* provider = s->source->getResourceProvider();`（`ControlBase.h:234/501`；`ControlImpl` 实现含父链回退 ✓）；
   - `provider->readFile(ConstDef::fontFiles.at(FontName::HarmonyOS_Sans_SC_Regular))` → `tr->loadFontFromMemory(data, size, 14)`；
2. **兜底保留**：provider 为空或数据空 → 仍尝试 `tr->loadFont(path, 14)`（真实失败保留日志，便于诊断）；异常继续 catch；
3. 缓存不变：`s->ghostFont` 首次加载后复用（每次拖动仅一次加载尝试）；字号 14px 与主题无关联保持。

## 3. 验收与测试矩阵

1. **无日志噪音**：拖动期控制台无 `Couldn't open fonts/…`；
2. **字体一致**：ghost 文本（中文）使用与控件相同的 HarmonyOS 字体（视觉核对 + 探针断言 `ghostFont != nullptr` 且 provider 命中）；
3. **回归**：P0-94 探针 8 用例全过（ghost 绘制路径覆盖）；`drag-visual=false` 不加载字体（保持）；
4. 设计器复测：拖页签日志干净、ghost 字体正常。

## 4. 影响面与文档

- 改动面：`src/UICornerstoneAPI.cpp`（`DragRenderOverlay` 字体加载，约 10 行）；无 ABI/schema 变更；
- 用户手册无需改（ghost 主题/视觉描述不变）；`design/P093_094_DragDrop_Design.md` §7.6 实施备注补一行；
- 设计器：无需改动（本项为引擎内部修复）。

## 5. 待确认（Q）

| # | 问题 | 建议 |
|---|---|---|
| Q-1 | provider 失败时保留 `loadFont(path)` 兜底（仍可能打日志） | 保留（真实失败可见） |
| Q-2 | 是否顺带在 `loadOrCreate` 失败日志中带路径（便于后续诊断） | 可选（低风险小改） |
| Q-3 | ghost 字号保持 14px（与源控件字体尺寸无关） | 确认 |
