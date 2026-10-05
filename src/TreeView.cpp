#define NOMINMAX
#include "TreeView.h"
#include "TextDraw.h"
#include "Actor.h"
#include "Texture.h"
#include "PropertyNames.h"
#include "PlatformUtils.h"
#include "EventQueue.h"
#include "LeadingControlSlot.h"
#include <algorithm>
#include <cmath>

// P0-62②：节点着色辅助（定义见 Property system 段；draw 先于此定义使用，需前置声明）
static uint8_t nodeStateBit(ControlState s);
static SColor resolveNodeTextColor(TreeNode* nd, StateColor& ctrl, ControlState st);

TreeView::TreeView(Control* parent, const SRect& rect,
                   float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
    , m_indentWidth(ConstDef::TREEVIEW_INDENT_WIDTH)
    , m_rowHeight(ConstDef::TREEVIEW_DEFAULT_ROW_HEIGHT)
    , m_arrowGap(16.0f)
    , m_fontName(FontName::HarmonyOS_Sans_SC_Regular)
    , m_fontSize(14)
    , m_font()
{
    m_ctlType = ControlType::TreeView;
    m_textColor.setNormal(ConstDef::TREEVIEW_TEXT_COLOR);   // P0-26：原自有缺省色迁入基类 StateColor
    m_rect = rect;
    setFocusable(true);
    setBorderVisible(true);
    syncStateColor();
}

TreeView::~TreeView() {
    for (auto& root : m_rootItems)
        clearNodeRecursive(root);
}

void TreeView::syncStateColor() {
    setBackgroundStateColor(StateColor(m_bgColor, m_bgColor, m_bgColor, m_bgColor));
    setBorderStateColor(StateColor(m_borderColor, m_borderColor, m_borderColor, m_borderColor));
}

void TreeView::clearNodeRecursive(const shared_ptr<TreeNode>& node) {
    shared_ptr<TreeView> self;
    try { self = std::dynamic_pointer_cast<TreeView>(getThis()); } catch (...) {}
    if (m_onClearNode && node->userData && node->userDataOwned)
        m_onClearNode(self, node->userData);
    TreeNodePayload tn = { node->id.c_str(), node->userData };
    fireCCallback(PropertyNames::kEventNodeRemoved, CCallbackData::TreeNode, &tn);
    for (auto& child : node->children)
        clearNodeRecursive(child);
}

void TreeView::create() {
    if (m_isCreated) return;
    if (GET_CONTEXT == nullptr) return;  // 未挂入实例上下文：延迟创建
    ControlImpl::create();

    if (m_scrollBar) removeControl(m_scrollBar);
    m_scrollBar = ScrollBarBuilder(this,
        {m_rect.width - ConstDef::SCROLLBAR_WIDTH, 0,
         ConstDef::SCROLLBAR_WIDTH, m_rect.height},
        ScrollBarOrientation::Vertical, 1.0f, 1.0f)
        .setStepSize(getStride())
        .setPageSize(m_rect.height)
        .setOnPositionChanged([this](shared_ptr<ScrollBar>, float, float, float, float) {
            m_scrollOffset = m_scrollBar->getValue();
        })
        .build();
    addControl(m_scrollBar);

    if (m_hScrollBar) removeControl(m_hScrollBar);
    m_hScrollBar = ScrollBarBuilder(this,
        {0, m_rect.height - ConstDef::SCROLLBAR_WIDTH,
         m_rect.width, ConstDef::SCROLLBAR_WIDTH},
        ScrollBarOrientation::Horizontal, 1.0f, 1.0f)
        .setStepSize(m_indentWidth)
        .setOnPositionChanged([this](shared_ptr<ScrollBar>, float, float, float, float) {
            m_hScrollOffset = m_hScrollBar->getValue();
        })
        .build();
    addControl(m_hScrollBar);

    ensureFont();
    m_isCreated = true;
}

void TreeView::ensureFont() {
    if (m_font) return;
    TextRenderer* renderer = getTextRenderer();
    if (!renderer) return;
    ResourceProvider* provider = getResourceProvider();
    if (!provider) return;

    auto it = ConstDef::fontFiles.find(m_fontName);
    if (it == ConstDef::fontFiles.end()) return;
    string fontPath = it->second;   // P0-54：统一相对路径约定（与 Label/Actor 一致）
    auto data = provider->readFile(fontPath);
    if (!data || data->empty()) return;

    int scaledSize = static_cast<int>(m_fontSize * getScaleXX());
    m_font = renderer->loadFontFromMemoryWithText(
        data->data(), data->size(), scaledSize, "W");
}

// 逐节点字体：fontSize>0 且与 TreeView 级不同 → 按节点 fontName/fontSize 创建并缓存；
// 否则（未设置/与级相同/加载失败）回退 m_font。节点销毁时缓存由 clearItems/setItems/removeNode 清理
SharedFont TreeView::getNodeFont(const shared_ptr<TreeNode>& node) {
    if (!node) return m_font;
    // P0-63④：fontSize<=0 时以控件级字号驱动逐节点字体（仅设字体名也生效；字号随控件级）
    const int effSize = node->fontSize > 0 ? node->fontSize : static_cast<int>(m_fontSize);
    if (node->fontName == m_fontName && effSize == static_cast<int>(m_fontSize)) return m_font;
    auto it = m_nodeFonts.find(node.get());
    if (it != m_nodeFonts.end()) return it->second;

    TextRenderer* renderer = getTextRenderer();
    ResourceProvider* provider = getResourceProvider();
    if (!renderer || !provider) return m_font;
    auto fit = ConstDef::fontFiles.find(node->fontName);
    if (fit == ConstDef::fontFiles.end()) return m_font;
    string fontPath = fit->second;   // P0-54：统一相对路径约定（与 Label/Actor 一致）
    auto data = provider->readFile(fontPath);
    if (!data || data->empty()) return m_font;

    int scaledSize = static_cast<int>(effSize * getScaleXX());
    SharedFont f = renderer->loadFontFromMemoryWithText(
        data->data(), data->size(), scaledSize, "W");
    m_nodeFonts[node.get()] = f;
    return f;
}

// 行控件挂/摘同步（rebuildFlatRows 内调用）：遍历当前 flatRows 收集 leadingControl
// 挂树并登记；对已摘除节点（收起/删除/替换）的控件 removeControl 并移出登记
void TreeView::syncRowControls() {
    vector<shared_ptr<Control>> keep;
    keep.reserve(m_flatRows.size());
    for (auto& row : m_flatRows)
        if (row.node->leadingControl)
            keep.push_back(row.node->leadingControl);

    for (auto& c : m_rowControls) {
        bool still = false;
        for (auto& k : keep)
            if (k.get() == c.get()) { still = true; break; }
        if (!still) removeControl(c);
    }
    for (auto& k : keep) {
        bool exists = false;
        for (auto& c : m_rowControls)
            if (c.get() == k.get()) { exists = true; break; }
        if (!exists) addControl(k);
    }
    m_rowControls = std::move(keep);
}

void TreeView::setFont(FontName fontName) {
    if (m_fontName == fontName && m_font) return;
    m_fontName = fontName;
    m_font.reset();
    m_nodeFonts.clear();  // 逐节点字体缓存随级字体失效
    if (m_isCreated) ensureFont();
}

void TreeView::setFontSize(int size) {
    if (m_fontSize == size) return;
    m_fontSize = size;
    m_font.reset();
    m_nodeFonts.clear();  // 逐节点字体缓存随级字体失效
    if (m_isCreated) ensureFont();
}

// 父链缩放变更时重建字体（保持 setFontSize 语义：字号随复合缩放）
void TreeView::refreshScaleWith(float parentXX, float parentYY){
    float oldScaleX = getScaleXX();
    float oldScaleY = getScaleYY();
    ControlImpl::refreshScaleWith(parentXX, parentYY);
    if (oldScaleX != getScaleXX() || oldScaleY != getScaleYY()) {
        m_font.reset();
        m_nodeFonts.clear();  // 逐节点字体缓存随缩放失效
        if (m_isCreated) ensureFont();
    }
}

void TreeView::setBgColor(const SColor& c) {
    m_bgColor = c;
    syncStateColor();
}

void TreeView::setBorderColor(const SColor& c) {
    m_borderColor = c;
    syncStateColor();
}

void TreeView::draw() {
    if (!m_visible) return;
    auto* dev = getRenderDevice();
    if (!dev) return;

    beforeDraw();

    updateScrollBar();

    float scaleX = getScaleXX();
    float scaleY = getScaleYY();
    float hSb = (m_hScrollBar && m_hScrollBar->getVisible()) ? ConstDef::SCROLLBAR_WIDTH * scaleX : 0;
    float vSb = (m_scrollBar && m_scrollBar->getVisible()) ? ConstDef::SCROLLBAR_WIDTH * scaleX : 0;

    SRect cr = m_frameDrawRect;
    cr.width -= vSb;
    cr.height -= hSb;
    dev->pushClipRect(cr);

    if (!m_flatRows.empty()) {
        float stride = getStride();
        int firstVisible = max(0, (int)(m_scrollOffset / stride));
        float scaledRowH = m_rowHeight * scaleY;
        float topY = cr.top - fmod(m_scrollOffset, stride) * scaleY;
        float leftX = cr.left - m_hScrollOffset * scaleX;
        TextRenderer* renderer = getTextRenderer();

        int fontHeight = 0;
        if (renderer && m_font)
            fontHeight = renderer->getFontHeight(m_font.get());

        for (int i = firstVisible; i < (int)m_flatRows.size(); i++) {
            float y = topY + (i - firstVisible) * stride * scaleY;
            if (y > cr.bottom()) break;

            float rowLeft = cr.left;
            float rowWidth = m_rect.width * scaleX - m_hScrollOffset * scaleX;
            if (rowLeft + rowWidth < cr.left) continue;

            auto& rowNode = m_flatRows[i].node;
            const ControlState rowSt = rowNode->disabled ? ControlState::Disabled
                                     : (i == m_pressedRow) ? ControlState::Pressed
                                     : (i == m_hoveredRow) ? ControlState::Hover
                                                           : ControlState::Normal;
            // P0-64①/P0-65②：背景绘制链（bgMask 位命中态填充；hover 显式解耦；selected 叠加）
            {
                const ControlState bgSt = (i == m_selectedRow) ? ControlState::Normal : rowSt;
                const bool bgHit = rowNode->hasBgStyle
                    && (rowNode->bgMask == 0 || (rowNode->bgMask & nodeStateBit(bgSt)));
                const bool hoverExplicit = (rowSt == ControlState::Hover)
                    && rowNode->hasHoverBg && (i != m_selectedRow);   // selected 优先（P0-64③ 同口径）
                if (bgHit) {
                    dev->setDrawColor(ControlImpl::resolveStateColor(rowNode->bgColor, bgSt));
                    dev->fillRect({cr.left, y, cr.width, scaledRowH});
                } else if (i == m_selectedRow) {
                    dev->setDrawColor(m_selectedColor);
                    dev->fillRect({cr.left, y, cr.width, scaledRowH});
                } else if (i == m_hoveredRow && !rowNode->hasHoverBg) {
                    dev->setDrawColor(m_hoverColor);
                    dev->fillRect({cr.left, y, cr.width, scaledRowH});
                }
                if (hoverExplicit) {   // 显式 hover 优先（覆盖 base/既有链）
                    dev->setDrawColor(rowNode->bgColor.getHover());
                    dev->fillRect({cr.left, y, cr.width, scaledRowH});
                }
                if (!rowNode->disabled) {   // 叠加 tint（hover 显式/掩码豁免；selected 恒叠加）
                    const SColor* tint = nullptr;
                    if (i == m_selectedRow) tint = &m_selectedColor;
                    else if (rowSt == ControlState::Hover && !hoverExplicit && !(rowNode->bgMask & 2)) tint = &m_hoverColor;
                    else if (rowSt == ControlState::Pressed && !(rowNode->bgMask & 4)) tint = &m_hoverColor;
                    if (tint) {
                        dev->setDrawColor(SColor(tint->red(), tint->green(), tint->blue(),
                                                 ConstDef::LIST_HIGHLIGHT_OVERLAY_ALPHA));
                        dev->fillRect({cr.left, y, cr.width, scaledRowH});
                    }
                }
            }
            if (rowNode->borderVisible) {   // P0-62②：边框独立稀疏
                dev->setDrawColor(ControlImpl::resolveStateColor(rowNode->borderColor, rowSt));
                dev->drawRect(SRect(cr.left, y, cr.width, scaledRowH));
            }

            float arrowX = leftX + LEFT_PADDING * scaleX + m_flatRows[i].depth * m_indentWidth * scaleX;
            float labelX = arrowX + m_arrowGap * scaleX;

            if (!m_flatRows[i].node->children.empty())
                drawArrow(dev, arrowX, y, m_flatRows[i].node->expanded);

            SharedFont nodeFont = getNodeFont(m_flatRows[i].node);
            int fontH = (renderer && nodeFont) ? renderer->getFontHeight(nodeFont.get()) : 0;

            // 行前置控件（TreeView 增强）：高自适应行高，宽按原始宽高比等比缩放；
            // rect 必须为局部坐标（绝对坐标会经 getDrawRect 二次叠加父偏移 = 双重偏移）
            float textX = labelX;
            if (m_flatRows[i].node->leadingControl) {
                auto& lc = m_flatRows[i].node->leadingControl;
                // 宽高比优先取纹理自然尺寸（Actor 等 rect 初始为 0 时仍可等比），
                // 其次取当前 rect，最后退化 1:1（CheckBox 16x16 → 正方形）
                float ratio = 1.0f;
                if (auto* actor = dynamic_cast<Actor*>(lc.get())) {
                    Texture* tex = actor->getTexture();
                    if (tex && tex->height() > 0)
                        ratio = (float)tex->width() / tex->height();
                }
                if (ratio == 1.0f) {
                    float ow = lc->getRect().width;
                    float oh = lc->getRect().height;
                    if (oh > 0 && ow > 0) ratio = ow / oh;
                }
                // 槽高 = 文字高度（行内垂直居中，行间留空隙）；无字体时回退行高；
                // fontH 为缩放后像素，先还原逻辑值，后续 slotH*scale 才是像素
                float slotH = (fontH > 0) ? (float)fontH / scaleY : m_rowHeight;
                float slotW = slotH * ratio;
                // 槽起点 = 原文本起点（labelX = arrowX + arrowGap）：
                // 图片缩进与无容器时文本缩进一致，有箭头行箭头在图片左侧不重叠
                float slotStartX = labelX;
                float slotWpx = slotW * scaleX;
                textX = slotStartX + slotWpx + m_flatRows[i].node->leadingGap * scaleX;
                SRect r;
                r.left = (slotStartX - cr.left) / scaleX;
                // 槽位垂直对齐（复用 Label 9 宫格，见 LeadingControlSlot.h）：
                // 水平分量忽略（槽位始终贴文本起点）
                float slotTopPx = y + (scaledRowH - slotH * scaleY)
                                  * LeadingControlSlot::verticalFactor(m_flatRows[i].node->leadingAlign);
                r.top = (slotTopPx - cr.top) / scaleY;
                r.width = slotW;
                r.height = slotH;
                lc->setRect(r);
                lc->draw();
            }

            if (renderer && nodeFont) {
                float textY = y + (scaledRowH - fontH) / 2;
                auto& nd = m_flatRows[i].node;
                TextDraw::withShadow(renderer, nodeFont.get(), nd->label,
                                     textX, textY,
                                     textX + nd->shadowOffsetX * scaleX, textY + nd->shadowOffsetY * scaleY,
                                     resolveNodeTextColor(nd.get(), m_textColor, rowSt),   // P0-62②：稀疏回退链
                                     nd->shadowEnabled,
                                     ControlImpl::resolveStateColor(nd->textShadowColor, rowSt));             // P032：item 阴影四态
            }
        }
    }

    dev->popClipRect();

    for (auto& child : m_children) {
        // 行控件已在行循环内（clip 区内）绘制，此处跳过防同帧重复绘制
        bool isRowControl = false;
        for (auto& rc : m_rowControls)
            if (rc.get() == child.get()) { isRowControl = true; break; }
        if (isRowControl) continue;
        child->draw();
    }

    afterDraw();
}

void TreeView::drawArrow(RenderDevice* dev, float x, float y, bool expanded) {
    float scale = getScaleYY();
    float cx = x + 6 * scale;
    float cy = y + m_rowHeight * scale / 2.0f;
    float size = 5.0f * scale;

    dev->setDrawColor(m_textColor.getNormal());
    if (expanded) {
        dev->drawTriangle(
            cx - size, cy - size * 0.577f,
            cx + size, cy - size * 0.577f,
            cx,        cy + size * 0.577f * 2,
            m_textColor.getNormal());
    } else {
        dev->drawTriangle(
            cx - size * 0.577f, cy - size,
            cx - size * 0.577f, cy + size,
            cx + size * 0.577f * 2, cy,
            m_textColor.getNormal());
    }
}

bool TreeView::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;

    if (event->m_type == EventType::KeyDown && getFocused()) {
        KeyCode kc = event->keyEvent.keycode;
        switch (kc) {
        case KeyCode::Up:
            if (m_selectedRow > 0) {
                selectNode(m_flatRows[m_selectedRow - 1].node->id);
            } else if (m_cycleNavigation && !m_flatRows.empty()) {
                selectNode(m_flatRows.back().node->id);
            }
            return true;

        case KeyCode::Down:
            if (m_selectedRow >= 0 && m_selectedRow < (int)m_flatRows.size() - 1) {
                selectNode(m_flatRows[m_selectedRow + 1].node->id);
            } else if (m_cycleNavigation && !m_flatRows.empty()) {
                selectNode(m_flatRows[0].node->id);
            }
            return true;

        case KeyCode::Left:
            if (m_selectedRow >= 0) {
                auto& node = m_flatRows[m_selectedRow].node;
                if (node->expanded)
                    collapseNode(node->id);
            }
            return true;

        case KeyCode::Right:
            if (m_selectedRow >= 0) {
                auto& node = m_flatRows[m_selectedRow].node;
                if (!node->children.empty() && !node->expanded)
                    expandNode(node->id);
            }
            return true;

        case KeyCode::PageUp: {
            int pageLines = max(1, (int)(m_frameDrawRect.height / getStride()));
            int target = max(0, m_selectedRow - pageLines);
            if (target < (int)m_flatRows.size())
                selectNode(m_flatRows[target].node->id);
            return true;
        }

        case KeyCode::PageDown: {
            int pageLines = max(1, (int)(m_frameDrawRect.height / getStride()));
            int target = min((int)m_flatRows.size() - 1, m_selectedRow + pageLines);
            if (target >= 0)
                selectNode(m_flatRows[target].node->id);
            return true;
        }

        case KeyCode::Home:
            if (!m_flatRows.empty())
                selectNode(m_flatRows[0].node->id);
            return true;

        case KeyCode::End:
            if (!m_flatRows.empty())
                selectNode(m_flatRows.back().node->id);
            return true;

        default:
            break;
        }
    }

    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left) {
        if (!isContainsPoint(event->mouseButton.x, event->mouseButton.y))
            return false;
        if (!getFocused()) setFocused(true);

        for (auto sb : {m_hScrollBar, m_scrollBar}) {
            if (sb && sb->getVisible() &&
                sb->isContainsPoint(event->mouseButton.x, event->mouseButton.y)) {
                return sb->handleEvent(event);
            }
        }

        int row = hitTestRow(event->mouseButton.x, event->mouseButton.y);
        if (row >= 0) {
            m_pressedRow = row;   // P032：item pressed 态
            if (hitTestArrow(row, event->mouseButton.x)) {
                toggleExpand(m_flatRows[row].node->id);
                return true;
            }
            // TreeView 增强：点击行前置控件 → 选中该行 + 不消费事件，
            // 事件落入子控件分发（ControlImpl::handleEvent）完成控件自身交互（如 CheckBox 勾选）
            if (m_flatRows[row].node->leadingControl &&
                m_flatRows[row].node->leadingControl->isContainsPoint(
                    event->mouseButton.x, event->mouseButton.y)) {
                selectNode(m_flatRows[row].node->id);
                return false;
            }
            selectNode(m_flatRows[row].node->id);
            return true;
        }
    }

    if (event->m_type == EventType::MouseMove) {
        if (isContainsPoint(event->mousePos.x, event->mousePos.y)) {
            m_hoveredRow = hitTestRow(event->mousePos.x, event->mousePos.y);
        } else {
            m_hoveredRow = -1;
        }
    }

    if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
        m_pressedRow = -1;   // P032：抬起清除 item pressed
    }

    if (event->m_type == EventType::MouseWheel) {
        if (!isContainsPoint(event->mouseWheel.x, event->mouseWheel.y))
            return false;

        // 无滚动余量（纵向与横向均无）：透传给容器，不空转消费
        float vMax = (m_scrollBar && m_scrollBar->getVisible()) ? m_scrollBar->getMaxValue() : 0.f;
        float hMax = (m_hScrollBar && m_hScrollBar->getVisible()) ? m_hScrollBar->getMaxValue() : 0.f;
        if (vMax <= 0.f && hMax <= 0.f) return false;

        if (event->mouseWheel.scrollY != 0 && m_scrollBar) {
            float step = getStride() * ConstDef::TREEVIEW_SCROLL_STEP_LINES;
            float newOffset = m_scrollOffset - event->mouseWheel.scrollY * step;
            m_scrollOffset = max(0.0f, newOffset);
            m_scrollBar->setValue(m_scrollOffset);
        }
        if (event->mouseWheel.scrollX != 0 && m_hScrollBar) {
            float step = m_indentWidth;
            float newOffset = m_hScrollOffset - event->mouseWheel.scrollX * step;
            m_hScrollOffset = max(0.0f, newOffset);
            m_hScrollBar->setValue(m_hScrollOffset);
        }
        return true;
    }

    for (auto sb : {m_hScrollBar, m_scrollBar}) {
        if (sb && sb->getVisible()) {
            if (event->m_type == EventType::MouseDown ||
                event->m_type == EventType::MouseUp ||
                event->m_type == EventType::MouseMove) {
                if (sb->handleEvent(event)) return true;
            }
        }
    }

    return ControlImpl::handleEvent(event);
}

