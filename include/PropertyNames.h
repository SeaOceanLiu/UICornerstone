#pragma once

#ifdef __cplusplus
#include <cstddef>
#define PROP_NS_BEGIN namespace PropertyNames {
#define PROP_NS_END }
#define PROP_CONSTEXPR inline constexpr
#else
#include <stddef.h>
#define PROP_NS_BEGIN
#define PROP_NS_END
#define PROP_CONSTEXPR static
#endif

PROP_NS_BEGIN

// -- 通用颜色 --
PROP_CONSTEXPR const char* kBackground          = "background";
PROP_CONSTEXPR const char* kBorder              = "border";
PROP_CONSTEXPR const char* kText                = "text";
PROP_CONSTEXPR const char* kTextShadow          = "text-shadow";

// StateColor 子键
PROP_CONSTEXPR const char* kStateHover          = "background.hover";
PROP_CONSTEXPR const char* kStatePressed        = "background.pressed";
PROP_CONSTEXPR const char* kStateDisabled       = "background.disabled";
PROP_CONSTEXPR const char* kBorderHover         = "border.hover";
PROP_CONSTEXPR const char* kBorderPressed       = "border.pressed";
PROP_CONSTEXPR const char* kBorderDisabled      = "border.disabled";
PROP_CONSTEXPR const char* kTextHover           = "text.hover";
PROP_CONSTEXPR const char* kTextPressed         = "text.pressed";
PROP_CONSTEXPR const char* kTextDisabled        = "text.disabled";
PROP_CONSTEXPR const char* kTextShadowHover     = "text-shadow.hover";
PROP_CONSTEXPR const char* kTextShadowPressed   = "text-shadow.pressed";
PROP_CONSTEXPR const char* kTextShadowDisabled  = "text-shadow.disabled";

// -- 控件特有颜色 --
PROP_CONSTEXPR const char* kTrack               = "track";
PROP_CONSTEXPR const char* kTrackFill           = "track-fill";
PROP_CONSTEXPR const char* kThumb               = "thumb";
PROP_CONSTEXPR const char* kThumbBorder         = "thumb-border";
PROP_CONSTEXPR const char* kThumbHover          = "thumb-hover";
PROP_CONSTEXPR const char* kThumbPressed        = "thumb-pressed";
PROP_CONSTEXPR const char* kTick                = "tick";
PROP_CONSTEXPR const char* kLabelColor          = "label";
PROP_CONSTEXPR const char* kArrow               = "arrow";
PROP_CONSTEXPR const char* kArrowHover          = "arrow-hover";
PROP_CONSTEXPR const char* kArrowPressed        = "arrow-pressed";
PROP_CONSTEXPR const char* kItemSelected        = "item-selected";
PROP_CONSTEXPR const char* kItemHover           = "item-hover";
PROP_CONSTEXPR const char* kItemDisabled        = "item-disabled";
PROP_CONSTEXPR const char* kListBg              = "list-bg";
PROP_CONSTEXPR const char* kListBorder          = "list-border";
PROP_CONSTEXPR const char* kCheck               = "check-color";
PROP_CONSTEXPR const char* kCross               = "cross-color";
PROP_CONSTEXPR const char* kIndeterminate       = "indeterminate-color";
PROP_CONSTEXPR const char* kBoxBorder           = "box-border-color";
PROP_CONSTEXPR const char* kProgress            = "progress";
PROP_CONSTEXPR const char* kLine                = "line";
PROP_CONSTEXPR const char* kLineHover           = "line-hover";
PROP_CONSTEXPR const char* kLineDrag            = "line-drag";
PROP_CONSTEXPR const char* kWinFrameBG          = "win-frame-bg";
PROP_CONSTEXPR const char* kWinFrameBorder      = "win-frame-border";
PROP_CONSTEXPR const char* kTitleBarBG          = "title-bar-bg";
PROP_CONSTEXPR const char* kTitleText           = "title-text";
PROP_CONSTEXPR const char* kClosedText          = "closed-text";
PROP_CONSTEXPR const char* kPopupBG             = "popup-bg";
PROP_CONSTEXPR const char* kFocusRingColor      = "focus-ring-color";

// TreeView 颜色
PROP_CONSTEXPR const char* kTreeSelected        = "selected";
PROP_CONSTEXPR const char* kTreeHover           = "hover";

// -- 通用 Bool --
PROP_CONSTEXPR const char* kVisible             = "visible";
PROP_CONSTEXPR const char* kEnabled             = "enabled";
PROP_CONSTEXPR const char* kTransparent         = "transparent";
PROP_CONSTEXPR const char* kBorderVisible       = "border-visible";
PROP_CONSTEXPR const char* kFocusable           = "focusable";
PROP_CONSTEXPR const char* kShowFocusRing       = "show-focus-ring";
PROP_CONSTEXPR const char* kFocusRingAlwaysVisible = "focus-ring-always-visible";
PROP_CONSTEXPR const char* kFocusBoundary       = "focus-boundary";
PROP_CONSTEXPR const char* kAlwaysOnTop         = "always-on-top";

// -- Button 状态图（字符串属性：图片路径；设置后创建对应状态 Actor）--
PROP_CONSTEXPR const char* kNormalImage         = "normal-image";
PROP_CONSTEXPR const char* kHoverImage          = "hover-image";
PROP_CONSTEXPR const char* kPressedImage        = "pressed-image";
PROP_CONSTEXPR const char* kDisabledImage       = "disabled-image";

// -- Bool 属性 --
PROP_CONSTEXPR const char* kTextShadowEnable    = "text-shadow-enable";
PROP_CONSTEXPR const char* kShadow              = "shadow";
PROP_CONSTEXPR const char* kExpand              = "expand";
PROP_CONSTEXPR const char* kClickable           = "clickable";
PROP_CONSTEXPR const char* kPasswordMode        = "password-mode";
PROP_CONSTEXPR const char* kWordWrap            = "word-wrap";
PROP_CONSTEXPR const char* kClipChildren        = "clip-children";
PROP_CONSTEXPR const char* kTriState            = "tri-state";
PROP_CONSTEXPR const char* kReverse             = "reverse";
PROP_CONSTEXPR const char* kShowValueLabel      = "show-value-label";
PROP_CONSTEXPR const char* kCycleEnabled        = "cycle-enabled";
PROP_CONSTEXPR const char* kCycleNavigation     = "cycle-navigation";
PROP_CONSTEXPR const char* kDefaultExpand       = "default-expand";
PROP_CONSTEXPR const char* kDebugDraw           = "debug-draw";
PROP_CONSTEXPR const char* kResizable           = "resizable";
PROP_CONSTEXPR const char* kCloseOnClickOutside = "close-on-click-outside";
PROP_CONSTEXPR const char* kCloseOnEsc          = "close-on-esc";
PROP_CONSTEXPR const char* kConfirmVisible      = "confirm-visible";
PROP_CONSTEXPR const char* kReadOnly            = "read-only";
PROP_CONSTEXPR const char* kChecked             = "checked";
PROP_CONSTEXPR const char* kHorizontal          = "horizontal";

// TreeView 树操作
PROP_CONSTEXPR const char* kExpandAll           = "expand-all";
PROP_CONSTEXPR const char* kCollapseAll         = "collapse-all";

