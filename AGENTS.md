# AGENTS.md - UICornerstone

## 交互语言
始终使用中文进行交互。

## 设计器项目只读铁律

- `D:\GitSpace\CornerstoneDesigner` 一律**只读**：禁止写入、修改、删除、重命名其中任何文件（含 Temp/、源代码、工程文件；读取不受限）。
- **唯一例外**：发布交付同步 `release/ → D:\GitSpace\CornerstoneDesigner\subModules\UICornerstone`（robocopy /MIR 交付通道）。
- 设计器发现的问题与需求一律走**配合修改清单**（`D:\GitSpace\CornerstoneDesigner\Temp\UICornerstone_配合修改清单_第三批.md`）→ 引擎分析 → 设计文档（待评审）→ 设计器复核 → 实施，**禁止直接改动设计器项目**。

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

## 提交与推送规则

- 提交（commit）和推送（push）必须经过用户的明确指示，**未经同意不得自行 push**，也不得自行执行 git add/commit。
- 用户未明确要求时，代码修改仅限于工作区内的文件变更，不创建任何提交。

## Quick Links

- [开发指南索引](design/guidelines/README.md) — 全部规范文档目录
