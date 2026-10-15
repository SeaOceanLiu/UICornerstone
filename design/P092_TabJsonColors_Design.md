# P092_TabJsonColors_Design — tab-* 配色键的 JSON 解析（导出应用丢失页签配色）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-12 追加：P0-92）
> 前置：P0-89/90/91 批已实施并同步（`tab-*` 运行时键齐备）
> 状态：**已放行，已实施**（2026-10-12；Q-1~Q-3 确认；补充 1~3 全部落实）
> 复核：`requirements/P092_复核意见.md`（通过；补充 1：schema 增键同时是设计器保存/导出透传前提——本批为 tab 配色**全链持久化**完整修复）
> 实施备注：① `parseTabControl` 增 **5 键**（字符串 → setColorProperty）+ `selected-text`（字符串）+ `page-background`（字符串=normal / 四态对象，**合并语义**：读当前 getStateColorProperty → 仅覆盖显式态 → setStateColorProperty 转发所有页）；② schema `tab-control` 增 6 键（x-default 对齐常量：tab-background #25252AFF 等；page-background oneOf color/state-color-4）；③ 样例 `all_controls.json` tab-control 补 5 键（strict 覆盖）；④ 用户手册：declarative-syntax tab-control 注 + tabcontrol.html JSON 行；⑤ 验证：**P0-92 探针**——92a 首载 7 键读回 ✓；92b 重载往返 ✓；92c 合并语义（先设 normal(10,20,30) → 仅给 hover 重载 → normal 保持 + hover=0x99AABB）✓；strict PASS；全量构建 0 错误、回归仅 4 项预存；⑥ 设计器可恢复 tab 配色保存/导出透传（schema 白名单）并补 save/open 往返断言

---

## 1. 问题确认（源码核实）

| 环节 | 事实 | 位置 |
|---|---|---|
| ① 解析缺口 | `parseTabControl` 仅 `applyCommonColors + parseCommonProperties + font-size/position/tabs/currentIndex` 等；**无任何 tab-* / selected-text / page-background 颜色 JSON 解析** → 导出应用 layout.json 中页签配色不被应用（设计器自身经 CABI 读写 ✓、导出工程 ✗） | src/LayoutParser.cpp:782-880 |
| ② schema 缺口 | `tab-control` def 无 5 键（strict 校验将拒绝含键的导出布局） | docs/schema/declarative-ui.schema.json `$defs/tab-control` |
| ③ 先例 | 控件级颜色键按类型**显式解析**（如 ListView 表头 `kHeaderText/kHeaderBackground/kHeaderShadow` → `setColorProperty(key, parseColor(v))`）——无通用扫描，故需按类型补 | LayoutParser.cpp:673-679 |
| ④ 文档/源码不一致（附带） | declarative-syntax 视觉矩阵注③称 "`page-background` 转发至所有页 panel（页显式自设优先）"——**JSON 解析实际缺失**（该键仅运行时可经 SetColor 设置） | docs/appendix/declarative-syntax.html:315 |

## 2. 修改方案

### 2.1 必做：`parseTabControl` 补 5 键（字符串色）
```cpp
// P0-92：页签条配色（与 ListView 表头同款；#RRGGBB[AA] 字符串）
static const char* kTabColorKeys[] = {
    PropertyNames::kTabBackground, PropertyNames::kTabHoverBackground,
    PropertyNames::kTabSelectedBackground, PropertyNames::kTabIndicatorColor,
    PropertyNames::kTabContentBackground };
for (const char* key : kTabColorKeys) {
    if (j.contains(key) && j[key].is_string()) {
        tc->setColorProperty(key, parseColor(j[key]));
    }
}
```

### 2.2 （Q-1，建议一并）`selected-text` / `page-background` JSON 解析
- `selected-text`（字符串）→ `setColorProperty(kSelectedText, parseColor(v))`；
- `page-background`（字符串=normal / 四态对象）→ **合并语义**（P0-74④ 口径）：读当前 `getStateColorProperty(kPageBackground)` → 仅覆盖显式态 → `setStateColorProperty`（TabControl 覆写已实现"转发所有页"）；字符串=normal；
- 依据：修复 ④ 的文档/源码不一致；两键与 tab-* 同族、成本低；设计器可随之暴露行。

### 2.3 schema / 样例 / 文档
- schema `tab-control` def：增 5 键（+ Q-1 两键）`$ref: color` / `state-color-4`（page-background），`x-default` 对齐常量默认值；
- 样例：`layouts/all_controls.json` 的 tab-control 补 2~3 键（ctest strict 覆盖；无需重配 tools）；
- 用户手册：`declarative-syntax.html` tab-control 示例/注 + `controls/tabcontrol.html` JSON 段注明可声明键（运行时键已在册）。

## 3. 测试计划
- 探针：LoadLayout（5 键字符串 + Q-1 两键）→ `GetColor` 逐键读回（含 page-background 四态对象显式态/未给态保持）；导出往返冒烟；
- ctest strict（样例扩展）+ 全量回归（仅 4 项预存）。

## 4. 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 是否同批补 `selected-text`（字符串）与 `page-background`（字符串/四态对象，合并语义） | **建议一并**（修复文档/源码不一致，成本低） |
| Q-2 | schema/样例范围：5 键（+Q-1 两键）入 schema；all_controls 样例补键 | 确认 |
| Q-3 | 值形态：字符串 `#RRGGBB[AA]`（与 header 先例一致；page-background 另接受四态对象） | 确认 |
