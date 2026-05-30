#include "LayoutEngine.h"

HFlowLayout::HFlowLayout(float gap, Margin padding)
    : m_gap(gap), m_padding(padding) {}

void HFlowLayout::apply(const SRect& containerRect,
                         vector<shared_ptr<Control>>& children,
                         unordered_map<Control*, FlowItemProps>& itemProps) {
    float totalFlex = 0.0f;
    float fixedWidth = 0.0f;
    size_t flexCount = 0;

    // First pass: sum fixed widths and flex weights
    for (auto& child : children) {
        if (!child->getVisible()) continue;
        SRect childRect = child->getRect();
        Margin margin = child->getMargin();
        float hMargin = margin.left + margin.right;

        auto it = itemProps.find(child.get());
        float fw = (it != itemProps.end()) ? it->second.flexWeight : 0.0f;

        if (fw > 0.0f) {
            totalFlex += fw;
            ++flexCount;
        } else {
            fixedWidth += childRect.width + hMargin;
        }
    }

    float innerWidth = containerRect.width - m_padding.left - m_padding.right;
    float innerHeight = containerRect.height - m_padding.top - m_padding.bottom;

    float totalGaps = (children.size() > 1) ? m_gap * (children.size() - 1) : 0.0f;
    float remainingWidth = innerWidth - fixedWidth - totalGaps;
    float flexUnit = (totalFlex > 0.0f) ? max(0.0f, remainingWidth) / totalFlex : 0.0f;

    float cursorX = m_padding.left;

    for (auto& child : children) {
        if (!child->getVisible()) continue;
        SRect childRect = child->getRect();
        Margin margin = child->getMargin();

        float w = childRect.width;
        auto it = itemProps.find(child.get());
        float fw = (it != itemProps.end()) ? it->second.flexWeight : 0.0f;

        if (fw > 0.0f) {
            w = flexUnit * fw;
        }

        float childX = cursorX + margin.left;
        float childY = m_padding.top + margin.top;
        float childH = innerHeight - margin.top - margin.bottom;

        child->setRect(SRect{childX, childY, w, childH});

        cursorX += w + margin.left + margin.right + m_gap;
    }
}

// ==================== VFlowLayout ====================

VFlowLayout::VFlowLayout(float gap, Margin padding)
    : m_gap(gap), m_padding(padding) {}

void VFlowLayout::apply(const SRect& containerRect,
                         vector<shared_ptr<Control>>& children,
                         unordered_map<Control*, FlowItemProps>& itemProps) {
    float totalFlex = 0.0f;
    float fixedHeight = 0.0f;
    size_t flexCount = 0;

    // First pass: sum fixed heights and flex weights
    for (auto& child : children) {
        if (!child->getVisible()) continue;
        SRect childRect = child->getRect();
        Margin margin = child->getMargin();
        float vMargin = margin.top + margin.bottom;

        auto it = itemProps.find(child.get());
        float fw = (it != itemProps.end()) ? it->second.flexWeight : 0.0f;

        if (fw > 0.0f) {
            totalFlex += fw;
            ++flexCount;
        } else {
            fixedHeight += childRect.height + vMargin;
        }
    }

    float innerWidth = containerRect.width - m_padding.left - m_padding.right;
    float innerHeight = containerRect.height - m_padding.top - m_padding.bottom;

    float totalGaps = (children.size() > 1) ? m_gap * (children.size() - 1) : 0.0f;
    float remainingHeight = innerHeight - fixedHeight - totalGaps;
    float flexUnit = (totalFlex > 0.0f) ? max(0.0f, remainingHeight) / totalFlex : 0.0f;

    float cursorY = m_padding.top;

    for (auto& child : children) {
        if (!child->getVisible()) continue;
        SRect childRect = child->getRect();
        Margin margin = child->getMargin();

        float h = childRect.height;
        auto it = itemProps.find(child.get());
        float fw = (it != itemProps.end()) ? it->second.flexWeight : 0.0f;

        if (fw > 0.0f) {
            h = flexUnit * fw;
        }

        float childX = m_padding.left + margin.left;
        float childY = cursorY + margin.top;
        float childW = innerWidth - margin.left - margin.right;

        child->setRect(SRect{childX, childY, childW, h});

        cursorY += h + margin.top + margin.bottom + m_gap;
    }
}
