#ifdef ThemeStyleH
#error ThemeStyleHAlreadyDefined
#endif
// CR20261102001/002/003：CSS-like 主题 + 通用 content-padding + 响应式主题实现
#include "ThemeStyle.h"
#include "Theme.h"
#include "ControlBase.h"
#include "Label.h"
#include "Button.h"
#include "EditBox.h"
#include "UIContext.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>

using namespace std;

namespace {

string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

string lower(string s) {
    for (char& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

bool parseColorValue(const string& raw, SColor& out) {
    string v = trim(raw);
    if (v.empty()) return false;
    if (v[0] == '#') return SColor::fromHex(v, out);
    string l = lower(v);
    if (l == "transparent") { out = SColor(0,0,0,0); return true; }
    if (l == "white") { out = SColor::White(); return true; }
    if (l == "black") { out = SColor::Black(); return true; }
    if (l == "red") { out = SColor::Red(); return true; }
    if (l == "green") { out = SColor::Green(); return true; }
    if (l == "blue") { out = SColor::Blue(); return true; }
    if (l.rfind("rgb(", 0) == 0 || l.rfind("rgba(", 0) == 0) {
        int r=0,g=0,b=0; float a=1.0f;
        const char* p = strchr(l.c_str(), '(');
        if (p) {
            if (sscanf(p + 1, "%d,%d,%d,%f", &r,&g,&b,&a) >= 3 ||
                sscanf(p + 1, "%d , %d , %d , %f", &r,&g,&b,&a) >= 3)
                { out = SColor(r,g,b,(int)(a*255.0f)); return true; }
        }
    }
    return false;
}

bool splitFloatList(const string& s, vector<float>& out) {
    out.clear(); const char* p = s.c_str();
    while (*p) {
        while (*p == ' ' || *p == ',' || *p == '\t') ++p;
        if (!*p) break;
        char* e = nullptr; float f = strtof(p, &e);
        if (e == p) return false;
        out.push_back(f); p = e;
    }
    return !out.empty();
}

bool isColorKey(const string& k) {
    return k == "background" || k == "border-color" || k == "text-color" ||
           k == "text-shadow" || k == "box-shadow-color";
}

string propForColorKey(const string& k) {
    if (k == "background") return PropertyNames::kBackground;
    if (k == "border-color") return PropertyNames::kBorder;
    if (k == "text-color") return PropertyNames::kText;
    if (k == "text-shadow") return PropertyNames::kTextShadow;
    return "box-shadow-color";
}

string stateSuffix(const string& st) {
    if (st == "hover") return ".hover";
    if (st == "pressed" || st == "selected") return ".pressed";
    if (st == "disabled") return ".disabled";
    return "";
}

void applyFontToControl(Control* ctl, const string& name, int size) {
    if (!ctl) return;
    FontName fn = FontName::HarmonyOS_Sans_SC_Regular;
    if (!name.empty()) fn = FontNameFromString(name.c_str());
    if (auto* label = dynamic_cast<Label*>(ctl)) {
        if (!name.empty()) label->setFont(fn);
        if (size > 0) label->setFontSize(size);
    } else if (auto* eb = dynamic_cast<EditBox*>(ctl)) {
        if (!name.empty()) eb->setFont(fn);
        if (size > 0) eb->setFontSize(size);
    } else if (auto* btn = dynamic_cast<Button*>(ctl)) {
        if (!name.empty()) btn->setFont(fn);
        if (size > 0) btn->setCaptionSize((float)size);
    } else {
        if (!name.empty()) ctl->setStringProperty(PropertyNames::kFont, name.c_str());
        if (size > 0) ctl->setIntProperty(PropertyNames::kFontSize, size);
    }
}

} // namespace

// ==================== ThemeStyleSheet ====================

bool ThemeStyleSheet::parse(const json& theme) {
    m_vars.clear(); m_base = StyleSet(); m_variants.clear();
    if (!theme.is_object()) return false;
    if (theme.contains("variables") && theme["variables"].is_object()) {
        for (auto it = theme["variables"].begin(); it != theme["variables"].end(); ++it) {
            if (it.value().is_string()) m_vars[it.key()] = it.value().get<string>();
            else if (it.value().is_number()) m_vars[it.key()] = to_string(it.value().get<double>());
        }
    }
    if (theme.contains("styles")) parseRules(theme["styles"], m_base);
    if (theme.contains("media") && theme["media"].is_array()) {
        for (const auto& m : theme["media"]) {
            if (!m.is_object()) continue;
            MediaQuery q; q.id = m.value("id", "");
            if (m.contains("query")) {
                const json& qq = m["query"];
                if (qq.is_string()) {
                    string s = lower(qq.get<string>());
                    auto numAfter = [&](const char* key) -> float {
                        size_t p = s.find(key); if (p == string::npos) return 0.0f;
                        p += strlen(key); while (p < s.size() && (s[p]==' '||s[p]==':'||s[p]=='(')) ++p;
                        return (float)atof(s.c_str() + p);
                    };
                    q.minW = numAfter("min-width"); q.maxW = numAfter("max-width");
                    q.minH = numAfter("min-height"); q.maxH = numAfter("max-height");
                    if (s.find("portrait") != string::npos) q.orientation = "portrait";
                    if (s.find("landscape") != string::npos) q.orientation = "landscape";
                } else if (qq.is_object()) {
                    q.minW = qq.value("min-width", 0.0f); q.maxW = qq.value("max-width", 0.0f);
                    q.minH = qq.value("min-height", 0.0f); q.maxH = qq.value("max-height", 0.0f);
                    q.minScale = qq.value("min-scale", 0.0f); q.maxScale = qq.value("max-scale", 0.0f);
                    q.orientation = qq.value("orientation", "");
                }
            }
            if (m.contains("variables") && m["variables"].is_object()) {
                for (auto it = m["variables"].begin(); it != m["variables"].end(); ++it)
                    if (it.value().is_string()) q.vars[it.key()] = it.value().get<string>();
            }
            if (m.contains("declarations") && m["declarations"].is_object()) {
                int ord = 0; collectDecls(m["declarations"], q.decls, 0, ord);
            }
            if (m.contains("styles")) {
                StyleSet tmp; parseRules(m["styles"], tmp);
                q.rules = std::move(tmp.rules);
                for (auto& rr : q.rules) { rr.order += 100000; for (auto& dd : rr.decls) dd.order += 100000; }
            }
            for (auto& dd : q.decls) dd.order += 100000;
            m_base.media.push_back(std::move(q));
        }
    }
    if (theme.contains("containerQueries") && theme["containerQueries"].is_array()) {
        for (const auto& c : theme["containerQueries"]) {
            if (!c.is_object()) continue;
            ContainerQuery q; q.id = c.value("id", ""); q.container = c.value("container", "");
            if (c.contains("query") && c["query"].is_object()) {
                q.minW = c["query"].value("min-width", 0.0f); q.maxW = c["query"].value("max-width", 0.0f);
                q.minH = c["query"].value("min-height", 0.0f); q.maxH = c["query"].value("max-height", 0.0f);
            }
            if (c.contains("variables") && c["variables"].is_object()) {
                for (auto it = c["variables"].begin(); it != c["variables"].end(); ++it)
                    if (it.value().is_string()) q.vars[it.key()] = it.value().get<string>();
            }
            if (c.contains("declarations") && c["declarations"].is_object()) {
                int ord = 0; collectDecls(c["declarations"], q.decls, 0, ord);
            }
            if (c.contains("styles")) {
                StyleSet tmp; parseRules(c["styles"], tmp);
                q.rules = std::move(tmp.rules);
                for (auto& rr : q.rules) { rr.order += 200000; for (auto& dd : rr.decls) dd.order += 200000; }
            }
            for (auto& dd : q.decls) dd.order += 200000;
            m_base.containers.push_back(std::move(q));
        }
    }
    if (theme.contains("variants") && theme["variants"].is_object()) {
        for (auto it = theme["variants"].begin(); it != theme["variants"].end(); ++it) {
            StyleSet s;
            if (it.value().is_object()) {
                if (it.value().contains("variables") && it.value()["variables"].is_object())
                    for (auto vit = it.value()["variables"].begin(); vit != it.value()["variables"].end(); ++vit)
                        if (vit.value().is_string()) s.vars[vit.key()] = vit.value().get<string>();
                if (it.value().contains("styles")) parseRules(it.value()["styles"], s);
                if (it.value().contains("declarations") && it.value()["declarations"].is_object()) {
                    Rule r; r.type = "*"; int ord = 0; collectDecls(it.value()["declarations"], r.decls, 1, ord);
                    s.rules.push_back(std::move(r));
                }
                if (it.value().contains("media") && it.value()["media"].is_array()) {
                    for (const auto& m : it.value()["media"]) {
                        if (!m.is_object()) continue;
                        MediaQuery q; q.id = m.value("id", "");
                        if (m.contains("query") && m["query"].is_object()) {
                            q.minW = m["query"].value("min-width", 0.0f); q.maxW = m["query"].value("max-width", 0.0f);
                            q.minH = m["query"].value("min-height", 0.0f); q.maxH = m["query"].value("max-height", 0.0f);
                        }
                        if (m.contains("variables") && m["variables"].is_object()) for (auto vit = m["variables"].begin(); vit != m["variables"].end(); ++vit) if (vit.value().is_string()) q.vars[vit.key()] = vit.value().get<string>();
                        if (m.contains("declarations") && m["declarations"].is_object()) { int ord = 0; collectDecls(m["declarations"], q.decls, 4000, ord); }
                        if (m.contains("styles")) { StyleSet tmp; parseRules(m["styles"], tmp); q.rules = std::move(tmp.rules); for (auto& rr : q.rules) { rr.order += 400000; for (auto& dd : rr.decls) dd.order += 400000; } }
                        s.media.push_back(std::move(q));
                    }
                }
                if (it.value().contains("containerQueries") && it.value()["containerQueries"].is_array()) {
                    for (const auto& c : it.value()["containerQueries"]) {
                        if (!c.is_object()) continue;
                        ContainerQuery q; q.id = c.value("id", ""); q.container = c.value("container", "");
                        if (c.contains("query") && c["query"].is_object()) {
                            q.minW = c["query"].value("min-width", 0.0f); q.maxW = c["query"].value("max-width", 0.0f);
                            q.minH = c["query"].value("min-height", 0.0f); q.maxH = c["query"].value("max-height", 0.0f);
                        }
                        if (c.contains("variables") && c["variables"].is_object()) for (auto vit = c["variables"].begin(); vit != c["variables"].end(); ++vit) if (vit.value().is_string()) q.vars[vit.key()] = vit.value().get<string>();
                        if (c.contains("declarations") && c["declarations"].is_object()) { int ord = 0; collectDecls(c["declarations"], q.decls, 5000, ord); }
                        if (c.contains("styles")) { StyleSet tmp; parseRules(c["styles"], tmp); q.rules = std::move(tmp.rules); for (auto& rr : q.rules) { rr.order += 500000; for (auto& dd : rr.decls) dd.order += 500000; } }
                        s.containers.push_back(std::move(q));
                    }
                }
            }
            for (auto& rr : s.rules) { rr.order += 300000; for (auto& dd : rr.decls) dd.order += 300000; }
            m_variants[it.key()] = std::move(s);
        }
    }
    return true;
}

void ThemeStyleSheet::parseStyleSet(const json&, StyleSet&) {}

void ThemeStyleSheet::parseRules(const json& arr, StyleSet& out) {
    if (!arr.is_array()) return;
    int ord = (int)out.rules.size();
    for (const auto& st : arr) {
        if (!st.is_object()) continue;
        string sel = trim(st.value("selector", ""));
        if (sel.empty() || !st.contains("declarations") || !st["declarations"].is_object()) continue;
        Rule r; r.order = ord++;
        size_t pos = 0;
        while (pos < sel.size()) {
            size_t sp = sel.find_first_of(" \t", pos);
            string tok = sel.substr(pos, sp == string::npos ? string::npos : sp - pos);
            pos = (sp == string::npos) ? sel.size() : sp + 1;
            if (tok.empty()) continue;
            size_t colon = tok.find(':');
            if (colon != string::npos) { r.state = lower(tok.substr(colon + 1)); tok = tok.substr(0, colon); }
            size_t h = tok.find('#');
            if (h != string::npos) { r.id = tok.substr(h + 1); tok = tok.substr(0, h); r.specificity += 100; }
            size_t d = 0;
            while ((d = tok.find('.', d)) != string::npos) {
                size_t e = tok.find('.', d + 1);
                r.classes.push_back(tok.substr(d + 1, (e == string::npos ? tok.size() : e) - d - 1));
                r.specificity += 10;
                if (e == string::npos) break;
                d = e;
            }
            size_t dot = tok.find('.');
            string type = dot == string::npos ? tok : tok.substr(0, dot);
            if (!type.empty()) { r.type = lower(type); r.specificity += 1; }
        }
        collectDecls(st["declarations"], r.decls, r.specificity, r.order);
        out.rules.push_back(std::move(r));
    }
}

void ThemeStyleSheet::collectDecls(const json& obj, std::vector<Decl>& out, int spec, int& order) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        Decl d; d.key = it.key(); d.specificity = spec; d.order = order++;
        if (it.value().is_string()) d.value = it.value().get<string>();
        else if (it.value().is_number()) d.value = to_string(it.value().get<double>());
        else if (it.value().is_boolean()) d.value = it.value().get<bool>() ? "1" : "0";
        else d.value = it.value().dump();
        out.push_back(std::move(d));
    }
}

