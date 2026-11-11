# CornerstoneDesigner《08_主题系统设计》引擎复核

> 复核方：UICornerstone 引擎
> 日期：2026-11-05
> 对象：`D:\GitSpace\CornerstoneDesigner\design\08_主题系统设计.md`（v1.0，2026-11-04）

## 结论

**有条件通过。** 设计边界清晰，项目主题/设计器 UI 主题/运行序列化拆分合理；以下阻塞项解决后可进入完整联测。

## 阻塞项

1. **§10 验证数据已失效**：当前设计器 Debug `auto=2` 在 `UICornerstone_SetHandleTarget` 稳定崩溃，说明 v1.0 的“auto=2 272/0”不能作为 as-built 证据。引擎已定位并修复 `HandleControl` 父容器悬垂访问及 stale handle 校验（见引擎回归），需重新发布、更新子模块指针后由设计器复测，再刷新本节数据。
2. **子模块指针未更新**：设计器父仓 `subModules/UICornerstone` 仍指向 `c1de0d1`，本地 DLL 只是副本；发布包已同步到子模块工作区，但父仓指针/提交未更新，导致“子模块包含 CR20261102001/002/003”在版本层面不可复现。需设计器更新指针并重新验证 `auto=2`/`auto=3`/`strict`。
3. **引擎与文档的能力口径需对齐**：
   - 变体 `variants.<name>.variables`、媒体 `min-scale/max-scale`、media/容器选择器权重已在引擎侧修复/补齐，但当前设计器使用的发布 DLL 尚未包含这些最终修复；联测必须使用重新发布后的 DLL。
   - 在旧版本引擎上，变体变量会被忽略、scale 条件不参与匹配；设计器 UI 应能识别版本能力，避免静默不生效。

## 需要修订/补充

1. **§4.2 unknown 字段**：设计称未知字段由引擎诊断；当前引擎对未知 query 键/未知声明多为忽略，不能保证有诊断。请改为“设计器管理器校验；引擎对未知字段忽略并保持兼容”，或补引擎诊断。
2. **§4.3 `:selected`**：引擎四态为 normal/hover/pressed/disabled，`:selected` 目前近似写入 pressed 通道，并非独立选中态。设计文档应明确，避免 UI 承诺独立 selected 外观。
3. **§4.4 选择器权重**：引擎现已按“选择器 specificity 优先，同权重按层序（基础→媒体→容器→变体）和 source order”合并；请在设计器 UI 中提示媒体规则需保持与目标规则相同的 selector 权重，否则 `button` 不能覆盖 `button.primary`。
4. **§4.2 content-padding 矩阵**：引擎已接入通用内容和主要容器/文本控件；ListView/TreeView 的滚动内容区、部分自绘控件仍不应在 UI 中承诺完整可用。建议在设计器属性面板按控件能力显隐，或补引擎矩阵测试后再放开。
5. **§6.4 查询字段**：设计器校验包含 `min-scale/max-scale`，请与最终引擎版本联测；引擎 API `RefreshResponsiveTheme` 以 Bench 当前缩放作为 scale 口径，设计器预览/画布缩放需明确传入/使用的口径。
6. **§8 设计器 UI 主题**：`ApplyTheme` 只作用于已存在控件；设计器动态创建的属性行/浮层若在应用后创建，不会自动继承主题。请在启动骨架加载后再应用，或补动态控件主题刷新。
7. **§11 重复条件 ID**：重复 ID 会污染 `GetActiveMedia` 字符串匹配；建议管理器禁止重复，或引擎返回值改为结构化列表。

## 已确认一致

- 项目主题/设计器 UI 主题隔离、`.csd` 保存/运行序列化分离、模板加载失败不替换旧主题。
- `styles` 选择器子集、变量、变体、媒体/容器结构、`box-shadow` 对象、`content-padding`/Dialog 旧 `padding` 语义。
- 设计器默认颜色剥离只影响运行序列化、不影响设计画布/撤销，符合引擎显式覆盖策略。
- `ApplyTheme`/`SetThemeVariant`/`GetActiveMedia`/`RefreshResponsiveTheme` 接口边界与设计器职责划分一致。

## 建议

先完成引擎最终 Release 重建并把修复后的子模块内容提交/更新指针，再按“Debug auto=2 / auto=3 / Release / strict / 主题模板 / 媒体+容器 / 显式颜色保留”顺序整链复测；通过后把 §10 的实测数据和子模块版本写入本设计。

## v1.1 复核（2026-11-05）

复核结论：**通过（版本门禁仍有效）**。

已确认修订：
- §4.2 unknown 字段改为设计器校验、引擎忽略/不保证诊断；
- §4.3 明确 `:selected` 近似 pressed；
- §4.4 明确 specificity 优先、同权重按基础→媒体→容器→变体层序；
- §4.2/§12 标明 `content-padding` 在 ListView/TreeView/自绘控件的能力边界；
- §6.3/§12 管理重复条件 ID；
- §6.4 明确 scale 以 Bench 当前缩放为口径；
- §8 明确 `ApplyTheme` 只作用于已存在控件，动态区域列为后续限制；
- §10 增加版本门禁，旧 DLL 不作为验收依据。

引擎侧对应修复已落地并纳入最终 Release：变体变量、variant 内媒体/容器、`min-scale/max-scale`、media/容器保持 selector specificity、HandleControl stale parent 修复。

发布复核：
- `release/UICornerstone.dll` 与设计器子模块 DLL SHA256 一致：`f2aa7e0d7a2e0330aad4a2daee9b17592312c5b92e4b19c82ed6fb9f7a0b26cd`
- 子模块父仓指针当前为 `dbef739866d3bb6e67008bff43e8773226a67d2e`，需设计器侧提交锁定。
- 设计器仍需用该最终 Release 复测 Debug/Release `auto=2`、`auto=3`、`strict`；本复核不替代其测试报告。
