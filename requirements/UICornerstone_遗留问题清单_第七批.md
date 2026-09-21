# UICornerstone 遗留问题清单（第七批·随 InstanceScale/容器三项之后）

- 提出方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-17
- 性质：遗留汇总（未完整验证项 / P1 未做项 / 新发现 / 历史 workaround），供引擎排期
- 前置：InstanceScale_CABI_Design（已实施已同步）、PanelReflow_PropertySymmetry_Clip_Design（已实施已同步）——设计器侧已完成首轮验证（探针 PASS），本文档为余项

## 一、已实施但未完整验证（需联合回归）

### 1.1 InstanceScale 深度效果

已验证：C ABI 调用（ok=1）、Get 复合快照（1.50）、重复读取值保持（override 生效）。
**未验证**（设计器当前为像素坐标旁路，需迁移后才可真实回归）：

- 子视口内控件 rect/字号的视觉跟随（×1.5 后 drawRect、重新光栅化）
- resize → `recomputeViewportTransform` 后 override 保持（探针仅重复读值，未走 SetViewport 触发路径）
- fit/stretch 模式切换清除 override（引擎接管语义）
- **设计器配合**：逻辑 rect 创建 + SetZoom 改 SetInstanceScale + 网格/选中框对策（复核意见 §3.1），迁移后联合像素回归

### 1.2 clip-children

已确认常量落地（`kClipChildren = "clip-children"`），**行为未验证**：

- 子项超界裁剪正确性（含边界：恰好半可见行）
- 嵌套 Panel 均开启时 clipStack 相交
- 与 ScrollBar 共存：容器内滚动条自身不被内容 rect 误裁（复核意见 §4-2）
- **设计器配合**：propDynamic 开启 + 滚动删"超界隐藏"改纯偏移

### 1.3 Panel::addControl/removeControl reflow

已实施，但设计器 propDynamic 已改 absolute（无 layoutEngine）→ **当前无触发路径**：

- 验证路径：propDynamic 恢复 v-flow 声明（JSON）+ 编程式 AddChild 即排
- removeControl 对称 reflow（挂入后移除、布局无残留）
- LoadLayout 回归（Parser 显式 reflow 幂等）

## 二、本批设计文档承诺的 P1（未实施）

| # | 项 | 出处 | 说明 |
|---|---|---|---|
| 1 | **getter 补齐 P1** | PanelReflow 设计 §2.2 | P0 仅 Button caption-size；全量 set/get 差集扫描补齐未做——设计器值回填仍多处回落缺省 |
| 2 | **LayoutParser kJson\* 合并** | 设计 §2.3 清单 4 | 890 处双常量体系（kJson\* 244 个）→ 属性名常量，长期规则化 |
| 3 | **动画 jsonc 范围确认** | 设计 §2.3 清单 5 / 待审核 6 | 动画键名是否属布局体系未答复；独立则豁免迁移 |
| 4 | **文档同步确认** | 设计 §5 | ViewportScale_Design.md、手册 5.6、API_Mapping_Table.md、7.3 速查的 4 项改名（check-color 等）是否已同步 |
| 5 | **测试补充确认** | InstanceScale 设计 §5 | test_viewport_scale override 新用例（SetInstanceScale → SetViewport → scale 保持）是否已加并全绿 |

## 三、首轮验证新发现

### 3.1 schema 残留未迁移键（3 个）

`luotiAni` / `providerName` / `resourceId`（button def 内）——与"schema 键名 == 运行时属性名"验收目标不一致。均为资源类，设计器 skip 不受影响，但请确认：**有意保留（专有名词/无对应运行时属性）还是遗漏**。

### 3.2 新控件类型首建 ~400ms（字体懒加载）

放置后动态属性行首次创建 NUD/EditBox/CheckBox 时，单帧卡顿 ~390ms（引擎字体首次加载/光栅化）。设计器属性面板交互可感知。