const ThemeStyleSheet::StyleSet* ThemeStyleSheet::variant(const std::string& name) const {
    auto it = m_variants.find(name);
    return it == m_variants.end() ? nullptr : &it->second;
}

std::vector<std::string> ThemeStyleSheet::variantNames() const {
    std::vector<std::string> v; for (auto& kv : m_variants) v.push_back(kv.first); return v;
}

std::string ThemeStyleSheet::resolveVar(const std::string& v) const {
    string s = trim(v);
    for (int depth = 0; depth < 16; ++depth) {
        if (s.rfind("var(", 0) != 0) break;
        size_t close = s.find(')');
        if (close == string::npos) break;
        string name = trim(s.substr(4, close - 4));
        if (name.size() > 2 && name[0] == '-' && name[1] == '-') {
            auto it = m_vars.find(name);
            if (it == m_vars.end()) break;
            s = it->second + s.substr(close + 1);
        } else break;
    }
    return s;
}

std::string ThemeRuntime::resolveActive(const std::string& v) const {
    string s = trim(v);
    for (int depth = 0; depth < 16; ++depth) {
        if (s.rfind("var(", 0) != 0) break;
        size_t close = s.find(')');
        if (close == string::npos) break;
        string name = trim(s.substr(4, close - 4));
        if (name.size() > 2 && name[0] == '-' && name[1] == '-') {
            auto vit = m_controlVars.find(name);
            if (vit != m_controlVars.end()) { s = vit->second + s.substr(close + 1); continue; }
            auto it = m_sheet.variables().find(name);
            if (it == m_sheet.variables().end()) break;
            s = it->second + s.substr(close + 1);
        } else break;
    }
    return s;
}

