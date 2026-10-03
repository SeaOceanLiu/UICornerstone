// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#ifndef LayoutEngineH
#define LayoutEngineH

#include <memory>
#include <vector>
#include <unordered_map>
#include "ControlBase.h"
#include "PropertyNames.h"

using namespace std;

struct FlowItemProps {
    float flexWeight = 1.0f;
};

struct AnchorInfo {
    string anchor = PropertyNames::kAlignLowerTopLeft;
    Margin offset;
    bool   hasAnchor = false;   // P0-60：仅显式设过 anchor 才激活锚定管理（offset-only 不激活）
};

struct GridItemProps;

class LayoutEngine {
public:
    virtual ~LayoutEngine() = default;
    virtual void apply(const SRect& containerRect,
                       vector<shared_ptr<Control>>& children,
                       unordered_map<Control*, FlowItemProps>& itemProps) = 0;
    virtual void applyAnchor(const SRect& containerRect,
                             vector<shared_ptr<Control>>& children,
                             unordered_map<Control*, AnchorInfo>& anchorProps) {}
    virtual void applyGrid(const SRect& containerRect,
                           vector<shared_ptr<Control>>& children,
                           unordered_map<Control*, GridItemProps>& gridProps) {}
    virtual string getType() const = 0;
    // 最近一次流式分配的单位弹性宽（Splitter 引擎模式拖拽权重换算用；未经过 apply 时为 0）
    float getLastFlexUnit() const { return m_lastFlexUnit; }
protected:
    float m_lastFlexUnit = 0.0f;
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
    string getType() const override { return PropertyNames::kLayoutTypeHFlow; }
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
    string getType() const override { return PropertyNames::kLayoutTypeVFlow; }
};

// ==================== AnchorLayout ====================

// ==================== GridLayout ====================

struct GridSize {
    enum Type { Fixed, Flex, Auto };
    Type type = Fixed;
    float value = 0.0f;
};

struct GridItemProps {
    int row = 0;
    int col = 0;
    int rowSpan = 1;
    int colSpan = 1;
};

class GridLayout : public LayoutEngine {
private:
    vector<GridSize> m_columns;
    vector<GridSize> m_rows;
    float m_gap;
    Margin m_padding;
public:
    GridLayout(float gap = 0, Margin padding = Margin{0,0,0,0});
    void setColumns(const vector<GridSize>& cols) { m_columns = cols; }
    void setRows(const vector<GridSize>& rows) { m_rows = rows; }
    void applyGrid(const SRect& containerRect,
                   vector<shared_ptr<Control>>& children,
                   unordered_map<Control*, GridItemProps>& gridProps);
    void apply(const SRect& containerRect,
               vector<shared_ptr<Control>>& children,
               unordered_map<Control*, FlowItemProps>& itemProps) override {}
    string getType() const override { return PropertyNames::kLayoutTypeGrid; }
};

class AnchorLayout : public LayoutEngine {
private:
    Margin m_padding;
public:
    AnchorLayout(Margin padding = Margin{0,0,0,0});
    void setPadding(const Margin& padding) { m_padding = padding; }
    void apply(const SRect& containerRect,
               vector<shared_ptr<Control>>& children,
               unordered_map<Control*, FlowItemProps>& itemProps) override {}
    void applyAnchor(const SRect& containerRect,
                     vector<shared_ptr<Control>>& children,
                     unordered_map<Control*, AnchorInfo>& anchorProps) override;
    string getType() const override { return PropertyNames::kLayoutTypeAnchor; }
};

#endif
