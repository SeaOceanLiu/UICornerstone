# P102_AniSrcRelative_Design — LuotiAni 帧图 `src` 支持"相对于动画描述文件（jsonc）目录"解析

> 提出方：引擎侧（用户需求，2026-10-18 确认）
> 前置核查：设计器配合修改清单（第三批，最新 2026-10-18）**无此要求**——既有相关项仅 P0-23（帧图文件回退，双基准 `exeDir/assets` → `exeDir`，2026-09-22 复核通过）；本项为**新增能力**，走"设计文档（待评审）→ 设计器复核 → 实施"流程。
> 状态：**已放行，已实施**（2026-10-18；复核：`requirements/P102_复核意见.md`——通过，附 1 条实施注意）
> 实施备注（2026-10-18）：① `m_descDir` 落地（`loadAniDesc(fs::path)` 写入 parent_path 的 **UTF-8**；`loadFromResource` 清空）；② 回退链按 **A**（jsonc 目录最优先，双基准顺延）——`getImageFromResource` 改用 vector 基准 + `fs::u8path/u8string`，错误日志含全部基准；③ **复核实施注意落实（UTF-8 感知全链）**：`loadAniDesc` 的 jsonc 打开改 `_wfopen`（宽字符）；`loadFromFile`/`LayoutParser`(×3)/`CreateAnimation` 的路径构造统一改 `fs::u8path`（中文/超长路径安全）；`Surface::loadFromFile` 传 `u8string`（SDL IMG_Load 按 UTF-8）；④ 探针 `temp_p102_probe` **3/3**——中文目录（`Temp/动画资源`）+ 同目录短名 src（created/playing/loop=1）/ 兼容回归（rotateBtn assets 根相对不变）/ src miss 回滚 + 全基准日志；⑤ 探针环境注记：静态链接探针 + DLL 后端时 Surface 工厂注册于 `UICornerstone.dll`（后端 DLL 链接核心导入符），静态探针取不到 → 本探针改走**静态回调**（`GetUIBackendCallbacks`，同 TestInstance 模式）；DLL 核心应用不受影响（后端 DLL 注册进核心 DLL 自身）；⑥ P0-93/94、P0-96/97、P0-98/99/100、P0-101 探针回归全过；全量回归仅 4 项预存；Debug/Release 零警告。

---

## 1. 需求与现状核实

**需求**：动画描述文件 `*.jsonc` 中帧图层 `src` 目前按"资源包完整路径（相对 assets 根）"解析（样例注释即如此：`"src": "animations/rotateBtn/rotateBtn.svg"  // 图片资源ID，实际上是图片在资源包中的完整路径`）。希望**新增**"相对于 jsonc 所在目录"的解析语义（如 `"src": "rotateBtn.svg"`，jsonc 与帧图同目录即可，任意目录可移植），**向后兼容**现有语义。

**现状（源码核实）**：

| # | 事实 | 依据 |
|---|---|---|
| ① 加载链 | `loadFromFile(path)`：`provider:` 前缀 → `loadFromResource`；否则记录 `m_filePathStr`（原值），相对路径拼 `Platform::GetBasePath()`（exe 目录）后 → `loadAniDesc(fs::path)`（provider 优先 → fopen 回退，P0-79） | `src/LuotiAni.cpp:156-215` |
| ② 资源引用链 | `loadFromResource(id)`：记 `m_resourceId`、清 `m_filePathStr` → `loadAniDesc(string)`（**仅 provider**；挂树后延迟补读同入口 :582-583） | `src/LuotiAni.cpp:168-171, 217-233` |
| ③ src 解析链 | `getImageFromResource(src)`：剥 `provider:` 前缀 → ①provider `readFile` 优先 → ②文件回退双基准 `{ <exeDir>/assets, <exeDir> }`（P0-23）→ 仍 miss throw（错误含双基准） | `src/LuotiAni.cpp:86-121` |
| ④ src 使用点 | 解析期 `setSrc(...)`（:285）→ 烘焙期 `getImageFromResource(layer->getSrc())`（:633） | 同左 |
| ⑤ 成员 | `m_filePathStr`（原值读回）/ `m_resourceId`（延迟补读）；**未保存 jsonc 解析后的所在目录** | `include/LuotiAni.h:267-268` |

**缺口**：jsonc 解析后的**绝对文件路径**（`loadAniDesc(fs::path)` 入参，已含 basePath 解析）未被记录 → `src` 解析无法以 jsonc 所在目录为基准。

