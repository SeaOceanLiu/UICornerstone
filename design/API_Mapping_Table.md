# UICornerstone API 全面映射表

版本:2026-08-20 · 覆盖:核心引擎内部 API ↔ JSON ↔ 属性系统 ↔ C ABI ↔ C++Binding

## 读表约定(缩写与图例)

- **内部API**列:方法名(去签名、去 get 对);引擎管线方法(create/draw/handleEvent 等)不在表内。
- **JSON** 列:声明式 UI 字段;— = 声明式不可表达。
- **属性**列:PropertyNames 属性键(即 `set*Property` 分发的键名)。
- **CABI** 列:有属性键 → 只写入口类型 `SetString/SetInt/SetFloat/SetBool/SetEnum/SetColor/SetStateColor/SetPtr/SetCallback`(键名见"属性"列,`Get*` 对称存在);专用函数 → 短名(完整名为 `UICornerstone_<短名>`)。
- **Binding** 列:有属性键 → 入口类型与 CABI 相同(经 `Control::`);专用 → `UICornerstone::<短名>` 或 `Control::<短名>`。
- **缺口**列:✅已覆盖 · 🔧专用CABI · ⚠️需补CABI/Binding · ⛔引擎内部/派生查询(无需)。
- 事件回调:CABI `SetCallback("事件名")`、Binding `Control::SetCallback("事件名")`、JSON `events.onXxx`。

---

## 1. 基类 Control(通用)

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setRect | rect | — | SetRect | SetRect | 🔧 |  |
| moveTo/resizeTo/setLeft/… | — | — | — | — | ⛔便捷组合 |  |
| setScaleX/Y | scale | — | — | — | ✅JSON | 属性 |
| setVisible | visible | visible | SetBool | SetBool | ✅ |  |
| setEnable | enabled | enabled | SetBool | SetBool | ✅ |  |
| setTransparent | transparent | transparent | SetBool | SetBool | ✅ |  |
| setBorderVisible | borderVisible | border-visible | SetBool | SetBool | ✅ |  |
| setBg/Border/Text/TextShadowStateColor | colors.4态 | background/border/text/text-shadow(+子键) | SetStateColor | SetStateColor | ✅ |  |
| 16个逐状态颜色setter | 同上 | 同上(子键) | SetStateColor | SetStateColor | ✅可替代 |  |
| setAlwaysOnTop | — | always-on-top | SetBool | SetBool | ✅ | 属性 |
| setState | — | state | SetEnum("state") | SetEnum("state") | ✅ | 属性 |
| setFocused/focusable/tabIndex | — | focusable/tab-index | SetBool/SetInt | SetBool/SetInt | ✅ | 属性 |
| setShowFocusRing/…/setFocusBoundary | — | show-focus-ring/focus-ring-always-visible/focus-ring-color/focus-ring-style/focus-boundary | SetBool/SetColor/SetEnum | 同 | ✅ | 属性 |
| addControl/removeControl | children | — | AddChildControl | — | 🔧 |  |
| setParent | — | — | — | — | ⛔ |  |
| setMargin | margin | margin-left/top/right/bottom | SetFloat("margin-*") | SetFloat("margin-*") | ✅ | 属性 |
| setId/getId | id | — | GetControlId | — | 🔧 | 暂不需要(见文末决策) |
| getControlType | type | control-type(只读) | GetString("control-type")并存GetControlType | GetString("control-type") | ✅ | 保留CABI(见文末决策) |
| isContainsPoint/map*/setContext | — | — | — | — | ⛔内部 |  |