int TreeView::hitTestRow(float mx, float my) {
    (void)mx;
    // P0-64：改用 getDrawRect()（m_frameDrawRect 仅在首次 draw 后有效，创建后立即 hover 会错位）
    float relY = (my - getDrawRect().top) / getScaleYY() + m_scrollOffset;
    int row = (int)(relY / getStride());
    if (row >= 0 && row < (int)m_flatRows.size()) return row;
    return -1;
}

bool TreeView::hitTestArrow(int row, float mx) {
    if (row < 0 || row >= (int)m_flatRows.size()) return false;
    auto& flat = m_flatRows[row];
    if (flat.node->children.empty()) return false;

    float scale = getScaleXX();
    float arrowX = m_frameDrawRect.left - m_hScrollOffset * scale + LEFT_PADDING * scale + flat.depth * m_indentWidth * scale;
    return mx >= arrowX && mx <= arrowX + m_arrowGap * scale;
}

void TreeView::rebuildFlatRows() {
    m_flatRows.clear();
    m_nodeMap.clear();
    m_selectedRow = -1;

    // 全量索引所有节点（含折叠子树），保证 findNodeById/removeNode 对折叠节点也有效
    function<void(const shared_ptr<TreeNode>&, int)> indexAll;
    indexAll = [&](const shared_ptr<TreeNode>& node, int) {
        m_nodeMap[node->id] = node;
        for (auto& child : node->children)
            indexAll(child, 0);
    };
    for (auto& root : m_rootItems) indexAll(root, 0);

    function<void(const shared_ptr<TreeNode>&, int)> flatten;
    flatten = [&](const shared_ptr<TreeNode>& node, int depth) {
        m_flatRows.push_back({node, depth});
        if (node->id == m_selectedId)
            m_selectedRow = (int)m_flatRows.size() - 1;
        if (node->expanded)
            for (auto& child : node->children)
                flatten(child, depth + 1);
    };

    for (auto& root : m_rootItems)
        flatten(root, 0);

    syncRowControls();
    updateScrollBar();
}

