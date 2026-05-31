// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#include "LayoutParser.h"
#include "Dialog.h"
#include "LayoutEngine.h"
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

    if (j.contains("theme") && j["theme"].is_object()) {
        m_theme.parse(j["theme"]);
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
    DataContext::instance()->unwatchAll();
    m_controlsById.clear();
    m_menuBars.clear();
    m_currentJsonPath.clear();
    m_currentLineNo = 0;
    m_rawJsonContent.clear();
}

void LayoutParser::reset() {
    clear();
    m_handlers.clear();
}

const vector<shared_ptr<MenuBar>>& LayoutParser::getMenuBars() const {
    return m_menuBars;
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
    } else if (type == "Dialog") {
        result = parseDialog(j, parent);
    } else if (type == "MenuBar") {
        // MenuBar 不加入控件树（会被父容器裁剪），独立存储后再由调用方加入 BENCH 顶层
        auto menuBar = parseMenuBar(j, parent);
        if (menuBar) {
            m_menuBars.push_back(menuBar);
        }
        result = nullptr;
        popJsonPath();
        return nullptr;
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

    m_theme.applyCommonColors(label, "label");
    m_theme.applyFont(label, "label");
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
    parseBindings(label, j);

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

    m_theme.applyCommonColors(btn, "button");
    parseCommonProperties(btn, j);

    // captionLabel embedding (Phase 2): 使用完整 Label 配置
    if (j.contains("captionLabel") && j["captionLabel"].is_object()) {
        pushJsonPath("captionLabel");

        const json& cl = j["captionLabel"];

        auto builder = LabelBuilder(btn.get(), SRect(0, 0, rect.width, rect.height));

        builder.setFont(m_theme.getFontName("button"));
        builder.setFontSize(m_theme.getFontSize("button"));

        if (cl.contains("caption") && cl["caption"].is_string()) {
            builder.setCaption(cl["caption"].get<string>());
        }

        if (cl.contains("font") && cl["font"].is_object()) {
            const json& font = cl["font"];
            if (font.contains("name") && font["name"].is_string()) {
                builder.setFont(parseFontName(font["name"].get<string>()));
            }
            if (font.contains("size") && font["size"].is_number()) {
                builder.setFontSize(font["size"].get<int>());
            }
            if (font.contains("style") && font["style"].is_string()) {
                builder.SetFontStyle(parseFontStyle(font["style"].get<string>()));
            }
        }

        if (cl.contains("alignment") && cl["alignment"].is_string()) {
            builder.setAlignmentMode(parseAlignment(cl["alignment"].get<string>()));
        }

        if (cl.contains("shadow") && cl["shadow"].is_object()) {
            const json& shadow = cl["shadow"];
            if (shadow.contains("enabled") && shadow["enabled"].is_boolean()) {
                builder.setShadow(shadow["enabled"].get<bool>());
            }
            if (shadow.contains("offset") && shadow["offset"].is_object()) {
                SPoint offset;
                offset.x = shadow["offset"].value("x", 1);
                offset.y = shadow["offset"].value("y", 1);
                builder.setShadowOffset(offset);
            }
        }

        if (cl.contains("colors") && cl["colors"].is_object()) {
            const json& colors = cl["colors"];
            if (colors.contains("text") && colors["text"].is_object()) {
                builder.setTextStateColor(parseStateColor(colors["text"], StateColor::Type::Text));
            }
            if (colors.contains("textShadow") && colors["textShadow"].is_object()) {
                builder.setTextShadowStateColor(parseStateColor(colors["textShadow"], StateColor::Type::TextShadow));
            }
        }

        auto label = builder.build();
        btn->setCaptionLabel(label);

        popJsonPath();
    } else {
        // 简单方式 (Phase 1): 仅 caption / captionSize / enableTextShadow
        int themeFontSize = m_theme.getFontSize("button");
        if (themeFontSize != 16) {
            btn->setCaptionSize((float)themeFontSize);
        }

        if (j.contains("caption") && j["caption"].is_string()) {
            btn->setCaption(j["caption"].get<string>());
        }

        if (j.contains("captionSize") && j["captionSize"].is_number()) {
            btn->setCaptionSize(j["captionSize"].get<float>());
        }

        if (j.contains("enableTextShadow") && j["enableTextShadow"].is_boolean()) {
            btn->setTextShadowEnable(j["enableTextShadow"].get<bool>());
        }
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
            ScaleType scaleType = ScaleType::STRETCH;
            if (v.is_string()) {
                filePath = v.get<string>();
            } else if (v.is_object()) {
                if (v.contains("file") && v["file"].is_string()) {
                    filePath = v["file"].get<string>();
                }
                if (v.contains("resourceId") && v["resourceId"].is_string()) {
                    resourceId = v["resourceId"].get<string>();
                }
                if (v.contains("scaleType") && v["scaleType"].is_string()) {
                    string st = v["scaleType"].get<string>();
                    if (st == "FIT_CENTER")      scaleType = ScaleType::FIT_CENTER;
                    else if (st == "CENTER_CROP") scaleType = ScaleType::CENTER_CROP;
                    else if (st == "NONE")        scaleType = ScaleType::NONE;
                }
            }
            shared_ptr<Actor> actor = nullptr;
            if (!filePath.empty()) {
                actor = make_shared<Actor>(btn.get(), fs::path(filePath), matchRect, 1.0f, 1.0f);
            } else if (!resourceId.empty()) {
                actor = make_shared<Actor>(btn.get(), resourceId, matchRect, 1.0f, 1.0f);
            }
            if (actor) {
                actor->setScaleType(scaleType);
            }
            return actor;
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
            try {
                auto luotiAni = make_shared<LuotiAni>(btn.get(), 1.0f, 1.0f);
                luotiAni->loadAniDesc(fs::path(filePath));
                luotiAni->setRect(SRect(0, 0, rect.width, rect.height));
                luotiAni->prepare(0);
                luotiAni->play();
                btn->setLuotiAni(luotiAni);
            } catch (...) {
                logWarn("failed to load luotiAni from file: " + filePath);
            }
        } else if (!resourceId.empty()) {
            try {
                auto luotiAni = make_shared<LuotiAni>(btn.get(), 1.0f, 1.0f);
                luotiAni->loadAniDesc(resourceId);
                luotiAni->setRect(SRect(0, 0, rect.width, rect.height));
                luotiAni->prepare(0);
                luotiAni->play();
                btn->setLuotiAni(luotiAni);
            } catch (...) {
                logWarn("failed to load luotiAni from resource: " + resourceId);
            }
        }
        popJsonPath();
    }

    parseEvents(btn, j);
    parseBindings(btn, j);

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

    m_theme.applyCommonColors(editBox, "editbox");
    editBox->setFont(m_theme.getFontName("editbox"));
    editBox->setFontSize(m_theme.getFontSize("editbox"));
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
    parseBindings(editBox, j);

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

    m_theme.applyCommonColors(panel, "panel");
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

    // Layout engine
    if (j.contains("layout") && j["layout"].is_object()) {
        pushJsonPath("layout");
        const json& layoutJson = j["layout"];
        string layoutType = layoutJson.value("type", "HFlow");
        float gap = layoutJson.value("gap", 0.0f);

        Margin padding{0,0,0,0};
        if (layoutJson.contains("padding") && layoutJson["padding"].is_object()) {
            padding = parseMargin(layoutJson["padding"]);
        }

        shared_ptr<LayoutEngine> engine;
        if (layoutType == "VFlow") {
            engine = make_shared<VFlowLayout>(gap, padding);
        } else if (layoutType == "Anchor") {
            engine = make_shared<AnchorLayout>(padding);
        } else if (layoutType == "Grid") {
            auto gridEngine = make_shared<GridLayout>(gap, padding);
            if (layoutJson.contains("columns") && layoutJson["columns"].is_array()) {
                vector<GridSize> cols;
                for (const auto& c : layoutJson["columns"]) {
                    cols.push_back(parseGridSize(c));
                }
                gridEngine->setColumns(cols);
            }
            if (layoutJson.contains("rows") && layoutJson["rows"].is_array()) {
                vector<GridSize> rows;
                for (const auto& r : layoutJson["rows"]) {
                    rows.push_back(parseGridSize(r));
                }
                gridEngine->setRows(rows);
            }
            engine = gridEngine;
        } else {
            engine = make_shared<HFlowLayout>(gap, padding);
        }
        panel->setLayoutEngine(engine);

        // Per-child layout properties
        if (j.contains("children") && j["children"].is_array()) {
            const json& children = j["children"];
            auto& panelChildren = panel->getChildren();
            for (size_t i = 0; i < children.size() && i < panelChildren.size(); ++i) {
                if (children[i].contains("flowWeight") && children[i]["flowWeight"].is_number()) {
                    float fw = children[i]["flowWeight"].get<float>();
                    FlowItemProps props;
                    props.flexWeight = fw;
                    panel->setChildFlowProps(panelChildren[i].get(), props);
                }
                if (children[i].contains("anchor") && children[i]["anchor"].is_string()) {
                    AnchorInfo info;
                    info.anchor = children[i]["anchor"].get<string>();
                    if (children[i].contains("anchorOffset") && children[i]["anchorOffset"].is_object()) {
                        info.offset = parseMargin(children[i]["anchorOffset"]);
                    }
                    panel->setChildAnchorProps(panelChildren[i].get(), info);
                }
                if (children[i].contains("grid") && children[i]["grid"].is_object()) {
                    const json& g = children[i]["grid"];
                    GridItemProps props;
                    props.row = g.value("row", 0);
                    props.col = g.value("col", 0);
                    props.rowSpan = g.value("rowSpan", 1);
                    props.colSpan = g.value("colSpan", 1);
                    panel->setChildGridProps(panelChildren[i].get(), props);
                }
            }
        }

        panel->resolveChildPercentages();
        panel->reflowChildren();
        popJsonPath();
    } else {
        panel->resolveChildPercentages();
    }

    panel->create();
    return panel;
}

