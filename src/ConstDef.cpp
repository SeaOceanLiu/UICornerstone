#include <filesystem>
#include <SDL3/SDL.h>

#include "ConstDef.h"

// 路径定义
const fs::path ConstDef::pathPrefix = fs::path(std::string(SDL_GetBasePath()) + "assets");
const fs::path ConstDef::workforldPath = ConstDef::pathPrefix;

// 缺省彩色定义
const SDL_Color ConstDef::DEFAULT_DISABLED_COLOR = { 128, 128, 128, 255 }; // 灰色
const SDL_Color ConstDef::DEFAULT_NORMAL_COLOR = { 23, 23, 24, 255 };  // Steel Blue
const SDL_Color ConstDef::DEFAULT_HOVER_COLOR = { 100, 149, 237, 255 };  // Cornflower Blue
const SDL_Color ConstDef::DEFAULT_DOWN_COLOR = { 30, 144, 255, 255 };    // Dodger Blue

const SDL_Color ConstDef::DEFAULT_BORDER_DISABLED_COLOR = { 128, 128, 128, 255 }; // 灰色
const SDL_Color ConstDef::DEFAULT_BORDER_NORMAL_COLOR = { 83, 83, 90, 255 };
const SDL_Color ConstDef::DEFAULT_BORDER_HOVER_COLOR = { 128, 128, 128, 255 };
const SDL_Color ConstDef::DEFAULT_BORDER_DOWN_COLOR = { 200, 200, 200, 255 };

const SDL_Color ConstDef::DEFAULT_TEXT_DISABLED_COLOR = { 109, 109, 109, 255 }; // 灰色
const SDL_Color ConstDef::DEFAULT_TEXT_NORMAL_COLOR = { 219, 219, 219, 255 };   // 黑色
const SDL_Color ConstDef::DEFAULT_TEXT_HOVER_COLOR = { 128, 128, 128, 255 };    // 灰色
const SDL_Color ConstDef::DEFAULT_TEXT_DOWN_COLOR = { 200, 200, 200, 255 };     // 深灰色,

const SDL_Color ConstDef::DEFAULT_TEXT_SHADOW_DISABLED_COLOR = { 128, 128, 128, 255 }; // 灰色
const SDL_Color ConstDef::DEFAULT_TEXT_SHADOW_NORMAL_COLOR = { 255, 255, 255, 255 };  // 白色
const SDL_Color ConstDef::DEFAULT_TEXT_SHADOW_HOVER_COLOR = { 255, 255, 255, 255 };  // 白色
const SDL_Color ConstDef::DEFAULT_TEXT_SHADOW_DOWN_COLOR = { 255, 255, 255, 255 };  // 白色

// 按钮相关常量
const float ConstDef::BUTTON_CAPTION_SIZE = 16;
const SDL_Color ConstDef::BUTTON_NORMAL_COLOR = { 70, 130, 180, 255 };  // Steel Blue
const SDL_Color ConstDef::BUTTON_HOVER_COLOR = { 100, 149, 237, 255 };  // Cornflower Blue
const SDL_Color ConstDef::BUTTON_DOWN_COLOR = { 30, 144, 255, 255 };    // Dodger Blue

const SDL_Color ConstDef::BUTTON_NORMAL_TEXT_COLOR = { 0, 0, 0, 255 };  // 黑色
const SDL_Color ConstDef::BUTTON_HOVER_TEXT_COLOR = { 128, 128, 128, 255 };   // 灰色
const SDL_Color ConstDef::BUTTON_DOWN_TEXT_COLOR  = { 200, 200, 200, 255 }; // 深灰色,

// 文本标签相关常量
const Margin ConstDef::LABEL_CAPTION_MARGIN = {5.0f, 5.0f, 5.0f, 5.0f};  // 菜单标题边距：左5，上5，右5，下5

// 菜单相关常量
const float ConstDef::MENU_BAR_HEIGHT = 52.0f;
const SDL_Color ConstDef::MENU_BORDER_COLOR = { 83, 83, 90, 255 }; // 边框颜色
const SDL_Color ConstDef::MENU_NORMAL_COLOR = { 23, 23, 24, 255 };  // 默认背景色
const SDL_Color ConstDef::MENU_HOVER_COLOR = { 51, 65, 85, 255 };   // 默认背景色
const SDL_Color ConstDef::MENU_DOWN_COLOR = { 51, 65, 85, 255 };    // 默认背景色
const SDL_Color ConstDef::MENU_DISABLED_COLOR = ConstDef::MENU_NORMAL_COLOR; // 禁用背景色
const SDL_Color ConstDef::MENU_TEXT_NORMAL_COLOR = {203, 213, 225, 255};    // 默认字体颜色
const SDL_Color ConstDef::MENU_TEXT_HOVER_COLOR = { 203, 213, 225, 255 };   // 默认字体颜色
const SDL_Color ConstDef::MENU_TEXT_DOWN_COLOR = { 203, 213, 225, 255 };    // 默认字体颜色
const SDL_Color ConstDef::MENU_TEXT_DISABLED_COLOR = { 109, 109, 109, 255 }; // 禁用字体颜色
const float ConstDef::MENU_MAIN_MENU_HEIGHT = 34.0f;
const float ConstDef::MENU_TEXT_SIZE = 20.0f;
const Margin ConstDef::MENU_ITEM_MARGIN = {5.0f, 0.0f, 5.0f, 0.0f}; // 菜单项内边距：左5，上5，右5，下5
const Margin ConstDef::MENU_CAPTION_MARGIN = {10.0f, 5.0f, 10.0f, 5.0f};  // 菜单标题边距：左10，上5，右10，下5
const float ConstDef::MENU_SEPARATOR_HEIGHT = 2.f; // 横向菜单分隔符高度
const float ConstDef::MENU_SEPARATOR_WIDTH = 2.f; // 竖向菜单分隔符宽度

