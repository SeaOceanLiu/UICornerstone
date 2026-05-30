// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
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

// ==================== GridLayout ====================

GridLayout::GridLayout(float gap, Margin padding)
    : m_gap(gap), m_padding(padding) {}

void GridLayout::applyGrid(const SRect& containerRect,
                            vector<shared_ptr<Control>>& children,
                            unordered_map<Control*, GridItemProps>& gridProps) {
    int numCols = (int)m_columns.size();
    int numRows = (int)m_rows.size();
    if (numCols == 0 || numRows == 0) return;

    float innerWidth = containerRect.width - m_padding.left - m_padding.right;
    float innerHeight = containerRect.height - m_padding.top - m_padding.bottom;

    vector<float> colWidths(numCols, 0.0f);
    vector<float> rowHeights(numRows, 0.0f);

    // ---- Phase 1: Auto columns - collect max child width per column ----
    for (size_t i = 0; i < numCols; ++i) {
        if (m_columns[i].type == GridSize::Fixed) {
            colWidths[i] = m_columns[i].value;
        } else if (m_columns[i].type == GridSize::Auto) {
            float maxW = 0.0f;
            for (auto& child : children) {
                if (!child->getVisible()) continue;
                auto it = gridProps.find(child.get());
                int col = (it != gridProps.end()) ? it->second.col : 0;
                int colSpan = (it != gridProps.end()) ? it->second.colSpan : 1;
                if (col <= (int)i && (int)i < col + colSpan) {
                    // Take child's initial width divided equally across spanned columns
                    float perCol = child->getRect().width / (float)colSpan;
                    if (perCol > maxW) maxW = perCol;
                }
            }
            colWidths[i] = maxW;
        }
    }

    // ---- Compute fixed+auto width sum and flex total ----
    float fixedAutoW = 0.0f;
    float totalFlex = 0.0f;
    for (int i = 0; i < numCols; ++i) {
        if (m_columns[i].type == GridSize::Flex) {
            totalFlex += m_columns[i].value;
        } else {
            fixedAutoW += colWidths[i];
        }
    }

    float totalGapsW = (numCols > 1) ? m_gap * (numCols - 1) : 0.0f;
    float remainingW = innerWidth - fixedAutoW - totalGapsW;
    float flexUnitW = (totalFlex > 0.0f) ? max(0.0f, remainingW) / totalFlex : 0.0f;

    for (int i = 0; i < numCols; ++i) {
        if (m_columns[i].type == GridSize::Flex) {
            colWidths[i] = flexUnitW * m_columns[i].value;
        }
    }

    // ---- Phase 2: Rows (same logic as columns) ----
    for (size_t i = 0; i < numRows; ++i) {
        if (m_rows[i].type == GridSize::Fixed) {
            rowHeights[i] = m_rows[i].value;
        } else if (m_rows[i].type == GridSize::Auto) {
            float maxH = 0.0f;
            for (auto& child : children) {
                if (!child->getVisible()) continue;
                auto it = gridProps.find(child.get());
                int row = (it != gridProps.end()) ? it->second.row : 0;
                int rowSpan = (it != gridProps.end()) ? it->second.rowSpan : 1;
                if (row <= (int)i && (int)i < row + rowSpan) {
                    float perRow = child->getRect().height / (float)rowSpan;
                    if (perRow > maxH) maxH = perRow;
                }
            }
            rowHeights[i] = maxH;
        }
    }

    float fixedAutoH = 0.0f;
    float totalFlexH = 0.0f;
    for (int i = 0; i < numRows; ++i) {
        if (m_rows[i].type == GridSize::Flex) {
            totalFlexH += m_rows[i].value;
        } else {
            fixedAutoH += rowHeights[i];
        }
    }

    float totalGapsH = (numRows > 1) ? m_gap * (numRows - 1) : 0.0f;
    float remainingH = innerHeight - fixedAutoH - totalGapsH;
    float flexUnitH = (totalFlexH > 0.0f) ? max(0.0f, remainingH) / totalFlexH : 0.0f;

    for (int i = 0; i < numRows; ++i) {
        if (m_rows[i].type == GridSize::Flex) {
            rowHeights[i] = flexUnitH * m_rows[i].value;
        }
    }

    // ---- Phase 3: Build cumulative offsets ----
    vector<float> colX(numCols + 1, 0.0f);
    colX[0] = m_padding.left;
    for (int i = 0; i < numCols; ++i) {
        colX[i + 1] = colX[i] + colWidths[i] + m_gap;
    }

    vector<float> rowY(numRows + 1, 0.0f);
    rowY[0] = m_padding.top;
    for (int i = 0; i < numRows; ++i) {
        rowY[i + 1] = rowY[i] + rowHeights[i] + m_gap;
    }

    // ---- Phase 4: Position children ----
    for (auto& child : children) {
        if (!child->getVisible()) continue;
        auto it = gridProps.find(child.get());
        int col = (it != gridProps.end()) ? it->second.col : 0;
        int row = (it != gridProps.end()) ? it->second.row : 0;
        int colSpan = (it != gridProps.end()) ? it->second.colSpan : 1;
        int rowSpan = (it != gridProps.end()) ? it->second.rowSpan : 1;

        col = min(col, numCols - 1);
        row = min(row, numRows - 1);
        colSpan = min(colSpan, numCols - col);
        rowSpan = min(rowSpan, numRows - row);

        float x = colX[col];
        float y = rowY[row];
        float w = colX[col + colSpan] - colX[col] - m_gap;
        float h = rowY[row + rowSpan] - rowY[row] - m_gap;

        child->setRect(SRect{x, y, w, h});
    }
}