// -- Int 属性 --
PROP_CONSTEXPR const char* kFontSize            = "font-size";
PROP_CONSTEXPR const char* kLineHeight          = "line-height";
PROP_CONSTEXPR const char* kScrollX             = "scroll-x";
PROP_CONSTEXPR const char* kScrollY             = "scroll-y";
PROP_CONSTEXPR const char* kLabelFontSize       = "label-font-size";
PROP_CONSTEXPR const char* kPresetCols          = "preset-cols";
PROP_CONSTEXPR const char* kPresetRows          = "preset-rows";
PROP_CONSTEXPR const char* kClosedFontSize      = "closed-font-size";
PROP_CONSTEXPR const char* kMaxVisibleItems     = "max-visible-items";
PROP_CONSTEXPR const char* kSelectedIndex       = "selected-index";
PROP_CONSTEXPR const char* kHoveredIndex        = "hovered-index";
PROP_CONSTEXPR const char* kDecimals            = "decimals";
PROP_CONSTEXPR const char* kTabIndex            = "tab-index";
PROP_CONSTEXPR const char* kPasswordChar        = "password-char";
// 只读查询属性
PROP_CONSTEXPR const char* kPercent             = "percent";
PROP_CONSTEXPR const char* kTotalFrames         = "total-frames";
PROP_CONSTEXPR const char* kCurrentFrame        = "current-frame";
PROP_CONSTEXPR const char* kPopupVisible        = "popup-visible";
PROP_CONSTEXPR const char* kResult              = "result";
PROP_CONSTEXPR const char* kSelectedLabel       = "selected-label";
// 命令式开关属性（Popup/Dialog）
PROP_CONSTEXPR const char* kOpen                = "open";
// 自由坐标锚点
PROP_CONSTEXPR const char* kAnchorX             = "anchor-x";
PROP_CONSTEXPR const char* kAnchorY             = "anchor-y";
// 子控件访问器（只读 Ptr 属性）
PROP_CONSTEXPR const char* kTitleBar            = "title-bar";
PROP_CONSTEXPR const char* kTitleLabel          = "title-label";
PROP_CONSTEXPR const char* kCloseButton         = "close-button";
PROP_CONSTEXPR const char* kClientPanel         = "client-panel";
PROP_CONSTEXPR const char* kListPanel           = "list-panel";
PROP_CONSTEXPR const char* kListScrollBar       = "list-scroll-bar";
PROP_CONSTEXPR const char* kScrollBarProp       = "scroll-bar";
PROP_CONSTEXPR const char* kHScrollBarProp      = "h-scroll-bar";
PROP_CONSTEXPR const char* kConfirmButton       = "confirm-button";
PROP_CONSTEXPR const char* kCancelButton        = "cancel-button";
// 布局子属性（Panel setChild*Props 属性系统入口）
PROP_CONSTEXPR const char* kChildTargetId       = "child-id";
PROP_CONSTEXPR const char* kChildFlowWeight     = "flow-weight";
PROP_CONSTEXPR const char* kChildAnchor          = "anchor";
PROP_CONSTEXPR const char* kChildAnchorOffsetX  = "anchor-offset-x";
PROP_CONSTEXPR const char* kChildAnchorOffsetY  = "anchor-offset-y";
PROP_CONSTEXPR const char* kChildGridRow        = "grid-row";
PROP_CONSTEXPR const char* kChildGridCol        = "grid-col";
PROP_CONSTEXPR const char* kChildGridRowSpan    = "grid-row-span";
PROP_CONSTEXPR const char* kChildGridColSpan    = "grid-col-span";

// -- Float 属性 --
PROP_CONSTEXPR const char* kLineSpacingRatio    = "line-spacing-ratio";
PROP_CONSTEXPR const char* kSizeRatio           = "size-ratio";
PROP_CONSTEXPR const char* kStep                = "step";
PROP_CONSTEXPR const char* kValue               = "value";
PROP_CONSTEXPR const char* kTrackThickness      = "track-thickness";
PROP_CONSTEXPR const char* kThumbSize           = "thumb-size";
PROP_CONSTEXPR const char* kTickInterval        = "tick-interval";
PROP_CONSTEXPR const char* kTickLength          = "tick-length";
PROP_CONSTEXPR const char* kLabelGap            = "label-gap";
PROP_CONSTEXPR const char* kRangeMin            = "range-min";
PROP_CONSTEXPR const char* kRangeMax            = "range-max";
PROP_CONSTEXPR const char* kAnimationSpeed      = "animation-speed";
PROP_CONSTEXPR const char* kScrollbarThickness  = "scrollbar-thickness";
PROP_CONSTEXPR const char* kPageSize            = "page-size";
PROP_CONSTEXPR const char* kStepSize            = "step-size";
PROP_CONSTEXPR const char* kThickness           = "thickness";
PROP_CONSTEXPR const char* kEdgeMargin          = "edge-margin";
PROP_CONSTEXPR const char* kBarHeight           = "bar-height";
PROP_CONSTEXPR const char* kManualPosition      = "manual-position";
PROP_CONSTEXPR const char* kItemHeightRatio     = "item-height-ratio";
PROP_CONSTEXPR const char* kArrowWidth          = "arrow-width";
PROP_CONSTEXPR const char* kItemHeight          = "item-height";
PROP_CONSTEXPR const char* kButtonHeight        = "button-height";
PROP_CONSTEXPR const char* kButtonGap           = "button-gap";
PROP_CONSTEXPR const char* kPadding             = "padding";
PROP_CONSTEXPR const char* kIndentWidth         = "indent-width";
PROP_CONSTEXPR const char* kRowHeight           = "row-height";
PROP_CONSTEXPR const char* kLineSpacing         = "line-spacing";
PROP_CONSTEXPR const char* kArrowGap            = "arrow-gap";
PROP_CONSTEXPR const char* kRatio               = "ratio";
PROP_CONSTEXPR const char* kClosedSwatchSize    = "closed-swatch-size";
PROP_CONSTEXPR const char* kPageStep            = "page-step";
PROP_CONSTEXPR const char* kButtonWidth         = "button-width";
PROP_CONSTEXPR const char* kCaptionSize         = "caption-size";
PROP_CONSTEXPR const char* kMarginLeft          = "margin-left";
PROP_CONSTEXPR const char* kMarginTop           = "margin-top";
PROP_CONSTEXPR const char* kMarginRight         = "margin-right";
PROP_CONSTEXPR const char* kMarginBottom        = "margin-bottom";
PROP_CONSTEXPR const char* kShadowOffsetX       = "shadow-offset-x";
PROP_CONSTEXPR const char* kShadowOffsetY       = "shadow-offset-y";

// Splitter
PROP_CONSTEXPR const char* kFirstMin            = "first-min";
PROP_CONSTEXPR const char* kSecondMin           = "second-min";

// -- String 属性 --
PROP_CONSTEXPR const char* kCaption             = "caption";
PROP_CONSTEXPR const char* kTextContent         = "text";
PROP_CONSTEXPR const char* kPlaceholder         = "placeholder";
PROP_CONSTEXPR const char* kSelectedValue       = "selected-value";
PROP_CONSTEXPR const char* kCustomText          = "custom-text";
PROP_CONSTEXPR const char* kLabelFormat         = "label-format";
PROP_CONSTEXPR const char* kShortcut            = "shortcut";
PROP_CONSTEXPR const char* kTitle               = "title";
PROP_CONSTEXPR const char* kConfirmText         = "confirm-text";
PROP_CONSTEXPR const char* kCancelText          = "cancel-text";
PROP_CONSTEXPR const char* kId                  = "id";
PROP_CONSTEXPR const char* kControlType         = "control-type";
PROP_CONSTEXPR const char* kColor               = "color";
PROP_CONSTEXPR const char* kAnimation           = "animation";
PROP_CONSTEXPR const char* kPlaying             = "playing";            // LuotiAni 播放/暂停（命令式，置 1=play 帧复位 0 重播）
PROP_CONSTEXPR const char* kLoop                = "loop";               // LuotiAni 循环开关
PROP_CONSTEXPR const char* kFrame               = "frame";              // LuotiAni 跳帧（只读当前帧经 getInt）
PROP_CONSTEXPR const char* kItems               = "items";

