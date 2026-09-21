// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#include "LayoutParser.h"
#include "Bench.h"
#include "WinFrame.h"
#include "Dialog.h"
#include "ComboBox.h"
#include "NumericUpDown.h"
#include "LuotiAni.h"
#include "Shape.h"
#include "ListView.h"
#include "StatusBar.h"
#include "ContextMenu.h"
#include "TabControl.h"
#include "Actor.h"
#include "Menu.h"
#include "LayoutEngine.h"
#include "PropertyNames.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include "PlatformUtils.h"

LayoutParser::LayoutParser(DataContext* dataContext)
    : m_dataContext(dataContext), m_currentLineNo(0)
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
        Platform::Log("[LayoutParser] [Line %d] [root] ERROR: JSON parse error: %s",
            lineNo, e.what());
        return nullptr;
    }

    if (j.contains(PropertyNames::kJsonTheme) && j[PropertyNames::kJsonTheme].is_object()) {
        m_theme.parse(j[PropertyNames::kJsonTheme]);
    }

    // 顶层 viewport 键：显式基准画布 + 初始缩放模式（先 canvas 后 mode，
    // mode 应用时按 canvas 重算根变换）；无实例目标（m_viewportTarget）时忽略
    if (m_viewportTarget != nullptr &&
        j.contains(PropertyNames::kJsonViewport) && j[PropertyNames::kJsonViewport].is_object()) {
        const json& vp = j[PropertyNames::kJsonViewport];
        if (vp.contains(PropertyNames::kJsonWidth) && vp.contains(PropertyNames::kJsonHeight)) {
            float w = vp[PropertyNames::kJsonWidth].get<float>();
            float h = vp[PropertyNames::kJsonHeight].get<float>();
            if (w > 0.0f && h > 0.0f) {
                m_viewportTarget->canvasWidth = w;
                m_viewportTarget->canvasHeight = h;
                // 三层模型：画布基准写入 bench rect（布局空间）；
                // off 下画布恒跟随视口，仅记录声明不生效
                if (m_viewportTarget->bench &&
                    m_viewportTarget->bench->getViewportScaleMode() != Bench::ViewportScaleMode::Off) {
                    m_viewportTarget->bench->setRect(SRect(0, 0, w, h));
                }
            }
        }
        if (m_viewportTarget->bench &&
            vp.contains(PropertyNames::kJsonViewportScaleMode) &&
            vp[PropertyNames::kJsonViewportScaleMode].is_string()) {
            std::string sm = vp[PropertyNames::kJsonViewportScaleMode].get<std::string>();
            int mode = 0;
            if (sm == "fit") mode = 1;
            else if (sm == "stretch") mode = 2;
            m_viewportTarget->bench->setViewportScaleMode(
                static_cast<Bench::ViewportScaleMode>(mode));
        }
    }

    // Component system: parse components before layouts
    if (j.contains(PropertyNames::kJsonComponents)) {
        parseComponents(j);
    }

    // Support both "layouts" (component system) and "controls" (original)
    const json* controlsArray = nullptr;
    if (j.contains(PropertyNames::kJsonLayouts) && j[PropertyNames::kJsonLayouts].is_array()) {
        controlsArray = &j[PropertyNames::kJsonLayouts];
    } else if (j.contains(PropertyNames::kJsonControls) && j[PropertyNames::kJsonControls].is_array()) {
        controlsArray = &j[PropertyNames::kJsonControls];
    }

    if (!controlsArray) {
        logError("'controls' or 'layouts' array is required");
        return nullptr;
    }

    const json& controls = *controlsArray;
    if (controls.empty()) {
        logWarn("'controls' or 'layouts' array is empty");
        return nullptr;
    }

    shared_ptr<Control> root = nullptr;
    for (size_t i = 0; i < controls.size(); ++i) {
        auto ctrl = parseControl(controls[i], nullptr, (int)i);
        if (ctrl) {
            root = ctrl;
        }
    }

    // 解析独立的 Dialogs（从 controls 树外管理）
    if (j.contains(PropertyNames::kJsonDialogs) && j[PropertyNames::kJsonDialogs].is_array()) {
        const json& dialogs = j[PropertyNames::kJsonDialogs];
        for (size_t i = 0; i < dialogs.size(); ++i) {
            auto dlg = parseControl(dialogs[i], nullptr, (int)i);
            if (auto pop = dynamic_pointer_cast<Popup>(dlg)) {
                m_dialogs.push_back(pop);
            }
        }
    }
    // 字体继承后处理：未显式声明 font/fontSize 的控件沿父链继承最近显式值
    resolveFontInheritance(root.get());
    return root;
}

