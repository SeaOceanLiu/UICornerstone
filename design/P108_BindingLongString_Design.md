# P108_BindingLongString_Design — Binding `Control::GetString` 长文本支持（变体 A：GetString 长度查询）

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-10-22 追加：P0-108 低）
> 状态：**已放行，已实施**（2026-10-29；第 3 版复核通过，破坏性 ABI 变更已确认可接受；探针 8/8 + 全量回归基线保持）
> 复核意见：`requirements/引擎P108_P110设计评审.md`（§6/§7）

---

## 1. 现状核实

| # | 事实 | 依据 |
|---|---|---|
| ① Binding 固定 256B | `Control::GetString` 用 `char buf[256]`，长文本**静默截断** ✗ | `binding/src/Control.cpp:82-87` |
| ② C ABI 无长度查询 | `UICornerstone_GetString(..., out, maxLen)` → `strncpy_s(..., _TRUNCATE)`（截断不崩；返回 1/0） | `src/UICornerstoneAPI.cpp:3560-3568` |
| ③ 长文本控件面广 | EditBox/TextArea（原始 JSON、5MB 级）/ ComboBox items / ListView 单元格与列名 / TreeView 标签 | 复核 §1.3 |
| ④ 设计器直连点 | 仅 `CanvasPane.cpp::readControlTextLong`（4KB×4 动态扩缓冲）——本批删除/改写 | 复核 §7.2 |
| ⑤ 消费者范围 | 设计器（唯一外部消费者）+ 仓库内测试/样例/Temp 探针（随构建重编）；无其它预编译应用（已确认） | 用户/设计器确认 |
| ⑥ **测试调用点全量（复核 §7.1-1 修正）** | 仓库内 **11 个测试文件 / 36 处**（13 静态 + 23 动态）+ **Temp 探针 9 文件 / 19 处**（直调）+ `Temp/patch_tests_wfcb.py` 旧签名片段 | 全量 grep（见 §3.1） |

## 2. 方案（变体 A：单一函数，破坏性签名变更）

### 2.1 C ABI：`GetString` 改为长度查询语义
```c
/* P0-108（变体 A）：返回 1 成功；outLen = 完整字节长度（不含 NUL，可大于 cap-1）；
   out 非空且 cap>0 时拷贝 min(cap-1, len) 字节 + NUL；out=NULL 或 cap<=0 时仅查长度。
   outLen 必填（NULL → 返回 0）。
   成功/失败语义（复核 §7.1-2）：
     - 属性支持且存在（含空串）→ 返回 1；空串时 *outLen=0；
     - 非法句柄/属性不支持/outLen==NULL → 返回 0（binding 需区分"空文本"与"读失败"）。
   len 取 strlen(s)；len==INT_MAX 防御（拒绝，返回 0）。文本为 NUL 终止语义
   （内嵌 NUL 不支持——复核 §7.1-8 已注明）。 */
UICORNERSTONE_API int UICornerstone_GetString(UIInstance instance, UIControlHandle ctl,
    const char* prop, char* out, int cap, int* outLen);
```
- **无新增符号**：旧 `GetString` 被替换（破坏性变更，已确认）；
- 引擎 impl：`getStringProperty` → `len = strlen(s)`（`len+1` 溢出保护）→ `*outLen = len` → `min(cap-1, len)` memcpy + NUL → 返回 1；
- **无长度上限**（长度查询 + 精确分配）。

### 2.2 Binding（单一路径 + 短文本快路径）
```cpp
std::string Control::GetString(const char* prop) const {
    if (!IsValid() || !Dyn::API().fnGetString) return std::string();
    int len = 0;
    char stackBuf[512];
    // 快路径（复核 §7.1-6）：短文本一次调用
    if (!Dyn::API().fnGetString(m_state->instance, m_state->handle, prop,
                                stackBuf, (int)sizeof(stackBuf), &len))
        return std::string();
    if (len < (int)sizeof(stackBuf)) return std::string(stackBuf, (size_t)len);
    // 长文本：按 len+1 精确分配再取（复核 §7.1-4：vector 缓冲 + 显式长度）
    std::vector<char> buf((size_t)len + 1);
    int got = 0;
    if (!Dyn::API().fnGetString(m_state->instance, m_state->handle, prop,
                                buf.data(), (int)buf.size(), &got))
        return std::string();
    return std::string(buf.data(), (size_t)got);
}
```
- 全部经 `Dyn::API().fnGetString`（复核 §7.1-4）；无增长重试、无旧 DLL 回退分支（见 §3.2）；
- 空串：`len==0` → 返回 `""`（成功，区别于失败 ✓）。

### 2.3 相关加固（Q-1）
- `TreeViewGetSelectedId` 固定 `char buf[512]`（binding/src/UICornerstone.cpp:715-722）：其 C ABI **不变**（1/0 无长度）→ binding 动态循环以 **`strlen == cap-1`** 判"可能截断"继续扩容（若 ABI 已改则无此问题，但本批不改该函数），直至未填满或 4MB 上限；无选中返回空串。

## 3. 影响面、风险与迁移

