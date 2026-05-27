#ifndef LayoutParserH
#define LayoutParserH

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <SDL3/SDL.h>
#include "nlohmann/json.hpp"
#include "ControlBase.h"
#include "Label.h"
#include "Button.h"
#include "EditBox.h"
#include "TextArea.h"
#include "CheckBox.h"
#include "ProgressBar.h"
#include "ScrollBar.h"
#include "Panel.h"

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

    void clear();
    void reset();

private:
    unordered_map<string, shared_ptr<Control>> m_controlsById;
    unordered_map<string, function<void(shared_ptr<Control>)>> m_handlers;

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
    shared_ptr<ScrollBar>   parseScrollBar(const json& j, Control* parent);
    shared_ptr<Panel>       parsePanel(const json& j, Control* parent);

    void parseCommonProperties(shared_ptr<ControlImpl> ctrl, const json& j);
    void parseEvents(shared_ptr<ControlImpl> ctrl, const json& j);
    void parseChildren(shared_ptr<Control> container, const json& j);

    SRect      parseRect(const json& j);
    Margin     parseMargin(const json& j);
    SDL_Color  parseColor(const json& j);
    StateColor parseStateColor(const json& j, StateColor::Type type);
    FontName   parseFontName(const string& name);
    AlignmentMode parseAlignment(const string& align);
    TTF_FontStyleFlags parseFontStyle(const string& style);
};

#endif
