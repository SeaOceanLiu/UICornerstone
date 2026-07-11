// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#ifndef LayoutParserH
#define LayoutParserH

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include "SColor.h"
#include "nlohmann/json.hpp"
#include "ControlBase.h"
#include "Label.h"
#include "Button.h"
#include "EditBox.h"
#include "TextArea.h"
#include "CheckBox.h"
#include "ProgressBar.h"
#include "Slider.h"
#include "ScrollBar.h"
#include "ColorPicker.h"
#include "Panel.h"
#include "WinFrame.h"
#include "Menu.h"
#include "Theme.h"
#include "DataContext.h"

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;

class LayoutParser {
public:
    LayoutParser();
    ~LayoutParser() = default;

    shared_ptr<Control> parseLayout(const string& jsonContent);
    shared_ptr<Control> parseLayoutFile(const fs::path& jsonPath);

    shared_ptr<Control> findControlById(const string& id);
    vector<string> getAllControlIds() const;

    void registerHandler(const string& name,
                         function<void(shared_ptr<Control>)> handler);
    void unregisterHandler(const string& name);
    void clearHandlers();

    const vector<shared_ptr<MenuBar>>& getMenuBars() const;

    void clear();

    void reset();

    struct ComponentSource {
        string name;
        int lineStart;
        int lineEnd;
    };

private:
    Theme m_theme;

    unordered_map<string, shared_ptr<Control>> m_controlsById;
    unordered_map<string, function<void(shared_ptr<Control>)>> m_handlers;
    vector<shared_ptr<MenuBar>> m_menuBars;

    // Component system
    unordered_map<string, json> m_components;
    unordered_map<string, ComponentSource> m_componentSourceLines;
    vector<string> m_instantiationStack;

    int m_currentLineNo;
    string m_currentJsonPath;
    string m_rawJsonContent;

    void logError(const string& message) const;
    void logWarn(const string& message) const;
    void pushJsonPath(const string& segment);
    void popJsonPath();
    int byteOffsetToLineNo(const string& content, size_t byteOffset) const;

    shared_ptr<Control> parseControl(const json& j, Control* parent, int index);

    shared_ptr<Label>       parseLabel(const json& j, Control* parent);
    shared_ptr<Button>      parseButton(const json& j, Control* parent);
    shared_ptr<EditBox>     parseEditBox(const json& j, Control* parent);
    shared_ptr<TextArea>    parseTextArea(const json& j, Control* parent);
    shared_ptr<CheckBox>    parseCheckBox(const json& j, Control* parent);
    shared_ptr<ProgressBar> parseProgressBar(const json& j, Control* parent);
    shared_ptr<Slider>      parseSlider(const json& j, Control* parent);
    shared_ptr<ScrollBar>   parseScrollBar(const json& j, Control* parent);
    shared_ptr<Panel>       parsePanel(const json& j, Control* parent);
    shared_ptr<ColorPicker> parseColorPicker(const json& j, Control* parent);
    shared_ptr<WinFrame>    parseWinFrame(const json& j, Control* parent);
    shared_ptr<MenuBar>     parseMenuBar(const json& j, Control* parent);
    void populateMenuPanel(shared_ptr<MenuPanel> panel, const json& items, float xScale, float yScale);

    void parseCommonProperties(shared_ptr<ControlImpl> ctrl, const json& j);
    void parseEvents(shared_ptr<ControlImpl> ctrl, const json& j);
    void parseBindings(shared_ptr<ControlImpl> ctrl, const json& j);
    void clearBindings();
    void parseChildren(shared_ptr<Control> container, const json& j);

    // Component system
    void parseComponents(const json& j);
    shared_ptr<Control> instantiateComponent(const string& name, const json& instanceJ, Control* parent, int index);
    void replacePlaceholders(json& node, const json& props, const json& instanceJ);
    void remapEvents(json& node, const json& instanceEvents);
    void prefixIds(json& node, const string& prefix);

    SRect      parseRect(const json& j);
    Margin     parseMargin(const json& j);
    SColor  parseColor(const json& j);
    StateColor parseStateColor(const json& j, StateColor::Type type);
    FontName   parseFontName(const string& name);
    AlignmentMode parseAlignment(const string& align);
    int parseFontStyle(const string& style);
    GridSize parseGridSize(const json& j);
};

#endif
