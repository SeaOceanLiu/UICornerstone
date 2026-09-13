# BindingCanvasGrid_Design 评审意见（CornerstoneDesigner → UICornerstone）

> 评审对象：`design/BindingCanvasGrid_Design.md`（状态：待审核）
> 需求来源：`requirements/UICornerstone_binding需求_画布网格图元纹理.md`
> 评审日期：2026-09-13
> 评审结论：**基本通过**——需求 1/2/3/5 已覆盖、需求 4 延后合理；
> 存在 1 个必须处理的前置缺陷（SDL3 `drawTexture` 忽略 srcRect）与 2 处需澄清/修正的细节，
> 修正后即可进入实施。

---

## 1. 逐条需求核对

| 需求 | 设计回应 | 评审结论 |
|------|---------|---------|
| 1. Shape 多图元 Binding 完整封装 | §3.1/§4.1：五方法一一对应 C ABI（`include/UICornerstoneAPI.h:493-504` 已存在），零引擎改动 | ✅ 完全满足，签名与需求逐字一致 |
| 2. 纹理平铺（Tile） | §3.2/§4.2：新增 `kScaleTypeTile` + Actor 内循环 `drawTexture` 铺满 | ⚠️ 方向正确，但边缘裁剪机制描述有误（见 §2.2） |
| 3. 图片源矩形（Source Rect） | §3.3/§4.3：Actor 成员 + C ABI + Binding 方法 | ❌ 前置依赖不成立：SDL3 后端当前忽略 srcRect（见 §2.1） |
| 4. 原始像素纹理（raw RGBA） | §3.4：本期不实施，保留扩展点 | ✅ 有条件同意（见 §4 Q1） |
| 5. 选型组合建议 | §3.5：A（tile）+ C（Shape 图元）实施；B 部分实施 | ✅ 与需求建议一致 |

---

## 2. 必须处理的问题

### 2.1【阻塞】SDL3 后端 `drawTexture` 忽略 `srcRect`

**位置**：`src/backend/sdl3/RenderDevice.cpp:247`

```cpp
void drawTexture(Texture* texture, const SRect* srcRect, const SRect* dstRect) override {
    if (!m_renderer || !texture || !dstRect) return;
    SDL3Texture* sdlTex = static_cast<SDL3Texture*>(texture);
    SDL_FRect sdlDst = { dstRect->left, dstRect->top, dstRect->width, dstRect->height };
    SDL_RenderTexture(m_renderer, sdlTex->native(), nullptr, &sdlDst);   // ← srcRect 被丢弃
}
```

**证据链**：
- `git log -L 247,253:src/backend/sdl3/RenderDevice.cpp`：`4f3d92e`（2026-06-15，RGBA8888/DLL bridge 提交）删除了 `if (srcRect) { ... }` 分支，之后未恢复。
- SFML（`src/backend/sfml/RenderDevice.cpp:532`）与 raylib（`src/backend/raylib/RenderDevice.cpp:585`）均正确使用 srcRect。
- Actor 的 `CENTER_CROP` 分支（`src/Actor.cpp:215`）正在传 srcRect——即该模式在 SDL3 后端目前就是坏的（整图拉伸而非居中裁剪）。

**影响**：
- 设计 §2 断言"drawTexture 已支持 srcRect（CENTER_CROP 在用）；三后端零改动"**对 SDL3 不成立**。
- CornerstoneDesigner 运行在 SDL3：不修复则需求 3（source-rect）完全无效；需求 2（tile）的末行/末列边缘裁剪同样无法实现（见 §2.2）。

**要求**：
1. 恢复 SDL3 `drawTexture` 的 srcRect 分支（`SDL_RenderTexture(..., &sdlSrc, &sdlDst)`）；
2. 补像素级回归测试：三后端各渲染一次 CENTER_CROP / source-rect，断言裁剪区域内容（不能只测 API 返回值）；
3. 设计文档 §2、§6 相应修正。

### 2.2 TILE 边缘裁剪机制须明确（不能依赖渲染器 clip）

设计 §3.2 称"dst 裁剪由渲染器 clip 保证"。**不成立**：clip 为视口/裁剪栈级，**无控件级裁剪**，末行/末列瓦片会溢出控件矩形（画到相邻面板上）。

