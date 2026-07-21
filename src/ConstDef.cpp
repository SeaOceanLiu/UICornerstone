#include <filesystem>
#include <string>

#include "ConstDef.h"
#include "PlatformUtils.h"

// 路径定义
const fs::path ConstDef::pathPrefix = fs::path(Platform::GetBasePath() + "assets");
const fs::path ConstDef::workforldPath = ConstDef::pathPrefix;

// 缺省彩色定义
const SColor ConstDef::DEFAULT_DISABLED_COLOR(128, 128, 128, 255);
const SColor ConstDef::DEFAULT_NORMAL_COLOR(23, 23, 24, 255);
const SColor ConstDef::DEFAULT_HOVER_COLOR(100, 149, 237, 255);
const SColor ConstDef::DEFAULT_DOWN_COLOR(30, 144, 255, 255);

const SColor ConstDef::DEFAULT_BORDER_DISABLED_COLOR(128, 128, 128, 255);
const SColor ConstDef::DEFAULT_BORDER_NORMAL_COLOR(83, 83, 90, 255);
const SColor ConstDef::DEFAULT_BORDER_HOVER_COLOR(128, 128, 128, 255);
const SColor ConstDef::DEFAULT_BORDER_DOWN_COLOR(200, 200, 200, 255);

const SColor ConstDef::DEFAULT_TEXT_DISABLED_COLOR(109, 109, 109, 255);
const SColor ConstDef::DEFAULT_TEXT_NORMAL_COLOR(219, 219, 219, 255);
const SColor ConstDef::DEFAULT_TEXT_HOVER_COLOR(128, 128, 128, 255);
const SColor ConstDef::DEFAULT_TEXT_DOWN_COLOR(200, 200, 200, 255);

const SColor ConstDef::DEFAULT_TEXT_SHADOW_DISABLED_COLOR(128, 128, 128, 255);
const SColor ConstDef::DEFAULT_TEXT_SHADOW_NORMAL_COLOR(255, 255, 255, 255);
const SColor ConstDef::DEFAULT_TEXT_SHADOW_HOVER_COLOR(255, 255, 255, 255);
const SColor ConstDef::DEFAULT_TEXT_SHADOW_DOWN_COLOR(255, 255, 255, 255);

const float ConstDef::BUTTON_CAPTION_SIZE = 16;
const SColor ConstDef::BUTTON_NORMAL_COLOR(70, 130, 180, 255);
const SColor ConstDef::BUTTON_HOVER_COLOR(100, 149, 237, 255);
const SColor ConstDef::BUTTON_DOWN_COLOR(30, 144, 255, 255);

const SColor ConstDef::BUTTON_NORMAL_TEXT_COLOR(0, 0, 0, 255);
const SColor ConstDef::BUTTON_HOVER_TEXT_COLOR(128, 128, 128, 255);
const SColor ConstDef::BUTTON_DOWN_TEXT_COLOR(200, 200, 200, 255);

const Margin ConstDef::LABEL_CAPTION_MARGIN = {5.0f, 5.0f, 5.0f, 5.0f};

const float ConstDef::MENU_BAR_HEIGHT = 52.0f;
const SColor ConstDef::MENU_BORDER_COLOR(83, 83, 90, 255);
const SColor ConstDef::MENU_NORMAL_COLOR(23, 23, 24, 255);
const SColor ConstDef::MENU_HOVER_COLOR(51, 65, 85, 255);
const SColor ConstDef::MENU_DOWN_COLOR(51, 65, 85, 255);
const SColor ConstDef::MENU_DISABLED_COLOR = ConstDef::MENU_NORMAL_COLOR;
const SColor ConstDef::MENU_TEXT_NORMAL_COLOR(203, 213, 225, 255);
const SColor ConstDef::MENU_TEXT_HOVER_COLOR(203, 213, 225, 255);
const SColor ConstDef::MENU_TEXT_DOWN_COLOR(203, 213, 225, 255);
const SColor ConstDef::MENU_TEXT_DISABLED_COLOR(109, 109, 109, 255);
const float ConstDef::MENU_MAIN_MENU_HEIGHT = 34.0f;
const float ConstDef::MENU_TEXT_SIZE = 20.0f;
const Margin ConstDef::MENU_ITEM_MARGIN = {5.0f, 0.0f, 5.0f, 0.0f};
const Margin ConstDef::MENU_CAPTION_MARGIN = {10.0f, 5.0f, 10.0f, 5.0f};
const float ConstDef::MENU_SEPARATOR_HEIGHT = 2.f;
const float ConstDef::MENU_SEPARATOR_WIDTH = 2.f;

