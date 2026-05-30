// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#include "Theme.h"
#include <algorithm>

static SDL_Color parseHexColor(const json& j) {
    SDL_Color color = {255, 255, 255, 255};
    if (j.is_string()) {
        string hex = j.get<string>();
        if (hex.empty() || hex[0] != '#') return color;
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
        }
    } else if (j.is_object()) {
        color.r = j.value("r", 255);
        color.g = j.value("g", 255);
        color.b = j.value("b", 255);
        color.a = j.value("a", 255);
    }
    return color;
}

static FontName parseFontName(const string& name) {
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
    return FontName::HarmonyOS_Sans_SC_Regular;
}

// Navigate json along dot-separated path, return ref or nullptr
static const json* navigate(const json& j, const string& path) {
    if (j.is_null()) return nullptr;
    const json* cur = &j;
    size_t start = 0;
    while (start < path.size()) {
        size_t dot = path.find('.', start);
        string key = path.substr(start, dot - start);
        if (!cur->is_object() || !cur->contains(key)) return nullptr;
        cur = &(*cur)[key];
        if (dot == string::npos) break;
        start = dot + 1;
    }
    return cur;
}

void Theme::clear() {
    m_data = json();
}

bool Theme::has(const string& path) const {
    return navigate(m_data, path) != nullptr;
}

void Theme::parse(const json& j) {
    m_data = j;
}

StateColor Theme::getStateColor(const string& path, StateColor::Type type) const {
    StateColor sc(type);
    const json* j = navigate(m_data, path);
    if (!j || !j->is_object()) return sc;
    if ((*j).contains("normal")) sc.setNormal(parseHexColor((*j)["normal"]));
    if ((*j).contains("hover"))  sc.setHover(parseHexColor((*j)["hover"]));
    if ((*j).contains("pressed")) sc.setPressed(parseHexColor((*j)["pressed"]));
    if ((*j).contains("disabled")) sc.setDisabled(parseHexColor((*j)["disabled"]));
    return sc;
}

SDL_Color Theme::getColor(const string& path) const {
    SDL_Color c = {255, 255, 255, 255};
    const json* j = navigate(m_data, path);
    if (j) c = parseHexColor(*j);
    return c;
}

bool Theme::getColorOpt(const string& path, SDL_Color& out) const {
    const json* j = navigate(m_data, path);
    if (!j) return false;
    out = parseHexColor(*j);
    return true;
}

FontName Theme::getFontName(const string& category) const {
    string path = "fonts." + category + ".name";
    const json* j = navigate(m_data, path);
    if (j && j->is_string()) return parseFontName(j->get<string>());
    if (category != "default") return getFontName("default");
    return FontName::HarmonyOS_Sans_SC_Regular;
}

int Theme::getFontSize(const string& category) const {
    string path = "fonts." + category + ".size";
    const json* j = navigate(m_data, path);
    if (j && j->is_number()) return j->get<int>();
    if (category != "default") return getFontSize("default");
    return 16;
}

void Theme::applyCommonColors(shared_ptr<ControlImpl> ctrl, const string& category) const {
    string bg = "colors." + category + ".bg";
    if (has(bg)) ctrl->setBackgroundStateColor(getStateColor(bg, StateColor::Type::Background));
    string border = "colors." + category + ".border";
    if (has(border)) ctrl->setBorderStateColor(getStateColor(border, StateColor::Type::Border));
    string text = "colors." + category + ".text";
    if (has(text)) ctrl->setTextStateColor(getStateColor(text, StateColor::Type::Text));
    string textShadow = "colors." + category + ".textShadow";
    if (has(textShadow)) ctrl->setTextShadowStateColor(getStateColor(textShadow, StateColor::Type::TextShadow));
}

void Theme::applyFont(shared_ptr<Label> label, const string& category) const {
    FontName fn = getFontName(category);
    label->setFont(fn);
    int fs = getFontSize(category);
    label->setFontSize(fs);
}
