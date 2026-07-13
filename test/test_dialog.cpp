#include <iostream>
#include <memory>
#include <string>
#include "Dialog.h"
#include "Label.h"
#include "Button.h"
#include "CheckBox.h"
#include "EditBox.h"
#include "Slider.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "PlatformUtils.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<Label> g_hint;
shared_ptr<Label> g_resultLabel;

void logResult(const string& msg) {
    cout << msg << endl;
    if (g_resultLabel)
        g_resultLabel->setCaption(msg);
}

// ==================== Demo 1: Popup (no buttons) ====================

shared_ptr<Button> g_btnPopup1;
shared_ptr<Popup> g_popup1;

void showPopup1() {
    logResult(u8"[Popup1] show()");
    g_popup1->open();
}

void createPopup1Demo() {
    g_btnPopup1 = make_shared<Button>(nullptr, SRect(20, 40, 180, 30));
    g_btnPopup1->setCaption(u8"1. Popup: 纯信息弹窗");
    g_btnPopup1->setOnClick([](shared_ptr<Button>) { showPopup1(); });
    g_btnPopup1->create();
    BENCH->addControl(g_btnPopup1);

    g_popup1 = make_shared<Popup>(nullptr, SRect(0, 0, 300, 150), 1.0f, 1.0f);
    g_popup1->setCentered();
    g_popup1->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_popup1->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_popup1->setCloseOnEsc(true);
    g_popup1->setCloseOnClickOutside(true);
    g_popup1->setOnClose([](shared_ptr<Popup>, DialogResult r) {
        logResult(u8"[Popup1] close() result=" + to_string((int)r));
    });
    auto label = make_shared<Label>(nullptr, SRect(0, 0, 1, 1));
    label->setCaption(u8"这是一个纯信息弹窗\n按 ESC 或点击外部关闭");
    label->setAlignmentMode(AlignmentMode::AM_CENTER);
    label->setTextNormalStateColor(SColor(200, 200, 200, 255));
    label->setFontSize(16);
    label->create();
    g_popup1->setContent(label);
    g_popup1->create();
}

// ==================== Demo 2: Popup anchoring ====================

shared_ptr<Button> g_btnPopup2;
shared_ptr<Popup> g_popup2;

void showPopup2() {
    logResult(u8"[Popup2] Anchored show()");
    g_popup2->open();
}

void createPopup2Demo() {
    g_btnPopup2 = make_shared<Button>(nullptr, SRect(20, 80, 180, 30));
    g_btnPopup2->setCaption(u8"2. Popup: 锚定弹出");
    g_btnPopup2->setOnClick([](shared_ptr<Button>) { showPopup2(); });
    g_btnPopup2->create();
    BENCH->addControl(g_btnPopup2);

    g_popup2 = make_shared<Popup>(nullptr, SRect(0, 0, 240, 120), 1.0f, 1.0f);
    g_popup2->setAnchored(g_btnPopup2.get(), SRect(0, 2, 0, 0));
    g_popup2->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_popup2->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_popup2->setCloseOnEsc(true);
    auto lb = make_shared<Label>(nullptr, SRect(0, 0, 1, 1));
    lb->setCaption(u8"锚定在按钮下方弹窗\n自动跟随按钮位置");
    lb->setAlignmentMode(AlignmentMode::AM_CENTER);
    lb->setTextNormalStateColor(SColor(200, 200, 200, 255));
    lb->setFontSize(14);
    lb->create();
    g_popup2->setContent(lb);
    g_popup2->create();
}

// ==================== Demo 3: ConfirmPopup ====================

shared_ptr<Button> g_btnConfirm1;
shared_ptr<ConfirmPopup> g_cp1;
shared_ptr<CheckBox> g_cb1;
int g_confirmCount = 0;

void showConfirm1() {
    logResult(u8"[ConfirmPopup1] show()");
    g_cp1->open();
}