const float ConstDef::WINDOW_TITLE_HEIGHT = 30;
const float ConstDef::FONT_MARGIN = 10;

const float ConstDef::SCROLLBAR_WIDTH = 16.0f;
const float ConstDef::SCROLLBAR_MIN_THUMB_SIZE = 20.0f;
const SColor ConstDef::SCROLLBAR_TRACK_COLOR(50, 50, 50, 255);
const SColor ConstDef::SCROLLBAR_THUMB_COLOR(120, 120, 120, 255);
const SColor ConstDef::SCROLLBAR_THUMB_HOVER_COLOR(150, 150, 150, 255);
const SColor ConstDef::SCROLLBAR_THUMB_PRESSED_COLOR(100, 100, 100, 255);

const float ConstDef::EDITBOX_DEFAULT_HEIGHT = 36.0f;
const float ConstDef::EDITBOX_BORDER_WIDTH = 1.0f;
const float ConstDef::EDITBOX_PADDING = 8.0f;
const float ConstDef::EDITBOX_CURSOR_WIDTH = 2.0f;
const int32_t ConstDef::EDITBOX_CURSOR_BLINK_INTERVAL = 500;
const SColor ConstDef::EDITBOX_SELECTION_COLOR(50, 100, 180, 255);
const char ConstDef::EDITBOX_DEFAULT_PASSWORD_CHAR = '*';

const float ConstDef::BOX_PEN_WIDTH = 2.0f;
const float ConstDef::MARK_PEN_WIDTH = 2.5f;
const Margin ConstDef::CHECKBOX_MARGIN = {5.0f, 5.0f, 5.0f, 5.0f};
const float ConstDef::CHECKBOX_SIZE_RATIO = 1.0f;
const Margin ConstDef::CHECKBOX_BOX_MARGIN = {2.0f, 2.0f, 2.0f, 2.0f};
const float ConstDef::CHECKBOX_DEFAULT_CAPTION_SIZE = 16.0f;
const SColor ConstDef::CHECKBOX_CHECK_COLOR(76, 175, 80, 255);
const SColor ConstDef::CHECKBOX_CROSS_COLOR(244, 67, 54, 255);
const SColor ConstDef::CHECKBOX_INDETERMINATE_COLOR(158, 158, 158, 255);

const float ConstDef::PROGRESSBAR_DEFAULT_HEIGHT = 20.0f;
const float ConstDef::PROGRESSBAR_MIN_HEIGHT = 8.0f;
const SColor ConstDef::PROGRESSBAR_DEFAULT_PROGRESS_COLOR(76, 175, 80, 255);
const SColor ConstDef::PROGRESSBAR_DEFAULT_BACKGROUND_COLOR(200, 200, 200, 255);
const float ConstDef::PROGRESSBAR_DEFAULT_ANIMATION_SPEED = 0.1f;
const SColor ConstDef::PROGRESSBAR_PROGRESS_COLOR(76, 175, 80, 255);
const SColor ConstDef::PROGRESSBAR_BACKGROUND_COLOR(200, 200, 200, 255);
const SColor ConstDef::PROGRESSBAR_TEXT_COLOR(255, 255, 255, 255);
const float ConstDef::PROGRESSBAR_ANIMATION_SPEED = 0.1f;
const float ConstDef::PROGRESSBAR_DEFAULT_FONT_SIZE = 14.0f;
const float ConstDef::PROGRESSBAR_TEXT_MARGIN = 5.0f;