// ==================== ThemeRuntime ====================

void ThemeRuntime::setTheme(const json& themeJson) {
    m_raw = themeJson;
    m_legacy = std::make_shared<Theme>();
    if (themeJson.is_object()) m_legacy->parse(themeJson);
    m_sheet.parse(themeJson);
    m_variant.clear();
    m_activeMedia.clear();
}

void ThemeRuntime::clear() {
    m_raw = json(); m_legacy.reset(); m_sheet = ThemeStyleSheet();
    m_variant.clear(); m_activeMedia.clear();
    // 不清理 m_explicit：LayoutParser 无 theme 时调用 clear()，已有控件的
    // JSON 显式值仍须在后续 ApplyTheme 时保留（B1）。
    m_cssFontName.clear(); m_cssFontSize.clear(); m_cssTextNormal.clear();
}

bool ThemeRuntime::jsonHasLegacyData(const json& j) {
    if (!j.is_object()) return false;
    auto nonEmpty = [](const json& v) { return v.is_object() && !v.empty(); };
    return (j.contains("fonts") && nonEmpty(j["fonts"])) ||
           (j.contains("colors") && nonEmpty(j["colors"]));
}

bool ThemeRuntime::jsonHasThemableData(const json& j) {
    if (!j.is_object()) return false;
    if (j.contains("styles") && j["styles"].is_array() && !j["styles"].empty()) return true;
    for (const char* k : {"media", "containerQueries"})
        if (j.contains(k) && j[k].is_array() && !j[k].empty()) return true;
    if (j.contains("variants") && j["variants"].is_object()) {
        for (auto it = j["variants"].begin(); it != j["variants"].end(); ++it) {
            if (!it.value().is_object()) continue;
            if (it.value().contains("styles") && it.value()["styles"].is_array() && !it.value()["styles"].empty()) return true;
            if (it.value().contains("media") && it.value()["media"].is_array() && !it.value()["media"].empty()) return true;
            if (it.value().contains("containerQueries") && it.value()["containerQueries"].is_array() && !it.value()["containerQueries"].empty()) return true;
        }
    }
    return jsonHasLegacyData(j);
}