void createConfirm1Demo() {
    g_btnConfirm1 = make_shared<Button>(nullptr, SRect(20, 120, 180, 30));
    g_btnConfirm1->setCaption(u8"3. ConfirmPopup: 确定");
    g_btnConfirm1->setOnClick([](shared_ptr<Button>) { showConfirm1(); });
    g_btnConfirm1->create();
    BENCH->addControl(g_btnConfirm1);

    g_cp1 = make_shared<ConfirmPopup>(nullptr, SRect(0, 0, 300, 170), 1.0f, 1.0f);
    g_cp1->setCentered();
    g_cp1->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_cp1->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_cp1->setPadding(12);
    g_cp1->setConfirmButtonText(u8"确认操作");

    g_cb1 = make_shared<CheckBox>(nullptr, SRect(0, 0, 1, 1));
    g_cb1->getCaption()->setCaption(u8"我已阅读并同意用户协议");
    g_cb1->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_cb1->setTextStateColor(StateColor(SColor(200,200,200,255), SColor(200,200,200,255), SColor(200,200,200,255), SColor(200,200,200,255)));
    g_cb1->create();
    g_cp1->setContent(g_cb1);

    g_cp1->setOnConfirm([](shared_ptr<ConfirmPopup>) {
        g_confirmCount++;
        string msg = u8"[ConfirmPopup1] 确认! 次数=" + to_string(g_confirmCount)
            + u8", checked=" + to_string(g_cb1->getCheckState() == CheckState::Checked ? 1 : 0);
        logResult(msg);
    });
    g_cp1->create();
}

// ==================== Demo 4: ConfirmPopup Enter key ====================

shared_ptr<Button> g_btnConfirm2;
shared_ptr<ConfirmPopup> g_cp2;

void showConfirm2() {
    logResult(u8"[ConfirmPopup2] show() - 按 Enter 触发确认");
    g_cp2->open();
}

void createConfirm2Demo() {
    g_btnConfirm2 = make_shared<Button>(nullptr, SRect(20, 160, 180, 30));
    g_btnConfirm2->setCaption(u8"4. ConfirmPopup: Enter");
    g_btnConfirm2->setOnClick([](shared_ptr<Button>) { showConfirm2(); });
    g_btnConfirm2->create();
    BENCH->addControl(g_btnConfirm2);

    g_cp2 = make_shared<ConfirmPopup>(nullptr, SRect(0, 0, 280, 150), 1.0f, 1.0f);
    g_cp2->setCentered();
    g_cp2->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_cp2->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_cp2->setConfirmButtonText(u8"确定 (Enter)");

    auto lb = make_shared<Label>(nullptr, SRect(0, 0, 1, 1));
    lb->setCaption(u8"按 Enter 键触发确定\n或点击确定按钮");
    lb->setAlignmentMode(AlignmentMode::AM_CENTER);
    lb->setTextNormalStateColor(SColor(200, 200, 200, 255));
    lb->setFontSize(14);
    lb->create();
    g_cp2->setContent(lb);

    g_cp2->setOnConfirm([](shared_ptr<ConfirmPopup>) {
        logResult(u8"[ConfirmPopup2] Enter 触发确认!");
    });
    g_cp2->create();
}

// ==================== Demo 5: ConfirmPopup custom buttons ====================

shared_ptr<Button> g_btnConfirm3;
shared_ptr<ConfirmPopup> g_cp3;

void showConfirm3() {
    logResult(u8"[ConfirmPopup3] show() - 定制按钮");
    g_cp3->open();
}