// TreeView
PROP_CONSTEXPR const char* kTreeExpand          = "expand";
PROP_CONSTEXPR const char* kTreeCollapse        = "collapse";
PROP_CONSTEXPR const char* kSelectedId          = "selected-id";
PROP_CONSTEXPR const char* kSelectedUserData    = "selected-user-data";
// TreeView item 级属性：先以字符串属性 "item-id" 定位目标节点，再用下列属性作用于该节点
PROP_CONSTEXPR const char* kTreeItemId             = "item-id";
PROP_CONSTEXPR const char* kTreeItemLeadingGap     = "item-leading-gap";
PROP_CONSTEXPR const char* kTreeItemLeadingAlign   = "item-leading-align";
PROP_CONSTEXPR const char* kTreeItemFontSize       = "item-font-size";
PROP_CONSTEXPR const char* kTreeItemFont           = "item-font";
PROP_CONSTEXPR const char* kTreeItemLeadingControl = "item-leading-control";

// -- Shape 形状控件 --
PROP_CONSTEXPR const char* kShape                  = "shape";             // 枚举属性：形状类型
PROP_CONSTEXPR const char* kPrimitives             = "primitives";        // Shape 多图元（组合图形）
PROP_CONSTEXPR const char* kFill                   = "fill";              // 颜色属性：填充色（缺省透明=空心）
PROP_CONSTEXPR const char* kStroke                 = "stroke";            // 颜色属性：描边色（缺省黑）
PROP_CONSTEXPR const char* kLineWidthProp          = "line-width";        // 浮点属性：描边宽 px
PROP_CONSTEXPR const char* kRadius                 = "radius";            // 浮点属性：圆角半径
PROP_CONSTEXPR const char* kRingWidth              = "ring-width";        // 浮点属性：环带宽度
PROP_CONSTEXPR const char* kJsonLineWidth = "line-width";         // JSON 键（camelCase）
PROP_CONSTEXPR const char* kJsonRingWidth = "ring-width";
// ShapeType 枚举值
PROP_CONSTEXPR const char* kShapeRect              = "rect";
PROP_CONSTEXPR const char* kShapeFilledRect        = "filled-rect";
PROP_CONSTEXPR const char* kShapeRoundRect         = "round-rect";
PROP_CONSTEXPR const char* kShapeCircle            = "circle";
PROP_CONSTEXPR const char* kShapeEllipse           = "ellipse";
PROP_CONSTEXPR const char* kShapePolyline          = "polyline";
PROP_CONSTEXPR const char* kShapePolygon           = "polygon";

// -- ListView 列表控件（selected-index/cycle-navigation/row-height 复用 TreeView 键）--
PROP_CONSTEXPR const char* kMode                   = "mode";               // 枚举属性：视图模式
PROP_CONSTEXPR const char* kMultiSelect            = "multi-select";       // bool：Ctrl 多选开关
PROP_CONSTEXPR const char* kHeaderHeight           = "header-height";      // float：列头行高
PROP_CONSTEXPR const char* kGridlines              = "gridlines";          // bool：竖网格线
PROP_CONSTEXPR const char* kHorizontalGridlines    = "horizontal-gridlines"; // bool：横网格线
PROP_CONSTEXPR const char* kHoverHighlight         = "hover";              // bool：hover 高亮开关
PROP_CONSTEXPR const char* kMinColumnWidth         = "min-column-width";   // float：列宽拖拽最小钳制
PROP_CONSTEXPR const char* kSortColumn             = "sort-column";        // int：排序列（-1 无）
PROP_CONSTEXPR const char* kSortAscending          = "sort-ascending";     // bool：排序方向
// ListView 枚举值
PROP_CONSTEXPR const char* kModeMulti              = "multi";
PROP_CONSTEXPR const char* kModeSingle             = "single";

// -- Image --
PROP_CONSTEXPR const char* kImage             = "image";              // 文件路径（只写不读）
PROP_CONSTEXPR const char* kImageResource     = "image-resource";     // 资源 ID（只写不读）
PROP_CONSTEXPR const char* kScaleType         = "scale-type";         // 枚举
PROP_CONSTEXPR const char* kMatchParentRect   = "match-parent-rect";  // 布尔
PROP_CONSTEXPR const char* kAlpha             = "alpha";              // 整数 0-255
PROP_CONSTEXPR const char* kAnchor            = "anchor";             // 枚举

// -- Ptr 属性 --
PROP_CONSTEXPR const char* kContent             = "content";
PROP_CONSTEXPR const char* kFirstLinked         = "first-linked";
PROP_CONSTEXPR const char* kSecondLinked        = "second-linked";

// -- Enum 属性名 --
PROP_CONSTEXPR const char* kAlign               = "align";
PROP_CONSTEXPR const char* kFont                = "font";
PROP_CONSTEXPR const char* kFontResource        = "font-resource";
PROP_CONSTEXPR const char* kFontFile            = "font-file";
PROP_CONSTEXPR const char* kCheckBoxStyle       = "style";
PROP_CONSTEXPR const char* kProgressBarStyle    = "style";
PROP_CONSTEXPR const char* kSliderStyle         = "style";
PROP_CONSTEXPR const char* kLayout              = "layout";
PROP_CONSTEXPR const char* kVerticalAlign       = "vertical-align";
PROP_CONSTEXPR const char* kTextMode            = "text-mode";
PROP_CONSTEXPR const char* kOrientation         = "orientation";
PROP_CONSTEXPR const char* kCheckState          = "check-state";
PROP_CONSTEXPR const char* kLabelFont           = "label-font";
PROP_CONSTEXPR const char* kCenteredMode        = "centered-mode";
PROP_CONSTEXPR const char* kState               = "state";
PROP_CONSTEXPR const char* kFocusRingStyle      = "focus-ring-style";
PROP_CONSTEXPR const char* kFontStyle           = "font-style";

// -- 枚举值常量 --

// CheckBoxStyle
PROP_CONSTEXPR const char* kStyleClassic        = "classic";
PROP_CONSTEXPR const char* kStyleCross          = "cross";
PROP_CONSTEXPR const char* kStyleCircle         = "circle";

// CheckState
PROP_CONSTEXPR const char* kCheckUnchecked      = "unchecked";
PROP_CONSTEXPR const char* kCheckChecked        = "checked";
PROP_CONSTEXPR const char* kCheckIndeterminate  = "indeterminate";

// ProgressBarStyle / SliderStyle / Orientation
PROP_CONSTEXPR const char* kOrientHorizontal    = "horizontal";
PROP_CONSTEXPR const char* kOrientVertical      = "vertical";

// ProgressBarTextMode
PROP_CONSTEXPR const char* kTextModeNone        = "none";
PROP_CONSTEXPR const char* kTextModePercent     = "percent";
PROP_CONSTEXPR const char* kTextModeCustom      = "custom";

// CheckBoxLayout
PROP_CONSTEXPR const char* kLayoutTextRight     = "text-right";
PROP_CONSTEXPR const char* kLayoutTextLeft      = "text-left";

// VerticalAlign
PROP_CONSTEXPR const char* kVAlignCenter        = "center";
PROP_CONSTEXPR const char* kVAlignTop           = "top";
PROP_CONSTEXPR const char* kVAlignBottom        = "bottom";

// TextAlign
PROP_CONSTEXPR const char* kAlignTopLeft        = "top-left";
PROP_CONSTEXPR const char* kAlignMidLeft        = "mid-left";
PROP_CONSTEXPR const char* kAlignBottomLeft     = "bottom-left";
PROP_CONSTEXPR const char* kAlignTopRight       = "top-right";
PROP_CONSTEXPR const char* kAlignMidRight       = "mid-right";
PROP_CONSTEXPR const char* kAlignBottomRight    = "bottom-right";
PROP_CONSTEXPR const char* kAlignTopCenter      = "top-center";
PROP_CONSTEXPR const char* kAlignCenter         = "center";
PROP_CONSTEXPR const char* kAlignBottomCenter   = "bottom-center";

