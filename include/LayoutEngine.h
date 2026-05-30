#ifndef LayoutEngineH
#define LayoutEngineH

#include <memory>
#include <vector>
#include <unordered_map>
#include "ControlBase.h"

using namespace std;

struct FlowItemProps {
    float flexWeight = 1.0f;
};

class LayoutEngine {
public:
    virtual ~LayoutEngine() = default;
    virtual void apply(const SRect& containerRect,
                       vector<shared_ptr<Control>>& children,
                       unordered_map<Control*, FlowItemProps>& itemProps) = 0;
    virtual string getType() const = 0;
};

class HFlowLayout : public LayoutEngine {
private:
    float m_gap;
    Margin m_padding;
public:
    HFlowLayout(float gap = 0, Margin padding = Margin{0,0,0,0});
    void setGap(float gap) { m_gap = gap; }
    void setPadding(const Margin& padding) { m_padding = padding; }
    void apply(const SRect& containerRect,
               vector<shared_ptr<Control>>& children,
               unordered_map<Control*, FlowItemProps>& itemProps) override;
    string getType() const override { return "HFlow"; }
};

class VFlowLayout : public LayoutEngine {
private:
    float m_gap;
    Margin m_padding;
public:
    VFlowLayout(float gap = 0, Margin padding = Margin{0,0,0,0});
    void setGap(float gap) { m_gap = gap; }
    void setPadding(const Margin& padding) { m_padding = padding; }
    void apply(const SRect& containerRect,
               vector<shared_ptr<Control>>& children,
               unordered_map<Control*, FlowItemProps>& itemProps) override;
    string getType() const override { return "VFlow"; }
};

#endif