const float ConstDef::SLIDER_TRACK_THICKNESS = 4.0f;
const float ConstDef::SLIDER_THUMB_SIZE = 16.0f;
const SColor ConstDef::SLIDER_TRACK_COLOR(60, 60, 60, 255);
const SColor ConstDef::SLIDER_TRACK_FILL_COLOR(0, 120, 255, 255);
const SColor ConstDef::SLIDER_THUMB_COLOR(255, 255, 255, 255);
const SColor ConstDef::SLIDER_THUMB_BORDER_COLOR(0, 120, 255, 255);
const SColor ConstDef::SLIDER_THUMB_HOVER_COLOR(200, 200, 200, 255);
const SColor ConstDef::SLIDER_LABEL_COLOR(255, 255, 255, 255);
const float ConstDef::SLIDER_TICK_INTERVAL = 0.0f;
const float ConstDef::SLIDER_TICK_LENGTH = 8.0f;
const SColor ConstDef::SLIDER_TICK_COLOR(100, 100, 100, 255);

// ComboBox 常量
const int     ConstDef::COMBOBOX_DEFAULT_MAX_VISIBLE_ITEMS = 6;
const float   ConstDef::COMBOBOX_DEFAULT_ITEM_HEIGHT       = 24.0f;
const float   ConstDef::COMBOBOX_DEFAULT_ARROW_WIDTH       = 20.0f;
const float   ConstDef::COMBOBOX_DROPDOWN_OFFSET           = 2.0f;
const float   ConstDef::COMBOBOX_LIST_PADDING              = 4.0f;
const float   ConstDef::COMBOBOX_ARROW_WIDTH_RATIO         = 0.40f;
const float   ConstDef::COMBOBOX_ARROW_HEIGHT_RATIO        = 0.346f;
const float   ConstDef::COMBOBOX_ARROW_MARGIN              = 4.0f;
const int     ConstDef::COMBOBOX_DEFAULT_FONT_SIZE         = 14;
const SColor  ConstDef::COMBOBOX_DEFAULT_ARROW_COLOR(180, 180, 180, 255);
const SColor  ConstDef::COMBOBOX_DEFAULT_ARROW_HOVER_COLOR(255, 255, 255, 255);
const SColor  ConstDef::COMBOBOX_DEFAULT_ITEM_SELECTED_COLOR(0, 120, 215, 255);
const SColor  ConstDef::COMBOBOX_DEFAULT_ITEM_HOVER_COLOR(50, 50, 50, 255);
const SColor  ConstDef::COMBOBOX_DEFAULT_ITEM_DISABLED_COLOR(80, 80, 80, 255);
const SColor  ConstDef::COMBOBOX_DEFAULT_LIST_BG_COLOR(37, 37, 38, 255);
const SColor  ConstDef::COMBOBOX_DEFAULT_LIST_BORDER_COLOR(60, 60, 60, 255);

// ColorPicker 常量
const float ConstDef::COLORPICKER_SWATCH_SIZE = 16.0f;
const float ConstDef::COLORPICKER_HEX_GAP = 4.0f;
const int   ConstDef::COLORPICKER_HEX_FONT_SIZE = 12;
const float ConstDef::COLORPICKER_POPUP_WIDTH = 296.0f;
const float ConstDef::COLORPICKER_POPUP_HEIGHT = 422.0f;
const float ConstDef::COLORPICKER_POPUP_PADDING = 10.0f;
const float ConstDef::COLORPICKER_SLIDER_LABEL_W = 14.0f;
const float ConstDef::COLORPICKER_HEX_HASH_W = 10.0f;
const SColor ConstDef::COLORPICKER_POPUP_BG(60, 60, 65, 255);
const SColor ConstDef::COLORPICKER_POPUP_BORDER(100, 100, 100, 255);
const SColor ConstDef::COLORPICKER_SHADOW_COLOR(0, 0, 0, 80);
const int   ConstDef::COLORPICKER_PRESET_COLS = 5;
const int   ConstDef::COLORPICKER_PRESET_ROWS = 4;
const float ConstDef::COLORPICKER_PRESET_CELL_W = 40.0f;
const float ConstDef::COLORPICKER_PRESET_CELL_H = 32.0f;
const float ConstDef::COLORPICKER_PRESET_GAP = 4.0f;
const SColor ConstDef::COLORPICKER_PRESET_SELECTED(255, 255, 255, 255);
const SColor ConstDef::COLORPICKER_PRESET_NORMAL(80, 80, 80, 255);
const float ConstDef::COLORPICKER_HEX_INPUT_H = 22.0f;
const float ConstDef::COLORPICKER_SLIDER_H = 20.0f;
const float ConstDef::COLORPICKER_SLIDER_GAP = 4.0f;
const float ConstDef::COLORPICKER_BTN_W = 60.0f;
const float ConstDef::COLORPICKER_BTN_H = 24.0f;
const float ConstDef::COLORPICKER_BTN_GAP = 8.0f;

