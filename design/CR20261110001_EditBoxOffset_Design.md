# CR20261110001 — EditBox 初始 placeholder/文本垂直偏移未刷新（分析 + 设计）

> 状态：**已关闭（2026-11-10；设计器联调通过）**
> 来源：`requirements/UICornerstone_配合修改清单_2026-11.md`（CR20261110001）
> 关联：EditBox / ComboBox / NumericUpDown / TextArea；CR20261102002（content-padding）

## 1. 问题与根因

设计器工具箱搜索框 `edToolSearch` 首次显示 placeholder 时文字偏低、被裁切；聚焦后恢复居中。

`EditBox::updateTextOffset()` 的垂直偏移为：

```cpp
fontHeight = (getTextRenderer() && getFont())
    ? getTextRenderer()->getFontHeight(getFont())
    : m_fontSize * scaleY;
m_textOffsetY = (drawRect.height - fontHeight) / 2.0f;
```

偏移依赖三类状态：

1. 实际字体高度（字体是否已加载、字号是否已应用）；
2. `getContentDrawRect()` 的高度（布局 rect、`content-padding`、缩放）；
3. 当前文本/placeholder（横向偏移依赖文本宽度）。

现有刷新入口包括构造、`create()`、字体变化、`setRect`、缩放、焦点获得/丢失等，但存在遗漏：

- `EditBox::setPlaceholder()` 只更新 `m_placeholderText`，不调用 `updateTextOffset()`；
- `ControlImpl::setContentPadding()` 改变 `getContentDrawRect()`，`EditBox` 未覆写刷新；
- 主题/外部流程在 `create()` 之后改变字体/布局/padding 时，若没有命中上述入口，`m_textOffsetY` 会保持上一次基于 fallback/旧 rect 的值；
- `onFocusGained()` 再次调用 `updateTextOffset()`，所以表现为“聚焦后才居中”。

这不是绘制层单独算错，而是**布局/字体状态变化后缺少统一失效机制**。

## 1.1 基线必失败证据（B1）

设计器最小场景（`edToolSearch`）在**不调用 `SetRect` 刷新**时偏移错误；基线探针按以下方式记录：

1. `create()` 后、首次 `draw()` 前，读取 `m_textOffsetY` 与当前 `(contentHeight - fontHeight)/2`，记录实际值与期望值；
2. 用例分别覆盖：
   - 创建后首次绘制（placeholder、无文本）；
   - `setContentPadding` / 主题 padding 后；
   - `setFontSize` / 字体主题后置；
   - `setPlaceholder` 后；
3. 基线（未修复）必须至少在“后置 padding/字体”或“首帧字体未就绪”用例失败；修复后不依赖设计器 `SetRect` 即成功；
4. 分别临时移除 `setPlaceholder` 显式刷新与 `draw` 前 `ensureTextOffset`，确认两条独立路径都能覆盖对应测试，避免“大范围兜底掩盖真实入口”。

测试可新增 `test_editbox_offset`（测试子类暴露 `m_textOffsetY`；或仅在该测试文件用受控访问方式），输出实际/期望值并在修复前标记 FAIL；不得把测试专用 getter 加入公共 API。

**基线实测（2026-11-10，当前引擎 build/test/Debug）**：

- 场景：`CreateEditBox(200×32)`，设置 placeholder `Search`，创建后调用 `SetContentPadding(4,6,4,6)`；
- `contentDrawRect = 192×20`，`fontHeight = 19.0`；
- 实际 `m_textOffsetY = 6.5`（仍按旧 32 高计算 `(32-19)/2`）；
- 期望 `m_textOffsetY = 0.5`（按新 content 高 `(20-19)/2`）；
- 基线对照：修复前该断言 FAIL；修复后同一用例 PASS，且不调用设计器 `SetRect`。

## 2. 目标与范围