void TreeView::setItems(const vector<shared_ptr<TreeNode>>& items) {
    for (auto& root : m_rootItems)
        clearNodeRecursive(root);
    m_nodeFonts.clear();  // 旧树节点已销毁，逐节点字体缓存 key 失效

    m_rootItems = items;

    function<void(shared_ptr<TreeNode>&)> applyDefault;
    applyDefault = [&](shared_ptr<TreeNode>& node) {
        if (!node->children.empty())
            node->expanded = m_defaultExpand;
        for (auto& child : node->children)
            applyDefault(child);
    };
    for (auto& root : m_rootItems)
        applyDefault(root);

    rebuildFlatRows();
}

bool TreeView::addChild(const string& parentId, shared_ptr<TreeNode> node) {
    if (!node) return false;
    auto parent = findNodeById(parentId);
    if (!parent) return false;

    parent->children.push_back(node);
    if (!parent->expanded) {
        parent->expanded = true;
        if (m_onExpand) m_onExpand(std::dynamic_pointer_cast<TreeView>(getThis()), parentId);
        fireCCallback(PropertyNames::kEventExpand, CCallbackData::String, parentId.c_str());
    }
    rebuildFlatRows();
    return true;
}

bool TreeView::addRootItem(shared_ptr<TreeNode> node) {
    if (!node) return false;
    m_rootItems.push_back(node);
    rebuildFlatRows();
    return true;
}