bool ThemeRuntime::hasLegacyData() const { return jsonHasLegacyData(m_raw); }
bool ThemeRuntime::hasThemableData() const { return jsonHasThemableData(m_raw); }

bool ThemeRuntime::hasResponsiveData() const {
    if (!m_raw.is_object()) return false;
    for (const char* k : {"media", "containerQueries"})
        if (m_raw.contains(k) && m_raw[k].is_array() && !m_raw[k].empty()) return true;
    if (m_raw.contains("variants") && m_raw["variants"].is_object())
        for (auto it = m_raw["variants"].begin(); it != m_raw["variants"].end(); ++it)
            if (it.value().is_object())
                for (const char* k : {"media", "containerQueries"})
                    if (it.value().contains(k) && it.value()[k].is_array() && !it.value()[k].empty()) return true;
    return false;
}

const ThemeStyleSheet::StyleSet* ThemeRuntime::activeSet() const { return m_sheet.base(); }

bool ThemeRuntime::matchRule(const ThemeStyleSheet::Rule& r, Control* ctl, const std::string& type,
                             const std::vector<std::string>& classes, const std::string& id) const {
    (void)ctl;
    if (!r.type.empty() && r.type != "*" && r.type != type) return false;
    if (!r.id.empty() && r.id != id) return false;
    for (const auto& c : r.classes) if (std::find(classes.begin(), classes.end(), c) == classes.end()) return false;
    return true;
}

bool ThemeRuntime::matchContainerSelector(const std::string& sel, Control* ctl) const {
    if (sel.empty() || sel == "*") return true;
    string s = trim(sel);
    size_t h = s.find('#'); string id = h == string::npos ? "" : s.substr(h + 1);
    size_t d = s.find('.'); string cls = d == string::npos ? "" : s.substr(d + 1, (h == string::npos || h > d ? string::npos : h - d - 1));
    string type = s.substr(0, std::min(d == string::npos ? s.size() : d, h == string::npos ? s.size() : h));
    if (!id.empty()) {
        const char* cur = nullptr;
        if (!ctl->getStringProperty(PropertyNames::kId, cur) || !cur || id != cur) return false;
    }
    if (!cls.empty()) {
        auto* impl = dynamic_cast<ControlImpl*>(ctl);
        if (!impl) return false;
        const auto& cs = impl->getThemeClasses();
        if (std::find(cs.begin(), cs.end(), cls) == cs.end()) return false;
    }
    if (!type.empty() && type != "*") {
        const char* cur = nullptr;
        if (!ctl->getStringProperty(PropertyNames::kControlType, cur) || !cur || lower(type) != cur) return false;
    }
    return true;
}

void ThemeRuntime::collectKeys(Control* ctl, const std::vector<ThemeStyleSheet::Decl>& decls) {
    for (const auto& d : decls) {
        if (d.key == "font-family") m_cssFontName.insert(ctl);
        else if (d.key == "font-size") m_cssFontSize.insert(ctl);
        else if (d.key == "text-color") m_cssTextNormal.insert(ctl);
    }
}

void ThemeRuntime::applyAll(Control* root) {
    applyAll(root, m_benchActive);
}

void ThemeRuntime::applyAll(Control* root, const ActiveConditions& active) {
    if (!root || !hasThemableData()) return;
    m_cssFontName.clear(); m_cssFontSize.clear(); m_cssTextNormal.clear();
    applySet(root, m_sheet.base(), true, active);
    if (auto* v = m_sheet.variant(m_variant)) applySet(root, v, true, active);
    inheritPass(root, false, {}, false, SColor());
}

void ThemeRuntime::applySet(Control* root, const ThemeStyleSheet::StyleSet* set, bool includeResponsive, const ActiveConditions& active) {
    if (!root || !set) return;
    std::function<void(Control*)> walk = [&](Control* c) {
        auto* impl = dynamic_cast<ControlImpl*>(c);
        if (!impl || !impl->isThemeEligible()) return;
        applyOne(c, set, includeResponsive, {}, active);
        for (auto& ch : impl->getChildren()) walk(ch.get());
    };
    walk(root);
}

