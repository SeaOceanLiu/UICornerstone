// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#include "Theme.h"
#include <algorithm>

static SColor parseHexColor(const json& j) {
    if (j.is_string()) {
        string hex = j.get<string>();
        if (hex.empty() || hex[0] != '#') return SColor(255, 255, 255, 255);
        hex = hex.substr(1);
        if (hex.length() == 3) {
            uint8_t r = (uint8_t)stoi(string(2, hex[0]), nullptr, 16);
            uint8_t g = (uint8_t)stoi(string(2, hex[1]), nullptr, 16);
            uint8_t b = (uint8_t)stoi(string(2, hex[2]), nullptr, 16);
            return SColor(r, g, b, 255);
        } else if (hex.length() == 6) {
            uint8_t r = (uint8_t)stoi(hex.substr(0, 2), nullptr, 16);
            uint8_t g = (uint8_t)stoi(hex.substr(2, 2), nullptr, 16);
            uint8_t b = (uint8_t)stoi(hex.substr(4, 2), nullptr, 16);
            return SColor(r, g, b, 255);
        } else if (hex.length() == 8) {
            uint8_t r = (uint8_t)stoi(hex.substr(0, 2), nullptr, 16);
            uint8_t g = (uint8_t)stoi(hex.substr(2, 2), nullptr, 16);
            uint8_t b = (uint8_t)stoi(hex.substr(4, 2), nullptr, 16);
            uint8_t a = (uint8_t)stoi(hex.substr(6, 2), nullptr, 16);
            return SColor(r, g, b, a);
        }
    } else if (j.is_object()) {
        uint8_t r = (uint8_t)j.value("r", 255);
        uint8_t g = (uint8_t)j.value("g", 255);
        uint8_t b = (uint8_t)j.value("b", 255);
        uint8_t a = (uint8_t)j.value("a", 255);
        return SColor(r, g, b, a);
    }
    return SColor(255, 255, 255, 255);
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

SColor Theme::getColor(const string& path) const {
    SColor c(255, 255, 255, 255);
    const json* j = navigate(m_data, path);
    if (j) c = parseHexColor(*j);
    return c;
}

bool Theme::getColorOpt(const string& path, SColor& out) const {
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
