#include "LayoutParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>

LayoutParser::LayoutParser()
    : m_currentLineNo(0)
{
}

// ==================== 布局加载 ====================

shared_ptr<Control> LayoutParser::parseLayout(const string& jsonContent) {
    m_rawJsonContent = jsonContent;
    m_currentLineNo = 1;
    m_currentJsonPath = "root";

    json j;
    try {
        j = json::parse(jsonContent);
    } catch (const json::parse_error& e) {
        int lineNo = byteOffsetToLineNo(jsonContent, e.byte);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "[LayoutParser] [Line %d] [root] ERROR: JSON parse error: %s",
            lineNo, e.what());
        return nullptr;
    }

    if (!j.contains("controls") || !j["controls"].is_array()) {
        logError("'controls' array is required");
        return nullptr;
    }

    const json& controls = j["controls"];
    if (controls.empty()) {
        logWarn("'controls' array is empty");
        return nullptr;
    }

    shared_ptr<Control> root = nullptr;
    for (size_t i = 0; i < controls.size(); ++i) {
        auto ctrl = parseControl(controls[i], nullptr, (int)i);
        if (ctrl) {
            root = ctrl;
        }
    }

    return root;
}

shared_ptr<Control> LayoutParser::parseLayoutFile(const fs::path& jsonPath) {
    if (!fs::exists(jsonPath)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "[LayoutParser] ERROR: Layout file not found: %s",
            jsonPath.string().c_str());
        return nullptr;
    }

    ifstream file(jsonPath);
    if (!file.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "[LayoutParser] ERROR: Failed to open layout file: %s",
            jsonPath.string().c_str());
        return nullptr;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string content = buffer.str();
    file.close();

    return parseLayout(content);
}

// ==================== ID 查找 ====================

shared_ptr<Control> LayoutParser::findControlById(const string& id) {
    auto it = m_controlsById.find(id);
    if (it != m_controlsById.end()) {
        return it->second;
    }
    return nullptr;
}

vector<string> LayoutParser::getAllControlIds() const {
    vector<string> ids;
    ids.reserve(m_controlsById.size());
    for (const auto& pair : m_controlsById) {
        ids.push_back(pair.first);
    }
    return ids;
}

// ==================== 处理器注册 ====================

void LayoutParser::registerHandler(const string& name,
                                    function<void(shared_ptr<Control>)> handler) {
    m_handlers[name] = move(handler);
}

void LayoutParser::unregisterHandler(const string& name) {
    m_handlers.erase(name);
}

void LayoutParser::clearHandlers() {
    m_handlers.clear();
}

// ==================== 状态管理 ====================

void LayoutParser::clear() {
    m_controlsById.clear();
    m_currentJsonPath.clear();
    m_currentLineNo = 0;
    m_rawJsonContent.clear();
}

void LayoutParser::reset() {
    clear();
    m_handlers.clear();
}

// ==================== 错误追踪 ====================

void LayoutParser::logError(const string& message) const {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "[LayoutParser] [Line %d] [%s] ERROR: %s",
        m_currentLineNo, m_currentJsonPath.c_str(), message.c_str());
}

void LayoutParser::logWarn(const string& message) const {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
        "[LayoutParser] [Line %d] [%s] WARN: %s",
        m_currentLineNo, m_currentJsonPath.c_str(), message.c_str());
}

void LayoutParser::pushJsonPath(const string& segment) {
    if (m_currentJsonPath.empty() || m_currentJsonPath == "root") {
        m_currentJsonPath = segment;
    } else {
        m_currentJsonPath += "." + segment;
    }
}

void LayoutParser::popJsonPath() {
    auto pos = m_currentJsonPath.rfind('.');
    if (pos != string::npos) {
        m_currentJsonPath = m_currentJsonPath.substr(0, pos);
    } else {
        m_currentJsonPath.clear();
    }
}

int LayoutParser::byteOffsetToLineNo(const string& content, size_t byteOffset) const {
    int line = 1;
    size_t limit = min(byteOffset, content.size());
    for (size_t i = 0; i < limit; ++i) {
        if (content[i] == '\n') {
            line++;
        }
    }
    return line;
}

// ==================== 控件工厂 ====================

shared_ptr<Control> LayoutParser::parseControl(const json& j, Control* parent, int index) {
    string indexPath = "controls[" + to_string(index) + "]";
    pushJsonPath(indexPath);

    if (!j.contains("type") || !j["type"].is_string()) {
        logError("'type' field is required");
        popJsonPath();
        return nullptr;
    }

    string type = j["type"].get<string>();

    shared_ptr<Control> result = nullptr;
    if (type == "Label") {
        result = parseLabel(j, parent);
    } else if (type == "Button") {
        result = parseButton(j, parent);
    } else if (type == "EditBox") {
        result = parseEditBox(j, parent);
    } else if (type == "TextArea") {
        result = parseTextArea(j, parent);
    } else if (type == "CheckBox") {
        result = parseCheckBox(j, parent);
    } else if (type == "ProgressBar") {
        result = parseProgressBar(j, parent);
    } else if (type == "ScrollBar") {
        result = parseScrollBar(j, parent);
    } else if (type == "Panel") {
        result = parsePanel(j, parent);
    } else {
        logWarn("unknown control type \"" + type + "\", skipping");
        popJsonPath();
        return nullptr;
    }

    popJsonPath();
    return result;
}