建议方向（引擎评估）：字体预热（首实例创建时后台加载常用字体集）或控件级字体共享缓存。**非阻塞**，交互可接受，排期靠后。

## 四、历史 workaround 清单（引擎能力就绪后设计器可回收）

| # | 设计器 workaround | 引擎根因 | 建议方案 | 优先级 |
|---|---|---|---|---|
| 1 | ColorPicker 取色经 C ABI 自定义 thunk（绕过 Binding Event） | Binding `Event` 无取色接口（`IsColorChanged` 恒 false）；ColorPicker 无读值属性 | Binding Event 补 `GetEventColor()`（UIEventData.color 已有载荷）；或 ColorPicker 补 getStringProperty(kColor) 读值 | 中（现方案可用，代码晦涩） |
| 2 | 编程式创建的控件 FindControl 找不到（设计器自存句柄映射） | 控件工厂（CreateButton 等）不注册 `controlsById`（仅 LoadLayout 注册） | 工厂创建后注册（可选 id 参数），或提供 `UICornerstone_SetControlId(instance, ctl, id)` | 中 |
| 3 | 手柄光标激活期每帧 SDL_SetCursor（防覆盖） | 引擎控件 hover 经 `Cursor::setCurrent` 恢复自身光标，覆盖外部设置 | 引擎提供应用层光标接管接口（如实例级 cursor override 栈）或"外部设置优先"约定 | 低（节流后开销可接受） |
| 4 | — | `Event.h:28` 注释"IsColorChanged 恒 false（ColorPicker 用轮询）"与实际（onOK 触发 kEventColorChanged C 回调）不符 | 注释修正（ColorPicker 的 color-changed 走 setCallbackProperty/onOK） | 低（文档性） |

## 补充（2026-09-17 验证批·二）

- ~~**Binding Event 无取色接口**~~ **已撤销**：`Event::GetChangedColor()` 实际存在（此前检索遗漏），设计器已删除 C ABI thunk 绕行（四.1 关闭）。`Event.h:28` IsColorChanged 恒 false 的注释修正仍建议（四.4）。
- **CheckBox 无 click 事件**：点击仅发 `check-changed`（intVal=CheckState），不发 `click`——应用层按 click 订阅静默失效。建议文档/速查标注各控件事件表（或 CheckBox 补发 click）。设计器已改订阅 check-changed。

## 补充（2026-09-17 验证批）

- **Button `getBoolProperty` 缺失**（getter P0 扩充）：`text-shadow-enable` 写有效（setBoolProperty 分发存在），但读失败 → 设计器 boolean 行勾选状态无法回填（当前已改为 check-changed 驱动写入，仅状态回填缺失）。
- **TextShadow 缺省色纯白**：`ConstDef::DEFAULT_TEXT_SHADOW_*_COLOR = (255,255,255,255)`——浅色控件面（设计器按钮 #FAFAFA）开启阴影后白影不可见，形同无效。建议缺省改深色半透明（如 rgba(0,0,0,120)）或按主题反色。设计器已显式设置半透明黑阴影绕过（styleControl）。
- **schema `font-size` 对 button 无效**：common 的 font-size 在 Button 无 setInt/setFloatProperty 分发（其文本字号是 caption-size）——schema 声明与控件实现不匹配，设计器已按类型 skip 该行；引擎侧建议 schema 按控件类型裁剪 common 键或属性系统补分发。

## 五、建议排期

1. **随下一批实施**：四.1（Binding Event 取色）、四.2（SetControlId）——设计器代码可显著简化
2. **随迁移联合回归**：一.1/1.2/1.3（设计器迁移即验证，引擎待命支持）
3. **P1 批次**：二.1（getter 全量）、二.2（kJson 合并）——不阻塞，按引擎节奏
4. **确认类**：二.3/4/5、三.1（3 个残留键）——请回复结论即可
5. **低优**：三.2（字体预热）、四.3（光标接管）、四.4（注释）
