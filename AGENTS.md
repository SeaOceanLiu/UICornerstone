# AGENTS.md - UICornerstone

## 交互语言
始终使用中文进行交互。

## 文件夹访问范围铁律（2026-10-18 用户要求）

- **访问范围仅限两个项目目录**：`D:\GitSpace\UIControls` 与 `D:\GitSpace\CornerstoneDesigner`——不触碰其它任何路径（含系统目录、用户目录；临时文件用仓库内 `Temp/`）。
- `D:\GitSpace\CornerstoneDesigner` 中**除 `subModules/` 外，其它目录（含 Temp/、源代码、工程文件）严格只读**：禁止写入、修改、删除、重命名其中任何文件（读取不受限）。
- **唯一写入例外**：发布交付同步 `release/ → D:\GitSpace\CornerstoneDesigner\subModules\UICornerstone`（robocopy /MIR 交付通道）。
- 设计器发现的问题与需求一律走**月度配合修改清单**（`D:\GitSpace\CornerstoneDesigner\Temp\UICornerstone_配合修改清单_<YYYY-MM>.md`，CR 编号 `CR<YYYYMMDD><NNN>`）→ 引擎分析/设计文档（待评审）→ 设计器 + 用户复核 → 实施 → 验证闭环；**禁止直接改动设计器项目**（详见下方 CR 协作规则）。

## Compulsory Reading Before Each Session

### [编码规范 →](design/guidelines/coding.md)
编码前思考、简洁优先、精准修改、目标驱动执行。**每次编码前必须通读。**

### [文件编码](design/guidelines/coding.md)
所有源代码文件（.h/.cpp）必须保存为 **UTF-8 with BOM** 编码格式。不要使用 UTF-8 without BOM 或其他编码。

### [临时文件](design/guidelines/coding.md)
测试临时文件与中间产物必须放入 **`Temp/`** 目录（`.gitignore` 已忽略，不会入库）。**禁止直接在项目根目录或源码目录下创建临时文件。**

### [设计规则 →](design/guidelines/design-rules.md)
位置数据存储规则、魔鬼数字规范。

### [设计文档规范 →](design/guidelines/design-docs.md)
架构决策原则、设计文档编写规范。

### [新增控件验收清单](design/guidelines/coding.md)
每次新增控件时，必须逐项完成以下工作：

1. 控件的声明式UI（JSON）、CABI接口、C++Binding API接口要经过初步测试。
2. 三个后端功能也要完成测试，避免各后端有功能性差异。
3. 控件的缩放功能要经过测试。
4. 确认所有属性字段要放到 PropertyNames.h。
5. 确认所有 JSON 字段要放到 PropertyNames.h。
6. 确认所有字面量都常量化，并酌情放到 PropertyNames.h。
7. 确认控件类型枚举和字符映射已添加。
8. 确认新增 CABI 能让用户从 C 接口创建和操作控件。
9. 确认 C++Binding API 封装了 CABI 接口。
10. 确认涉及的设计文档已刷新，与源码核对无误，并至少自检 2 遍。
11. 确认刷新了 README.md。
12. 确认刷新了用户手册的相关章节（包括但不限于：控件相关、声明式UI语法速查、属性速查表、CABI速查表、C++Binding速查表）。
13. 确认 JSON Schema 已刷新并验证完成（`docs/schema/declarative-ui.schema.json` 同步新增/修改的属性，且 `tools/validate_layout --strict` 校验通过；构建：`cmake -S tools -B build/tools && cmake --build build/tools`）。（2026-08-21 设计审核通过，解除 2026-08-19 暂缓，见 design/JSON_Schema_Design.md）
14. 确认 `design/API_Mapping_Table.md` 已刷新，对照"补"列的缺口标注，逐条决策是否需要补充 CABI / C++Binding / 属性键，并优先实施高优先级项。

## 测试运行约定

- 带窗口的测试（如 `test_property_cabi` / `test_resourceprovider_cabi`）必须传入 `auto=<秒>` 自动关闭参数，否则帧循环会一直等待窗口关闭；统一用 **`auto=5`**（需要更多帧时用 `auto=10`）。
- **启动任何测试/探针程序必须加 shell `timeout` 时间保护**（如 `timeout 30 ./test_x.exe auto=5`）：程序异常/崩溃挂起时不浪费时间（2026-10-08 用户要求）。
- 测试须在 `build/test/Debug` 目录下运行（资源相对路径基于工作目录）。
- 全量回归：`bash testing_scripts/run_regression.sh`（已入库；相对脚本定位仓库根与 build/test/Debug，脚本已处理 auto/CWD），grep `FAILED`。

## Reference (Read as Needed)

| Topic | Document |
|-------|----------|
| 构建与测试 | [build.md](design/guidelines/build.md) |
| RGBA8888 像素格式（关键） | [pixel-format.md](design/guidelines/pixel-format.md) |
| 子模块依赖 | [dependencies.md](design/guidelines/dependencies.md) |
| Session 历史记录 | [history.md](design/guidelines/history.md) |
| 测试用例规范 | [testing.md](design/guidelines/testing.md) |
| C ABI 属性系统 | [CABI_Property_Design.md](design/CABI_Property_Design.md) |

## 配合修改清单（CR）协作规则（2026-11-01 起）

- **按月建档**：每月新需求单独成档 `UICornerstone_配合修改清单_<YYYY-MM>.md`（设计器 Temp），历史清单只读归档；引擎镜像到 `requirements/`。
- **CR 格式**：编号 `CR<YYYYMMDD><NNN>`（当日 001 起）；每条 CR 含类型（requirement/bug）、修订记录、验收、CR 状态。
- **状态归属**：设计器只能修改「设计器」项，引擎只能修改「引擎」项，**不得代改对方**；镜像清单中引擎仅维护「引擎」状态。
- **归档门禁**：仅当「设计器」「引擎」两项均为 `closed` 后，才可归档该 CR 相关文档。
- **交付物**：每个 CR 必须有对应的**分析 + 设计文档**（`design/CR..._Design.md`），并覆盖：自动化测试计划、用户手册更新计划（含示例可运行性）。
- **评审门禁**：分析/设计文档输出后，须等待设计器与用户评审通过，**方可开始实施**；未通过按评审意见修订，再次提交复核，直至通过。
- **实施闭环**：实施后刷新设计文档状态、用户手册与 schema、补自动化测试/探针，跑全量回归与发布同步；将镜像清单「引擎」项置 `closed`。

## 提交与推送规则

- 提交（commit）和推送（push）必须经过用户的明确指示，**未经同意不得自行 push**，也不得自行执行 git add/commit。
- 用户未明确要求时，代码修改仅限于工作区内的文件变更，不创建任何提交。

## Quick Links

- [开发指南索引](design/guidelines/README.md) — 全部规范文档目录
