# CR20261109002 — StatusBar 段 onClick handler 未绑定（分析 + 设计）

> 状态：**已关闭（2026-11-11；设计器联调通过）**——解析 events.onClick/旧 onClick，绑定 m_handlers；无新增 API
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261109002）
> 关联：MenuItem/StatusItem 事件、LayoutParser `registerHandler`、CR20261109003（ContextMenu items）

## 1. 问题与根因

`src/LayoutParser.cpp` 的 `parseStatusBar` 当前读取段级 `onClick`：

```cpp
if (ij.contains(PropertyNames::kJsonOnClick) && ij[PropertyNames::kJsonOnClick].is_string()) {
    string action = ij[PropertyNames::kJsonOnClick].get<string>();
    bar->setStatusItemOnClick(id, [action](shared_ptr<StatusItem>){});
}
```

`action` 被捕获但从未查 `m_handlers`，`StatusItem::onClick` 实际为空；用户点击无效果。`StatusBar::handleEvent` 对无 menu 的段仍会触发控件级 `status-item-click` C ABI 回调，但 JSON action 语义丢失；有 menu 的段只打开菜单，不触发该 C ABI，也不执行 JSON action。

这不是设计器可绕过的问题，必须修引擎 LayoutParser。

## 2. 目标与范围

- 解析段级 action 并在 `LayoutParser::m_handlers` 中查找已注册 handler；
- 找到后绑定到 `StatusBar::setStatusItemOnClick`；找不到时 `logMissingHandler` 诊断且不崩溃；
- 保持旧行为：无 action / 无 handler / 有 menu 的段不受影响；
- **不新增 C ABI/Binding/LayoutParser 公共 API**；
- 支持设计器事件页最终采用的 `items[].events.onClick`，并保留现有 `items[].onClick` 兼容；
- 不改变 `status-item-click` C ABI 事件的现有语义：仅无 menu 的段触发；有 menu 的段只打开菜单。

## 3. JSON 契约

V1 同时接受：

```json
{
  "type": "status-bar", "id": "sb",
  "items": [
    { "id": "branch", "text": "main",
      "events": { "onClick": "onStatusBranch" } },
    { "id": "enc", "text": "UTF-8", "onClick": "onStatusEnc" }
  ]
}
```

- `events.onClick`：与 MenuItem 一致的 action 键，推荐给设计器事件页；
- `onClick`：既有直写键，继续兼容；
- 优先级：`events.onClick` > `onClick`（同时存在时使用前者，可输出一次 warning）；
- action 必须是非空字符串；空字符串等同未设置；
- 不要求段 id 之外的新标识；段 id 仍用于 `setStatusItemOnClick` 定位；
- 有 `menu` 的段点击打开菜单，不触发段 action（保持现有 StatusBar 行为）。

## 4. 实现设计

在 `parseStatusBar` 的段循环末尾统一提取 action：

```text
action = events.onClick（string）
       ?: item.onClick（string）
if action 非空:
    handler = m_handlers.find(action)
    if handler == end:
        logMissingHandler(action, 段 id 或状态栏 id)
    else:
        bar->setStatusItemOnClick(id,
            [handler](shared_ptr<StatusItem>) { handler(nullptr); })
```

要点：

1. `handler` 类型为 `std::function<void(shared_ptr<Control>)>`；StatusItem 不是 Control，按 StatusBar/Menu 既有约定传 `nullptr`，不伪造 Control。
2. 先把 handler 复制到 lambda（`auto handler = it->second;`），不要捕获迭代器/`this`。
3. 缺少 handler 时**不设置空 lambda**；段点击仍触发 `status-item-click`，只是 JSON action 不执行。
4. `onClick` 与 `events.onClick` 的解析放在段样式/字体之后、循环结束前；不改变 `parseEvents(bar, j)` 对控件级事件的处理。
5. 保留现有 `id` 缺省回退（`id = text`）；绑定按最终 id 生效。
6. `events` 非对象/`onClick` 非字符串：忽略并保持旧行为；不新增错误抛出。

## 5. 兼容与边界