bool TreeView::removeNode(const string& id) {
    if (m_rootItems.empty()) return false;
    if (m_nodeMap.find(id) == m_nodeMap.end()) return false;

    // Check root items first
    for (auto it = m_rootItems.begin(); it != m_rootItems.end(); ++it) {
        if ((*it)->id == id) {
            clearNodeRecursive(*it);
            m_rootItems.erase(it);
            if (m_selectedId == id) {
                m_selectedId.clear();
                m_selectedRow = -1;
            }
            rebuildFlatRows();
            m_nodeFonts.clear();  // 被删节点销毁，缓存 key 失效
            return true;
        }
    }

    // Search children recursively
    function<bool(shared_ptr<TreeNode>&)> removeRecursive;
    removeRecursive = [&](shared_ptr<TreeNode>& node) -> bool {
        for (auto it = node->children.begin(); it != node->children.end(); ++it) {
            if ((*it)->id == id) {
                clearNodeRecursive(*it);
                node->children.erase(it);
                if (m_selectedId == id) {
                    m_selectedId.clear();
                    m_selectedRow = -1;
                }
                rebuildFlatRows();
                m_nodeFonts.clear();  // 被删节点销毁，缓存 key 失效
                return true;
            }
            if (removeRecursive(*it)) return true;
        }
        return false;
    };

    for (auto& root : m_rootItems) {
        if (removeRecursive(root)) return true;
    }
    return false;
}