void ThemeRuntime::applyOne(Control* ctl, const ThemeStyleSheet::StyleSet* set, bool includeResponsive,
                            const std::unordered_map<std::string, std::string>&, const ActiveConditions& active) {
    auto* impl = dynamic_cast<ControlImpl*>(ctl);
    if (!impl || !impl->isThemeEligible()) return;
    const char* typeP = nullptr; string type = ctl->getStringProperty(PropertyNames::kControlType, typeP) && typeP ? typeP : "";
    const char* idP = nullptr; string id = ctl->getStringProperty(PropertyNames::kId, idP) && idP ? idP : "";
    const auto& classes = impl->getThemeClasses();

    // 旧 fonts/colors 主题为底层基线（ApplyTheme 重建），CSS/显式随后覆盖
    if (m_legacy && hasLegacyData() && !impl->getThemeCategory().empty()) {
        m_legacy->applyCommonColors(std::dynamic_pointer_cast<ControlImpl>(ctl->getThis()), impl->getThemeCategory());
        int fs = m_legacy->getFontSize(impl->getThemeCategory());
        FontName fn = m_legacy->getFontName(impl->getThemeCategory());
        applyFontToControl(ctl, FontNameToString(fn), fs);
    }
    std::unordered_map<string, ThemeStyleSheet::Decl> merged;
    std::vector<ThemeStyleSheet::Decl> mergedList;
    auto mergeRules = [&](const std::vector<ThemeStyleSheet::Rule>& rules) {
        for (const auto& r : rules) {
            bool mr = matchRule(r, ctl, type, classes, id);
            if (!mr) continue;
            for (auto d : r.decls) {
                if (!r.state.empty() && isColorKey(d.key)) d.key = propForColorKey(d.key) + stateSuffix(r.state);
                auto it = merged.find(d.key);
                if (it == merged.end() || d.specificity > it->second.specificity ||
                    (d.specificity == it->second.specificity && d.order >= it->second.order)) merged[d.key] = d;
            }
        }
    };
    mergeRules(set->rules);
    if (includeResponsive) {
        for (const auto& q : set->media) {
            const std::string& mids = active.mediaIds;
            bool on = (mids == q.id) || (mids.rfind(q.id + ",", 0) == 0) ||
                      (mids.find("," + q.id + ",") != string::npos) ||
                      (mids.size() >= q.id.size() && mids.compare(mids.size() - q.id.size(), q.id.size(), q.id) == 0);
            if (!on) continue;
            for (auto d : q.decls) merged[d.key] = d;
            mergeRules(q.rules);
        }
        for (const auto& q : set->containers) {
            bool on = (active.containerIds.find(q.id) != string::npos);
            if (!on) continue;
            for (auto d : q.decls) merged[d.key] = d;
            mergeRules(q.rules);
        }
    }
    m_controlVars.clear();
    for (auto& vv : set->vars) m_controlVars[vv.first] = vv.second;
    if (includeResponsive) {
        for (const auto& q : set->media) {
            const std::string& mids = active.mediaIds;
            if (mids == q.id || mids.rfind(q.id + ",", 0) == 0 ||
                mids.find("," + q.id + ",") != string::npos ||
                (mids.size() >= q.id.size() && mids.compare(mids.size() - q.id.size(), q.id.size(), q.id) == 0))
                for (auto& vv : q.vars) m_controlVars[vv.first] = vv.second;
        }
        for (const auto& q : set->containers) if (active.containerIds.find(q.id) != string::npos)
            for (auto& vv : q.vars) m_controlVars[vv.first] = vv.second;
    }
    for (auto& kv : merged) { applyValue(ctl, kv.first, resolveActive(kv.second.value)); mergedList.push_back(kv.second); }
    collectKeys(ctl, mergedList);
    replayExplicit(ctl);
}

void ThemeRuntime::applyValue(Control* ctl, const std::string& rawKey, const std::string& value) {
    string key = rawKey;
    if (key == "background") key = PropertyNames::kBackground;
    else if (key == "border-color") key = PropertyNames::kBorder;
    else if (key == "text-color") key = PropertyNames::kText;
    else if (key == "text-shadow") key = PropertyNames::kTextShadow;
    if (key == "border-radius") key = PropertyNames::kCornerRadius;
    if (key == "padding" || key == PropertyNames::kContentPadding) key = PropertyNames::kContentPadding;
    if (key == "font-family") key = PropertyNames::kFont;
    if (key == "font-size") key = PropertyNames::kFontSize;

    if (key == "box-shadow") {
        try {
            json sh = json::parse(value);
            if (sh.is_object()) {
                SColor c;
                std::string cs = sh.value("color", "");
                if (!cs.empty() && parseColorValue(cs, c)) ctl->setColorProperty(PropertyNames::kBoxShadowColor, c);
                if (sh.contains("offset-x")) ctl->setFloatProperty(PropertyNames::kBoxShadowOffsetX, sh["offset-x"].get<float>());
                if (sh.contains("offset-y")) ctl->setFloatProperty(PropertyNames::kBoxShadowOffsetY, sh["offset-y"].get<float>());
                if (sh.contains("blur")) ctl->setFloatProperty(PropertyNames::kBoxShadowRadius, sh["blur"].get<float>());
                if (sh.contains("spread")) ctl->setFloatProperty(PropertyNames::kBoxShadowSpread, sh["spread"].get<float>());
            }
        } catch (...) {}
        return;
    }
    if (key == PropertyNames::kBackground || key == PropertyNames::kBorder ||
        key == PropertyNames::kText || key == PropertyNames::kTextShadow ||
        key == "box-shadow-color") {
        SColor c;
        if (parseColorValue(value, c)) ctl->setColorProperty(key.c_str(), c);
        return;
    }
    if (key.find(".hover") != string::npos || key.find(".pressed") != string::npos || key.find(".disabled") != string::npos) {
        SColor c;
        if (parseColorValue(value, c)) ctl->setColorProperty(key.c_str(), c);
        return;
    }
    if (key == PropertyNames::kFont) {
        string v = value; if (v.size() >= 2 && v.front() == '"') v = v.substr(1, v.size()-2);
        applyFontToControl(ctl, v, 0);
        return;
    }
    if (key == PropertyNames::kFontSize) {
        applyFontToControl(ctl, "", atoi(value.c_str()));
        return;
    }
    if (key == PropertyNames::kFontStyle) {
        if (auto* label = dynamic_cast<Label*>(ctl)) label->setStringProperty(PropertyNames::kFontStyle, value.c_str());
        return;
    }
    if (key == PropertyNames::kContentPadding) {
        vector<float> v; if (!splitFloatList(value, v)) return;
        if (v.size() == 1) ctl->setFloatProperty(PropertyNames::kContentPadding, v[0]);
        else if (v.size() == 2) ctl->setStringProperty(PropertyNames::kContentPadding, (to_string(v[0]) + " " + to_string(v[1])).c_str());
        else if (v.size() == 4) ctl->setStringProperty(PropertyNames::kContentPadding, (to_string(v[0])+" "+to_string(v[1])+" "+to_string(v[2])+" "+to_string(v[3])).c_str());
        return;
    }
    if (key == PropertyNames::kCornerRadius) { ctl->setFloatProperty(key.c_str(), (float)atof(value.c_str())); return; }
    if (key == PropertyNames::kShadowOffsetX || key == PropertyNames::kShadowOffsetY) {
        ctl->setFloatProperty(key.c_str(), (float)atof(value.c_str())); return;
    }
    if (key == "box-shadow-offset-x" || key == "box-shadow-offset-y" || key == "box-shadow-radius") {
        ctl->setFloatProperty(key.c_str(), (float)atof(value.c_str())); return;
    }
    SColor c;
    if (parseColorValue(value, c)) { if (ctl->setColorProperty(key.c_str(), c)) return; }
    char* end = nullptr; double d = strtod(value.c_str(), &end);
    if (end && *end == '\0') {
        if (ctl->setFloatProperty(key.c_str(), (float)d)) return;
        if (ctl->setIntProperty(key.c_str(), (int)d)) return;
    }
    ctl->setStringProperty(key.c_str(), value.c_str());
}