// ==================== Label ====================

shared_ptr<Label> LayoutParser::parseLabel(const json& j, Control* parent) {
    pushJsonPath("rect");
    SRect rect = parseRect(j["rect"]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains("scale") && j["scale"].is_object()) {
        xScale = j["scale"].value("x", 1.0f);
        yScale = j["scale"].value("y", 1.0f);
    }

    auto label = make_shared<Label>(parent, rect, xScale, yScale);

    parseCommonProperties(label, j);

    // caption
    if (j.contains("caption") && j["caption"].is_string()) {
        label->setCaption(j["caption"].get<string>());
    }

    // alignment
    if (j.contains("alignment") && j["alignment"].is_string()) {
        label->setAlignmentMode(parseAlignment(j["alignment"].get<string>()));
    }

    // font
    if (j.contains("font") && j["font"].is_object()) {
        pushJsonPath("font");
        const json& font = j["font"];
        if (font.contains("name") && font["name"].is_string()) {
            label->setFont(parseFontName(font["name"].get<string>()));
        }
        if (font.contains("size") && font["size"].is_number()) {
            label->setFontSize(font["size"].get<int>());
        }
        if (font.contains("style") && font["style"].is_string()) {
            label->SetFontStyle(parseFontStyle(font["style"].get<string>()));
        }
        popJsonPath();
    }

    // shadow
    if (j.contains("shadow") && j["shadow"].is_object()) {
        pushJsonPath("shadow");
        const json& shadow = j["shadow"];
        label->setShadow(shadow.value("enabled", false));
        if (shadow.contains("offset") && shadow["offset"].is_object()) {
            float ox = shadow["offset"].value("x", 1.0f);
            float oy = shadow["offset"].value("y", 1.0f);
            label->setShadowOffset(SPoint(ox, oy));
        }
        popJsonPath();
    }

    // lineHeight
    if (j.contains("lineHeight") && j["lineHeight"].is_number()) {
        label->setLineHeight(j["lineHeight"].get<int>());
    }

    // lineSpacingRatio
    if (j.contains("lineSpacingRatio") && j["lineSpacingRatio"].is_number()) {
        label->setLineSpacingRatio(j["lineSpacingRatio"].get<float>());
    }

    // enableExpand
    if (j.contains("enableExpand") && j["enableExpand"].is_boolean()) {
        label->setEnableExpand(j["enableExpand"].get<bool>());
    }

    // debugDraw
    if (j.contains("debugDraw") && j["debugDraw"].is_boolean()) {
        label->setDebugDraw(j["debugDraw"].get<bool>());
    }

    // events
    parseEvents(label, j);

    // id
    if (j.contains("id") && j["id"].is_string()) {
        m_controlsById[j["id"].get<string>()] = label;
    }

    label->create();

    return label;
}

// ==================== Button ====================