// CenteredMode
PROP_CONSTEXPR const char* kCentered            = "centered";

// FocusRingStyle
PROP_CONSTEXPR const char* kFocusRingSolid      = "solid";
PROP_CONSTEXPR const char* kFocusRingDashed     = "dashed";

// -- Font 枚举值（仅保留 assets/fonts 内实际存在的字体）--
PROP_CONSTEXPR const char* kFontAsulBold                    = "asul-bold";
PROP_CONSTEXPR const char* kFontHarmonySansSCRegular        = "harmonyos-sans-sc-regular";
PROP_CONSTEXPR const char* kFontHarmonySansSCThin           = "harmonyos-sans-sc-thin";
PROP_CONSTEXPR const char* kFontMapleMonoNFCNRegular        = "maplemono-nf-cn-regular";
PROP_CONSTEXPR const char* kFontMuYaoSoftBrush              = "muyao-softbrush";
PROP_CONSTEXPR const char* kFontQuandoRegular               = "quando-regular";

// -- Events --
PROP_CONSTEXPR const char* kEventClick            = "click";
PROP_CONSTEXPR const char* kEventEnter            = "enter";
PROP_CONSTEXPR const char* kEventTextChanged      = "text-changed";
PROP_CONSTEXPR const char* kEventClose            = "close";
PROP_CONSTEXPR const char* kEventConfirm          = "confirm";
PROP_CONSTEXPR const char* kEventCancel           = "cancel";
PROP_CONSTEXPR const char* kEventCheckChanged     = "check-changed";
PROP_CONSTEXPR const char* kEventValueChanged     = "value-changed";
PROP_CONSTEXPR const char* kEventPositionChanged  = "position-changed";
PROP_CONSTEXPR const char* kEventSelectionChanged = "selection-changed";
PROP_CONSTEXPR const char* kEventMoved            = "moved";
PROP_CONSTEXPR const char* kEventColorChanged     = "color-changed";
PROP_CONSTEXPR const char* kEventSelect           = "select";
PROP_CONSTEXPR const char* kEventListSelectionChanged = "list-selection-changed";
PROP_CONSTEXPR const char* kEventItemClick        = "item-click";
PROP_CONSTEXPR const char* kEventColumnSort       = "column-sort";
PROP_CONSTEXPR const char* kEventAnimationEnded   = "animation-ended";
PROP_CONSTEXPR const char* kEventStatusItemClick  = "status-item-click";
PROP_CONSTEXPR const char* kEventTabChanged       = "tab-changed";
PROP_CONSTEXPR const char* kEventExpand           = "expand";
PROP_CONSTEXPR const char* kEventCollapse         = "collapse";
PROP_CONSTEXPR const char* kEventInitial          = "initial";
PROP_CONSTEXPR const char* kEventPropertyChanged  = "property-changed";
PROP_CONSTEXPR const char* kEventNodeRemoved       = "node-removed";

// ============================================================
// -- JSON 数据契约键（LuotiAni 动画描述 / LayoutParser 布局 / Theme）--
// ============================================================

// LuotiAni 动画描述 JSON
PROP_CONSTEXPR const char* kJsonOverview        = "overview";
PROP_CONSTEXPR const char* kJsonName            = "name";
PROP_CONSTEXPR const char* kJsonVersion         = "version";
PROP_CONSTEXPR const char* kJsonView            = "view";
PROP_CONSTEXPR const char* kJsonWidth           = "width";
PROP_CONSTEXPR const char* kJsonHeight          = "height";
PROP_CONSTEXPR const char* kJsonFrameRate       = "frameRate";
PROP_CONSTEXPR const char* kJsonTotalFrames     = "totalFrames";
PROP_CONSTEXPR const char* kJsonLoop            = "loop";
PROP_CONSTEXPR const char* kJsonLayers          = "layers";
PROP_CONSTEXPR const char* kJsonType            = "type";
PROP_CONSTEXPR const char* kJsonSrc             = "src";
PROP_CONSTEXPR const char* kJsonOpacity         = "opacity";
PROP_CONSTEXPR const char* kJsonBlendMode       = "blendMode";
PROP_CONSTEXPR const char* kJsonKeyFrames       = "keyFrames";
PROP_CONSTEXPR const char* kJsonFrame           = "frame";
PROP_CONSTEXPR const char* kJsonOperation       = "operation";
PROP_CONSTEXPR const char* kJsonEasing          = "easing";
PROP_CONSTEXPR const char* kJsonPath            = "path";
PROP_CONSTEXPR const char* kJsonTx              = "tx";
PROP_CONSTEXPR const char* kJsonTy              = "ty";
PROP_CONSTEXPR const char* kJsonSx              = "sx";
PROP_CONSTEXPR const char* kJsonSy              = "sy";
PROP_CONSTEXPR const char* kJsonAngle           = "angle";
PROP_CONSTEXPR const char* kJsonCx              = "cx";
PROP_CONSTEXPR const char* kJsonCy              = "cy";
PROP_CONSTEXPR const char* kJsonVisible         = "visible";
PROP_CONSTEXPR const char* kJsonC1x             = "c1x";
PROP_CONSTEXPR const char* kJsonC1y             = "c1y";
PROP_CONSTEXPR const char* kJsonC2x             = "c2x";
PROP_CONSTEXPR const char* kJsonC2y             = "c2y";
PROP_CONSTEXPR const char* kJsonVx              = "vx";
PROP_CONSTEXPR const char* kJsonVy              = "vy";
PROP_CONSTEXPR const char* kJsonPoints          = "points";
PROP_CONSTEXPR const char* kJsonX               = "x";
PROP_CONSTEXPR const char* kJsonY               = "y";

