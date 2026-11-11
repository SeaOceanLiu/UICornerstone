#ifndef ThemeStyleH
#define ThemeStyleH

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <nlohmann/json.hpp>
#include "SColor.h"
#include "Utility.h"

class Control;
class ControlImpl;
class Theme;
using json = nlohmann::json;

// CR20261102001/003：CSS-like 主题样式表 + 运行时层（选择器/变量/状态/变体/媒体/容器查询）。
class ThemeStyleSheet {
public:
    struct Decl { std::string key; std::string value; int specificity = 0; int order = 0; };
    struct Rule {
        std::string type;
        std::vector<std::string> classes;
        std::string id;
        std::string state;
        std::vector<Decl> decls;
        int specificity = 0;
        int order = 0;
    };
    struct MediaQuery {
        std::string id;
        float minW = 0, maxW = 0, minH = 0, maxH = 0;
        float minScale = 0, maxScale = 0;
        std::string orientation;
        std::unordered_map<std::string, std::string> vars;
        std::vector<Decl> decls;
        std::vector<Rule> rules;
    };
    struct ContainerQuery {
        std::string id;
        std::string container;
        float minW = 0, maxW = 0, minH = 0, maxH = 0;
        std::unordered_map<std::string, std::string> vars;
        std::vector<Decl> decls;
        std::vector<Rule> rules;
    };
    struct StyleSet {
        std::unordered_map<std::string, std::string> vars;
        std::vector<Rule> rules;
        std::vector<MediaQuery> media;
        std::vector<ContainerQuery> containers;
    };

    bool parse(const json& theme);
    bool empty() const { return m_base.rules.empty() && m_variants.empty(); }
    const StyleSet* base() const { return &m_base; }
    const StyleSet* variant(const std::string& name) const;
    std::vector<std::string> variantNames() const;
    std::string resolveVar(const std::string& v) const;
    const std::unordered_map<std::string, std::string>& variables() const { return m_vars; }

private:
    void parseStyleSet(const json& j, StyleSet& out);
    void parseRules(const json& arr, StyleSet& out);
    void collectDecls(const json& obj, std::vector<Decl>& out, int spec, int& order);
    std::unordered_map<std::string, std::string> m_vars;
    StyleSet m_base;
    std::unordered_map<std::string, StyleSet> m_variants;
};

class ThemeRuntime {
public:
    struct ActiveConditions {
        std::string mediaIds;
        std::string containerIds;
    };
    void setTheme(const json& themeJson);
    void clear();
    bool hasTheme() const { return !m_raw.empty(); }
    void applyAll(Control* root);
    void applyAll(Control* root, const ActiveConditions& active);
    void applyControl(Control* ctl);
    void applyControl(Control* ctl, const ActiveConditions& active);
    ActiveConditions evaluateConditions(Control* root, float viewportW, float viewportH, float viewportScale = 1.0f) const;
    void setBenchActive(const ActiveConditions& active) { m_benchActive = active; m_activeMedia = active.mediaIds; if (!active.containerIds.empty()) { if (!m_activeMedia.empty()) m_activeMedia += ","; m_activeMedia += active.containerIds; } }
    bool hasThemableData() const;
    bool hasResponsiveData() const;
    bool hasLegacyData() const;
    static bool jsonHasThemableData(const json& j);
    static bool jsonHasLegacyData(const json& j);
    void setVariant(Control* root, const std::string& name);
    const std::string& getVariant() const { return m_variant; }

    void captureExplicitFromJson(Control* ctl, const json& controlJson);
    void recordExplicitColor(Control* ctl, const char* prop, SColor color);
    void recordExplicitNumber(Control* ctl, const char* prop, double value);
    void recordExplicitString(Control* ctl, const char* prop, const std::string& value);
    void clearOverrides(Control* ctl);

    void markResponsiveDirty() { m_responsiveDirty = true; }
    bool consumeResponsiveDirty() { bool d = m_responsiveDirty; m_responsiveDirty = false; return d; }
    void refreshResponsive(Control* root, float viewportW, float viewportH, float viewportScale = 1.0f);
    std::string getActiveMedia() const { return m_activeMedia; }
    void invalidateDirty(Control* root) { applyAll(root); }

private:
    const ThemeStyleSheet::StyleSet* activeSet() const;
    void applySet(Control* root, const ThemeStyleSheet::StyleSet* set, bool includeResponsive, const ActiveConditions& active);
    void applyOne(Control* ctl, const ThemeStyleSheet::StyleSet* set, bool includeResponsive,
                  const std::unordered_map<std::string, std::string>& inheritedFont,
                  const ActiveConditions& active);
    bool matchRule(const ThemeStyleSheet::Rule& r, Control* ctl, const std::string& type,
                   const std::vector<std::string>& classes, const std::string& id) const;
    bool matchContainerSelector(const std::string& sel, Control* ctl) const;
    void applyValue(Control* ctl, const std::string& key, const std::string& value);
    void replayExplicit(Control* ctl);
    bool explicitValueMatches(Control* ctl, const std::string& key, const json& value);
    void inheritPass(Control* ctl, bool parentEligible,
                     const std::unordered_map<std::string, std::string>& parentFont,
                     bool parentTextValid, SColor parentText);
    std::unordered_map<Control*, json> m_explicit;
    ThemeStyleSheet m_sheet;
    std::shared_ptr<Theme> m_legacy;
    json m_raw;
    std::string m_variant;
    bool m_responsiveDirty = false;
    float m_lastViewportW = 0, m_lastViewportH = 0;
    std::string m_activeMedia;
    ActiveConditions m_benchActive;
    std::unordered_set<Control*> m_cssFontName, m_cssFontSize, m_cssTextNormal;
    std::unordered_map<std::string, std::string> m_controlVars;
    std::string resolveActive(const std::string& v) const;
    void collectKeys(Control* ctl, const std::vector<ThemeStyleSheet::Decl>& decls);
};

#endif
