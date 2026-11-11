# CR20261109003 — ContextMenu JSON items / events.onClick（分析 + 设计）

> 状态：**已关闭（2026-11-11；设计器联调通过）**——独立 context-menu items/onClick；复用 addItem 自动关闭；无新增 API
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261109003）
> 关联：CR20261109002（StatusBar 段 onClick）、MenuBar 菜单项 events、P0-45 ContextMenu

## 1. 问题与根因

`ContextMenu` 已有完整运行期能力：

- C ABI `CreateContextMenu/ContextMenuAddItemEx/AddSeparator/Show/Close`；
- `ContextMenu::addItem(shared_ptr<MenuItem>)` 会包装 `onClick`，执行用户回调后自动关闭；
- 控件级 `context-menu` 键通过 `populateMenuPanel` 解析 items。

但独立类型 `type:"context-menu"` 的 `LayoutParser::parseContextMenu` 只创建隐藏浮层，**不读取 `items[]`**；schema 的独立 `context-menu` 定义也没有 items。设计器无法把独立右键菜单的条目写入布局，也无法在 LoadLayout 时自动绑定条目事件。

## 2. 目标与范围

- 独立 `context-menu` 支持声明式 `items[]`：
  - `caption`、`shortcut`、`type:"separator"`；
  - `events.onClick`（action 名），并兼容直写 `items[].onClick`，`events.onClick` 优先；
  - V1 不支持嵌套子菜单、icon/leadingControl/逐项样式，遇到时诊断并降级；
- 复用 `ContextMenu::addItem/addSeparator`，点击 action 后再自动关闭（现有语义）；
- 不新增 C ABI/Binding API；
- 不改变控件级 `context-menu` 键的既有解析路径（本次仅独立类型）；
- 无 items 的旧 ContextMenu 行为不变。

## 3. 架构选择

### 方案 A（采用）：独立 parseContextMenu 增量解析

在 `parseContextMenu` 内增加 `items` 循环，使用现有 `ContextMenu` 方法：

- 普通项：构造 MenuItem，设置 caption/shortcut/onClick，再 `cm->addItem(item)`；
- separator：`cm->addSeparator()`；
- handler 从 `LayoutParser::m_handlers` 查表；缺 handler 只诊断；
- `ContextMenu::addItem` 现有包装负责“执行回调后关闭”。

优点：改动局部、不碰控件级路径、复用经过测试的关闭/面板逻辑。

### 方案 B（暂不采用）：抽象通用 MenuItem JSON 构建器

把 MenuBar、控件级 ContextMenu、独立 ContextMenu、StatusBar 的 items 统一为 builder。长期可减少重复，但会触碰多条路径和生命周期，风险大于本 CR；后续有更多结构化菜单需求时再重构。

**定稿：方案 A。**

## 4. JSON 契约

```json
{
  "type": "context-menu",
  "id": "ctxMain",
  "items": [
    { "caption": "复制", "shortcut": "Ctrl+C", "events": { "onClick": "onCtxCopy" } },
    { "type": "separator" },
    { "caption": "删除", "events": { "onClick": "onCtxDelete" } }
  ],
  "close-on-esc": true,
  "close-on-click-outside": true
}
```

V1 规则：

- `items` 必须是数组；非数组时忽略并 warning，保持旧隐藏菜单；
- 普通项必须有非空 `caption`（空 caption 忽略并 warning）；
- `type:"separator"` 只生成分隔线；separator 上的 caption/events 忽略；
- `shortcut` 可选字符串，设置到 MenuItem；
- `events.onClick` 或 `items[].onClick` 可选字符串 action；`events.onClick` 优先，二者同时存在时 warning 一次；不支持其它事件名；
- 不识别 `items[].items`（嵌套子菜单）：V1 每个 ContextMenu **只输出一次** warning，并将父项按普通叶子项创建，避免整个菜单缺项；
- 不识别 icon/leadingControl/逐项颜色/字体：忽略并保留默认 MenuItem 样式；不静默改变已有菜单项；
- 连续 separator/末尾 separator 按现有 MenuPanel 行为处理，不做额外合并；
- `visible` 仍为 false，菜单由 `ContextMenuShow`/控件绑定弹出。

## 5. 解析与绑定设计