// 布局 JSON（顶层/通用）
PROP_CONSTEXPR const char* kJsonTheme           = "theme";
PROP_CONSTEXPR const char* kJsonComponents      = "components";
PROP_CONSTEXPR const char* kJsonLayouts         = "layouts";
PROP_CONSTEXPR const char* kJsonControls        = "controls";
PROP_CONSTEXPR const char* kJsonDialogs         = "dialogs";
PROP_CONSTEXPR const char* kViewportScaleMode   = "viewport-scale-mode";
PROP_CONSTEXPR const char* kViewportScaleOff     = "off";
PROP_CONSTEXPR const char* kViewportScaleFit     = "fit";
PROP_CONSTEXPR const char* kViewportScaleStretch = "stretch";
PROP_CONSTEXPR const char* kJsonViewport        = "viewport";
PROP_CONSTEXPR const char* kJsonViewportScaleMode = "scale-mode";
PROP_CONSTEXPR const char* kJsonRect            = "rect";
PROP_CONSTEXPR const char* kJsonW               = "w";
PROP_CONSTEXPR const char* kJsonH               = "h";
PROP_CONSTEXPR const char* kJsonScale           = "scale";
PROP_CONSTEXPR const char* kJsonId              = "id";
PROP_CONSTEXPR const char* kJsonFont            = "font";
PROP_CONSTEXPR const char* kJsonFontResource = "font-resource";   // 内存字体引用（String 属性 font-resource 的 JSON 键）
PROP_CONSTEXPR const char* kJsonFontFile = "font-file";       // 任意字体文件路径（String 属性 font-file 的 JSON 键）
PROP_CONSTEXPR const char* kJsonSize            = "size";
PROP_CONSTEXPR const char* kJsonStyle           = "style";
PROP_CONSTEXPR const char* kJsonColors          = "colors";
PROP_CONSTEXPR const char* kJsonText            = "text";
PROP_CONSTEXPR const char* kJsonTextShadow = "text-shadow";
PROP_CONSTEXPR const char* kJsonShadow          = "shadow";
PROP_CONSTEXPR const char* kJsonEnabled         = "enabled";
PROP_CONSTEXPR const char* kJsonOffset          = "offset";
PROP_CONSTEXPR const char* kJsonCaption         = "caption";
PROP_CONSTEXPR const char* kJsonAlignment       = "alignment";
PROP_CONSTEXPR const char* kJsonEvents          = "events";
PROP_CONSTEXPR const char* kJsonBind            = "bind";
PROP_CONSTEXPR const char* kJsonSource          = "source";
PROP_CONSTEXPR const char* kJsonMode            = "mode";
PROP_CONSTEXPR const char* kJsonChildren        = "children";
PROP_CONSTEXPR const char* kJsonMargin          = "margin";
PROP_CONSTEXPR const char* kJsonLeft            = "left";
PROP_CONSTEXPR const char* kJsonTop             = "top";
PROP_CONSTEXPR const char* kJsonRight           = "right";
PROP_CONSTEXPR const char* kJsonBottom          = "bottom";
PROP_CONSTEXPR const char* kJsonR               = "r";
PROP_CONSTEXPR const char* kJsonG               = "g";
PROP_CONSTEXPR const char* kJsonB               = "b";
PROP_CONSTEXPR const char* kJsonA               = "a";
PROP_CONSTEXPR const char* kJsonNormal          = "normal";
PROP_CONSTEXPR const char* kJsonHover           = "hover";
PROP_CONSTEXPR const char* kJsonPressed         = "pressed";
PROP_CONSTEXPR const char* kJsonDisabled        = "disabled";
PROP_CONSTEXPR const char* kJsonTitle           = "title";
PROP_CONSTEXPR const char* kJsonTabs            = "tabs";
PROP_CONSTEXPR const char* kJsonPage            = "page";
PROP_CONSTEXPR const char* kJsonPosition        = "position";
PROP_CONSTEXPR const char* kTabPosition           = "position";   // TabControl 属性系统键（TabPosition）
 PROP_CONSTEXPR const char* kTabPositionTop        = "top";
 PROP_CONSTEXPR const char* kTabPositionBottom     = "bottom";
 PROP_CONSTEXPR const char* kTabPositionLeft       = "left";
 PROP_CONSTEXPR const char* kTabPositionRight      = "right";
PROP_CONSTEXPR const char* kJsonCurrentIndex    = "currentIndex";
PROP_CONSTEXPR const char* kTabCurrentIndex      = "currentIndex";   // TabControl 属性系统键
PROP_CONSTEXPR const char* kJsonValue           = "value";
PROP_CONSTEXPR const char* kJsonItems           = "items";
PROP_CONSTEXPR const char* kJsonLabel           = "label";
PROP_CONSTEXPR const char* kJsonColor           = "color";
PROP_CONSTEXPR const char* kJsonBorderVisible = "border-visible";

