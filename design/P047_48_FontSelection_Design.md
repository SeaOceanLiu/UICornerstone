# P047_48_FontSelection_Design — 设计器第三轮补充问题确认与修改方案（P0-47 / P0-48 系列）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-09-30 增补：P0-47 / P0-48 / P0-48 扩展 / P0-47 扩展）
> 前置：P0-38~46 批已实施并同步
> 状态：**已放行，已实施**（2026-09-28）
> 复核：`requirements/P047_48_FontSelection_Design_复核意见.md`（通过，放行实施；§4 五问全部确认；P0-48 扩展① 设计器正式关闭——用户实测可见字体变化）
> 实施备注：① Slider kFont 读回（与 label-font 等价）；② `setSelectedIndex` 变更守卫 + 同口径触发（C++ handler + fireCCallback；同值不触发）；③ 新增 `UICornerstone_GetFontCount/GetFontName`（索引按 FontName 枚举序，单一数据源）+ Binding 静态封装（`GetFontCount/GetFontName/GetFontNames`，未加载安全降级）；④ 测试落位：`test_property_cabi`（字体清单 6 项/越界、Slider 读回、selected-index 触发+守卫）+ `test_combobox_cabi`（直接 SetCallback + SetInt 触发/守卫）；⑤ 回归仅 4 项预存失败，无新增

---

## 1. 问题确认（源码核实 + C ABI/Binding 双通道复现探针）

| # | 报告 | 复现结论 | 根因/证据 |
|---|---|---|---|
| P0-47① | Slider：getEnumProperty(kFont)/getStringProperty(kFont) 读回缺失 | **确认（已复现）** | `Slider::getEnumProperty` 仅处理 `label-font`，无 `kFont` 分支 → GetEnum/GetString("font") 返回 0；写入通道已通（`setEnumProperty` 的 kFont 已别名到 label 字体，实测 Set 后 `label-font` 读回 = 所设值）。探针：`GetEnum(font) before rc=0`；Set 后 `GetEnum(label-font)='muyao-softbrush'` |
| P0-47② | WinFrame：getStringProperty(kFont) 返回 title 的 caption 内容 | **未复现（C + Binding 双通道均正确）** | C ABI：初始 `GetString(font)='harmonyos-sans-sc-regular'`；Set 后 `='muyao-softbrush'`；`GetString(title)` 独立无串扰。Binding：同结果，title 再设后 font 读回不受影响。代码核实：`WinFrame::getStringProperty` 的 `kTitle` 分支（"title"）与 `kFont`（"font"）无键位重叠，`getEnumProperty(kFont)` 返回 `m_fontName`（成员有确定缺省）。**请设计器提供复现用例**（控件句柄来源/调用序列/键常量/DLL 版本）——疑设计器侧面板键位错配（如把 title/caption 的读取用于字体槽） |
| P0-48 | ComboBox selection-changed 经 Binding SetCallback 未触发 | **部分确认**：交互通道正常，**程序化 selected-index 写入不触发** | 交互（下拉点选）：Binding SetCallback 实测 7 次全部触发（idx/val 正确），C 通道同（test_combobox_cabi 既有注入用例亦通过）。程序化 `SetInt("selected-index")`：**fired=0** —— `ComboBox::setSelectedIndex`（:703）只更新索引/文本，**不触发事件**；`selectItem`（:518）触发（C++ handler + `fireCCallback(Selection)`）。Binding 桥（CallbackThunk/Event 载荷）实测正常，非"桥接问题" |
| P0-48 扩展① | font 设置后控件文字字体不变 | **未复现（像素级验证字体切换生效）** | 探针：Button（默认 vs `muyao-softbrush`）视觉差异 **519px**；Label（默认 vs `asul-bold`）差异 **595px**。设置链核查：`SetString("font")`→基类别名→控件 `setEnumProperty(kFont)`→Font 对象→caption Label 更新→重绘，探针实测通过。**支持矩阵**见 §2.4——全部有文字控件均支持 kFont（Slider 读回缺失见 P0-47①，写入可用）。**请提供复现用例**（控件类型/字体名/写回路径）；另：字体名不匹配时 `FontNameFromString` 静默回退 HarmonyOS 常体（可作为设计器侧诊断点） |
| P0-47 扩展 | kFont 未设置时读回实际生效的缺省字体名 | **部分确认**：Slider 返回空即其根因；其余文字控件实测均返回有效缺省 | 探针（未显式设置）：button/check-box/label/progress-bar/combo-box/edit-box 均 `rc=1 'harmonyos-sans-sc-regular'`；Slider `rc=0`（同 P0-47①）。修复 Slider 后全矩阵覆盖（§2.4）。"实际生效"以控件字体成员为准（Font 对象无名字反射；继承路径的边界在 §4 说明） |
| P0-48 扩展② | 合法字体枚举名清单/查询 API | **确认缺口** | 引擎枚举 6 项（`kFontAsulBold` 等，PropertyNames.h:369-374）与 schema `$defs/font-name`（6 项）**一致**；但无运行时查询 API。设计器 ComboBox 选项需与其对齐，建议新增清单 API（§2.3） |

## 2. 修改方案

