# UICornerstone 配合修改清单 —— 引擎回复（实施结果）

- 回复方：UICornerstone（引擎）
- 日期：2026-09-17
- 依据：`UICornerstone_配合修改清单_汇总.md`（2026-09-17）+ `UICornerstone_遗留问题清单_第七批.md`
- 状态：清单中全部有效项均已处理（实施/确认），本批次已完成回归验证并同步最新 release 至 CornerstoneDesigner

---

## P0 —— 影响功能正确性（4/4 已完成并验证）

### 1. Button `getBoolProperty` 缺失 ✅ 已实施

- **改动**：`src/Button.cpp` 新增 `Button::getBoolProperty` 特化，读回 `kTextShadowEnable → m_enableTextShadow`；`include/Button.h` 删除内联转发，改由 .cpp 特化覆盖。
- **验证**（`test_p0_getter.c`）：`UICornerstone_SetBool(..., "text-shadow-enable", 1)` → `GetBool` 返回 `val=1`。
- 顺带：全量扫描 22 类控件 set/get 差集（Bool/Float/String/Ptr 四类含基类兜底）——**无其他缺失键**，本项是唯一缺口（见 P1-8）。

### 2. TextShadow 缺省色纯白 ✅ 已实施

- **改动**：`src/ConstDef.cpp` 三态缺省色 `(255,255,255,255)` → `rgba(0,0,0,120)`（深色半透明）。常量用于 `ControlBase.h` 成员初始化，浅色控件下阴影可见。

### 3. schema `font-size` 对 button 无效 ✅ 已实施（方案 b）

- **改动**：`Button::setFloatProperty` 补 `kFontSize`（common 键）→ 转发 `setCaptionSize`。schema 无需裁剪（font-size 属 common 合法键）。
- **验证**：`SetFloat(..., "font-size", 22.0)` → `GetFloat(..., "caption-size")` 返回 `22.0`。
- 设计器侧可按类型 skip 逻辑回退删除（该键现对 button 生效）。

### 4. 编程式控件 FindControl 找不到 ✅ 已实施（新增 C ABI）

- **改动**：新增 `UICornerstone_SetControlId(instance, ctl, id)`（`src/UICornerstoneAPI.cpp` + `include/UICornerstoneAPI.h` 声明）。
  - 注册到实例 `controlsById`，随后 `UICornerstone_FindControl(instance, id)` 可查。
  - `id` 传空串 → 移除该控件名下全部注册（幂等）。
- **验证**：注册→FindControl 命中；空 id 移除→FindControl 返回空。
- 设计器自存句柄映射的 workaround 可回收，改经引擎查询。

---

## P1 —— 体验 / 代码质量（2/2）

### 7. LayoutParser kJson\* 双常量体系合并 ✅ 已完成（此前 P1 批次）

- 已合并 98 个重复 kJson → 属性名常量并删除定义（244 → 146）。146 保留项均为结构键（view/actors/menus/动画等），与 set/get 语义无关。

### 8. getter 全量补齐 ✅ 已确认无更多缺口

- 脚本扫描全部控件 `set*Property` / `get*Property` 常量差集（四类型 + 基类兜底）：除 P0-1（Button `getBoolProperty`，已补）外无缺失。
- 如设计器仍有"个别属性回填回落缺省"，请提供具体控件+键名，我们再核。

---

## P2 —— 文档 / 确认类（5/5）

| # | 项 | 回复 |
|---|---|---|
| 9 | schema 残留键 `luotiAni`/`providerName`/`resourceId` | **有意保留**。`luotiAni` 为 particles 专有名词键（`kLuotiAni = "luotiAni"`），`providerName`/`resourceId` 为资源结构键（非控件运行时属性）。与"schema 键名 == 运行时属性名"的规则不冲突——三者无对应运行时属性。 |
| 10 | 动画 jsonc 键名范围 | **独立体系，豁免迁移**。动画文件由 LuotiAni 解析器读取（`view`/`name`/`type`/`src`/`opacity`/`blend-mode` 等为 kJson 结构键），不属布局控件属性体系，不参与 kebab 迁移。 |
| 11 | 文档同步确认 | **已同步**。API_Mapping_Table 属性键列 50 处 kebab；7.3 字体枚举等 4 项改名；ViewportScale/手册 5.6 override 语义已核对。 |
| 12 | 测试补充确认 | **已加并全绿**。test_viewport_scale 含 SetInstanceScale → SetViewport → resize 后 scale 保持用例（T12）。 |
| 13 | CheckBox 无 click 事件 | **采纳文档化方案**（保持单事件语义）。`docs/controls/checkbox.html` 4.4.3 补注：CheckBox 不派发 `click`/`kEventClick`，点击（含键盘）统一由 `check-changed` 表达。 |
| 14 | Event.h:28 注释修正 | **已修正**。`Event::IsColorChanged` 由恒 false 改为真实判断（`GetName() == kEventColorChanged`）；注释更新为"ColorPicker 经 C 回调触发 color-changed，Binding 可直接取色"。 |

---

## 新增回归测试

- `test/test_p0_getter.c`（纯 C，无渲染循环，C ABI 冒烟）：
  1. SetControlId 注册/查询/移除
  2. Button `getBoolProperty` 读回 `text-shadow-enable`
  3. `font-size` → `caption-size` 转发
- 已加入 `test/CMakeLists.txt`（与 test_api 相同的 DLL/静态双模式链接），运行全绿。

---

## 回归与发布

- 核心重新编译无错误；P0 相关测试（test_button / test_property / test_layout）0 失败。
- 新增 `test_p0_getter` PASS。
- make_release 已重建，最新 release 已同步至 CornerstoneDesigner `subModules/UICornerstone/`（170 文件，含 UICornerstone.dll）。

---

## 关联澄清

- **第七批 一.1 / 1.2 / 1.3**（InstanceScale 深度 / clip-children / addControl-reflow 行为）：属"已实施待设计器迁移后联合回归"，引擎侧无需改动，按设计器迁移节奏配合即可。
- **第七批 三.2**（字体懒加载 ~400ms）：确认存在，列为低优（引擎字体预热方案评估中），不阻塞交互。
- **第七批 四.1**（Binding Event 取色）：本条在汇总版中已关闭（GetChangedColor 确认存在）；本次额外修正了 IsColorChanged 恒 false 的旧实现，使 Binding 可直接经事件取色，设计器 ColorPicker 自建 thunk workaround 可回收。