// 布局 JSON（控件级）
PROP_CONSTEXPR const char* kJsonCaptionLabel = "caption-label";
PROP_CONSTEXPR const char* kJsonCaptionSize = "caption-size";
PROP_CONSTEXPR const char* kJsonEnableTextShadow = "text-shadow-enable";
PROP_CONSTEXPR const char* kJsonActors              = "actors";
PROP_CONSTEXPR const char* kJsonMatchParentRect = "match-parent-rect";
PROP_CONSTEXPR const char* kJsonFile                = "file";
PROP_CONSTEXPR const char* kJsonImage               = "image";                // 独立 image 节点：图片文件路径
PROP_CONSTEXPR const char* kJsonImageResource = "image-resource";        // 独立 image 节点：内存资源 ID
PROP_CONSTEXPR const char* kJsonResourceId          = "resourceId";
PROP_CONSTEXPR const char* kJsonProviderName        = "providerName";
PROP_CONSTEXPR const char* kProviderPrefix          = "provider:";            // 资源引用前缀：provider:<resourceId>
PROP_CONSTEXPR const char* kJsonResourceProviders   = "resourceProviders";    // 布局顶层挂载点数组键
PROP_CONSTEXPR const char* kJsonRPMountName         = "name";                 // 挂载点项：资源名
PROP_CONSTEXPR const char* kJsonRPMountPath         = "path";                 // 挂载点项：相对路径
PROP_CONSTEXPR const char* kJsonScaleType = "scale-type";
PROP_CONSTEXPR const char* kJsonAlpha               = "alpha";            // 图片透明度 0-255
PROP_CONSTEXPR const char* kJsonLuotiAni            = "luotiAni";
PROP_CONSTEXPR const char* kLuotiAni              = "luotiAni";   // Button 属性系统键（SetPtr）
PROP_CONSTEXPR const char* kJsonPlaying             = "playing";           // 内嵌动画：置 true 挂树后自动播放
PROP_CONSTEXPR const char* kJsonPlaceholder         = "placeholder";
PROP_CONSTEXPR const char* kJsonPasswordMode = "password-mode";
PROP_CONSTEXPR const char* kJsonPasswordChar = "password-char";
PROP_CONSTEXPR const char* kJsonEditable            = "editable";
PROP_CONSTEXPR const char* kJsonItemHeight = "item-height";
PROP_CONSTEXPR const char* kJsonSelectedIndex = "selected-index";
PROP_CONSTEXPR const char* kJsonArrowWidth = "arrow-width";
// -- StatusBar --
PROP_CONSTEXPR const char* kStatusBar             = "status-bar";
PROP_CONSTEXPR const char* kJsonAlign             = "align";
PROP_CONSTEXPR const char* kItemIcon              = "icon";                // StatusBar/Menu item 图标（资源引用）
PROP_CONSTEXPR const char* kJsonFontSize = "font-size";           // StatusBar 字号
PROP_CONSTEXPR const char* kJsonMenu              = "menu";               // StatusBar item 内嵌菜单（单数）
PROP_CONSTEXPR const char* kJsonMaxVisibleItems = "max-visible-items";
PROP_CONSTEXPR const char* kJsonCycleEnabled = "cycle-enabled";
PROP_CONSTEXPR const char* kJsonTransparent         = "transparent";
PROP_CONSTEXPR const char* kJsonBgColor = "background";
PROP_CONSTEXPR const char* kJsonBorderColor = "border";
PROP_CONSTEXPR const char* kJsonLayout              = "layout";
PROP_CONSTEXPR const char* kJsonGap                 = "gap";
PROP_CONSTEXPR const char* kJsonPadding             = "padding";
PROP_CONSTEXPR const char* kJsonColumns             = "columns";
PROP_CONSTEXPR const char* kJsonRows                = "rows";
// ListView JSON 键
PROP_CONSTEXPR const char* kJsonMultiSelect = "multi-select";
PROP_CONSTEXPR const char* kJsonHeaderHeight = "header-height";
PROP_CONSTEXPR const char* kJsonGridlines           = "gridlines";
PROP_CONSTEXPR const char* kJsonHorizontalGridlines = "horizontal-gridlines";
PROP_CONSTEXPR const char* kJsonMinColumnWidth = "min-column-width";
PROP_CONSTEXPR const char* kJsonSortColumn = "sort-column";
PROP_CONSTEXPR const char* kJsonSortAscending = "sort-ascending";
PROP_CONSTEXPR const char* kJsonSortable            = "sortable";
PROP_CONSTEXPR const char* kJsonIcon                = "icon";                // 列头/行图标（StatusBar icon 机制）
PROP_CONSTEXPR const char* kJsonCellControls        = "cellControls";        // 单元格级控件 [{"col","control"}]
PROP_CONSTEXPR const char* kJsonFlowWeight = "flow-weight";
PROP_CONSTEXPR const char* kJsonAnchor              = "anchor";
PROP_CONSTEXPR const char* kJsonAnchorOffset        = "anchorOffset";
PROP_CONSTEXPR const char* kJsonGrid                = "grid";
PROP_CONSTEXPR const char* kJsonRow                 = "row";
PROP_CONSTEXPR const char* kJsonCol                 = "col";
PROP_CONSTEXPR const char* kJsonRowSpan             = "rowSpan";
PROP_CONSTEXPR const char* kJsonColSpan             = "colSpan";
PROP_CONSTEXPR const char* kJsonPresets             = "presets";
PROP_CONSTEXPR const char* kJsonPresetLayout        = "presetLayout";
PROP_CONSTEXPR const char* kJsonSwatchSize = "closed-swatch-size";
PROP_CONSTEXPR const char* kJsonClosedFontSize = "closed-font-size";
PROP_CONSTEXPR const char* kJsonClosedTextColor = "closed-text-color";
PROP_CONSTEXPR const char* kJsonPopupBGColor = "popup-bg";
PROP_CONSTEXPR const char* kJsonEdgeMargin = "edge-margin";
PROP_CONSTEXPR const char* kJsonResizable           = "resizable";
PROP_CONSTEXPR const char* kJsonTitleBar = "title-bar";
PROP_CONSTEXPR const char* kJsonBg                  = "bg";
PROP_CONSTEXPR const char* kJsonTitleText = "title-text";
PROP_CONSTEXPR const char* kJsonMenus               = "menus";
PROP_CONSTEXPR const char* kJsonShortcut            = "shortcut";
PROP_CONSTEXPR const char* kJsonChecked             = "checked";
PROP_CONSTEXPR const char* kJsonWordWrap = "word-wrap";
PROP_CONSTEXPR const char* kJsonLineHeight = "line-height";
PROP_CONSTEXPR const char* kJsonLineSpacingRatio = "line-spacing-ratio";
PROP_CONSTEXPR const char* kJsonEnableExpand = "expand";
PROP_CONSTEXPR const char* kJsonDebugDraw = "debug-draw";
PROP_CONSTEXPR const char* kJsonScrollBarThickness = "scrollbar-thickness";
PROP_CONSTEXPR const char* kJsonCheckState = "check-state";
PROP_CONSTEXPR const char* kJsonSizeRatio = "size-ratio";
PROP_CONSTEXPR const char* kJsonTriState = "tri-state";
PROP_CONSTEXPR const char* kJsonCheckColor          = "check-color";
PROP_CONSTEXPR const char* kJsonCrossColor          = "cross-color";
PROP_CONSTEXPR const char* kJsonIndeterminateColor  = "indeterminate-color";
PROP_CONSTEXPR const char* kJsonBoxBorderColor      = "box-border-color";
PROP_CONSTEXPR const char* kJsonRange               = "range";
PROP_CONSTEXPR const char* kJsonMin                 = "min";
PROP_CONSTEXPR const char* kJsonMax                 = "max";
PROP_CONSTEXPR const char* kJsonCustomText = "custom-text";
PROP_CONSTEXPR const char* kJsonAnimationSpeed = "animation-speed";
PROP_CONSTEXPR const char* kJsonProgressColor = "progress";
PROP_CONSTEXPR const char* kJsonBackgroundColor = "background";
PROP_CONSTEXPR const char* kJsonTrack               = "track";
PROP_CONSTEXPR const char* kJsonThumb               = "thumb";
PROP_CONSTEXPR const char* kJsonTick                = "tick";
PROP_CONSTEXPR const char* kJsonThickness           = "thickness";
PROP_CONSTEXPR const char* kJsonFillColor = "fill";
PROP_CONSTEXPR const char* kJsonHoverColor = "hover";
PROP_CONSTEXPR const char* kJsonInterval            = "interval";
PROP_CONSTEXPR const char* kJsonLength              = "length";
PROP_CONSTEXPR const char* kJsonShowValueLabel = "show-value-label";
PROP_CONSTEXPR const char* kJsonLabelFormat = "label-format";
PROP_CONSTEXPR const char* kJsonLabelGap = "label-gap";
PROP_CONSTEXPR const char* kJsonPageStep = "page-step";
PROP_CONSTEXPR const char* kJsonDecimals            = "decimals";
PROP_CONSTEXPR const char* kJsonReadOnly = "read-only";
PROP_CONSTEXPR const char* kJsonButtonWidth = "button-width";
PROP_CONSTEXPR const char* kJsonOrientation         = "orientation";
PROP_CONSTEXPR const char* kJsonVerticalAlign = "vertical-align";
PROP_CONSTEXPR const char* kJsonTextMode = "text-mode";
PROP_CONSTEXPR const char* kJsonStep                = "step";
PROP_CONSTEXPR const char* kJsonReverse             = "reverse";
PROP_CONSTEXPR const char* kJsonBarHeight = "bar-height";
PROP_CONSTEXPR const char* kJsonManualPosition = "manual-position";
PROP_CONSTEXPR const char* kJsonItemHeightRatio = "item-height-ratio";
PROP_CONSTEXPR const char* kJsonPresetCols          = "cols";
PROP_CONSTEXPR const char* kJsonPresetRows          = "rows";
PROP_CONSTEXPR const char* kJsonFirstPanel = "first-linked";
PROP_CONSTEXPR const char* kJsonSecondPanel = "second-linked";
PROP_CONSTEXPR const char* kJsonMinFirst = "first-min";
PROP_CONSTEXPR const char* kJsonMinSecond = "second-min";
PROP_CONSTEXPR const char* kJsonRatio               = "ratio";
PROP_CONSTEXPR const char* kJsonIndentWidth = "indent-width";
PROP_CONSTEXPR const char* kJsonRowHeight = "row-height";
PROP_CONSTEXPR const char* kJsonCycleNavigation = "cycle-navigation";
PROP_CONSTEXPR const char* kJsonDefaultExpand = "default-expand";
PROP_CONSTEXPR const char* kJsonExpanded            = "expanded";
PROP_CONSTEXPR const char* kJsonUserData            = "userData";
// TreeView item 级增强（二期）：前置控件容器 + 逐 Item 字体/间隔
PROP_CONSTEXPR const char* kJsonLeadingControl      = "leadingControl";   // 前置控件描述（复用控件 JSON：type/checked/image 等）
PROP_CONSTEXPR const char* kJsonLeadingGap          = "leadingGap";       // 控件容器与文本之间的间隔（局部 px）
PROP_CONSTEXPR const char* kJsonItemFont            = "font";             // 逐 Item 字体枚举（如 "harmonyos-sans-sc-bold"）
PROP_CONSTEXPR const char* kJsonItemFontSize        = "size";             // 逐 Item 字号（0 = 继承 TreeView 级）
PROP_CONSTEXPR const char* kJsonPageSize = "page-size";
PROP_CONSTEXPR const char* kJsonStepSize = "step-size";
PROP_CONSTEXPR const char* kJsonCentered            = "centered";
PROP_CONSTEXPR const char* kJsonCloseOnEsc = "close-on-esc";
PROP_CONSTEXPR const char* kJsonCloseOnClickOutside = "close-on-click-outside";
PROP_CONSTEXPR const char* kJsonConfirmButton = "confirm-button";
PROP_CONSTEXPR const char* kJsonCancelButton = "cancel-button";
PROP_CONSTEXPR const char* kJsonButtonHeight = "button-height";
PROP_CONSTEXPR const char* kJsonButtonGap = "button-gap";
PROP_CONSTEXPR const char* kJsonTemplate            = "template";
PROP_CONSTEXPR const char* kJsonProps               = "props";
PROP_CONSTEXPR const char* kJsonDefault             = "default";
PROP_CONSTEXPR const char* kJsonXScale              = "xScale";
PROP_CONSTEXPR const char* kJsonYScale              = "yScale";
PROP_CONSTEXPR const char* kJsonEnable              = "enable";

