# CR20261105003_WindowVisibility_Design 三次复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-05
- 对象：`D:/GitSpace/UIControls/design/CR20261105003_WindowVisibility_Design.md`（修订版 v2）
- 结论：**通过（仅保留 raylib 实测/编号小修等实施注意）**

## 1. 已闭合

- **子视口/共享实例**：仅 `owner==nullptr && ownsBackend==true` 可操作，子视口返回 0，不转操作 owner；`Create(Hidden)` 不传播。
- **可见性语义**：`GetWindowVisible` 为本地状态，最小化/遮挡仍 true；空指针返回 0；`SetWindowVisible` 幂等成功。
- **三后端**：SDL3 Hidden 创建期/运行期；SFML 允许一帧闪烁降级；raylib InitWindow 前 flag、headless 不碰全局、运行期能力待实测。
- **Launcher 降级**：新 API 缺失、native handle 空/非 SDL3 时直接显示，不留下隐藏窗口。
- **旧 DLL**：Binding false，诊断只打印一次。
- 设计主体可进入实现。

## 2. 实施注意

1. **raylib 运行期隐藏**仍是唯一能力不确定项：`SetWindowState(FLAG_WINDOW_HIDDEN)`/`ClearWindowState` 是否真正隐藏/显示必须在实现前实测；无效返回 0 并标注降级，不能假成功。
2. **SFML 首帧闪烁**：Launcher 验收记录；若要求无闪烁，SFML 只能平台层处理。
3. **测试列表编号**：§5 当前出现重复的 6/7 编号，建议整理（不影响设计语义）。
4. **`Create` 复用已有窗口**（`CreateSDL3WindowFromExisting` 等）时 Hidden 的行为未提；Launcher 不使用复用窗口，可标注不支持/不承诺。
5. **`GetWindowVisible` 空指针/幂等/headless** 已写；实现时补充“非法实例不崩”的 C ABI 测试。
6. **`SetWindowVisible` 失败原因**建议诊断（backend unsupported/invalid/headless），便于 Launcher 降级判断。

## 3. 放行结论

**通过**。实现前完成 raylib 实测、修正测试编号，并按已列验收矩阵回归；Launcher 集成可并行使用 SDL3 native 临时方案，CR20261105003 交付后切换。
