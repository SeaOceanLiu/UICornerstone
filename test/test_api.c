#include "UICornerstoneAPI.h"
#include <stdio.h>
#include <string.h>

static void onBtnClick(UIControlHandle ctl, void* user) {
    (void)ctl; (void)user;
    static int count = 0;
    printf("  Button clicked! (%d)\n", ++count); fflush(stdout);
}

static const char* LAYOUT_JSON =
"{"
"  \"controls\": ["
"    {"
"      \"type\": \"Panel\","
"      \"id\": \"root\","
"      \"rect\": {\"x\": 0, \"y\": 0, \"w\": 800, \"h\": 480},"
"      \"children\": ["
"        {"
"          \"type\": \"Label\","
"          \"id\": \"title\","
"          \"rect\": {\"x\": 20, \"y\": 10, \"w\": 760, \"h\": 30},"
"          \"font\": {\"size\": 18},"
"          \"colors\": { \"text\": { \"normal\": \"#FFFFFFFF\" } },"
"          \"caption\": \"UICornerstone C ABI Controls Demo\""
"        },"
"        {\"type\":\"Label\",\"id\":\"hint_button\",\"rect\":{\"x\":20,\"y\":42,\"w\":240,\"h\":16},\"colors\":{\"text\":{\"normal\":\"#AAAAAAFF\"}},\"caption\":\"Button (click)\"},"
"        {\"type\":\"Label\",\"id\":\"hint_label\",\"rect\":{\"x\":280,\"y\":42,\"w\":240,\"h\":16},\"colors\":{\"text\":{\"normal\":\"#AAAAAAFF\"}},\"caption\":\"Label (multiline)\"},"
"        {\"type\":\"Label\",\"id\":\"hint_checkbox\",\"rect\":{\"x\":540,\"y\":42,\"w\":240,\"h\":16},\"colors\":{\"text\":{\"normal\":\"#AAAAAAFF\"}},\"caption\":\"CheckBox (toggle)\"},"
"        {\"type\":\"Button\",\"id\":\"btn_demo\",\"caption\":\"Button\",\"rect\":{\"x\":20,\"y\":66,\"w\":240,\"h\":95},\"events\":{\"onClick\":\"onBtnClick\"}},"
"        {\"type\":\"Label\",\"id\":\"lbl_demo\",\"caption\":\"Label\\nLine 2\\nLine 3\",\"rect\":{\"x\":280,\"y\":66,\"w\":240,\"h\":95},\"colors\":{\"background\":{\"normal\":\"#3A3A3AFF\"},\"text\":{\"normal\":\"#CCCCCCFF\"}}},"
"        {\"type\":\"CheckBox\",\"id\":\"cb_demo\",\"caption\":\"Check Option\",\"rect\":{\"x\":540,\"y\":66,\"w\":240,\"h\":95},\"colors\":{\"text\":{\"normal\":\"#FFFFFFFF\"}}},"
"        {\"type\":\"Label\",\"id\":\"hint_editbox\",\"rect\":{\"x\":20,\"y\":169,\"w\":240,\"h\":16},\"colors\":{\"text\":{\"normal\":\"#AAAAAAFF\"}},\"caption\":\"EditBox (type here)\"},"
"        {\"type\":\"Label\",\"id\":\"hint_progress\",\"rect\":{\"x\":280,\"y\":169,\"w\":240,\"h\":16},\"colors\":{\"text\":{\"normal\":\"#AAAAAAFF\"}},\"caption\":\"ProgressBar\"},"
"        {\"type\":\"Label\",\"id\":\"hint_panel\",\"rect\":{\"x\":540,\"y\":169,\"w\":240,\"h\":16},\"colors\":{\"text\":{\"normal\":\"#AAAAAAFF\"}},\"caption\":\"Panel (sub-controls)\"},"
"        {\"type\":\"EditBox\",\"id\":\"eb_demo\",\"rect\":{\"x\":20,\"y\":193,\"w\":240,\"h\":95},\"colors\":{\"background\":{\"normal\":\"#2A2A2AFF\"},\"text\":{\"normal\":\"#FFFFFFFF\"}},\"text\":\"EditBox sample\"},"
"        {\"type\":\"ProgressBar\",\"id\":\"pb_demo\",\"rect\":{\"x\":280,\"y\":193,\"w\":240,\"h\":95}},"
"        {\"type\":\"Panel\",\"id\":\"panel_demo\",\"rect\":{\"x\":540,\"y\":193,\"w\":240,\"h\":95},\"colors\":{\"background\":{\"normal\":\"#3A3A5AFF\"}},\"children\":["
"          {\"type\":\"Label\",\"id\":\"panel_label\",\"rect\":{\"x\":10,\"y\":10,\"w\":220,\"h\":30},\"colors\":{\"text\":{\"normal\":\"#FFFFFFFF\"}},\"caption\":\"Panel Label\"},"
"          {\"type\":\"Button\",\"id\":\"panel_btn\",\"caption\":\"Panel Btn\",\"rect\":{\"x\":10,\"y\":50,\"w\":220,\"h\":35},\"events\":{\"onClick\":\"onBtnClick\"}}"
"        ]},"
"        {\"type\":\"Label\",\"id\":\"hint_textarea\",\"rect\":{\"x\":20,\"y\":296,\"w\":760,\"h\":16},\"colors\":{\"text\":{\"normal\":\"#AAAAAAFF\"}},\"caption\":\"TextArea (scrollable)\"},"
"        {\"type\":\"TextArea\",\"id\":\"ta_demo\",\"rect\":{\"x\":20,\"y\":320,\"w\":760,\"h\":155},\"colors\":{\"background\":{\"normal\":\"#2A2A2AFF\"},\"text\":{\"normal\":\"#CCCCCCFF\"}},"
"          \"text\":\"This is a multi-line TextArea.\\n\\nIt supports:\\n- Arrow keys to navigate\\n- Shift+Arrow to select\\n- Ctrl+C/X/V\\n- Ctrl+A select all\\n- Word wrap\","
"          \"wordWrap\":true,\"scrollBarThickness\":10}"
"      ]"
"    }"
"  ]"
"}";