// ==================== Dialog ====================

shared_ptr<Dialog> LayoutParser::parseDialog(const json& j, Control* parent) {
    pushJsonPath("rect");
    SRect rect = parseRect(j["rect"]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains("scale") && j["scale"].is_object()) {
        xScale = j["scale"].value("x", 1.0f);
        yScale = j["scale"].value("y", 1.0f);
    }

    auto dialog = make_shared<Dialog>(parent, rect, xScale, yScale);

    if (j.contains("title") && j["title"].is_string()) {
        dialog->setTitle(j["title"].get<string>());
    }

    if (j.contains("text") && j["text"].is_string()) {
        dialog->setText(j["text"].get<string>());
    }

    if (j.contains("okBtnCaption") && j["okBtnCaption"].is_string()) {
        dialog->setOkBtnCaption(j["okBtnCaption"].get<string>());
    }

    if (j.contains("id") && j["id"].is_string()) {
        m_controlsById[j["id"].get<string>()] = dialog;
    }

    dialog->create();
    dialog->hide();
    return dialog;
}

// ==================== MenuBar ====================

shared_ptr<MenuBar> LayoutParser::parseMenuBar(const json& j, Control* parent) {
    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains("scale") && j["scale"].is_object()) {
        xScale = j["scale"].value("x", 1.0f);
        yScale = j["scale"].value("y", 1.0f);
    }

    // 使用 nullptr parent：MenuBar 独立于控件树，调用方负责添加到 BENCH 顶层
    auto menuBar = make_shared<MenuBar>(nullptr, xScale, yScale);

    m_theme.applyCommonColors(menuBar, "menubar");
    parseCommonProperties(menuBar, j);

    // font.size (static global setting)
    if (j.contains("font") && j["font"].is_object()) {
        pushJsonPath("font");
        if (j["font"].contains("size") && j["font"]["size"].is_number()) {
            float fontSize = (float)j["font"]["size"].get<int>();
            MenuBar::setFontSize(fontSize);
            // auto-recalculate barHeight if not explicitly set
            if (!j.contains("barHeight")) {
                menuBar->setBarHeight(fontSize * 1.6f);
            }
        }
        popJsonPath();
    }

    // barHeight (overrides auto-calculation from font.size)
    if (j.contains("barHeight") && j["barHeight"].is_number()) {
        menuBar->setBarHeight(j["barHeight"].get<float>());
    }

    // menus array
    if (j.contains("menus") && j["menus"].is_array()) {
        pushJsonPath("menus");
        const json& menus = j["menus"];
        for (size_t i = 0; i < menus.size(); ++i) {
            const json& menuJson = menus[i];
            string caption = menuJson.value("caption", "Menu");

            if (menuJson.contains("items") && menuJson["items"].is_array()) {
                auto panel = make_shared<MenuPanel>(nullptr, xScale, yScale);
                populateMenuPanel(panel, menuJson["items"], xScale, yScale);
                menuBar->addMenu(caption, panel);
            } else {
                pushJsonPath("menus[" + to_string(i) + "]");
                logWarn("menu entry \"" + caption + "\" has no 'items' array, skipping");
                popJsonPath();
            }
        }
        popJsonPath();
    }

    if (j.contains("id") && j["id"].is_string()) {
        m_controlsById[j["id"].get<string>()] = menuBar;
    }

    return menuBar;
}