- 新建 EditBox（placeholder、无文本）在 `create` / 主题应用 / 首次绘制时，不聚焦也垂直居中、不裁切；
- 字体加载、主题字号/字体、`content-padding`、布局 resize/锚定、`setPlaceholder` 后偏移及时刷新；
- 聚焦/失焦往返不跳位；
- ComboBox、NumericUpDown 内部 EditBox、TextArea 的既有行为不回归；
- 不改变输入、光标、选择、密码模式语义；不新增公共 API。

## 3. 方案选择

### 方案 A（采用）：补齐失效入口 + 绘制前轻量签名校验

1. **`setPlaceholder` 刷新**
   ```cpp
   void EditBox::setPlaceholder(const std::string& placeholder) {
       m_placeholderText = placeholder;
       updateTextOffset();
   }
   ```

2. **覆写 `setContentPadding`**
   ```cpp
   void EditBox::setContentPadding(Margin padding) override {
       ControlImpl::setContentPadding(padding);
       updateTextOffset();
   }
   ```
   ComboBox/NumericUpDown/TextArea 继承该入口；TextArea 不依赖 `m_textOffsetY`，但保持基类状态一致。

3. **绘制前签名校验（兜底）**
   - 新增缓存字段：字体高度、content rect 宽/高、`getFont()` 指针。
   - `updateTextOffset()` 计算垂直偏移后立即写入签名。
   - 新增 `ensureTextOffset()`：上述字段任一变化才调用 `updateTextOffset()`。
   - `draw()` 开始处调用一次；正常帧只做轻量字段比较，不重复测量文本。
   - 这样即使未来新增“创建后改字体/主题/布局”的路径忘记显式刷新，也能在首次绘制纠正，避免再次出现焦点前后跳位。

4. **保留现有刷新点**
   - `create()`、`loadFontInternal()` 后、`setRect`、`refreshScaleWith`、`setFont/setFontSize`、`setMargin` 现有调用不变；
   - `update()` 的 `m_fontScaleDirty` 路径继续加载字体后刷新。

### 方案 B（排除）：只在 `draw()` 每帧调用完整 `updateTextOffset`

会每帧计算文本宽度/裁剪，开销不必要；且不能解释/修复 placeholder/padding 语义，故不采用。

### 方案 C（排除）：只在设计器临时 `SetRect` 刷新

只修一个控件，不能覆盖 ComboBox/NUD/对话框等；CR 明确要求根修。

## 4. 实现细节

### 4.1 签名字段

在 `EditBox` 增加（protected/private 均可）：

```cpp
float m_offsetFontHeight = -1.0f;
float m_offsetDrawWidth = -1.0f;
float m_offsetDrawHeight = -1.0f;
const Font* m_offsetFont = nullptr;
```

`updateTextOffset()` 计算后写入：`fontHeight`、`getContentDrawRect()` 的宽/高、`getFont()` 指针。

`ensureTextOffset()`：

```cpp
void EditBox::ensureTextOffset() {
    SRect dr = getContentDrawRect();
    float fh = (getTextRenderer() && getFont())
        ? (float)getTextRenderer()->getFontHeight(getFont())
        : m_fontSize * getScaleYY();
    if (fh != m_offsetFontHeight || dr.width != m_offsetDrawWidth ||
        dr.height != m_offsetDrawHeight || getFont() != m_offsetFont)
        updateTextOffset();
}
```

- 签名扩展（B2）覆盖宽度变化（左右 padding/margin）、高度、字体指针/延迟加载；
- 不缓存/比较文本宽度：文本/placeholder 变化已有显式 setter 刷新，避免每帧 `measureText` 开销；
- 签名变化才重算；正常帧只做轻量字段比较；
- `getFont()` 从空变为有效会触发重算。

### 4.2 调用位置