const std::vector<SColor> ConstDef::COLORPICKER_DEFAULT_PRESETS = {
    SColor(0xFF, 0x00, 0x00, 0xFF), SColor(0x00, 0xFF, 0x00, 0xFF),
    SColor(0x00, 0x00, 0xFF, 0xFF), SColor(0xFF, 0xFF, 0x00, 0xFF),
    SColor(0xFF, 0x00, 0xFF, 0xFF), SColor(0x00, 0xFF, 0xFF, 0xFF),
    SColor(0xFF, 0xFF, 0xFF, 0xFF), SColor(0x00, 0x00, 0x00, 0xFF),
    SColor(0x80, 0x80, 0x80, 0xFF), SColor(0xC0, 0xC0, 0xC0, 0xFF),
    SColor(0x80, 0x00, 0x00, 0xFF), SColor(0x80, 0x80, 0x00, 0xFF),
    SColor(0x00, 0x80, 0x00, 0xFF), SColor(0x80, 0x00, 0x80, 0xFF),
    SColor(0x00, 0x80, 0x80, 0xFF), SColor(0x00, 0x00, 0x80, 0xFF),
    SColor(0xFF, 0xA5, 0x00, 0xFF), SColor(0xFF, 0xC0, 0xCB, 0xFF),
    SColor(0xA5, 0x2A, 0x2A, 0xFF), SColor(0xF0, 0xF8, 0xFF, 0xFF),
};

// ── NumericUpDown ──
const float ConstDef::NUMERICUPDOWN_BUTTON_WIDTH           = 28.0f;
const float ConstDef::NUMERICUPDOWN_MIN_WIDTH_PADDING      = 10.0f;
const float ConstDef::NUMERICUPDOWN_TRIANGLE_HEIGHT_RATIO  = 0.22f;
const float ConstDef::NUMERICUPDOWN_TRIANGLE_WIDTH_RATIO   = 1.2f;
const float ConstDef::NUMERICUPDOWN_ARROW_GAP              = 3.0f;
const int   ConstDef::NUMERICUPDOWN_FONT_SIZE              = 14;
const int   ConstDef::NUMERICUPDOWN_DECIMALS_MAX           = 6;
const double ConstDef::NUMERICUPDOWN_REPEAT_DELAY_SEC      = 0.5;
const double ConstDef::NUMERICUPDOWN_REPEAT_INTERVAL_SEC   = 0.1;
const double ConstDef::NUMERICUPDOWN_DEFAULT_STEP          = 1.0;
const double ConstDef::NUMERICUPDOWN_DEFAULT_PAGESTEP      = 10.0;
const int    ConstDef::NUMERICUPDOWN_DEFAULT_DECIMALS      = 0;
const SColor ConstDef::NUMERICUPDOWN_BG_COLOR(45, 45, 45, 255);
const SColor ConstDef::NUMERICUPDOWN_ARROW_BG(55, 55, 55, 255);
const SColor ConstDef::NUMERICUPDOWN_ARROW_BG_HOVER(65, 65, 65, 255);
const SColor ConstDef::NUMERICUPDOWN_ARROW_BG_PRESS(75, 75, 75, 255);
const SColor ConstDef::NUMERICUPDOWN_SEPARATOR_COLOR(40, 40, 40, 255);
const SColor ConstDef::NUMERICUPDOWN_ARROW_COLOR(180, 180, 180, 255);
const SColor ConstDef::NUMERICUPDOWN_ARROW_HOVER_COLOR(255, 255, 255, 255);
const SColor ConstDef::NUMERICUPDOWN_ARROW_PRESS_COLOR(220, 220, 220, 255);