shared_ptr<Button> LayoutParser::parseButton(const json& j, Control* parent) {
    pushJsonPath("rect");
    SRect rect = parseRect(j["rect"]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains("scale") && j["scale"].is_object()) {
        xScale = j["scale"].value("x", 1.0f);
        yScale = j["scale"].value("y", 1.0f);
    }

    auto btn = make_shared<Button>(parent, rect, xScale, yScale);

    parseCommonProperties(btn, j);

    if (j.contains("caption") && j["caption"].is_string()) {
        btn->setCaption(j["caption"].get<string>());
    }

    if (j.contains("captionSize") && j["captionSize"].is_number()) {
        btn->setCaptionSize(j["captionSize"].get<float>());
    }

    if (j.contains("enableTextShadow") && j["enableTextShadow"].is_boolean()) {
        btn->setTextShadowEnable(j["enableTextShadow"].get<bool>());
    }

    // Actors (state images)
    if (j.contains("actors") && j["actors"].is_object()) {
        pushJsonPath("actors");
        const json& actors = j["actors"];

        bool matchRect = actors.value("matchParentRect", false);

        auto createActor = [&](const json& v) -> shared_ptr<Actor> {
            if (v.is_null()) return nullptr;
            string filePath;
            string resourceId;
            if (v.is_string()) {
                filePath = v.get<string>();
            } else if (v.is_object()) {
                if (v.contains("file") && v["file"].is_string()) {
                    filePath = v["file"].get<string>();
                }
                if (v.contains("resourceId") && v["resourceId"].is_string()) {
                    resourceId = v["resourceId"].get<string>();
                }
            }
            if (!filePath.empty()) {
                return make_shared<Actor>(btn.get(), fs::path(filePath), matchRect, 1.0f, 1.0f);
            }
            if (!resourceId.empty()) {
                return make_shared<Actor>(btn.get(), resourceId, matchRect, 1.0f, 1.0f);
            }
            return nullptr;
        };

        if (actors.contains("normal") && !actors["normal"].is_null()) {
            auto actor = createActor(actors["normal"]);
            if (actor) btn->setNormalStateActor(actor);
        }
        if (actors.contains("hover") && !actors["hover"].is_null()) {
            auto actor = createActor(actors["hover"]);
            if (actor) btn->setHoverStateActor(actor);
        }
        if (actors.contains("pressed") && !actors["pressed"].is_null()) {
            auto actor = createActor(actors["pressed"]);
            if (actor) btn->setPressedStateActor(actor);
        }
        if (actors.contains("disabled") && !actors["disabled"].is_null()) {
            auto actor = createActor(actors["disabled"]);
            if (actor) btn->setDisabledStateActor(actor);
        }

        popJsonPath();
    }

    // LuotiAni (particle animation)
    if (j.contains("luotiAni") && !j["luotiAni"].is_null()) {
        pushJsonPath("luotiAni");
        const json& la = j["luotiAni"];
        string filePath;
        string resourceId;
        if (la.is_string()) {
            filePath = la.get<string>();
        } else if (la.is_object()) {
            if (la.contains("file") && la["file"].is_string()) {
                filePath = la["file"].get<string>();
            }
            if (la.contains("resourceId") && la["resourceId"].is_string()) {
                resourceId = la["resourceId"].get<string>();
            }
        }
        if (!filePath.empty()) {
            auto luotiAni = make_shared<LuotiAni>(btn.get(), 1.0f, 1.0f);
            luotiAni->loadAniDesc(fs::path(filePath));
            btn->setLuotiAni(luotiAni);
        } else if (!resourceId.empty()) {
            auto luotiAni = make_shared<LuotiAni>(btn.get(), 1.0f, 1.0f);
            luotiAni->loadAniDesc(resourceId);
            btn->setLuotiAni(luotiAni);
        }
        popJsonPath();
    }

    parseEvents(btn, j);

    if (j.contains("id") && j["id"].is_string()) {
        m_controlsById[j["id"].get<string>()] = btn;
    }

    btn->create();
    return btn;
}

// ==================== EditBox ====================

shared_ptr<EditBox> LayoutParser::parseEditBox(const json& j, Control* parent) {
    pushJsonPath("rect");
    SRect rect = parseRect(j["rect"]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains("scale") && j["scale"].is_object()) {
        xScale = j["scale"].value("x", 1.0f);
        yScale = j["scale"].value("y", 1.0f);
    }

    auto editBox = make_shared<EditBox>(parent, rect, xScale, yScale);

    parseCommonProperties(editBox, j);

    if (j.contains("text") && j["text"].is_string()) {
        editBox->setText(j["text"].get<string>());
    }

    if (j.contains("placeholder") && j["placeholder"].is_string()) {
        editBox->setPlaceholder(j["placeholder"].get<string>());
    }

    if (j.contains("passwordMode") && j["passwordMode"].is_boolean()) {
        editBox->setPasswordMode(j["passwordMode"].get<bool>());
    }

    if (j.contains("passwordChar") && j["passwordChar"].is_string()) {
        string pc = j["passwordChar"].get<string>();
        if (!pc.empty()) {
            editBox->setPasswordChar(pc[0]);
        }
    }

    if (j.contains("font") && j["font"].is_object()) {
        pushJsonPath("font");
        const json& font = j["font"];
        if (font.contains("name") && font["name"].is_string()) {
            editBox->setFont(parseFontName(font["name"].get<string>()));
        }
        if (font.contains("size") && font["size"].is_number()) {
            editBox->setFontSize(font["size"].get<int>());
        }
        popJsonPath();
    }

    if (j.contains("alignment") && j["alignment"].is_string()) {
        editBox->setAlignmentMode(parseAlignment(j["alignment"].get<string>()));
    }

    if (j.contains("margin")) {
        editBox->setMargin(parseMargin(j["margin"]));
    }

    parseEvents(editBox, j);

    if (j.contains("id") && j["id"].is_string()) {
        m_controlsById[j["id"].get<string>()] = editBox;
    }

    editBox->create();
    return editBox;
}

// ==================== Panel ====================