## 2. Actor(Image)

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| loadFromFile/Resource | image/imageResource | image/image-resource | SetString | SetString | ✅ |  |
| loadTextureFromSurface/setTexture | — | — | — | — | ⚠️对象注入 | CABI/Binding |
| setScaleType | scaleType | scale-type | SetEnum（stretch/fit-center/center-crop/none/**tile** v1.1.1） | SetEnum | ✅ |  |
| setSourceRect/clearSourceRect | sourceRect（后续） | — | ActorSetSourceRect（清除 x<0/w≤0/h≤0） | ImageSetSourceRect（v1.1.1） | ✅专用 |  |
| setMatchParentRect | matchParentRect | match-parent-rect | SetBool | SetBool | ✅ |  |
| setAlpha | — | alpha | SetInt | SetInt | ✅ | JSON |
| setAnchorPoint(枚举) | anchor | anchor | SetEnum | SetEnum | ✅方向锚 |  |
| setAnchorPoint(x,y) | — | anchor-x/anchor-y | SetFloat("anchor-x"/"anchor-y") | SetFloat(...) | ✅ | 属性 |
| setRect | rect | — | SetRect | SetRect | 🔧 |  |

## 3. Button

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setCaption | caption | caption | SetString | SetString | ✅ |  |
| setCaptionSize | captionSize | caption-size | SetFloat | SetFloat | ✅ |  |
| setCaptionLabel | captionLabel | — | — | — | ⚠️对象注入 | 属性 |
| setTextShadowEnable | enableTextShadow | text-shadow-enable | SetBool | SetBool | ✅ |  |
| setTextStateColor | colors.text | text | SetStateColor | SetStateColor | ✅ |  |
| 4个状态Actor | actors.* | normal/hover/pressed/disabled-image | SetString | SetString | ✅路径式 |  |
| setLuotiAni | luotiAni | animation | SetString | SetString | ✅路径式 |  |
| setOnClick | onClick | click | SetCallback | SetCallback | ✅ |  |
| getCaptionRect | — | — | — | — | ⛔查询 |  |
| CreateImageButton(n,h,p) | →Button+3态Actor | — | CreateImageButton | CreateImageButton | 🔧工厂 | 暂不需要 |
| CreateAnimatedButton(jsonc) | →Button+luotiAni | — | CreateAnimatedButton | CreateAnimatedButton | 🔧工厂 | 暂不需要 |

## 4. CheckBox

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setCheckState | checkState | check-state | SetEnum | SetEnum | ✅ |  |
| setTriStateEnabled | triState | tri-state | SetBool | SetBool | ✅ |  |
| setStyle | style | style | SetEnum | SetEnum | ✅ |  |
| setLayout | layout | layout | SetEnum | SetEnum | ✅ |  |
| setVerticalAlign | verticalAlign | vertical-align | SetEnum | SetEnum | ✅ |  |
| setSizeRatio | sizeRatio | size-ratio | SetFloat | SetFloat | ✅ |  |
| setCaptionSize | captionSize | caption-size | SetFloat | SetFloat | ✅ |  |
| 4个标记颜色 | checkColor/crossColor/indeterminateColor/boxBorderColor | check/cross/indeterminate/box-border | SetColor | SetColor | ✅ |  |
| setOnCheckChanged | onCheckChanged | check-changed | SetCallback | SetCallback | ✅ |  |

## 5. ColorPicker

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setColor | color | color | SetString | SetString | ✅ |  |
| setPresetColors | presets | — | — | — | ✅JSON静态 |  |
| setPresetLayout | presetLayout | preset-cols/preset-rows | SetInt | SetInt | ✅ |  |
| setClosedSwatchSize | swatchSize | closed-swatch-size | SetFloat | SetFloat | ✅ |  |
| setClosedFontSize | closedFontSize | closed-font-size | SetInt | SetInt | ✅ |  |
| setClosedTextColor | closedTextColor | closed-text | SetColor | SetColor | ✅ |  |
| setPopupBGColor | popupBGColor | popup-bg | SetColor | SetColor | ✅ |  |
| openPopup(私有) | — | — | — | — | ⛔交互驱动 | 暂不需要 |
| isPopupVisible | — | popup-visible | GetBool("popup-visible") | GetBool("popup-visible") | ✅ | 属性 |
| setOnColorChanged | onColorChanged | color-changed | SetCallback | SetCallback | ✅ |  |

## 6. ComboBox

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setItems | items | — | — | — | ✅JSON静态 | CABI/Binding |
| addItem/removeItem/clearItems | — | — | ComboBoxAddItem/ComboBoxRemoveItem/ComboBoxClearItems | 同 | 🔧专用CABI | CABI/Binding |
| setSelectedIndex | selectedIndex | selected-index | SetInt | SetInt | ✅ |  |
| setSelectedValue | — | selected-value | SetString | SetString | ✅ |  |
| setPlaceholder | placeholder | placeholder | SetString | SetString | ✅ |  |
| setEditable | editable | editable | SetBool | SetBool | ✅ |  |
| setCycleEnabled | cycleEnabled | cycle-enabled | SetBool | SetBool | ✅ |  |
| 箭头/项/列表尺寸 | arrowWidth/itemHeight/maxVisibleItems | arrow-width/item-height/max-visible-items | SetFloat×2/SetInt | 同 | ✅ |  |
| 7个颜色 | arrow/item-*… | arrow/arrow-hover/item-selected/item-hover/item-disabled/list-bg/list-border | SetColor | SetColor | ✅ |  |
| openPopup(私有) | — | — | — | — | ⛔交互驱动 | 暂不需要 |
| getSelectedLabel/getItems/… | — | selected-label | GetString("selected-label") | GetString("selected-label") | ✅ | 属性 |
| getListPanel/getListScrollBar | — | list-panel/list-scroll-bar | GetPtr("list-panel"…) | GetPtr("list-panel"…) | ✅ | 属性 |
| openPopupForTest | — | — | — | — | ⛔测试入口 | 暂不需要 |
| setOnSelectionChanged | onSelectionChanged | selection-changed | SetCallback | SetCallback | ✅ |  |

## 7. Dialog / Popup / ConfirmPopup

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| open/close | dialogs声明 | visible(open/close)/popup-visible/result | SetBool("visible")→open+close;GetBool("popup-visible");GetInt("result") | 同 | ✅ | 属性 |
| isPopupVisible/getResult | — | popup-visible/result | GetBool("popup-visible")/GetInt("result") | 同 | ✅ | 属性 |
| getConfirmButton/getCancelButton | — | confirm-button/cancel-button | GetPtr("confirm-button"…) | GetPtr("confirm-button"…) | ✅ | 属性 |
| recreateButtons | — | — | — | — | ⛔内部 | 暂不需要 |
| setCentered/Anchored/Absolute | centered | centered-mode | SetEnum | SetEnum | ✅(offset需JSON) |  |
| setContent | content | content | SetPtr | SetPtr | ✅ |  |
| setCloseOnClickOutside/Esc | closeOnClickOutside/closeOnEsc | close-on-click-outside/close-on-esc | SetBool | SetBool | ✅ |  |
| ConfirmBtn可见/文本/尺寸 | confirmButton/buttonHeight/buttonGap | confirm-visible/confirm-text/button-height/button-gap | SetBool/SetString/SetFloat×2 | 同 | ✅ |  |
| setCancelButtonText | cancelButton | cancel-text | SetString | SetString | ✅ |  |
| 按钮矩形/setPadding | — | — | — | — | ⚠️ | 属性 |
| setOnClose/Confirm/Cancel | onClose/onConfirm/onCancel | close/confirm/cancel | SetCallback | SetCallback | ✅ |  |

## 8. EditBox

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setText | text | text | SetString | SetString | ✅ |  |
| setPlaceholder | placeholder | placeholder | SetString | SetString | ✅ |  |
| setPasswordMode | passwordMode | password-mode | SetBool | SetBool | ✅ |  |
| setPasswordChar | passwordChar | password-char | SetInt("password-char") | SetInt("password-char") | ✅ | 属性 |
| setFont/fontSize/alignment | font/size/alignment | font/font-size/align | SetEnum/SetInt/SetEnum | 同 | ✅ |  |
| setMargin | margin | margin-left/top/right/bottom | SetFloat("margin-*") | SetFloat("margin-*") | ✅ | 属性 |
| selectAll/setSelection/… | — | — | EditBoxSelectAll/EditBoxSetSelection/… | EditBoxSetSelection等 | 🔧专用CABI | CABI/Binding |
| copy/cut/paste | — | — | EditBoxCopy/EditBoxCut/EditBoxPaste | 同 | 🔧专用CABI | CABI/Binding |
| getCursorPosition | — | — | — | — | ⛔查询 |  |
| setOnTextChanged | onTextChanged | text-changed | SetCallback | SetCallback | ✅ |  |
| setOnEnter | onEnter | enter | SetCallback | SetCallback | ✅ |  |

## 9. HandleControl(编辑器辅助,建议豁免)

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setTarget/detach | — | — | CreateHandleControl(target,…) | CreateHandleControl | 🔧绑定 | 已满足 |
| setHandleSize/MinSize/3色/4可见性 | — | — | — | — | ⚠️全部无键 | 暂不需要 |

## 10. Label

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setCaption | caption/text | caption | SetString | SetString | ✅ |  |
| setFont | font | font | SetEnum | SetEnum | ✅ |  |
| setFontSize | size | font-size | SetInt | SetInt | ✅ |  |
| SetFontStyle | font.style（数组组合） | font-style + font-bold/font-italic/font-underline/font-strikethrough | SetEnum("font-style") / SetBool("font-bold")… | 同（SetEnum/SetBool） | ✅ | 组合=JSON数组或 Bool 分位 |
| setAlignmentMode | alignment | align | SetEnum | SetEnum | ✅ |  |
| setShadow | shadow | shadow | SetBool | SetBool | ✅ |  |
| setShadowOffset | — | shadow-offset-x/shadow-offset-y | SetFloat | SetFloat | ✅ | JSON/属性 |
| setLineHeight | lineHeight | line-height | SetInt | SetInt | ✅ |  |
| setLineSpacingRatio | lineSpacingRatio | line-spacing-ratio | SetFloat | SetFloat | ✅ |  |
| setEnableExpand | enableExpand | expand | SetBool | SetBool | ✅ |  |
| setClickable | — | clickable | SetBool | SetBool | ✅ |  |
| setDebugDraw | debugDraw | debug-draw | SetBool("debug-draw") | SetBool("debug-draw") | ✅ | 属性 |
| setOnClick | onClick | click | SetCallback | SetCallback | ✅ |  |
| setOnPropertyChanged | — | property-changed | SetCallback | SetCallback | ✅ | JSON |
| loadFromFile(字体) | fontFile/fontResource | font-file/font-resource | SetString | SetString | ✅ |  |
| getHotRect | — | — | — | — | ⛔查询 |  |

## 11. LuotiAni(Animation)

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| loadFromFile/Resource | luotiAni/animation | animation | SetString | SetString | ✅ |  |
| play/pause | playing | playing | SetBool | SetBool | ✅ |  |
| resume | — | — | — | — | ⚠️语义区分(playing 可部分表达) | 属性 |
| prepare(startFrame) | — | — | AnimationPrepare(startFrame) | AnimationPrepare(startFrame) | 🔧专用CABI | CABI/Binding |
| setFrame | frame | frame | SetInt | SetInt | ✅ |  |
| setLoop | loop | loop | SetBool | SetBool | ✅ |  |
| setFrameFilter | — | — | AnimationSetFrameFilter(flag) | AnimationSetFrameFilter(flag) | 🔧专用CABI | CABI/Binding |
| getTotalFrames/… | — | total-frames/current-frame | GetInt("total-frames"…) | GetInt("total-frames"…) | ✅ | 属性 |
| AnimationEnded 回调 | — | — | SetCallback（kEventAnimationEnded，intVal=m_id） | SetCallback + Event::IsAnimationEnded/GetAnimationEndedId | ✅ |  |

## 12. MenuBar / MenuPanel / MenuItem

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| MenuBar::addMenu/removeMenu | menus | — | MenuBarAddMenu | MenuBarAddMenu | 🔧 |  |
| setBarHeight | barHeight | bar-height | SetFloat | SetFloat | ✅ |  |
| setManualPosition | manualPosition | manual-position | SetBool | SetBool | ✅ |  |
| setItemHeightRatio/fontSize | — | item-height-ratio/label-font-size | SetFloat | SetFloat | ✅ |  |
| enterMenuMode/…/closeAllMenus | — | — | — | — | ⚠️命令式 | 暂不需要 |
| MenuPanel::addItem/addSeparator | menus内items | — | MenuPanelAddItem/AddSeparator | MenuPanelAddItem/AddSeparator | 🔧 |  |
| show/hide/setPosition | — | visible | SetBool | SetBool | ✅位置⚠️ |  |
| setFontSize/ItemHeightRatio/FontName | — | label-font-size/item-height-ratio/font | SetFloat×2/SetEnum | 同 | ✅ | JSON |
| hitTest/getItemAt/… | — | — | — | — | ⛔查询 |  |
| MenuItem::setCaption/Shortcut/Checked | caption/shortcut/checked | caption/shortcut/checked | SetString×2/SetBool | 同 | ✅ |  |
| setItemId | — | item-id | SetString | SetString | ✅ |  |
| setSubMenu | 嵌套menus | — | MenuItemSetSubMenu | MenuItemSetSubMenu | 🔧 |  |
| setLeadingControl/Gap/FontName/OwnFontSize | leadingControl/leadingGap/font/size | item-leading-control/item-leading-gap/item-font/item-font-size | SetPtr/SetFloat/SetEnum/SetInt | 同 | ✅ |  |
| setOnClick | onClick | click | SetCallback | SetCallback | ✅ |  |
| closeMenuChain | — | — | — | — | ⚠️命令式 | 暂不需要 |

## 13. NumericUpDown

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setValue | value | value | SetFloat | SetFloat | ✅ |  |
| setRange | range.min/max | range-min/range-max | SetFloat | SetFloat | ✅ |  |
| setStep | step | step | SetFloat | SetFloat | ✅ |  |
| setPageStep | pageStep | page-step | SetFloat | SetFloat | ✅ |  |
| setDecimals | decimals | decimals | SetInt | SetInt | ✅ |  |
| setReadOnly | readOnly | read-only | SetBool | SetBool | ✅ |  |
| setButtonWidth | buttonWidth | button-width | SetFloat | SetFloat | ✅ |  |
| setArrowColor | arrow+… | arrow/arrow-hover/arrow-pressed | SetColor | SetColor | ✅ |  |
| stepValue | — | — | NumericUpDownStep(dir) | NumericUpDownStep(dir) | 🔧专用CABI | CABI/Binding |
| setOnValueChanged | onValueChanged | value-changed | SetCallback | SetCallback | ✅ |  |

## 14. Panel

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| addControl/removeAll | children | — | AddChildControl | — | 🔧 |  |
| setLayoutEngine | layout(type/gap/padding/columns/rows) | layout | SetEnum | SetEnum | ✅参数可声明 |  |
| 3个setChild*Props | flowWeight/anchor/grid… | child-id/flow-weight/anchor/anchor-offset-x/anchor-offset-y/grid-row/grid-col/grid-row-span/grid-col-span | SetString("child-id")定位+SetFloat/SetInt/SetEnum | 同 | ✅ | 属性 |
| reflowChildren | — | — | — | — | ⛔内部 |  |
| setRect | rect | — | SetRect | SetRect | 🔧 |  |

## 15. ProgressBar

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setValue | value | value | SetFloat | SetFloat | ✅ |  |
| setRange | range.min/max | range-min/range-max | SetFloat | SetFloat | ✅ |  |
| setStyle | style | style | SetEnum | SetEnum | ✅ |  |
| setTextMode | textMode | text-mode | SetEnum | SetEnum | ✅ |  |
| setCustomText | customText | custom-text | SetString | SetString | ✅ |  |
| 3个颜色 | progressColor/backgroundColor/text | progress/background/text | SetColor | SetColor | ✅ |  |
| setAnimationSpeed | animationSpeed | animation-speed | SetFloat | SetFloat | ✅ |  |
| setFont/Size/Align | font/size/alignment | font/font-size/align | SetEnum/SetInt/SetEnum | 同 | ✅ |  |
| setOnValueChanged | onValueChanged | value-changed | SetCallback | SetCallback | ✅(基类接管,已可用) | 属性 |
| getPercent/getTextLabel | — | percent | GetFloat("percent") | GetFloat("percent") | ✅ | 属性 |

## 16. ScrollBar

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setValue | value | value | SetFloat | SetFloat | ✅ |  |
| setRange | range.min/max | range-min/range-max | SetFloat | SetFloat | ✅ |  |
| setPageSize | pageSize | page-size | SetFloat | SetFloat | ✅ |  |
| setStepSize | stepSize | step-size | SetFloat | SetFloat | ✅ |  |
| setOrientation | orientation | orientation | SetEnum | SetEnum | ✅ |  |
| setThickness | thickness | thickness | SetFloat | SetFloat | ✅ |  |
| 4个颜色 | track/thumb+… | track/thumb/thumb-hover/thumb-pressed | SetColor | SetColor | ✅ |  |
| setOnPositionChanged | onPositionChanged | position-changed | SetCallback | SetCallback | ✅ |  |
| shouldShow/isDragging | — | — | — | — | ⛔查询 |  |

## 17. Slider

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setRange | range.min/max | range-min/range-max | SetFloat | SetFloat | ✅ |  |
| setStep | step | step | SetFloat | SetFloat | ✅ |  |
| setValue | value | value | SetFloat | SetFloat | ✅ |  |
| setStyle | style | style | SetEnum | SetEnum | ✅ |  |
| setReverse | reverse | reverse | SetBool | SetBool | ✅ |  |
| setTrackThickness/ThumbSize | thickness/thumb | track-thickness/thumb-size | SetFloat | SetFloat | ✅ |  |
| 5个轨道/滑块颜色 | track/fillColor/thumb+… | track/track-fill/thumb/thumb-border/thumb-hover | SetColor | SetColor | ✅ |  |
| 3个刻度 | interval/length/tick | tick-interval/tick-length/tick | SetFloat×2/SetColor | 同 | ✅ |  |
| setShowValueLabel | showValueLabel | show-value-label | SetBool | SetBool | ✅ |  |
| 5个标签配置 | labelFormat/labelGap/label | label-font/label-font-size/label/label-format/label-gap | SetEnum/SetInt/SetColor/SetString/SetFloat | 同 | ✅(label-font-size已补) | 属性 |
| setOnValueChanged | onValueChanged | value-changed | SetCallback | SetCallback | ✅ |  |

## 18. Splitter

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setOrientation | orientation | horizontal | SetBool | SetBool | ✅ |  |
| setLinkedControls/… | firstPanel/secondPanel | first-linked/second-linked | SetPtr | SetPtr | ✅ |  |
| setSplitRatio | ratio | ratio | SetFloat | SetFloat | ✅ |  |
| setMinSize | minFirst/minSecond | first-min/second-min | SetFloat | SetFloat | ✅ |  |
| setThickness | thickness | thickness | SetFloat | SetFloat | ✅ |  |
| setColor | line+… | line/line-hover/line-drag | SetColor | SetColor | ✅ |  |
| setOnSplitterMoved | onSplitterMoved | moved | SetCallback | SetCallback | ✅ |  |

## 19. TextArea

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setText | text | text | SetString | SetString | ✅ |  |
| insertTextAtCursor | — | — | — | — | ⚠️命令式 | 属性 |
| setScrollX/Y | — | scroll-x/scroll-y | SetInt | SetInt | ✅ |  |
| scrollToBottom | — | — | — | — | ⚠️命令式 | 属性 |
| setWordWrap | wordWrap | word-wrap | SetBool | SetBool | ✅ |  |
| setLineHeight | lineHeight | line-height | SetInt | SetInt | ✅ |  |
| setScrollBarThickness | scrollBarThickness | scrollbar-thickness | SetFloat | SetFloat | ✅ |  |
| setOnTextChanged | onTextChanged | text-changed | SetCallback | SetCallback | ✅ |  |

## 20. TreeView

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setItems | items(id/label/expanded/children/…) | — | — | — | ✅JSON静态 | CABI/Binding |
| addRootItem/addChild | — | — | TreeViewAddNode(parentId空=根) | TreeViewAddNode | 🔧 |  |
| removeNode | — | — | TreeViewRemoveNode | TreeViewRemoveNode | 🔧 |  |
| setNodeLabel | — | — | TreeViewSetNodeLabel | TreeViewSetNodeLabel | 🔧 |  |
| setNodeUserData | — | — | TreeViewSetNodeUserData | TreeViewSetNodeUserData | 🔧 |  |
| clearItems | — | — | TreeViewClearItems | TreeViewClearItems | 🔧 |  |
| selectNode/getSelectedId | — | selected-id/selected-user-data | SetString("selected-id")读取TreeViewGetSelectedId | 同 | ✅属性+专用CABI并存 | 应该只需要有属性就够了 |
| clearSelection | — | — | TreeViewClearSelection | TreeViewClearSelection | 🔧 |  |
| expandNode/collapseNode | — | — | TreeViewExpandNode/TreeViewCollapseNode | 同 | 🔧按id |  |
| expandAll/collapseAll | — | expand-all/collapse-all | TreeViewExpandAll/CollapseAll(亦SetBool) | 同 | 🔧 |  |
| setIndentWidth/RowHeight/LineSpacing/ArrowGap | indentWidth/rowHeight/… | indent-width/row-height/line-spacing/arrow-gap | SetFloat | SetFloat | ✅ |  |
| setCycleNavigation/DefaultExpand | cycleNavigation/defaultExpand | cycle-navigation/default-expand | SetBool | SetBool | ✅ |  |
| 5个颜色 | bg/border/hover/selected/text | background/border/hover/selected/text | SetColor | SetColor | ✅ |  |
| setFont/Size | font/size | font/font-size | SetEnum/SetInt | 同 | ✅ |  |
| item级5配置 | leadingGap/font/size/leadingControl | item-leading-gap/…/item-id定位 | SetString("item-id")+Set* | 同 | ✅ |  |
| setOnSelect(Data) | onSelect | select | SetCallback | SetCallback | ✅ |  |
| setOnExpand/Collapse | onExpand/onCollapse | expand/collapse | SetCallback | SetCallback | ✅ |  |
| setOnClearNode | — | node-removed | SetCallback | SetCallback | ✅ |  |
| findNodeById/… | — | — | — | — | ⛔查询 | 暂不需要 |
| getScrollBar/getHScrollBar | — | scroll-bar/h-scroll-bar | GetPtr("scroll-bar"…) | GetPtr("scroll-bar"…) | ✅ | 属性 |

## 21. WinFrame

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setTitle | title | title | SetString | SetString | ✅ |  |
| 4个颜色 | bg/border/titleBar.bg/titleText | win-frame-bg/win-frame-border/title-bar-bg/title-text | SetColor | SetColor | ✅ |  |
| setEdgeMargin | edgeMargin | edge-margin | SetFloat | SetFloat | ✅ |  |
| setResizable | resizable | resizable | SetBool | SetBool | ✅ |  |
| addToClient | children | — | AddChildControl | — | 🔧 | Binding似乎已经有了通用的AddChild |
| show/hide | visible | visible | SetBool | SetBool | ✅ |  |
| 关闭按钮→onClose | onClose | click | SetCallback | SetCallback | ✅ |  |
| getTitleBar/getTitleLabel/getCloseButton/getClientPanel | — | title-bar/title-label/close-button/client-panel | GetPtr("title-bar"…) | GetPtr("title-bar"…) | ✅ | 属性 |

## 22. ContextMenu(右键菜单)

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| show/close | — | — | ContextMenuShow/Close | ContextMenuBuilder.build 后同 C++ | 🔧专用 |  |
| addItem/addSeparator | contextMenu.items[] | — | ContextMenuAddItem/AddSeparator | ContextMenuBuilder.addItem | 🔧专用 |  |
| setContextMenu（控件绑定） | contextMenu（控件级键） | context-menu | SetPtr("context-menu") | SetPtr | ✅ |  |
| getMenuPanel | — | — | — | — | ⛔内部 |  |

## 23. ListView(含单列 ListBox 模式)

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setMode/setMultiSelect/setSelectedRow/setCycleNavigation | mode/multiSelect/selectedIndex/cycleNavigation | mode/multi-select/selected-index/cycle-navigation | SetEnum/SetBool/SetInt | 同 | ✅ |  |
| setRowHeight/setHeaderHeight/setMinColumnWidth | rowHeight/headerHeight/minColumnWidth | row-height/header-height/min-column-width | SetFloat | SetFloat | ✅ |  |
| setGridlines/setHorizontalGridlines/setHoverHighlight | gridlines/horizontalGridlines/hover | gridlines/horizontal-gridlines/hover | SetBool | SetBool | ✅ |  |
| setSortColumn/setSortAscending | sortColumn/sortAscending | sort-column/sort-ascending | SetInt/SetBool | 同 | ✅ |  |
| addRow/insertRow/removeRow/setRowCells/setCell | rows/cells | — | ListViewAddRow/InsertRow/RemoveRow/SetRowCells/SetCellText/GetCellText | UICornerstone::ListView* | 🔧专用 |  |
| addColumn/insertColumn/removeColumn/setColumnWidth | columns | — | ListViewAddColumn/InsertColumn/RemoveColumn/SetColumnWidth | 同 | 🔧专用 |  |
| setRowLeadingControl/setColumnLeadingControl/setCellLeadingControl | icon/columns.icon/cellControls | — | ListViewSetRowLeadingControl/SetColumnIcon/SetCellLeadingControl | 同 | 🔧专用 |  |
| setColumnSorter/sortByColumn | —（运行时注入） | — | ListViewSetColumnSorter | 同 | 🔧专用 |  |
| setOnSelectionChanged/setOnItemClick/setOnColumnSort | events.on* | — | SetCallback（kEventListSelectionChanged/kEventItemClick/kEventColumnSort，负载 grid） | SetCallback + Event::IsListSelectionChanged/IsItemClick/IsColumnSort/GetGridRow/GetGridCol/GetGridAsc | ✅ |  |

## 24. Shape(形状)

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setShape | shape | shape | SetEnum("shape") | SetEnum | ✅ |  |
| setFillColor/setStrokeColor | fill/stroke | fill/stroke | SetColor | SetColor | ✅ |  |
| setLineWidth/setRadius/setRingWidth | lineWidth/radius/ring-width | line-width/radius/ring-width | SetFloat | SetFloat | ✅ |  |
| setPoints | points | — | ShapeSetPoints | — | 🔧 |  |
| mapToDrawPoint/getDrawPoint | — | — | ShapeMapToDrawPoint | — | ⛔查询 |  |
| 多图元 addPrimitive/setPrimitive* | primitives | — | ShapeAddPrimitive/ShapeSetPrimitiveColor/ShapeSetPrimitiveFloat/ShapeSetPrimitivePoints/ShapeClearPrimitives | ShapeBuilder.addPrimitive/setPrimitive* + UICornerstone::ShapeAddPrimitive 等五方法（v1.1.1 Binding 全链） | ✅Builder |  |
| setBackgroundStateColor | colors.background | background | SetStateColor | SetStateColor | ✅ |  |

## 25. StatusBar(VSCode 风格状态栏)

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setFontSize/setItemHeight | fontSize/itemHeight | font-size/item-height | SetFloat/GetFloat | SetFloat/GetFloat | ✅ |  |
| addStatusItem/updateStatusItemText/removeStatusItem | items[] | — | StatusBarAddItem/SetItemText/RemoveItem | StatusBarBuilder.addStatusItem | 🔧专用 |  |
| setStatusItemMenu | items[].menu | — | StatusBarSetItemMenu | StatusBarBuilder.setStatusItemMenu | 🔧专用 |  |
| setStatusItemLeadingControl | items[].icon | — | StatusBarSetItemIcon | StatusBarBuilder.setStatusItemLeadingControl | 🔧专用 |  |
| setStatusItemOnClick | items[].onClick | — | SetCallback（kEventStatusItemClick，"status-item-click", intVal=段索引） | StatusBarBuilder.setStatusItemOnClick + Event::IsStatusItemClick/GetStatusItemIndex | ✅ |  |
| openPopup/closePopup | — | — | — | — | ⛔内部(点击驱动) |  |

## 26. TabControl(选项卡)

| 内部API | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| setPosition | position | position | SetEnum/GetEnum | SetEnum | ✅ |  |
| setFontSize | fontSize | font-size | SetFloat/GetFloat | SetFloat | ✅ |  |
| setCurrentIndex/getCurrentIndex | currentIndex | current-index | SetInt/GetInt | SetInt | ✅ |  |
| addTab/insertTab/removeTab | tabs[] | — | TabAddPage | TabControlBuilder.addTab | 🔧专用 |  |
| setTabText/setTabPage | tabs[].title/page | — | TabSetTitle/TabSetPage | 同 | 🔧专用 |  |
| setTabLeadingControl | tabs[].icon | — | TabSetTabLeadingControl | — | ⚠️Binding | Builder 扩展后续 |
| setOnTabChange | events.onTabChange | — | SetCallback（kEventTabChanged，"tab-changed", intVal=新页索引） | SetCallback + Event::IsTabChanged/GetTabChangedIndex | ✅ |  |

## 27. 实例/视口/布局/引擎级

| 能力 | JSON | 属性 | CABI | Binding | 缺口 | 补 |
|---|---|---|---|---|---|---|
| 实例创建/销毁 | — | — | CreateInstance/DestroyInstance/… | UICornerstone::Create | 🔧 |  |
| 子视口 | viewport | — | CreateViewport/SetViewport/GetViewport | 同 | 🔧 |  |
| 视口背景色 | — | — | SetViewportBackgroundColor | 同 | 🔧 |  |
| 视口缩放 | scale-mode | — | SetViewportScaleMode/GetViewportScaleMode/SetCanvasSize/GetViewportScale/SetViewportAnchor | 同 | 🔧 |  |
| 实例缩放（画布语义） | — | — | SetInstanceScale/GetInstanceScale（v1.1.1，off 分支尊重手动值） | SetInstanceScale/GetInstanceScaleX/Y | ✅ |  |
| 帧循环 | — | — | ProcessEvents/Update/Render/Clear/Present/IsQuitRequested | 同+Run | 🔧 |  |
| 事件注入 | — | — | PushUIEvent | PushEvent/PushMouse*/PushKey/PushTextInput | 🔧 |  |
| 布局加载 | 布局JSON | — | LoadLayout/LoadLayoutFromFile/FindControl | 同 | 🔧 |  |
| 动作注册 | events.onXxx | — | RegisterAction | RegisterAction | 🔧 |  |
| 控件工厂 | — | — | CreateButton/CreateLabel/…等30个工厂(含CreateListView/CreateStatusBar/CreateTabControl/CreateContextMenu/CreateShape/CreateImageButton/CreateAnimatedButton/CreateImage/CreateActor/CreateAnimation/CreateDialog) | 同(24个,无CreateActor) | 🔧 |  |
| 控件销毁 | — | — | DestroyControl | — | 🔧 | Binding |
| 后端配置 | — | — | SetBackendConfig/SetBackendConfigInt/SetBackendConfigBool/GetBackendConfig/GetBackendConfigInt/GetBackendConfigBool | SetBackendConfig/SetBackendConfigBool/GetBackendConfigBool | 🔧 |  |
| 后端能力查询 | — | — | GetBackendCapabilities | GetBackendCapabilities | 🔧 |  |
| 从插件创建实例 | — | — | CreateInstanceFromPlugin | —(Binding经Config::backend) | 🔧 |  |
| 属性系统通用入口 | — | 全部属性键 | Set*/Get*8类型+SetCallback | Control::Set*/Get*+SetCallback | ✅ |  |
| 控件类型查询 | type | — | GetControlType | — | 🔧 | 属性 |
| 内存资源注册 | resourceProviders | — | 回调查表MemoryProvider | RegisterResource/AdoptResource | 🔧 |  |
| 截图读回 | — | — | CaptureRect/CaptureViewport/CaptureBench/CaptureControl/SavePixelsToFile | — | 🔧 | Binding |
| Debug辅助 | — | — | Debug_GetAliveCount等7个 | DebugGetAliveCount等 | 🔧 |         |

