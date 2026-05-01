#ifndef ConstDefH
#define ConstDefH

#include <filesystem>

#include "Utility.h"

// using namespace std; // 先不使用 using namespace std，避免在Windows下编译因命名冲突而失败
namespace fs = std::filesystem;

// 前向声明
struct SDL_Color;
class Margin; // 前向声明Margin类

// ********************************在这里定义创建主窗体的初始参数********************************
#define APP_NAME u8"LuotiAniTool"
#define APP_VERSION "1.0.0"
#define APP_AUTHOR "SeaOcean"
#define APP_COPYRIGHT "Copyright (c) 2023 SeaOcean"
#define APP_COMPANY "SeaOcean.Ltd."
#define APP_IDENTIFY "com.seaocean.luotianitool"
// 主窗体横向像素个数
#define INITIAL_WIDTH  1920
// 主窗体纵向像素个数
#define INITIAL_HEIGHT 1080
// 主窗体初始位置X坐标，SDL_WINDOWPOS_CENTERED表示居中
#define INITIAL_POSX SDL_WINDOWPOS_CENTERED
// 主窗体初始位置Y坐标，SDL_WINDOWPOS_CENTERED表示居中
#define INITIAL_POSY SDL_WINDOWPOS_CENTERED
// 在这里定义窗口特性标志
#define WINDOW_FLAG (SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY)
// 是否开启垂直同步，0表示不启用垂直同步，1表示启用垂直同步
#define VSYNC_FLAG (0)

// **********************************************************************************************
#define DEFAULT_BTN_MS_INTERVAL (200)       // 默认按钮事件间隔时间，单位毫秒
#define DEFAULT_BTN_REPEAT_MS_INTERVAL (50) // 默认按钮重复事件间隔时间，单位毫秒
#define DEFAULT_BTN_MS_REPEAT (50)          // 按钮自动重复的时间间隔，单位毫秒

class ConstDef {
public:
    // 路径定义
    static const fs::path workforldPath;  // 工作目录路径
    static const fs::path pathPrefix;     // 资源路径前缀

    // 缺省彩色定义
    static const SDL_Color DEFAULT_DISABLED_COLOR;
    static const SDL_Color DEFAULT_NORMAL_COLOR;
    static const SDL_Color DEFAULT_HOVER_COLOR;
    static const SDL_Color DEFAULT_DOWN_COLOR;

    static const SDL_Color DEFAULT_BORDER_DISABLED_COLOR;
    static const SDL_Color DEFAULT_BORDER_NORMAL_COLOR;
    static const SDL_Color DEFAULT_BORDER_HOVER_COLOR;
    static const SDL_Color DEFAULT_BORDER_DOWN_COLOR;

    static const SDL_Color DEFAULT_TEXT_DISABLED_COLOR;
    static const SDL_Color DEFAULT_TEXT_NORMAL_COLOR;
    static const SDL_Color DEFAULT_TEXT_HOVER_COLOR;
    static const SDL_Color DEFAULT_TEXT_DOWN_COLOR;

    static const SDL_Color DEFAULT_TEXT_SHADOW_DISABLED_COLOR;
    static const SDL_Color DEFAULT_TEXT_SHADOW_NORMAL_COLOR;
    static const SDL_Color DEFAULT_TEXT_SHADOW_HOVER_COLOR;
    static const SDL_Color DEFAULT_TEXT_SHADOW_DOWN_COLOR;

    // 按钮相关常量
    static const float BUTTON_CAPTION_SIZE;
    static const SDL_Color BUTTON_NORMAL_COLOR;
    static const SDL_Color BUTTON_HOVER_COLOR;
    static const SDL_Color BUTTON_DOWN_COLOR;

    static const SDL_Color BUTTON_NORMAL_TEXT_COLOR;
    static const SDL_Color BUTTON_HOVER_TEXT_COLOR;
    static const SDL_Color BUTTON_DOWN_TEXT_COLOR;