bool TreeView::setNodeLabel(const string& id, const string& label) {
    auto node = findNodeById(id);
    if (!node) return false;
    node->label = label;
    return true;
}

bool TreeView::setNodeUserData(const string& id, void* userData) {
    auto node = findNodeById(id);
    if (!node) return false;
    node->userData = userData;
    node->userDataOwned = false;  // 外部指针由调用方管理，TreeView 不负责清理
    return true;
}

void TreeView::clearItems() {
    // 行控件先行摘除（节点随后销毁；clearItems 不触发 rebuildFlatRows）
    for (auto& c : m_rowControls)
        removeControl(c);
    m_rowControls.clear();
    m_nodeFonts.clear();
    for (auto& root : m_rootItems)
        clearNodeRecursive(root);
    m_rootItems.clear();
    m_flatRows.clear();
    m_nodeMap.clear();
    m_selectedId.clear();
    m_selectedRow = -1;
    m_hoveredRow = -1;
    m_scrollOffset = 0;
    updateScrollBar();
}

shared_ptr<TreeNode> TreeView::findNodeById(const string& id) {
    auto it = m_nodeMap.find(id);
    return it != m_nodeMap.end() ? it->second : nullptr;
}

bool TreeView::toggleExpand(const string& id) {
    auto node = findNodeById(id);
    if (!node || node->children.empty()) return false;
    if (node->expanded)
        return collapseNode(id);
    else
        return expandNode(id);
}

bool TreeView::expandNode(const string& id) {
    auto node = findNodeById(id);
    if (!node || node->expanded || node->children.empty()) return false;
    node->expanded = true;
    rebuildFlatRows();
    if (m_onExpand) m_onExpand(std::dynamic_pointer_cast<TreeView>(getThis()), id);
    fireCCallback(PropertyNames::kEventExpand, CCallbackData::String, id.c_str());
    return true;
}

bool TreeView::collapseNode(const string& id) {
    auto node = findNodeById(id);
    if (!node || !node->expanded) return false;
    node->expanded = false;
    rebuildFlatRows();
    if (m_onCollapse) m_onCollapse(std::dynamic_pointer_cast<TreeView>(getThis()), id);
    fireCCallback(PropertyNames::kEventCollapse, CCallbackData::String, id.c_str());
    return true;
}

void TreeView::expandAll() {
    function<void(vector<shared_ptr<TreeNode>>&)> expandRecursive;
    expandRecursive = [&](vector<shared_ptr<TreeNode>>& items) {
        for (auto& node : items) {
            if (!node->children.empty()) {
                node->expanded = true;
                expandRecursive(node->children);
            }
        }
    };
    expandRecursive(m_rootItems);
    rebuildFlatRows();
}

void TreeView::collapseAll() {
    function<void(vector<shared_ptr<TreeNode>>&)> collapseRecursive;
    collapseRecursive = [&](vector<shared_ptr<TreeNode>>& items) {
        for (auto& node : items) {
            if (!node->children.empty()) {
                node->expanded = false;
                collapseRecursive(node->children);
            }
        }
    };
    collapseRecursive(m_rootItems);
    rebuildFlatRows();
}

bool TreeView::selectNode(const string& id) {
    if (m_nodeMap.find(id) == m_nodeMap.end())
        return false;
    m_selectedId = id;
    rebuildFlatRows();
    ensureSelectedVisible();
    if (m_onSelect) m_onSelect(std::dynamic_pointer_cast<TreeView>(getThis()), id);
    if (m_onSelectData) {
        auto node = findNodeById(id);
        m_onSelectData(std::dynamic_pointer_cast<TreeView>(getThis()), id, node ? node->userData : nullptr);
    }
    {
        auto node = findNodeById(id);
        TreeNodePayload tn = { id.c_str(), node ? node->userData : nullptr };
        fireCCallback(PropertyNames::kEventSelect, CCallbackData::TreeNode, &tn);
    }
    return true;
}

void TreeView::ensureSelectedVisible() {
    if (m_selectedRow < 0 || !m_scrollBar) return;
    float stride = getStride();
    float rowTop = m_selectedRow * stride;
    float rowBottom = rowTop + m_rowHeight;
    float viewH = m_rect.height;

    if (rowTop < m_scrollOffset) {
        m_scrollOffset = rowTop;
    } else if (rowBottom > m_scrollOffset + viewH) {
        m_scrollOffset = rowBottom - viewH;
    }

    m_scrollOffset = max(0.0f, m_scrollOffset);
    m_scrollBar->setValue(m_scrollOffset);
}

float TreeView::calcContentWidth() {
    if (m_flatRows.empty() || !m_font) return 0;
    float maxW = 0;
    TextRenderer* renderer = getTextRenderer();
    float scale = getScaleXX();
    for (auto& row : m_flatRows) {
        float labelW = 0;
        SharedFont nodeFont = getNodeFont(row.node);
        if (renderer) {
            SSize sz = renderer->measureText(nodeFont.get(), row.node->label);
            labelW = sz.width / scale;
        }
        float rowW = LEFT_PADDING + row.depth * m_indentWidth + m_arrowGap + labelW + RIGHT_GAP;
        // 前置控件（leadingControl）计入内容宽：与绘制同公式（槽宽 = 行字号高 × 宽高比）
        if (row.node->leadingControl) {
            auto& lc = row.node->leadingControl;
            float ratio = 1.0f;
            if (auto* actor = dynamic_cast<Actor*>(lc.get())) {
                Texture* tex = actor->getTexture();
                if (tex && tex->height() > 0)
                    ratio = (float)tex->width() / tex->height();
            }
            if (ratio == 1.0f) {
                float ow = lc->getRect().width;
                float oh = lc->getRect().height;
                if (oh > 0 && ow > 0) ratio = ow / oh;
            }
            float fontH = 0;
            if (renderer) fontH = renderer->getFontHeight(nodeFont.get());
            float slotW = (fontH > 0 ? fontH : m_rowHeight) * ratio;
            rowW += slotW + row.node->leadingGap;
        }
        if (rowW > maxW) maxW = rowW;
    }
    return maxW;
}