// ==================== AnchorLayout ====================

AnchorLayout::AnchorLayout(Margin padding)
    : m_padding(padding) {}

void AnchorLayout::applyAnchor(const SRect& containerRect,
                                vector<shared_ptr<Control>>& children,
                                unordered_map<Control*, AnchorInfo>& anchorProps) {
    float innerLeft = m_padding.left;
    float innerTop = m_padding.top;
    float innerWidth = containerRect.width - m_padding.left - m_padding.right;
    float innerHeight = containerRect.height - m_padding.top - m_padding.bottom;

    for (auto& child : children) {
        if (!child->getVisible()) continue;
        SRect childRect = child->getRect();

        string anchor = "TOP_LEFT";
        Margin offset;
        auto it = anchorProps.find(child.get());
        if (it != anchorProps.end()) {
            anchor = it->second.anchor;
            offset = it->second.offset;
        }

        float x = innerLeft + offset.left;
        float y = innerTop + offset.top;
        float w = childRect.width;
        float h = childRect.height;

        if (anchor == "TOP_RIGHT") {
            x = innerLeft + innerWidth - w - offset.right;
        } else if (anchor == "BOTTOM_LEFT") {
            y = innerTop + innerHeight - h - offset.bottom;
        } else if (anchor == "BOTTOM_RIGHT") {
            x = innerLeft + innerWidth - w - offset.right;
            y = innerTop + innerHeight - h - offset.bottom;
        } else if (anchor == "TOP_CENTER") {
            x = innerLeft + (innerWidth - w) * 0.5f + offset.left;
        } else if (anchor == "BOTTOM_CENTER") {
            x = innerLeft + (innerWidth - w) * 0.5f + offset.left;
            y = innerTop + innerHeight - h - offset.bottom;
        } else if (anchor == "MID_LEFT") {
            y = innerTop + (innerHeight - h) * 0.5f + offset.top;
        } else if (anchor == "MID_RIGHT") {
            x = innerLeft + innerWidth - w - offset.right;
            y = innerTop + (innerHeight - h) * 0.5f + offset.top;
        } else if (anchor == "CENTER") {
            x = innerLeft + (innerWidth - w) * 0.5f + offset.left;
            y = innerTop + (innerHeight - h) * 0.5f + offset.top;
        } else if (anchor == "TOP_STRETCH") {
            x = innerLeft + offset.left;
            w = innerWidth - offset.left - offset.right;
        } else if (anchor == "BOTTOM_STRETCH") {
            x = innerLeft + offset.left;
            w = innerWidth - offset.left - offset.right;
            y = innerTop + innerHeight - h - offset.bottom;
        } else if (anchor == "LEFT_STRETCH") {
            y = innerTop + offset.top;
            h = innerHeight - offset.top - offset.bottom;
        } else if (anchor == "RIGHT_STRETCH") {
            x = innerLeft + innerWidth - w - offset.right;
            y = innerTop + offset.top;
            h = innerHeight - offset.top - offset.bottom;
        } else if (anchor == "FILL") {
            x = innerLeft + offset.left;
            y = innerTop + offset.top;
            w = innerWidth - offset.left - offset.right;
            h = innerHeight - offset.top - offset.bottom;
        }

        child->setRect(SRect{x, y, w, h});
    }
}