    // 文本标签相关常量
    static const Margin LABEL_CAPTION_MARGIN;

    // 菜单相关常量
    static const float MENU_BAR_HEIGHT;
    static const SDL_Color MENU_BORDER_COLOR;
    static const SDL_Color MENU_NORMAL_COLOR;
    static const SDL_Color MENU_HOVER_COLOR;
    static const SDL_Color MENU_DOWN_COLOR;
    static const SDL_Color MENU_DISABLED_COLOR;
    static const SDL_Color MENU_TEXT_NORMAL_COLOR;
    static const SDL_Color MENU_TEXT_HOVER_COLOR;
    static const SDL_Color MENU_TEXT_DOWN_COLOR;
    static const SDL_Color MENU_TEXT_DISABLED_COLOR;
    static const float MENU_MAIN_MENU_HEIGHT;
    static const float MENU_TEXT_SIZE;

    static const Margin MENU_ITEM_MARGIN;
    static const Margin MENU_CAPTION_MARGIN;
    static const float MENU_SEPARATOR_HEIGHT;   // 横向菜单分隔符高度
    static const float MENU_SEPARATOR_WIDTH;    // 竖向菜单分隔符宽度

    // 窗口相关常量
    static const float WINDOW_TITLE_HEIGHT;

    // 字体相关常量
    static const float FONT_MARGIN;

    // 滚动条相关常量
    static const float SCROLLBAR_WIDTH;
    static const float SCROLLBAR_MIN_THUMB_SIZE;
    static const SDL_Color SCROLLBAR_TRACK_COLOR;
    static const SDL_Color SCROLLBAR_THUMB_COLOR;
    static const SDL_Color SCROLLBAR_THUMB_HOVER_COLOR;
    static const SDL_Color SCROLLBAR_THUMB_PRESSED_COLOR;

    // 编辑框相关常量
    static const float EDITBOX_DEFAULT_HEIGHT;
    static const float EDITBOX_BORDER_WIDTH;
    static const float EDITBOX_PADDING;
    static const float EDITBOX_CURSOR_WIDTH;
    static const int32_t EDITBOX_CURSOR_BLINK_INTERVAL;
    static const SDL_Color EDITBOX_SELECTION_COLOR;
    static const char EDITBOX_DEFAULT_PASSWORD_CHAR;

    // 复选框相关常量
    static const float BOX_PEN_WIDTH;
    static const float MARK_PEN_WIDTH;
    static const Margin CHECKBOX_MARGIN;
    static const float CHECKBOX_SIZE_RATIO;
    static const Margin CHECKBOX_BOX_MARGIN;
    static const float CHECKBOX_DEFAULT_CAPTION_SIZE;
    static const SDL_Color CHECKBOX_CHECK_COLOR;
    static const SDL_Color CHECKBOX_CROSS_COLOR;
    static const SDL_Color CHECKBOX_INDETERMINATE_COLOR;

    // 进度条相关常量
    static const float PROGRESSBAR_DEFAULT_HEIGHT;
    static const float PROGRESSBAR_MIN_HEIGHT;
    static const SDL_Color PROGRESSBAR_DEFAULT_PROGRESS_COLOR;
    static const SDL_Color PROGRESSBAR_DEFAULT_BACKGROUND_COLOR;
    static const float PROGRESSBAR_DEFAULT_ANIMATION_SPEED;
    static const SDL_Color PROGRESSBAR_PROGRESS_COLOR;
    static const SDL_Color PROGRESSBAR_BACKGROUND_COLOR;
    static const SDL_Color PROGRESSBAR_TEXT_COLOR;
    static const float PROGRESSBAR_ANIMATION_SPEED;
    static const float PROGRESSBAR_DEFAULT_FONT_SIZE;
    static const float PROGRESSBAR_TEXT_MARGIN;
};

#endif // ConstDefH