void TreeView::setIndentWidth(float px) {
    m_indentWidth = max(0.0f, px);
}

void TreeView::setRowHeight(float px) {
    m_rowHeight = max(1.0f, px);
    if (m_scrollBar) m_scrollBar->setStepSize(getStride());
}

void TreeView::setLineSpacing(float px) {
    m_lineSpacing = max(0.0f, px);
    if (m_scrollBar) m_scrollBar->setStepSize(getStride());
}

void TreeView::setArrowGap(float px) {
    m_arrowGap = max(0.0f, px);
}

void TreeView::updateScrollBar() {
    if (!m_scrollBar || !m_hScrollBar) return;

    float stride = getStride();
    float contentH = m_flatRows.size() * stride;
    float contentW = calcContentWidth();
    float viewH = m_rect.height;
    float viewW = m_rect.width;

    float sb = ConstDef::SCROLLBAR_WIDTH;

    // Two-pass layout: scrollbar mutual exclusion
    bool vVis = contentH > viewH;
    bool hVis = contentW > (viewW - (vVis ? sb : 0));
    vVis = contentH > (viewH - (hVis ? sb : 0));
    hVis = contentW > (viewW - (vVis ? sb : 0));

    float vH = viewH - (hVis ? sb : 0);
    float hW = viewW - (vVis ? sb : 0);

    // Vertical scrollbar
    m_scrollBar->setVisible(vVis);
    if (vVis) {
        float maxScroll = contentH - vH;
        m_scrollOffset = min(m_scrollOffset, maxScroll);
        m_scrollBar->setRange(0, maxScroll);
        m_scrollBar->setPageSize(vH);
        m_scrollBar->setValue(m_scrollOffset);
    } else {
        m_scrollOffset = 0;
        m_scrollBar->setValue(0);
    }
    m_scrollBar->setRect({viewW - sb, 0, sb, vH});

    // Horizontal scrollbar
    m_hScrollBar->setVisible(hVis);
    if (hVis) {
        float maxScroll = contentW - hW;
        m_hScrollOffset = min(m_hScrollOffset, maxScroll);
        m_hScrollBar->setRange(0, maxScroll);
        m_hScrollBar->setPageSize(hW);
        m_hScrollBar->setValue(m_hScrollOffset);
    } else {
        m_hScrollOffset = 0;
        m_hScrollBar->setValue(0);
    }
    m_hScrollBar->setRect({0, viewH - sb, hW, sb});
}

// P0-62②：节点着色状态位与未设态回退（显式态 -> 节点 normal -> 控件级）
static uint8_t nodeStateBit(ControlState s) {
    switch (s) {
        case ControlState::Hover:    return 2;
        case ControlState::Pressed:  return 4;
        case ControlState::Disabled: return 8;
        default:                     return 1;
    }
}
static SColor resolveNodeTextColor(TreeNode* nd, StateColor& ctrl, ControlState st) {
    if (nd->textColorMask & nodeStateBit(st)) {
        switch (st) {
            case ControlState::Hover:    return nd->textColor.getHover();
            case ControlState::Pressed:  return nd->textColor.getPressed();
            case ControlState::Disabled: return nd->textColor.getDisabled();
            default:                     return nd->textColor.getNormal();
        }
    }
    if (nd->textColorMask & 1) return nd->textColor.getNormal();   // 未设态回退节点 normal
    return ControlImpl::resolveStateColor(ctrl, st);               // 未设文字色 -> 控件级
}
void TreeView::setNodeTextColor(const string& id, SColor color, ControlState state) {
    auto node = findNodeById(id);
    if (!node) return;
    switch (state) {
        case ControlState::Hover:    node->textColor.setHover(color);    break;
        case ControlState::Pressed:  node->textColor.setPressed(color);  break;
        case ControlState::Disabled: node->textColor.setDisabled(color); break;
        default:                     node->textColor.setNormal(color);   break;
    }
    node->textColorMask |= nodeStateBit(state);
    node->hasTextStyle = true;
    node->hasStyle = true;
}
void TreeView::setNodeBackgroundColor(const string& id, SColor color) {
    auto node = findNodeById(id);
    if (!node) return;
    node->bgColor = StateColor(color, color, color, color);   // 单色语义：四态同色（无回退需求）
    node->hasBgStyle = true;
    node->hasStyle = true;
}
void TreeView::setNodeHoverBackgroundColor(const string& id, SColor color) {   // P0-64④ / P0-65②
    auto node = findNodeById(id);
    if (!node) return;
    node->bgColor.setHover(color);   // hover 色存储
    node->hasHoverBg = true;         // P0-65②：解耦标记（不置 bgMask/hasBgStyle → 不影响常态背景）
}
void TreeView::setNodeFont(const string& id, FontName name, int size) {   // P0-63④
    auto node = findNodeById(id);
    if (!node) return;
    node->fontName = name;
    node->fontSize = size > 0 ? size : 0;   // <=0 继承控件级
    m_nodeFonts.clear();
}
void TreeView::setNodeShadow(const string& id, SColor color, float offsetX, float offsetY) {
    auto node = findNodeById(id);
    if (!node) return;
    node->textShadowColor = StateColor(color, color, color, color);
    node->shadowOffsetX = offsetX;
    node->shadowOffsetY = offsetY;
    node->shadowEnabled = true;
    node->hasStyle = true;
}

// Property system
// P032：item 级四态（对象路径；item-id 定位）
int TreeView::setStateColorProperty(const char* prop, StateColor stateColor) {
    if (strcmp(prop, PropertyNames::kItemBackground) == 0 ||
        strcmp(prop, PropertyNames::kItemBorder) == 0 ||
        strcmp(prop, PropertyNames::kItemText) == 0 ||
        strcmp(prop, PropertyNames::kTreeItemTextShadow) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (!node) return 0;
        if (strcmp(prop, PropertyNames::kItemBackground) == 0)     { node->bgColor = stateColor; node->bgMask = 0xF; node->hasBgStyle = true; }
        else if (strcmp(prop, PropertyNames::kItemBorder) == 0)    node->borderColor = stateColor;
        else if (strcmp(prop, PropertyNames::kItemText) == 0)      { node->textColor = stateColor; node->textColorMask = 0xF; node->hasTextStyle = true; }
        else                                                       node->textShadowColor = stateColor;
        node->hasStyle = true;
        return 1;
    }
    return ControlImpl::setStateColorProperty(prop, stateColor);
}
int TreeView::getStateColorProperty(const char* prop, StateColor& out) {
    auto node = findNodeById(m_itemTargetId);
    if (!node) return 0;
    if (strcmp(prop, PropertyNames::kItemBackground) == 0)     { out = node->bgColor;         return 1; }
    if (strcmp(prop, PropertyNames::kItemBorder) == 0)         { out = node->borderColor;     return 1; }
    if (strcmp(prop, PropertyNames::kItemText) == 0)           { out = node->textColor;       return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemTextShadow) == 0) { out = node->textShadowColor; return 1; }
    return ControlImpl::getStateColorProperty(prop, out);
}