shared_ptr<Panel> LayoutParser::parsePanel(const json& j, Control* parent) {
    pushJsonPath("rect");
    SRect rect = parseRect(j["rect"]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains("scale") && j["scale"].is_object()) {
        xScale = j["scale"].value("x", 1.0f);
        yScale = j["scale"].value("y", 1.0f);
    }

    auto panel = make_shared<Panel>(parent, rect, xScale, yScale);

    parseCommonProperties(panel, j);

    if (j.contains("transparent") && j["transparent"].is_boolean()) {
        panel->setTransparent(j["transparent"].get<bool>());
    }

    if (j.contains("bgColor")) {
        SDL_Color bgColor = parseColor(j["bgColor"]);
        StateColor sc(StateColor::Type::Background);
        sc.setNormal(bgColor);
        panel->setBackgroundStateColor(sc);
    }

    if (j.contains("borderColor")) {
        SDL_Color borderColor = parseColor(j["borderColor"]);
        StateColor sc(StateColor::Type::Border);
        sc.setNormal(borderColor);
        panel->setBorderStateColor(sc);
    }

    // Panel has no events in Phase 1

    if (j.contains("id") && j["id"].is_string()) {
        m_controlsById[j["id"].get<string>()] = panel;
    }

    parseChildren(panel, j);

    panel->create();
    return panel;
}

// ==================== TextArea ====================

shared_ptr<TextArea> LayoutParser::parseTextArea(const json& j, Control* parent) {
    pushJsonPath("rect");
    SRect rect = parseRect(j["rect"]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains("scale") && j["scale"].is_object()) {
        xScale = j["scale"].value("x", 1.0f);
        yScale = j["scale"].value("y", 1.0f);
    }

    auto textArea = make_shared<TextArea>(parent, rect, xScale, yScale);

    parseCommonProperties(textArea, j);

    if (j.contains("text") && j["text"].is_string()) {
        textArea->setText(j["text"].get<string>());
    }

    if (j.contains("placeholder") && j["placeholder"].is_string()) {
        textArea->setPlaceholder(j["placeholder"].get<string>());
    }

    if (j.contains("wordWrap") && j["wordWrap"].is_boolean()) {
        textArea->setWordWrap(j["wordWrap"].get<bool>());
    }

    if (j.contains("lineHeight") && j["lineHeight"].is_number()) {
        textArea->setLineHeight(j["lineHeight"].get<int>());
    }

    if (j.contains("scrollBarThickness") && j["scrollBarThickness"].is_number()) {
        textArea->setScrollBarThickness(j["scrollBarThickness"].get<float>());
    }

    if (j.contains("font") && j["font"].is_object()) {
        pushJsonPath("font");
        const json& font = j["font"];
        if (font.contains("name") && font["name"].is_string()) {
            textArea->setFont(parseFontName(font["name"].get<string>()));
        }
        if (font.contains("size") && font["size"].is_number()) {
            textArea->setFontSize(font["size"].get<int>());
        }
        popJsonPath();
    }

    if (j.contains("alignment") && j["alignment"].is_string()) {
        textArea->setAlignmentMode(parseAlignment(j["alignment"].get<string>()));
    }

    if (j.contains("margin")) {
        textArea->setMargin(parseMargin(j["margin"]));
    }

    parseEvents(textArea, j);

    if (j.contains("id") && j["id"].is_string()) {
        m_controlsById[j["id"].get<string>()] = textArea;
    }

    textArea->create();
    return textArea;
}

// ==================== CheckBox ====================

static CheckState parseCheckState(const string& s) {
    if (s == "Checked")        return CheckState::Checked;
    if (s == "Indeterminate")  return CheckState::Indeterminate;
    return CheckState::Unchecked;
}

static CheckBoxStyle parseCheckBoxStyle(const string& s) {
    if (s == "Cross")   return CheckBoxStyle::Cross;
    if (s == "Circle")  return CheckBoxStyle::Circle;
    return CheckBoxStyle::Classic;
}

static CheckBoxLayout parseCheckBoxLayout(const string& s) {
    if (s == "TextLeft") return CheckBoxLayout::TextLeft;
    return CheckBoxLayout::TextRight;
}

static CheckBoxVerticalAlign parseCheckBoxVerticalAlign(const string& s) {
    if (s == "Top")    return CheckBoxVerticalAlign::Top;
    if (s == "Bottom") return CheckBoxVerticalAlign::Bottom;
    return CheckBoxVerticalAlign::Center;
}