- `draw()` 最前面、`ControlImpl::beforeDraw()` 之后调用 `ensureTextOffset()`（确保使用最终 drawRect/字体）；
- 不改变 `update()` 的动画/焦点逻辑；
- `setPlaceholder` / `setContentPadding` 立即刷新，保证即使不绘制也能得到正确状态（测试/布局探针）；
- `setContentPadding(Margin)` 在 `EditBox` 中为 `public/protected override`；`ControlImpl::setContentPadding` 是 virtual，`setStringProperty("content-padding")`/`SetFloat` 等属性路径最终走虚调用，Theme/属性写回同样覆盖；
- `updateTextOffset()` 计算垂直偏移后**立即写入签名字段**，再做横向文本宽度分支/提前 return；不得因横向分支导致垂直签名未更新。

### 4.3 字体延迟加载的失败闸门（B3/R1）

采用**显式变更驱动 + 单次尝试闸门**，避免“每次绘制都试”与“只试一次不自愈”的矛盾：

- 新增 `m_fontLoadPending`、`m_fontRetryBlocked`；
- 仅在以下**显式字体/尺寸变更**时置 `pending=true`、`blocked=false`：
  - `create()`、`setFont()`、`setFontSize()`、`refreshScaleWith()`；
- `ensureTextOffset()` 中：
  - 若 `getFont()!=nullptr`：正常签名检查；
  - 若 `getFont()==nullptr` 且 `m_fontLoadPending && getTextRenderer() && getResourceProvider()`：
    - `loadFontInternal()` **尝试一次**；
    - 成功：`pending=false`、`blocked=false`，更新 offset；
    - 失败：`pending=false`、`blocked=true`，记录一次失败原因；
  - 若 `pending==false`（已失败闸门关闭）：本帧不再尝试，**不逐帧加载/打印**；
- 恢复路径：资源恢复后必须触发一次显式字体/尺寸变更（如 `setFont`/`setFontSize`/recreate），重新置 pending；或由设计器/宿主重新设置字体；
- 永久不可用：只尝试一次，后续无日志风暴；测试必须覆盖“创建不可用 → 显式变更后恢复”和“永久不可用不逐帧重试”；
- `getFont()` 从空变有效时，签名中的字体指针变化会触发重算。

### 4.4 边界

- `fontHeight` 为 0/负值时沿用现有回退，不新增异常；
- `m_textOffsetY` 仍相对 `getContentDrawRect().top`，不改坐标系；
- 不改变 placeholder 颜色/文本布局策略；
- 不引入新的属性/C ABI/Binding/JSON 键；
- TextArea 的逐行绘制不受影响（它不使用 `m_textOffsetY`）。

### 4.5 TextArea 覆盖路径（B4）

- `TextArea::draw()` 覆盖 `EditBox::draw()`，因此 EditBox 的 draw 前 `ensureTextOffset()` 不会覆盖 TextArea；
- `TextArea` 不使用 `m_textOffsetY`，但继承 `EditBox::setContentPadding()` 时会更新基类 offset；不得把 `ensureTextOffset` 上移到 `ControlImpl` 或 TextArea draw 中，避免影响 `m_lineHeight` 懒检测；
- 增加 TextArea 的 content-padding/字体/字号/rect 回归，确认逐行绘制、行高缓存和滚动条不变。

## 5. 测试/验收

### 5.1 自动化

**B5 测试矩阵**：

1. **基线**：未修复时目标用例必须 FAIL；修复后不依赖设计器 `SetRect` 首帧居中；
2. `edToolSearch` 首帧 placeholder；普通 EditBox、对话框 EditBox；
3. 创建后应用字号 18/20/字体主题、`content-padding`（四边不同值）、resize/锚定；
4. 后置 `setPlaceholder`；聚焦/失焦往返不跳位；
5. ComboBox、NumericUpDown 内部 EditBox 同场景；
6. 字体延迟加载：创建时 provider/font 不可用，显式 `setFont`/`setFontSize`/recreate 后恢复，首帧无焦点也居中；
7. 永久不可用：失败后不逐帧 `loadFontInternal`/打印日志（记录调用次数）；
8. TextArea 的 content-padding/字体/字号/rect 回归，逐行绘制/行高/滚动条不变；
9. 回归 `test_editbox`、`test_combobox`、`test_numericupdown`、`test_textarea`、`test_cr_theme`；`auto=2`/`auto=3`/strict。