---

# 缺口汇总(需补 CABI / C++Binding)

| 优先级 | 能力 | 归属 | 建议 |
|---|---|---|---|
| ~~高~~ | ~~焦点体系(focusable/tab-index/focus-ring*/state)~~ | 基类 | ✅已实现(2026-08-21) |
| 高 | ~~setOnValueChanged 未 override setCallbackProperty~~ | ProgressBar | ✅基类无条件接管,已可用(2026-08-21) |
| ~~中~~ | ~~SetFontStyle / setShadowOffset~~ | Label | ✅已实现(2026-08-21) |
| ~~中~~ | ~~setPasswordChar(仅JSON)~~ | EditBox | ✅已实现(2026-08-21) |
| ~~中~~ | ~~label-font-size 缺分发~~ | Slider | ✅已实现(2026-08-21) |
| ~~中~~ | ~~选区/剪贴板/stepValue~~ | EditBox/TextArea/NumericUpDown | ✅已实现专用CABI(2026-08-21) |
| ~~中~~ | ~~open/close~~ | Popup/Dialog | ✅已实现(visible/popup-visible/result,2026-08-21) |
| ~~中~~ | ~~addItem/clearItems(运行期)~~ | ComboBox | ✅已实现专用CABI(2026-08-21) |
| ~~中~~ | ~~prepare/setFrameFilter~~ | LuotiAni | ✅已实现专用CABI(2026-08-21) |
| 剩余 | 查询属性:percent/total-frames/current-frame/selected-label/result/popup-visible | 各控件 | ✅已实现(2026-08-21) |
| 剩余 | 子控件访问器:title-bar/title-label/close-button/client-panel/list-panel/list-scroll-bar/scroll-bar/h-scroll-bar/confirm-button/cancel-button | WinFrame/ComboBox/TreeView/Dialog | ✅已实现(2026-08-21) |
| 剩余 | anchor-x/anchor-y(自由锚点) | Actor | ✅已实现(2026-08-21) |
| 剩余 | margin-left/top/right/bottom 属性键 | 基类 | ✅已实现(2026-08-21) |
| 剩余 | debug-draw 属性 + alpha JSON + onPropertyChanged JSON + MenuPanel font/itemHeightRatio JSON | Label/Actor/Menu | ✅已实现(2026-08-21) |
| 低 | ~~setOnSelectData/setOnClearNode~~ | TreeView | 数据载荷已含,无需新键 |
| 低 | 全配置无键 | HandleControl | 建议豁免 |
| 低 | offset/padding/按钮矩形 | Dialog | 视需求补 |
| 低 | ~~setChild*Props 运行期注入~~ | Panel | JSON已可声明 |
| 低 | setTexture/loadTextureFromSurface 对象注入 | Actor | 建议豁免(纹理句柄无CABI桥接) |
| 低 | ~~enterMenuMode/closeMenuChain~~ | Menu | 用户标注暂不需要 |
| 低 | getId/getControlType 转属性删除CABI | 基类 | 见文末决策说明 |

---

## 待决策:getId / getControlType 转属性

- **getControlType**:已补 `control-type` 只读字符串属性(getter),与 `GetControlType` CABI 并存(映射表已标 ✅)。删除 CABI 会破坏现有 DLL 用户,建议保留。
- **getId**:`UICornerstone_GetControlId` 是**实例级反查**(`instance->controlsById` 按对象找字符串 id),控件自身只有 `setId(int)`(整数序号,非 JSON id)。转属性需在控件上存字符串 id 并与 LayoutParser 的 JSON "id" 打通,改动大、易冲突。**建议保留 GetControlId CABI,**"补"列改为"暂不需要"。