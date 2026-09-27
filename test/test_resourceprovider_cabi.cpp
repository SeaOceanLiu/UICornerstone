// =========================================================================
// test_resourceprovider_cabi.cpp -- MemoryResourceProvider 全链路（all backends）
// 覆盖：Register(拷贝) / Adopt(零拷贝+freeFn契约) / JSON resourceProviders 挂载 /
//       "provider:" 入口分流（布局属性 + 工厂路径 + actors providerName 对象式）/
//       Label font-resource / LuotiAni provider 动画 / 负用例
// 资源读盘发生在场景 CreateInstance 之前；之后引擎全程不碰磁盘（resourceRoot 指向不存在目录）。
// =========================================================================

#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "../../include/UICornerstoneAPI.h"

extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);

// ===== C ABI 函数指针类型 =====
typedef UIInstance (*UICreateInstanceFn)(const UIBackendCallbacks*, const UIInstanceConfig*);
typedef void  (*UISetViewportFn)(UIInstance,float,float,float,float);
typedef void  (*UIProcessEventsFn)(UIInstance);
typedef void  (*UIUpdateFn)(UIInstance,double);
typedef void  (*UIClearFn)(UIInstance);
typedef void  (*UIRenderFn)(UIInstance);
typedef void  (*UIPresentFn)(UIInstance);
typedef int   (*UIIsQuitFn)(UIInstance);
typedef void  (*UIPushUIEventFn)(UIInstance,const UIEvent*);
typedef void  (*UIDestroyInstanceFn)(UIInstance);
typedef int   (*UILoadLayoutFn)(UIInstance,const char*);
typedef void* (*UIFindControlFn)(UIInstance,const char*);
typedef int   (*UISetStringFn)(UIInstance,void*,const char*,const char*);
typedef int   (*UISetBoolFn)(UIInstance,void*,const char*,int);
typedef int   (*UIGetRectFn)(UIInstance,void*,float*,float*,float*,float*);
typedef void* (*UICreateImageFn)(UIInstance,const char*,float,float,float,float,float,float);
typedef void* (*UICreateButtonFn)(UIInstance,const char*,float,float,float,float,float,float);
typedef void* (*UICreateAnimationFn)(UIInstance,const char*,float,float,float,float,float,float);

static UICreateInstanceFn  uiCreateInstance     = nullptr;
static UISetViewportFn     uiSetViewport        = nullptr;
static UIProcessEventsFn   uiProcessEvents      = nullptr;
static UIUpdateFn          uiUpdate             = nullptr;
static UIClearFn           uiClear              = nullptr;
static UIRenderFn          uiRender             = nullptr;
static UIPresentFn         uiPresent            = nullptr;
static UIIsQuitFn          uiIsQuitRequested    = nullptr;
static UIPushUIEventFn     uiPushUIEvent        = nullptr;
static UIDestroyInstanceFn uiDestroyInstance    = nullptr;
static UILoadLayoutFn      uiLoadLayout         = nullptr;
static UIFindControlFn     uiFindControl        = nullptr;
static UISetStringFn       uiSetString          = nullptr;
static UISetBoolFn         uiSetBool            = nullptr;
static UIGetRectFn         uiGetRect            = nullptr;
static UICreateImageFn     uiCreateImage        = nullptr;
static UICreateButtonFn    uiCreateButton       = nullptr;
static UICreateAnimationFn uiCreateAnimation    = nullptr;

static UIInstance      g_inst = nullptr;
static HMODULE         g_uiDll = nullptr;
static UIResourceProviderHandle g_provider = nullptr;
static int             g_autoSec = 3;
static int             g_freeCount = 0;

static void countFree(void* p) { free(p); ++g_freeCount; printf("adopt freeFn invoked (%d)\n", g_freeCount); }

static char* readAllocFile(const char* path, size_t* outLen) {
    FILE* f = fopen(path, "rb");
    if (!f) return nullptr;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        fclose(f);
        return nullptr;
    }
    fclose(f);
    *outLen = (size_t)n;
    return buf;
}

static void loadAllProcs(HMODULE dll) {
#define RESOLVE(name) \
    *(void**)&ui##name = GetProcAddress(dll, "UICornerstone_" #name)

    RESOLVE(CreateInstance);
    RESOLVE(SetViewport);
    RESOLVE(ProcessEvents);
    RESOLVE(Update);
    RESOLVE(Clear);
    RESOLVE(Render);
    RESOLVE(Present);
    RESOLVE(IsQuitRequested);
    RESOLVE(PushUIEvent);
    RESOLVE(DestroyInstance);
    RESOLVE(LoadLayout);
    RESOLVE(FindControl);
    RESOLVE(SetString);
    RESOLVE(SetBool);
    RESOLVE(GetRect);
    RESOLVE(CreateImage);
    RESOLVE(CreateButton);
    RESOLVE(CreateAnimation);
#undef RESOLVE
}