**要求**：在 Actor 的 TILE 分支显式实现：
- 末行/末列瓦片：`dst 与 drawRect 求交` + 对应 `srcRect` 部分裁剪（依赖 §2.1 的修复）；
- 验收标准保留"边缘格正确裁剪"，并补一条边缘像素断言测试。

### 2.3 C ABI `ActorSetSourceRect` 必须补（非可选）

设计 §4.3/§7-Q3 将 C ABI 标注为"可选（若设计器仅走 Binding 可免）"——**方向反了**：

- Binding 是**纯 C ABI 动态封装**（`binding/src/UICornerstone.cpp` 经 `Dyn::API().fnXxx` 调用，见 `binding/src/DynamicApi.h` 现有 `fnShapeSetPoints` 同款模式）。
- `ImageSetSourceRect` 要落地，**必须**有对应 C ABI 导出可解析；否则 Binding 无实现路径（属性系统又已决定不设键）。

**要求**：补 `UICornerstone_ActorSetSourceRect(inst, handle, x, y, w, h)`（x<0/w≤0 等值清除语义写清），并在 DynamicApi.h/.cpp 增加函数指针与 RESOLVE。

---

## 3. 逐项设计确认（无异议部分）

- **§3.1 Shape 五方法**：与需求签名逐字一致；"零引擎改动、仅 Binding 四文件"判断正确。
- **§3.3 属性系统不设键、走 Binding 专用方法**：与 `ShapeSetPoints` 先例一致，认可。
- **§3.4 raw RGBA 延后**：实施路径（ResourceProvider 加格式标记 + Texture 加载分支）描述正确；保留扩展点即可。
- **§3.5 选型组合表**：与需求建议一致。

---

## 4. 对 §7 三个待审核问题的答复

**Q1：需求 4（raw RGBA）本期不实施——同意？**
**同意**。前提条件：
- §2.1（SDL3 srcRect）修复后 tile 方案完整可用（含边缘裁剪）；
- 若 §5 性能验收在"大画布 + 小瓦片"场景不达标，方案 B（大纹理 + source-rect）需重新纳入，
  届时 raw RGBA（或大 PNG 复用）作为其支撑一并复议。

**Q2：source-rect 与 tile 组合语义 = 从子区域取瓦片铺平——确认？**
**确认**。补充要求：
- 语义顺序：先按 source-rect 从纹理取子区域，再以该子区域为瓦片平铺；source-rect 空 = 整图；
- 明确与 `xScale/yScale` 的叠加关系（tile 分块尺寸 = 瓦片逻辑尺寸 × 控件复合缩放，取整规则写明，避免 1px 缝隙/重叠）；
- 明确与 `match-parent-rect` 的组合行为。

**Q3：C ABI 是否补 `ActorSetSourceRect`？**
**必须补**（理由见 §2.3），并非可选。

---

## 5. 性能验收补充建议

TILE 每瓦片一次 `drawTexture`，帧内调用数 ≈ 控件面积 / 瓦片面积。极端例：25% 缩放、10px 瓦片、
708×704 画布 ≈ **4900 次/帧**——可能不达"性能恒常数"预期。建议验收补充：

1. 增加"大画布（如 1600×1000）+ 小瓦片（如 16×16）"帧耗时实测，给出预算（如 < 2ms）；
2. 允许并推荐应用侧使用**多单元瓦片**（如 4×4 个网格单元合成一张瓦片，调用数降为 1/16），
   文档注明该用法；
3. 若实测仍不达标：后续以方案 B（大纹理 + source-rect）替代 TILE 作为网格铺底，raw RGBA 复议。

---

## 6. 结论与放行条件

设计**基本通过**。放行条件（全部满足即可实施）：

- [ ] §2.1 SDL3 `drawTexture` srcRect 修复 + 三后端像素级回归（含 CENTER_CROP）；
- [ ] §2.2 TILE 边缘裁剪实现方案写清（dst∩drawRect + srcRect 部分裁剪）；
- [ ] §2.3 明确 `ActorSetSourceRect` C ABI 必补；
- [ ] §4 三问答复纳入设计（Q2 的取整/缩放叠加语义、Q3 的 C ABI 结论）；
- [ ] §5 性能验收场景补充（大画布 + 小瓦片）。

CornerstoneDesigner 侧待本轮引擎落地后进行接入验证（网格 tile 化 + 覆盖层 Shape 图元化）。