void ThemeRuntime::inheritPass(Control* ctl, bool parentEligible,
                               const std::unordered_map<std::string, std::string>& parentFont,
                               bool parentTextValid, SColor parentText) {
    auto* impl = dynamic_cast<ControlImpl*>(ctl);
    if (!impl || !impl->isThemeEligible()) return;
    auto expIt = m_explicit.find(ctl);
    bool hasExp = expIt != m_explicit.end();
    bool fontNameOwn = m_cssFontName.count(ctl) || (hasExp && expIt->second.contains(PropertyNames::kFont));
    bool fontSizeOwn = m_cssFontSize.count(ctl) || (hasExp && expIt->second.contains(PropertyNames::kFontSize));
    if (parentEligible) {
        if (!fontNameOwn && parentFont.count("name")) applyFontToControl(ctl, parentFont.at("name"), 0);
        if (!fontSizeOwn && parentFont.count("size")) applyFontToControl(ctl, "", atoi(parentFont.at("size").c_str()));
        bool textOwn = m_cssTextNormal.count(ctl) || (hasExp && expIt->second.contains(PropertyNames::kText));
        if (!textOwn && parentTextValid) ctl->setColorProperty(PropertyNames::kText, parentText);
    }
    std::unordered_map<std::string, std::string> font;
    if (!fontNameOwn && parentFont.count("name")) font["name"] = parentFont.at("name");
    if (!fontSizeOwn && parentFont.count("size")) font["size"] = parentFont.at("size");
    if (fontNameOwn) { const char* s=nullptr; if (ctl->getStringProperty(PropertyNames::kFont, s) && s) font["name"] = s; }
    if (fontSizeOwn) { float fs=0; if (ctl->getFloatProperty(PropertyNames::kFontSize, fs)) font["size"] = to_string((int)fs); }
    bool textValid = m_cssTextNormal.count(ctl) || (hasExp && expIt->second.contains(PropertyNames::kText)) || parentTextValid;
    SColor text = parentText;
    if (m_cssTextNormal.count(ctl) || (hasExp && expIt->second.contains(PropertyNames::kText))) {
        SColor c; if (ctl->getColorProperty(PropertyNames::kText, c)) text = c;
    }
    for (auto& ch : impl->getChildren()) inheritPass(ch.get(), true, font, textValid, text);
}

void ThemeRuntime::applyControl(Control* ctl) {
    applyControl(ctl, m_benchActive);
}

void ThemeRuntime::applyControl(Control* ctl, const ActiveConditions& active) {
    if (!ctl || !hasThemableData()) return;
    if (auto* set = m_sheet.base()) applyOne(ctl, set, true, {}, active);
    if (auto* v = m_sheet.variant(m_variant)) applyOne(ctl, v, true, {}, active);
    replayExplicit(ctl);
}

void ThemeRuntime::setVariant(Control* root, const std::string& name) {
    (void)root;
    if (!m_sheet.variant(name) && !name.empty() && name != "default") return;
    m_variant = (name == "default") ? "" : name;
}

