// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#ifndef ThemeH
#define ThemeH

#include <memory>
#include <string>
#include <unordered_map>
#include "SColor.h"
#include "nlohmann/json.hpp"
#include "ControlBase.h"
#include "Label.h"

using namespace std;
using json = nlohmann::json;

class Theme {
public:
    Theme() = default;

    void clear();
    bool has(const string& path) const;
    void parse(const json& j);

    StateColor getStateColor(const string& path, StateColor::Type type) const;
    SColor getColor(const string& path) const;
    bool getColorOpt(const string& path, SColor& out) const;

    FontName getFontName(const string& category) const;
    int getFontSize(const string& category) const;

    void applyCommonColors(shared_ptr<ControlImpl> ctrl, const string& category) const;
    void applyFont(shared_ptr<Label> label, const string& category) const;

private:
    json m_data;
};

#endif