void createConfirm3Demo() {
    g_btnConfirm3 = make_shared<Button>(nullptr, SRect(20, 200, 180, 30));
    g_btnConfirm3->setCaption(u8"5. ConfirmPopup: 自定义按钮");
    g_btnConfirm3->setOnClick([](shared_ptr<Button>) { showConfirm3(); });
    g_btnConfirm3->create();
    BENCH->addControl(g_btnConfirm3);

    g_cp3 = make_shared<ConfirmPopup>(nullptr, SRect(0, 0, 320, 170), 1.0f, 1.0f);
    g_cp3->setCentered();
    g_cp3->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_cp3->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_cp3->setConfirmButtonText(u8"应用");
    g_cp3->setConfirmButtonRect(SRect(120, 115, 80, 28));
    g_cp3->setButtonHeight(28);
    g_cp3->setPadding(10);

    auto btn = g_cp3->getConfirmButton();
    if (btn) {
        btn->setNormalStateBGColor(SColor(0, 120, 200, 255));
    }

    auto lb = make_shared<Label>(nullptr, SRect(0, 0, 1, 1));
    lb->setCaption(u8"自定义蓝色确认按钮\n居中于底部");
    lb->setAlignmentMode(AlignmentMode::AM_CENTER);
    lb->setTextNormalStateColor(SColor(200, 200, 200, 255));
    lb->setFontSize(14);
    lb->create();
    g_cp3->setContent(lb);

    g_cp3->setOnConfirm([](shared_ptr<ConfirmPopup>) {
        logResult(u8"[ConfirmPopup3] 应用按钮被点击!");
    });
    g_cp3->create();
}

// ==================== Demo 6: Dialog ====================

shared_ptr<Button> g_btnDialog1;
shared_ptr<Dialog> g_dlg1;

void showDialog1() {
    logResult(u8"[Dialog1] show()");
    g_dlg1->open();
}

void createDialog1Demo() {
    g_btnDialog1 = make_shared<Button>(nullptr, SRect(20, 240, 180, 30));
    g_btnDialog1->setCaption(u8"6. Dialog: 确定+取消");
    g_btnDialog1->setOnClick([](shared_ptr<Button>) { showDialog1(); });
    g_btnDialog1->create();
    BENCH->addControl(g_btnDialog1);

    g_dlg1 = make_shared<Dialog>(nullptr, SRect(0, 0, 320, 180), 1.0f, 1.0f);
    g_dlg1->setCentered();
    g_dlg1->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_dlg1->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_dlg1->setPadding(12);
    g_dlg1->setConfirmButtonText(u8"保存");
    g_dlg1->setCancelButtonText(u8"放弃");

    auto lb = make_shared<Label>(nullptr, SRect(0, 0, 1, 1));
    lb->setCaption(u8"确定/取消示例\n点击保存保存修改，点击放弃丢弃更改");
    lb->setAlignmentMode(AlignmentMode::AM_CENTER);
    lb->setTextNormalStateColor(SColor(200, 200, 200, 255));
    lb->setFontSize(14);
    lb->create();
    g_dlg1->setContent(lb);

    g_dlg1->setOnConfirm([](shared_ptr<ConfirmPopup>) {
        logResult(u8"[Dialog1] 保存确认!");
    });
    g_dlg1->setOnCancel([](shared_ptr<Dialog>) {
        logResult(u8"[Dialog1] 放弃取消!");
    });
    g_dlg1->create();
}

// ==================== Demo 7: Dialog ESC/Enter ====================

shared_ptr<Button> g_btnDialog2;
shared_ptr<Dialog> g_dlg2;

void showDialog2() {
    logResult(u8"[Dialog2] show() - ESC=取消, Enter=确定");
    g_dlg2->open();
}

void createDialog2Demo() {
    g_btnDialog2 = make_shared<Button>(nullptr, SRect(20, 280, 180, 30));
    g_btnDialog2->setCaption(u8"7. Dialog: ESC/Enter");
    g_btnDialog2->setOnClick([](shared_ptr<Button>) { showDialog2(); });
    g_btnDialog2->create();
    BENCH->addControl(g_btnDialog2);

    g_dlg2 = make_shared<Dialog>(nullptr, SRect(0, 0, 300, 170), 1.0f, 1.0f);
    g_dlg2->setCentered();
    g_dlg2->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_dlg2->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_dlg2->setPadding(12);

    auto lb = make_shared<Label>(nullptr, SRect(0, 0, 1, 1));
    lb->setCaption(u8"ESC 关闭 → onCancel 触发\nEnter → onConfirm 触发\n点击外部 → onCancel 触发");
    lb->setAlignmentMode(AlignmentMode::AM_CENTER);
    lb->setTextNormalStateColor(SColor(200, 200, 200, 255));
    lb->setFontSize(14);
    lb->create();
    g_dlg2->setContent(lb);

    g_dlg2->setOnConfirm([](shared_ptr<ConfirmPopup>) {
        logResult(u8"[Dialog2] Enter 确定!");
    });
    g_dlg2->setOnCancel([](shared_ptr<Dialog>) {
        logResult(u8"[Dialog2] ESC/外部 取消!");
    });
    g_dlg2->create();
}