### 2.1 P0-47① + P0-47 扩展：Slider kFont 读回
- `Slider::getEnumProperty`：新增 `kFont` 分支（与既有 `kLabelFont` 等价返回 `FontNameToString(m_labelFont)`）；
- 效果：`GetString/GetEnum("font")` 未设置时返回有效缺省（label 字体默认值），设置后返回所设值 —— 与其它文字控件对齐。

### 2.2 P0-48：selected-index 程序化写入触发对齐
- `ComboBox::setSelectedIndex`：增加**变更检测**（`bool changed = (index != m_selectedIndex)`），变更时与 `selectItem` 同口径触发（`m_onSelectionChanged` + `fireCCallback(kEventSelectionChanged, Selection)`）；
- 安全边界：解析期（JSON `selected-index`/`value`）发生在事件回调注册之前 → 天然不触发；`setSelectedValue` 复用同路径（语义一致）；
- 测试用例：`test_combobox_cabi` 增补直接 `SetCallback("selection-changed") + SetInt("selected-index")` 断言（用例号随实现排定）。

### 2.3 P0-48 扩展②：字体枚举清单 API
- C ABI（静态，与实例无关；索引按引擎枚举顺序）：
  ```
  int UICornerstone_GetFontCount(void);                       // = 6
  int UICornerstone_GetFontName(int index, char* out, int maxLen);  // 越界返回 0
  ```
  实现取 `ConstDef` 字体表（`FontNameToString`），单一数据源；
- Binding：`static int GetFontCount()` / `static std::string GetFontName(int)`（+ 便捷 `std::vector<std::string> GetFontNames()`）；
- 文档：CABI 速查表 + properties/binding 速查表 + 声明式语法字体枚举引用一行；
- schema `$defs/font-name` 已与引擎一致（6 项），无需改动，作为声明式镜像。

### 2.4 字体读回/视觉支持矩阵（引擎复测结论；本批仅补 Slider 读回）
| 控件 | set | get（缺省回读） | 字体切换视觉 |
|---|---|---|---|
| label / button / check-box / edit-box / combo-box / numeric-up-down / text-area | ✓ | ✓ | ✓（像素实测） |
| progress-bar / slider | ✓ | progress ✓ / **slider ✗（本批修复）** | ✓ |
| status-bar / tab-control / tree-view / list-view（控件级） | ✓ | ✓ | ✓ |
| menu-bar / menu-item / win-frame / color-picker（闭合态） | ✓ | ✓ | ✓ |
| scroll-bar / splitter / panel / shape / image / animation | —（无文字） | — | — |

### 2.5 P0-47② / P0-48 扩展①：待复现（不建议无问题改动）
- 两项在 C ABI + Binding + 像素级复测中均正常；**本批不实施对应代码改动**，改为向设计器请求复现信息（§4-③）：
  - P0-47②：控件句柄来源（是否 `FindControl` 的 win-frame 本体 / 还是 title-label）、读取键常量、DLL 版本；
  - P0-48 扩展①：控件类型、字体名原文、写回调用序列（SetString/SetEnum 通道）、是否含主题字体覆盖。

## 3. 验收

| # | 验收 | 方式 |
|---|---|---|
| 1 | Slider `GetString/GetEnum("font")` 未设置返回缺省、设置后读回一致 | 读写回探针 |
| 2 | `SetInt("selected-index")` 变更触发回调（含 idx/val）；同值不重复触发 | C ABI 测试用例 |
| 3 | `GetFontCount/GetFontName` 返回 6 项且与 `PropertyNames` kFont* 一致 | 新增测试 + tools 校验 |
| 4 | 字体读回矩阵全绿（§2.4），像素级字体切换（Button/Label）保持 | 探针复扫 |
| 5 | 全量回归（仅既有 4 项预存失败） | 回归扫描 |

## 4. 待审核问题

1. **P0-48 触发策略**：程序化 `setSelectedIndex` 变更即触发（与 `selectItem` 对齐；解析期无回调注册天然不触发）——确认？
2. **P0-48 同值写入**：同值 `selected-index` 写入不触发（变更守卫）——确认？
3. **P0-47② / P0-48 扩展①**：请设计器提供复现用例（见 §2.5 清单）；若确认系设计器侧键位/名称错配，则本批两项关闭（不实施引擎改动）——确认？
4. **字体清单 API 形态**：`GetFontCount/GetFontName(index)`（+Binding 封装）还是单函数 JSON 数组（`GetFontNames`）？建议前者（零解析）。
5. **P0-47 扩展语义边界**：读回取控件"字体成员"（显式设置或控件缺省）；父链字体继承的显示名不参与（Font 对象无名字反射）——确认可接受？

## 5. 配套改动

- 代码：`src/Slider.cpp`（getEnumProperty）；`src/ComboBox.cpp`（setSelectedIndex 变更触发）；`include/UICornerstoneAPI.h` + `src/UICornerstoneAPI.cpp`（字体清单 API）；`binding/include/UICornerstone.h` + `src`（Binding 封装）；
- 测试：`test/test_combobox_cabi.cpp`（selected-index 触发用例）；字体清单 API 测试；探针复扫；
- 文档：CABI 速查表 / Binding 速查表 / properties 字体段（清单 API 一行）；设计文档标注 + 复核归档 + make_release + 同步。
