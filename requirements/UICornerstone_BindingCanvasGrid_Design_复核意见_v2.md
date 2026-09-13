# BindingCanvasGrid_Design 第二轮复核意见（v2 评审修正版）

> 复核对象：`design/BindingCanvasGrid_Design.md`（评审修正版 v2，2026-09-13）
> 关联：`CornerstoneDesigner/Temp/UICornerstone_BindingCanvasGrid_Design_评审意见.md`（第一轮）
> 复核日期：2026-09-13
> 复核结论：**通过——放行实施**（第一轮 5 项放行条件全部满足）
> 附：3 条非阻塞建议（实施时可选采纳，不构成放行障碍）

---

## 1. 第一轮问题闭合核验

| 第一轮问题 | v2 处理 | 复核 |
|---|---|---|
| §2.1【阻塞】SDL3 `drawTexture` 忽略 srcRect | §3.0 实施项 I0：恢复 srcRect 分支 + 三后端像素级回归（含 CENTER_CROP） | ✅ 闭合。且复核了完整调用链：`CallbackRenderDevice::drawTexture`（CallbackAdapters.cpp:169，正确转发 srcRect）→ `bridge_drawTexture`（BackendBridge.h:119）→ SDL3 `RenderDevice::drawTexture`（RenderDevice.cpp:247，丢 srcRect）——修复点定位准确 |
| §2.2 TILE 边缘裁剪不能靠渲染器 clip | §3.2：显式 `dst∩drawRect` 求交 + 对应 srcRect 部分裁剪；取整规则（同一步进值）写入 | ✅ 闭合 |
| §2.3 C ABI `ActorSetSourceRect` 必补 | §4.3：定为必补，含清除语义（x<0 或 w≤0 或 h≤0）；DynamicApi 同步 | ✅ 闭合 |
| Q2 取整/缩放叠加、match-parent-rect 语义 | §3.2：取整规则 + source-rect×tile 叠加 + match-parent-rect 仅影响尺寸来源 | ✅ 闭合 |
| §5 性能验收 | §5：1600×1000 + 16×16 实测、预算 <2ms、多单元瓦片降级路径 | ✅ 闭合 |

§8 放行条件 5 项复选框（[x]）与实际章节一一对应，无遗漏。

## 2. 非阻塞建议（3 条）

### 建议 1：source-rect 与既有 4 种 scaleType 的交互补充一句界定

§3.3 的绘制描述 `drawTexture(tex, 有效?&m_sourceRect:nullptr, &drawRect)` 实际只覆盖 STRETCH 路径；
FIT_CENTER / NONE 的尺寸计算（`texW/texH`）与 CENTER_CROP 的裁剪基准未说明是否改用源子区域尺寸。

设计器实际需要的是：**STRETCH + source-rect**（方案 B 移动窗口）与 **TILE + source-rect**（已定义）。
建议补一句界定，例如：
> "本期 source-rect 对 STRETCH 与 TILE 生效；FIT_CENTER / CENTER_CROP / NONE 忽略 source-rect（后续按需扩展）。"
（或补全 4 模式矩阵；不补亦可，但测试范围应与之保持一致。）

### 建议 2：末块部分瓦片的 srcRect 需 clamp；回归补一个非整数缩放用例

- 末块 `srcTail.w = lastDst.w / (tileW / srcW)` 建议 `min(…, srcW)` 钳制，避免浮点误差越界
  （SDL3 `SDL_RenderTexture` 对越界 src 行为不稳定）；
- §5 的 tile 验收（16×16 → 400×300，缩放 1.0）不触发 §3.2 的取整规则；
  建议补一个**非整数复合缩放**用例（如 xScale=1.3：texW 16 → tileW 21），锁定取整/步进/末块行为。

### 建议 3：文档卫生——跨仓库引用路径

文档头部引用的 `CornerstoneDesigner/Temp/UICornerstone_BindingCanvasGrid_Design_评审意见.md`
位于另一仓库的 gitignored 目录，UICornerstone 仓库内不可访问。
建议将评审意见复制进 `requirements/`（或改为按标题引用），保证文档自包含。

## 3. 复核结论

**通过，可进入实施**。建议 1-3 为完善项：其中建议 2 的用例补充成本低、建议纳入 v2 实施清单；
建议 1/3 可按需处理，不影响 I0 与需求 1/2/3 的开发启动。

实施完成后，CornerstoneDesigner 侧将进行接入验证：
- 网格：单元瓦片（`scale-type=tile`）+ resize 零重建；
- 覆盖层：Shape 多图元（选中框/控点/对齐线）；
- 回退路径（性能不达标时）：大纹理 + `ImageSetSourceRect`。
