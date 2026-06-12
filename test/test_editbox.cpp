#include <iostream>
#include <memory>
#include "EditBox.h"
#include "TextArea.h"
#include "ScrollBar.h"
#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "Button.h"
#include "GraphTool.h"
#include "TestUtils.h"

using namespace std;

shared_ptr<EditBox> g_editBox1;
shared_ptr<EditBox> g_editBox2;
shared_ptr<EditBox> g_editBox3;
shared_ptr<EditBox> g_editBox4;
shared_ptr<TextArea> g_textArea;
shared_ptr<TextArea> g_textAreaWrap;
shared_ptr<TextArea> g_textAreaBuilder2x;
shared_ptr<Button> g_clearButton;
shared_ptr<Button> g_addTextButton;

void testBenchInitialize(void) {
    TestUtil::log("testEditBoxInitialize");

    g_editBox1 = make_shared<EditBox>(nullptr, SRect(50, 50, 300, 40));
    g_editBox1->setPlaceholder("Single line input...");
    g_editBox1->setOnTextChanged([](shared_ptr<Control>, string text) {
        cout << "EditBox1 text changed: " << text << endl;
    });
    g_editBox1->setOnEnter([](shared_ptr<Control>) {
        cout << "EditBox1 Enter pressed" << endl;
    });
    BENCH->addControl(g_editBox1);
    TestUtil::log("EditBox1 created with placeholder and event handlers");

    g_editBox2 = make_shared<EditBox>(nullptr, SRect(50, 100, 300, 40));
    g_editBox2->setPlaceholder("Password input...");
    g_editBox2->setPasswordMode(true);
    g_editBox2->setOnTextChanged([](shared_ptr<Control>, string text) {
        cout << "EditBox2 (password) text changed, length: " << text.length() << endl;
    });
    BENCH->addControl(g_editBox2);
    TestUtil::log("EditBox2 created in password mode with placeholder and event handler");

    g_editBox3 = make_shared<EditBox>(nullptr, SRect(50, 150, 300, 40));
    g_editBox3->setText("Disabled editbox");
    g_editBox3->setEnable(false);
    BENCH->addControl(g_editBox3);
    TestUtil::log("EditBox3 created as disabled editbox");


    g_editBox4 = EditBoxBuilder(nullptr, SRect(50, 430, 300, 80), 2.0f, 2.0f)
        .setPlaceholder("2x scale editbox (builder)...")
        .setAlignmentMode(AlignmentMode::AM_CENTER)
        .setOnTextChanged([](shared_ptr<Control>, string text) {
            cout << "EditBox4 (2x scale) text changed: " << text << endl;
        })
        .build();
    BENCH->addControl(g_editBox4);
    TestUtil::log("EditBox4 created with builder pattern and 2x scale");

    g_textArea = make_shared<TextArea>(nullptr, SRect(50, 210, 400, 200));
    g_textArea->setPlaceholder("Multi-line text area...\nTry typing here!");
    g_textArea->setOnTextChanged([](shared_ptr<Control>, string text) {
        cout << "TextArea text changed, length: " << text.length() << endl;
    });
    BENCH->addControl(g_textArea);
    TestUtil::log("TextArea created with placeholder and event handler");

    g_textAreaWrap = make_shared<TextArea>(nullptr, SRect(500, 50, 250, 300));
    g_textAreaWrap->setWordWrap(true);
    g_textAreaWrap->setLineHeight(22);
    g_textAreaWrap->setText(
        "This is a long text that should wrap to multiple lines automatically. "
        "Line wrapping allows text to flow within the boundaries of the text area.");
    g_textAreaWrap->setPlaceholder("Line wrapping TextArea. Type long text here!");
    g_textAreaWrap->setOnTextChanged([](shared_ptr<Control>, string text) {
        cout << "TextAreaWrap text changed, length: " << text.length() << endl;
    });
    BENCH->addControl(g_textAreaWrap);
    TestUtil::log("TextAreaWrap created with word wrap enabled");

    g_textAreaBuilder2x = TextAreaBuilder(nullptr, SRect(750, 360, 200, 100), 2.0f, 2.0f)
        .setText("This is a 2x scaled TextArea created with Builder pattern. Try typing long text here to test horizontal scrolling!")
        .setPlaceholder("2x scaled TextArea (Builder)...")
        .setOnTextChanged([](shared_ptr<Control>, string text) {
            cout << "TextAreaBuilder2x text changed, length: " << text.length() << endl;
        })
        .build();
    BENCH->addControl(g_textAreaBuilder2x);
    TestUtil::log("TextAreaBuilder2x created with builder pattern and 2x scale");

    g_clearButton = make_shared<Button>(nullptr, SRect(50, 420, 100, 40));
    TestUtil::log("Creating clear button...");
    g_clearButton->setCaption("Clear All");
    TestUtil::log("Setting clear button caption...");
    g_clearButton->setOnClick([](shared_ptr<Button> btn) {
        if (g_textArea) {
            g_textArea->setText("");
        }
        cout << "Clear button clicked" << endl;
    });
    TestUtil::log("Setting clear button click handler...");
    BENCH->addControl(g_clearButton);
    TestUtil::log("Clear button created");

    g_addTextButton = make_shared<Button>(nullptr, SRect(160, 420, 100, 40));
    g_addTextButton->setCaption("Add Text");
    g_addTextButton->setOnClick([](shared_ptr<Button> btn) {
        if (g_textArea) {
            string current = g_textArea->getText();
            g_textArea->setText(current + "\nNew line added at " + to_string(TestUtil::getTicks()));
        }
        cout << "Add text button clicked" << endl;
    });
    BENCH->addControl(g_addTextButton);
    TestUtil::log("Add text button created");

    TestUtil::log("EditBox test controls created");
}

class EditBoxApp : public AppCallbacks {
public:
    bool onInit() override {
        MAINWIN->setTitle("test_editbox");
        cout << "test_editbox test" << endl;

        SSize displaySize = MAINWIN->getDisplaySize();
        GET_INPUTBACKEND->startTextInput();
        BENCH->setOnInitial(testBenchInitialize);
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
        TestUtil::log("Application quit");
    }
};

int main(int argc, char* argv[]) {
    EditBoxApp app;
    return MAINWIN->run(&app);
}