// ==================== Demo 8: Dialog with EditBox + Slider ====================

shared_ptr<Button> g_btnDialog3;
shared_ptr<Dialog> g_dlg3;
shared_ptr<EditBox> g_debugEB;
shared_ptr<Slider> g_debugSlider;
shared_ptr<Label> g_debugVal;

void showDialog3() {
    logResult(u8"[Dialog3] show() - 带 EditBox + Slider");
    g_dlg3->open();
}

void createDialog3Demo() {
    g_btnDialog3 = make_shared<Button>(nullptr, SRect(20, 320, 180, 30));
    g_btnDialog3->setCaption(u8"8. Dialog: 复杂内容");
    g_btnDialog3->setOnClick([](shared_ptr<Button>) { showDialog3(); });
    g_btnDialog3->create();
    BENCH->addControl(g_btnDialog3);

    g_dlg3 = make_shared<Dialog>(nullptr, SRect(0, 0, 340, 240), 1.0f, 1.0f);
    g_dlg3->setCentered();
    g_dlg3->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_dlg3->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_dlg3->setPadding(12);
    g_dlg3->setConfirmButtonText(u8"确定");
    g_dlg3->setCancelButtonText(u8"取消");

    g_debugEB = make_shared<EditBox>(nullptr, SRect(0, 0, 300, 30));
    g_debugEB->setText(u8"请输入文字");
    g_debugEB->setNormalStateBGColor(SColor(50, 50, 55, 255));
    g_debugEB->create();

    g_debugVal = make_shared<Label>(nullptr, SRect(0, 0, 1, 1));
    g_debugVal->setCaption(u8"滑块值: 50");
    g_debugVal->setFontSize(12);
    g_debugVal->setTextNormalStateColor(SColor(200, 200, 200, 255));
    g_debugVal->create();

    g_debugSlider = make_shared<Slider>(nullptr, SRect(0, 44, 300, 24));
    g_debugSlider->setRange(0, 100);
    g_debugSlider->setValue(50);
    g_debugSlider->setShowValueLabel(true);
    g_debugSlider->create();

    auto contentPanel = make_shared<Panel>(nullptr, SRect(0, 0, 316, 120));
    contentPanel->setTransparent(true);
    contentPanel->setBorderVisible(false);
    contentPanel->create();

    g_debugEB->setParent(contentPanel.get());
    g_debugEB->setRenderDevice(contentPanel->getRenderDevice());
    g_debugEB->setTextRenderer(contentPanel->getTextRenderer());
    contentPanel->addControl(g_debugEB);

    g_debugVal->setParent(contentPanel.get());
    g_debugVal->setRenderDevice(contentPanel->getRenderDevice());
    g_debugVal->setTextRenderer(contentPanel->getTextRenderer());
    contentPanel->addControl(g_debugVal);

    g_debugSlider->setParent(contentPanel.get());
    g_debugSlider->setRenderDevice(contentPanel->getRenderDevice());
    g_debugSlider->setTextRenderer(contentPanel->getTextRenderer());
    contentPanel->addControl(g_debugSlider);

    g_dlg3->setContent(contentPanel);

    g_dlg3->setOnConfirm([](shared_ptr<ConfirmPopup>) {
        char buf[128];
        snprintf(buf, sizeof(buf), u8"[Dialog3] 确定: 文字=\"%s\", 滑块值=%.0f",
            g_debugEB->getText().c_str(), g_debugSlider->getValue());
        logResult(buf);
    });
    g_dlg3->setOnCancel([](shared_ptr<Dialog>) {
        logResult(u8"[Dialog3] 取消");
    });
    g_dlg3->create();
}