void ThemeRuntime::captureExplicitFromJson(Control* ctl, const json& j) {
    if (!ctl || !j.is_object()) return;
    json& e = m_explicit[ctl];
    auto colorToStr = [](SColor c) {
        char b[16]; snprintf(b, sizeof(b), "#%02X%02X%02X%02X", (int)(c.red()*255), (int)(c.green()*255), (int)(c.blue()*255), (int)(c.alpha()*255)); return string(b);
    };
    if (j.contains("colors") && j["colors"].is_object()) {
        auto scanGroup = [&](const char* group, const char* prop) {
            if (!j["colors"].contains(group)) return;
            const json& g = j["colors"][group];
            if (g.is_string()) { SColor c; if (parseColorValue(g.get<string>(), c)) e[prop] = colorToStr(c); }
            else if (g.is_object()) {
                const char* states[] = {"normal","hover","pressed","disabled"};
                const char* suffix[] = {"",".hover",".pressed",".disabled"};
                for (int i=0;i<4;++i) if (g.contains(states[i]) && g[states[i]].is_string()) {
                    SColor c; if (parseColorValue(g[states[i]].get<string>(), c)) e[string(prop)+suffix[i]] = colorToStr(c);
                }
            }
        };
        scanGroup("background", PropertyNames::kBackground);
        scanGroup("border", PropertyNames::kBorder);
        scanGroup("text", PropertyNames::kText);
        scanGroup("text-shadow", PropertyNames::kTextShadow);
    }
    const char* direct[] = {"background","border","text","text-shadow"};
    const char* directProp[] = {PropertyNames::kBackground, PropertyNames::kBorder, PropertyNames::kText, PropertyNames::kTextShadow};
    for (int i=0;i<4;++i) if (j.contains(direct[i]) && j[direct[i]].is_string()) {
        SColor c; if (parseColorValue(j[direct[i]].get<string>(), c)) e[directProp[i]] = colorToStr(c);
    }
    if (j.contains(PropertyNames::kFont) && j[PropertyNames::kFont].is_object()) {
        const json& f = j[PropertyNames::kFont];
        if (f.contains("name") && f["name"].is_string()) e[PropertyNames::kFont] = f["name"].get<string>();
        if (f.contains("size") && f["size"].is_number()) e[PropertyNames::kFontSize] = to_string(f["size"].get<int>());
    }
    if (j.contains(PropertyNames::kFontSize) && j[PropertyNames::kFontSize].is_number()) e[PropertyNames::kFontSize] = to_string(j[PropertyNames::kFontSize].get<int>());
    if (j.contains(PropertyNames::kFontStyle) && j[PropertyNames::kFontStyle].is_string()) e[PropertyNames::kFontStyle] = j[PropertyNames::kFontStyle].get<string>();
    if (j.contains(PropertyNames::kContentPadding)) {
        const json& p = j[PropertyNames::kContentPadding];
        if (p.is_number()) e[PropertyNames::kContentPadding] = to_string(p.get<float>());
        else if (p.is_string()) e[PropertyNames::kContentPadding] = p.get<string>();
        else if (p.is_object()) {
            float l=p.value("left",0.f), t=p.value("top",0.f), r=p.value("right",0.f), b=p.value("bottom",0.f);
            e[PropertyNames::kContentPadding] = to_string(l)+" "+to_string(t)+" "+to_string(r)+" "+to_string(b);
        }
    }
    if (j.contains(PropertyNames::kCornerRadius) && j[PropertyNames::kCornerRadius].is_number()) e[PropertyNames::kCornerRadius] = to_string(j[PropertyNames::kCornerRadius].get<float>());
    if (j.contains("shadow") && j["shadow"].is_object()) {
        const json& sh = j["shadow"];
        if (sh.contains("offset") && sh["offset"].is_object()) {
            if (sh["offset"].contains("x")) e[PropertyNames::kShadowOffsetX] = to_string(sh["offset"]["x"].get<float>());
            if (sh["offset"].contains("y")) e[PropertyNames::kShadowOffsetY] = to_string(sh["offset"]["y"].get<float>());
        }
    }
}

void ThemeRuntime::recordExplicitColor(Control* ctl, const char* prop, SColor c) {
    if (!ctl || !prop) return;
    char b[16]; snprintf(b, sizeof(b), "#%02X%02X%02X%02X", (int)(c.red()*255), (int)(c.green()*255), (int)(c.blue()*255), (int)(c.alpha()*255));
    m_explicit[ctl][prop] = string(b);
}

void ThemeRuntime::recordExplicitNumber(Control* ctl, const char* prop, double v) {
    if (ctl && prop) m_explicit[ctl][prop] = v;
}

void ThemeRuntime::recordExplicitString(Control* ctl, const char* prop, const std::string& v) {
    if (ctl && prop) m_explicit[ctl][prop] = v;
}

void ThemeRuntime::clearOverrides(Control* ctl) {
    if (!ctl) return;
    auto* impl = dynamic_cast<ControlImpl*>(ctl);
    if (impl) {
        m_explicit.erase(ctl);
        for (auto& ch : impl->getChildren()) clearOverrides(ch.get());
    }
}

static bool sameColor(const SColor& a, const SColor& b) {
    const float eps = 0.5f / 255.0f;
    return std::abs(a.red()-b.red()) <= eps && std::abs(a.green()-b.green()) <= eps &&
           std::abs(a.blue()-b.blue()) <= eps && std::abs(a.alpha()-b.alpha()) <= eps;
}

bool ThemeRuntime::explicitValueMatches(Control* ctl, const std::string& key, const json& v) {
    if (!ctl || !v.is_string()) return false;
    const std::string value = v.get<std::string>();
    if (key == PropertyNames::kFont) {
        const char* cur = nullptr;
        return ctl->getStringProperty(PropertyNames::kFont, cur) && cur && lower(cur) == lower(value);
    }
    if (key == PropertyNames::kFontSize) {
        int cur = 0;
        return ctl->getIntProperty(PropertyNames::kFontSize, cur) && cur == atoi(value.c_str());
    }
    if (key == PropertyNames::kFontStyle) {
        const char* cur = nullptr;
        return ctl->getStringProperty(PropertyNames::kFontStyle, cur) && cur && lower(cur) == lower(value);
    }
    if (key == PropertyNames::kContentPadding || key == PropertyNames::kPadding) {
        const char* cur = nullptr;
        if (!ctl->getStringProperty(PropertyNames::kContentPadding, cur) || !cur) return false;
        vector<float> a, b;
        if (!splitFloatList(value, a) || !splitFloatList(cur, b)) return false;
        auto expand = [](vector<float>& v) {
            if (v.size() == 1) v = {v[0],v[0],v[0],v[0]};
            else if (v.size() == 2) v = {v[1],v[0],v[1],v[0]};
            return v.size() == 4;
        };
        return expand(a) && expand(b) && a == b;
    }
    if (key == PropertyNames::kCornerRadius || key == PropertyNames::kShadowOffsetX || key == PropertyNames::kShadowOffsetY) {
        float cur = 0;
        return ctl->getFloatProperty(key.c_str(), cur) && std::abs(cur - (float)atof(value.c_str())) < 0.01f;
    }
    SColor color;
    if (parseColorValue(value, color)) {
        SColor cur;
        return ctl->getColorProperty(key.c_str(), cur) && sameColor(cur, color);
    }
    return false;
}

