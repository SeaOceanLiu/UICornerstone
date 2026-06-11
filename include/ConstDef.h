#ifndef ConstDefH
#define ConstDefH

#include <filesystem>
#include <unordered_map>
#include <string>

#include "Utility.h"

// using namespace std; // 先不使用 using namespace std，避免在Windows下编译因命名冲突而失败
namespace fs = std::filesystem;

#include "SColor.h"

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
// 主窗体初始位置X坐标，0x2FFF0000表示居中(SDL_WINDOWPOS_CENTERED)
#define INITIAL_POSX 0x2FFF0000
// 主窗体初始位置Y坐标，0x2FFF0000表示居中(SDL_WINDOWPOS_CENTERED)
#define INITIAL_POSY 0x2FFF0000
// 在这里定义窗口特性标志：0x00000020=可调整大小, 0x00002000=高DPI支持
#define WINDOW_FLAG (0x00000020 | 0x00002000)
// 是否开启垂直同步，0表示不启用垂直同步，1表示启用垂直同步
#define VSYNC_FLAG (0)

// **********************************************************************************************
#define DEFAULT_BTN_MS_INTERVAL (200)       // 默认按钮事件间隔时间，单位毫秒
#define DEFAULT_BTN_REPEAT_MS_INTERVAL (50) // 默认按钮重复事件间隔时间，单位毫秒
#define DEFAULT_BTN_MS_REPEAT (50)          // 按钮自动重复的时间间隔，单位毫秒

enum class FontName {
    Asul_Bold,
    Asul_Regular,
    HarmonyOS_Sans_Condensed_Regular,
    HarmonyOS_Sans_Condensed_Thin,
    HarmonyOS_Sans_SC_Black,
    HarmonyOS_Sans_SC_Bold,
    HarmonyOS_Sans_SC_Light,
    HarmonyOS_Sans_SC_Medium,
    HarmonyOS_Sans_SC_Regular,
    HarmonyOS_Sans_SC_Thin,
    MapleMono_NF_CN_Bold,
    MapleMono_NF_CN_BoldItalic,
    MapleMono_NF_CN_ExtraBold,
    MapleMono_NF_CN_ExtraBoldItalic,
    MapleMono_NF_CN_ExtraLight,
    MapleMono_NF_CN_ExtraLightItalic,
    MapleMono_NF_CN_Italic,
    MapleMono_NF_CN_Light,
    MapleMono_NF_CN_LightItalic,
    MapleMono_NF_CN_Medium,
    MapleMono_NF_CN_MediumItalic,
    MapleMono_NF_CN_Regular,
    MapleMono_NF_CN_SemiBold,
    MapleMono_NF_CN_SemiBoldItalic,
    MapleMono_NF_CN_Thin,
    MapleMono_NF_CN_ThinItalic,
    Muyao_Softbrush,
    Quando_Regular
};

class ConstDef {
public:
    // 路径定义
    static const fs::path workforldPath;  // 工作目录路径
    static const fs::path pathPrefix;     // 资源路径前缀

    static const std::unordered_map<FontName, std::string> fontFiles;

    // 缺省彩色定义
    static const SColor DEFAULT_DISABLED_COLOR;
    static const SColor DEFAULT_NORMAL_COLOR;
    static const SColor DEFAULT_HOVER_COLOR;
    static const SColor DEFAULT_DOWN_COLOR;

    static const SColor DEFAULT_BORDER_DISABLED_COLOR;
    static const SColor DEFAULT_BORDER_NORMAL_COLOR;
    static const SColor DEFAULT_BORDER_HOVER_COLOR;
    static const SColor DEFAULT_BORDER_DOWN_COLOR;

    static const SColor DEFAULT_TEXT_DISABLED_COLOR;
    static const SColor DEFAULT_TEXT_NORMAL_COLOR;
    static const SColor DEFAULT_TEXT_HOVER_COLOR;
    static const SColor DEFAULT_TEXT_DOWN_COLOR;

    static const SColor DEFAULT_TEXT_SHADOW_DISABLED_COLOR;
    static const SColor DEFAULT_TEXT_SHADOW_NORMAL_COLOR;
    static const SColor DEFAULT_TEXT_SHADOW_HOVER_COLOR;
    static const SColor DEFAULT_TEXT_SHADOW_DOWN_COLOR;

    // 按钮相关常量
    static const float BUTTON_CAPTION_SIZE;
    static const SColor BUTTON_NORMAL_COLOR;
    static const SColor BUTTON_HOVER_COLOR;
    static const SColor BUTTON_DOWN_COLOR;

    static const SColor BUTTON_NORMAL_TEXT_COLOR;
    static const SColor BUTTON_HOVER_TEXT_COLOR;
    static const SColor BUTTON_DOWN_TEXT_COLOR;

    // 文本标签相关常量
    static const Margin LABEL_CAPTION_MARGIN;

    // 菜单相关常量
    static const float MENU_BAR_HEIGHT;
    static const SColor MENU_BORDER_COLOR;
    static const SColor MENU_NORMAL_COLOR;
    static const SColor MENU_HOVER_COLOR;
    static const SColor MENU_DOWN_COLOR;
    static const SColor MENU_DISABLED_COLOR;
    static const SColor MENU_TEXT_NORMAL_COLOR;
    static const SColor MENU_TEXT_HOVER_COLOR;
    static const SColor MENU_TEXT_DOWN_COLOR;
    static const SColor MENU_TEXT_DISABLED_COLOR;
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
    static const SColor SCROLLBAR_TRACK_COLOR;
    static const SColor SCROLLBAR_THUMB_COLOR;
    static const SColor SCROLLBAR_THUMB_HOVER_COLOR;
    static const SColor SCROLLBAR_THUMB_PRESSED_COLOR;

    // 编辑框相关常量
    static const float EDITBOX_DEFAULT_HEIGHT;
    static const float EDITBOX_BORDER_WIDTH;
    static const float EDITBOX_PADDING;
    static const float EDITBOX_CURSOR_WIDTH;
    static const int32_t EDITBOX_CURSOR_BLINK_INTERVAL;
    static const SColor EDITBOX_SELECTION_COLOR;
    static const char EDITBOX_DEFAULT_PASSWORD_CHAR;

    // 复选框相关常量
    static const float BOX_PEN_WIDTH;
    static const float MARK_PEN_WIDTH;
    static const Margin CHECKBOX_MARGIN;
    static const float CHECKBOX_SIZE_RATIO;
    static const Margin CHECKBOX_BOX_MARGIN;
    static const float CHECKBOX_DEFAULT_CAPTION_SIZE;
    static const SColor CHECKBOX_CHECK_COLOR;
    static const SColor CHECKBOX_CROSS_COLOR;
    static const SColor CHECKBOX_INDETERMINATE_COLOR;

    // 进度条相关常量
    static const float PROGRESSBAR_DEFAULT_HEIGHT;
    static const float PROGRESSBAR_MIN_HEIGHT;
    static const SColor PROGRESSBAR_DEFAULT_PROGRESS_COLOR;
    static const SColor PROGRESSBAR_DEFAULT_BACKGROUND_COLOR;
    static const float PROGRESSBAR_DEFAULT_ANIMATION_SPEED;
    static const SColor PROGRESSBAR_PROGRESS_COLOR;
    static const SColor PROGRESSBAR_BACKGROUND_COLOR;
    static const SColor PROGRESSBAR_TEXT_COLOR;
    static const float PROGRESSBAR_ANIMATION_SPEED;
    static const float PROGRESSBAR_DEFAULT_FONT_SIZE;
    static const float PROGRESSBAR_TEXT_MARGIN;
};

#endif // ConstDefH