shared_ptr<Control> LayoutParser::parseLayoutFile(const fs::path& jsonPath) {
    if (!fs::exists(jsonPath)) {
        Platform::Log("[LayoutParser] ERROR: Layout file not found: %s",
            jsonPath.string().c_str());
        return nullptr;
    }

    ifstream file(jsonPath);
    if (!file.is_open()) {
        Platform::Log("[LayoutParser] ERROR: Failed to open layout file: %s",
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
    if (m_dataContext) m_dataContext->unwatchAll();
    m_controlsById.clear();
    m_menuBars.clear();
    m_dialogs.clear();
    m_currentJsonPath.clear();
    m_currentLineNo = 0;
    m_rawJsonContent.clear();
    m_components.clear();
    m_componentSourceLines.clear();
    m_instantiationStack.clear();
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
    Platform::Log("[LayoutParser] [Line %d] [%s] ERROR: %s",
        m_currentLineNo, m_currentJsonPath.c_str(), message.c_str());
}

void LayoutParser::logWarn(const string& message) const {
    Platform::Log("[LayoutParser] [Line %d] [%s] WARN: %s",
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

    if (!j.contains(PropertyNames::kJsonType) || !j[PropertyNames::kJsonType].is_string()) {
        logError("'type' field is required");
        popJsonPath();
        return nullptr;
    }

    string type = j[PropertyNames::kJsonType].get<string>();

    shared_ptr<Control> result = nullptr;
    if (type == PropertyNames::kControlTypeLabel) {
        result = parseLabel(j, parent);
    } else if (type == PropertyNames::kControlTypeButton) {
        result = parseButton(j, parent);
    } else if (type == PropertyNames::kControlTypeImageButton) {
        // image-button：与 button 同语法（caption/actors/styles/scale/events），
        // 图片经 "actors" 或状态图属性设置；别名避免与普通按钮语义混淆
        result = parseButton(j, parent);
    } else if (type == PropertyNames::kControlTypeImage) {
        result = parseImage(j, parent);
    } else if (type == PropertyNames::kControlTypeShape) {
        result = parseShape(j, parent);
    } else if (type == PropertyNames::kControlTypeStatusBar) {
        result = parseStatusBar(j, parent);
    } else if (type == PropertyNames::kControlTypeAnimation) {
        result = parseAnimation(j, parent);
    } else if (type == PropertyNames::kControlTypeEditBox) {
        result = parseEditBox(j, parent);
    } else if (type == PropertyNames::kControlTypeComboBox) {
        result = parseComboBox(j, parent);
    } else if (type == PropertyNames::kControlTypeTextArea) {
        result = parseTextArea(j, parent);
    } else if (type == PropertyNames::kControlTypeCheckBox) {
        result = parseCheckBox(j, parent);
    } else if (type == PropertyNames::kControlTypeProgressBar) {
        result = parseProgressBar(j, parent);
    } else if (type == PropertyNames::kControlTypeSlider) {
        result = parseSlider(j, parent);
    } else if (type == PropertyNames::kControlTypeScrollBar) {
        result = parseScrollBar(j, parent);
    } else if (type == PropertyNames::kControlTypePanel) {
        result = parsePanel(j, parent);
    } else if (type == PropertyNames::kControlTypeWinFrame) {
        result = parseWinFrame(j, parent);
    } else if (type == PropertyNames::kControlTypeColorPicker) {
        result = parseColorPicker(j, parent);
    } else if (type == PropertyNames::kControlTypePopup) {
        result = parsePopup(j, parent);
    } else if (type == PropertyNames::kControlTypeConfirmPopup) {
        result = parseConfirmPopup(j, parent);
    } else if (type == PropertyNames::kControlTypeDialog) {
        result = parseDialog(j, parent);
    } else if (type == PropertyNames::kControlTypeMenuBar) {
        auto menuBar = parseMenuBar(j, parent);
        if (menuBar) {
            // 容器 children 内（parent 非空，如 v-flow 布局）：正常返回挂树，
            // setParent 自动切换手动定位参与布局引擎重排（弹出面板顶层挂载机制保证不被裁剪）；
            // 顶层裸用（parent 为 null）：保持"独立存储 + 调用方挂 BENCH 顶层"的旧语义（兼容 test_layout 等）
            if (parent == nullptr) {
                m_menuBars.push_back(menuBar);
                result = nullptr;
                popJsonPath();
                return nullptr;
            }
            result = menuBar;
        }
    } else if (type == PropertyNames::kControlTypeNumericUpDown) {
        result = parseNumericUpDown(j, parent);
    } else if (type == PropertyNames::kControlTypeSplitter) {
        result = parseSplitter(j, parent);
    } else if (type == PropertyNames::kControlTypeTreeView) {
        result = parseTreeView(j, parent);
    } else if (type == PropertyNames::kControlTypeListView) {
        result = parseListView(j, parent);
    } else if (type == PropertyNames::kControlTypeTabControl) {
        result = parseTabControl(j, parent);
    } else if (m_components.find(type) != m_components.end()) {
        // Component type: instantiate from template
        result = instantiateComponent(type, j, parent, index);
    } else {
        logWarn("unknown control type \"" + type + "\", skipping");
        popJsonPath();
        return nullptr;
    }

    popJsonPath();

    // 右键上下文菜单（决策点 2-C）：控件级 "contextMenu" 键（内嵌 items，复用 populateMenuPanel）
    if (result && j.contains(PropertyNames::kJsonContextMenu) && j[PropertyNames::kJsonContextMenu].is_object()) {
        auto cm = make_shared<ContextMenu>(parent, 1.f, 1.f);
        const auto& cmj = j[PropertyNames::kJsonContextMenu];
        if (cmj.contains(PropertyNames::kItems) && cmj[PropertyNames::kItems].is_array()) {
            populateMenuPanel(cm->getMenuPanel(), cmj[PropertyNames::kItems], 1.f, 1.f);
        }
        if (auto* ci = dynamic_cast<ControlImpl*>(result.get())) {
            ci->setContextMenu(cm);
        }
    }

    return result;
}

// ==================== Label ====================

shared_ptr<Label> LayoutParser::parseLabel(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto label = make_shared<Label>(parent, rect, xScale, yScale);

    m_theme.applyCommonColors(label, PropertyNames::kThemeCatLabel);
    m_theme.applyFont(label, PropertyNames::kThemeCatLabel);
    parseCommonProperties(label, j);

    // caption
    if (j.contains(PropertyNames::kCaption) && j[PropertyNames::kCaption].is_string()) {
        label->setCaption(j[PropertyNames::kCaption].get<string>());
    }

    // alignment
    if (j.contains(PropertyNames::kJsonAlignment) && j[PropertyNames::kJsonAlignment].is_string()) {
        label->setAlignmentMode(parseAlignment(j[PropertyNames::kJsonAlignment].get<string>()));
    }

    // font
    if (j.contains(PropertyNames::kFont) && j[PropertyNames::kFont].is_object()) {
        pushJsonPath(PropertyNames::kFont);
        const json& font = j[PropertyNames::kFont];
        if (font.contains(PropertyNames::kJsonName) && font[PropertyNames::kJsonName].is_string()) {
            label->setFont(parseFontName(font[PropertyNames::kJsonName].get<string>()));
        }
        if (font.contains(PropertyNames::kJsonSize) && font[PropertyNames::kJsonSize].is_number()) {
            label->setFontSize(font[PropertyNames::kJsonSize].get<int>());
        }
        if (font.contains(PropertyNames::kJsonStyle) && (font[PropertyNames::kJsonStyle].is_string() || font[PropertyNames::kJsonStyle].is_array())) {
            label->SetFontStyle(parseFontStyle(font[PropertyNames::kJsonStyle]));
        }
        popJsonPath();
    }

    // fontResource：内存字体引用（String 属性 font-resource，两阶段：create() 补读）
    if (j.contains(PropertyNames::kFontResource) && j[PropertyNames::kFontResource].is_string()) {
        label->setStringProperty(PropertyNames::kFontResource,
                                 j[PropertyNames::kFontResource].get<string>().c_str());
    }

    // fontFile：任意字体文件路径（String 属性 font-file，两阶段：create() 补读）
    if (j.contains(PropertyNames::kFontFile) && j[PropertyNames::kFontFile].is_string()) {
        label->setStringProperty(PropertyNames::kFontFile,
                                 j[PropertyNames::kFontFile].get<string>().c_str());
    }

    // shadow
    if (j.contains(PropertyNames::kShadow) && j[PropertyNames::kShadow].is_object()) {
        pushJsonPath(PropertyNames::kShadow);
        const json& shadow = j[PropertyNames::kShadow];
        label->setShadow(shadow.value(PropertyNames::kEnabled, false));
        if (shadow.contains(PropertyNames::kJsonOffset) && shadow[PropertyNames::kJsonOffset].is_object()) {
            float ox = shadow[PropertyNames::kJsonOffset].value(PropertyNames::kJsonX, 1.0f);
            float oy = shadow[PropertyNames::kJsonOffset].value(PropertyNames::kJsonY, 1.0f);
            label->setShadowOffset(SPoint(ox, oy));
        }
        popJsonPath();
    }

    // lineHeight
    if (j.contains(PropertyNames::kLineHeight) && j[PropertyNames::kLineHeight].is_number()) {
        label->setLineHeight(j[PropertyNames::kLineHeight].get<int>());
    }

    // lineSpacingRatio
    if (j.contains(PropertyNames::kLineSpacingRatio) && j[PropertyNames::kLineSpacingRatio].is_number()) {
        label->setLineSpacingRatio(j[PropertyNames::kLineSpacingRatio].get<float>());
    }

    // enableExpand
    if (j.contains(PropertyNames::kJsonEnableExpand) && j[PropertyNames::kJsonEnableExpand].is_boolean()) {
        label->setEnableExpand(j[PropertyNames::kJsonEnableExpand].get<bool>());
    }

    // debugDraw
    if (j.contains(PropertyNames::kDebugDraw) && j[PropertyNames::kDebugDraw].is_boolean()) {
        label->setDebugDraw(j[PropertyNames::kDebugDraw].get<bool>());
    }

    // events
    parseEvents(label, j);
    parseBindings(label, j);

    // id
    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = label;
    }

    label->create();

    return label;
}

// ==================== Animation (LuotiAni) ====================

shared_ptr<Control> LayoutParser::parseAnimation(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto ani = make_shared<LuotiAni>(parent, xScale, yScale);
    ani->setRect(rect);
    m_theme.applyCommonColors(ani, PropertyNames::kThemeCatPanel);
    parseCommonProperties(ani, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = ani;
    }

    // 动画描述文件路径（"path"）；w/h 传 0 → prepare 回退到画布尺寸。
    // parse 阶段尚无渲染设备：只解析描述（loadFromFile），挂树后由
    // LuotiAni::setRenderDevice 补 prepare（与 C ABI CreateAnimation 一致：
    // 创建后不自动播放，SetBool "playing" 控制）
    if (j.contains(PropertyNames::kJsonPath) && j[PropertyNames::kJsonPath].is_string()) {
        string p = j[PropertyNames::kJsonPath].get<string>();
        fs::path fp(p);
        // provider: 前缀是资源引用（非文件路径）：不拼 base，由 loadFromFile 分流
        if (fp.is_relative() && p.rfind(PropertyNames::kProviderPrefix, 0) != 0) fp = fs::path(Platform::GetBasePath()) / fp;
        try {
            ani->loadFromFile(fp);
        } catch (...) {
            logWarn("animation load failed: " + p);
            return nullptr;
        }
    }
    // 与 parseButton 内嵌 luotiAni 一致：JSON "playing": true → 挂树补 prepare 后自动播放
    if (j.contains(PropertyNames::kPlaying)) {
        ani->setBoolProperty(PropertyNames::kPlaying, j[PropertyNames::kPlaying].get<bool>() ? 1 : 0);
    }
    return ani;
}

// ==================== Image（Actor 独立节点） ====================

shared_ptr<Control> LayoutParser::parseShape(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto shape = make_shared<Shape>(parent, rect, xScale, yScale);
    m_theme.applyCommonColors(shape, PropertyNames::kThemeCatPanel);
    parseCommonProperties(shape, j);

    // 形状类型
    if (j.contains(PropertyNames::kShape) && j[PropertyNames::kShape].is_string()) {
        const std::string& st = j[PropertyNames::kShape].get<std::string>();
        if (st == PropertyNames::kShapeRect)              shape->setShape(ShapeType::Rect);
        else if (st == PropertyNames::kShapeFilledRect)   shape->setShape(ShapeType::FilledRect);
        else if (st == PropertyNames::kShapeRoundRect)    shape->setShape(ShapeType::RoundRect);
        else if (st == PropertyNames::kShapeCircle)       shape->setShape(ShapeType::Circle);
        else if (st == PropertyNames::kShapeEllipse)      shape->setShape(ShapeType::Ellipse);
        else if (st == PropertyNames::kShapePolyline)     shape->setShape(ShapeType::Polyline);
        else if (st == PropertyNames::kShapePolygon)      shape->setShape(ShapeType::Polygon);
        else logWarn("shape: unknown shape \"" + st + "\", keep default rect");
    }
    // 颜色（hex 字符串复用现有 parseColor 链路）
    if (j.contains(PropertyNames::kFill) && j[PropertyNames::kFill].is_string())
        shape->setFillColor(parseColor(j[PropertyNames::kFill]));
    if (j.contains(PropertyNames::kStroke) && j[PropertyNames::kStroke].is_string())
        shape->setStrokeColor(parseColor(j[PropertyNames::kStroke]));
    // 数值参数
    if (j.contains(PropertyNames::kJsonLineWidth) && j[PropertyNames::kJsonLineWidth].is_number())
        shape->setLineWidth(j[PropertyNames::kJsonLineWidth].get<float>());
    if (j.contains(PropertyNames::kRadius) && j[PropertyNames::kRadius].is_number())
        shape->setRadius(j[PropertyNames::kRadius].get<float>());
    if (j.contains(PropertyNames::kRingWidth) && j[PropertyNames::kRingWidth].is_number())
        shape->setRingWidth(j[PropertyNames::kRingWidth].get<float>());
    // 点集（本地像素，决策 3.3）
    if (j.contains(PropertyNames::kJsonPoints) && j[PropertyNames::kJsonPoints].is_array()) {
        std::vector<Shape::SPointF> pts;
        for (const auto& p : j[PropertyNames::kJsonPoints]) {
            if (p.is_object() && p.contains(PropertyNames::kJsonX) && p.contains(PropertyNames::kJsonY) &&
                p[PropertyNames::kJsonX].is_number() && p[PropertyNames::kJsonY].is_number())
                pts.push_back({p[PropertyNames::kJsonX].get<float>(), p[PropertyNames::kJsonY].get<float>()});
        }
        if (!pts.empty()) shape->setPoints(pts);
    }

    // 多图元（组合图形）：非空时替代单图元渲染
    if (j.contains(PropertyNames::kPrimitives) && j[PropertyNames::kPrimitives].is_array()) {
        pushJsonPath(PropertyNames::kPrimitives);
        for (const auto& pj : j[PropertyNames::kPrimitives]) {
            if (!pj.is_object()) continue;
            ShapeType t = ShapeType::Rect;
            const std::string st = pj.value(PropertyNames::kShape, std::string());
            if (!shapeTypeFromString(st, t)) {
                logWarn("shape primitive: unknown shape \"" + st + "\", skipped");
                continue;
            }
            SRect lr(0, 0, 0, 0);
            if (pj.contains(PropertyNames::kJsonRect) && pj[PropertyNames::kJsonRect].is_array() &&
                pj[PropertyNames::kJsonRect].size() >= 4) {
                const auto& ra = pj[PropertyNames::kJsonRect];
                lr = SRect(ra[0].get<float>(), ra[1].get<float>(),
                           ra[2].get<float>(), ra[3].get<float>());
            }
            const int idx = shape->addPrimitive(t, lr);
            if (pj.contains(PropertyNames::kFill) && pj[PropertyNames::kFill].is_string())
                shape->setPrimitiveFill(idx, parseColor(pj[PropertyNames::kFill]));
            if (pj.contains(PropertyNames::kStroke) && pj[PropertyNames::kStroke].is_string())
                shape->setPrimitiveStroke(idx, parseColor(pj[PropertyNames::kStroke]));
            if (pj.contains(PropertyNames::kJsonLineWidth) && pj[PropertyNames::kJsonLineWidth].is_number())
                shape->setPrimitiveLineWidth(idx, pj[PropertyNames::kJsonLineWidth].get<float>());
            if (pj.contains(PropertyNames::kRadius) && pj[PropertyNames::kRadius].is_number())
                shape->setPrimitiveRadius(idx, pj[PropertyNames::kRadius].get<float>());
            if (pj.contains(PropertyNames::kRingWidth) && pj[PropertyNames::kRingWidth].is_number())
                shape->setPrimitiveRingWidth(idx, pj[PropertyNames::kRingWidth].get<float>());
            if (pj.contains(PropertyNames::kJsonPoints) && pj[PropertyNames::kJsonPoints].is_array()) {
                std::vector<Shape::SPointF> pts;
                for (const auto& p : pj[PropertyNames::kJsonPoints]) {
                    if (p.is_object() && p.contains(PropertyNames::kJsonX) && p.contains(PropertyNames::kJsonY) &&
                        p[PropertyNames::kJsonX].is_number() && p[PropertyNames::kJsonY].is_number())
                        pts.push_back({p[PropertyNames::kJsonX].get<float>(), p[PropertyNames::kJsonY].get<float>()});
                }
                if (!pts.empty()) shape->setPrimitivePoints(idx, pts);
            }
        }
        popJsonPath();
    }

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<std::string>()] = shape;

    shape->create();
    return shape;
}

shared_ptr<Control> LayoutParser::parseListView(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto lv = make_shared<ListView>(parent, rect, xScale, yScale);
    m_theme.applyCommonColors(lv, PropertyNames::kThemeCatPanel);
    parseCommonProperties(lv, j);

    // 属性（§5.6 矩阵一期全量）
    if (j.contains(PropertyNames::kMode) && j[PropertyNames::kMode].is_string()) {
        const string m = j[PropertyNames::kMode].get<string>();
        if (m == PropertyNames::kModeSingle) lv->setMode(ListView::Mode::Single);
        else if (m == PropertyNames::kModeMulti) lv->setMode(ListView::Mode::Multi);
        else logWarn("list-view: unknown mode \"" + m + "\"");
    }
    if (j.contains(PropertyNames::kMultiSelect) && j[PropertyNames::kMultiSelect].is_boolean())
        lv->setMultiSelect(j[PropertyNames::kMultiSelect].get<bool>());
    if (j.contains(PropertyNames::kSelectedIndex) && j[PropertyNames::kSelectedIndex].is_number_integer())
        lv->setSelectedRow(j[PropertyNames::kSelectedIndex].get<int>());
    if (j.contains(PropertyNames::kCycleNavigation) && j[PropertyNames::kCycleNavigation].is_boolean())
        lv->setCycleNavigation(j[PropertyNames::kCycleNavigation].get<bool>());
    if (j.contains(PropertyNames::kRowHeight) && j[PropertyNames::kRowHeight].is_number())
        lv->setRowHeight(j[PropertyNames::kRowHeight].get<float>());
    if (j.contains(PropertyNames::kHeaderHeight) && j[PropertyNames::kHeaderHeight].is_number())
        lv->setHeaderHeight(j[PropertyNames::kHeaderHeight].get<float>());
    if (j.contains(PropertyNames::kGridlines) && j[PropertyNames::kGridlines].is_boolean())
        lv->setGridlines(j[PropertyNames::kGridlines].get<bool>());
    if (j.contains(PropertyNames::kHorizontalGridlines) && j[PropertyNames::kHorizontalGridlines].is_boolean())
        lv->setHorizontalGridlines(j[PropertyNames::kHorizontalGridlines].get<bool>());
    if (j.contains(PropertyNames::kJsonHover) && j[PropertyNames::kJsonHover].is_boolean())
        lv->setHoverHighlight(j[PropertyNames::kJsonHover].get<bool>());
    if (j.contains(PropertyNames::kMinColumnWidth) && j[PropertyNames::kMinColumnWidth].is_number())
        lv->setMinColumnWidth(j[PropertyNames::kMinColumnWidth].get<float>());

    // columns [{title,width,sortable,(icon 一期暂缓：StatusBar icon 机制未落地)}]
    if (j.contains(PropertyNames::kJsonColumns) && j[PropertyNames::kJsonColumns].is_array()) {
        for (const auto& cj : j[PropertyNames::kJsonColumns]) {
            const string title = cj.value(PropertyNames::kTitle, string());
            const float width = cj.value(PropertyNames::kJsonWidth, 100.0f);
            const bool sortable = cj.value(PropertyNames::kJsonSortable, false);
            lv->addColumn(title, width, sortable);
            if (cj.contains(PropertyNames::kJsonIcon))
                logWarn("list-view: column icon deferred (StatusBar icon mechanism pending), skipped");
        }
    }
    // sortColumn/sortAscending（初始排序状态；列存在后触发重排）
    if (j.contains(PropertyNames::kSortColumn) && j[PropertyNames::kSortColumn].is_number_integer()) {
        lv->setSortColumn(j[PropertyNames::kSortColumn].get<int>());
        if (j.contains(PropertyNames::kSortAscending) && j[PropertyNames::kSortAscending].is_boolean())
            lv->setSortAscending(j[PropertyNames::kSortAscending].get<bool>());
    }

    // rows [{id,cells[],(icon 暂缓),cellControls}]
    if (j.contains(PropertyNames::kJsonRows) && j[PropertyNames::kJsonRows].is_array()) {
        for (const auto& rj : j[PropertyNames::kJsonRows]) {
            const string id = rj.value(PropertyNames::kId, string());
            vector<string> cells;
            if (rj.contains(PropertyNames::kJsonCells) && rj[PropertyNames::kJsonCells].is_array())
                for (const auto& c : rj[PropertyNames::kJsonCells])
                    cells.push_back(c.is_string() ? c.get<string>() : string());
            if (cells.empty() && !id.empty()) cells.push_back(id);   // 单列兜底
            lv->addRow(id, cells);
            const int rowIdx = lv->getRowCount() - 1;
            if (rj.contains(PropertyNames::kJsonIcon))
                logWarn("list-view: row icon deferred (StatusBar icon mechanism pending), skipped");
            // cellControls: [{"col":0,"control":{...}}]
            if (rj.contains(PropertyNames::kJsonCellControls) && rj[PropertyNames::kJsonCellControls].is_array()) {
                for (const auto& cc : rj[PropertyNames::kJsonCellControls]) {
                    if (!cc.is_object() || !cc.contains(PropertyNames::kJsonCol) || !cc[PropertyNames::kJsonCol].is_number_integer() ||
                        !cc.contains(PropertyNames::kJsonControl) || !cc[PropertyNames::kJsonControl].is_object())
                        continue;
                    json lcJson = cc[PropertyNames::kJsonControl];
                    if (!lcJson.contains(PropertyNames::kJsonRect)) {
                        lcJson[PropertyNames::kJsonRect] = {
                            {PropertyNames::kJsonX, 0}, {PropertyNames::kJsonY, 0},
                            {PropertyNames::kJsonW, 0}, {PropertyNames::kJsonH, 0}
                        };
                    }
                    auto lc = parseControl(lcJson, nullptr, 0);
                    if (lc) lv->setCellLeadingControl(rowIdx, cc[PropertyNames::kJsonCol].get<int>(), lc);
                    else logWarn("list-view cellControl parse failed, skipped");
                }
            }
        }
    }

    parseEvents(lv, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<std::string>()] = lv;

    lv->create();
    return lv;
}

shared_ptr<Control> LayoutParser::parseTabControl(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto tc = make_shared<TabControl>(parent, rect, xScale, yScale);
    m_theme.applyCommonColors(tc, PropertyNames::kThemeCatPanel);
    parseCommonProperties(tc, j);

    if (j.contains(PropertyNames::kJsonPosition) && j[PropertyNames::kJsonPosition].is_string()) {
        const string p = j[PropertyNames::kJsonPosition].get<string>();
        if (p == PropertyNames::kTabPositionBottom) tc->setPosition(TabPosition::Bottom);
        else if (p == PropertyNames::kTabPositionLeft) tc->setPosition(TabPosition::Left);
        else if (p == PropertyNames::kTabPositionRight) tc->setPosition(TabPosition::Right);
        else tc->setPosition(TabPosition::Top);
    }
    if (j.contains(PropertyNames::kFontSize) && j[PropertyNames::kFontSize].is_number())
        tc->setFontSize(j[PropertyNames::kFontSize].get<float>());

    if (j.contains(PropertyNames::kJsonTabs) && j[PropertyNames::kJsonTabs].is_array()) {
        pushJsonPath(PropertyNames::kJsonTabs);
        for (const auto& tj : j[PropertyNames::kJsonTabs]) {
            const string title = tj.value(PropertyNames::kTitle, string());
            shared_ptr<Control> page;
            if (tj.contains(PropertyNames::kJsonPage) && tj[PropertyNames::kJsonPage].is_object()) {
                json pj = tj[PropertyNames::kJsonPage];
                if (!pj.contains(PropertyNames::kJsonRect)) {
                    pj[PropertyNames::kJsonRect] = {
                        {PropertyNames::kJsonX, 0}, {PropertyNames::kJsonY, 0},
                        {PropertyNames::kJsonW, 0}, {PropertyNames::kJsonH, 0}
                    };
                }
                page = parseControl(pj, nullptr, 0);
            }
            tc->addTab(title, page);
            int idx = tc->getTabCount() - 1;
            if (tj.contains(PropertyNames::kJsonIcon) && tj[PropertyNames::kJsonIcon].is_string()) {
                string iconPath = tj[PropertyNames::kJsonIcon].get<string>();
                shared_ptr<Actor> actor = nullptr;
                if (iconPath.rfind(PropertyNames::kProviderPrefix, 0) == 0) {
                    actor = make_shared<Actor>(nullptr, iconPath.substr(strlen(PropertyNames::kProviderPrefix)), false, xScale, yScale);
                } else {
                    fs::path p(iconPath);
                    if (p.is_relative()) p = fs::path(Platform::GetBasePath()) / p;
                    actor = make_shared<Actor>(nullptr, p, false, xScale, yScale);
                }
                if (actor) { actor->create(); tc->setTabLeadingControl(idx, actor); }
            }
        }
        popJsonPath();
    }

    if (j.contains(PropertyNames::kJsonCurrentIndex) && j[PropertyNames::kJsonCurrentIndex].is_number_integer())
        tc->setCurrentIndex(j[PropertyNames::kJsonCurrentIndex].get<int>());

    parseEvents(tc, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<std::string>()] = tc;

    tc->create();
    return tc;
}

shared_ptr<Control> LayoutParser::parseImage(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    string filePath;
    string resourceId;
    if (j.contains(PropertyNames::kImage) && j[PropertyNames::kImage].is_string()) {
        filePath = j[PropertyNames::kImage].get<string>();
    } else if (j.contains(PropertyNames::kImageResource) && j[PropertyNames::kImageResource].is_string()) {
        resourceId = j[PropertyNames::kImageResource].get<string>();
    } else if (j.contains(PropertyNames::kJsonResourceId) && j[PropertyNames::kJsonResourceId].is_string()) {
        resourceId = j[PropertyNames::kJsonResourceId].get<string>();
    } else if (j.contains(PropertyNames::kJsonProviderName) && j[PropertyNames::kJsonProviderName].is_string()) {
        resourceId = j[PropertyNames::kJsonProviderName].get<string>();
    } else {
        logWarn("image control requires \"image\" or \"imageResource\" key, skipping");
        return nullptr;
    }

    bool matchRect = j.value(PropertyNames::kMatchParentRect, false);
    shared_ptr<Actor> actor = nullptr;
    if (!filePath.empty()) {
        actor = make_shared<Actor>(parent, fs::path(filePath), matchRect, xScale, yScale);
    } else {
        actor = make_shared<Actor>(parent, resourceId, matchRect, xScale, yScale);
    }
    actor->setRect(rect);
    if (j.contains(PropertyNames::kScaleType) && j[PropertyNames::kScaleType].is_string()) {
        string st = j[PropertyNames::kScaleType].get<string>();
        if (st == PropertyNames::kScaleTypeFitCenter)       actor->setScaleType(ScaleType::FIT_CENTER);
        else if (st == PropertyNames::kScaleTypeCenterCrop) actor->setScaleType(ScaleType::CENTER_CROP);
        else if (st == PropertyNames::kScaleTypeNone)       actor->setScaleType(ScaleType::NONE);
    }
    m_theme.applyCommonColors(actor, PropertyNames::kThemeCatPanel);
    parseCommonProperties(actor, j);

    if (j.contains(PropertyNames::kAlpha) && j[PropertyNames::kAlpha].is_number_integer()) {
        actor->setAlpha(static_cast<uint8_t>(j[PropertyNames::kAlpha].get<int>()));
    }

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = actor;
    }
    return actor;
}

// ==================== Button ====================