// 布局 JSON 事件键（events 对象）
PROP_CONSTEXPR const char* kEventKeyClick            = "onClick";
PROP_CONSTEXPR const char* kEventKeyEnter            = "onEnter";
PROP_CONSTEXPR const char* kEventKeyTextChanged      = "onTextChanged";
PROP_CONSTEXPR const char* kEventKeySelectionChanged = "onSelectionChanged";
PROP_CONSTEXPR const char* kEventKeyCheckChanged     = "onCheckChanged";
PROP_CONSTEXPR const char* kEventKeySelect           = "onSelect";
PROP_CONSTEXPR const char* kEventKeyValueChanged     = "onValueChanged";
PROP_CONSTEXPR const char* kEventKeyPositionChanged  = "onPositionChanged";
PROP_CONSTEXPR const char* kEventKeyColorChanged     = "onColorChanged";
PROP_CONSTEXPR const char* kEventKeyConfirm          = "onConfirm";
PROP_CONSTEXPR const char* kEventKeyCancel           = "onCancel";
PROP_CONSTEXPR const char* kEventKeyClose            = "onClose";
PROP_CONSTEXPR const char* kEventKeySplitterMoved    = "onSplitterMoved";
PROP_CONSTEXPR const char* kEventKeyPropertyChanged  = "onPropertyChanged";

// -- 控件类型名（布局 JSON type 字段，小写 kebab-case）--
PROP_CONSTEXPR const char* kControlTypeLabel = "label";
PROP_CONSTEXPR const char* kControlTypeButton = "button";
PROP_CONSTEXPR const char* kControlTypeEditBox = "edit-box";
PROP_CONSTEXPR const char* kControlTypeComboBox = "combo-box";
PROP_CONSTEXPR const char* kControlTypeTextArea = "text-area";
PROP_CONSTEXPR const char* kControlTypeCheckBox = "check-box";
PROP_CONSTEXPR const char* kControlTypeProgressBar = "progress-bar";
PROP_CONSTEXPR const char* kControlTypeSlider = "slider";
PROP_CONSTEXPR const char* kControlTypeScrollBar = "scroll-bar";
PROP_CONSTEXPR const char* kControlTypePanel = "panel";
PROP_CONSTEXPR const char* kControlTypeWinFrame = "win-frame";
PROP_CONSTEXPR const char* kControlTypeColorPicker = "color-picker";
PROP_CONSTEXPR const char* kControlTypePopup = "popup";
PROP_CONSTEXPR const char* kPropContextMenu = "context-menu";
PROP_CONSTEXPR const char* kJsonContextMenu = "context-menu";  // 控件级右键菜单（ContextMenu）
PROP_CONSTEXPR const char* kJsonCells        = "cells";        // ListView 行单元格
PROP_CONSTEXPR const char* kJsonControl      = "control";      // cellControls 元素内嵌控件
PROP_CONSTEXPR const char* kJsonOnClick      = "onClick";      // 项级点击事件键
PROP_CONSTEXPR const char* kControlTypeConfirmPopup = "confirm-popup";
PROP_CONSTEXPR const char* kControlTypeDialog = "dialog";
PROP_CONSTEXPR const char* kControlTypeMenuBar = "menu-bar";
PROP_CONSTEXPR const char* kControlTypeNumericUpDown = "numeric-up-down";
PROP_CONSTEXPR const char* kControlTypeSplitter = "splitter";
PROP_CONSTEXPR const char* kControlTypeTreeView = "tree-view";
PROP_CONSTEXPR const char* kControlTypeImageButton = "image-button";
PROP_CONSTEXPR const char* kControlTypeImage       = "image";
PROP_CONSTEXPR const char* kControlTypeAnimation = "animation";
PROP_CONSTEXPR const char* kControlTypeShape = "shape";
PROP_CONSTEXPR const char* kControlTypeListView = "list-view";
PROP_CONSTEXPR const char* kControlTypeStatusBar = "status-bar";
PROP_CONSTEXPR const char* kControlTypeTabControl = "tab-control";
PROP_CONSTEXPR const char* kControlTypeHandleControl = "handle-control";   // 仅 C ABI GetControlType 返回（JSON 无此类型）
PROP_CONSTEXPR const char* kControlTypeMenuItem = "menu-item";              // 同上（menu-bar 内部项）
PROP_CONSTEXPR const char* kControlTypeMenuPanel = "menu-panel";            // 同上（menu-bar 内部面板）

// ============================================================
// -- LuotiAni 枚举值 --
// ============================================================

// Easing
PROP_CONSTEXPR const char* kEasingLinear            = "linear";
PROP_CONSTEXPR const char* kEasingEaseIn            = "ease-in";
PROP_CONSTEXPR const char* kEasingEaseOut           = "ease-out";
PROP_CONSTEXPR const char* kEasingEaseInOut         = "ease-in-out";
PROP_CONSTEXPR const char* kEasingQuad              = "quad";
PROP_CONSTEXPR const char* kEasingSine              = "sine";
PROP_CONSTEXPR const char* kEasingCubicBezierPrefix = "cubic-bezier(";

// Path 类型
PROP_CONSTEXPR const char* kPathBezier              = "bezier";
PROP_CONSTEXPR const char* kPathParabola            = "parabola";
PROP_CONSTEXPR const char* kPathCatmullRom          = "catmull-rom";

// Layer 类型
PROP_CONSTEXPR const char* kLayerTypeImage          = "image";
PROP_CONSTEXPR const char* kLayerTypeShape          = "shape";
PROP_CONSTEXPR const char* kLayerTypeText           = "text";
PROP_CONSTEXPR const char* kLayerTypeNull           = "null_layer";

// Operation 类型
PROP_CONSTEXPR const char* kOpTypeTranslate         = "translate";
PROP_CONSTEXPR const char* kOpTypeScale             = "scale";
PROP_CONSTEXPR const char* kOpTypeRotate            = "rotate";
PROP_CONSTEXPR const char* kOpTypeOpacity           = "opacity";
PROP_CONSTEXPR const char* kOpTypeVisible           = "visible";
PROP_CONSTEXPR const char* kOpTypeNull              = "null_operation";

// BlendMode
PROP_CONSTEXPR const char* kBlendNormal                = "normal";
PROP_CONSTEXPR const char* kBlendAdditive              = "additive";
PROP_CONSTEXPR const char* kBlendAdditivePremultiplied = "additive-premultiplied";
PROP_CONSTEXPR const char* kBlendModulate              = "modulate";
PROP_CONSTEXPR const char* kBlendBlend                 = "blend";
PROP_CONSTEXPR const char* kBlendBlendPremultiplied    = "blend-premultiplied";
PROP_CONSTEXPR const char* kBlendMultiply              = "multiply";

// ============================================================
// -- 布局 JSON 枚举值（小写风格，与 CABI 属性值域一致）--
// ============================================================

// 布局引擎类型
PROP_CONSTEXPR const char* kLayoutTypeHFlow        = "h-flow";
PROP_CONSTEXPR const char* kLayoutTypeVFlow        = "v-flow";
PROP_CONSTEXPR const char* kLayoutTypeAnchor       = "anchor";
PROP_CONSTEXPR const char* kLayoutTypeGrid         = "grid";

// Anchor 扩展（布局引擎使用，小写风格）
PROP_CONSTEXPR const char* kJsonAnchorTopStretch   = "top-stretch";
PROP_CONSTEXPR const char* kJsonAnchorBottomStretch = "bottom-stretch";
PROP_CONSTEXPR const char* kJsonAnchorLeftStretch  = "left-stretch";
PROP_CONSTEXPR const char* kJsonAnchorRightStretch = "right-stretch";
PROP_CONSTEXPR const char* kJsonAnchorFill         = "fill";

