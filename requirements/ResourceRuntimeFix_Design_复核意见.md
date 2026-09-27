# ResourceRuntimeFix_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-22
- 对象：`design/ResourceRuntimeFix_Design.md`（P0-17~21 + Bonus A/B，状态：待评审）
- 结论：**通过，放行实施**（§7 六个待审核问题全部确认；设计器随批 1 项删除动作见 §3）

## 1. 分项评审

### P0-17 不复现——接受，修正设计器侧误判

- 引擎探针实证相对路径 LOADED + 像素绘制（`loadFromFile` 内部已拼 basePath，Actor.cpp:81-83）。
- **设计器此前凭 `Actor::setStringProperty` 一行直传判断"未拼 basePath"系误判**（未追进 loadFromFile 内部）——以引擎探针为准，撤回 P0-17。
- 提示的"image-resource 后 image 重建覆盖"陷阱已知悉，设计器暂无此组合场景，如出现另立需求。

### P0-18 维持现状——确认

解析期结构键，设计器已过滤无效槽 ✓。

### P0-20 四 setter 补"可见 + 条件补建"——确认（与设计器清单建议一致）

- `setVisible(true)` 双保险（create 成功亦 setVisible，覆盖 `GET_CONTEXT==nullptr` 早退）✓；
- 条件补建仅 `isCreated()` 时执行，parse 期由 `Button::create → ensureActor` 兜底 ✓；
- 同型排查（CreateImageButton/LayoutParser actors/animation）无涉 ✓。

### P0-21 读回以目标对象稳定存储为准——确认

- 数据源为 Actor/LuotiAni 成员（Builder/解析注入同样可读回，无双份状态）✓；
- 返回设置原值（相对保持相对，与设计器输入一致）✓；
- 资源引用不冒充文件路径（读回 0，键不同不越权）✓；
- 生命周期约定与 caption 一致 ✓。
- **设计器影响：零改动**——extended 兜底逻辑为"引擎读空才取"，引擎读回原值后自动兼容（后续清理可选）。

### Bonus A（animation 异常保护）+ Bonus B（frames 转发）——确认

- try/catch 镜像 LuotiAni 语义（失败返 0/保留旧动画/可重试）✓；
- **setLuotiAni 移至 loadFromFile 之后**——失败不替换旧动画，优于原顺序 ✓。

### P0-19 schema 5 键 + LayoutParser 字符串支持——确认同批

- "校验通过但布局无效"的担忧成立，同批正确；
- 复用运行时分发（解析期未创建，ensureActor 补建，与 actors 路径同构已证安全）✓；
- actors 对象优先、字符串键补空缺——对既有布局零影响 ✓。

## 2. §7 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | schema + LayoutParser 同批 vs schema-only | **同批**（推荐采纳） |
| 2 | 读回值原值 vs 绝对路径 | **设置原值**（推荐采纳） |
| 3 | 资源场景读回 0 vs provider: 前缀 | **返回 0**（推荐采纳） |
| 4 | setVisible(true) 覆盖显式隐藏 | **接受**（推荐采纳） |
| 5 | Bonus A 失败语义 | **确认** |
| 6 | P0-17 复现信息 | 设计器 image 行绝对路径实测显示正常；相对路径引擎探针已证可用——**按不复现结案** |

## 3. 设计器随批动作（引擎实施同步后）

1. **删除 Button 硬编码资源段**（CanvasPane.cpp `Button 资源行` 段，5 行内置）——schema 补齐后通用动态行会生成同 5 行，**不删将双份重复**；
2. extended 读回兜底**保留**（自动兼容引擎读回，清理可选不急）；
3. 联测项：Button 四态图相对/绝对路径显示、读回原值保持、无效 animation 路径不再崩溃、LuotiAni 旧动画保留。

## 4. 放行

**结论：放行实施**。实施完成后同步 subModules，设计器按 §3 随批，颜色组之后的资源属性链路全闭环。