shared_ptr<Button> LayoutParser::parseButton(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto btn = make_shared<Button>(parent, rect, xScale, yScale);

    m_theme.applyCommonColors(btn, PropertyNames::kThemeCatButton);
    parseCommonProperties(btn, j);

    // caption-label embedding (Phase 2): 使用完整 Label 配置（键与运行时属性键同键同常量）
    if (j.contains(PropertyNames::kCaptionLabel) && j[PropertyNames::kCaptionLabel].is_object()) {
        pushJsonPath(PropertyNames::kCaptionLabel);

        const json& cl = j[PropertyNames::kCaptionLabel];

        auto builder = LabelBuilder(btn.get(), SRect(0, 0, rect.width, rect.height));

        builder.setFont(m_theme.getFontName(PropertyNames::kThemeCatButton));
        builder.setFontSize(m_theme.getFontSize(PropertyNames::kThemeCatButton));

        if (cl.contains(PropertyNames::kCaption) && cl[PropertyNames::kCaption].is_string()) {
            builder.setCaption(cl[PropertyNames::kCaption].get<string>());
        }
        if (cl.contains(PropertyNames::kFont) && cl[PropertyNames::kFont].is_object()) {
            const json& font = cl[PropertyNames::kFont];
            if (font.contains(PropertyNames::kJsonName) && font[PropertyNames::kJsonName].is_string()) {
                builder.setFont(parseFontName(font[PropertyNames::kJsonName].get<string>()));
            }
            if (font.contains(PropertyNames::kJsonSize) && font[PropertyNames::kJsonSize].is_number()) {
                builder.setFontSize(font[PropertyNames::kJsonSize].get<int>());
            }
            if (font.contains(PropertyNames::kJsonStyle) && (font[PropertyNames::kJsonStyle].is_string() || font[PropertyNames::kJsonStyle].is_array())) {
                builder.SetFontStyle(parseFontStyle(font[PropertyNames::kJsonStyle]));
            }
        }

        if (cl.contains(PropertyNames::kJsonAlignment) && cl[PropertyNames::kJsonAlignment].is_string()) {
            builder.setAlignmentMode(parseAlignment(cl[PropertyNames::kJsonAlignment].get<string>()));
        }

        if (cl.contains(PropertyNames::kShadow) && cl[PropertyNames::kShadow].is_object()) {
            const json& shadow = cl[PropertyNames::kShadow];
            if (shadow.contains(PropertyNames::kEnabled) && shadow[PropertyNames::kEnabled].is_boolean()) {
                builder.setShadow(shadow[PropertyNames::kEnabled].get<bool>());
            }
            if (shadow.contains(PropertyNames::kJsonOffset) && shadow[PropertyNames::kJsonOffset].is_object()) {
                SPoint offset;
                offset.x = shadow[PropertyNames::kJsonOffset].value(PropertyNames::kJsonX, 1);
                offset.y = shadow[PropertyNames::kJsonOffset].value(PropertyNames::kJsonY, 1);
                builder.setShadowOffset(offset);
            }
        }

        if (cl.contains(PropertyNames::kJsonColors) && cl[PropertyNames::kJsonColors].is_object()) {
            const json& colors = cl[PropertyNames::kJsonColors];
            if (colors.contains(PropertyNames::kText) && colors[PropertyNames::kText].is_object()) {
                builder.setTextStateColor(parseStateColor(colors[PropertyNames::kText], StateColor::Type::Text));
            }
            if (colors.contains(PropertyNames::kTextShadow) && colors[PropertyNames::kTextShadow].is_object()) {
                builder.setTextShadowStateColor(parseStateColor(colors[PropertyNames::kTextShadow], StateColor::Type::TextShadow));
            }
        }

        auto label = builder.build();
        btn->setCaptionLabel(label);

        popJsonPath();
    } else {
        // 简单方式 (Phase 1): 仅 caption / captionSize / enableTextShadow
        int themeFontSize = m_theme.getFontSize(PropertyNames::kThemeCatButton);
        if (themeFontSize != 16) {
            btn->setCaptionSize((float)themeFontSize);
        }

        if (j.contains(PropertyNames::kCaption) && j[PropertyNames::kCaption].is_string()) {
            btn->setCaption(j[PropertyNames::kCaption].get<string>());
        }

        if (j.contains(PropertyNames::kFontSize) && j[PropertyNames::kFontSize].is_number()) {
            btn->setCaptionSize(j[PropertyNames::kFontSize].get<float>());
        }

    }

    // Actors (state images)
    if (j.contains(PropertyNames::kJsonActors) && j[PropertyNames::kJsonActors].is_object()) {
        pushJsonPath(PropertyNames::kJsonActors);
        const json& actors = j[PropertyNames::kJsonActors];

        bool matchRect = actors.value(PropertyNames::kMatchParentRect, false);

        auto createActor = [&](const json& v) -> shared_ptr<Actor> {
            if (v.is_null()) return nullptr;
            string filePath;
            string resourceId;
            ScaleType scaleType = ScaleType::STRETCH;
            if (v.is_string()) {
                filePath = v.get<string>();
            } else if (v.is_object()) {
                if (v.contains(PropertyNames::kJsonFile) && v[PropertyNames::kJsonFile].is_string()) {
                    filePath = v[PropertyNames::kJsonFile].get<string>();
                }
                if (v.contains(PropertyNames::kJsonResourceId) && v[PropertyNames::kJsonResourceId].is_string()) {
                    resourceId = v[PropertyNames::kJsonResourceId].get<string>();
                }
                if (v.contains(PropertyNames::kJsonProviderName) && v[PropertyNames::kJsonProviderName].is_string()) {
                    resourceId = v[PropertyNames::kJsonProviderName].get<string>();
                }
                if (v.contains(PropertyNames::kScaleType) && v[PropertyNames::kScaleType].is_string()) {
                    string st = v[PropertyNames::kScaleType].get<string>();
                    if (st == PropertyNames::kScaleTypeFitCenter)      scaleType = ScaleType::FIT_CENTER;
                    else if (st == PropertyNames::kScaleTypeCenterCrop) scaleType = ScaleType::CENTER_CROP;
                    else if (st == PropertyNames::kScaleTypeNone)        scaleType = ScaleType::NONE;
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

        if (actors.contains(PropertyNames::kStateKeyNormal) && !actors[PropertyNames::kStateKeyNormal].is_null()) {
            auto actor = createActor(actors[PropertyNames::kStateKeyNormal]);
            if (actor) btn->setNormalStateActor(actor);
        }
        if (actors.contains(PropertyNames::kStateKeyHover) && !actors[PropertyNames::kStateKeyHover].is_null()) {
            auto actor = createActor(actors[PropertyNames::kStateKeyHover]);
            if (actor) btn->setHoverStateActor(actor);
        }
        if (actors.contains(PropertyNames::kStateKeyPressed) && !actors[PropertyNames::kStateKeyPressed].is_null()) {
            auto actor = createActor(actors[PropertyNames::kStateKeyPressed]);
            if (actor) btn->setPressedStateActor(actor);
        }
        if (actors.contains(PropertyNames::kStateKeyDisabled) && !actors[PropertyNames::kStateKeyDisabled].is_null()) {
            auto actor = createActor(actors[PropertyNames::kStateKeyDisabled]);
            if (actor) btn->setDisabledStateActor(actor);
    }

    

    popJsonPath();
    }

    // LuotiAni (particle animation)
    if (j.contains(PropertyNames::kLuotiAni) && !j[PropertyNames::kLuotiAni].is_null()) {
        pushJsonPath(PropertyNames::kLuotiAni);
        const json& la = j[PropertyNames::kLuotiAni];
        string filePath;
        string resourceId;
        if (la.is_string()) {
            filePath = la.get<string>();
        } else if (la.is_object()) {
            if (la.contains(PropertyNames::kJsonFile) && la[PropertyNames::kJsonFile].is_string()) {
                filePath = la[PropertyNames::kJsonFile].get<string>();
            }
            if (la.contains(PropertyNames::kJsonResourceId) && la[PropertyNames::kJsonResourceId].is_string()) {
                resourceId = la[PropertyNames::kJsonResourceId].get<string>();
            }
        }
        if (!filePath.empty()) {
            try {
                // 构造 scale 恒为 1.0：按钮 scale 经 setParent 复合缩放作用于
                // 内嵌动画（setParent 中 m_xxScale = m_xScale * parent scale），
                // 传按钮 scale 会造成双重缩放
                auto luotiAni = make_shared<LuotiAni>(btn.get(), 1.0f, 1.0f);
                fs::path rp(filePath);
                // provider: 前缀是资源引用（非文件路径）：不拼 base，由 loadFromFile 分流
                if (rp.is_relative() && filePath.rfind(PropertyNames::kProviderPrefix, 0) != 0) rp = fs::path(Platform::GetBasePath()) / rp;
                luotiAni->loadFromFile(rp);
                luotiAni->setRect(SRect(0, 0, rect.width, rect.height));
                // parse 阶段尚无渲染设备：不 prepare/play，挂树后由
                // LuotiAni::setRenderDevice 补 prepare（同 CreateAnimation 模式），
                // 播放由 SetBool "playing" 控制
                btn->setLuotiAni(luotiAni);
            } catch (const char* e) {
                logWarn("failed to load luotiAni from file: " + filePath + " (" + e + ")");
            } catch (...) {
                logWarn("failed to load luotiAni from file: " + filePath);
            }
        } else if (!resourceId.empty()) {
            try {
                auto luotiAni = make_shared<LuotiAni>(btn.get(), btn->getScaleXX(), btn->getScaleYY());
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

    // playing：声明式启动内嵌动画（挂树前设置 → 记录请求，prepare 完成后自动播放）
    if (j.contains(PropertyNames::kPlaying) && j[PropertyNames::kPlaying].is_boolean()
        && j[PropertyNames::kPlaying].get<bool>()) {
        btn->setBoolProperty(PropertyNames::kPlaying, 1);
    }

    parseEvents(btn, j);
    parseBindings(btn, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = btn;
    }

    btn->create();
    return btn;
}

// ==================== EditBox ====================

shared_ptr<EditBox> LayoutParser::parseEditBox(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto editBox = make_shared<EditBox>(parent, rect, xScale, yScale);

    m_theme.applyCommonColors(editBox, PropertyNames::kThemeCatEditBox);
    editBox->setFont(m_theme.getFontName(PropertyNames::kThemeCatEditBox));
    editBox->setFontSize(m_theme.getFontSize(PropertyNames::kThemeCatEditBox));
    parseCommonProperties(editBox, j);

    if (j.contains(PropertyNames::kText) && j[PropertyNames::kText].is_string()) {
        editBox->setText(j[PropertyNames::kText].get<string>());
    }

    if (j.contains(PropertyNames::kPlaceholder) && j[PropertyNames::kPlaceholder].is_string()) {
        editBox->setPlaceholder(j[PropertyNames::kPlaceholder].get<string>());
    }

    if (j.contains(PropertyNames::kPasswordMode) && j[PropertyNames::kPasswordMode].is_boolean()) {
        editBox->setPasswordMode(j[PropertyNames::kPasswordMode].get<bool>());
    }

    if (j.contains(PropertyNames::kPasswordChar) && j[PropertyNames::kPasswordChar].is_string()) {
        string pc = j[PropertyNames::kPasswordChar].get<string>();
        if (!pc.empty()) {
            editBox->setPasswordChar(pc[0]);
        }
    }

    if (j.contains(PropertyNames::kFont) && j[PropertyNames::kFont].is_object()) {
        pushJsonPath(PropertyNames::kFont);
        const json& font = j[PropertyNames::kFont];
        if (font.contains(PropertyNames::kJsonName) && font[PropertyNames::kJsonName].is_string()) {
            editBox->setFont(parseFontName(font[PropertyNames::kJsonName].get<string>()));
        }
        if (font.contains(PropertyNames::kJsonSize) && font[PropertyNames::kJsonSize].is_number()) {
            editBox->setFontSize(font[PropertyNames::kJsonSize].get<int>());
        }
        popJsonPath();
    }

    if (j.contains(PropertyNames::kJsonAlignment) && j[PropertyNames::kJsonAlignment].is_string()) {
        editBox->setAlignmentMode(parseAlignment(j[PropertyNames::kJsonAlignment].get<string>()));
    }

    if (j.contains(PropertyNames::kJsonMargin)) {
        editBox->setMargin(parseMargin(j[PropertyNames::kJsonMargin]));
    }

    parseEvents(editBox, j);
    parseBindings(editBox, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = editBox;
    }

    editBox->create();
    return editBox;
}

// ==================== ComboBox ====================

shared_ptr<ComboBox> LayoutParser::parseComboBox(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto combo = make_shared<ComboBox>(parent, rect, xScale, yScale);

    m_theme.applyCommonColors(combo, PropertyNames::kThemeCatEditBox);
    combo->setFont(m_theme.getFontName(PropertyNames::kThemeCatEditBox));
    combo->setFontSize(m_theme.getFontSize(PropertyNames::kThemeCatEditBox));
    parseCommonProperties(combo, j);

    if (j.contains(PropertyNames::kText) && j[PropertyNames::kText].is_string()) {
        combo->setText(j[PropertyNames::kText].get<string>());
    }

    if (j.contains(PropertyNames::kEditable) && j[PropertyNames::kEditable].is_boolean()) {
        combo->setEditable(j[PropertyNames::kEditable].get<bool>());
    }

    if (j.contains(PropertyNames::kPlaceholder) && j[PropertyNames::kPlaceholder].is_string()) {
        combo->setPlaceholder(j[PropertyNames::kPlaceholder].get<string>());
    }

    if (j.contains(PropertyNames::kItems) && j[PropertyNames::kItems].is_array()) {
        pushJsonPath(PropertyNames::kItems);
        vector<ComboBoxItem> items;
        for (size_t i = 0; i < j[PropertyNames::kItems].size(); ++i) {
            const json& ji = j[PropertyNames::kItems][i];
            ComboBoxItem item;
            item.label = ji.value(PropertyNames::kJsonLabel, "");
            item.value = ji.value(PropertyNames::kValue, item.label);
            item.disabled = ji.value(PropertyNames::kJsonDisabled, false);
            items.push_back(item);
        }
        combo->setItems(items);
        popJsonPath();
    }

    if (j.contains(PropertyNames::kSelectedIndex) && j[PropertyNames::kSelectedIndex].is_number()) {
        combo->setSelectedIndex(j[PropertyNames::kSelectedIndex].get<int>());
    }

    if (j.contains(PropertyNames::kArrowWidth) && j[PropertyNames::kArrowWidth].is_number()) {
        combo->setArrowWidth(j[PropertyNames::kArrowWidth].get<float>());
    }

    if (j.contains(PropertyNames::kItemHeight) && j[PropertyNames::kItemHeight].is_number()) {
        combo->setItemHeight(j[PropertyNames::kItemHeight].get<float>());
    }

    if (j.contains(PropertyNames::kMaxVisibleItems) && j[PropertyNames::kMaxVisibleItems].is_number()) {
        combo->setMaxVisibleItems(j[PropertyNames::kMaxVisibleItems].get<int>());
    }

    if (j.contains(PropertyNames::kCycleEnabled) && j[PropertyNames::kCycleEnabled].is_boolean()) {
        combo->setCycleEnabled(j[PropertyNames::kCycleEnabled].get<bool>());
    }

    if (j.contains(PropertyNames::kFont) && j[PropertyNames::kFont].is_object()) {
        pushJsonPath(PropertyNames::kFont);
        const json& font = j[PropertyNames::kFont];
        if (font.contains(PropertyNames::kJsonName) && font[PropertyNames::kJsonName].is_string()) {
            combo->setFont(parseFontName(font[PropertyNames::kJsonName].get<string>()));
        }
        if (font.contains(PropertyNames::kJsonSize) && font[PropertyNames::kJsonSize].is_number()) {
            combo->setFontSize(font[PropertyNames::kJsonSize].get<int>());
        }
        popJsonPath();
    }

    if (j.contains(PropertyNames::kJsonAlignment) && j[PropertyNames::kJsonAlignment].is_string()) {
        combo->setAlignmentMode(parseAlignment(j[PropertyNames::kJsonAlignment].get<string>()));
    }

    parseEvents(combo, j);
    parseBindings(combo, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = combo;
    }

    combo->create();
    return combo;
}

// ==================== Panel ====================

shared_ptr<Panel> LayoutParser::parsePanel(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto panel = make_shared<Panel>(parent, rect, xScale, yScale);

    m_theme.applyCommonColors(panel, PropertyNames::kThemeCatPanel);
    parseCommonProperties(panel, j);

    if (j.contains(PropertyNames::kTransparent) && j[PropertyNames::kTransparent].is_boolean()) {
        panel->setTransparent(j[PropertyNames::kTransparent].get<bool>());
    }

    if (j.contains(PropertyNames::kJsonBgColor)) {
        SColor bgColor = parseColor(j[PropertyNames::kJsonBgColor]);
        StateColor sc(StateColor::Type::Background);
        sc.setNormal(bgColor);
        panel->setBackgroundStateColor(sc);
    }

    if (j.contains(PropertyNames::kJsonBorderColor)) {
        SColor borderColor = parseColor(j[PropertyNames::kJsonBorderColor]);
        StateColor sc(StateColor::Type::Border);
        sc.setNormal(borderColor);
        panel->setBorderStateColor(sc);
    }

    // Panel has no events in Phase 1

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = panel;
    }

    parseChildren(panel, j);

    // Layout engine
    if (j.contains(PropertyNames::kLayout) && j[PropertyNames::kLayout].is_object()) {
        pushJsonPath(PropertyNames::kLayout);
        const json& layoutJson = j[PropertyNames::kLayout];
        string layoutType = layoutJson.value(PropertyNames::kJsonType, PropertyNames::kLayoutTypeHFlow);
        float gap = layoutJson.value(PropertyNames::kJsonGap, 0.0f);

        Margin padding{0,0,0,0};
        if (layoutJson.contains(PropertyNames::kPadding) && layoutJson[PropertyNames::kPadding].is_object()) {
            padding = parseMargin(layoutJson[PropertyNames::kPadding]);
        }

        shared_ptr<LayoutEngine> engine;
        if (layoutType == PropertyNames::kLayoutTypeVFlow) {
            engine = make_shared<VFlowLayout>(gap, padding);
        } else if (layoutType == PropertyNames::kLayoutTypeAnchor) {
            engine = make_shared<AnchorLayout>(padding);
        } else if (layoutType == PropertyNames::kLayoutTypeGrid) {
            auto gridEngine = make_shared<GridLayout>(gap, padding);
            if (layoutJson.contains(PropertyNames::kJsonColumns) && layoutJson[PropertyNames::kJsonColumns].is_array()) {
                vector<GridSize> cols;
                for (const auto& c : layoutJson[PropertyNames::kJsonColumns]) {
                    cols.push_back(parseGridSize(c));
                }
                gridEngine->setColumns(cols);
            }
            if (layoutJson.contains(PropertyNames::kJsonRows) && layoutJson[PropertyNames::kJsonRows].is_array()) {
                vector<GridSize> rows;
                for (const auto& r : layoutJson[PropertyNames::kJsonRows]) {
                    rows.push_back(parseGridSize(r));
                }
                gridEngine->setRows(rows);
            }
            engine = gridEngine;
        } else {
            engine = make_shared<HFlowLayout>(gap, padding);
        }
        panel->setLayoutEngine(engine);

        // Per-child layout properties——children 与 panelChildren 一一对应
        // （menu-bar 在容器 children 内现已正常挂树；若将来出现别的"跳过挂树"特判类型，
        // 需在此恢复"实际挂树计数"匹配，避免 flowWeight 等按 JSON 下标错绑）
        if (j.contains(PropertyNames::kJsonChildren) && j[PropertyNames::kJsonChildren].is_array()) {
            const json& children = j[PropertyNames::kJsonChildren];
            auto& panelChildren = panel->getChildren();
            for (size_t i = 0; i < children.size() && i < panelChildren.size(); ++i) {
                if (children[i].contains(PropertyNames::kJsonFlowWeight) && children[i][PropertyNames::kJsonFlowWeight].is_number()) {
                    float fw = children[i][PropertyNames::kJsonFlowWeight].get<float>();
                    FlowItemProps props;
                    props.flexWeight = fw;
                    panel->setChildFlowProps(panelChildren[i].get(), props);
                }
                if (children[i].contains(PropertyNames::kAnchor) && children[i][PropertyNames::kAnchor].is_string()) {
                    AnchorInfo info;
                    info.anchor = children[i][PropertyNames::kAnchor].get<string>();
                    if (children[i].contains(PropertyNames::kJsonAnchorOffset) && children[i][PropertyNames::kJsonAnchorOffset].is_object()) {
                        info.offset = parseMargin(children[i][PropertyNames::kJsonAnchorOffset]);
                    }
                    panel->setChildAnchorProps(panelChildren[i].get(), info);
                }
                if (children[i].contains(PropertyNames::kJsonGrid) && children[i][PropertyNames::kJsonGrid].is_object()) {
                    const json& g = children[i][PropertyNames::kJsonGrid];
                    GridItemProps props;
                    props.row = g.value(PropertyNames::kJsonRow, 0);
                    props.col = g.value(PropertyNames::kJsonCol, 0);
                    props.rowSpan = g.value(PropertyNames::kJsonRowSpan, 1);
                    props.colSpan = g.value(PropertyNames::kJsonColSpan, 1);
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


// ==================== StatusBar ====================

// parseStatusBar 暂时禁用（待 StatusBar 编译修复后恢复）
shared_ptr<Control> LayoutParser::parseStatusBar(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto bar = make_shared<StatusBar>(parent, rect, xScale, yScale);
    m_theme.applyCommonColors(bar, PropertyNames::kThemeCatPanel);
    parseCommonProperties(bar, j);

    if (j.contains(PropertyNames::kFontSize) && j[PropertyNames::kFontSize].is_number())
        bar->setFontSize(j[PropertyNames::kFontSize].get<float>());
    if (j.contains(PropertyNames::kItemHeight) && j[PropertyNames::kItemHeight].is_number())
        bar->setItemHeight(j[PropertyNames::kItemHeight].get<float>());

    if (j.contains(PropertyNames::kItems) && j[PropertyNames::kItems].is_array()) {
        pushJsonPath(PropertyNames::kItems);
        for (const auto& ij : j[PropertyNames::kItems]) {
            string id = ij.value(PropertyNames::kId, string());
            string text = ij.value(PropertyNames::kText, id);
            bool right = ij.value(PropertyNames::kAlign, "left") == "right";
            if (id.empty()) id = text;
            bar->addStatusItem(id, text, right);

            // icon → leadingControl（Actor 资源引用）
            if (ij.contains(PropertyNames::kItemIcon) && ij[PropertyNames::kItemIcon].is_string()) {
                string iconPath = ij[PropertyNames::kItemIcon].get<string>();
                shared_ptr<Actor> actor = nullptr;
                if (iconPath.rfind(PropertyNames::kProviderPrefix, 0) == 0) {
                    string rid = iconPath.substr(strlen(PropertyNames::kProviderPrefix));
                    actor = make_shared<Actor>(nullptr, rid, false, xScale, yScale);
                } else {
                    fs::path p(iconPath);
                    if (p.is_relative()) p = fs::path(Platform::GetBasePath()) / p;
                    actor = make_shared<Actor>(nullptr, p, false, xScale, yScale);
                }
                if (actor) {
                    actor->create();
                    bar->setStatusItemLeadingControl(id, actor);
                }
            }

            // menu → 内嵌 items 解析（复用 populateMenuPanel）
            if (ij.contains(PropertyNames::kJsonMenu) && ij[PropertyNames::kJsonMenu].is_object()) {
                const auto& mj = ij[PropertyNames::kJsonMenu];
                if (mj.contains(PropertyNames::kItems) && mj[PropertyNames::kItems].is_array()) {
                    auto panel = make_shared<MenuPanel>(nullptr, xScale, yScale);
                    populateMenuPanel(panel, mj[PropertyNames::kItems], xScale, yScale);
                    bar->setStatusItemMenu(id, panel);
                }
            }

            // onClick 事件
            if (ij.contains(PropertyNames::kJsonOnClick) && ij[PropertyNames::kJsonOnClick].is_string()) {
                string action = ij[PropertyNames::kJsonOnClick].get<string>();
                bar->setStatusItemOnClick(id, [action](shared_ptr<StatusItem>){});
            }
        }
        popJsonPath();
    }

    parseEvents(bar, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<std::string>()] = bar;

    bar->create();
    return bar;
}


// ==================== ColorPicker ====================

shared_ptr<ColorPicker> LayoutParser::parseColorPicker(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    auto cp = make_shared<ColorPicker>(parent, rect);

    m_theme.applyCommonColors(cp, PropertyNames::kThemeCatColorPicker);
    parseCommonProperties(cp, j);

    if (j.contains(PropertyNames::kColor))
        cp->setColor(parseColor(j[PropertyNames::kColor]));

    if (j.contains(PropertyNames::kJsonPresets) && j[PropertyNames::kJsonPresets].is_array()) {
        vector<SColor> colors;
        for (auto& c : j[PropertyNames::kJsonPresets])
            colors.push_back(parseColor(c));
        if (!colors.empty())
            cp->setPresetColors(colors);
    }

    if (j.contains(PropertyNames::kJsonPresetLayout) && j[PropertyNames::kJsonPresetLayout].is_object()) {
        const json& pl = j[PropertyNames::kJsonPresetLayout];
        int cols = pl.value(PropertyNames::kJsonPresetCols, ConstDef::COLORPICKER_PRESET_COLS);
        int rows = pl.value(PropertyNames::kJsonPresetRows, ConstDef::COLORPICKER_PRESET_ROWS);
        cp->setPresetLayout(cols, rows);
    }

    if (j.contains(PropertyNames::kJsonSwatchSize) && j[PropertyNames::kJsonSwatchSize].is_number())
        cp->setClosedSwatchSize(j[PropertyNames::kJsonSwatchSize].get<float>());

    if (j.contains(PropertyNames::kClosedFontSize) && j[PropertyNames::kClosedFontSize].is_number())
        cp->setClosedFontSize(j[PropertyNames::kClosedFontSize].get<int>());

    if (j.contains(PropertyNames::kJsonClosedTextColor))
        cp->setClosedTextColor(parseColor(j[PropertyNames::kJsonClosedTextColor]));

    if (j.contains(PropertyNames::kJsonPopupBGColor))
        cp->setPopupBGColor(parseColor(j[PropertyNames::kJsonPopupBGColor]));

    parseEvents(cp, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<string>()] = cp;

    cp->create();
    return cp;
}

// ==================== WinFrame ====================

shared_ptr<WinFrame> LayoutParser::parseWinFrame(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto winFrame = make_shared<WinFrame>(parent, rect, xScale, yScale);

    if (j.contains(PropertyNames::kTitle) && j[PropertyNames::kTitle].is_string()) {
        winFrame->setTitle(j[PropertyNames::kTitle].get<string>());
    }

    if (j.contains(PropertyNames::kEdgeMargin) && j[PropertyNames::kEdgeMargin].is_number()) {
        winFrame->setEdgeMargin(j[PropertyNames::kEdgeMargin].get<float>());
    }

    if (j.contains(PropertyNames::kResizable) && j[PropertyNames::kResizable].is_boolean()) {
        winFrame->setResizable(j[PropertyNames::kResizable].get<bool>());
    }

    // Color parsing
    if (j.contains(PropertyNames::kJsonColors) && j[PropertyNames::kJsonColors].is_object()) {
        const json& colors = j[PropertyNames::kJsonColors];
        if (colors.contains(PropertyNames::kBackground) && colors[PropertyNames::kBackground].is_object()) {
            winFrame->setBackgroundStateColor(parseStateColor(colors[PropertyNames::kBackground], StateColor::Type::Background));
        }
        if (colors.contains(PropertyNames::kBorder) && colors[PropertyNames::kBorder].is_object()) {
            winFrame->setBorderStateColor(parseStateColor(colors[PropertyNames::kBorder], StateColor::Type::Border));
        }
        if (colors.contains(PropertyNames::kTitleBar) && colors[PropertyNames::kTitleBar].is_object()) {
            const json& tb = colors[PropertyNames::kTitleBar];
            if (tb.contains(PropertyNames::kJsonBg) && tb[PropertyNames::kJsonBg].is_object()) {
                winFrame->getTitleBar()->setBackgroundStateColor(
                    parseStateColor(tb[PropertyNames::kJsonBg], StateColor::Type::Background));
            }
        }
        if (colors.contains(PropertyNames::kTitleText) && colors[PropertyNames::kTitleText].is_object()) {
            winFrame->getTitleLabel()->setTextStateColor(
                parseStateColor(colors[PropertyNames::kTitleText], StateColor::Type::Text));
        }
    }

    // Children go into ClientPanel
    if (j.contains(PropertyNames::kJsonChildren)) {
        parseChildren(static_pointer_cast<Control>(winFrame->getClientPanel()), j);
    }

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = winFrame;
    }

    // Events
    applyShadowDecl(std::static_pointer_cast<ControlImpl>(winFrame), j);   // #12：win-frame 不走 common，显式应用 shadow
    parseEvents(static_pointer_cast<ControlImpl>(winFrame), j);

    winFrame->create();
    winFrame->hide();
    return winFrame;
}

// ==================== MenuBar ====================

shared_ptr<MenuBar> LayoutParser::parseMenuBar(const json& j, Control* parent) {
    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    // 使用 nullptr parent：MenuBar 独立于控件树，调用方负责添加到 BENCH 顶层
    auto menuBar = make_shared<MenuBar>(nullptr, xScale, yScale);

    m_theme.applyCommonColors(menuBar, PropertyNames::kThemeCatMenuBar);
    parseCommonProperties(menuBar, j);

    // rect：MenuBar 一直未解析通用 rect（历史遗留），容器内模式（自动手动定位）下
    // rect 生效（x/y/宽/高），顶层模式高度仍以 barHeight 为准（全宽贴顶布局覆盖 rect）
    if (j.contains(PropertyNames::kJsonRect) && j[PropertyNames::kJsonRect].is_object()) {
        pushJsonPath(PropertyNames::kJsonRect);
        menuBar->setRect(parseRect(j[PropertyNames::kJsonRect]));
        popJsonPath();
    }

    // font.size (static global setting)
    if (j.contains(PropertyNames::kFont) && j[PropertyNames::kFont].is_object()) {
        pushJsonPath(PropertyNames::kFont);
        if (j[PropertyNames::kFont].contains(PropertyNames::kJsonSize) && j[PropertyNames::kFont][PropertyNames::kJsonSize].is_number()) {
            float fontSize = (float)j[PropertyNames::kFont][PropertyNames::kJsonSize].get<int>();
            menuBar->setFontSize(fontSize);
            // auto-recalculate barHeight if not explicitly set。
            // 若 JSON 显式给出 rect.h（容器内优先经 rect 控高），则尊重 rect.h：
            // 容器内模式（自动手动定位）下 rect 生效，覆盖 auto（font.size×1.6）而非反向覆盖
            if (!j.contains(PropertyNames::kBarHeight)) {
                bool rectHasH = j.contains(PropertyNames::kJsonRect) &&
                                j[PropertyNames::kJsonRect].is_object() &&
                                j[PropertyNames::kJsonRect].contains(PropertyNames::kJsonH);
                if (!rectHasH) {
                    menuBar->setBarHeight(fontSize * 1.6f);
                }
            }
        }
        popJsonPath();
    }

    // barHeight (overrides auto-calculation from font.size)
    if (j.contains(PropertyNames::kBarHeight) && j[PropertyNames::kBarHeight].is_number()) {
        menuBar->setBarHeight(j[PropertyNames::kBarHeight].get<float>());
    }

    // manualPosition（手动定位：跳过全宽布局，setRect 自由生效——同屏多 MenuBar / 缩放对比）
    if (j.contains(PropertyNames::kManualPosition) && j[PropertyNames::kManualPosition].is_boolean()) {
        menuBar->setManualPosition(j[PropertyNames::kManualPosition].get<bool>());
        // manual 模式在通用 rect 解析之后才生效，重放 rect 使自由定位立即落地
        if (j.contains(PropertyNames::kJsonRect) && j[PropertyNames::kJsonRect].is_object()) {
            menuBar->setRect(parseRect(j[PropertyNames::kJsonRect]));
        }
    }

    // menus array
    if (j.contains(PropertyNames::kJsonMenus) && j[PropertyNames::kJsonMenus].is_array()) {
        pushJsonPath(PropertyNames::kJsonMenus);
        const json& menus = j[PropertyNames::kJsonMenus];
        for (size_t i = 0; i < menus.size(); ++i) {
            const json& menuJson = menus[i];
            string caption = menuJson.value(PropertyNames::kCaption, PropertyNames::kDefaultMenuTitle);

            if (menuJson.contains(PropertyNames::kItems) && menuJson[PropertyNames::kItems].is_array()) {
                auto panel = make_shared<MenuPanel>(nullptr, xScale, yScale);
                if (menuJson.contains(PropertyNames::kFont) && menuJson[PropertyNames::kFont].is_object()) {
                    if (menuJson[PropertyNames::kFont].contains(PropertyNames::kJsonSize) &&
                        menuJson[PropertyNames::kFont][PropertyNames::kJsonSize].is_number()) {
                        panel->setFontSize((float)menuJson[PropertyNames::kFont][PropertyNames::kJsonSize].get<int>());
                    }
                    if (menuJson[PropertyNames::kFont].contains(PropertyNames::kJsonName) &&
                        menuJson[PropertyNames::kFont][PropertyNames::kJsonName].is_string()) {
                        panel->setFontName(FontNameFromString(menuJson[PropertyNames::kFont][PropertyNames::kJsonName].get<string>().c_str()));
                    }
                }
                if (menuJson.contains(PropertyNames::kItemHeightRatio) && menuJson[PropertyNames::kItemHeightRatio].is_number()) {
                    panel->setItemHeightRatio(menuJson[PropertyNames::kItemHeightRatio].get<float>());
                }
                populateMenuPanel(panel, menuJson[PropertyNames::kItems], xScale, yScale);
                menuBar->addMenu(caption, panel);
            } else {
                pushJsonPath("menus[" + to_string(i) + "]");
                logWarn("menu entry \"" + caption + "\" has no 'items' array, skipping");
                popJsonPath();
            }
        }
        popJsonPath();
    }

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = menuBar;
    }

    return menuBar;
}

void LayoutParser::populateMenuPanel(shared_ptr<MenuPanel> panel, const json& items, float xScale, float yScale) {
    for (const auto& itemJson : items) {
        // Separator
        if (itemJson.contains(PropertyNames::kJsonType) && itemJson[PropertyNames::kJsonType].is_string() &&
            itemJson[PropertyNames::kJsonType].get<string>() == PropertyNames::kMenuTypeSeparator) {
            panel->addSeparator();
            continue;
        }

        // Determine type: SubMenu if has nested "items"
        MenuItemType type = MenuItemType::Normal;
        if (itemJson.contains(PropertyNames::kItems) && itemJson[PropertyNames::kItems].is_array()) {
            type = MenuItemType::SubMenu;
        }

        auto item = make_shared<MenuItem>(panel.get(), type, xScale, yScale);

        // Caption
        if (itemJson.contains(PropertyNames::kCaption) && itemJson[PropertyNames::kCaption].is_string()) {
            item->setCaption(itemJson[PropertyNames::kCaption].get<string>());
        }

        // Item id（CABI item-id 属性定位的目标）
        if (itemJson.contains(PropertyNames::kId) && itemJson[PropertyNames::kId].is_string()) {
            item->setItemId(itemJson[PropertyNames::kId].get<string>());
        }

        // Shortcut
        if (itemJson.contains(PropertyNames::kShortcut) && itemJson[PropertyNames::kShortcut].is_string()) {
            item->setShortcut(itemJson[PropertyNames::kShortcut].get<string>());
        }

        // Checked
        if (itemJson.contains(PropertyNames::kChecked) && itemJson[PropertyNames::kChecked].is_boolean()) {
            item->setChecked(itemJson[PropertyNames::kChecked].get<bool>());
        }

        // Enabled
        if (itemJson.contains(PropertyNames::kEnabled) && itemJson[PropertyNames::kEnabled].is_boolean()) {
            item->setEnable(itemJson[PropertyNames::kEnabled].get<bool>());
        }

        // SubMenu (recursive)
        if (itemJson.contains(PropertyNames::kItems) && itemJson[PropertyNames::kItems].is_array()) {
            auto subPanel = make_shared<MenuPanel>(nullptr, xScale, yScale);
            populateMenuPanel(subPanel, itemJson[PropertyNames::kItems], xScale, yScale);
            item->setSubMenu(subPanel);
        }

        // Events (onClick)
        if (itemJson.contains(PropertyNames::kJsonEvents) && itemJson[PropertyNames::kJsonEvents].is_object()) {
            parseEvents(item, itemJson);
        }

        // Menu 增强：前置控件容器（复用控件 JSON type 分发；缺 rect 补 {0,0,0,0} 占位，
        // 容器 rect 由 MenuItem::draw 按 icon 区布局覆盖；parent 传 nullptr，由 setLeadingControl 挂树）
        if (itemJson.contains(PropertyNames::kJsonLeadingControl) && itemJson[PropertyNames::kJsonLeadingControl].is_object()) {
            json lcJson = itemJson[PropertyNames::kJsonLeadingControl];
            if (!lcJson.contains(PropertyNames::kJsonRect)) {
                lcJson[PropertyNames::kJsonRect] = {
                    {PropertyNames::kJsonX, 0}, {PropertyNames::kJsonY, 0},
                    {PropertyNames::kJsonW, 0}, {PropertyNames::kJsonH, 0}
                };
            }
            auto lc = parseControl(lcJson, nullptr, 0);
            if (lc) item->setLeadingControl(lc);
            else logWarn("menu item leadingControl parse failed, skipped");
        }
        if (itemJson.contains(PropertyNames::kJsonLeadingGap) && itemJson[PropertyNames::kJsonLeadingGap].is_number())
            item->setLeadingGap(itemJson[PropertyNames::kJsonLeadingGap].get<float>());
        if (itemJson.contains(PropertyNames::kJsonItemFont) && itemJson[PropertyNames::kJsonItemFont].is_string())
            item->setFontName(FontNameFromString(itemJson[PropertyNames::kJsonItemFont].get<string>().c_str()));
        if (itemJson.contains(PropertyNames::kJsonItemFontSize) && itemJson[PropertyNames::kJsonItemFontSize].is_number())
            item->setOwnFontSize(itemJson[PropertyNames::kJsonItemFontSize].get<int>());

        item->create();

        panel->addItem(item);
    }
}

// ==================== TextArea ====================

shared_ptr<TextArea> LayoutParser::parseTextArea(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto textArea = make_shared<TextArea>(parent, rect, xScale, yScale);

    m_theme.applyCommonColors(textArea, PropertyNames::kThemeCatTextArea);
    textArea->setFont(m_theme.getFontName(PropertyNames::kThemeCatTextArea));
    textArea->setFontSize(m_theme.getFontSize(PropertyNames::kThemeCatTextArea));
    parseCommonProperties(textArea, j);

    if (j.contains(PropertyNames::kText) && j[PropertyNames::kText].is_string()) {
        textArea->setText(j[PropertyNames::kText].get<string>());
    }

    if (j.contains(PropertyNames::kPlaceholder) && j[PropertyNames::kPlaceholder].is_string()) {
        textArea->setPlaceholder(j[PropertyNames::kPlaceholder].get<string>());
    }

    if (j.contains(PropertyNames::kWordWrap) && j[PropertyNames::kWordWrap].is_boolean()) {
        textArea->setWordWrap(j[PropertyNames::kWordWrap].get<bool>());
    }

    if (j.contains(PropertyNames::kLineHeight) && j[PropertyNames::kLineHeight].is_number()) {
        textArea->setLineHeight(j[PropertyNames::kLineHeight].get<int>());
    }

    if (j.contains(PropertyNames::kJsonScrollBarThickness) && j[PropertyNames::kJsonScrollBarThickness].is_number()) {
        textArea->setScrollBarThickness(j[PropertyNames::kJsonScrollBarThickness].get<float>());
    }

    if (j.contains(PropertyNames::kFont) && j[PropertyNames::kFont].is_object()) {
        pushJsonPath(PropertyNames::kFont);
        const json& font = j[PropertyNames::kFont];
        if (font.contains(PropertyNames::kJsonName) && font[PropertyNames::kJsonName].is_string()) {
            textArea->setFont(parseFontName(font[PropertyNames::kJsonName].get<string>()));
        }
        if (font.contains(PropertyNames::kJsonSize) && font[PropertyNames::kJsonSize].is_number()) {
            textArea->setFontSize(font[PropertyNames::kJsonSize].get<int>());
        }
        popJsonPath();
    }

    if (j.contains(PropertyNames::kJsonAlignment) && j[PropertyNames::kJsonAlignment].is_string()) {
        textArea->setAlignmentMode(parseAlignment(j[PropertyNames::kJsonAlignment].get<string>()));
    }

    if (j.contains(PropertyNames::kJsonMargin)) {
        textArea->setMargin(parseMargin(j[PropertyNames::kJsonMargin]));
    }

    parseEvents(textArea, j);
    parseBindings(textArea, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = textArea;
    }

    textArea->create();
    return textArea;
}

// ==================== CheckBox ====================

static CheckState parseCheckState(const string& s) {
    if (s == PropertyNames::kCheckChecked)        return CheckState::Checked;
    if (s == PropertyNames::kCheckIndeterminate)  return CheckState::Indeterminate;
    return CheckState::Unchecked;
}

static CheckBoxStyle parseCheckBoxStyle(const string& s) {
    if (s == PropertyNames::kStyleCross)   return CheckBoxStyle::Cross;
    if (s == PropertyNames::kStyleCircle)  return CheckBoxStyle::Circle;
    return CheckBoxStyle::Classic;
}

static CheckBoxLayout parseCheckBoxLayout(const string& s) {
    if (s == PropertyNames::kLayoutTextLeft) return CheckBoxLayout::TextLeft;
    return CheckBoxLayout::TextRight;
}

static CheckBoxVerticalAlign parseCheckBoxVerticalAlign(const string& s) {
    if (s == PropertyNames::kVAlignTop)    return CheckBoxVerticalAlign::Top;
    if (s == PropertyNames::kVAlignBottom) return CheckBoxVerticalAlign::Bottom;
    return CheckBoxVerticalAlign::Center;
}

shared_ptr<CheckBox> LayoutParser::parseCheckBox(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto checkBox = make_shared<CheckBox>(parent, rect, xScale, yScale);

    m_theme.applyCommonColors(checkBox, PropertyNames::kThemeCatCheckBox);
    parseCommonProperties(checkBox, j);

    if (j.contains(PropertyNames::kCaption) && j[PropertyNames::kCaption].is_string()) {
        checkBox->setStringProperty(PropertyNames::kCaption, j[PropertyNames::kCaption].get<string>().c_str());   // #15：经宿主字段
    }

    int cbFontSize = m_theme.getFontSize(PropertyNames::kThemeCatCheckBox);
    checkBox->setCaptionSize((float)cbFontSize);   // #15：经宿主字段（recreate 不回落）

    if (j.contains(PropertyNames::kFontSize) && j[PropertyNames::kFontSize].is_number()) {
        checkBox->setCaptionSize(j[PropertyNames::kFontSize].get<float>());   // #15：经宿主字段
    }

    if (j.contains(PropertyNames::kCheckState) && j[PropertyNames::kCheckState].is_string()) {
        checkBox->setCheckState(parseCheckState(j[PropertyNames::kCheckState].get<string>()));
    }

    if (j.contains(PropertyNames::kJsonStyle) && j[PropertyNames::kJsonStyle].is_string()) {
        checkBox->setStyle(parseCheckBoxStyle(j[PropertyNames::kJsonStyle].get<string>()));
    }

    if (j.contains(PropertyNames::kLayout) && j[PropertyNames::kLayout].is_string()) {
        checkBox->setLayout(parseCheckBoxLayout(j[PropertyNames::kLayout].get<string>()));
    }

    if (j.contains(PropertyNames::kVerticalAlign) && j[PropertyNames::kVerticalAlign].is_string()) {
        checkBox->setVerticalAlign(parseCheckBoxVerticalAlign(j[PropertyNames::kVerticalAlign].get<string>()));
    }

    if (j.contains(PropertyNames::kSizeRatio) && j[PropertyNames::kSizeRatio].is_number()) {
        checkBox->setSizeRatio(j[PropertyNames::kSizeRatio].get<float>());
    }

    if (j.contains(PropertyNames::kTriState) && j[PropertyNames::kTriState].is_boolean()) {
        checkBox->setTriStateEnabled(j[PropertyNames::kTriState].get<bool>());
    }

    // Theme CheckBox-specific colors (defaults)
    SColor themeCheck;
    if (m_theme.getColorOpt(PropertyNames::kThemeCheckboxCheck, themeCheck))
        checkBox->setCheckColor(themeCheck);
    SColor themeCross;
    if (m_theme.getColorOpt(PropertyNames::kThemeCheckboxCross, themeCross))
        checkBox->setCrossColor(themeCross);
    SColor themeIndet;
    if (m_theme.getColorOpt(PropertyNames::kThemeCheckboxIndeterminate, themeIndet))
        checkBox->setIndeterminateColor(themeIndet);
    SColor themeBoxBorder;
    if (m_theme.getColorOpt(PropertyNames::kThemeCheckboxBoxBorder, themeBoxBorder))
        checkBox->setBoxBorderColor(themeBoxBorder);

    // CheckBox-specific colors (JSON overrides)
    if (j.contains(PropertyNames::kJsonColors) && j[PropertyNames::kJsonColors].is_object()) {
        const json& colors = j[PropertyNames::kJsonColors];
        if (colors.contains(PropertyNames::kJsonCheckColor)) {
            checkBox->setCheckColor(parseColor(colors[PropertyNames::kJsonCheckColor]));
        }
        if (colors.contains(PropertyNames::kJsonCrossColor)) {
            checkBox->setCrossColor(parseColor(colors[PropertyNames::kJsonCrossColor]));
        }
        if (colors.contains(PropertyNames::kJsonIndeterminateColor)) {
            checkBox->setIndeterminateColor(parseColor(colors[PropertyNames::kJsonIndeterminateColor]));
        }
        if (colors.contains(PropertyNames::kJsonBoxBorderColor)) {
            checkBox->setBoxBorderColor(parseColor(colors[PropertyNames::kJsonBoxBorderColor]));
        }
    }

    parseEvents(checkBox, j);
    parseBindings(checkBox, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = checkBox;
    }

    checkBox->create();
    return checkBox;
}

// ==================== ProgressBar ====================

static ProgressBarStyle parseProgressBarStyle(const string& s) {
    if (s == PropertyNames::kOrientVertical) return ProgressBarStyle::Vertical;
    return ProgressBarStyle::Horizontal;
}

static ProgressBarTextMode parseProgressBarTextMode(const string& s) {
    if (s == PropertyNames::kTextModeNone)    return ProgressBarTextMode::None;
    if (s == PropertyNames::kTextModeCustom)  return ProgressBarTextMode::Custom;
    return ProgressBarTextMode::Percent;
}

shared_ptr<ProgressBar> LayoutParser::parseProgressBar(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto progressBar = make_shared<ProgressBar>(parent, rect, xScale, yScale);

    m_theme.applyCommonColors(progressBar, PropertyNames::kThemeCatProgressBar);
    progressBar->setFont(m_theme.getFontName(PropertyNames::kThemeCatProgressBar));
    progressBar->setFontSize(m_theme.getFontSize(PropertyNames::kThemeCatProgressBar));
    parseCommonProperties(progressBar, j);

    if (j.contains(PropertyNames::kValue) && j[PropertyNames::kValue].is_number()) {
        progressBar->setValue(j[PropertyNames::kValue].get<float>());
    }

    if (j.contains(PropertyNames::kJsonRange) && j[PropertyNames::kJsonRange].is_object()) {
        float minVal = j[PropertyNames::kJsonRange].value(PropertyNames::kJsonMin, 0.0f);
        float maxVal = j[PropertyNames::kJsonRange].value(PropertyNames::kJsonMax, 100.0f);
        progressBar->setRange(minVal, maxVal);
    }

    if (j.contains(PropertyNames::kJsonStyle) && j[PropertyNames::kJsonStyle].is_string()) {
        progressBar->setStyle(parseProgressBarStyle(j[PropertyNames::kJsonStyle].get<string>()));
    }

    if (j.contains(PropertyNames::kTextMode) && j[PropertyNames::kTextMode].is_string()) {
        progressBar->setTextMode(parseProgressBarTextMode(j[PropertyNames::kTextMode].get<string>()));
    }

    if (j.contains(PropertyNames::kCustomText) && j[PropertyNames::kCustomText].is_string()) {
        progressBar->setCustomText(j[PropertyNames::kCustomText].get<string>());
    }

    if (j.contains(PropertyNames::kAnimationSpeed) && j[PropertyNames::kAnimationSpeed].is_number()) {
        progressBar->setAnimationSpeed(j[PropertyNames::kAnimationSpeed].get<float>());
    }

    if (j.contains(PropertyNames::kFont) && j[PropertyNames::kFont].is_object()) {
        pushJsonPath(PropertyNames::kFont);
        const json& font = j[PropertyNames::kFont];
        if (font.contains(PropertyNames::kJsonName) && font[PropertyNames::kJsonName].is_string()) {
            progressBar->setFont(parseFontName(font[PropertyNames::kJsonName].get<string>()));
        }
        if (font.contains(PropertyNames::kJsonSize) && font[PropertyNames::kJsonSize].is_number()) {
            progressBar->setFontSize(font[PropertyNames::kJsonSize].get<int>());
        }
        popJsonPath();
    }

    if (j.contains(PropertyNames::kJsonAlignment) && j[PropertyNames::kJsonAlignment].is_string()) {
        progressBar->setAlignmentMode(parseAlignment(j[PropertyNames::kJsonAlignment].get<string>()));
    }

    // Theme ProgressBar-specific colors (defaults)
    SColor themeProgress;
    if (m_theme.getColorOpt(PropertyNames::kThemeProgressbarProgress, themeProgress))
        progressBar->setProgressColor(themeProgress);
    SColor themeTrack;
    if (m_theme.getColorOpt(PropertyNames::kThemeProgressbarTrack, themeTrack))
        progressBar->setBackgroundColor(themeTrack);

    // ProgressBar-specific colors (JSON overrides)
    if (j.contains(PropertyNames::kJsonColors) && j[PropertyNames::kJsonColors].is_object()) {
        const json& colors = j[PropertyNames::kJsonColors];
        if (colors.contains(PropertyNames::kJsonProgressColor)) {
            progressBar->setProgressColor(parseColor(colors[PropertyNames::kJsonProgressColor]));
        }
        if (colors.contains(PropertyNames::kJsonBackgroundColor)) {
            progressBar->setBackgroundColor(parseColor(colors[PropertyNames::kJsonBackgroundColor]));
        }
    }

    parseEvents(progressBar, j);
    parseBindings(progressBar, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = progressBar;
    }

    progressBar->create();
    return progressBar;
}

// ==================== Slider ====================

static SliderStyle parseSliderStyle(const string& s) {
    if (s == PropertyNames::kOrientVertical) return SliderStyle::Vertical;
    return SliderStyle::Horizontal;
}

shared_ptr<Slider> LayoutParser::parseSlider(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    auto slider = make_shared<Slider>(parent, rect);

    if (j.contains(PropertyNames::kJsonRange) && j[PropertyNames::kJsonRange].is_object()) {
        float minVal = j[PropertyNames::kJsonRange].value(PropertyNames::kJsonMin, 0.0f);
        float maxVal = j[PropertyNames::kJsonRange].value(PropertyNames::kJsonMax, 100.0f);
        slider->setRange(minVal, maxVal);
    }

    if (j.contains(PropertyNames::kValue) && j[PropertyNames::kValue].is_number())
        slider->setValue(j[PropertyNames::kValue].get<float>());

    if (j.contains(PropertyNames::kStep) && j[PropertyNames::kStep].is_number())
        slider->setStep(j[PropertyNames::kStep].get<float>());

    if (j.contains(PropertyNames::kJsonStyle) && j[PropertyNames::kJsonStyle].is_string())
        slider->setStyle(parseSliderStyle(j[PropertyNames::kJsonStyle].get<string>()));

    if (j.contains(PropertyNames::kReverse) && j[PropertyNames::kReverse].is_boolean())
        slider->setReverse(j[PropertyNames::kReverse].get<bool>());

    if (j.contains(PropertyNames::kTrack) && j[PropertyNames::kTrack].is_object()) {
        const json& track = j[PropertyNames::kTrack];
        if (track.contains(PropertyNames::kThickness) && track[PropertyNames::kThickness].is_number())
            slider->setTrackThickness(track[PropertyNames::kThickness].get<float>());
        if (track.contains(PropertyNames::kColor) && track[PropertyNames::kColor].is_object())
            slider->setTrackColor(parseStateColor(track[PropertyNames::kColor], StateColor::Type::Background).getNormal());
        if (track.contains(PropertyNames::kJsonFillColor) && track[PropertyNames::kJsonFillColor].is_object())
            slider->setTrackFillColor(parseStateColor(track[PropertyNames::kJsonFillColor], StateColor::Type::Background).getNormal());
    }

    if (j.contains(PropertyNames::kThumb) && j[PropertyNames::kThumb].is_object()) {
        const json& thumb = j[PropertyNames::kThumb];
        if (thumb.contains(PropertyNames::kJsonSize) && thumb[PropertyNames::kJsonSize].is_number())
            slider->setThumbSize(thumb[PropertyNames::kJsonSize].get<float>());
        if (thumb.contains(PropertyNames::kColor) && thumb[PropertyNames::kColor].is_object())
            slider->setThumbColor(parseStateColor(thumb[PropertyNames::kColor], StateColor::Type::Background).getNormal());
        if (thumb.contains(PropertyNames::kJsonBorderColor) && thumb[PropertyNames::kJsonBorderColor].is_object())
            slider->setThumbBorderColor(parseStateColor(thumb[PropertyNames::kJsonBorderColor], StateColor::Type::Background).getNormal());
        if (thumb.contains(PropertyNames::kJsonHoverColor) && thumb[PropertyNames::kJsonHoverColor].is_object())
            slider->setThumbHoverColor(parseStateColor(thumb[PropertyNames::kJsonHoverColor], StateColor::Type::Background).getNormal());
    }

    if (j.contains(PropertyNames::kShowValueLabel) && j[PropertyNames::kShowValueLabel].is_boolean())
        slider->setShowValueLabel(j[PropertyNames::kShowValueLabel].get<bool>());

    if (j.contains(PropertyNames::kLabelFormat) && j[PropertyNames::kLabelFormat].is_string())
        slider->setLabelFormat(j[PropertyNames::kLabelFormat].get<string>());

    if (j.contains(PropertyNames::kLabelGap) && j[PropertyNames::kLabelGap].is_number())
        slider->setLabelGap(j[PropertyNames::kLabelGap].get<float>());

    if (j.contains(PropertyNames::kTick) && j[PropertyNames::kTick].is_object()) {
        const json& tick = j[PropertyNames::kTick];
        if (tick.contains(PropertyNames::kJsonInterval) && tick[PropertyNames::kJsonInterval].is_number())
            slider->setTickInterval(tick[PropertyNames::kJsonInterval].get<float>());
        if (tick.contains(PropertyNames::kJsonLength) && tick[PropertyNames::kJsonLength].is_number())
            slider->setTickLength(tick[PropertyNames::kJsonLength].get<float>());
        if (tick.contains(PropertyNames::kColor) && tick[PropertyNames::kColor].is_object())
            slider->setTickColor(parseStateColor(tick[PropertyNames::kColor], StateColor::Type::Background).getNormal());
    }

    parseCommonProperties(std::static_pointer_cast<ControlImpl>(slider), j);
    parseEvents(std::static_pointer_cast<ControlImpl>(slider), j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<string>()] = slider;

    slider->create();
    return slider;
}

// ==================== NumericUpDown ====================

shared_ptr<NumericUpDown> LayoutParser::parseNumericUpDown(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto nud = make_shared<NumericUpDown>(parent, rect, xScale, yScale);

    m_theme.applyCommonColors(nud, PropertyNames::kThemeCatNumericUpDown);
    parseCommonProperties(nud, j);

    if (j.contains(PropertyNames::kValue))
        nud->setValue(j[PropertyNames::kValue].get<double>());

    if (j.contains(PropertyNames::kJsonRange) && j[PropertyNames::kJsonRange].is_object()) {
        double mn = j[PropertyNames::kJsonRange].value(PropertyNames::kJsonMin, 0.0);
        double mx = j[PropertyNames::kJsonRange].value(PropertyNames::kJsonMax, 100.0);
        nud->setRange(mn, mx);
    }

    if (j.contains(PropertyNames::kStep))
        nud->setStep(j[PropertyNames::kStep].get<double>());

    if (j.contains(PropertyNames::kPageStep))
        nud->setPageStep(j[PropertyNames::kPageStep].get<double>());

    if (j.contains(PropertyNames::kDecimals))
        nud->setDecimals(j[PropertyNames::kDecimals].get<int>());

    if (j.contains(PropertyNames::kPlaceholder))
        nud->setPlaceholder(j[PropertyNames::kPlaceholder].get<string>());

    if (j.contains(PropertyNames::kReadOnly))
        nud->setReadOnly(j[PropertyNames::kReadOnly].get<bool>());

    if (j.contains(PropertyNames::kButtonWidth))
        nud->setButtonWidth(j[PropertyNames::kButtonWidth].get<float>());

    parseEvents(nud, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<string>()] = nud;

    nud->create();
    return nud;
}

// ==================== Splitter ====================

shared_ptr<Splitter> LayoutParser::parseSplitter(const json& j, Control* parent) {
    SRect rect = {0, 0, 10, 10};
    if (j.contains(PropertyNames::kJsonRect)) rect = parseRect(j[PropertyNames::kJsonRect]);

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto sp = make_shared<Splitter>(parent, rect, xScale, yScale);
    m_theme.applyCommonColors(sp, PropertyNames::kThemeCatSplitter);
    parseCommonProperties(sp, j);

    if (j.contains(PropertyNames::kOrientation)) {
        string orient = j[PropertyNames::kOrientation].get<string>();
        sp->setOrientation(orient == PropertyNames::kOrientVertical);
    }

    if (j.contains(PropertyNames::kJsonFirstPanel) && j.contains(PropertyNames::kJsonSecondPanel)) {
        string firstId = j[PropertyNames::kJsonFirstPanel].get<string>();
        string secondId = j[PropertyNames::kJsonSecondPanel].get<string>();
        auto first = findControlById(firstId);
        auto second = findControlById(secondId);
        if (first && second) {
            sp->setLinkedControls(first, second);
            SRect fr = first->getRect();
            if (sp->isHorizontal()) {
                sp->setRect({fr.left + fr.width, fr.top,
                             sp->getThickness(), fr.height});
            } else {
                sp->setRect({fr.left, fr.top + fr.height,
                             fr.width, sp->getThickness()});
            }
        }
    }

    if (j.contains(PropertyNames::kJsonMinFirst))
        sp->setMinSize(j[PropertyNames::kJsonMinFirst].get<float>(), sp->getMinSecond());
    if (j.contains(PropertyNames::kJsonMinSecond))
        sp->setMinSize(sp->getMinFirst(), j[PropertyNames::kJsonMinSecond].get<float>());
    if (j.contains(PropertyNames::kThickness))
        sp->setThickness(j[PropertyNames::kThickness].get<float>());
    if (j.contains(PropertyNames::kRatio))
        sp->setSplitRatio(j[PropertyNames::kRatio].get<float>());

    parseEvents(sp, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<string>()] = sp;

    sp->create();
    return sp;
}

// ==================== TreeView ====================

shared_ptr<TreeView> LayoutParser::parseTreeView(const json& j, Control* parent) {
    SRect rect = {0, 0, 200, 300};
    if (j.contains(PropertyNames::kJsonRect)) rect = parseRect(j[PropertyNames::kJsonRect]);

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto tv = make_shared<TreeView>(parent, rect, xScale, yScale);
    m_theme.applyCommonColors(tv, PropertyNames::kThemeCatTreeView);
    parseCommonProperties(tv, j);

    if (j.contains(PropertyNames::kIndentWidth))
        tv->setIndentWidth(j[PropertyNames::kIndentWidth].get<float>());
    if (j.contains(PropertyNames::kRowHeight))
        tv->setRowHeight(j[PropertyNames::kRowHeight].get<float>());
    if (j.contains(PropertyNames::kCycleNavigation))
        tv->setCycleNavigation(j[PropertyNames::kCycleNavigation].get<bool>());
    if (j.contains(PropertyNames::kDefaultExpand))
        tv->setDefaultExpand(j[PropertyNames::kDefaultExpand].get<bool>());

    if (j.contains(PropertyNames::kItems) && j[PropertyNames::kItems].is_array()) {
        vector<shared_ptr<TreeNode>> items;
        function<void(const json&, vector<shared_ptr<TreeNode>>&)> parseItems;
        parseItems = [&](const json& arr, vector<shared_ptr<TreeNode>>& out) {
            for (const auto& item : arr) {
                auto node = make_shared<TreeNode>();
                node->id = item.value(PropertyNames::kId, "");
                node->label = item.value(PropertyNames::kJsonLabel, "");
                node->expanded = item.value(PropertyNames::kJsonExpanded, false);
                if (item.contains(PropertyNames::kJsonLeadingGap) && item[PropertyNames::kJsonLeadingGap].is_number())
                    node->leadingGap = item[PropertyNames::kJsonLeadingGap].get<float>();
                if (item.contains(PropertyNames::kJsonAlignment) && item[PropertyNames::kJsonAlignment].is_string())
                    node->leadingAlign = parseAlignment(item[PropertyNames::kJsonAlignment].get<string>());
                if (item.contains(PropertyNames::kJsonItemFont) && item[PropertyNames::kJsonItemFont].is_string())
                    node->fontName = FontNameFromString(item[PropertyNames::kJsonItemFont].get<string>().c_str());
                if (item.contains(PropertyNames::kJsonItemFontSize) && item[PropertyNames::kJsonItemFontSize].is_number())
                    node->fontSize = item[PropertyNames::kJsonItemFontSize].get<int>();
                if (item.contains(PropertyNames::kJsonLeadingControl) && item[PropertyNames::kJsonLeadingControl].is_object()) {
                    // 前置控件容器：复用控件 JSON（type + 控件属性），
                    // parent 传 nullptr，由 TreeView::syncRowControls 挂树（create 在挂树后重放）
                    json lcJson = item[PropertyNames::kJsonLeadingControl];
                    if (!lcJson.contains(PropertyNames::kJsonRect)) {
                        // 行控件 rect 由 TreeView 按行高/文字高自适应覆盖，解析期仅需占位
                        lcJson[PropertyNames::kJsonRect] = {
                            {PropertyNames::kJsonX, 0}, {PropertyNames::kJsonY, 0},
                            {PropertyNames::kJsonW, 0}, {PropertyNames::kJsonH, 0}
                        };
                    }
                    auto lc = parseControl(lcJson, nullptr, 0);
                    if (lc) node->leadingControl = lc;
                    else logWarn("treeview item \"" + node->id + "\" leadingControl parse failed, skipped");
                }
                if (item.contains(PropertyNames::kJsonUserData) && item[PropertyNames::kJsonUserData].is_string()) {
                    node->userData = static_cast<void*>(new std::string(item[PropertyNames::kJsonUserData].get<string>()));
                    node->userDataOwned = true;
                }
                if (item.contains(PropertyNames::kJsonChildren) && item[PropertyNames::kJsonChildren].is_array()) {
                    parseItems(item[PropertyNames::kJsonChildren], node->children);
                }
                out.push_back(node);
            }
        };
        parseItems(j[PropertyNames::kItems], items);
        tv->setItems(items);
        tv->setOnClearNode([](shared_ptr<TreeView>, void* ud) {
            delete static_cast<std::string*>(ud);
        });
    }

    parseEvents(tv, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<string>()] = tv;

    tv->create();
    return tv;
}

// ==================== ScrollBar ====================

static ScrollBarOrientation parseScrollBarOrientation(const string& s) {
    if (s == PropertyNames::kOrientHorizontal) return ScrollBarOrientation::Horizontal;
    return ScrollBarOrientation::Vertical;
}

shared_ptr<ScrollBar> LayoutParser::parseScrollBar(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    ScrollBarOrientation orientation = ScrollBarOrientation::Vertical;
    if (j.contains(PropertyNames::kOrientation) && j[PropertyNames::kOrientation].is_string()) {
        orientation = parseScrollBarOrientation(j[PropertyNames::kOrientation].get<string>());
    }

    auto scrollBar = make_shared<ScrollBar>(parent, rect, orientation, xScale, yScale);

    m_theme.applyCommonColors(scrollBar, PropertyNames::kThemeCatScrollBar);
    parseCommonProperties(scrollBar, j);

    if (j.contains(PropertyNames::kValue) && j[PropertyNames::kValue].is_number()) {
        scrollBar->setValue(j[PropertyNames::kValue].get<float>());
    }

    if (j.contains(PropertyNames::kJsonRange) && j[PropertyNames::kJsonRange].is_object()) {
        float minVal = j[PropertyNames::kJsonRange].value(PropertyNames::kJsonMin, 0.0f);
        float maxVal = j[PropertyNames::kJsonRange].value(PropertyNames::kJsonMax, 100.0f);
        scrollBar->setRange(minVal, maxVal);
    }

    if (j.contains(PropertyNames::kPageSize) && j[PropertyNames::kPageSize].is_number()) {
        scrollBar->setPageSize(j[PropertyNames::kPageSize].get<float>());
    }

    if (j.contains(PropertyNames::kStepSize) && j[PropertyNames::kStepSize].is_number()) {
        scrollBar->setStepSize(j[PropertyNames::kStepSize].get<float>());
    }

    if (j.contains(PropertyNames::kThickness) && j[PropertyNames::kThickness].is_number()) {
        scrollBar->setThickness(j[PropertyNames::kThickness].get<float>());
    }

    parseEvents(scrollBar, j);
    parseBindings(scrollBar, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string()) {
        m_controlsById[j[PropertyNames::kId].get<string>()] = scrollBar;
    }

    scrollBar->create();
    return scrollBar;
}

// ==================== Popup ====================

shared_ptr<Popup> LayoutParser::parsePopup(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto popup = make_shared<Popup>(parent, rect, xScale, yScale);
    m_theme.applyCommonColors(popup, PropertyNames::kThemeCatPopup);
    parseCommonProperties(popup, j);

    // Popup-specific
    if (j.contains(PropertyNames::kCentered) && j[PropertyNames::kCentered].is_boolean())
        popup->setCentered();
    if (j.contains(PropertyNames::kCloseOnEsc) && j[PropertyNames::kCloseOnEsc].is_boolean())
        popup->setCloseOnEsc(j[PropertyNames::kCloseOnEsc].get<bool>());
    if (j.contains(PropertyNames::kCloseOnClickOutside) && j[PropertyNames::kCloseOnClickOutside].is_boolean())
        popup->setCloseOnClickOutside(j[PropertyNames::kCloseOnClickOutside].get<bool>());

    parseEvents(popup, j);
    parseBindings(popup, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<string>()] = popup;

    parseChildren(popup, j);
    popup->create();
    return popup;
}

// ==================== ConfirmPopup ====================

shared_ptr<ConfirmPopup> LayoutParser::parseConfirmPopup(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto cp = make_shared<ConfirmPopup>(parent, rect, xScale, yScale);
    m_theme.applyCommonColors(cp, PropertyNames::kThemeCatConfirmPopup);
    parseCommonProperties(cp, j);

    if (j.contains(PropertyNames::kCentered) && j[PropertyNames::kCentered].is_boolean())
        cp->setCentered();
    if (j.contains(PropertyNames::kCloseOnEsc) && j[PropertyNames::kCloseOnEsc].is_boolean())
        cp->setCloseOnEsc(j[PropertyNames::kCloseOnEsc].get<bool>());
    if (j.contains(PropertyNames::kCloseOnClickOutside) && j[PropertyNames::kCloseOnClickOutside].is_boolean())
        cp->setCloseOnClickOutside(j[PropertyNames::kCloseOnClickOutside].get<bool>());

    // confirm button
    if (j.contains(PropertyNames::kConfirmButton) && j[PropertyNames::kConfirmButton].is_object()) {
        const json& btn = j[PropertyNames::kConfirmButton];
        if (btn.contains(PropertyNames::kText) && btn[PropertyNames::kText].is_string())
            cp->setConfirmButtonText(btn[PropertyNames::kText].get<string>());
        if (btn.contains(PropertyNames::kJsonRect) && btn[PropertyNames::kJsonRect].is_object())
            cp->setConfirmButtonRect(parseRect(btn[PropertyNames::kJsonRect]));
        if (btn.contains(PropertyNames::kVisible) && btn[PropertyNames::kVisible].is_boolean())
            cp->setConfirmButtonVisible(btn[PropertyNames::kVisible].get<bool>());
    }

    if (j.contains(PropertyNames::kButtonHeight) && j[PropertyNames::kButtonHeight].is_number())
        cp->setButtonHeight(j[PropertyNames::kButtonHeight].get<float>());
    if (j.contains(PropertyNames::kButtonGap) && j[PropertyNames::kButtonGap].is_number())
        cp->setButtonGap(j[PropertyNames::kButtonGap].get<float>());
    if (j.contains(PropertyNames::kPadding) && j[PropertyNames::kPadding].is_number())
        cp->setPadding(j[PropertyNames::kPadding].get<float>());

    parseEvents(cp, j);
    parseBindings(cp, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<string>()] = cp;

    parseChildren(cp, j);
    cp->create();
    return cp;
}

// ==================== Dialog ====================

shared_ptr<Dialog> LayoutParser::parseDialog(const json& j, Control* parent) {
    pushJsonPath(PropertyNames::kJsonRect);
    SRect rect = parseRect(j[PropertyNames::kJsonRect]);
    popJsonPath();

    float xScale = 1.0f, yScale = 1.0f;
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        xScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        yScale = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
    }

    auto dlg = make_shared<Dialog>(parent, rect, xScale, yScale);
    m_theme.applyCommonColors(dlg, PropertyNames::kThemeCatDialog);
    parseCommonProperties(dlg, j);

    if (j.contains(PropertyNames::kCentered) && j[PropertyNames::kCentered].is_boolean())
        dlg->setCentered();
    if (j.contains(PropertyNames::kCloseOnEsc) && j[PropertyNames::kCloseOnEsc].is_boolean())
        dlg->setCloseOnEsc(j[PropertyNames::kCloseOnEsc].get<bool>());
    if (j.contains(PropertyNames::kCloseOnClickOutside) && j[PropertyNames::kCloseOnClickOutside].is_boolean())
        dlg->setCloseOnClickOutside(j[PropertyNames::kCloseOnClickOutside].get<bool>());

    // confirm button
    if (j.contains(PropertyNames::kConfirmButton) && j[PropertyNames::kConfirmButton].is_object()) {
        const json& btn = j[PropertyNames::kConfirmButton];
        if (btn.contains(PropertyNames::kText) && btn[PropertyNames::kText].is_string())
            dlg->setConfirmButtonText(btn[PropertyNames::kText].get<string>());
        if (btn.contains(PropertyNames::kJsonRect) && btn[PropertyNames::kJsonRect].is_object())
            dlg->setConfirmButtonRect(parseRect(btn[PropertyNames::kJsonRect]));
        if (btn.contains(PropertyNames::kVisible) && btn[PropertyNames::kVisible].is_boolean())
            dlg->setConfirmButtonVisible(btn[PropertyNames::kVisible].get<bool>());
    }

    // cancel button
    if (j.contains(PropertyNames::kCancelButton) && j[PropertyNames::kCancelButton].is_object()) {
        const json& btn = j[PropertyNames::kCancelButton];
        if (btn.contains(PropertyNames::kText) && btn[PropertyNames::kText].is_string())
            dlg->setCancelButtonText(btn[PropertyNames::kText].get<string>());
        if (btn.contains(PropertyNames::kJsonRect) && btn[PropertyNames::kJsonRect].is_object())
            dlg->setCancelButtonRect(parseRect(btn[PropertyNames::kJsonRect]));
    }

    if (j.contains(PropertyNames::kButtonHeight) && j[PropertyNames::kButtonHeight].is_number())
        dlg->setButtonHeight(j[PropertyNames::kButtonHeight].get<float>());
    if (j.contains(PropertyNames::kButtonGap) && j[PropertyNames::kButtonGap].is_number())
        dlg->setButtonGap(j[PropertyNames::kButtonGap].get<float>());
    if (j.contains(PropertyNames::kPadding) && j[PropertyNames::kPadding].is_number())
        dlg->setPadding(j[PropertyNames::kPadding].get<float>());

    parseEvents(dlg, j);
    parseBindings(dlg, j);

    if (j.contains(PropertyNames::kId) && j[PropertyNames::kId].is_string())
        m_controlsById[j[PropertyNames::kId].get<string>()] = dlg;

    parseChildren(dlg, j);
    dlg->create();
    // Dialog 默认隐藏：布局加载时不弹出，由事件驱动 open() 挂树显示；
    // 显式 visible:true 时立即打开（open() 以 getVisible()==false 为前提）
    dlg->setVisible(false);
    if (j.value(PropertyNames::kVisible, false)) {
        dlg->open();
    }
    return dlg;
}

// ==================== 通用属性解析 ====================

void LayoutParser::parseCommonProperties(shared_ptr<ControlImpl> ctrl, const json& j) {
    if (!ctrl) return;

    // scale
    if (j.contains(PropertyNames::kJsonScale) && j[PropertyNames::kJsonScale].is_object()) {
        float sx = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonX, 1.0f);
        float sy = j[PropertyNames::kJsonScale].value(PropertyNames::kJsonY, 1.0f);
        ctrl->setScaleX(sx);
        ctrl->setScaleY(sy);
    }

    // margin
    if (j.contains(PropertyNames::kJsonMargin)) {
        ctrl->setMargin(parseMargin(j[PropertyNames::kJsonMargin]));
    }

    // visible
    ctrl->setVisible(j.value(PropertyNames::kVisible, true));

    // enabled
    ctrl->setEnable(j.value(PropertyNames::kEnabled, true));

    // borderVisible
    if (j.contains(PropertyNames::kBorderVisible) && j[PropertyNames::kBorderVisible].is_boolean())
        ctrl->setBorderVisible(j[PropertyNames::kBorderVisible].get<bool>());

    // colors
    if (j.contains(PropertyNames::kJsonColors) && j[PropertyNames::kJsonColors].is_object()) {
        pushJsonPath(PropertyNames::kJsonColors);
        const json& colors = j[PropertyNames::kJsonColors];
        if (colors.contains(PropertyNames::kBackground)) {
            ctrl->setBackgroundStateColor(
                parseStateColor(colors[PropertyNames::kBackground], StateColor::Type::Background));
        }
        if (colors.contains(PropertyNames::kBorder)) {
            ctrl->setBorderStateColor(
                parseStateColor(colors[PropertyNames::kBorder], StateColor::Type::Border));
        }
        if (colors.contains(PropertyNames::kText)) {
            ctrl->setTextStateColor(
                parseStateColor(colors[PropertyNames::kText], StateColor::Type::Text));
        }
        if (colors.contains(PropertyNames::kTextShadow)) {
            ctrl->setTextShadowStateColor(
                parseStateColor(colors[PropertyNames::kTextShadow], StateColor::Type::TextShadow));
        }
        popJsonPath();
    }

    // 字体键（font{name,size} / font-size / fontSize）统一解析（显式声明）
    applyFontDecl(ctrl, j);

    applyShadowDecl(ctrl, j);
}

// #12：通用 shadow 对象解析（{enabled, offset:{x,y}}；经属性系统转发内部 Label）
void LayoutParser::applyShadowDecl(std::shared_ptr<ControlImpl> ctl, const json& j) {
    if (!ctl) return;
    if (!(j.contains(PropertyNames::kShadow) && j[PropertyNames::kShadow].is_object())) return;
    const json& sh = j[PropertyNames::kShadow];
    if (sh.contains(PropertyNames::kEnabled) && sh[PropertyNames::kEnabled].is_boolean())
        ctl->setBoolProperty(PropertyNames::kShadow, sh[PropertyNames::kEnabled].get<bool>() ? 1 : 0);
    if (sh.contains(PropertyNames::kJsonOffset) && sh[PropertyNames::kJsonOffset].is_object()) {
        const json& off = sh[PropertyNames::kJsonOffset];
        if (off.contains(PropertyNames::kJsonX) && off[PropertyNames::kJsonX].is_number())
            ctl->setFloatProperty(PropertyNames::kShadowOffsetX, off[PropertyNames::kJsonX].get<float>());
        if (off.contains(PropertyNames::kJsonY) && off[PropertyNames::kJsonY].is_number())
            ctl->setFloatProperty(PropertyNames::kShadowOffsetY, off[PropertyNames::kJsonY].get<float>());
    }
}

// 应用字体到支持的控件（继承/显式共用；仅设置，不写 context）
static void ApplyFontToControl(Control* ctl, FontName name, float size, bool applyName) {
    if (!ctl) return;
    if (size > 0.0f) {
        if (auto* l = dynamic_cast<Label*>(ctl)) l->setFontSize((int)size);
        else if (auto* e = dynamic_cast<EditBox*>(ctl)) e->setFontSize((int)size);
        else if (auto* p = dynamic_cast<ProgressBar*>(ctl)) p->setFontSize((int)size);
        else if (auto* tr = dynamic_cast<TreeView*>(ctl)) tr->setFontSize((int)size);
        else if (auto* tc = dynamic_cast<TabControl*>(ctl)) tc->setFontSize(size);
        else if (auto* sb = dynamic_cast<StatusBar*>(ctl)) sb->setFontSize(size);
        else if (auto* mb = dynamic_cast<MenuBar*>(ctl)) mb->setFontSize(size);
        else if (auto* lv = dynamic_cast<ListView*>(ctl)) lv->setFontSize((int)size);
    }
    // 字体名仅当父显式声明 font.name 时覆盖（未声明时沿用控件自身默认字体名）
    if (!applyName) return;
    if (auto* l = dynamic_cast<Label*>(ctl)) l->setFont(name);
    else if (auto* e = dynamic_cast<EditBox*>(ctl)) e->setFont(name);
    else if (auto* p = dynamic_cast<ProgressBar*>(ctl)) p->setFont(name);
    else if (auto* tr = dynamic_cast<TreeView*>(ctl)) tr->setFont(name);
    else if (auto* lv = dynamic_cast<ListView*>(ctl)) lv->setFont(name);
}

void LayoutParser::applyFontDecl(shared_ptr<ControlImpl> ctl, const json& j) {
    if (!ctl) return;
    bool hasName = false, hasSize = false;
    FontName name = ctl->getFontContextName();
    float size = 0.0f;

    if (j.contains(PropertyNames::kFont) && j[PropertyNames::kFont].is_object()) {
        const json& font = j[PropertyNames::kFont];
        if (font.contains(PropertyNames::kJsonName) && font[PropertyNames::kJsonName].is_string()) {
            name = FontNameFromString(font[PropertyNames::kJsonName].get<string>().c_str());
            hasName = true;
        }
        if (font.contains(PropertyNames::kJsonSize) && font[PropertyNames::kJsonSize].is_number()) {
            size = (float)font[PropertyNames::kJsonSize].get<int>();
            hasSize = true;
        }
    }
    if (j.contains(PropertyNames::kFontSize) && j[PropertyNames::kFontSize].is_number()) {
        size = (float)j[PropertyNames::kFontSize].get<float>();
        hasSize = true;
    }
    if (!hasName && !hasSize) return;
    ctl->setFontContext(name, size, true, hasName);
    ApplyFontToControl(ctl.get(), hasName ? name : ctl->getFontContextName(),
                       hasSize ? size : 0.0f, hasName);
}

void LayoutParser::resolveFontInheritance(Control* root) {
    if (!root) return;
    auto impl = dynamic_cast<ControlImpl*>(root);
    if (!impl) return;
    // 本控件 effective（显式 or 继承）；子无显式时沿用
    vector<shared_ptr<Control>>& kids = impl->getChildren();
    for (auto& child : kids) {
        auto cimpl = dynamic_cast<ControlImpl*>(child.get());
        if (!cimpl) continue;
        if (!cimpl->hasExplicitFont()) {
            // 从父继承最近显式字体（父若无显式、本身也是继承值）；仅父显式声明
            // font.name 时才覆盖子默认字体名，避免 fontSize-only 父把默认字体名强加给子
            ApplyFontToControl(child.get(),
                impl->getFontContextName(), impl->getFontContextSize(),
                impl->hasExplicitFontName());
            cimpl->setFontContext(impl->getFontContextName(), impl->getFontContextSize(),
                                  false, impl->hasExplicitFontName());
        }
        resolveFontInheritance(child.get());
    }
}

void LayoutParser::parseEvents(shared_ptr<ControlImpl> ctrl, const json& j) {
    if (!j.contains(PropertyNames::kJsonEvents) || j[PropertyNames::kJsonEvents].is_null() || !j[PropertyNames::kJsonEvents].is_object()) return;

    pushJsonPath(PropertyNames::kJsonEvents);
    const json& events = j[PropertyNames::kJsonEvents];

    // Button: onClick
    if (auto btn = dynamic_pointer_cast<Button>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeyClick) && events[PropertyNames::kEventKeyClick].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyClick].get<string>();
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
        if (events.contains(PropertyNames::kEventKeyClick) && events[PropertyNames::kEventKeyClick].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyClick].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                label->setOnClick([handler](shared_ptr<Label> sender) {
                    handler(sender);
                });
            }
        }
        if (events.contains(PropertyNames::kEventKeyPropertyChanged) && events[PropertyNames::kEventKeyPropertyChanged].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyPropertyChanged].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                label->setOnPropertyChanged([handler](shared_ptr<Label> sender) {
                    handler(sender);
                });
            }
        }
    }

    // EditBox & TextArea: onTextChanged, onEnter
    if (auto editBox = dynamic_pointer_cast<EditBox>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeyTextChanged) && events[PropertyNames::kEventKeyTextChanged].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyTextChanged].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                editBox->setOnTextChanged([handler](shared_ptr<Control> sender, string) {
                    handler(sender);
                });
            }
        }

        if (events.contains(PropertyNames::kEventKeyEnter) && events[PropertyNames::kEventKeyEnter].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyEnter].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                editBox->setOnEnter([handler](shared_ptr<Control> sender) {
                    handler(sender);
                });
            }
        }
    }

    // ComboBox: onSelectionChanged
    if (auto combo = dynamic_pointer_cast<ComboBox>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeySelectionChanged) && events[PropertyNames::kEventKeySelectionChanged].is_string()) {
            string handlerName = events[PropertyNames::kEventKeySelectionChanged].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                combo->setOnSelectionChanged([handler](shared_ptr<ComboBox> sender, int, const string&) {
                    handler(sender);
                });
            }
        }
    }

    // CheckBox: onCheckChanged
    if (auto cb = dynamic_pointer_cast<CheckBox>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeyCheckChanged) && events[PropertyNames::kEventKeyCheckChanged].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyCheckChanged].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                cb->setOnCheckChanged([handler](shared_ptr<CheckBox> sender, CheckState, CheckState) {
                    handler(sender);
                });
            }
        }
    }

    // TreeView: onSelect
    if (auto tv = dynamic_pointer_cast<TreeView>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeySelect) && events[PropertyNames::kEventKeySelect].is_string()) {
            string handlerName = events[PropertyNames::kEventKeySelect].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                tv->setOnSelect([handler, tv](shared_ptr<TreeView>, const string&) {
                    handler(tv);
                });
            }
        }
    }

    // ProgressBar: onValueChanged
    if (auto pb = dynamic_pointer_cast<ProgressBar>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeyValueChanged) && events[PropertyNames::kEventKeyValueChanged].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyValueChanged].get<string>();
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
        if (events.contains(PropertyNames::kEventKeyPositionChanged) && events[PropertyNames::kEventKeyPositionChanged].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyPositionChanged].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                sb->setOnPositionChanged([handler](shared_ptr<ScrollBar> sender, float, float, float, float) {
                    handler(sender);
                });
            }
        }
    }

    // Slider: onValueChanged
    if (auto sl = dynamic_pointer_cast<Slider>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeyValueChanged) && events[PropertyNames::kEventKeyValueChanged].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyValueChanged].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                sl->setOnValueChanged([handler](shared_ptr<Slider> sender, float) {
                    handler(sender);
                });
            }
        }
    }

    // ColorPicker: onColorChanged
    if (auto cp = dynamic_pointer_cast<ColorPicker>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeyColorChanged) && events[PropertyNames::kEventKeyColorChanged].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyColorChanged].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                cp->setOnColorChanged([handler](shared_ptr<ColorPicker>, const SColor&) {
                    handler(nullptr);
                });
            }
        }
    }

    // Dialog/ConfirmPopup/Popup: onConfirm, onCancel, onClose
    if (auto dlg = dynamic_pointer_cast<Dialog>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeyConfirm) && events[PropertyNames::kEventKeyConfirm].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyConfirm].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                dlg->setOnConfirm([handler](shared_ptr<ConfirmPopup> sender) {
                    handler(sender);
                });
            }
        }
        if (events.contains(PropertyNames::kEventKeyCancel) && events[PropertyNames::kEventKeyCancel].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyCancel].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                dlg->setOnCancel([handler](shared_ptr<Dialog> sender) {
                    handler(sender);
                });
            }
        }
        if (events.contains(PropertyNames::kEventKeyClose) && events[PropertyNames::kEventKeyClose].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyClose].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                dlg->setOnClose([handler](shared_ptr<Popup> sender, DialogResult) {
                    handler(sender);
                });
            }
        }
    } else if (auto cp = dynamic_pointer_cast<ConfirmPopup>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeyConfirm) && events[PropertyNames::kEventKeyConfirm].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyConfirm].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                cp->setOnConfirm([handler](shared_ptr<ConfirmPopup> sender) {
                    handler(sender);
                });
            }
        }
    } else if (auto pop = dynamic_pointer_cast<Popup>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeyClose) && events[PropertyNames::kEventKeyClose].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyClose].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                pop->setOnClose([handler](shared_ptr<Popup> sender, DialogResult) {
                    handler(sender);
                });
            }
        }
    }

    // MenuItem: onClick
    if (auto mi = dynamic_pointer_cast<MenuItem>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeyClick) && events[PropertyNames::kEventKeyClick].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyClick].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                mi->setOnClick([handler](shared_ptr<MenuItem> sender) {
                    handler(sender);
                });
            }
        }
    }

    // NumericUpDown: onValueChanged
    if (auto nud = dynamic_pointer_cast<NumericUpDown>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeyValueChanged) && events[PropertyNames::kEventKeyValueChanged].is_string()) {
            string handlerName = events[PropertyNames::kEventKeyValueChanged].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                nud->setOnValueChanged([handler](shared_ptr<NumericUpDown>, double) {
                    handler(nullptr);
                });
            }
        }
    }

    // Splitter: onSplitterMoved
    if (auto sp = dynamic_pointer_cast<Splitter>(ctrl)) {
        if (events.contains(PropertyNames::kEventKeySplitterMoved) && events[PropertyNames::kEventKeySplitterMoved].is_string()) {
            string handlerName = events[PropertyNames::kEventKeySplitterMoved].get<string>();
            auto it = m_handlers.find(handlerName);
            if (it != m_handlers.end()) {
                auto handler = it->second;
                sp->setOnSplitterMoved([handler](shared_ptr<Splitter>, float) {
                    handler(nullptr);
                });
            }
        }
    }

    popJsonPath();
}

