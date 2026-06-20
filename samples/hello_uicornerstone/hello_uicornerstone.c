// =========================================================================
// hello_uicornerstone.c --- UICornerstone 最简示例（声明式 UI——JSON 布局）
//
// 【集成模式】完全静态链接
//   整个框架（控件 + 后端）编译进一个 exe，零外部 DLL 依赖。
//   仅需第三方后端 DLL（SDL3.dll / sfml-graphics-3.dll 等）。
//
// 【CMake 构建命令】
//   cmake --build build\sdl3 --config Debug --target hello_uicornerstone
//
// 【学习要点】
//   ① InitFromPlugin — 自动检测后端（SDL3/SFML/Raylib），无需手动传入回调
//   ② RegisterAction — 将 C 函数注册为 JSON 中可引用的动作名称
//   ③ LoadLayout — 解析 JSON 字符串，递归创建控件树
//   ④ FindControl — 通过 id 查找控件，用于运行时更新
//   ⑤ 帧循环 — ProcessEvents / Update / Clear / Render / Present
//
// 【如何开发自己的应用】
//   1. 修改 LAYOUT 中的 JSON 定义你的 UI 布局
//   2. 添加更多回调函数，通过 RegisterAction 注册
//   3. 在 while 循环中添加你的业务逻辑（如游戏状态更新）
//   4. 如需更多控件类型，参考 include/UICornerstoneAPI.h 中的 C ABI
// =========================================================================

#include "UICornerstoneAPI.h"
#include <stdio.h>

// ======== 回调函数 ========
//
// UICornerstone_SetOnClick / RegisterAction 的回调签名：
//   void callback(UIControlHandle ctl, void* userData);
//
// 在 JSON 布局中，通过 "events":{"onClick":"函数名"} 绑定。
// 在代码中，通过 UICornerstone_SetOnClick(handle, fn, userData) 绑定。

static int g_clickCount = 0;

static void onBtnClick(UIControlHandle ctl, void* user) {
    (void)ctl; (void)user;
    g_clickCount++;

    char buf[64];
    snprintf(buf, sizeof(buf), "Clicked: %d", g_clickCount);

    // FindControl 通过 id 在全局控件注册表中查找控件句柄
    UIControlHandle status = UICornerstone_FindControl("status");
    if (status) UICornerstone_SetText(status, buf);
}

// ======== JSON 布局（声明式 UI） ========
//
// 所有空间、坐标、颜色、事件绑定全部在一个 JSON 字符串中定义，
// 调用 LoadLayout 一次性解析构建控件树。零初始化代码。
//
// 支持的 JSON 字段：
//   type     — 控件类型：Panel / Button / Label / CheckBox / EditBox
//               ProgressBar / TextArea / WinFrame / MenuBar
//   id       — 唯一标识符（可选），用于 FindControl 查找
//   rect     — {x, y, w, h} 位置和尺寸
//   caption  — 文本内容（Button / Label / CheckBox 等）
//   font     — {size, name, style} 字体配置（可选）
//   colors   — {background: {normal, hover, pressed}} 颜色（可选）
//   events   — {onClick: "函数名"} 事件绑定（可选）
//   children — 子控件数组（仅 Panel 支持）
//   window   — WinFrame 专用 {title, style, resizeable} 等

static const char* LAYOUT =
"{"
"  \"controls\": [{"
"    \"type\": \"Panel\", \"id\": \"root\","
"    \"rect\": {\"x\":0,\"y\":0,\"w\":800,\"h\":480},"
"    \"children\": ["
"      {\"type\":\"Label\",\"id\":\"title\","
"       \"rect\":{\"x\":20,\"y\":10,\"w\":760,\"h\":30},"
"       \"font\":{\"size\":18},\"caption\":\"UICornerstone Sample\"},"
"      {\"type\":\"Button\",\"id\":\"btn\",\"caption\":\"Click Me\","
"       \"rect\":{\"x\":20,\"y\":60,\"w\":200,\"h\":80},"
"       \"colors\":{\"background\":{\"normal\":\"#4A90D9\",\"hover\":\"#5BA0E9\",\"pressed\":\"#3A80C9\"}},"
"       \"events\":{\"onClick\":\"onBtnClick\"}},"
"      {\"type\":\"Label\",\"id\":\"status\","
"       \"rect\":{\"x\":20,\"y\":160,\"w\":400,\"h\":24},"
"       \"caption\":\"Click the button above\"}"
"    ]"
"  }]}";

// ======== main ========
//
// 应用入口。所有 UICornerstone 应用的生命周期：
//
//   ① Init / InitFromPlugin → 初始化后端和控件引擎
//   ② SetViewport           → 设置渲染视口（像素坐标）
//   ③ 创建控件               → LoadLayout(JSON) 或 CreateXxx 工厂函数
//   ④ 帧循环（while）        → ProcessEvents / Update / Clear / Render / Present
//   ⑤ Shutdown               → 释放所有资源

int main(void) {
    // InitFromPlugin(backendName) 自动加载后端插件。
    // UICORNERSTONE_BACKEND_NAME 由 CMake 编译宏定义。
    if (!UICornerstone_InitFromPlugin(UICORNERSTONE_BACKEND_NAME)) return 1;

    // 视口：整个窗口的渲染区域（左上 x, 左上 y, 宽, 高）
    UICornerstone_SetViewport(0, 0, 800, 480);

    // 注册 JSON 中 onClick 引用的回调函数
    UICornerstone_RegisterAction("onBtnClick", onBtnClick, NULL);

    // 加载 JSON 布局（返回 1=成功，0=失败）
    if (!UICornerstone_LoadLayout(LAYOUT)) {
        UICornerstone_Shutdown();
        return 1;
    }

    // 帧循环：6 个步骤顺序不可颠倒
    while (!UICornerstone_IsQuitRequested()) {
        UICornerstone_ProcessEvents();    // 轮询输入事件并分发到控件树
        UICornerstone_Update(1.0f / 60.0f); // 更新控件状态（dt = 帧时间，秒）
        UICornerstone_Clear();            // 清除帧缓冲
        UICornerstone_Render();           // 绘制控件树
        UICornerstone_Present();          // 交换缓冲到屏幕
    }

    UICornerstone_Shutdown();
    return 0;
}