// ==================== Demo 9: DialogResult query ====================

shared_ptr<Button> g_btnDialog4;
shared_ptr<Dialog> g_dlg4;

void showDialog4() {
    logResult(u8"[Dialog9] show() - DialogResult 查询");
    g_dlg4->open();
}

void createDialog4Demo() {
    g_btnDialog4 = make_shared<Button>(nullptr, SRect(20, 360, 180, 30));
    g_btnDialog4->setCaption(u8"9. DialogResult 查询");
    g_btnDialog4->setOnClick([](shared_ptr<Button>) { showDialog4(); });
    g_btnDialog4->create();
    BENCH->addControl(g_btnDialog4);

    g_dlg4 = make_shared<Dialog>(nullptr, SRect(0, 0, 300, 160), 1.0f, 1.0f);
    g_dlg4->setCentered();
    g_dlg4->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_dlg4->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_dlg4->setPadding(12);

    auto lb = make_shared<Label>(nullptr, SRect(0, 0, 1, 1));
    lb->setCaption(u8"关闭后通过 getResult() 查询结果\n在 onClose 回调中查看");
    lb->setAlignmentMode(AlignmentMode::AM_CENTER);
    lb->setTextNormalStateColor(SColor(200, 200, 200, 255));
    lb->setFontSize(14);
    lb->create();
    g_dlg4->setContent(lb);

    g_dlg4->setOnConfirm([](shared_ptr<ConfirmPopup>) {
        logResult(u8"[Dialog9] 确认按钮已点击");
    });
    g_dlg4->setOnCancel([](shared_ptr<Dialog>) {
        logResult(u8"[Dialog9] 取消按钮已点击");
    });
    g_dlg4->setOnClose([](shared_ptr<Popup> sender, DialogResult r) {
        const char* name = (r == DialogResult::Confirmed) ? "Confirmed" :
                           (r == DialogResult::Cancelled) ? "Cancelled" : "None";
        char buf[64];
        snprintf(buf, sizeof(buf), u8"[Dialog9] DialogResult=%s", name);
        logResult(buf);
    });
    g_dlg4->create();
}

// ==================== Demo 10: FocusBoundary ====================

shared_ptr<Button> g_btnDialog5;
shared_ptr<Dialog> g_dlg5;
shared_ptr<EditBox> g_focusEB1;
shared_ptr<EditBox> g_focusEB2;

void showDialog5() {
    logResult(u8"[Dialog10] show() - Tab 在弹窗内部循环");
    g_dlg5->open();
}

