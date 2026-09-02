// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#include "Theme.h"
#include "PropertyNames.h"
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
        uint8_t r = (uint8_t)j.value(PropertyNames::kChannelR, 255);
        uint8_t g = (uint8_t)j.value(PropertyNames::kChannelG, 255);
        uint8_t b = (uint8_t)j.value(PropertyNames::kChannelB, 255);
        uint8_t a = (uint8_t)j.value(PropertyNames::kChannelA, 255);
        return SColor(r, g, b, a);
    }
    return SColor(255, 255, 255, 255);
}

static FontName parseFontName(const string& name) {
    static const unordered_map<string, FontName> nameMap = {
        {"HarmonyOS_Sans_SC_Regular",   FontName::HarmonyOS_Sans_SC_Regular},
        {"HarmonyOS_Sans_SC_Thin",      FontName::HarmonyOS_Sans_SC_Thin},
        {"MapleMono_NF_CN_Regular",     FontName::MapleMono_NF_CN_Regular},
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
    if ((*j).contains(PropertyNames::kStateKeyNormal)) sc.setNormal(parseHexColor((*j)[PropertyNames::kStateKeyNormal]));
    if ((*j).contains(PropertyNames::kStateKeyHover))  sc.setHover(parseHexColor((*j)[PropertyNames::kStateKeyHover]));
    if ((*j).contains(PropertyNames::kStateKeyPressed)) sc.setPressed(parseHexColor((*j)[PropertyNames::kStateKeyPressed]));
    if ((*j).contains(PropertyNames::kStateKeyDisabled)) sc.setDisabled(parseHexColor((*j)[PropertyNames::kStateKeyDisabled]));
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
    string path = PropertyNames::kThemeFontsPrefix + category + PropertyNames::kThemeNameSuffix;
    const json* j = navigate(m_data, path);
    if (j && j->is_string()) return parseFontName(j->get<string>());
    if (category != PropertyNames::kThemeCategoryDefault) return getFontName(PropertyNames::kThemeCategoryDefault);
    return FontName::HarmonyOS_Sans_SC_Regular;
}

int Theme::getFontSize(const string& category) const {
    string path = PropertyNames::kThemeFontsPrefix + category + PropertyNames::kThemeSizeSuffix;
    const json* j = navigate(m_data, path);
    if (j && j->is_number()) return j->get<int>();
    if (category != PropertyNames::kThemeCategoryDefault) return getFontSize(PropertyNames::kThemeCategoryDefault);
    return 16;
}

void Theme::applyCommonColors(shared_ptr<ControlImpl> ctrl, const string& category) const {
    string bg = PropertyNames::kThemeColorsPrefix + category + PropertyNames::kThemeBgSuffix;
    if (has(bg)) ctrl->setBackgroundStateColor(getStateColor(bg, StateColor::Type::Background));
    string border = PropertyNames::kThemeColorsPrefix + category + PropertyNames::kThemeBorderSuffix;
    if (has(border)) ctrl->setBorderStateColor(getStateColor(border, StateColor::Type::Border));
    string text = PropertyNames::kThemeColorsPrefix + category + PropertyNames::kThemeTextSuffix;
    if (has(text)) ctrl->setTextStateColor(getStateColor(text, StateColor::Type::Text));
    string textShadow = PropertyNames::kThemeColorsPrefix + category + PropertyNames::kThemeTextShadowSuffix;
    if (has(textShadow)) ctrl->setTextShadowStateColor(getStateColor(textShadow, StateColor::Type::TextShadow));
}

void Theme::applyFont(shared_ptr<Label> label, const string& category) const {
    FontName fn = getFontName(category);
    label->setFont(fn);
    int fs = getFontSize(category);
    label->setFontSize(fs);
}
