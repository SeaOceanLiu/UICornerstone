#ifndef PanelH
#define PanelH

#include <functional>
#include <vector>
#include <unordered_map>
#include "ControlBase.h"
#include "SColor.h"
#include "Actor.h"
#include "LayoutEngine.h"

using namespace std;

class Panel: public ControlImpl {
    friend class PanelBuilder;
    using OnClickHandler = std::function<void (shared_ptr<Control>)>;
private:
    unordered_map<shared_ptr<Actor>, SPoint> m_actors;
    shared_ptr<LayoutEngine> m_layoutEngine;
    unordered_map<Control*, FlowItemProps> m_flowItemProps;
    unordered_map<Control*, AnchorInfo> m_anchorItemProps;
    unordered_map<Control*, GridItemProps> m_gridItemProps;
    string m_childTargetId;
    bool m_clipChildren = false;   // 子项裁剪（默认关；滚动容器/裁剪面板用，v1.1.1）
public:
    Panel(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);
    void update(void) override;
    void draw(void) override;
    bool handleEvent(shared_ptr<Event> event) override;

    void addControl(shared_ptr<Control> control) override;
    void removeControl(shared_ptr<Control> child) override;
    void removeAllControls();

    void setLayoutEngine(shared_ptr<LayoutEngine> engine) { m_layoutEngine = engine; }
    shared_ptr<LayoutEngine> getLayoutEngine() const { return m_layoutEngine; }
    void setClipChildren(bool clip) { m_clipChildren = clip; }
    bool getClipChildren() const { return m_clipChildren; }
    // ── Property system overrides ──
    int setBoolProperty(const char* prop, int value) override;
    int getBoolProperty(const char* prop, int& out) override;
    void setChildFlowProps(Control* child, FlowItemProps props) { m_flowItemProps[child] = props; }
    // 读取子项流权重（Splitter 引擎模式拖拽权重换算用；未设置 = 0 = 固定）
    float getChildFlowWeight(const Control* child) const {
        auto it = m_flowItemProps.find(const_cast<Control*>(child));
        return (it != m_flowItemProps.end()) ? it->second.flexWeight : 0.0f;
    }
    void setChildAnchorProps(Control* child, AnchorInfo props) { m_anchorItemProps[child] = props; }
    void setChildGridProps(Control* child, GridItemProps props) { m_gridItemProps[child] = props; }
    void reflowChildren();
    void resolveChildPercentages();
    int setStringProperty(const char* prop, const char* value) override;
    int setFloatProperty(const char* prop, float value) override;
    int setIntProperty(const char* prop, int value) override;
    int setEnumProperty(const char* prop, const char* value) override;
    // P0-59：运行期布局模式读回（layout）/ 锚点读回（child-id + anchor / anchor-offset-x/y）
    int getEnumProperty(const char* prop, const char*& out) override;
    int getFloatProperty(const char* prop, float& out) override;
    int getStringProperty(const char* prop, const char*& out) override;
    void setRect(SRect rect) override;
    void resized(SRect newRect) override;
};
class PanelBuilder {
private:
    shared_ptr<Panel> m_panel;
public:
    PanelBuilder(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);
    PanelBuilder& setBGColor(SColor color);
    PanelBuilder& setBorderColor(SColor color);
    PanelBuilder& setTransparent(bool isTransparent);
    PanelBuilder& setBorderVisible(bool isBorderVisible);
    PanelBuilder& addControl(shared_ptr<Control> control);
    shared_ptr<Panel> build(void);
};
#endif