shared_ptr<CheckBox> LayoutParser::parseCheckBox(const json& j, Control* parent) {
    pushJsonPath("rect");
    SRect rect = parseRect(j["rect"]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains("scale") && j["scale"].is_object()) {
        xScale = j["scale"].value("x", 1.0f);
        yScale = j["scale"].value("y", 1.0f);
    }

    auto checkBox = make_shared<CheckBox>(parent, rect, xScale, yScale);

    parseCommonProperties(checkBox, j);

    if (j.contains("caption") && j["caption"].is_string()) {
        checkBox->getCaption()->setCaption(j["caption"].get<string>());
    }

    if (j.contains("captionSize") && j["captionSize"].is_number()) {
        checkBox->getCaption()->setFontSize(j["captionSize"].get<int>());
    }

    if (j.contains("checkState") && j["checkState"].is_string()) {
        checkBox->setCheckState(parseCheckState(j["checkState"].get<string>()));
    }

    if (j.contains("style") && j["style"].is_string()) {
        checkBox->setStyle(parseCheckBoxStyle(j["style"].get<string>()));
    }

    if (j.contains("layout") && j["layout"].is_string()) {
        checkBox->setLayout(parseCheckBoxLayout(j["layout"].get<string>()));
    }

    if (j.contains("verticalAlign") && j["verticalAlign"].is_string()) {
        checkBox->setVerticalAlign(parseCheckBoxVerticalAlign(j["verticalAlign"].get<string>()));
    }

    if (j.contains("sizeRatio") && j["sizeRatio"].is_number()) {
        checkBox->setSizeRatio(j["sizeRatio"].get<float>());
    }

    if (j.contains("triState") && j["triState"].is_boolean()) {
        checkBox->setTriStateEnabled(j["triState"].get<bool>());
    }

    // CheckBox-specific colors
    if (j.contains("colors") && j["colors"].is_object()) {
        const json& colors = j["colors"];
        if (colors.contains("checkColor")) {
            checkBox->setCheckColor(parseColor(colors["checkColor"]));
        }
        if (colors.contains("crossColor")) {
            checkBox->setCrossColor(parseColor(colors["crossColor"]));
        }
        if (colors.contains("indeterminateColor")) {
            checkBox->setIndeterminateColor(parseColor(colors["indeterminateColor"]));
        }
        if (colors.contains("boxBorderColor")) {
            checkBox->setBoxBorderColor(parseColor(colors["boxBorderColor"]));
        }
    }

    parseEvents(checkBox, j);

    if (j.contains("id") && j["id"].is_string()) {
        m_controlsById[j["id"].get<string>()] = checkBox;
    }

    checkBox->create();
    return checkBox;
}

// ==================== ProgressBar ====================

static ProgressBarStyle parseProgressBarStyle(const string& s) {
    if (s == "Vertical") return ProgressBarStyle::Vertical;
    return ProgressBarStyle::Horizontal;
}

static ProgressBarTextMode parseProgressBarTextMode(const string& s) {
    if (s == "None")    return ProgressBarTextMode::None;
    if (s == "Custom")  return ProgressBarTextMode::Custom;
    return ProgressBarTextMode::Percent;
}

shared_ptr<ProgressBar> LayoutParser::parseProgressBar(const json& j, Control* parent) {
    pushJsonPath("rect");
    SRect rect = parseRect(j["rect"]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains("scale") && j["scale"].is_object()) {
        xScale = j["scale"].value("x", 1.0f);
        yScale = j["scale"].value("y", 1.0f);
    }

    auto progressBar = make_shared<ProgressBar>(parent, rect, xScale, yScale);

    parseCommonProperties(progressBar, j);

    if (j.contains("value") && j["value"].is_number()) {
        progressBar->setValue(j["value"].get<float>());
    }

    if (j.contains("range") && j["range"].is_object()) {
        float minVal = j["range"].value("min", 0.0f);
        float maxVal = j["range"].value("max", 100.0f);
        progressBar->setRange(minVal, maxVal);
    }

    if (j.contains("style") && j["style"].is_string()) {
        progressBar->setStyle(parseProgressBarStyle(j["style"].get<string>()));
    }

    if (j.contains("textMode") && j["textMode"].is_string()) {
        progressBar->setTextMode(parseProgressBarTextMode(j["textMode"].get<string>()));
    }

    if (j.contains("customText") && j["customText"].is_string()) {
        progressBar->setCustomText(j["customText"].get<string>());
    }

    if (j.contains("animationSpeed") && j["animationSpeed"].is_number()) {
        progressBar->setAnimationSpeed(j["animationSpeed"].get<float>());
    }

    if (j.contains("font") && j["font"].is_object()) {
        pushJsonPath("font");
        const json& font = j["font"];
        if (font.contains("name") && font["name"].is_string()) {
            progressBar->setFont(parseFontName(font["name"].get<string>()));
        }
        if (font.contains("size") && font["size"].is_number()) {
            progressBar->setFontSize(font["size"].get<int>());
        }
        popJsonPath();
    }

    if (j.contains("alignment") && j["alignment"].is_string()) {
        progressBar->setAlignmentMode(parseAlignment(j["alignment"].get<string>()));
    }

    // ProgressBar-specific colors
    if (j.contains("colors") && j["colors"].is_object()) {
        const json& colors = j["colors"];
        if (colors.contains("progressColor")) {
            progressBar->setProgressColor(parseColor(colors["progressColor"]));
        }
        if (colors.contains("backgroundColor")) {
            progressBar->setBackgroundColor(parseColor(colors["backgroundColor"]));
        }
    }

    parseEvents(progressBar, j);

    if (j.contains("id") && j["id"].is_string()) {
        m_controlsById[j["id"].get<string>()] = progressBar;
    }

    progressBar->create();
    return progressBar;
}