static void applyBinding(shared_ptr<ControlImpl> ctrl, const string& prop, const DataValue& val) {
    if (prop == PropertyNames::kVisible) { ctrl->setVisible(val.asBool()); return; }
    if (prop == PropertyNames::kEnabled) { ctrl->setEnable(val.asBool()); return; }

    if (prop == PropertyNames::kCaption) {
        if (auto label = dynamic_pointer_cast<Label>(ctrl)) { label->setCaption(val.asString()); return; }
        if (auto btn = dynamic_pointer_cast<Button>(ctrl)) { btn->setCaption(val.asString()); return; }
    }
    if (prop == PropertyNames::kText) {
        if (auto eb = dynamic_pointer_cast<EditBox>(ctrl)) { eb->setText(val.asString()); return; }
        if (auto ta = dynamic_pointer_cast<TextArea>(ctrl)) { ta->setText(val.asString()); return; }
    }
    if (prop == PropertyNames::kPlaceholder) {
        if (auto eb = dynamic_pointer_cast<EditBox>(ctrl)) { eb->setPlaceholder(val.asString()); return; }
    }
    if (prop == PropertyNames::kValue) {
        if (auto pb = dynamic_pointer_cast<ProgressBar>(ctrl)) { pb->setValue((float)val.asDouble()); return; }
        if (auto sb = dynamic_pointer_cast<ScrollBar>(ctrl)) { sb->setValue((float)val.asDouble()); return; }
    }
    if (prop == PropertyNames::kCheckState) {
        if (auto cb = dynamic_pointer_cast<CheckBox>(ctrl)) {
            CheckState s = CheckState::Unchecked;
            string vs = val.asString();
            if (vs == PropertyNames::kCheckChecked) s = CheckState::Checked;
            else if (vs == PropertyNames::kCheckIndeterminate) s = CheckState::Indeterminate;
            cb->setCheckState(s);
            return;
        }
    }
}

