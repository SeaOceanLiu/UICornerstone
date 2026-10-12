# P070_TextFollow_ResourceDir_Design — TextArea 输入跟随核实 + resourceProviders 目录挂载

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-05 追加：P0-70）
> 前置：P0-68 批已实施并同步
> 状态：**已放行，已实施**（2026-10-06；①Q-1 补防御行；②Q-5 采纳 (a)；③Q-4 暂缓）
> 复核：`requirements/P070_TextFollow_ResourceDir_Design_复核意见.md`（通过；① 设计器报告更正为误报）
> 实施备注：① TextInput 分支 `insertText` 后显式补 `ensureCursorVisible()`（防御性，零行为变化；TextArea.cpp）；② 目录挂载：`MemoryResourceProvider` 最长挂载名前缀解析（精确优先；`\`→`/` 归一化；`..`/盘符/前导分隔符拒绝；懒加载缓存键=完整子路径）+ `exists` 镜像 + `mountPath` 尾部去分隔符；schema resourceProviders 描述；declarative-ui.html 说明；③ ListView 列宽/tooltip 暂缓；④ 测试：test_resourceprovider_cabi 新增目录挂载 4 断言（32x32 正例 / 反斜杠归一化 / `..` 逃逸拒绝 / 布局加载）全过；test_property_cabi 191/0；回归仅 4 项预存

---

## ① TextArea 输入路径纵向光标跟随（设计器报告"输入不跟随"）

### 1.1 源码核实（与设计器报告有出入）

| 路径 | 设计器报告 | 源码核实 |
|---|---|---|
| TextInput 输入字符 | TextInput 分支（TextArea.cpp:644-661）`insertText(filtered); ensureCursorHorizontalVisible();` **缺纵向跟随** | 分支内确实无直接调用，但 **`insertText`（src/TextArea.cpp:1395-1402）末尾已调用 `ensureCursorVisible()`** → 纵向跟随已成立 |
| 回车/换行 | "实测不跟随" | Return 分支（src/TextArea.cpp:669-702 区域）已显式 `ensureCursorVisible()` → 有跟随 |
| 粘贴 Ctrl+V | — | 已显式 `ensureCursorVisible()`（src/TextArea.cpp:839-845） |

### 1.2 探针实测（Temp/temp_p070_probe.cpp，当前构建）
- 条件：TextArea 200×60、10 行文本、`auto-scroll=false`、光标文末、`scroll-y` 置 0；
- 结果：TextInput 输入字符 → `scroll-y=168`（= max，跟随）✓；Return → `scroll-y=188`（11 行 **新** max，跟随）✓；
- **结论：引擎侧未复现"输入/回车不跟随"**。

### 1.3 原因分析（设计器侧）
1. **过期 DLL 嫌疑最大**：P0-66 复核已出现同类前科（设计器自注"同步 subModules 后必须先重建设计器"）；
2. **文本同步路径**：若设计器的编辑流程经 `SetString("text")`/`setText` 回写模型而非真实键入，则命中 P0-68 Q-3 已定语义（`false` 下 `setText` 整体替换不自动滚动）——表现为"输入不跟随"。

### 1.4 方案（Q-1 二选一）
- **a) 防御性补一行（建议）**：TextInput 分支在 `insertText` 后显式补 `ensureCursorVisible()`——当前无行为变化（insertText 已调），与方向键/回车/粘贴分支风格一致，防 `insertText` 内部演进回归；成本一行；
- **b) 引擎不改**：设计器重建后重测；若确为 `SetString` 同步路径，设计器侧在同步后调 `scrollToBottom()`（或待引擎后续提供 cursor-visible 触发 API）。

---

## ② resourceProviders 目录挂载语义

### 2.1 现状（源码核实）
- 挂载：`MemoryResourceProvider::mountPath(name, path, basePath)`（src/ResourceProvider.cpp:104-111）= `name → 单文件路径`，首挂懒创建 `FilesystemResourceProvider(basePath)` 委托；
- 读取：`readFile`（src/ResourceProvider.cpp:129-157）：`entries`（内存注册）精确 → `paths`（挂载）精确 → `delegate->readFile(path)`（`fs::path m_basePath / path`，src/ResourceProvider.cpp:20-46）→ P0-54 assets 前缀剥离重试；`exists` 同构（:160-172）；
- **不支持目录/子路径**：`provider:main`（目录）→ 委托 fopen 目录失败 → not found；`provider:main/rel.ext` 无匹配键。
- 设计器 UI 为"名称 + 目录选择"（SHBrowseForFolder）→ 目录条目完全不可用。

### 2.2 方案 (a)：目录挂载 + 前缀解析（推荐，设计器倾向）

在 `MemoryResourceProvider` 精确匹配 miss 后增加**最长挂载名前缀解析**：

```
key（已剥 provider:） 示例: "main/anim/bomb.json"
  ├─ entries 精确命中？ → 返回（不变）
  ├─ paths 精确命中？   → 返回（不变，文件型挂载兼容）
  └─ 前缀解析：在 paths 中找最长挂载名 M，满足 key = M + "/" + rest
       → rest 合法性检查（见下）
       → delegate->readFile(M.path + "/" + rest)（绝对/相对均兼容：
          operator/ 绝对 rhs 覆盖 basePath；相对 rhs 拼 basePath）
       → 命中缓存 m_impl->lazy[key]（键=完整 key，不同子路径独立缓存）