// ==================== ScrollBar ====================

static ScrollBarOrientation parseScrollBarOrientation(const string& s) {
    if (s == "Horizontal") return ScrollBarOrientation::Horizontal;
    return ScrollBarOrientation::Vertical;
}

shared_ptr<ScrollBar> LayoutParser::parseScrollBar(const json& j, Control* parent) {
    pushJsonPath("rect");
    SRect rect = parseRect(j["rect"]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains("scale") && j["scale"].is_object()) {
        xScale = j["scale"].value("x", 1.0f);
        yScale = j["scale"].value("y", 1.0f);
    }

    ScrollBarOrientation orientation = ScrollBarOrientation::Vertical;
    if (j.contains("orientation") && j["orientation"].is_string()) {
        orientation = parseScrollBarOrientation(j["orientation"].get<string>());
    }

    auto scrollBar = make_shared<ScrollBar>(parent, rect, orientation, xScale, yScale);

    parseCommonProperties(scrollBar, j);

    if (j.contains("value") && j["value"].is_number()) {
        scrollBar->setValue(j["value"].get<float>());
    }

    if (j.contains("range") && j["range"].is_object()) {
        float minVal = j["range"].value("min", 0.0f);
        float maxVal = j["range"].value("max", 100.0f);
        scrollBar->setRange(minVal, maxVal);
    }

    if (j.contains("pageSize") && j["pageSize"].is_number()) {
        scrollBar->setPageSize(j["pageSize"].get<float>());
    }

    if (j.contains("stepSize") && j["stepSize"].is_number()) {
        scrollBar->setStepSize(j["stepSize"].get<float>());
    }

    if (j.contains("thickness") && j["thickness"].is_number()) {
        scrollBar->setThickness(j["thickness"].get<float>());
    }

    parseEvents(scrollBar, j);

    if (j.contains("id") && j["id"].is_string()) {
        m_controlsById[j["id"].get<string>()] = scrollBar;
    }

    scrollBar->create();
    return scrollBar;
}

// ==================== 通用属性解析 ====================

void LayoutParser::parseCommonProperties(shared_ptr<ControlImpl> ctrl, const json& j) {
    if (!ctrl) return;

    // scale
    if (j.contains("scale") && j["scale"].is_object()) {
        float sx = j["scale"].value("x", 1.0f);
        float sy = j["scale"].value("y", 1.0f);
        ctrl->setScaleX(sx);
        ctrl->setScaleY(sy);
    }

    // margin
    if (j.contains("margin")) {
        ctrl->setMargin(parseMargin(j["margin"]));
    }

    // visible
    ctrl->setVisible(j.value("visible", true));

    // enabled
    ctrl->setEnable(j.value("enabled", true));

    // colors
    if (j.contains("colors") && j["colors"].is_object()) {
        pushJsonPath("colors");
        const json& colors = j["colors"];
        if (colors.contains("background")) {
            ctrl->setBackgroundStateColor(
                parseStateColor(colors["background"], StateColor::Type::Background));
        }
        if (colors.contains("border")) {
            ctrl->setBorderStateColor(
                parseStateColor(colors["border"], StateColor::Type::Border));
        }
        if (colors.contains("text")) {
            ctrl->setTextStateColor(
                parseStateColor(colors["text"], StateColor::Type::Text));
        }
        if (colors.contains("textShadow")) {
            ctrl->setTextShadowStateColor(
                parseStateColor(colors["textShadow"], StateColor::Type::TextShadow));
        }
        popJsonPath();
    }
}

