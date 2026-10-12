# P080_081_ProviderScope_Design — 绑定内存 provider 回退继承 / 多实例 LoadLayout 解析期 provider 作用域

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-09 追加：P0-80、P0-81）
> 前置：P0-74⑥~P0-79 批已实施并同步（设计器联测主体全生效）
> 状态：**已放行，已实施**（2026-10-09；Q-1~Q-3 确认，Q-4 另立项）
> 复核：`requirements/P080_081_ProviderScope_Design_复核意见.md`（通过；采纳"单漏斗 + 构造点绑定"与措辞修正）
> 实施备注：① **P0-80**：`MemoryResourceProvider::hasFilesystemFallback` + `inheritFilesystemFallback(resourceRoot)`（引擎侧实现，resourceRoot 空→pathPrefix；幂等）+ `bridge_setResourceProvider` 替换时继承（三后端同头文件、零 Binding 改动；`sample_loadlibrary` 独立 TU stub 同步）；② **P0-81**：`LayoutParser::adoptContext`（构造即绑定 `m_viewportTarget`；30 处构造点（含 Actor 图标/按钮动画）插入 + `parseControl` 尾单漏斗兜底；顶层裸 MenuBar 分支天然豁免）；③ 措辞修正已采纳（setContext 会触发 recreate——解析期即建/缓存目标设备与 provider）；④ 验证：**P0-80/81 探针**——多实例 i2 解析期无 "No resource provider"、动画 `prepared=1`（SVG 帧亦通）、替换无回退 memory provider 后 `hasFallback=1` 相对键可读 5072B；P0-75 探针复跑全过；⑤ 全量构建 0 错误、回归仅 4 项预存；⑥ 设计器可回收 `registerFonts` + `ResolvePreviewAnimationPaths`（绕过清零）；⑦ Q-4（SVG 静态探针环境）另立项

---

## 1. P0-80 确认：绑定内存 provider 替换后丢失文件系统回退

**根因链（源码核实）**：
- `createMemoryResourceProvider`（src/backend/BackendBridge.h:298-300）= `new MemoryResourceProvider`（**无 delegate/回退**）；
- `bridge_setResourceProvider`（:311-316）= 直接赋值 `ctx->resourceProvider = h`（**不继承任何回退**）；
- 设计器流程：`RegisterResource` → Binding `ensureMemoryProvider` → 上述两函数 → 替换 MainWindow 的 memory+fallback provider → 此后 `fonts/...` 等相对键全部 `not found`（Label 字体 ×71）✓ 与报告一致；
- P0-78 只改了 MainWindow 自建 provider，**未覆盖"用户替换 provider"路径**（本项即该场景遗漏）。

**修改方案**：
- `MemoryResourceProvider` 增 `bool hasFilesystemFallback() const`（= `m_impl->delegate != nullptr`）；
- `bridge_setResourceProvider`：赋值后
  `if (auto* mem = dynamic_cast<MemoryResourceProvider*>(ctx->resourceProvider); mem && !mem->hasFilesystemFallback()) mem->setFilesystemFallback(ctx->resourceRoot.empty() ? ConstDef::pathPrefix.string() : ctx->resourceRoot);`
  - 规则与 MainWindow 一致（resourceRoot 空 → pathPrefix）；
  - `setFilesystemFallback` 幂等（P0-78 设计：已有 delegate 不覆盖）→ 与 `mountPath` 懒建共存；
- Binding 路径经同一 C ABI bridge，自动覆盖（零 Binding 改动）；三个后端 bridge 同步（同一头文件）。
- 设计器可回收 `registerFonts` 绕过。

## 2. P0-81 确认：多实例 LoadLayout 解析期 provider 作用域

