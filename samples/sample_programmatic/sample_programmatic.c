// =========================================================================
// sample_programmatic.c — UICornerstone 编程式控件创建（命令式 UI）
//
// 【集成模式】完全静态链接
//   与 hello_uicornerstone 相同，框架编译进 exe，零 DLL 依赖。
//
// 【与 JSON 示例的区别】
//   hello_uicornerstone 用 JSON 字符串声明 UI，
//   本示例用 C 代码直接调用工厂函数创建控件。
//   两者可混合使用（部分控件用代码创建，部分用 JSON）。
//
// 【学习要点】
//   ① CreatePanel / CreateButton / CreateLabel — 工厂函数，返回 UIControlHandle
//   ② AddChild — 建立父子关系（Panel 可容纳任意子控件）
//   ③ SetBGColor — 设置按钮背景色，自动生成 hover/pressed 变体
//   ④ SetOnClick — 代码中绑定点击回调（比 JSON events 更灵活）
//   ⑤ 所有 C ABI 函数见 include/UICornerstoneAPI.h
//
// 【如何开发自己的应用】
//   1. 先创建根 Panel（覆盖窗口），再将其他控件 AddChild 进去
//   2. 工厂函数参数：text/name, x, y, w, h（均为 float 像素值）
//   3. 回调中保存句柄到全局变量，用于运行时更新控件
//   4. 参考 include/UICornerstoneAPI.h 查看更多 API
// =========================================================================

#include "UICornerstoneAPI.h"
#include <stdio.h>

// ======== 回调函数 ========
//
// 注意：和 JSON 示例不同，这里不需要 RegisterAction。
// SetOnClick 直接将函数指针绑定到控件，不经过字符串映射。

static int g_clickCount = 0;
static UIControlHandle g_statusLabel = NULL;   // 保存句柄，供回调中更新

static void onBtnClick(UIControlHandle ctl, void* user) {
    (void)ctl; (void)user;

    g_clickCount++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Clicked: %d", g_clickCount);

    // g_statusLabel 在 main 中赋值后保持不变。
    // 如果控件可能在运行时被销毁，需先检查句柄有效性。
    if (g_statusLabel)
        UICornerstone_SetText(g_statusLabel, buf);
}

// ======== main ========

int main(void) {
    // ── 初始化（与 JSON 示例完全相同） ──────────────────────────
    if (!UICornerstone_InitFromPlugin(UICORNERSTONE_BACKEND_NAME)) return 1;
    UICornerstone_SetViewport(0, 0, 800, 480);

    // ── 编程式创建控件树 ────────────────────────────────────────
    //
    // 控件创建与加载 JSON 的二选一路径：
    //   方式 A: UICornerstone_LoadLayout(JSON)  ← hello_uicornerstone
    //   方式 B: 工厂函数 + AddChild              ← 本示例
    //
    // 工厂函数签名（所有控件通用）：
    //   UICornerstone_CreateButton(caption, x, y, w, h) → UIControlHandle
    //   UICornerstone_CreateLabel(text, fontSize, x, y, w, h) → UIControlHandle
    //   返回值：成功→非空句柄，失败→NULL
    //
    // 父子关系规则：
    //   子控件的位置相对于父控件的左上角（绝对坐标）。
    //   父控件移动时，子控件跟随移动。
    //   父控件裁剪子控件的绘制区域。

    // 1) 创建根 Panel（覆盖整个视口）
    UIControlHandle root = UICornerstone_CreatePanel(0, 0, 800, 480);

    // 2) 创建标题标签并挂到根
    UIControlHandle title = UICornerstone_CreateLabel(
        "UICornerstone Sample (Programmatic)", 18,
        20, 10, 760, 30);
    UICornerstone_AddChild(root, title);

    // 3) 创建按钮
    UIControlHandle btn = UICornerstone_CreateButton("Click Me",
        20, 60, 200, 80);

    // SetBGColor: 设置 normal 色，自动生成 hover（变亮 ~30%）和
    // pressed（变暗 ~30%）。参数为 RGBA 分量（0-255）。
    UICornerstone_SetBGColor(btn, 74, 144, 217, 255);

    // SetOnClick: 代码中绑定点击回调。
    // 第三个参数 userData 会透传给回调（本例传 NULL）。
    UICornerstone_SetOnClick(btn, onBtnClick, NULL);
    UICornerstone_AddChild(root, btn);

    // 4) 创建状态标签（用于显示点击次数）
    g_statusLabel = UICornerstone_CreateLabel(
        "Click the button above", 14,
        20, 160, 400, 24);
    UICornerstone_AddChild(root, g_statusLabel);

    // ── 帧循环（与 JSON 示例完全相同） ──────────────────────────
    while (!UICornerstone_IsQuitRequested()) {
        UICornerstone_ProcessEvents();
        UICornerstone_Update(1.0f / 60.0f);
        UICornerstone_Clear();
        UICornerstone_Render();
        UICornerstone_Present();
    }

    UICornerstone_Shutdown();
    return 0;
}