void createDialog5Demo() {
    g_btnDialog5 = make_shared<Button>(nullptr, SRect(20, 400, 180, 30));
    g_btnDialog5->setCaption(u8"10. Tab 焦点边界");
    g_btnDialog5->setOnClick([](shared_ptr<Button>) { showDialog5(); });
    g_btnDialog5->create();
    BENCH->addControl(g_btnDialog5);

    g_dlg5 = make_shared<Dialog>(nullptr, SRect(0, 0, 320, 240), 1.0f, 1.0f);
    g_dlg5->setCentered();
    g_dlg5->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_dlg5->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_dlg5->setPadding(12);
    g_dlg5->setConfirmButtonText(u8"确定");
    g_dlg5->setCancelButtonText(u8"取消");

    g_focusEB1 = make_shared<EditBox>(nullptr, SRect(0, 0, 280, 28));
    g_focusEB1->setText(u8"焦点可在此循环");
    g_focusEB1->setNormalStateBGColor(SColor(50, 50, 55, 255));
    g_focusEB1->create();

    g_focusEB2 = make_shared<EditBox>(nullptr, SRect(0, 38, 280, 28));
    g_focusEB2->setText(u8"Tab 不会超出弹窗边界");
    g_focusEB2->setNormalStateBGColor(SColor(50, 50, 55, 255));
    g_focusEB2->create();

    auto infoLb = make_shared<Label>(nullptr, SRect(0, 76, 280, 40));
    infoLb->setCaption(u8"Tab 在输入框和按钮间循环\n不会跑出弹窗边界");
    infoLb->setFontSize(12);
    infoLb->setTextNormalStateColor(SColor(180, 180, 180, 255));
    infoLb->create();

    auto contentP = make_shared<Panel>(nullptr, SRect(0, 0, 296, 140));
    contentP->setTransparent(true);
    contentP->setBorderVisible(false);
    contentP->create();

    g_focusEB1->setParent(contentP.get());
    g_focusEB1->setRenderDevice(contentP->getRenderDevice());
    g_focusEB1->setTextRenderer(contentP->getTextRenderer());
    contentP->addControl(g_focusEB1);

    g_focusEB2->setParent(contentP.get());
    g_focusEB2->setRenderDevice(contentP->getRenderDevice());
    g_focusEB2->setTextRenderer(contentP->getTextRenderer());
    contentP->addControl(g_focusEB2);

    infoLb->setParent(contentP.get());
    infoLb->setRenderDevice(contentP->getRenderDevice());
    infoLb->setTextRenderer(contentP->getTextRenderer());
    contentP->addControl(infoLb);

    g_dlg5->setContent(contentP);

    g_dlg5->setOnConfirm([](shared_ptr<ConfirmPopup>) {
        logResult(u8"[Dialog10] 确定");
    });
    g_dlg5->setOnCancel([](shared_ptr<Dialog>) {
        logResult(u8"[Dialog10] 取消");
    });
    g_dlg5->create();
}

// ==================== Demo 11: 2x scaling ====================

shared_ptr<Button> g_btnDialog6;
shared_ptr<Dialog> g_dlg6;

void showDialog6() {
    logResult(u8"[Dialog11] show() - 2x 缩放弹窗");
    g_dlg6->open();
}

void createDialog6Demo() {
    g_btnDialog6 = make_shared<Button>(nullptr, SRect(20, 440, 180, 30));
    g_btnDialog6->setCaption(u8"11. Dialog: 2x 缩放");
    g_btnDialog6->setOnClick([](shared_ptr<Button>) { showDialog6(); });
    g_btnDialog6->create();
    BENCH->addControl(g_btnDialog6);

    g_dlg6 = make_shared<Dialog>(nullptr, SRect(0, 0, 220, 150), 2.0f, 2.0f);
    g_dlg6->setCentered();
    g_dlg6->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_dlg6->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_dlg6->setConfirmButtonText(u8"确定");
    g_dlg6->setCancelButtonText(u8"取消");

    auto lb = make_shared<Label>(nullptr, SRect(0, 0, 1, 1));
    lb->setCaption(u8"2x 缩放弹窗\n按钮和文字都放大");
    lb->setAlignmentMode(AlignmentMode::AM_CENTER);
    lb->setTextNormalStateColor(SColor(200, 200, 200, 255));
    lb->setFontSize(14);
    lb->create();
    g_dlg6->setContent(lb);

    g_dlg6->setOnConfirm([](shared_ptr<ConfirmPopup>) {
        logResult(u8"[Dialog11] 2x 确定");
    });
    g_dlg6->setOnCancel([](shared_ptr<Dialog>) {
        logResult(u8"[Dialog11] 2x 取消");
    });
    g_dlg6->create();
}

// ==================== Demo 12: Popup absolute positioning ====================

shared_ptr<Button> g_btnPopup3;
shared_ptr<Popup> g_popup3;

void showPopup3() {
    logResult(u8"[Popup3] show() - 绝对定位 (100, 100)");
    g_popup3->open();
}