void LayoutParser::parseEvents(shared_ptr<ControlImpl> ctrl, const json& j) {
    if (!j.contains("events") || j["events"].is_null() || !j["events"].is_object()) return;

    pushJsonPath("events");
    const json& events = j["events"];

    // Button: onClick
    if (auto btn = dynamic_pointer_cast<Button>(ctrl)) {
        if (events.contains("onClick") && events["onClick"].is_string()) {
            string handlerName = events["onClick"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                btn->setOnClick([handler](shared_ptr<Button> sender) {
                    handler(sender);
                });
            }
        }
    }

    // EditBox & TextArea: onTextChanged, onEnter
    if (auto editBox = dynamic_pointer_cast<EditBox>(ctrl)) {
        if (events.contains("onTextChanged") && events["onTextChanged"].is_string()) {
            string handlerName = events["onTextChanged"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                editBox->setOnTextChanged([handler](string text) {
                    handler(nullptr);
                });
            }
        }

        if (events.contains("onEnter") && events["onEnter"].is_string()) {
            string handlerName = events["onEnter"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                editBox->setOnEnter([handler]() {
                    handler(nullptr);
                });
            }
        }
    }

    // CheckBox: onCheckChanged
    if (auto cb = dynamic_pointer_cast<CheckBox>(ctrl)) {
        if (events.contains("onCheckChanged") && events["onCheckChanged"].is_string()) {
            string handlerName = events["onCheckChanged"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                cb->setOnCheckChanged([handler](shared_ptr<CheckBox> sender, CheckState state) {
                    handler(sender);
                });
            }
        }
    }

    // ProgressBar: onValueChanged
    if (auto pb = dynamic_pointer_cast<ProgressBar>(ctrl)) {
        if (events.contains("onValueChanged") && events["onValueChanged"].is_string()) {
            string handlerName = events["onValueChanged"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                pb->setOnValueChanged([handler](float val) {
                    handler(nullptr);
                });
            }
        }
    }

    // ScrollBar: onPositionChanged
    if (auto sb = dynamic_pointer_cast<ScrollBar>(ctrl)) {
        if (events.contains("onPositionChanged") && events["onPositionChanged"].is_string()) {
            string handlerName = events["onPositionChanged"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                sb->setOnPositionChanged([handler](float value, float min, float max) {
                    handler(nullptr);
                });
            }
        }
    }

    popJsonPath();
}

void LayoutParser::parseChildren(shared_ptr<Control> container, const json& j) {
    if (!j.contains("children") || !j["children"].is_array()) return;

    pushJsonPath("children");
    const json& children = j["children"];
    for (size_t i = 0; i < children.size(); ++i) {
        auto child = parseControl(children[i], container.get(), (int)i);
        if (child) {
            auto containerImpl = dynamic_pointer_cast<ControlImpl>(container);
            if (containerImpl) {
                containerImpl->addControl(child);
            }
        }
    }
    popJsonPath();
}

// ==================== 基础类型解析 ====================

SRect LayoutParser::parseRect(const json& j) {
    SRect rect;

    if (!j.is_object()) {
        pushJsonPath("rect");
        logError("'rect' must be an object with {x, y, w, h}");
        popJsonPath();
        return rect;
    }

    rect.left   = j.value("x", 0.0f);
    rect.top    = j.value("y", 0.0f);
    rect.width  = j.value("w", 0.0f);
    rect.height = j.value("h", 0.0f);

    vector<string> missing;
    if (!j.contains("x")) missing.push_back("x");
    if (!j.contains("y")) missing.push_back("y");
    if (!j.contains("w")) missing.push_back("w");
    if (!j.contains("h")) missing.push_back("h");

    if (!missing.empty()) {
        string fields;
        for (size_t i = 0; i < missing.size(); ++i) {
            if (i > 0) fields += ", ";
            fields += missing[i];
        }
        logWarn("rect missing field(s): " + fields + ", defaulting to 0");
    }

    return rect;
}

Margin LayoutParser::parseMargin(const json& j) {
    Margin margin;

    if (!j.is_object()) {
        pushJsonPath("margin");
        logWarn("'margin' must be an object, using defaults");
        popJsonPath();
        return margin;
    }

    margin.left   = j.value("left",   0.0f);
    margin.top    = j.value("top",    0.0f);
    margin.right  = j.value("right",  0.0f);
    margin.bottom = j.value("bottom", 0.0f);

    return margin;
}

SDL_Color LayoutParser::parseColor(const json& j) {
    SDL_Color color = {255, 255, 255, 255};

    if (j.is_string()) {
        string hex = j.get<string>();
        if (hex.empty() || hex[0] != '#') {
            logWarn("invalid color format \"" + hex + "\", using default white");
            return color;
        }
        hex = hex.substr(1);

        if (hex.length() == 3) {
            string r(2, hex[0]), g(2, hex[1]), b(2, hex[2]);
            color.r = (uint8_t)stoi(r, nullptr, 16);
            color.g = (uint8_t)stoi(g, nullptr, 16);
            color.b = (uint8_t)stoi(b, nullptr, 16);
            color.a = 255;
        } else if (hex.length() == 6) {
            color.r = (uint8_t)stoi(hex.substr(0, 2), nullptr, 16);
            color.g = (uint8_t)stoi(hex.substr(2, 2), nullptr, 16);
            color.b = (uint8_t)stoi(hex.substr(4, 2), nullptr, 16);
            color.a = 255;
        } else if (hex.length() == 8) {
            color.r = (uint8_t)stoi(hex.substr(0, 2), nullptr, 16);
            color.g = (uint8_t)stoi(hex.substr(2, 2), nullptr, 16);
            color.b = (uint8_t)stoi(hex.substr(4, 2), nullptr, 16);
            color.a = (uint8_t)stoi(hex.substr(6, 2), nullptr, 16);
        } else {
            logWarn("invalid hex color length \"" + hex + "\", using default white");
        }
    } else if (j.is_object()) {
        color.r = j.value("r", 255);
        color.g = j.value("g", 255);
        color.b = j.value("b", 255);
        color.a = j.value("a", 255);
    }

    return color;
}