void LayoutParser::populateMenuPanel(shared_ptr<MenuPanel> panel, const json& items, float xScale, float yScale) {
    for (const auto& itemJson : items) {
        // Separator
        if (itemJson.contains("type") && itemJson["type"].is_string() &&
            itemJson["type"].get<string>() == "Separator") {
            panel->addSeparator();
            continue;
        }

        // Determine type: SubMenu if has nested "items"
        MenuItemType type = MenuItemType::Normal;
        if (itemJson.contains("items") && itemJson["items"].is_array()) {
            type = MenuItemType::SubMenu;
        }

        auto item = make_shared<MenuItem>(panel.get(), type, xScale, yScale);

        // Caption
        if (itemJson.contains("caption") && itemJson["caption"].is_string()) {
            item->setCaption(itemJson["caption"].get<string>());
        }

        // Shortcut
        if (itemJson.contains("shortcut") && itemJson["shortcut"].is_string()) {
            item->setShortcut(itemJson["shortcut"].get<string>());
        }

        // Checked
        if (itemJson.contains("checked") && itemJson["checked"].is_boolean()) {
            item->setChecked(itemJson["checked"].get<bool>());
        }

        // Enabled
        if (itemJson.contains("enabled") && itemJson["enabled"].is_boolean()) {
            item->setEnable(itemJson["enabled"].get<bool>());
        }

        // SubMenu (recursive)
        if (itemJson.contains("items") && itemJson["items"].is_array()) {
            auto subPanel = make_shared<MenuPanel>(nullptr, xScale, yScale);
            populateMenuPanel(subPanel, itemJson["items"], xScale, yScale);
            item->setSubMenu(subPanel);
        }

        // Events (onClick)
        if (itemJson.contains("events") && itemJson["events"].is_object()) {
            parseEvents(item, itemJson);
        }

        item->create();

        panel->addItem(item);
    }
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

    m_theme.applyCommonColors(textArea, "textarea");
    textArea->setFont(m_theme.getFontName("textarea"));
    textArea->setFontSize(m_theme.getFontSize("textarea"));
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
    parseBindings(textArea, j);

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

    m_theme.applyCommonColors(checkBox, "checkbox");
    parseCommonProperties(checkBox, j);

    if (j.contains("caption") && j["caption"].is_string()) {
        checkBox->getCaption()->setCaption(j["caption"].get<string>());
    }

    int cbFontSize = m_theme.getFontSize("checkbox");
    checkBox->getCaption()->setFontSize(cbFontSize);

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

    // Theme CheckBox-specific colors (defaults)
    SDL_Color themeCheck;
    if (m_theme.getColorOpt("colors.checkbox.check", themeCheck))
        checkBox->setCheckColor(themeCheck);
    SDL_Color themeCross;
    if (m_theme.getColorOpt("colors.checkbox.cross", themeCross))
        checkBox->setCrossColor(themeCross);
    SDL_Color themeIndet;
    if (m_theme.getColorOpt("colors.checkbox.indeterminate", themeIndet))
        checkBox->setIndeterminateColor(themeIndet);
    SDL_Color themeBoxBorder;
    if (m_theme.getColorOpt("colors.checkbox.boxBorder", themeBoxBorder))
        checkBox->setBoxBorderColor(themeBoxBorder);

    // CheckBox-specific colors (JSON overrides)
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
    parseBindings(checkBox, j);

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

    m_theme.applyCommonColors(progressBar, "progressbar");
    progressBar->setFont(m_theme.getFontName("progressbar"));
    progressBar->setFontSize(m_theme.getFontSize("progressbar"));
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

    // Theme ProgressBar-specific colors (defaults)
    SDL_Color themeProgress;
    if (m_theme.getColorOpt("colors.progressbar.progress", themeProgress))
        progressBar->setProgressColor(themeProgress);
    SDL_Color themeTrack;
    if (m_theme.getColorOpt("colors.progressbar.track", themeTrack))
        progressBar->setBackgroundColor(themeTrack);

    // ProgressBar-specific colors (JSON overrides)
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
    parseBindings(progressBar, j);

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

    m_theme.applyCommonColors(scrollBar, "scrollbar");
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
    parseBindings(scrollBar, j);

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

    // Label: onClick (supports hyperlink mode)
    if (auto label = dynamic_pointer_cast<Label>(ctrl)) {
        if (events.contains("onClick") && events["onClick"].is_string()) {
            string handlerName = events["onClick"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                label->setOnClick([handler](shared_ptr<Label> sender) {
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
                editBox->setOnTextChanged([handler](shared_ptr<Control> sender, string) {
                    handler(sender);
                });
            }
        }

        if (events.contains("onEnter") && events["onEnter"].is_string()) {
            string handlerName = events["onEnter"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                editBox->setOnEnter([handler](shared_ptr<Control> sender) {
                    handler(sender);
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
                cb->setOnCheckChanged([handler](shared_ptr<CheckBox> sender, CheckState, CheckState) {
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
                pb->setOnValueChanged([handler](shared_ptr<ProgressBar> sender, float, float) {
                    handler(sender);
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
                sb->setOnPositionChanged([handler](shared_ptr<ScrollBar> sender, float, float, float, float) {
                    handler(sender);
                });
            }
        }
    }

    // MenuItem: onClick
    if (auto mi = dynamic_pointer_cast<MenuItem>(ctrl)) {
        if (events.contains("onClick") && events["onClick"].is_string()) {
            string handlerName = events["onClick"].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                mi->setOnClick([handler](shared_ptr<MenuItem> sender) {
                    handler(sender);
                });
            }
        }
    }

    popJsonPath();
}

static void applyBinding(shared_ptr<ControlImpl> ctrl, const string& prop, const DataValue& val) {
    if (prop == "visible") { ctrl->setVisible(val.asBool()); return; }
    if (prop == "enabled") { ctrl->setEnable(val.asBool()); return; }

    if (prop == "caption") {
        if (auto label = dynamic_pointer_cast<Label>(ctrl)) { label->setCaption(val.asString()); return; }
        if (auto btn = dynamic_pointer_cast<Button>(ctrl)) { btn->setCaption(val.asString()); return; }
    }
    if (prop == "text") {
        if (auto eb = dynamic_pointer_cast<EditBox>(ctrl)) { eb->setText(val.asString()); return; }
        if (auto ta = dynamic_pointer_cast<TextArea>(ctrl)) { ta->setText(val.asString()); return; }
    }
    if (prop == "placeholder") {
        if (auto eb = dynamic_pointer_cast<EditBox>(ctrl)) { eb->setPlaceholder(val.asString()); return; }
    }
    if (prop == "value") {
        if (auto pb = dynamic_pointer_cast<ProgressBar>(ctrl)) { pb->setValue((float)val.asDouble()); return; }
        if (auto sb = dynamic_pointer_cast<ScrollBar>(ctrl)) { sb->setValue((float)val.asDouble()); return; }
    }
    if (prop == "checkState") {
        if (auto cb = dynamic_pointer_cast<CheckBox>(ctrl)) {
            CheckState s = CheckState::Unchecked;
            string vs = val.asString();
            if (vs == "Checked") s = CheckState::Checked;
            else if (vs == "Indeterminate") s = CheckState::Indeterminate;
            cb->setCheckState(s);
            return;
        }
    }
}

static void bindProperty(shared_ptr<ControlImpl> ctrl, const string& prop, const string& source, const string& mode) {
    auto ctx = DataContext::instance();
    weak_ptr<ControlImpl> weakCtrl = ctrl;
    if (mode == "oneWay" || mode == "twoWay") {
        ctx->watch(source, [weakCtrl, prop](const DataValue& val) {
            auto locked = dynamic_pointer_cast<ControlImpl>(weakCtrl.lock());
            if (locked) applyBinding(locked, prop, val);
        });
    }
    if (mode == "twoWay") {
        if (prop == "text") {
            if (auto eb = dynamic_pointer_cast<EditBox>(ctrl)) {
                auto s = source;
                eb->setOnTextChanged([s](shared_ptr<Control>, string text) {
                    DataContext::instance()->set(s, text);
                });
            }
        }
        if (prop == "checkState") {
            if (auto cb = dynamic_pointer_cast<CheckBox>(ctrl)) {
                auto s = source;
                shared_ptr<CheckBox> weakCB = cb;
                cb->setOnCheckChanged([s, weakCB](shared_ptr<CheckBox>, CheckState, CheckState newState) {
                    string vs = "Unchecked";
                    if (newState == CheckState::Checked) vs = "Checked";
                    else if (newState == CheckState::Indeterminate) vs = "Indeterminate";
                    DataContext::instance()->set(s, vs);
                });
            }
        }
        if (prop == "value") {
            if (auto sb = dynamic_pointer_cast<ScrollBar>(ctrl)) {
                auto s = source;
                sb->setOnPositionChanged([s](shared_ptr<ScrollBar>, float, float newValue, float, float) {
                    DataContext::instance()->set(s, (double)newValue);
                });
            }
        }
    }
}

void LayoutParser::parseBindings(shared_ptr<ControlImpl> ctrl, const json& j) {
    if (!j.contains("bind") || !j["bind"].is_object()) return;
    pushJsonPath("bind");
    const json& bind = j["bind"];
    for (auto it = bind.begin(); it != bind.end(); ++it) {
        string prop = it.key();
        string source;
        string mode = "oneWay";
        if (it.value().is_string()) {
            source = it.value().get<string>();
        } else if (it.value().is_object()) {
            source = it.value().value("source", "");
            mode = it.value().value("mode", "oneWay");
        } else {
            continue;
        }
        bindProperty(ctrl, prop, source, mode);
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

    auto parseField = [&](const string& key, float& pixel, bool& isPct, float& pct, float defaultVal) {
        if (!j.contains(key)) {
            pixel = defaultVal;
            return;
        }
        const json& v = j[key];
        if (v.is_string()) {
            string s = v.get<string>();
            if (s.size() > 1 && s.back() == '%') {
                isPct = true;
                pct = stof(s.substr(0, s.size() - 1));
                return;
            }
        }
        if (v.is_number()) {
            pixel = v.get<float>();
        }
    };

    parseField("x", rect.left,   rect.leftIsPct,   rect.leftPct,   0.0f);
    parseField("y", rect.top,    rect.topIsPct,    rect.topPct,    0.0f);
    parseField("w", rect.width,  rect.widthIsPct,  rect.widthPct,  0.0f);
    parseField("h", rect.height, rect.heightIsPct, rect.heightPct, 0.0f);

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

GridSize LayoutParser::parseGridSize(const json& j) {
    GridSize gs;
    if (j.is_string()) {
        string s = j.get<string>();
        if (s == "auto") {
            gs.type = GridSize::Auto;
        } else if (s.size() > 2 && s.substr(s.size() - 2) == "fr") {
            gs.type = GridSize::Flex;
            gs.value = stof(s.substr(0, s.size() - 2));
        } else if (s.size() > 2 && s.substr(s.size() - 2) == "px") {
            gs.type = GridSize::Fixed;
            gs.value = stof(s.substr(0, s.size() - 2));
        } else {
            gs.type = GridSize::Fixed;
            gs.value = stof(s);
        }
    } else if (j.is_number()) {
        gs.type = GridSize::Fixed;
        gs.value = j.get<float>();
    }
    return gs;
}