void ThemeRuntime::replayExplicit(Control* ctl) {
    if (!ctl) return;
    auto it = m_explicit.find(ctl);
    if (it == m_explicit.end()) return;
    for (auto& kv : it->second.items()) {
        const string& key = kv.key();
        const json& v = kv.value();
        if (explicitValueMatches(ctl, key, v)) continue;
        if (key == PropertyNames::kFont && v.is_string()) {
            applyFontToControl(ctl, v.get<string>(), 0);
        } else if (key == PropertyNames::kFontSize && v.is_string()) {
            applyFontToControl(ctl, "", atoi(v.get<string>().c_str()));
        } else if (key == PropertyNames::kFontStyle && v.is_string()) {
            if (auto* label = dynamic_cast<Label*>(ctl)) label->setStringProperty(PropertyNames::kFontStyle, v.get<string>().c_str());
        } else if (key == PropertyNames::kContentPadding && v.is_string()) {
            ctl->setStringProperty(key.c_str(), v.get<string>().c_str());
        } else if (v.is_string()) {
            SColor c;
            if (parseColorValue(v.get<string>(), c)) ctl->setColorProperty(key.c_str(), c);
            else ctl->setStringProperty(key.c_str(), v.get<string>().c_str());
        } else if (v.is_number()) {
            if (ctl->setFloatProperty(key.c_str(), v.get<float>())) {}
            else ctl->setIntProperty(key.c_str(), (int)v.get<double>());
        } else if (v.is_boolean()) ctl->setBoolProperty(key.c_str(), v.get<bool>() ? 1 : 0);
    }
}

ThemeRuntime::ActiveConditions ThemeRuntime::evaluateConditions(Control* root, float viewportW, float viewportH, float viewportScale) const {
    ActiveConditions out;
    auto append = [](std::string& dst, const std::string& id) {
        if (id.empty()) return;
        if (dst == id || dst.find(id + ",") == 0 || dst.find("," + id + ",") != string::npos ||
            (dst.size() >= id.size() && dst.compare(dst.size() - id.size(), id.size(), id) == 0)) return;
        if (!dst.empty()) dst += ",";
        dst += id;
    };
    auto addMedia = [&](const ThemeStyleSheet::StyleSet& set) {
        for (const auto& q : set.media) {
            bool ok = true;
            if (q.minW > 0 && viewportW < q.minW) ok = false;
            if (q.maxW > 0 && viewportW > q.maxW) ok = false;
            if (q.minH > 0 && viewportH < q.minH) ok = false;
            if (q.maxH > 0 && viewportH > q.maxH) ok = false;
            if (q.minScale > 0 && viewportScale < q.minScale) ok = false;
            if (q.maxScale > 0 && viewportScale > q.maxScale) ok = false;
            if (!q.orientation.empty()) ok = ok && ((q.orientation == "landscape") == (viewportW >= viewportH));
            if (ok) append(out.mediaIds, q.id);
        }
    };
    addMedia(*m_sheet.base());
    const ThemeStyleSheet::StyleSet* variantSet = m_sheet.variant(m_variant);
    if (variantSet) addMedia(*variantSet);
    if (root) {
        std::function<void(Control*)> scanContainers = [&](Control* c) {
            auto* impl = dynamic_cast<ControlImpl*>(c);
            if (!impl || !impl->isThemeEligible()) return;
            auto scanSet = [&](const ThemeStyleSheet::StyleSet& set) {
                for (const auto& q : set.containers) {
                    SRect cr; bool found = false;
                    for (Control* p = impl->getParent(); p; p = p->getParent()) {
                        if (matchContainerSelector(q.container, p)) {
                            auto* pi = dynamic_cast<ControlImpl*>(p);
                            if (pi) cr = pi->getContentRect();
                            found = true; break;
                        }
                    }
                    if (!found) continue;
                    bool ok = true;
                    if (q.minW > 0 && cr.width < q.minW) ok = false;
                    if (q.maxW > 0 && cr.width > q.maxW) ok = false;
                    if (q.minH > 0 && cr.height < q.minH) ok = false;
                    if (q.maxH > 0 && cr.height > q.maxH) ok = false;
                    if (ok) append(out.containerIds, q.id);
                }
            };
            scanSet(*m_sheet.base());
            if (variantSet) scanSet(*variantSet);
            for (auto& ch : impl->getChildren()) scanContainers(ch.get());
        };
        scanContainers(root);
    }
    return out;
}

void ThemeRuntime::refreshResponsive(Control* root, float viewportW, float viewportH, float viewportScale) {
    m_lastViewportW = viewportW; m_lastViewportH = viewportH;
    if (!root) return;
    auto active = evaluateConditions(root, viewportW, viewportH, viewportScale);
    m_benchActive = active;
    m_activeMedia = active.mediaIds;
    if (!active.containerIds.empty()) { if (!m_activeMedia.empty()) m_activeMedia += ","; m_activeMedia += active.containerIds; }
    applyAll(root, active);
    m_responsiveDirty = false;
}