int main(void) {
    printf("=== UICornerstone C ABI Controls Demo ===\n"); fflush(stdout);

    if (!UICornerstone_InitFromPlugin(UICORNERSTONE_BACKEND_NAME)) {
        printf("FAIL: InitFromPlugin (%s)\n", UICORNERSTONE_BACKEND_NAME); return 1;
    }
    UICornerstone_SetViewport(0, 0, 800, 480);
    UICornerstone_RegisterAction("onBtnClick", onBtnClick, NULL);

    printf("Loading layout...\n"); fflush(stdout);
    if (!UICornerstone_LoadLayout(LAYOUT_JSON)) {
        printf("FAIL: LoadLayout\n");
        UICornerstone_Shutdown(); return 1;
    }
    printf("Layout loaded\n"); fflush(stdout);

    /* Verify all IDs */
    {int nF = 0;
    const char* ids[] = {"root","title",
        "hint_button","hint_label","hint_checkbox",
        "btn_demo","lbl_demo","cb_demo",
        "hint_editbox","hint_progress","hint_panel",
        "eb_demo","pb_demo","panel_demo",
        "panel_label","panel_btn",
        "hint_textarea","ta_demo"};
    for (int i = 0; i < (int)(sizeof(ids)/sizeof(ids[0])); i++) {
        if (UICornerstone_FindControl(ids[i])) nF++;
        else printf("  Missing: %s\n", ids[i]);
    }
    printf("  FindControl: %d/%zu found\n", nF, sizeof(ids)/sizeof(ids[0])); fflush(stdout);}

    printf("  Frame loop running (close window to exit)...\n"); fflush(stdout);
    while (!UICornerstone_IsQuitRequested()) {
        UICornerstone_ProcessEvents();
        UICornerstone_Update(1.0 / 60.0);
        UICornerstone_Clear();
        UICornerstone_Render();
        UICornerstone_Present();
    }

    printf("  Window closed by user\n");
    UICornerstone_Shutdown();
    printf("  === PASS ===\n"); fflush(stdout);
    return 0;
}