在 `parseContextMenu` 中，chrome/flags 解析后、`parseEvents(cm,j)` 之前：

```text
for (itemJson : j.items):
    if type == "separator":
        cm->addSeparator(); continue
    if itemJson.items 存在:
        warn("context-menu nested items unsupported; treated as leaf")
    item = make_shared<MenuItem>(nullptr, Normal, xScale, yScale)
    item->setCaption(caption)
    if shortcut: item->setShortcut(shortcut)
    action = events.onClick（string，优先） ?: item.onClick（string）
    if action 非空:
        handler = m_handlers.find(action)
        if handler == end: logMissingHandler(action, 控件 id)
        else:
            item->setOnClick([handler](shared_ptr<MenuItem>){
                try { handler(nullptr); }
                catch (const std::exception& e) { logError("context-menu handler exception: " + e.what()); }
                catch (...) { logError("context-menu handler exception: unknown"); }
            })
    cm->addItem(item)       // 包装 onClick：先执行 handler（异常已捕获），再关闭菜单
```

要点：

- 先设置 item 的 `onClick`，再交给 `ContextMenu::addItem`；`addItem` 会保存原回调并在之后关闭，不会丢失 handler。
- 缺 handler 时不设置 onClick；`ContextMenu::addItem` 仍安装“仅关闭”回调，点击菜单项正常关闭。
- handler 捕获副本，避免 parser 生命周期问题；传 `nullptr` 与 Menu/StatusBar 现有 action 约定一致。
- **B1 异常保护**：parser 注入的 lambda 内捕获 `std::exception`/其它异常，`logError` 记录；异常不阻止 `ContextMenu::addItem` 的后续 close，避免菜单状态残留。宿主 C ABI callback 路径不改变异常边界。
- 已注册 handler 点击只执行一次：MenuItem Normal 的 `m_onClick` 触发一次，`ContextMenu::addItem` 只包装同一回调，不额外重复调用。
- `parseEvents(cm, j)` 仍处理控件级事件，不受 items 解析影响。
- `cm->create()` 保持原有位置；items 可在 create 前加入（MenuPanel 延迟创建）。
- 不改变 `ContextMenuAddItemEx` / `ContextMenuAddItem` 行为；C ABI 创建的菜单仍由宿主 `SetCallback` 管理。

## 6. 自动关闭与生命周期

- `ContextMenu::addItem(shared_ptr<MenuItem>)` 已包装 `onClick`：先执行原回调，再 `close()`；本设计直接复用它，不重复实现关闭。
- `MenuItem` 点击回调由引擎延迟一帧执行（P0-103）；按键/点击探针需在下一帧 `ProcessEvents` 后检查。
- 菜单面板由 ContextMenu 持有；item 随 ContextMenu 销毁，无额外生命周期 API。
- 嵌套子菜单不支持，避免子项回调不关闭的语义缺口。

## 7. Schema/文档

- `docs/schema/declarative-ui.schema.json` 的独立 `$defs/context-menu` 增加 `items`：
  - 数组；元素仅允许 `caption`（string）、`shortcut`（string）、`type`（仅 `separator`）、`events`（object，`onClick` string）、`onClick`（string）；
  - `additionalProperties: false`，防止 V1 未支持的 icon/leadingControl 被 schema 误放行；
  - 普通项需有 caption；separator 用 `type:"separator"`；`onClick`/`events.onClick` 不强制互斥（schema 均放行），运行期 events 优先并 warning 一次；
- 控件级 `context-menu` description 补 `items[].events.onClick`/`items[].onClick`；
- `docs/controls/contextmenu.html` 的 4.7.4 独立声明示例加入 items + `events.onClick`，明确 `onClick` 兼容别名，移除“items 随后续批次补”描述；
- `docs/appendix/declarative-syntax.html` 的 context-menu 示例补事件绑定；
- 用户手册“事件/菜单”章节说明：ContextMenu 项事件与 MenuBar 同构，宿主 `RegisterAction` 后生效；不承诺预览执行业务 handler。

## 8. 测试/验收

### 8.1 自动化

