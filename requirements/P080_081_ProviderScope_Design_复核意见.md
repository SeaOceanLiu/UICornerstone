# P080_081_ProviderScope_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-10-09
- 对象：`design/P080_081_ProviderScope_Design.md`（P0-80 / P0-81，状态：待评审）
- 结论：**通过，放行实施**（技术路线正确；附 2 项实现建议 + 1 处措辞修正）

## 1. P0-80（绑定内存 provider 回退继承）——✓

- 根因链核实 ✓（`createMemoryResourceProvider`=裸 `new`；`bridge_setResourceProvider` 直接赋值不继承；P0-78 仅覆盖 MainWindow 自建路径）；
- 方案 ✓（`hasFilesystemFallback` + 替换时补 `ctx->resourceRoot` 回退，规则与 MainWindow 一致）——**建议**：与 `mountPath` 懒建 delegate 的幂等共存已由 `setFilesystemFallback` 的 `if (!delegate)` 保证 ✓；三后端共用同一头文件 ✓ 零 Binding 改动 ✓；
- Q-1（回退基准 resourceRoot 空→pathPrefix）✓ 确认；
- 设计器回收：`registerFonts` 删除 ✓（本批后设计器画布字体经回退解析）。

## 2. P0-81（多实例 LoadLayout 解析期上下文绑定）——✓（关键机制已核实）

- 根因：解析期控件 `m_context` 为空（父链根未挂树）→ `getResourceProvider()` 返回 null；设计器实测的"延迟补读 No provider"与纯探针差异归因（+P0-80 组合）**合理** ✓；
- 方案 `adoptContext`（构造后绑定目标实例）**技术正确**——关键核实：`GET_CONTEXT` 宏 = **控件自身 `m_context`**（include/ControlBase.h:624），非全局 → `setContext(target)` 后 `recreate()` 链（ControlBase.cpp:112-131）与 `inheritRenderer()`（:914-926）均取**目标实例**的 renderDevice/provider ✓，与 CABI `CreateXxx`（以 bench 为父构造）路径等价 ✓；
- **措辞修正**：设计文"setContext 两阶段语义不变（**不触发** create/设备缓存）"不准确——`ControlImpl::setContext` **会 `recreate()`**（:121），且目标上下文非空时 create 会实际执行（这正是所欲：解析期即建/即缓存目标设备与 provider）。建议改为"会在目标上下文触发 recreate（与 CABI 构造路径一致）；两阶段守卫不变"。解析期属性随后解析覆盖 ✓（与 CABI 相同节奏）；
- **实现建议（替代 ~20 处 parseX 调用点）**：优先**单漏斗**——在 `parseControl` 分派尾（:286-）统一 `adoptContext(result)`，天然全覆盖、少遗漏、后续新增控件类型自动受益；需注意两点：① `parseControl` 的顶层菜单栏分支提前 `return nullptr`（MenuBar 入 `m_menuBars`）——其上下文由调用方挂 BENCH 路径承担，豁免即可；② 审计不经 `parseControl` 的旁路构造（MenuPanel/MenuItem——其上下文由 `addMenu`/挂载路径设置，确认无需补）；
- Q-2：同意**全部覆盖**（单漏斗方案天然满足；若坚持逐点调用亦须覆盖 Button 内嵌动画/状态图等全部资源入口）；
- Q-3：同意时机安全（按上述措辞修正后确认）；
- Q-4：同意**另立项**（设计器 SDL3 环境 SVG 帧正常；静态探针环境解码差异不阻塞本批）。

## 3. 设计器回收与联测计划（同步后）

1. 删除 `registerFonts`（P0-80）+ `ResolvePreviewAnimationPaths`（P0-81）——**至此全部绕过清零**；
2. 联测：字体加载零 `not found`；预览 provider 动画直通（无 Resolve）`playing=1 loop=1 total=60`；编辑态动画；多实例交错加载（i1 无挂载/i2 有挂载互不串扰）；
3. 全量：auto=1/2/3（五探针）、strict、矩阵、Debug/Release。

## 4. 放行

**结论：放行实施**。P0-80 方案最小且复用既有幂等回退；P0-81 的解析期上下文绑定与 CABI 构造路径等价、机制链已核实；建议采纳"单漏斗"实现与措辞修正。