### 3.1 调用点全量清单（复核 §7.1-1：repo 全量 grep 为准）
| 类别 | 文件（调用数） | 合计 |
|---|---|---|
| 静态直调 | `test/test_p0_getter.c`（13） | 13 |
| **动态解析（`UIGetStringFn` + GetProcAddress）** | `test_combobox_cabi.cpp`(5) / `test_dialog_cabi.cpp`(1) / `test_fromsource_cabi.cpp`(3) / `test_gaps_cabi.cpp`(2) / `test_image.cpp`(1) / `test_menu_cabi.cpp`(2) / `test_multiinstance_visual_cabi.cpp`(3) / `test_multiviewport_visual_cabi.cpp`(2) / `test_property_cabi.cpp`(3) / `test_treeview_cabi.cpp`(1) | **23** |
| **仓库内测试合计** | **11 文件 / 36 处** + 10 个 typedef | 36 |
| Temp 探针（构建资产，需同步更新或退役） | 9 文件 / 19 处：`temp_actor_resource_probe` / `temp_p027_probe` / `temp_p038_probe` / `temp_p047_probe` / `temp_p083_probe` / `temp_p084_probe` / `temp_p084b_probe` / `temp_p084c_probe` / `temp_p085_probe` | 19 |
| 工具脚本 | `Temp/patch_tests_wfcb.py`（含旧 5 参片段，同步修正避免复发） | — |
| 设计器（外部） | `CanvasPane.cpp::readControlTextLong`（唯一 → 删除/改写；`TextEditorDialog::ReadLong` 改走 `Control::GetString`） | 1 |
| SDK 副本 | `release/binding/src/Control.cpp` / `DynamicApi.h`——**由构建重新生成**（勿手工改副本；release 流程从源构建） | — |

- 动态测试：**typedef `UIGetStringFn` 改 6 参** → 未更新的调用**编译期报错**（杜绝静默）；各调用点加 `outLen`；`RESOLVE(GetString)` 不变。

### 3.2 残余风险与版本要求（复核 §7.1-5 修正表述）
1. **必须同批匹配 DLL/子模块**：新 binding + 旧 DLL **不保证**"返回空串不崩"（x64 多传参可能忽略、x86 栈约定不一致；且旧实现要求 `out!=NULL/maxLen>0`，新调用 `out=NULL` 会直接失败/未填 outLen）。按"ABI 版本不匹配"处理：同批发布 + 强制重编；若有 ABI 版本号则在加载时校验。
2. 旧二进制 + 新 DLL = 栈破坏/UB（含旧测试 exe、导出项目旧二进制）→ 同步后全量重编。
3. 内嵌 NUL 不支持（`strlen` 语义）——文档注明（复核 §7.1-8）。
4. `release/binding` 等 SDK 副本必须由构建生成（复核 §7.1-7），验收含"生成副本与源一致"检查。

### 3.3 文档
- capi/binding 速查：`GetString` 新签名 + 长度查询用法 + "空串=成功/读失败=0"语义；
- ABI 版本说明：破坏性变更记录 + 最低版本匹配要求；
- 设计器回收 `readControlTextLong`（本批即删/改写）。

## 4. 验收与测试矩阵

1. **长文本（任意长度）**：TextArea 5MB / EditBox 长文本 / ComboBox 长项 / ListView 长单元格 / TreeView 长标签 → binding `GetString` 读回完整；
2. **长度查询与拷贝一致**：`GetString(NULL,0,&len)` 后按 `len+1` 取，返回长度 == len；短文本走 512B 快路径（1 次 C ABI 调用）；
3. **空/失败区分**：属性存在但空串 → 返回 1 且 `*outLen==0`（binding 返回 `""` 成功）；非法句柄/属性不支持/`outLen==NULL` → 0；
4. **边界**：255/256/511/512/513/4095/4096/4097 等；`cap` 小于 `len+1` → 截断 + NUL，`outLen` 仍返回完整长度；`len+1` 溢出保护；
5. **UTF-8**：多字节字符完整（无上限）；内嵌 NUL 明确不支持（文档）；
6. **测试全量更新**：10 个动态测试（23 处）+ `test_p0_getter.c`（13 处）编译并全过；Temp 探针（9 文件 19 处）同步更新或退役；`release/binding` 生成副本与源一致；
7. **全量 grep 校验**：`UICornerstone_GetString(` 实参 6 个、`UIGetStringFn` 6 参 typedef、无 5 参残留；
8. **设计器回归**：JSON/代码编辑器长文本（`ReadLong` 改走 `Control::GetString`）；
9. 全量回归仅 4 项预存；Debug/Release 零警告。

## 5. 待确认（Q，含复核答复）

| # | 问题 | 结论 |
|---|---|---|
| Q-1 | `TreeViewGetSelectedId` 一并动态加固 | **采纳**（该 C ABI 不变，`len==cap-1` 判截断） |
| Q-2 | 增长上限 1MB 作为最终上限 | **不采纳**：无上限（长度查询 + 精确分配） |
| Q-3（复核） | 短文本性能（栈优先） | **采纳**：512B 快路径（短文本 1 次调用；长文本 2 次） |
| Q-4（复核） | 上限可观测 | 不适用（无上限） |
| Q-5（用户/设计器） | 直接改 `GetString` 签名（破坏性 ABI） | **采纳**（影响面一处直连 + 全量测试清单） |
| Q-6（复核） | 空串=成功（返回 1/outLen=0）与读失败（返回 0）区分 | **采纳** |
| Q-7（复核） | 旧 DLL 混用 | **不承诺优雅**：按 ABI 版本不匹配处理，强制同批发布/重编 |