static void bindProperty(DataContext* dataContext, shared_ptr<ControlImpl> ctrl, const string& prop, const string& source, const string& mode) {
    if (!dataContext) return;
    weak_ptr<ControlImpl> weakCtrl = ctrl;
    if (mode == PropertyNames::kBindModeOneWay || mode == PropertyNames::kBindModeTwoWay) {
        dataContext->watch(source, [weakCtrl, prop](const DataValue& val) {
            auto locked = dynamic_pointer_cast<ControlImpl>(weakCtrl.lock());
            if (locked) applyBinding(locked, prop, val);
        });
    }
    if (mode == PropertyNames::kBindModeTwoWay) {
        if (prop == PropertyNames::kText) {
            if (auto eb = dynamic_pointer_cast<EditBox>(ctrl)) {
                auto s = source;
                eb->setOnTextChanged([s, dataContext](shared_ptr<Control>, string text) {
                    dataContext->set(s, text);
                });
            }
        }
    if (prop == PropertyNames::kCheckState) {
            if (auto cb = dynamic_pointer_cast<CheckBox>(ctrl)) {
                auto s = source;
                shared_ptr<CheckBox> weakCB = cb;
                cb->setOnCheckChanged([s, dataContext, weakCB](shared_ptr<CheckBox>, CheckState, CheckState newState) {
                    string vs = PropertyNames::kCheckUnchecked;
                    if (newState == CheckState::Checked) vs = PropertyNames::kCheckChecked;
                    else if (newState == CheckState::Indeterminate) vs = PropertyNames::kCheckIndeterminate;
                    dataContext->set(s, vs);
                });
            }
        }
        if (prop == PropertyNames::kValue) {
            if (auto sb = dynamic_pointer_cast<ScrollBar>(ctrl)) {
                auto s = source;
                sb->setOnPositionChanged([s, dataContext](shared_ptr<ScrollBar>, float, float newValue, float, float) {
                    dataContext->set(s, (double)newValue);
                });
            }
        }
    }

    // (bindProperty end)
}