- 旧布局只有 `items[].onClick`，行为从“空回调”变为真正执行 action；这是本次 bug 修复目标；
- 旧布局没有 action：不设置回调，行为不变；
- 已注册同名 action 可被多个段/控件复用（与现有 handler 机制一致）；
- 段有 `menu`：菜单优先，action 不执行（现有语义）；
- 无 menu 段：`status-item-click` C ABI 回调与 JSON action 可同时存在；有 menu 段：只打开菜单，二者都不执行；宿主自行避免重复业务逻辑；
- 不修改 StatusBar/MenuItem C++ 类接口。

## 6. 测试与验收

### 6.1 自动化

1. 新增/扩展现有测试布局，包含左段/右段/separator/普通段，至少两个段分别使用 `events.onClick` 与 `onClick`；
2. 注册对应 handler，模拟点击段，断言 handler 恰好调用一次，参数为 `nullptr`；
3. 未注册 handler：加载不崩溃，日志含 `handler not registered`，点击不崩溃；
4. 有 `menu` 的段点击只打开菜单、不调用 action、不触发 `status-item-click`（C ABI 计数为 0）；
5. 无 menu 且 action 缺 handler：`status-item-click` 仍触发、不崩溃、日志有诊断；
6. `events.onClick` 与 `onClick` 同时存在：前者优先，warning 一次；
7. 旧布局无 action：点击行为/`status-item-click` 不回归；
8. `validate_layout --strict` 通过；
9. 回归 `test_statusbar`、`test_layout`、`test_cr_theme`。

### 6.2 手工/探针

- 多段/右侧段/分隔项点击；
- 同段同时配置 action 与 C ABI 回调，确认两者语义清晰；
- 缺 handler 与实际 handler 的日志差异。

## 7. 文档/Schema

- `docs/schema/declarative-ui.schema.json`：status-bar `items` 描述增加 `events.onClick`，并说明与 `onClick` 的优先级；
- `docs/controls/statusbar.html`：4.22.4 JSON 键表补 `items[].events.onClick`，保留 `items[].onClick`；
- `docs/appendix/declarative-syntax.html`：status-bar 属性行补 events；
- `docs/appendix/properties.html`：StatusBar 段数据说明补事件键；
- API Mapping 无新增 CABI/属性，无需新增映射；可在 CR 记录注明“仅修复解析”。

## 8. 风险

| 风险 | 处理 |
|---|---|
| 旧 `onClick` 语义变化导致意外执行业务 | 这是 bug 修复；在发布说明/CR 中明确 |
| action 与 `status-item-click` 双触发 | 文档说明；不自动去重，宿主可选择一种 |
| 段 id 重复 | 沿用 StatusBar 现有 id 定位行为，不扩展 |
| handler 生命周期 | 仅保存 `std::function` 副本，不捕获 parser 指针 |

## 9. 实施顺序

1. LayoutParser action 解析与 handler 查找；
2. schema/文档；
3. 自动化布局 + 点击测试；
4. 全量回归、Release 同步。

## 修订记录

- 2026-11-09 初稿：确认空 lambda 根因；支持 `events.onClick` 优先、兼容 `onClick`；复用 `m_handlers`，不新增 API。
- 2026-11-09 v1（按评审 B1/B2）：修正 `status-item-click` 语义（有 menu 段不触发）；补有 menu/缺 handler/双键优先级测试。

## 10. 实施记录（2026-11-09）

- `parseStatusBar` 段循环已读取 `events.onClick`（优先）与旧 `onClick`，查 `m_handlers`；缺 handler 使用 `logMissingHandler`，不安装空回调。
- 无 menu 段 action 与 `status-item-click` 可同时触发；有 menu 段只打开菜单，不执行 action/C ABI（与评审 B1 一致）。
- 新增 `test_cr_menu_item_events`：覆盖 events、旧 onClick、双键优先级、缺 handler、menu 段解析；全量回归 **57 OK + 4 项预存失败**。
- schema/statusbar 手册/declarative-syntax/properties 已补 `events.onClick` 与优先级说明。
- Release 已同步，DLL SHA256 `dab958f11a514c3820a98e87f12ddb2ca0f3f1c3ea491d0008d79ad97d2c1826`；schema strict 测试布局通过。
- 联调（2026-11-11）：设计器侧 closed；Release dab958f1...；CR 设计器/引擎均 closed。
