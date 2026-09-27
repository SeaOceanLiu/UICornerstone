# ActorUnload_AniResourceFallback_Design 复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-22
- 对象：`design/ActorUnload_AniResourceFallback_Design.md`（P0-22 + 补充 + P0-23，状态：待评审）
- 结论：**通过，放行实施**（§7 五个待审核问题全部确认；设计器随批零代码改动，联测项见 §3）

## 1. 分项评审

### P0-22 空路径 = 卸载（§3.1）✓

- 早退不拼 basePath、不刷日志 ✓；`clearImage()` 全量清理（surface/texture/filePath/filePathStr/**resourceId**）✓。
- **m_resourceId 一并清正确**：否则"先资源图后清空 → recreate 复活资源图"= 删除失效——清掉才能保证卸载语义幂等。
- create() 双字段空跳过加载（既有分支天然成立）✓。
- **设计器闭环确认**：写 `""` → 无图 + 读回 0 → extended 兜底同为空 → 行显示空——"清空=删除"全链一致。

### P0-22 补充 失败 = 无图（§3.2）✓

- 两处失败点统一清 surface/texture（消除 device null/非 null 分支不一致）✓。
- **路径字段保留（确认）**：读回=尝试值（设计器行显示用户输入 ✓）；recreate 可重试（资源晚挂载场景）✓。
- loadFromResource 失败分支不改（两阶段设计、provider 晚注册补读）✓。

### P0-23 帧图 provider miss → 文件回退（§3.3）✓

- **双基准**（`ConstDef::pathPrefix` 即 `<exeDir>/assets` 优先 → `GetBasePath` 兜底）——前者经引擎实测是 jsonc src 的实际根，后者兼容 src 自带 `assets/` 前缀——基准选择有据 ✓。
- provider null 不再直接 throw（无 provider 宿主零配置可用）✓。
- `loadFromMemory` 失败继续回退（资源包内数据损坏时文件可能有完好副本）——增强 ✓。
- `fs::exists` 短路、prepare 一次性成本可忽略 ✓；错误信息含两基准（可诊断）✓。
- **设计器动画工作流落地形态**：jsonc 与帧图同目录放好（相对 assets 根或 exe 旁）→ animation 行输入相对路径即用，零资源包注册。

## 2. §7 待审核问题答复

| # | 问题 | 答复 |
|---|---|---|
| 1 | clearImage 一并清 m_resourceId | **确认**（防 recreate 复活） |
| 2 | 失败路径字段保留 vs 清空 | **保留**（读回=尝试值 + 可重试） |
| 3 | 双基准顺序 assets → exe | **确认** |
| 4 | provider: 剥前缀走同一回退链 | **确认** |
| 5 | loadFromMemory 失败继续回退 | **确认**（增强） |

## 3. 设计器随批（引擎实施同步后）

- **设计器代码零改动**（空串放行/extended 空值/中间态过滤已就绪，与本设计语义完全对齐）。
- 联测项：
  1. Button/Image 清空路径：**无失败日志**、图消失、再设有效路径恢复；
  2. Image 换图输入坏路径：旧图消失（失败=无图）；
  3. animation 行输入 `assets/animations/rotateBtn/rotateBtn.jsonc`：**动画播放**（帧图文件回退生效）；
  4. 帧图两基准均 miss：不崩溃、失败日志含基准信息。

## 4. 放行

**结论：放行实施**。实施完成后同步 subModules，设计器按 §3 联测，资源属性链路（含动画）全闭环。