void LayoutParser::parseBindings(shared_ptr<ControlImpl> ctrl, const json& j) {
    if (!j.contains(PropertyNames::kJsonBind) || !j[PropertyNames::kJsonBind].is_object()) return;
    pushJsonPath(PropertyNames::kJsonBind);
    const json& bind = j[PropertyNames::kJsonBind];
    for (auto it = bind.begin(); it != bind.end(); ++it) {
        string prop = it.key();
        string source;
        string mode = PropertyNames::kBindModeOneWay;
        if (it.value().is_string()) {
            source = it.value().get<string>();
        } else if (it.value().is_object()) {
            source = it.value().value(PropertyNames::kJsonSource, "");
            mode = it.value().value(PropertyNames::kMode, PropertyNames::kBindModeOneWay);
        } else {
            continue;
        }
        bindProperty(m_dataContext, ctrl, prop, source, mode);
    }
    popJsonPath();
}

void LayoutParser::parseChildren(shared_ptr<Control> container, const json& j) {
    if (!j.contains(PropertyNames::kJsonChildren) || !j[PropertyNames::kJsonChildren].is_array()) return;

    pushJsonPath(PropertyNames::kJsonChildren);
    const json& children = j[PropertyNames::kJsonChildren];
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
    vector<string> missing;

    if (!j.is_object()) {
        pushJsonPath(PropertyNames::kJsonRect);
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

    parseField(PropertyNames::kJsonX, rect.left,   rect.leftIsPct,   rect.leftPct,   0.0f);
    parseField(PropertyNames::kJsonY, rect.top,    rect.topIsPct,    rect.topPct,    0.0f);
    parseField(PropertyNames::kJsonW, rect.width,  rect.widthIsPct,  rect.widthPct,  0.0f);
    parseField(PropertyNames::kJsonH, rect.height, rect.heightIsPct, rect.heightPct, 0.0f);

    if (!j.contains(PropertyNames::kJsonX)) missing.push_back(PropertyNames::kJsonX);
    if (!j.contains(PropertyNames::kJsonY)) missing.push_back(PropertyNames::kJsonY);
    if (!j.contains(PropertyNames::kJsonW)) missing.push_back(PropertyNames::kJsonW);
    if (!j.contains(PropertyNames::kJsonH)) missing.push_back(PropertyNames::kJsonH);

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
        pushJsonPath(PropertyNames::kJsonMargin);
        logWarn("'margin' must be an object, using defaults");
        popJsonPath();
        return margin;
    }

    margin.left   = j.value(PropertyNames::kJsonLeft,   0.0f);
    margin.top    = j.value(PropertyNames::kJsonTop,    0.0f);
    margin.right  = j.value(PropertyNames::kJsonRight,  0.0f);
    margin.bottom = j.value(PropertyNames::kJsonBottom, 0.0f);

    return margin;
}