void createPopup3Demo() {
    g_btnPopup3 = make_shared<Button>(nullptr, SRect(230, 40, 180, 30));
    g_btnPopup3->setCaption(u8"12. Popup: 绝对定位");
    g_btnPopup3->setOnClick([](shared_ptr<Button>) { showPopup3(); });
    g_btnPopup3->create();
    BENCH->addControl(g_btnPopup3);

    g_popup3 = make_shared<Popup>(nullptr, SRect(100, 100, 240, 130), 1.0f, 1.0f);
    g_popup3->setAbsolute(SRect(100, 100, 240, 130));
    g_popup3->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_popup3->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));

    auto lb = make_shared<Label>(nullptr, SRect(0, 0, 1, 1));
    lb->setCaption(u8"绝对定位弹窗\n固定在 (100, 100)");
    lb->setAlignmentMode(AlignmentMode::AM_CENTER);
    lb->setTextNormalStateColor(SColor(200, 200, 200, 255));
    lb->setFontSize(14);
    lb->create();
    g_popup3->setContent(lb);
    g_popup3->create();
}

// ==================== Demo 13: ConfirmPopup button hidden ====================

shared_ptr<Button> g_btnConfirm4;
shared_ptr<ConfirmPopup> g_cp4;

void showConfirm4() {
    logResult(u8"[ConfirmPopup4] show() - 隐藏确定按钮, 仅 ESC 可关闭");
    g_cp4->open();
}

void createConfirm4Demo() {
    g_btnConfirm4 = make_shared<Button>(nullptr, SRect(230, 80, 180, 30));
    g_btnConfirm4->setCaption(u8"13. ConfirmPopup: 隐藏按钮");
    g_btnConfirm4->setOnClick([](shared_ptr<Button>) { showConfirm4(); });
    g_btnConfirm4->create();
    BENCH->addControl(g_btnConfirm4);

    g_cp4 = make_shared<ConfirmPopup>(nullptr, SRect(0, 0, 280, 130), 1.0f, 1.0f);
    g_cp4->setCentered();
    g_cp4->setNormalStateBGColor(SColor(60, 60, 65, 255));
    g_cp4->setBorderStateColor(StateColor(SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255), SColor(100,100,110,255)));
    g_cp4->setConfirmButtonVisible(false);

    auto lb = make_shared<Label>(nullptr, SRect(0, 0, 1, 1));
    lb->setCaption(u8"按钮被隐藏\n只能按 ESC 或点击外部关闭");
    lb->setAlignmentMode(AlignmentMode::AM_CENTER);
    lb->setTextNormalStateColor(SColor(200, 200, 200, 255));
    lb->setFontSize(14);
    lb->create();
    g_cp4->setContent(lb);

    g_cp4->setOnClose([](shared_ptr<Popup> p, DialogResult r) {
        logResult(u8"[ConfirmPopup4] 关闭, result=" + to_string((int)r));
    });
    g_cp4->create();
}

// ==================== Init ====================

void testDialogInitialize() {
    TestUtil::log("testDialogInitialize");

    createPopup1Demo();
    createPopup2Demo();
    createPopup3Demo();
    createConfirm1Demo();
    createConfirm2Demo();
    createConfirm3Demo();
    createConfirm4Demo();
    createDialog1Demo();
    createDialog2Demo();
    createDialog3Demo();
    createDialog4Demo();
    createDialog5Demo();
    createDialog6Demo();

    g_resultLabel = LabelBuilder(nullptr, SRect(20, 490, 700, 20))
        .setCaption(u8"点击按钮显示弹窗测试; 结果在此显示")
        .setFontSize(12)
        .build();
    BENCH->addControl(g_resultLabel);

    TestUtil::log("testDialogInitialize done - 13 demo groups");
}

// ==================== App ====================

class DialogTestApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_dialog");
        BENCH->setOnInitial(testDialogInitialize);
        return true;
    }

    void onUpdate() override {
        BENCH->eventLoopEntry();
        BENCH->update();
    }

    void onRender() override {
        GET_RENDERDEVICE->setDrawColor(SColor(40.0f/255.0f, 40.0f/255.0f, 40.0f/255.0f, 1.0f));
        GET_RENDERDEVICE->clear();
        BENCH->draw();
    }

    void onQuit() override {
        TestUtil::log("Dialog test quit");
    }
};

int main(int argc, char* argv[]) {
    DialogTestApp app;
    return MAINWIN->run(&app);
}