## 2. 方案

### 2.1 记录 jsonc 所在目录（新成员）
- `include/LuotiAni.h` 增 `std::string m_descDir;`（jsonc 所在目录，绝对；空=无文件基准）；
- 写入点：`loadAniDesc(fs::path filePath)` 进入文件路径处理时 `m_descDir = filePath.parent_path().string();`（该入参已经过 basePath 解析，绝对）；
- 清理点：`loadFromResource(id)` 中 `m_descDir.clear();`（防切换动画后残留上一 jsonc 的目录基准）。

### 2.2 扩展 `getImageFromResource` 回退链（建议 A）
```
provider readFile（不变，资源包优先）
→ ① m_descDir/<src>          ← 新增：jsonc 相对（m_descDir 非空时）
→ ② <exeDir>/assets/<src>    （P0-23 既有）
→ ③ <exeDir>/<src>           （P0-23 既有）
→ throw（错误信息含全部基准，含 m_descDir）
```
- **兼容性**：现有 assets 工程 src 均为 assets 根相对路径（如 `animations/rotateBtn/rotateBtn.svg`）——`m_descDir/<src>` 不会命中（`…/rotateBtn/animations/…` 不存在）→ 自然落回既有基准，**行为零变化**；新语义仅在 src 为"jsonc 同目录短名"时生效；
- `provider:` 显式前缀语义不变（剥前缀走同一链）；
- `src` 为绝对路径时 `fs::path::operator/` 语义自动取 src（与现状一致）。

### 2.3 备选（Q-1）
- **A（推荐）**：`m_descDir` 置于双基准**之前**——最具体上下文优先，防同名图跨动画串用；
- **B**：置于双基准**之后**——纯追加、零优先级变化（但 jsonc 同目录同名图会输给 exeDir/assets 同名文件）。

### 2.4 范围界定（Q-2/Q-3）
- **provider 加载的描述**（`provider:` 引用 / 内存资源）：`m_descDir` 不设（无文件语义），走原链（provider 优先已覆盖）——`loadAniDesc(fs::path)` 的 provider 命中场景仍按入参文件路径设置 `m_descDir`（无害：相对基准自然 miss 后回落）；
- **Actor 不动**：Actor 无描述文件语义（`image` 键直接路径），维持 `basePath` 拼接现状。

## 3. 验收与测试矩阵

1. **新语义**：`Temp/ani_rel/anim.jsonc` + `Temp/ani_rel/icon.svg`，`src:"icon.svg"` → `prepare()` 烘焙成功、播放出帧（像素/帧数断言）；
2. **兼容回归**：`assets/animations/rotateBtn/rotateBtn.jsonc`（src=assets 根相对）行为不变（帧烘焙 OK）；
3. **资源包优先**：provider 注册同名键 → 取 provider 版本（新增基准不越权）；
4. **切换清理**：先加载文件 jsonc（设 m_descDir）→ 再 `SetString("path","provider:xxx")` → 旧 descDir 不参与解析；
5. **miss 诊断**：jsonc 目录基准纳入错误日志；
6. 全量回归仅 4 项预存；Debug/Release 零警告。

## 4. 影响面与文档

- 源码：`include/LuotiAni.h`（+1 成员）、`src/LuotiAni.cpp`（写入/清理/回退链/日志，局部）；
- 无 C ABI/JSON schema 变更（jsonc 为引擎自有描述格式）；
- 文档（实施后）：`design/LuotiAni_DevGuide.md` §2.2、`design/LuotiAni_Design.md`（src 语义）、用户手册 `docs/advanced/luotiani.html`（`src` 注释与解析链说明）；
- 设计器侧：新能力可用于"动画 jsonc 与帧图同目录、工程任意位置"的工作流（复核后由设计器联测）。

## 5. 待确认（Q）

| # | 问题 | 建议 |
|---|---|---|
| Q-1 | 新增基准位置：jsonc 目录置于双基准之前（A）或之后（B） | **A**（最具体上下文优先）——**用户已确认倾向 A（2026-10-18），最终以设计器评审判定** |
| Q-2 | provider 加载的描述文件是否设文件目录基准 | 不设（provider 优先已覆盖；避免语义混淆） |
| Q-3 | Actor 是否同步（`image` 键支持"相对自身"） | 不动（无描述文件语义，不扩面） |
| Q-4 | 是否同步更新样例 jsonc 注释（subModules/assets） | 实施后视需要（可选） |
