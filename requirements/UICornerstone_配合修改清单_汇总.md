# UICornerstone 配合修改清单（汇总版）

- 提出方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-17
- 说明：基于 InstanceScale C ABI、容器 reflow/属性对称/Panel 裁剪两批实施后的设计器验证结果，替代此前分散的遗留条目，为当前**全部有效**的引擎配合项。已关闭项（Binding Event 取色——GetChangedColor 实际存在）不再列入。

## P0 —— 影响功能正确性

### 1. Button `getBoolProperty` 缺失（getter 补齐 P0）

- **现象**：`SetBool("text-shadow-enable")` 写成功，读恒失败 → 属性面板勾选状态无法回填（写入已由 check-changed 驱动，仅状态回读缺失）。
- **根因**：Button 仅重载 setBoolProperty（Button.cpp:368），无 getBoolProperty。
- **建议**：补 `getBoolProperty`（kTextShadowEnable → m_enableTextShadow）；顺带扫描 22 类控件 set/get 差集，高频编辑项优先补齐（见 P1-1）。

### 2. TextShadow 缺省色纯白，浅色控件上阴影不可见

- **现象**：`setTextShadowEnable(true)` 后阴影画了但看不见（白影贴浅色面）。
- **根因**：`ConstDef::DEFAULT_TEXT_SHADOW_NORMAL/HOVER/DOWN_COLOR = (255,255,255,255)`（ConstDef.cpp:29-31）。深色主题下白影合理，浅色控件主题下缺省即无效。
- **建议**：缺省改深色半透明（如 rgba(0,0,0,120)），或跟随主题明暗反色。设计器已显式设置半透明黑绕过。

### 3. schema `font-size` 对 button 无效（声明与实现不匹配）

- **现象**：动态属性行对 button 生成 font-size 行，写入无分发（Button 文本字号是 caption-size），用户编辑无效。
- **根因**：common 的 font-size 声明于全部控件，Button 无 font-size 的 setInt/setFloatProperty 分发。
- **建议**（二选一）：a) schema 按控件类型裁剪 common 键（button/caption 类不含 font-size）；b) Button 补 font-size 分发（转发到 caption-size）。设计器已按类型 skip 该行绕过。

### 4. 编程式创建的控件 FindControl 找不到

- **现象**：工厂（CreateButton 等）创建的控件不在 `controlsById`，`UICornerstone_FindControl` 返回空——设计器被迫自存 id→句柄映射。
- **根因**：仅 LoadLayout 注册 controlsById（UICornerstoneAPI.cpp:979），工厂创建不注册。
- **建议**：工厂增加可选 id 参数并注册；或新增 `UICornerstone_SetControlId(instance, ctl, id)`。

### 3.1 升级（2026-09-17 晚）：font-size 统一 + caption-size 废弃

- **升级背景**：Button 补 font-size（Float 分发）后仍有两处混乱——schema 类型 integer 走 SetInt 失配（Button::setIntProperty 无此键）；且 button 动态行同时出现 caption-size/font-size 两个语义重复的字号键。
- **根因**：Button 早期自建 captionLabel 简化体系（caption/caption-size/text-shadow-enable），未走通用 font-size；而 font-size 已是所有文本控件的事实标准（Label/EditBox/TextArea/ListView/ProgressBar 均在 setIntProperty）。
- **建议（最终方案）**：
  1. Button 补 `setIntProperty(kFontSize) → setCaptionSize`（与 Label 对齐，Float 分发保留）；
  2. schema button def **删除 caption-size 键**；
  3. `caption-size` 运行时键保留单行转发作别名（或直接删除——无正式应用发布，可趁窗口期除名）。
- 设计器零改动（schema 删键后行自动消失；数值双写对单键照常工作）。

## P1 —— 体验 / 代码质量

### 5. 新控件类型首建 ~400ms（字体懒加载）

- **现象**：首次创建 NUD/EditBox/CheckBox 等含文本控件时单帧卡顿 ~390ms（字体首次加载/光栅化）。
- **建议**：常用字体集预热（首实例初始化后台加载）或控件级字体共享缓存。非阻塞。

### 6. 应用层光标被引擎控件 hover 覆盖

- **现象**：设计器手柄 resize 光标（SDL 系统光标）被引擎控件 hover 的 `Cursor::setCurrent` 覆盖，需每帧重发（激活期）维持。
- **建议**：提供实例级光标 override 栈或"应用设置优先"约定（应用设置后引擎控件不再覆盖，直到应用清除）。

### 7. LayoutParser kJson\* 双常量体系合并（P1 重构，已批准未实施）

- 890 处 kJson\* 引用改用属性名常量，长期规则化。命名统一后两套常量语义已一致，可分批机械替换。

### 8. getter 全量补齐（P1）

- 脚本扫描 set\*Property/get\*Property 常量差集，逐控件补齐（P0 项 1 的全集）。属性面板值回填的完整依赖。

## P2 —— 文档 / 确认类（回复结论即可）

| # | 项 | 说明 |
|---|---|---|
| 9 | **schema 残留未迁移键** | button def 的 `luotiAni`/`providerName`/`resourceId` 仍 camelCase——有意保留（专有名词/无运行时属性）还是遗漏？ |
| 10 | **动画 jsonc 键名范围** | 动画文件是否属布局体系（含可编辑键）？独立则豁免迁移 |
| 11 | **文档同步确认** | API_Mapping_Table.md、文档 7.3 速查的 4 项改名（check-color 等）；ViewportScale_Design.md 与手册 5.6 的 override 语义说明 |
| 12 | **测试补充确认** | test_viewport_scale 的 override 用例（SetInstanceScale → SetViewport → scale 保持）是否已加并全绿 |
| 13 | **CheckBox 无 click 事件** | 点击仅发 check-changed——建议控件事件表入文档/速查（或 CheckBox 补发 click） |
| 14 | **Event.h:28 注释修正** | "IsColorChanged 恒 false（ColorPicker 用轮询）"与实际（onOK 触发 color-changed C 回调、GetChangedColor 可取）不符 |

## 关联背景

- 已落地并验证：InstanceScale C ABI（设计器已迁移至引擎画布语义）、Panel::addControl/removeControl reflow、clip-children、schema/布局键名统一 kebab、Button caption-size getter、splitter linked 语义 + flow-weight 布局（设计器侧配置修复）。
- 设计器绕行项（上述 2/3/4 有绕行）将在引擎修复后回收。