int TreeView::setColorProperty(const char* prop, SColor color) {
    if (strcmp(prop, PropertyNames::kTreeSelected) == 0) { setSelectedColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kTreeHover) == 0)    { setHoverColor(color);    return 1; }
    if (strcmp(prop, PropertyNames::kBackground) == 0)   { setBgColor(color);       return 1; }
    if (strcmp(prop, PropertyNames::kBorder) == 0)       { setBorderColor(color);   return 1; }
    if (strcmp(prop, PropertyNames::kText) == 0)         { setTextColor(color);     return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemTextShadow) == 0) {   // P0-26/P032：item 文本阴影色（单色语义）
        auto node = findNodeById(m_itemTargetId);
        if (node) {
            node->textShadowColor = StateColor(color, color, color, color);   // P0-62②：单色四态同色（防未设态回退缺省）
            node->shadowEnabled = true;
            node->hasStyle = true;
            return 1;
        }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kItemBackground) == 0 ||
        strcmp(prop, PropertyNames::kItemBorder) == 0 ||
        strcmp(prop, PropertyNames::kItemText) == 0) {              // P032：item 色单态（normal）
        auto node = findNodeById(m_itemTargetId);
        if (!node) return 0;
        if (strcmp(prop, PropertyNames::kItemBackground) == 0)      { node->bgColor = StateColor(color, color, color, color); node->hasBgStyle = true; }   // P0-62②：单色四态同色
        else if (strcmp(prop, PropertyNames::kItemBorder) == 0)     node->borderColor.setNormal(color);
        else                                                        { node->textColor.setNormal(color); node->textColorMask |= 1; node->hasTextStyle = true; }
        node->hasStyle = true;
        return 1;
    }
    return ControlImpl::setColorProperty(prop, color);
}

int TreeView::setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) {
    if (strcmp(event, PropertyNames::kEventSelect) == 0 ||
        strcmp(event, PropertyNames::kEventExpand) == 0 ||
        strcmp(event, PropertyNames::kEventCollapse) == 0) {
        return ControlImpl::setCallbackProperty(event, cb, userData);
    }
    return ControlImpl::setCallbackProperty(event, cb, userData);
}

int TreeView::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kTreeExpand) == 0)   { return value && expandNode(value) ? 1 : 0; }
    if (strcmp(prop, PropertyNames::kTreeCollapse) == 0) { return value && collapseNode(value) ? 1 : 0; }
    if (strcmp(prop, PropertyNames::kTreeItemId) == 0)   { m_itemTargetId = value ? value : ""; return 1; }
    return ControlImpl::setStringProperty(prop, value);
}

int TreeView::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kSelectedId) == 0) { out = m_selectedId.c_str(); return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemId) == 0) { out = m_itemTargetId.c_str(); return 1; }
    return ControlImpl::getStringProperty(prop, out);
}

int TreeView::setPtrProperty(const char* prop, void* value) {
    if (strcmp(prop, PropertyNames::kSelectedUserData) == 0) {
        auto node = findNodeById(m_selectedId);
        if (node) { node->userData = value; return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kTreeItemLeadingControl) == 0) {
        // 前置控件容器（借用语义，生命周期由调用方保证，与 selected-user-data 同约定）：
        // 无删除器包装，避免 TreeView 误删外部控件；挂树由 syncRowControls 完成
        auto node = findNodeById(m_itemTargetId);
        if (!node) return 0;
        if (value) {
            node->leadingControl = shared_ptr<Control>(static_cast<Control*>(value), [](Control*) {});
        } else {
            node->leadingControl.reset();   // NULL = 解除容器并摘树
        }
        syncRowControls();
        return 1;
    }
    return ControlImpl::setPtrProperty(prop, value);
}

int TreeView::getPtrProperty(const char* prop, void*& out) {
    if (strcmp(prop, PropertyNames::kSelectedUserData) == 0) {
        auto node = findNodeById(m_selectedId);
        if (node) { out = node->userData; return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kTreeItemLeadingControl) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (node && node->leadingControl) { out = node->leadingControl.get(); return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kScrollBarProp) == 0)  { out = m_scrollBar  ? static_cast<Control*>(m_scrollBar.get())  : nullptr; return 1; }
    if (strcmp(prop, PropertyNames::kHScrollBarProp) == 0) { out = m_hScrollBar ? static_cast<Control*>(m_hScrollBar.get()) : nullptr; return 1; }
    return ControlImpl::getPtrProperty(prop, out);
}

int TreeView::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kItemBorderVisible) == 0 ||
        strcmp(prop, PropertyNames::kItemShadow) == 0 ||
        strcmp(prop, PropertyNames::kItemDisabled) == 0) {   // P032：item 级开关（item-id 定位）
        auto node = findNodeById(m_itemTargetId);
        if (!node) return 0;
        if (strcmp(prop, PropertyNames::kItemBorderVisible) == 0) node->borderVisible = (value != 0);
        else if (strcmp(prop, PropertyNames::kItemShadow) == 0)   node->shadowEnabled = (value != 0);
        else                                                      node->disabled = (value != 0);
        node->hasStyle = true;
        return 1;
    }
    if (strcmp(prop, PropertyNames::kCycleNavigation) == 0) { setCycleNavigation(value != 0); return 1; }
    if (strcmp(prop, PropertyNames::kDefaultExpand) == 0)   { setDefaultExpand(value != 0);   return 1; }
    if (strcmp(prop, PropertyNames::kExpandAll) == 0)       { expandAll(); return 1; }
    if (strcmp(prop, PropertyNames::kCollapseAll) == 0)     { collapseAll(); return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}