测试实现：

- 新增轻量 `test_editbox_offset`，使用测试子类暴露 `m_textOffsetY`（protected）或测试文件内受控访问，断言实际值与 `(contentHeight - fontHeight)/2`；
- 保留设计器 `edToolSearch` 像素/截图验收（无裁切、聚焦前后位置一致）；
- 基线版本先 FAIL，修复后 PASS；仅 auto 回归不算验收；
- 如测试子类无法稳定获得 fontHeight，则用 `getFont()` + `getTextRenderer()` 计算期望值；无 renderer 时使用 `m_fontSize * scaleY` 回退。

### 5.2 手工

- 设计器工具箱搜索框首次显示、主题切换、窗口缩放后 placeholder 居中；
- 点击聚焦/失焦、输入文本/清空、密码模式切换；
- 对话框/ComboBox/NUD 输入框同样检查。

## 6. 兼容与风险

| 风险 | 处理 |
|---|---|
| 每帧签名检查开销 | 仅轻量字段比较；签名不变不重算文本宽度 |
| 字体指针变化但高度相同 | 签名含字体指针，触发重算 |
| `content-padding` 左右变化高度不变 | 显式 `setContentPadding` 刷新；签名含宽度兜底 |
| TextArea 自定义绘制 | 不使用 `m_textOffsetY`，不改其 draw；继承 setContentPadding 刷新；回归验证 |
| 主题/布局在 draw 前才最终确定 | `draw` 前 ensure 兜底 |
| 不新增 API | 仅内部字段/方法，C ABI/Binding/schema 不变 |

## 7. 文档/验收

- 无新增公共 API、JSON 键、schema 属性；用户手册无需新增章节；
- 可在 EditBox 内部注释说明 offset 的失效条件；
- 回归 `auto=2`、`auto=3`、`strict` 与现有控件测试；
- 设计器侧 `edToolSearch` 临时 `SetRect` 规避可在引擎交付后移除并复测。

## 8. 实施顺序

1. `setPlaceholder` / `setContentPadding` 刷新；
2. 签名缓存 + `ensureTextOffset` + draw 前置校验；
3. `test_editbox_offset` 与像素/截图探针；
4. 全量回归、Release 同步、设计器联调。

## 9. 待设计器确认

1. 初始问题是否只在 `edToolSearch`，还是其它 EditBox 也可复现（决定测试覆盖优先级）；
2. 是否接受“draw 前签名校验”的内部实现（不新增 API）；
3. 是否需要额外暴露测试探针/截图对比，或沿用设计器现有像素验收。

## 修订记录

- 2026-11-10 初稿：定位 offset 失效入口（placeholder/content-padding/后置主题/布局）；采用补齐刷新 + draw 前高度签名兜底；不新增 API，待评审。
- 2026-11-10 v1（按评审 B1~B5）：补基线必失败探针；签名扩展宽度/字体指针；补字体延迟加载一次性重试；补 TextArea 覆盖路径与完整验收矩阵。
- 2026-11-10 v2（按复核 R1~R3）：实际基线记录；字体失败闸门明确为显式变更驱动/单次尝试；补 setContentPadding 虚调用与签名写入位置。

## 10. 实施记录（2026-11-10）

- `EditBox::setPlaceholder` / `setContentPadding` 刷新；draw 前 `ensureTextOffset`；签名含字体高度、content 宽/高、字体指针。
- `test_editbox_offset` 通过（基线 6.5 → 期望 0.5）；`test_editbox`/ComboBox/NUD/TextArea 回归通过。
- 全量回归 **60 OK + 4 项预存失败**；Release 已同步，DLL SHA256 `a3fcef001c36975f14c6c75c513fa3e65a3f1e50a71f11e3b27a028f815f7c70`。
- 收尾（2026-11-10）：`isContextMenuHit` 为 `protected virtual`；设计器移除 SetRect 兜底、781/a3f 实测通过；Release `a3fcef00...` 已同步。