1. 独立 ContextMenu `items`：2 个普通项 + 1 个 separator，加载后 MenuPanel 项数/顺序正确；
2. 注册 handler，模拟点击普通项，handler 调用一次且菜单关闭；separator 不可点击；
3. 缺 handler：加载/点击不崩溃，日志有 `handler not registered`，点击仍关闭；
4. 空 `items`/无 `items`：旧 ContextMenu 行为不变；
5. 嵌套 `items`：只产生一次 warning、菜单不崩溃、父项存在且可点击关闭（按 V1 叶子降级）；
6. `events.onClick` 与 `onClick` 同时存在：events 优先、warning 一次；
7. 已注册 handler 点击：handler 恰好一次、菜单关闭；缺 handler：菜单关闭、无崩溃；
8. handler 抛异常（测试注入）：异常被记录，菜单仍关闭、无残留；
9. 控件级 `context-menu` 键回归：原有 items/事件/图标行为不变；
10. 布局 JSON `validate_layout --strict` 通过；
11. 运行 `test_contextmenu`、`test_contextmenu_cabi`、`test_layout`、`test_cr_theme`。

### 8.2 手工

- 右键弹出独立菜单、多分隔线、快捷键、中文/空 caption；
- 缺 handler 与已注册 handler 的差异；
- 菜单点击后关闭、外部点击/Esc 关闭不回归；
- 设计器事件树/导出任务的后续联调（本 CR 只负责引擎 JSON 解析）。

## 9. 兼容/风险

| 风险 | 处理 |
|---|---|
| 嵌套子菜单需求 | V1 明确不支持并诊断；后续按 ContextMenu 能力单独扩展 |
| 逐项 icon/样式 | V1 忽略；菜单默认样式不变，后续结构化编辑再扩展 |
| action 与 C ABI 回调重复 | 文档说明：二者可并存，宿主自行避免重复业务 |
| 旧 `context-menu` 控件级路径 | 不修改，避免回归 |
| `addItem` 自动关闭与 handler 顺序/异常 | 复用现有包装；parser lambda 捕获异常并记录，确保 close 仍执行；探针按延迟一帧检查 |

## 10. 实施顺序

1. parseContextMenu items + handler 查找 + 诊断；
2. schema/用户手册；
3. 自动化布局与点击探针；
4. 全量回归、Release 同步、设计器联调（事件树/导出）。

## 11. 待设计器确认

1. V1 嵌套子菜单：设计器已接受“父项降级为叶子 + 单次 warning”。
2. 独立 ContextMenu 同时兼容 `events.onClick` 与直写 `onClick`（events 优先），与 StatusBar 保持一致。
3. 逐项 icon/leadingControl：设计器已接受不纳入 V1。

## 修订记录

- 2026-11-09 初稿：独立 `context-menu` items 复用 ContextMenu 现有方法；V1 caption/shortcut/separator/events.onClick；不新增 C ABI；待评审。
- 2026-11-09 v1（按评审 B1~B4）：handler 单次/异常保护与关闭测试；兼容 `items[].onClick`（events 优先）；嵌套 warning 单次；schema 收紧为 caption/shortcut/type/events；控件级路径不变。

## 12. 实施记录（2026-11-09）

- `parseContextMenu` 已解析独立 `items[]`：caption/shortcut/separator、`events.onClick`（优先）与兼容 `onClick`；普通项通过 `ContextMenu::addItem` 保留“执行后关闭”语义。
- 缺 handler/空 caption/嵌套 items 诊断；嵌套父项按叶子降级，warning 每个菜单最多一次；handler 异常在 parser 注入 lambda 内捕获，确保 close 仍执行。
- 控件级 `context-menu` 路径未改动；无新增 C ABI/Binding。
- 新增 `test_cr_menu_item_events`：覆盖 items 顺序/separator、events/旧 onClick/双键优先级、缺 handler、嵌套降级、handler 异常；全量回归 **57 OK + 4 项预存失败**。
- schema/contextmenu 手册/declarative-syntax 已更新；`validate_layout --strict` 测试布局通过。
- Release 已同步，DLL SHA256 `dab958f11a514c3820a98e87f12ddb2ca0f3f1c3ea491d0008d79ad97d2c1826`；schema strict 测试布局通过。
- 联调（2026-11-11）：设计器侧 closed；Release dab958f1...；CR 设计器/引擎均 closed。
