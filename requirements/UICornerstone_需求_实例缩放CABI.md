# UICornerstone 需求：实例缩放 C ABI（画布缩放语义）

- 提出方：CornerstoneDesigner（UI 设计器）
- 日期：2026-09-15
- 优先级：P1（阻塞设计器画布缩放的正确实现，见"背景与现状"）
- 关联仓库：UICornerstone（本需求仅涉及 C ABI 与 binding，引擎核心机制已存在，无需改动）

## 1. 背景与现状

设计器中央画布为**子视口**（viewport instance），内含被设计的控件。画布缩放（工具栏 -/+ 档位）期望语义：

- 控件 rect **按比例跟随**（视觉位置/大小变化）；
- 控件内**文本字号同步跟随**；
- 控件的**逻辑 rect 保持不变**（模型存逻辑坐标，缩放是视图变换）。

调研确认引擎**已原生支持**该语义（无需新增核心机制）：

| 机制 | 位置（UIControls 源码） |
|---|---|
| 逻辑坐标渲染 | `Bench::getDrawRect() = { rect × scale }`（Bench.cpp:229） |
| 字号自动复合 | `Label.cpp:314`: `scaledSize = m_fontSize * getScaleXX()` |
| 父链级联复合 | `ControlImpl` 构造：`m_xxScale = xScale * parent->getScaleXX()` |
| scale 变更自动重建文本 | `Label::refreshScaleWith`（Label.cpp:497-510，含 `m_fontScaleDirty` 不可见延后重建） |
| 根缩放入口（画布语义） | `Bench::setScaleX/setScaleY`（Bench.cpp:214-227，注释原文："根：布局缩放 = 复合缩放（无父级），且必须保持 m_rect 不变（画布语义）"） |
| 整树刷新 | `ControlImpl::refreshScaleWith`（ControlBase.cpp:453-459）+ Button/EditBox 等各自覆写收口 |

**缺口**：`Bench::setScaleX/setScaleY` 未暴露到 C ABI（`UICornerstoneAPI.h` 无任何 setScale 接口；binding `Control.h` 同样没有），应用层无法调用。

设计器当前被迫采用像素坐标 + scale=1 + 手动同步 rect/字号的旁路方案（每类控件字号属性名不统一：`font-size`/`caption-size`，维护成本高且部分控件无读接口）。

## 2. 需求内容

### 2.1 C ABI（UICornerstoneAPI.h）

```c
/* ── 实例缩放（画布语义）── */
// 设置实例根缩放（bench 的 m_xScale/m_yScale 与复合快照），并触发整棵控件树
// refreshScaleWith（Label 等文本控件自动按新复合缩放重建字号）。
// 控件逻辑 rect 不变（画布语义：缩放是视图变换，见 Bench::setScaleX 注释）。
// 返回 1 成功，0 参数无效。
UICORNERSTONE_API int UICornerstone_SetInstanceScale(UIInstance instance,
                                                     float xScale, float yScale);

// 读取当前实例根缩放（未设置过返回 1,1）。
UICORNERSTONE_API int UICornerstone_GetInstanceScale(UIInstance instance,
                                                     float* outXScale, float* outYScale);
```

### 2.2 语义要求

1. **实现**：透传 `bench->setScaleX(xScale); bench->setScaleY(yScale);`（或合 并单次遍历，若两连调用会重复整树刷新则建议 Bench 提供 `setScale(float x, float y)` 合并入口，避免两次 O(n) 递归）。
2. **不改变 bench m_rect**：保持"画布语义"（视口尺寸与缩放解耦）。
3. **幂等**：与当前值相等时快速返回（不触发整树刷新）。
4. **子视口可用**：viewport instance 与主实例行为一致（设计器主场景是子视口）。

### 2.3 binding（UICornerstone.h / UICornerstone.cpp）

```cpp
// ── 实例缩放（画布语义）──
bool SetInstanceScale(float xScale, float yScale);
float GetInstanceScaleX() const;   // 或 std::pair<float,float> GetInstanceScale() const;
float GetInstanceScaleY() const;
```

## 3. 验收条件（放行条件）

1. **放置后缩放**：子视口内经工厂创建（非 LoadLayout）的控件，`SetInstanceScale(1.5, 1.5)` 后：
   - `GetRect()` 返回值 × 1.5（渲染坐标），控件内文本字号 × 1.5（Label/Button/EditBox 各抽一）；
   - 控件**逻辑 rect**（再次 `SetInstanceScale(1,1)` 后 `GetRect()`）与缩放前一致。
2. **字号重建**：缩放后 Label 文本清晰（无拉伸模糊，即重新光栅化而非位图缩放）——由 Label::refreshScaleWith 既有逻辑保证，抽验即可。
3. **幂等无害**：相同 scale 重复调用无副作用、无可感知开销。
4. **带内部子控件的复合控件**（Button 的 caption、CheckBox 的 box、ComboBox 等）字号/布局随缩放一致——抽验 Button/CheckBox/ComboBox/NumericUpDown 四类。
5. **回归**：主实例（设计器自身 UI）不调用该接口时行为与现状完全一致。

## 4. 设计器侧配合改动（接口就绪后，我方实施）

1. `createViewportControl` 改用**逻辑 rect** 创建（去掉 `× m_zoom`）；
2. `CanvasPane::SetZoom` 改为调 `SetInstanceScale(zoom, zoom)` + 选中框同步；
3. **删除**手动同步代码：SetZoom 的 rect 循环、`writeFontPx`/`readFontPx`、`extended["font-size"]` 记录；
4. 属性面板 x/y/宽/高 直读逻辑坐标（模型），语义不变。

## 5. 参考

- 引擎机制评审记录：见 `Temp/UICornerstone_BindingCanvasGrid_Design_复核意见_v2.md`（画布网格需求，同属画布渲染体系）
- 设计器当前旁路方案：CornerstoneDesigner `src/CanvasPane.cpp` 的 `SetZoom`（fe35965..366f3c3 引入的手动同步，接口就绪后删除）