const float ConstDef::WINDOW_TITLE_HEIGHT = 30;
const float ConstDef::FONT_MARGIN = 10;

// 滚动条相关常量
const float ConstDef::SCROLLBAR_WIDTH = 16.0f;
const float ConstDef::SCROLLBAR_MIN_THUMB_SIZE = 20.0f;
const SDL_Color ConstDef::SCROLLBAR_TRACK_COLOR = { 50, 50, 50, 255 };
const SDL_Color ConstDef::SCROLLBAR_THUMB_COLOR = { 120, 120, 120, 255 };
const SDL_Color ConstDef::SCROLLBAR_THUMB_HOVER_COLOR = { 150, 150, 150, 255 };
const SDL_Color ConstDef::SCROLLBAR_THUMB_PRESSED_COLOR = { 100, 100, 100, 255 };

// 编辑框相关常量
const float ConstDef::EDITBOX_DEFAULT_HEIGHT = 36.0f;
const float ConstDef::EDITBOX_BORDER_WIDTH = 1.0f;
const float ConstDef::EDITBOX_PADDING = 8.0f;
const float ConstDef::EDITBOX_CURSOR_WIDTH = 2.0f;
const int32_t ConstDef::EDITBOX_CURSOR_BLINK_INTERVAL = 500;
const SDL_Color ConstDef::EDITBOX_SELECTION_COLOR = { 50, 100, 180, 255 };
const char ConstDef::EDITBOX_DEFAULT_PASSWORD_CHAR = '*';

// 复选框相关常量
const float ConstDef::BOX_PEN_WIDTH = 2.0f;
const float ConstDef::MARK_PEN_WIDTH = 2.5f;
const Margin ConstDef::CHECKBOX_MARGIN = {5.0f, 5.0f, 5.0f, 5.0f};
const float ConstDef::CHECKBOX_SIZE_RATIO = 1.0f;
const Margin ConstDef::CHECKBOX_BOX_MARGIN = {2.0f, 2.0f, 2.0f, 2.0f};
const float ConstDef::CHECKBOX_DEFAULT_CAPTION_SIZE = 16.0f;
const SDL_Color ConstDef::CHECKBOX_CHECK_COLOR = { 76, 175, 80, 255 };     // 绿色
const SDL_Color ConstDef::CHECKBOX_CROSS_COLOR = { 244, 67, 54, 255 };     // 红色
const SDL_Color ConstDef::CHECKBOX_INDETERMINATE_COLOR = { 158, 158, 158, 255 }; // 灰色

// 进度条相关常量
const float ConstDef::PROGRESSBAR_DEFAULT_HEIGHT = 20.0f;
const float ConstDef::PROGRESSBAR_MIN_HEIGHT = 8.0f;
const SDL_Color ConstDef::PROGRESSBAR_DEFAULT_PROGRESS_COLOR = { 76, 175, 80, 255 };  // 绿色
const SDL_Color ConstDef::PROGRESSBAR_DEFAULT_BACKGROUND_COLOR = { 200, 200, 200, 255 }; // 浅灰色
const float ConstDef::PROGRESSBAR_DEFAULT_ANIMATION_SPEED = 0.1f;
const SDL_Color ConstDef::PROGRESSBAR_PROGRESS_COLOR = { 76, 175, 80, 255 };
const SDL_Color ConstDef::PROGRESSBAR_BACKGROUND_COLOR = { 200, 200, 200, 255 };
const SDL_Color ConstDef::PROGRESSBAR_TEXT_COLOR = { 255, 255, 255, 255 };
const float ConstDef::PROGRESSBAR_ANIMATION_SPEED = 0.1f;
const float ConstDef::PROGRESSBAR_DEFAULT_FONT_SIZE = 14.0f;
const float ConstDef::PROGRESSBAR_TEXT_MARGIN = 5.0f;