**复现探针（Temp/temp_p081_probe，本机当前构建）**：两实例 i1/i2，i2 `LoadLayout` 含
`resourceProviders:[{name:"test", path:"animations/bombBlock"}]` + 动画 `path:"provider:test/bombBlock.jsonc"`：
- 解析期：`LuotiAni::loadAniDesc: No resource provider`（**provider 为 null**——控件尚未挂树，父链根 context 为空，`ControlImpl::getResourceProvider`（src/ControlBase.cpp:611-623）返回 nullptr）；
- 挂树后延迟补读（`setRenderDevice` → `loadAniDesc(m_resourceId)`，LuotiAni.cpp:580-584）：**desc 成功**（`total-frames=60`，provider 经父链解析到 i2 ✓）；
- 与报告差异说明：本机纯实例场景延迟补读可成功；设计器实测延迟补读 `No resource provider`（第二/三实例 + Binding 替换 provider 的组合，P0-80 同源）——两条路径的共同根因都是**解析期控件未绑定目标实例上下文**，解析期即加载的动画描述无法命中目标实例 provider。

**修改方案（解析期上下文绑定）**：
- `LayoutParser` 已有 `m_viewportTarget`（LoadLayout 传入目标实例）；新增 `void adoptContext(const std::shared_ptr<Control>& ctrl)`：
  `if (m_viewportTarget && ctrl && ctrl->getContext() == nullptr) ctrl->setContext(m_viewportTarget);`
- 在各 `parseX` 的 `make_shared<...>` 构造后立即调用（约 20 处，机械；至少覆盖**解析期有资源加载**的控件：Animation / Label（font-resource/font-file）/ Actor（provider 图片）/ Button（动画/状态图））；
- 效果：解析期 `getResourceProvider()` 直接命中目标实例 provider（LoadLayout 的 `resourceProviders` 挂载**先于**控件解析，LayoutParser.cpp:1047-1061）→ 动画描述解析期即成功；挂树后延迟补读自然同源；多实例互不串扰；
- 时机安全：与 C ABI `CreateXxx` 路径一致（构造即带上下文——CABI 以 `instance->bench` 为父）；`setContext` 会在目标上下文触发 `recreate()`（目标上下文非空时 create 实际执行——正是所需：解析期即建/即缓存目标 renderDevice/provider），两阶段守卫不变（评审措辞修正）；
- `m_resourceProvider` 缓存：绑定后首次解析即缓存**目标** provider，无需清理逻辑。
- 设计器可回收 `ResolvePreviewAnimationPaths` 绕过（编辑态解析同步受益）。

## 3. 附带观察（非本批范围）
- 探针中 `marker.svg` 帧加载：provider `readFile` 成功（5869B）但 `Surface::loadFromMemory/loadFromFile` 均返回 null（静态链接探针环境；设计器插件构建中 SVG 帧正常、PNG 经 provider 帧正常——P0-79 探针已证）。属静态探针环境/SVG 解码路径差异，**不影响本批结论**；如需跨环境 SVG 内存解码保障，另立项核查（Q-4）。

## 4. 测试计划
- **80**：CABI 探针——`createMemoryResourceProvider`（不设回退）→ `setResourceProvider` → 断言相对键资源（如 `CreateImage("images/cross_up.png")` 经 provider 或 Label `font-file`）可读；已挂载目录/内存注册优先不回归；
- **81**：两实例探针——i1 无 mounts、i2 有 mounts；i2 `LoadLayout` 动画 `provider:test/bombBlock.jsonc`（PNG 帧版本避免 SVG 环境差异）→ 断言解析期即无 "No resource provider"、`total-frames>0`（无需延迟补读）、i1 provider 不受影响；多实例顺序/交错加载回归；
- 全量回归（仅 4 项预存）+ 设计器联测（回收两项绕过后复测预览 provider 图片/动画/字体）。

## 5. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | P0-80 回退基准：`ctx->resourceRoot` 空 → `pathPrefix`（与 MainWindow 同规则） | 确认 |
| Q-2 | P0-81 绑定范围：全部 parseX 构造点 vs 仅资源相关控件 | **全部**（一致性；机械调用同一 helper） |
| Q-3 | 构造后立即 `setContext` 的时机安全（两阶段） | 确认（与 CABI 构造路径等价） |
| Q-4 | SVG 帧内存/文件解码的静态探针环境差异是否另立项 | 另立项（不阻塞本批） |