SColor LayoutParser::parseColor(const json& j) {
    if (j.is_string()) {
        string hex = j.get<string>();
        if (hex.empty() || hex[0] != '#') {
            logWarn("invalid color format \"" + hex + "\", using default white");
            return SColor(255, 255, 255, 255);
        }
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
        } else {
            logWarn("invalid hex color length \"" + hex + "\", using default white");
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

StateColor LayoutParser::parseStateColor(const json& j, StateColor::Type type) {
    StateColor stateColor(type);

    if (j.is_null()) return stateColor;

    if (j.contains(PropertyNames::kStateKeyNormal)) {
        pushJsonPath(PropertyNames::kStateKeyNormal);
        stateColor.setNormal(parseColor(j[PropertyNames::kStateKeyNormal]));
        popJsonPath();
    }
    if (j.contains(PropertyNames::kStateKeyHover)) {
        pushJsonPath(PropertyNames::kStateKeyHover);
        stateColor.setHover(parseColor(j[PropertyNames::kStateKeyHover]));
        popJsonPath();
    }
    if (j.contains(PropertyNames::kStateKeyPressed)) {
        pushJsonPath(PropertyNames::kStateKeyPressed);
        stateColor.setPressed(parseColor(j[PropertyNames::kStateKeyPressed]));
        popJsonPath();
    }
    if (j.contains(PropertyNames::kStateKeyDisabled)) {
        pushJsonPath(PropertyNames::kStateKeyDisabled);
        stateColor.setDisabled(parseColor(j[PropertyNames::kStateKeyDisabled]));
        popJsonPath();
    }

    return stateColor;
}

FontName LayoutParser::parseFontName(const string& name) {
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

    logWarn("unknown font name \"" + name + "\", using default");
    return FontName::HarmonyOS_Sans_SC_Regular;
}

AlignmentMode LayoutParser::parseAlignment(const string& align) {
    static const unordered_map<string, AlignmentMode> alignMap = {
        {PropertyNames::kAlignLowerTopLeft,     AlignmentMode::AM_TOP_LEFT},
        {PropertyNames::kAlignLowerTopCenter,   AlignmentMode::AM_TOP_CENTER},
        {PropertyNames::kAlignLowerTopRight,    AlignmentMode::AM_TOP_RIGHT},
        {PropertyNames::kAlignLowerMidLeft,     AlignmentMode::AM_MID_LEFT},
        {PropertyNames::kAlignLowerCenter,      AlignmentMode::AM_CENTER},
        {PropertyNames::kAlignLowerMidRight,    AlignmentMode::AM_MID_RIGHT},
        {PropertyNames::kAlignLowerBottomLeft,  AlignmentMode::AM_BOTTOM_LEFT},
        {PropertyNames::kAlignLowerBottomCenter,AlignmentMode::AM_BOTTOM_CENTER},
        {PropertyNames::kAlignLowerBottomRight, AlignmentMode::AM_BOTTOM_RIGHT},
    };

    auto it = alignMap.find(align);
    if (it != alignMap.end()) return it->second;

    logWarn("unknown alignment \"" + align + "\", using TOP_LEFT");
    return AlignmentMode::AM_TOP_LEFT;
}

int LayoutParser::parseFontStyle(const json& styleNode) {
    static const unordered_map<string, int> styleMap = {
        {PropertyNames::kFontStyleNormal,        0},
        {PropertyNames::kFontStyleBold,          1},
        {PropertyNames::kFontStyleItalic,        2},
        {PropertyNames::kFontStyleUnderline,     4},
        {PropertyNames::kFontStyleStrikethrough, 8},
    };

    auto toBits = [&](const string& s) -> int {
        auto it = styleMap.find(s);
        return it != styleMap.end() ? it->second : 0;
    };

    // 数组（组合，如 ["bold","italic"]）：各元素位 OR
    if (styleNode.is_array()) {
        int bits = 0;
        for (const auto& e : styleNode)
            if (e.is_string()) bits |= toBits(e.get<string>());
        return bits;
    }
    // 字符串（单值，向后兼容 "bold"）
    if (styleNode.is_string()) return toBits(styleNode.get<string>());
    return 0;
}

GridSize LayoutParser::parseGridSize(const json& j) {
    GridSize gs;
    if (j.is_string()) {
        string s = j.get<string>();
        if (s == PropertyNames::kGridSizeAuto) {
            gs.type = GridSize::Auto;
        } else if (s.size() > 2 && s.substr(s.size() - 2) == PropertyNames::kGridSizeFrSuffix) {
            gs.type = GridSize::Flex;
            gs.value = stof(s.substr(0, s.size() - 2));
        } else if (s.size() > 2 && s.substr(s.size() - 2) == PropertyNames::kGridSizePxSuffix) {
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

// ==================== 组件系统 ====================

void LayoutParser::parseComponents(const json& j) {
    if (!j.contains(PropertyNames::kJsonComponents) || !j[PropertyNames::kJsonComponents].is_object()) return;

    pushJsonPath(PropertyNames::kJsonComponents);
    const json& comps = j[PropertyNames::kJsonComponents];

    // Known control type names to check against
    unordered_set<string> knownTypes = {
        PropertyNames::kControlTypeLabel, PropertyNames::kControlTypeButton, PropertyNames::kControlTypeEditBox,
        PropertyNames::kControlTypeComboBox, PropertyNames::kControlTypeTextArea, PropertyNames::kControlTypeCheckBox,
        PropertyNames::kControlTypeProgressBar, PropertyNames::kControlTypeScrollBar, PropertyNames::kControlTypePanel,
        PropertyNames::kControlTypeWinFrame, PropertyNames::kControlTypeMenuBar,
        PropertyNames::kControlTypeColorPicker, PropertyNames::kControlTypeSlider, PropertyNames::kControlTypePopup,
        PropertyNames::kControlTypeConfirmPopup, PropertyNames::kControlTypeDialog,
        PropertyNames::kControlTypeTreeView
    };

    for (auto it = comps.begin(); it != comps.end(); ++it) {
        const string& name = it.key();
        const json& def = it.value();

        if (knownTypes.find(name) != knownTypes.end()) {
            logWarn("component name \"" + name + "\" conflicts with built-in control type");
            continue;
        }

        if (!def.contains(PropertyNames::kJsonTemplate) || !def[PropertyNames::kJsonTemplate].is_object()) {
            logWarn("component \"" + name + "\" is missing 'template' object");
            continue;
        }

        if (def.contains(PropertyNames::kJsonProps) && !def[PropertyNames::kJsonProps].is_object()) {
            logWarn("component \"" + name + "\" 'props' must be an object");
            continue;
        }

        // Validate props
        if (def.contains(PropertyNames::kJsonProps)) {
            const json& props = def[PropertyNames::kJsonProps];
            bool propsValid = true;
            for (auto p = props.begin(); p != props.end(); ++p) {
                if (!p.value().contains(PropertyNames::kJsonType) || !p.value()[PropertyNames::kJsonType].is_string()) {
                    logWarn("component \"" + name + "\" prop \"" + p.key() + "\" missing 'type' field");
                    propsValid = false;
                } else {
                    string ptype = p.value()[PropertyNames::kJsonType].get<string>();
                    if (ptype != PropertyNames::kPropTypeString && ptype != PropertyNames::kPropTypeNumber && ptype != PropertyNames::kPropTypeBool) {
                        logWarn("component \"" + name + "\" prop \"" + p.key() + "\" invalid type \"" + ptype + "\"");
                        propsValid = false;
                    }
                }
            }
            if (!propsValid) continue;
        }

        m_components[name] = def;

        // Record source line number (approximate from JSON content position)
        ComponentSource src;
        src.name = name;
        // Try to get the component's approximate position in the raw content
        string searchStr = "\"" + name + "\"";
        size_t pos = m_rawJsonContent.find(searchStr);
        if (pos != string::npos) {
            src.lineStart = byteOffsetToLineNo(m_rawJsonContent, pos);
            src.lineEnd = src.lineStart;  // approximate
        } else {
            src.lineStart = 0;
            src.lineEnd = 0;
        }
        m_componentSourceLines[name] = src;
    }

    popJsonPath();
}

void LayoutParser::replacePlaceholders(json& node, const json& props, const json& instanceJ) {
    if (node.is_string()) {
        string val = node.get<string>();
        size_t start = val.find("{{");
        if (start != string::npos) {
            size_t end = val.find("}}", start);
            if (end != string::npos) {
                string propName = val.substr(start + 2, end - start - 2);
                // Look up in instanceJ first, then props default
                if (instanceJ.contains(propName)) {
                    const json& propVal = instanceJ[propName];
                    if (propVal.is_string()) {
                        val.replace(start, end - start + 2, propVal.get<string>());
                        node = val;
                    } else if (propVal.is_number()) {
                        val.replace(start, end - start + 2, to_string(propVal.get<double>()));
                        node = val;
                    } else if (propVal.is_boolean()) {
                        val.replace(start, end - start + 2, propVal.get<bool>() ? "true" : "false");
                        node = val;
                    }
                } else if (props.contains(propName) && props[propName].contains(PropertyNames::kJsonDefault)) {
                    // Use default value
                    const json& def = props[propName][PropertyNames::kJsonDefault];
                    if (def.is_string()) {
                        val.replace(start, end - start + 2, def.get<string>());
                        node = val;
                    } else if (def.is_number()) {
                        val.replace(start, end - start + 2, to_string(def.get<double>()));
                        node = val;
                    } else if (def.is_boolean()) {
                        val.replace(start, end - start + 2, def.get<bool>() ? "true" : "false");
                        node = val;
                    }
                } else {
                    logWarn("placeholder \"{{" + propName + "}}\" not provided and no default");
                }
            }
        }
    } else if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            replacePlaceholders(it.value(), props, instanceJ);
        }
    } else if (node.is_array()) {
        for (auto& item : node) {
            replacePlaceholders(item, props, instanceJ);
        }
    }
}

void LayoutParser::remapEvents(json& node, const json& instanceEvents) {
    if (node.is_object()) {
        // Check if this node has events
        if (node.contains(PropertyNames::kJsonEvents) && node[PropertyNames::kJsonEvents].is_object()) {
            json& events = node[PropertyNames::kJsonEvents];
            vector<string> keysToRemove;
            for (auto it = events.begin(); it != events.end(); ++it) {
                if (it.value().is_string()) {
                    string handlerName = it.value().get<string>();
                    if (handlerName.find("_comp_") == 0) {
                        // Internal event: _comp_onSearch -> look for onSearch in instanceEvents
                        string eventKey = handlerName.substr(6);  // Remove "_comp_"
                        if (instanceEvents.contains(eventKey) && instanceEvents[eventKey].is_string()) {
                            it.value() = instanceEvents[eventKey].get<string>();
                        } else {
                            logWarn("event \"_comp_" + eventKey + "\" has no mapping in instance events");
                        }
                    }
                }
            }
        }

        // Recurse into children
        for (auto it = node.begin(); it != node.end(); ++it) {
            remapEvents(it.value(), instanceEvents);
        }
    } else if (node.is_array()) {
        for (auto& item : node) {
            remapEvents(item, instanceEvents);
        }
    }
}

void LayoutParser::prefixIds(json& node, const string& prefix) {
    if (node.is_object()) {
        // Prefix the id if present
        if (node.contains(PropertyNames::kId) && node[PropertyNames::kId].is_string()) {
            string originalId = node[PropertyNames::kId].get<string>();
            node[PropertyNames::kId] = prefix + "__" + originalId;
        }

        // Recurse
        for (auto it = node.begin(); it != node.end(); ++it) {
            prefixIds(it.value(), prefix);
        }
    } else if (node.is_array()) {
        for (auto& item : node) {
            prefixIds(item, prefix);
        }
    }
}

shared_ptr<Control> LayoutParser::instantiateComponent(const string& name, const json& instanceJ, Control* parent, int index) {
    string indexPath = "controls[" + to_string(index) + "]";
    pushJsonPath(indexPath);

    // Detect circular reference
    if (find(m_instantiationStack.begin(), m_instantiationStack.end(), name) != m_instantiationStack.end()) {
        logWarn("circular component reference detected: \"" + name + "\"");
        popJsonPath();
        return nullptr;
    }

    m_instantiationStack.push_back(name);

    auto compIt = m_components.find(name);
    if (compIt == m_components.end()) {
        logWarn("component \"" + name + "\" not found");
        m_instantiationStack.pop_back();
        popJsonPath();
        return nullptr;
    }

    const json& compDef = compIt->second;
    const json& templateJ = compDef[PropertyNames::kJsonTemplate];
    const json props = compDef.contains(PropertyNames::kJsonProps) ? compDef[PropertyNames::kJsonProps] : json::object();

    // Get source line info for better error messages
    auto srcIt = m_componentSourceLines.find(name);
    string srcInfo = "";
    if (srcIt != m_componentSourceLines.end() && srcIt->second.lineStart > 0) {
        srcInfo = " [defined at line " + to_string(srcIt->second.lineStart) + "]";
    }

    pushJsonPath("component:" + name + srcInfo + " instantiated at");

    // Deep copy the template JSON
    json expanded = templateJ;

    // Replace placeholders
    replacePlaceholders(expanded, props, instanceJ);

    // Remap events: _comp_xxx -> instance events
    json instanceEvents = instanceJ.contains(PropertyNames::kJsonEvents) ? instanceJ[PropertyNames::kJsonEvents] : json::object();
    remapEvents(expanded, instanceEvents);

    // Prefix IDs for uniqueness (BEFORE injecting instance id/rect, so root ID isn't double-prefixed)
    string idPrefix = instanceJ.contains(PropertyNames::kId) && instanceJ[PropertyNames::kId].is_string()
        ? instanceJ[PropertyNames::kId].get<string>()
        : PropertyNames::kCompEventPrefix + name;
    prefixIds(expanded, idPrefix);

    // Inject instance attributes LAST so they override template/defaults without double-prefixing
    if (instanceJ.contains(PropertyNames::kId) && instanceJ[PropertyNames::kId].is_string()) {
        expanded[PropertyNames::kId] = instanceJ[PropertyNames::kId].get<string>();
    }
    if (instanceJ.contains(PropertyNames::kJsonRect) && instanceJ[PropertyNames::kJsonRect].is_object()) {
        expanded[PropertyNames::kJsonRect] = instanceJ[PropertyNames::kJsonRect];
    }
    // Forward other standard control attributes
    for (const string& attr : {"xScale", "yScale", "enable", "visible"}) {
        if (instanceJ.contains(attr)) {
            expanded[attr] = instanceJ[attr];
        }
    }

    // Parse the expanded template
    shared_ptr<Control> result = parseControl(expanded, parent, index);

    m_instantiationStack.pop_back();
    popJsonPath();
    popJsonPath();
    return result;
}