// 字体样式
PROP_CONSTEXPR const char* kFontStyleNormal        = "normal";
PROP_CONSTEXPR const char* kFontStyleBold          = "bold";
PROP_CONSTEXPR const char* kFontStyleItalic        = "italic";
PROP_CONSTEXPR const char* kFontStyleUnderline     = "underline";
PROP_CONSTEXPR const char* kFontStyleStrikethrough = "strikethrough";
PROP_CONSTEXPR const char* kFontBold          = "font-bold";             // 字体样式--粗体（Bool 属性，可组合）
PROP_CONSTEXPR const char* kFontItalic        = "font-italic";           // 斜体
PROP_CONSTEXPR const char* kFontUnderline     = "font-underline";        // 下划线
PROP_CONSTEXPR const char* kFontStrikethrough = "font-strikethrough";    // 删除线

// 绑定模式
PROP_CONSTEXPR const char* kBindModeOneWay         = "oneway";
PROP_CONSTEXPR const char* kBindModeTwoWay         = "twoway";

// 组件 prop 类型
PROP_CONSTEXPR const char* kPropTypeString         = "string";
PROP_CONSTEXPR const char* kPropTypeNumber         = "number";
PROP_CONSTEXPR const char* kPropTypeBool           = "bool";

// GridSize 后缀
PROP_CONSTEXPR const char* kGridSizeAuto           = "auto";
PROP_CONSTEXPR const char* kGridSizeFrSuffix       = "fr";
PROP_CONSTEXPR const char* kGridSizePxSuffix       = "px";

// 菜单项类型
PROP_CONSTEXPR const char* kMenuTypeSeparator      = "separator";

// ============================================================
// -- Theme / 状态 / 其它 --
// ============================================================

// StateColor 子键
PROP_CONSTEXPR const char* kStateKeyNormal         = "normal";
PROP_CONSTEXPR const char* kStateKeyHover          = "hover";
PROP_CONSTEXPR const char* kStateKeyPressed        = "pressed";
PROP_CONSTEXPR const char* kStateKeyDisabled       = "disabled";

// Theme 路径片段
PROP_CONSTEXPR const char* kThemeFontsPrefix       = "fonts.";
PROP_CONSTEXPR const char* kThemeNameSuffix        = ".name";
PROP_CONSTEXPR const char* kThemeSizeSuffix        = ".size";
PROP_CONSTEXPR const char* kThemeColorsPrefix      = "colors.";
PROP_CONSTEXPR const char* kThemeBgSuffix          = ".bg";
PROP_CONSTEXPR const char* kThemeBorderSuffix      = ".border";
PROP_CONSTEXPR const char* kThemeTextSuffix        = ".text";
PROP_CONSTEXPR const char* kThemeTextShadowSuffix  = ".textShadow";
PROP_CONSTEXPR const char* kThemeCheckboxCheck          = "colors.checkbox.check";
PROP_CONSTEXPR const char* kThemeCheckboxCross          = "colors.checkbox.cross";
PROP_CONSTEXPR const char* kThemeCheckboxIndeterminate  = "colors.checkbox.indeterminate";
PROP_CONSTEXPR const char* kThemeCheckboxBoxBorder      = "colors.checkbox.boxBorder";
PROP_CONSTEXPR const char* kThemeProgressbarProgress    = "colors.progressbar.progress";
PROP_CONSTEXPR const char* kThemeProgressbarTrack       = "colors.progressbar.track";
PROP_CONSTEXPR const char* kThemeCategoryDefault        = "default";

// Theme 字体类别
PROP_CONSTEXPR const char* kThemeCatLabel          = "label";
PROP_CONSTEXPR const char* kThemeCatButton         = "button";
PROP_CONSTEXPR const char* kThemeCatEditBox        = "editbox";
PROP_CONSTEXPR const char* kThemeCatCheckBox       = "checkbox";
PROP_CONSTEXPR const char* kThemeCatPanel          = "panel";
PROP_CONSTEXPR const char* kThemeCatColorPicker    = "colorpicker";
PROP_CONSTEXPR const char* kThemeCatMenuBar        = "menubar";
PROP_CONSTEXPR const char* kThemeCatTextArea       = "textarea";
PROP_CONSTEXPR const char* kThemeCatProgressBar    = "progressbar";
PROP_CONSTEXPR const char* kThemeCatNumericUpDown  = "numericupdown";
PROP_CONSTEXPR const char* kThemeCatSplitter       = "splitter";
PROP_CONSTEXPR const char* kThemeCatTreeView       = "treeview";
PROP_CONSTEXPR const char* kThemeCatScrollBar      = "scrollbar";
PROP_CONSTEXPR const char* kThemeCatPopup          = "popup";
PROP_CONSTEXPR const char* kThemeCatConfirmPopup   = "confirmpopup";
PROP_CONSTEXPR const char* kThemeCatDialog         = "dialog";

// 颜色通道
PROP_CONSTEXPR const char* kChannelR               = "r";
PROP_CONSTEXPR const char* kChannelG               = "g";
PROP_CONSTEXPR const char* kChannelB               = "b";
PROP_CONSTEXPR const char* kChannelA               = "a";

// 组件系统
PROP_CONSTEXPR const char* kPlaceholderOpen        = "{{";
PROP_CONSTEXPR const char* kPlaceholderClose       = "}}";
PROP_CONSTEXPR const char* kCompEventPrefix        = "_comp_";
PROP_CONSTEXPR const char* kIdPrefixSep            = "__";

// 后端配置键
PROP_CONSTEXPR const char* kBackendKeyVsync        = "vsync";
PROP_CONSTEXPR const char* kBackendKeySwapRatio    = "swap-ratio";
PROP_CONSTEXPR const char* kBackendKeyRendererName = "renderer-name";

// 其它
PROP_CONSTEXPR const char* kDefaultWindowTitle     = "UICornerstone";
PROP_CONSTEXPR const char* kDefaultWinFrameTitle   = "WinFrame";
PROP_CONSTEXPR const char* kDefaultMenuTitle       = "Menu";
PROP_CONSTEXPR const char* kAssetsDirName          = "assets";

// 属性缺口补齐
PROP_CONSTEXPR const char* kEditable               = "editable";
PROP_CONSTEXPR const char* kScaleTypeStretch       = "stretch";
PROP_CONSTEXPR const char* kScaleTypeFitCenter     = "fit-center";
PROP_CONSTEXPR const char* kScaleTypeCenterCrop    = "center-crop";
PROP_CONSTEXPR const char* kScaleTypeNone          = "none";
PROP_CONSTEXPR const char* kScaleTypeTile          = "tile";

// 对齐值（小写风格，AlignmentMode/Actor 锚点 CABI 属性字符串值）
PROP_CONSTEXPR const char* kAlignLowerTopLeft     = "top-left";
PROP_CONSTEXPR const char* kAlignLowerMidLeft     = "mid-left";
PROP_CONSTEXPR const char* kAlignLowerBottomLeft  = "bottom-left";
PROP_CONSTEXPR const char* kAlignLowerTopRight    = "top-right";
PROP_CONSTEXPR const char* kAlignLowerMidRight    = "mid-right";
PROP_CONSTEXPR const char* kAlignLowerBottomRight = "bottom-right";
PROP_CONSTEXPR const char* kAlignLowerTopCenter   = "top-center";
PROP_CONSTEXPR const char* kAlignLowerCenter      = "center";
PROP_CONSTEXPR const char* kAlignLowerBottomCenter= "bottom-center";

PROP_NS_END

#undef PROP_CONSTEXPR
#undef PROP_NS_BEGIN
#undef PROP_NS_END