// ── Splitter ──
const float ConstDef::SPLITTER_THICKNESS_H        = 4.0f;
const float ConstDef::SPLITTER_THICKNESS_V        = 6.0f;
const float ConstDef::SPLITTER_MIN_SIZE_DEFAULT   = 20.0f;
const SColor ConstDef::SPLITTER_COLOR_NORMAL(128, 128, 128, 255);
const SColor ConstDef::SPLITTER_COLOR_HOVER(74, 144, 217, 255);
const SColor ConstDef::SPLITTER_COLOR_DRAG(58, 128, 201, 255);
const int    ConstDef::SPLITTER_DOUBLE_CLICK_MS   = 300;
const float  ConstDef::SPLITTER_KEY_STEP          = 5.0f;
const float  ConstDef::SPLITTER_KEY_FINE_STEP     = 1.0f;

const std::unordered_map<FontName, std::string> ConstDef::fontFiles = {
    {FontName::Asul_Bold, "fonts/Asul-Bold.ttf"},
    {FontName::Asul_Regular, "fonts/Asul-Regular.ttf"},
    {FontName::HarmonyOS_Sans_Condensed_Regular, "fonts/HarmonyOS_Sans_Condensed_Regular.ttf"},
    {FontName::HarmonyOS_Sans_Condensed_Thin, "fonts/HarmonyOS_Sans_Condensed_Thin.ttf"},
    {FontName::HarmonyOS_Sans_SC_Black, "fonts/HarmonyOS_Sans_SC_Black.ttf"},
    {FontName::HarmonyOS_Sans_SC_Bold, "fonts/HarmonyOS_Sans_SC_Bold.ttf"},
    {FontName::HarmonyOS_Sans_SC_Light, "fonts/HarmonyOS_Sans_SC_Light.ttf"},
    {FontName::HarmonyOS_Sans_SC_Medium, "fonts/HarmonyOS_Sans_SC_Medium.ttf"},
    {FontName::HarmonyOS_Sans_SC_Regular, "fonts/HarmonyOS_Sans_SC_Regular.ttf"},
    {FontName::HarmonyOS_Sans_SC_Thin, "fonts/HarmonyOS_Sans_SC_Thin.ttf"},
    {FontName::MapleMono_NF_CN_Bold, "fonts/MapleMono-NF-CN-Bold.ttf"},
    {FontName::MapleMono_NF_CN_BoldItalic, "fonts/MapleMono-NF-CN-BoldItalic.ttf"},
    {FontName::MapleMono_NF_CN_ExtraBold, "fonts/MapleMono-NF-CN-ExtraBold.ttf"},
    {FontName::MapleMono_NF_CN_ExtraBoldItalic, "fonts/MapleMono-NF-CN-ExtraBoldItalic.ttf"},
    {FontName::MapleMono_NF_CN_ExtraLight, "fonts/MapleMono-NF-CN-ExtraLight.ttf"},
    {FontName::MapleMono_NF_CN_ExtraLightItalic, "fonts/MapleMono-NF-CN-ExtraLightItalic.ttf"},
    {FontName::MapleMono_NF_CN_Italic, "fonts/MapleMono-NF-CN-Italic.ttf"},
    {FontName::MapleMono_NF_CN_Light, "fonts/MapleMono-NF-CN-Light.ttf"},
    {FontName::MapleMono_NF_CN_LightItalic, "fonts/MapleMono-NF-CN-LightItalic.ttf"},
    {FontName::MapleMono_NF_CN_Medium, "fonts/MapleMono-NF-CN-Medium.ttf"},
    {FontName::MapleMono_NF_CN_MediumItalic, "fonts/MapleMono-NF-CN-MediumItalic.ttf"},
    {FontName::MapleMono_NF_CN_Regular, "fonts/MapleMono-NF-CN-Regular.ttf"},
    {FontName::MapleMono_NF_CN_SemiBold, "fonts/MapleMono-NF-CN-SemiBold.ttf"},
    {FontName::MapleMono_NF_CN_SemiBoldItalic, "fonts/MapleMono-NF-CN-SemiBoldItalic.ttf"},
    {FontName::MapleMono_NF_CN_Thin, "fonts/MapleMono-NF-CN-Thin.ttf"},
    {FontName::MapleMono_NF_CN_ThinItalic, "fonts/MapleMono-NF-CN-ThinItalic.ttf"},
    {FontName::Muyao_Softbrush, "fonts/Muyao-Softbrush.ttf"},
    {FontName::Quando_Regular, "fonts/Quando-Regular.ttf"},
};