StateColor LayoutParser::parseStateColor(const json& j, StateColor::Type type) {
    StateColor stateColor(type);

    if (j.is_null()) return stateColor;

    if (j.contains("normal")) {
        pushJsonPath("normal");
        stateColor.setNormal(parseColor(j["normal"]));
        popJsonPath();
    }
    if (j.contains("hover")) {
        pushJsonPath("hover");
        stateColor.setHover(parseColor(j["hover"]));
        popJsonPath();
    }
    if (j.contains("pressed")) {
        pushJsonPath("pressed");
        stateColor.setPressed(parseColor(j["pressed"]));
        popJsonPath();
    }
    if (j.contains("disabled")) {
        pushJsonPath("disabled");
        stateColor.setDisabled(parseColor(j["disabled"]));
        popJsonPath();
    }

    return stateColor;
}

FontName LayoutParser::parseFontName(const string& name) {
    static const unordered_map<string, FontName> nameMap = {
        {"HarmonyOS_Sans_SC_Regular",   FontName::HarmonyOS_Sans_SC_Regular},
        {"HarmonyOS_Sans_SC_Bold",      FontName::HarmonyOS_Sans_SC_Bold},
        {"HarmonyOS_Sans_SC_Light",     FontName::HarmonyOS_Sans_SC_Light},
        {"HarmonyOS_Sans_SC_Thin",      FontName::HarmonyOS_Sans_SC_Thin},
        {"HarmonyOS_Sans_SC_Medium",    FontName::HarmonyOS_Sans_SC_Medium},
        {"HarmonyOS_Sans_SC_Black",     FontName::HarmonyOS_Sans_SC_Black},
        {"MapleMono_NF_CN_Regular",     FontName::MapleMono_NF_CN_Regular},
        {"MapleMono_NF_CN_Bold",        FontName::MapleMono_NF_CN_Bold},
        {"Muyao_Softbrush",             FontName::Muyao_Softbrush},
        {"Asul_Bold",                   FontName::Asul_Bold},
        {"Quando_Regular",              FontName::Quando_Regular},
    };

    auto it = nameMap.find(name);
    if (it != nameMap.end()) return it->second;

    logWarn("unknown font name \"" + name + "\", using default");
    return FontName::HarmonyOS_Sans_SC_Regular;
}

AlignmentMode LayoutParser::parseAlignment(const string& align) {
    static const unordered_map<string, AlignmentMode> alignMap = {
        {"TOP_LEFT",      AlignmentMode::AM_TOP_LEFT},
        {"TOP_CENTER",    AlignmentMode::AM_TOP_CENTER},
        {"TOP_RIGHT",     AlignmentMode::AM_TOP_RIGHT},
        {"MID_LEFT",      AlignmentMode::AM_MID_LEFT},
        {"CENTER",        AlignmentMode::AM_CENTER},
        {"MID_RIGHT",     AlignmentMode::AM_MID_RIGHT},
        {"BOTTOM_LEFT",   AlignmentMode::AM_BOTTOM_LEFT},
        {"BOTTOM_CENTER", AlignmentMode::AM_BOTTOM_CENTER},
        {"BOTTOM_RIGHT",  AlignmentMode::AM_BOTTOM_RIGHT},
    };

    auto it = alignMap.find(align);
    if (it != alignMap.end()) return it->second;

    logWarn("unknown alignment \"" + align + "\", using TOP_LEFT");
    return AlignmentMode::AM_TOP_LEFT;
}

TTF_FontStyleFlags LayoutParser::parseFontStyle(const string& style) {
    static const unordered_map<string, TTF_FontStyleFlags> styleMap = {
        {"NORMAL",        TTF_STYLE_NORMAL},
        {"BOLD",          TTF_STYLE_BOLD},
        {"ITALIC",        TTF_STYLE_ITALIC},
        {"UNDERLINE",     TTF_STYLE_UNDERLINE},
        {"STRIKETHROUGH", TTF_STYLE_STRIKETHROUGH},
    };

    auto it = styleMap.find(style);
    if (it != styleMap.end()) return it->second;

    return TTF_STYLE_NORMAL;
}