static int runTest(const char* shortName, const char* displayName) {
    printf("=== test_resourceprovider_cabi: UICornerstone.dll + %s ===\n", displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    loadAllProcs(g_uiDll);
    if (!uiCreateInstance) { printf("FAIL: GetProcAddress(CreateInstance)\n"); FreeLibrary(g_uiDll); return 1; }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) { printf("FAIL: GetUIBackendCallbacks\n"); FreeLibrary(g_uiDll); return 1; }
    if (!callbacks->createMemoryResourceProvider || !callbacks->memoryProviderRegister ||
        !callbacks->memoryProviderAdopt || !callbacks->setResourceProvider) {
        printf("FAIL: backend missing memory-provider callbacks\n");
        FreeLibrary(g_uiDll); return 1;
    }

    // ---- 场景初始化之前：资源读入堆内存 + 注册（引擎全程不再碰磁盘）----
    size_t pngLen = 0, pngDownLen = 0, fontLen = 0, aniLen = 0, svgLen = 0;
    char* pngBuf     = readAllocFile("assets/images/cross_up.png", &pngLen);
    char* pngDownBuf = readAllocFile("assets/images/cross_down.png", &pngDownLen);
    char* fontBuf    = readAllocFile("assets/fonts/MapleMono-NF-CN-Regular.ttf", &fontLen);
    char* aniBuf     = readAllocFile("assets/animations/bombBlock/bombBlock.jsonc", &aniLen);
    // 动画 JSON 内层图片引用：以 jsonc 中 src 原串（animations/bombBlock/marker.svg）作为资源 ID
    char* svgBuf     = readAllocFile("assets/animations/bombBlock/marker.svg", &svgLen);
    if (!pngBuf || !pngDownBuf || !fontBuf || !aniBuf || !svgBuf) {
        printf("FAIL: read asset to memory\n");
        return 1;
    }
    printf("OK: assets loaded to heap before init (png=%zu pngDown=%zu font=%zu ani=%zu svg=%zu)\n",
           pngLen, pngDownLen, fontLen, aniLen, svgLen);

    g_provider = callbacks->createMemoryResourceProvider();
    if (!g_provider) { printf("FAIL: createMemoryResourceProvider\n"); return 1; }
    if (!callbacks->memoryProviderRegister(g_provider, "cross-up", pngBuf, (int)pngLen))
        { printf("FAIL: register cross-up\n"); return 1; }
    if (!callbacks->memoryProviderRegister(g_provider, "maple-font", fontBuf, (int)fontLen))
        { printf("FAIL: register maple-font\n"); return 1; }
    if (!callbacks->memoryProviderRegister(g_provider, "bomb-ani", aniBuf, (int)aniLen))
        { printf("FAIL: register bomb-ani\n"); return 1; }
    if (!callbacks->memoryProviderRegister(g_provider, "animations/bombBlock/marker.svg", svgBuf, (int)svgLen))
        { printf("FAIL: register marker.svg\n"); return 1; }
    if (!callbacks->memoryProviderAdopt(g_provider, "cross-down", pngDownBuf, (int)pngDownLen, countFree))
        { printf("FAIL: adopt cross-down\n"); return 1; }
    // adopt = 零拷贝引用：调用方保持缓冲有效（引擎不复制），首次 readFile 命中后引擎
    // 包装进自家缓存；引擎销毁 provider 时经 freeFn 回调通知调用方释放原缓冲
    printf("OK: memory provider registered (4 register + 1 adopt, zero-copy)\n");

    UIInstanceConfig cfg = UI_INSTANCE_CONFIG_DEFAULT;
    cfg.windowTitle   = "test_resourceprovider_cabi";
    cfg.windowWidth   = 800;
    cfg.windowHeight  = 600;
    cfg.resourceRoot  = "_nonexistent_dir_";  // 引擎无磁盘资源可用
    g_inst = uiCreateInstance(callbacks, &cfg);
    if (!g_inst) { printf("FAIL: CreateInstance\n"); FreeLibrary(g_uiDll); return 1; }
    uiSetViewport(g_inst, 0, 0, 800, 600);
    printf("OK: initialized (resourceRoot=_nonexistent_dir_)\n");

    if (!callbacks->setResourceProvider(g_inst, g_provider))
        { printf("FAIL: setResourceProvider\n"); return 1; }
    printf("OK: memory provider mounted\n");

    // ---- 布局：JSON resourceProviders 挂载 + 各引用语法 ----
    const char* layoutJson = R"json({
    "version": "1.0",
    "resourceProviders": [
        { "name": "lazy-image", "path": "images/cross_over.png" }
    ],
    "controls": [
        {
            "type": "panel",
            "id": "root",
            "rect": { "x": 0, "y": 0, "w": 800, "h": 600 },
            "children": [
                {
                    "type": "label",
                    "id": "lbl",
                    "rect": { "x": 20, "y": 20, "w": 760, "h": 32 },
                    "caption": "Memory font label",
                    "font-size": 20,
                    "font-resource": "maple-font"
                },
                {
                    "type": "label",
                    "id": "lblFile",
                    "rect": { "x": 20, "y": 56, "w": 760, "h": 32 },
                    "caption": "Font-file label",
                    "font-size": 16,
                    "font-file": "provider:maple-font"
                },
                {
                    "type": "button",
                    "id": "imgBtn",
                    "rect": { "x": 20, "y": 70, "w": 64, "h": 64 },
                    "actors": {
                        "normal": { "providerName": "cross-up" },
                        "hover":  "provider:cross-down",
                        "pressed": { "providerName": "cross-up" }
                    }
                },
                {
                    "type": "animation",
                    "id": "ani",
                    "rect": { "x": 120, "y": 70, "w": 64, "h": 64 },
                    "path": "provider:bomb-ani"
                }
            ]
        }
    ]
})json";

    if (!uiLoadLayout(g_inst, layoutJson)) { printf("FAIL: LoadLayout\n"); uiDestroyInstance(g_inst); FreeLibrary(g_uiDll); return 1; }
    printf("OK: layout loaded (with JSON resourceProviders + provider refs)\n");

    void* lbl = uiFindControl(g_inst, "lbl");
    void* imgBtn = uiFindControl(g_inst, "imgBtn");
    void* ani = uiFindControl(g_inst, "ani");
    if (!lbl || !imgBtn || !ani) {
        printf("FAIL: find controls lbl/imgBtn/ani\n");
        uiDestroyInstance(g_inst); FreeLibrary(g_uiDll); return 1;
    }

    // ---- 工厂路径分流：CreateImage / 状态图 Button / CreateAnimation（P0-31：ImageButton 已移除）----
    void* img = uiCreateImage(g_inst, "provider:cross-up", 220, 70, 0, 0, 1.0f, 1.0f);
    void* imgBtn2 = uiCreateButton(g_inst, "", 300, 70, 64, 64, 1.0f, 1.0f);
    if (imgBtn2) {
        uiSetString(g_inst, imgBtn2, "normal-image", "provider:cross-up");
        uiSetString(g_inst, imgBtn2, "hover-image", "provider:cross-down");
        uiSetString(g_inst, imgBtn2, "pressed-image", "provider:cross-up");
    }
    void* ani2 = uiCreateAnimation(g_inst, "provider:bomb-ani", 380, 70, 64, 64, 1.0f, 1.0f);
    if (!img || !imgBtn2 || !ani2) { printf("FAIL: factory provider creation\n"); uiDestroyInstance(g_inst); FreeLibrary(g_uiDll); return 1; }
    printf("OK: factory paths resolved provider refs\n");

    // 纹理自然尺寸（cross_up.png = 32x32）：证明图片来自内存注册表
    float texX = 0.0f;
    float texY = 0.0f;
    float texW = 0.0f;
    float texH = 0.0f;
    uiGetRect(g_inst, img, &texX, &texY, &texW, &texH);
    if (texW != 32.0f || texH != 32.0f) { printf("FAIL: image tex size %.0fx%.0f (expected 32x32)\n", texW, texH); }
    else printf("OK: image texture size 32x32 from memory\n");

    if (!uiSetBool(g_inst, ani, "playing", 1)) { printf("FAIL: set animation playing\n"); }
    else printf("OK: animation (memory jsonc) playing set\n");

    if (!uiSetBool(g_inst, ani2, "playing", 1)) { printf("FAIL: set animation2 playing\n"); }
    else printf("OK: animation2 (factory provider) playing set\n");

    // ---- 负用例：未注册名 ----
    void* imgBad = uiCreateImage(g_inst, "provider:not-exists", 460, 70, 64, 64, 1.0f, 1.0f);
    if (!imgBad) { printf("FAIL: negative case should create control anyway\n"); uiDestroyInstance(g_inst); FreeLibrary(g_uiDll); return 1; }
    printf("OK: negative case (provider:not-exists) survives with empty texture\n");

    printf("Frame loop... (closed automatically)\n");
    ULONGLONG autoT0 = GetTickCount64();
    while (!uiIsQuitRequested(g_inst)) {
        if (g_autoSec > 0 && (GetTickCount64() - autoT0) >= (ULONGLONG)g_autoSec * 1000) {
            UIEvent ue; memset(&ue, 0, sizeof(ue)); ue.type = UI_EVENT_WINDOW_CLOSE;
            uiPushUIEvent(g_inst, &ue);
        }
        uiProcessEvents(g_inst);
        uiUpdate(g_inst, 1.0 / 60.0);
        uiClear(g_inst);
        uiRender(g_inst);
        uiPresent(g_inst);
    }

    uiDestroyInstance(g_inst);
    g_inst = nullptr;

    // ---- adopt 释放契约：provider 析构时 freeFn 恰被调用一次 ----
    callbacks->destroyResourceProvider(g_provider);
    g_provider = nullptr;
    if (g_freeCount != 1) { printf("FAIL: adopt freeFn count=%d (expected 1)\n", g_freeCount); FreeLibrary(g_uiDll); return 1; }
    printf("OK: adopt freeFn invoked exactly once on provider destroy\n");

    FreeLibrary(g_uiDll);
    g_uiDll = nullptr;
    printf("test_resourceprovider_cabi_%s: done\n", shortName);
    return 0;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "auto=", 5) == 0) g_autoSec = atoi(argv[i] + 5);
    }
    return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME);
}