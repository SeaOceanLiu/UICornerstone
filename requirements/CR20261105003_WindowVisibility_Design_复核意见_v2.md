# CR20261105003_WindowVisibility_Design 二次复核意见

- 复核方：CornerstoneDesigner（UI 设计器）
- 日期：2026-11-05
- 对象：`D:/GitSpace/UIControls/design/CR20261105003_WindowVisibility_Design.md`（修订版）
- 前次意见：`Temp/CR20261105003_WindowVisibility_Design_评审意见.md`
- 结论：**有条件通过（设计主体通过；raylib 实际隐藏能力与少量边界需在实现验收中确认）**

## 1. 已闭合

- **B1 子视口保护**：明确仅 `owner == nullptr && ownsBackend == true` 可操作；子视口/共享实例返回 0，不转操作 owner；Create(Hidden) 不传播。
- **B2 GetWindowVisible**：固定为引擎本地可见状态，最小化/遮挡仍 true；与 native handle/focus 无关；旧 DLL Binding 返回 false。
- **B3 后端细节**：SDL3 renderer/首帧/任务栏；SFML 允许一帧闪烁降级；raylib InitWindow 前 flag、headless 不碰全局；本地 m_visible 一致。
- **B4 Launcher**：新 API 不可用时 SDL3 native 降级；失败不得保持隐藏无窗口；ready/显示/退出兜底纳入验收。
- 命名/默认值/线程限制/旧 DLL 降级均已明确，方向可实施。

## 2. 实施时仍需确认

1. **raylib 运行期隐藏**：设计写 `HideWindow/ShowWindow` 或 `SetWindowState(FLAG_WINDOW_HIDDEN)`/`ClearWindowState`。当前 raylib 头未发现 `HideWindow`，需实测 `SetWindowState/ClearWindowState` 是否真正隐藏/显示；若无效，按设计原则返回 0 并标注降级，不能假成功。
2. **`GetWindowVisible` 参数**：`visible == nullptr` 的返回值未定义；建议明确返回 0 或仅返回状态失败，不崩溃。
3. **`SetWindowVisible` 同态调用**：已显示再显示、已隐藏再隐藏应返回 1（幂等）；需测试。
4. **headless owner 判定**：`owner == nullptr` 但 `window == nullptr`（headless）返回 0；与子视口 guard 组合覆盖。
5. **SFML Hidden 首帧**：允许一帧闪烁需要在 Launcher 验收中记录；若 Launcher 不接受闪烁，SFML 只能显示后由平台层处理，不能宣称无闪烁。
6. **C ABI/Binding 旧 DLL**：`IsWindowVisible` 缺失入口时返回 false 的诊断只打印一次，避免启动日志刷屏。
7. **Launcher 降级顺序**：新 API 不存在且 native handle 为空的实例，必须直接显示/不隐藏；不能停留在 hidden 状态。

## 3. 验收补充

- SDL3：Create(Hidden) 无首帧可见；Show/Hide 往返；控件树/布局/主题/焦点字段不变；任务栏无临时窗口。
- 子视口：Set/Get 返回 0；owner 主窗口可见性/焦点/布局不变；Create(Hidden) 不传播。
- headless/共享实例/非法实例/空 visible 指针返回 0，不崩。
- raylib：首实例 Hidden/Show/Hide 实测；headless 第二实例返回 0 且不影响首实例；`SetWindowState` 能力有明确结果。
- SFML：构造后不可见；允许的一帧闪烁记录；运行期往返；旧 DLL Binding false。
- Launcher：ready 前隐藏、ready 后显示；新 API 不可用时 SDL3 native 降级有效；失败/取消不留下隐藏窗口。
- 回归：`auto=2` 272/0、`auto=3`、strict、`test_cr_theme`。

## 4. 放行结论

设计主体通过。实现前把 raylib 隐藏能力实测、`SetWindowVisible` 幂等、`GetWindowVisible` 空指针、headless/子视口边界和 Launcher 降级补进测试；这些不改变 API 方案，不阻塞进入编码。