```

- `exists` 镜像同逻辑；
- 名称冲突：最长前缀优先（挂载 `a` 与 `a/b` 并存时 `a/b/x` 走 `a/b`）；
- 兼容性：精确匹配优先，文件型挂载/内存注册/旧引用**零变化**；目录本身 `provider:name` 自然 not found（fopen 目录失败）；
- 安全（Q-2）：`rest` 含 `..` 段或盘符绝对路径 → 拒绝（返回 not found），防止目录逃逸；
- 分隔符（Q-3）：匹配键统一 `/`；Windows 反斜杠 `\` 归一化为 `/` 后再匹配；
- 改动面集中：src/ResourceProvider.cpp（lookup/exists 两处 + 小工具函数，约 40 行）。

### 2.3 方案 (b)：保持单文件语义
- 引擎零改动；设计器把资源面板改为文件选择对话框（每条目=一个资源文件），与 `name → path` 现状一致。

### 2.4 评估
- (a) 实现小、向后兼容、直接满足"选目录"UX，且与内存注册表（精确键）不冲突 → **推荐 (a)**；(b) 作为回退。
- 文档/模式同步（如采纳 (a)）：schema `resourceProviders` 描述补充"path 可为文件或目录；目录经 `provider:name/相对路径` 引用"；docs/declarative-ui.html、docs/appendix/capi.html 挂载说明同步。

### 2.5 测试计划（如采纳 (a)）
- 扩展 `test/test_resourceprovider_cabi.cpp`：运行时创建自包含临时目录（复制现有测试资源）→ `resourceProviders` 挂载 `{name, path:<dir>}` → 引用 `provider:<name>/<file>` 断言资源加载成功；负例 `provider:<name>/not-exists`、`../` 逃逸（返回 not found 不崩溃）；文件型挂载与目录型并存；内存注册名与目录名冲突时精确优先。

---

## ③ （可选）ListView 列宽拖拽 / 单元格 hover tooltip

- 设计器已有过渡：选中行全路径显示于状态栏 + 列宽加宽（80/160）；
- 优先级低、不阻塞 → **建议引擎暂缓**（入 backlog，后续批次评估；Q-4）。

---

## 待审核问题
| # | 问题 | 建议 |
|---|---|---|
| Q-1 | ① 是否防御性补显式 `ensureCursorVisible()`（当前无行为变化） | **补**（一行，分支风格统一 + 防回归）；亦可选不改、设计器重测 |
| Q-2 | ② `rest` 含 `..`/盘符绝对路径拒绝？ | 确认（防目录逃逸） |
| Q-3 | ② 分隔符归一化（`\` → `/`） | 确认 |
| Q-4 | ③ ListView 列宽/hover tooltip 暂缓？ | 暂缓 |
| Q-5 | ② 采用 (a) 目录挂载，还是 (b) 设计器改文件选择？ | **(a)**（改动小、兼容、匹配 UX） |