int TreeView::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) { setFontSize(value); return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemFontSize) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (node) { node->fontSize = value; m_nodeFonts.clear(); updateScrollBar(); return 1; }
        return 0;
    }
    return ControlImpl::setIntProperty(prop, value);
}
int TreeView::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kIndentWidth) == 0) { setIndentWidth(value);  return 1; }
    if (strcmp(prop, PropertyNames::kRowHeight) == 0)   { setRowHeight(value);    return 1; }
    if (strcmp(prop, PropertyNames::kLineSpacing) == 0) { setLineSpacing(value);  return 1; }
    if (strcmp(prop, PropertyNames::kArrowGap) == 0)    { setArrowGap(value);     return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemLeadingGap) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (node) { node->leadingGap = value; return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kTreeItemShadowOffsetX) == 0) {   // P0-26
        auto node = findNodeById(m_itemTargetId);
        if (node) { node->shadowOffsetX = value; return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kTreeItemShadowOffsetY) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (node) { node->shadowOffsetY = value; return 1; }
        return 0;
    }
    return ControlImpl::setFloatProperty(prop, value);
}
static bool parseItemAlign(const char* value, AlignmentMode& out) {
    if (!value) return false;
    static const unordered_map<string, AlignmentMode> alignMap = {
        {PropertyNames::kAlignLowerTopLeft,      AlignmentMode::AM_TOP_LEFT},
        {PropertyNames::kAlignLowerTopCenter,    AlignmentMode::AM_TOP_CENTER},
        {PropertyNames::kAlignLowerTopRight,     AlignmentMode::AM_TOP_RIGHT},
        {PropertyNames::kAlignLowerMidLeft,      AlignmentMode::AM_MID_LEFT},
        {PropertyNames::kAlignLowerCenter,       AlignmentMode::AM_CENTER},
        {PropertyNames::kAlignLowerMidRight,     AlignmentMode::AM_MID_RIGHT},
        {PropertyNames::kAlignLowerBottomLeft,   AlignmentMode::AM_BOTTOM_LEFT},
        {PropertyNames::kAlignLowerBottomCenter, AlignmentMode::AM_BOTTOM_CENTER},
        {PropertyNames::kAlignLowerBottomRight,  AlignmentMode::AM_BOTTOM_RIGHT},
    };
    auto it = alignMap.find(value);
    if (it == alignMap.end()) return false;
    out = it->second;
    return true;
}

int TreeView::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        setFont(FontNameFromString(value));
        return 1;
    }
    if (strcmp(prop, PropertyNames::kTreeItemFont) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (node && value) { node->fontName = FontNameFromString(value); m_nodeFonts.clear(); updateScrollBar(); return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kTreeItemLeadingAlign) == 0) {
        auto node = findNodeById(m_itemTargetId);
        AlignmentMode mode;
        if (!node || !parseItemAlign(value, mode)) return 0;
        node->leadingAlign = mode;
        return 1;
    }
    return ControlImpl::setEnumProperty(prop, value);
}
int TreeView::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kItemBorderVisible) == 0 ||
        strcmp(prop, PropertyNames::kItemShadow) == 0 ||
        strcmp(prop, PropertyNames::kItemDisabled) == 0) {   // P032
        auto node = findNodeById(m_itemTargetId);
        if (!node) return 0;
        if (strcmp(prop, PropertyNames::kItemBorderVisible) == 0) out = node->borderVisible ? 1 : 0;
        else if (strcmp(prop, PropertyNames::kItemShadow) == 0)   out = node->shadowEnabled ? 1 : 0;
        else                                                      out = node->disabled ? 1 : 0;
        return 1;
    }
    if (strcmp(prop, PropertyNames::kCycleNavigation) == 0) { out = m_cycleNavigation ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kDefaultExpand) == 0)   { out = m_defaultExpand ? 1 : 0;   return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}
int TreeView::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) { out = m_fontSize; return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemFontSize) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (node) { out = node->fontSize; return 1; }
        return 0;
    }
    return ControlImpl::getIntProperty(prop, out);
}
int TreeView::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kTreeItemTextShadow) == 0) {   // P0-26/P032：item 阴影色读回（normal）
        auto node = findNodeById(m_itemTargetId);
        if (node) { out = node->textShadowColor.getNormal(); return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kItemBackground) == 0 ||
        strcmp(prop, PropertyNames::kItemBorder) == 0 ||
        strcmp(prop, PropertyNames::kItemText) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (!node) return 0;
        if (strcmp(prop, PropertyNames::kItemBackground) == 0)      out = node->bgColor.getNormal();
        else if (strcmp(prop, PropertyNames::kItemBorder) == 0)     out = node->borderColor.getNormal();
        else                                                        out = node->textColor.getNormal();
        return 1;
    }
    return ControlImpl::getColorProperty(prop, out);
}

int TreeView::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kTreeItemShadowOffsetX) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (node) { out = node->shadowOffsetX; return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kTreeItemShadowOffsetY) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (node) { out = node->shadowOffsetY; return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kIndentWidth) == 0) { out = m_indentWidth; return 1; }
    if (strcmp(prop, PropertyNames::kRowHeight) == 0)   { out = m_rowHeight;   return 1; }
    if (strcmp(prop, PropertyNames::kLineSpacing) == 0) { out = m_lineSpacing; return 1; }
    if (strcmp(prop, PropertyNames::kArrowGap) == 0)    { out = m_arrowGap;    return 1; }
    if (strcmp(prop, PropertyNames::kTreeItemLeadingGap) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (node) { out = node->leadingGap; return 1; }
        return 0;
    }
    return ControlImpl::getFloatProperty(prop, out);
}
int TreeView::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        out = FontNameToString(m_fontName);
        return 1;
    }
    if (strcmp(prop, PropertyNames::kTreeItemFont) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (node) { out = FontNameToString(node->fontName); return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kTreeItemLeadingAlign) == 0) {
        auto node = findNodeById(m_itemTargetId);
        if (!node) return 0;
        switch (node->leadingAlign) {
            case AlignmentMode::AM_TOP_LEFT:      out = PropertyNames::kAlignLowerTopLeft;      return 1;
            case AlignmentMode::AM_TOP_CENTER:    out = PropertyNames::kAlignLowerTopCenter;    return 1;
            case AlignmentMode::AM_TOP_RIGHT:     out = PropertyNames::kAlignLowerTopRight;     return 1;
            case AlignmentMode::AM_MID_LEFT:      out = PropertyNames::kAlignLowerMidLeft;      return 1;
            case AlignmentMode::AM_CENTER:        out = PropertyNames::kAlignLowerCenter;       return 1;
            case AlignmentMode::AM_MID_RIGHT:     out = PropertyNames::kAlignLowerMidRight;     return 1;
            case AlignmentMode::AM_BOTTOM_LEFT:   out = PropertyNames::kAlignLowerBottomLeft;   return 1;
            case AlignmentMode::AM_BOTTOM_CENTER: out = PropertyNames::kAlignLowerBottomCenter; return 1;
            case AlignmentMode::AM_BOTTOM_RIGHT:  out = PropertyNames::kAlignLowerBottomRight;  return 1;
        }
        return 1;
    }
    return ControlImpl::getEnumProperty(prop, out);
}
