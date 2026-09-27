#define NOMINMAX
#include "ColorPicker.h"
#include "GraphTool.h"
#include "PropertyNames.h"
#include "FocusManager.h"
#include "Bench.h"
#include <algorithm>

// ==================== PresetCell ====================

ColorPicker::PresetCell::PresetCell(Control* parent, SRect rect,
    const SColor& color, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale), m_color(color)
{
    setRect(rect);
    setFocusable(true);
}

void ColorPicker::PresetCell::draw() {
    if (!m_visible) return;
    beforeDraw();
    SRect dr = getDrawRect();
    GET_RENDERDEVICE->setDrawColor(m_color);
    GET_RENDERDEVICE->fillRect(dr);
    if (m_selected) {
        GET_RENDERDEVICE->setDrawColor(ConstDef::COLORPICKER_PRESET_SELECTED);
        GET_RENDERDEVICE->drawRect(dr);
        SRect inner(dr.left + 1, dr.top + 1, dr.width - 2, dr.height - 2);
        GET_RENDERDEVICE->setDrawColor(ConstDef::COLORPICKER_PRESET_SELECTED);
        GET_RENDERDEVICE->drawRect(inner);
    } else {
        GET_RENDERDEVICE->setDrawColor(ConstDef::COLORPICKER_PRESET_NORMAL);
        GET_RENDERDEVICE->drawRect(dr);
    }
    afterDraw();
}

bool ColorPicker::PresetCell::handleEvent(shared_ptr<Event> event) {
    if (!m_visible || !m_enable) return false;
    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left) {
        SPoint mp(event->mouseButton.x, event->mouseButton.y);
        if (isContainsPoint(mp.x, mp.y)) {
            if (m_onClick)
                m_onClick(getThis());
            fireCCallback(PropertyNames::kEventClick, CCallbackData::None, nullptr);
            return true;
        }
    }
    if (event->m_type == EventType::KeyDown && getFocused() &&
        (event->keyEvent.keycode == KeyCode::Return || event->keyEvent.keycode == KeyCode::Space)) {
        if (m_onClick)
            m_onClick(getThis());
        fireCCallback(PropertyNames::kEventClick, CCallbackData::None, nullptr);
        return true;
    }
    return false;
}

// ==================== ColorPicker ====================

ColorPicker::ColorPicker(Control* parent, SRect rect,
    float xScale, float yScale)
    : Panel(parent, rect, xScale, yScale),
    m_color(SColor::White()),
    m_committedColor(SColor::White()),
    m_presetColors(ConstDef::COLORPICKER_DEFAULT_PRESETS),
    m_presetCols(ConstDef::COLORPICKER_PRESET_COLS),
    m_presetRows(ConstDef::COLORPICKER_PRESET_ROWS),
    m_popupWidth(ConstDef::COLORPICKER_POPUP_WIDTH),
    m_popupHeight(ConstDef::COLORPICKER_POPUP_HEIGHT)
{
    m_ctlType = ControlType::ColorPicker;
    setFocusable(true);
    setTransparent(true);
    setBorderVisible(false);
}

ColorPicker::~ColorPicker() {
}

void ColorPicker::create() {
    if (m_isCreated) return;
    if (GET_CONTEXT == nullptr) return;  // 未挂入实例上下文：延迟创建

    Panel::create();
    setTransparent(true);
    setBorderVisible(false);
    // 关闭态（swatch + hex Label）：秒建。弹窗子树（Dialog/按钮/预设/滑块/hex）
    // 延迟到首次 openPopup()——见 ensurePopupBuilt()（P0-4：避免选中即付 ~3.5s 构建）
    createClosedStateControls();
}

// 构建弹窗子树（首次打开时调用一次，之后缓存复用）。原 create() 的 Dialog 段。
void ColorPicker::ensurePopupBuilt() {
    if (m_dialog || GET_CONTEXT == nullptr) return;
    m_dialog = make_shared<Dialog>(nullptr, SRect(0, 0, m_popupWidth, m_popupHeight),
                                   m_xScale, m_yScale);
    m_dialog->setConfirmButtonText(u8"确定");
    m_dialog->setCancelButtonText(u8"取消");
    m_dialog->setNormalStateBGColor(m_popupBGColor);
    m_dialog->setBorderVisible(true);
    m_dialog->setNormalStateBDColor(ConstDef::COLORPICKER_POPUP_BORDER);
    m_dialog->setRenderDevice(getRenderDevice());
    m_dialog->setTextRenderer(getTextRenderer());
    m_dialog->setResourceProvider(getResourceProvider());
    m_dialog->setInputBackend(getInputBackend());
    // 浮层继承宿主实例上下文：Dialog 以 nullptr 构造，无 setContext 传播路径，
    // 必须在宿主挂树后（GET_CONTEXT 就绪）显式补建
    m_dialog->setContext(GET_CONTEXT);
    m_dialog->setOnConfirm([this](shared_ptr<ConfirmPopup>) {
        onOK();
    });
    m_dialog->setOnClose([this](shared_ptr<Popup>, DialogResult r) {
        if (r == DialogResult::Cancelled) {
            m_color = m_committedColor;
            syncUIFromColor();
        }
    });
    m_dialog->setCloseOnClickOutside(true);
    m_dialog->setCloseOnEsc(true);
    m_dialog->setPadding(0);
    m_dialog->create();
    m_dialog->setVisible(false);
    recreatePopupContent();
}

void ColorPicker::recreatePopupContent() {
    if (!m_dialog) return;
    m_dialog->removeAllControls();
    m_dialog->recreateButtons();
    m_presetCells.clear();
    m_previewSwatch.reset();
    m_hexInput.reset();
    m_sliderR.reset();
    m_sliderG.reset();
    m_sliderB.reset();
    m_sliderA.reset();
    m_presetGridCreated = false;
    createPresetGrid();
    createHexInput();
    createSliders();
    layoutButtons();
}

void ColorPicker::setRect(SRect rect) {
    Panel::setRect(rect);
    if (!m_closedSwatch || !m_closedLabel) return;
    float yOff = (m_rect.height - m_swatchSize) / 2.0f;
    m_closedSwatch->setRect(SRect(0, yOff, m_swatchSize, m_swatchSize));
    m_closedLabel->setRect(SRect(m_swatchSize + ConstDef::COLORPICKER_HEX_GAP, 0,
        m_rect.width - m_swatchSize - ConstDef::COLORPICKER_HEX_GAP, m_rect.height));
}

// ==================== Closed State ====================

void ColorPicker::createClosedStateControls() {
    // 幂等：清理可能存在的旧实例（pre-create 属性应用、setContext 级联 recreate 等路径
    // 可能重复进入本函数——先清理再创建，保证关闭态恒为唯一一对）
    if (m_closedSwatch) { removeControl(m_closedSwatch); m_closedSwatch.reset(); }
    if (m_closedLabel)  { removeControl(m_closedLabel);  m_closedLabel.reset(); }

    float yOff = (m_rect.height - m_swatchSize) / 2.0f;

    m_closedSwatch = make_shared<Panel>(this,
        SRect(0, yOff, m_swatchSize, m_swatchSize), 1.0f, 1.0f);
    m_closedSwatch->setNormalStateBGColor(m_color);
    m_closedSwatch->setDisabledStateBGColor(m_color);
    m_closedSwatch->setBorderVisible(true);
    m_closedSwatch->setNormalStateBDColor(SColor(100, 100, 100, 255));
    m_closedSwatch->setEnable(false);
    m_closedSwatch->create();
    addControl(m_closedSwatch);

    m_closedLabel = make_shared<Label>(this,
        SRect(m_swatchSize + ConstDef::COLORPICKER_HEX_GAP, 0,
              m_rect.width - m_swatchSize - ConstDef::COLORPICKER_HEX_GAP, m_rect.height),
        1.0f, 1.0f);
    m_closedLabel->setCaption(m_color.toHex(true));
    m_closedLabel->setFontSize(m_closedFontSize);
    m_closedLabel->setFont(m_closedFontName);
    m_closedLabel->setMargin({2, 0, 0, 0});
    m_closedLabel->setAlignmentMode(AlignmentMode::AM_MID_LEFT);
    m_closedLabel->setTextNormalStateColor(m_closedTextColor);
    m_closedLabel->setTextDisabledStateColor(m_closedTextColor);
    m_closedLabel->setEnable(false);
    m_closedLabel->create();
    addControl(m_closedLabel);
}

void ColorPicker::recreateClosedState() {
    if (m_closedSwatch) removeControl(m_closedSwatch);
    if (m_closedLabel) removeControl(m_closedLabel);
    m_closedSwatch.reset();
    m_closedLabel.reset();
    createClosedStateControls();
    if (m_closedSwatch) {
        m_closedSwatch->setNormalStateBGColor(m_color);
        m_closedSwatch->setDisabledStateBGColor(m_color);
    }
    if (m_closedLabel)
        m_closedLabel->setCaption(m_color.toHex(true));
}

// ==================== Popup Control ====================

bool ColorPicker::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;   // enable/visible 守卫
    if (event->m_type == EventType::KeyDown &&
        (event->keyEvent.keycode == KeyCode::Return || event->keyEvent.keycode == KeyCode::Space) &&
        getFocused() && !isPopupVisible()) {
        togglePopup();
        return true;
    }
    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left) {
        SPoint mp(event->mouseButton.x, event->mouseButton.y);
        if (isContainsPoint(mp.x, mp.y)) {
            togglePopup();
            return true;
        }
    }
    return Panel::handleEvent(event);
}

SRect ColorPicker::computePopupRect() {
    SRect dr = getDrawRect();
    float sx = getScaleXX();
    float sy = getScaleYY();
    float pw = m_popupWidth;
    float ph = m_popupHeight;
    float renderedW = pw * sx;
    float renderedH = ph * sy;
    // 视口绝对钳制（多视口场景弹层按视口区域钳制；dr 为窗口绝对
    // 坐标，钳制边界须为 vp.left/top + 尺寸）
    SRect vp = GET_CONTEXT ? GET_CONTEXT->viewport : SRect(0, 0, 1024, 768);
    float screenW = vp.left + vp.width;
    float screenH = vp.top + vp.height;

    // 1) Try below
    float by = dr.bottom() + 2.0f;
    if (by + renderedH <= screenH) {
        float x = dr.left;
        if (x + renderedW > screenW) x = screenW - renderedW;
        if (x < vp.left) x = vp.left;
        return SRect(x, by, pw, ph);
    }

    // 2) Try right (vertically centered on CP)
    float rx = dr.right();
    float ry = dr.top + (dr.height - renderedH) * 0.5f;
    if (rx + renderedW <= screenW) {
        if (ry < vp.top) ry = vp.top;
        if (ry + renderedH > screenH) ry = screenH - renderedH;
        return SRect(rx, ry, pw, ph);
    }

    // 3) Try left (vertically centered on CP)
    float lx = dr.left - renderedW - 2.0f;
    float ly = dr.top + (dr.height - renderedH) * 0.5f;
    if (lx >= vp.left) {
        if (ly < vp.top) ly = vp.top;
        if (ly + renderedH > screenH) ly = screenH - renderedH;
        return SRect(lx, ly, pw, ph);
    }

    // 4) Try above
    float ay = dr.top - renderedH - 2.0f;
    if (ay >= vp.top) {
        float x = dr.left;
        if (x + renderedW > screenW) x = screenW - renderedW;
        if (x < vp.left) x = vp.left;
        return SRect(x, ay, pw, ph);
    }

    // 4) Fallback: clamp below position to viewport
    float fx = dr.left;
    if (fx + renderedW > screenW) fx = screenW - renderedW;
    if (fx < vp.left) fx = vp.left;
    float fy = dr.bottom() + 2.0f;
    if (fy + renderedH > screenH) fy = screenH - renderedH;
    return SRect(fx, fy, pw, ph);
}

void ColorPicker::openPopup() {
    ensurePopupBuilt();
    if (!m_dialog) return;
    SRect pr = computePopupRect();
    // 绝对坐标（computePopupRect 基于 getDrawRect）转父（bench）相对本地坐标。
    // 弹层复合恒 1（物理像素面板），但位置经 getDrawRect 乘父复合 → 需除根复合反查
    SRect br = BENCH ? BENCH->getDrawRect() : SRect(0, 0, 0, 0);
    float bsx = BENCH ? BENCH->getScaleXX() : 1.0f;
    float bsy = BENCH ? BENCH->getScaleYY() : 1.0f;
    pr.left = (pr.left - br.left) / bsx;
    pr.top = (pr.top - br.top) / bsy;
    m_dialog->setAbsolute(pr);
    layoutButtons();
    m_committedColor = m_color;
    syncUIFromColor();
    m_dialog->open();
    if (m_hexInput)
        GET_FOCUSMANAGER->focusControl(m_hexInput.get());
}

void ColorPicker::closePopup() {
    if (!m_dialog) return;
    m_dialog->close();
    GET_FOCUSMANAGER->focusControl(this);
}

void ColorPicker::togglePopup() {
    if (m_dialog && m_dialog->getVisible())
        closePopup();
    else
        openPopup();
}

bool ColorPicker::isPopupVisible() const {
    return m_dialog && m_dialog->getVisible();
}

// ==================== Create Sub-Controls ====================

void ColorPicker::createPresetGrid() {
    if (m_presetGridCreated || !m_dialog) return;
    float pad = ConstDef::COLORPICKER_POPUP_PADDING;
    float gap = ConstDef::COLORPICKER_PRESET_GAP;
    float cellH = ConstDef::COLORPICKER_PRESET_CELL_H;
    float usable = m_popupWidth - pad * 2 - (m_presetCols - 1) * gap;
    float cellW = usable / m_presetCols;
    float gridTop = pad;
    int total = (std::min)((int)m_presetColors.size(), m_presetCols * m_presetRows);
    for (int i = 0; i < total; i++) {
        int col = i % m_presetCols;
        int row = i / m_presetCols;
        SRect r(pad + col * (cellW + gap), gridTop + row * (cellH + gap),
                cellW, cellH);
        auto cell = make_shared<PresetCell>(m_dialog.get(), r, m_presetColors[i],
            1.0f, 1.0f);
        cell->setSelected(m_presetColors[i] == m_color);
        SColor color = m_presetColors[i];
        cell->setOnClick([this, color](shared_ptr<Control>) {
            onPresetClicked(color);
        });
        cell->create();
        m_presetCells.push_back(cell);
        m_dialog->addControl(cell);
    }
    m_presetGridCreated = true;
}

void ColorPicker::createHexInput() {
    if (!m_dialog) return;
    float pad = ConstDef::COLORPICKER_POPUP_PADDING;
    float gap = ConstDef::COLORPICKER_PRESET_GAP;
    float cellH = ConstDef::COLORPICKER_PRESET_CELL_H;
    float hashW = ConstDef::COLORPICKER_HEX_HASH_W;
    float inputH = ConstDef::COLORPICKER_HEX_INPUT_H;
    float usable = m_popupWidth - pad * 2 - (m_presetCols - 1) * gap;
    float cellW = usable / m_presetCols;
    float gridBottom = pad + ConstDef::COLORPICKER_PRESET_ROWS * (cellH + gap);
    float rowTop = gridBottom + 8.0f;

    // Preview swatch (same size as one preset cell)
    m_previewSwatch = make_shared<Panel>(m_dialog.get(),
        SRect(pad, rowTop, cellW, cellH), 1.0f, 1.0f);
    m_previewSwatch->setNormalStateBGColor(m_color);
    m_previewSwatch->setDisabledStateBGColor(m_color);
    m_previewSwatch->setBorderVisible(true);
    m_previewSwatch->setNormalStateBDColor(ConstDef::COLORPICKER_POPUP_BORDER);
    m_previewSwatch->setEnable(false);
    m_previewSwatch->create();
    m_dialog->addControl(m_previewSwatch);

    // "#" label
    float hashX = pad + cellW + 5.0f;
    float hashY = rowTop + (cellH - 16.0f) / 2.0f;
    auto hashLabel = make_shared<Label>(m_dialog.get(),
        SRect(hashX, hashY, hashW, 16.0f), 1.0f, 1.0f);
    hashLabel->setCaption("#");
    hashLabel->setFontSize(ConstDef::COLORPICKER_HEX_FONT_SIZE);
    hashLabel->setMargin({1, 1, 1, 1});
    hashLabel->setTextNormalStateColor(ConstDef::DEFAULT_TEXT_NORMAL_COLOR);
    hashLabel->setTextDisabledStateColor(ConstDef::DEFAULT_TEXT_NORMAL_COLOR);
    hashLabel->setEnable(false);
    hashLabel->create();
    m_dialog->addControl(hashLabel);

    // Hex input
    float hexX = hashX + hashW + 5.0f;
    float hexW = m_popupWidth - hexX - pad;
    float hexY = rowTop + (cellH - inputH) / 2.0f;
    m_hexInput = make_shared<EditBox>(m_dialog.get(),
        SRect(hexX, hexY, hexW, inputH), 1.0f, 1.0f);
    m_hexInput->setText(m_color.toRRGGBBAA());
    m_hexInput->setOnEnter([this](shared_ptr<Control>) {
        onHexInputChanged(m_hexInput->getText());
    });
    m_hexInput->create();
    m_dialog->addControl(m_hexInput);
}

void ColorPicker::createSliders() {
    if (!m_dialog) return;
    float pad = ConstDef::COLORPICKER_POPUP_PADDING;
    float gap = ConstDef::COLORPICKER_PRESET_GAP;
    float cellH = ConstDef::COLORPICKER_PRESET_CELL_H;
    float sliderH = ConstDef::COLORPICKER_SLIDER_H;
    float labelW = ConstDef::COLORPICKER_SLIDER_LABEL_W;
    float gridBottom = pad + ConstDef::COLORPICKER_PRESET_ROWS * (cellH + gap);
    float rowTop = gridBottom + 8.0f;
    float rowBottom = rowTop + cellH;

    // Track area: pad + label(14) + gap(5) → track starts, ends at pad from right
    float trackX = pad + labelW + 5.0f;
    float trackW = m_popupWidth - trackX - pad;

    // Slider body Y = previous_bottom + 10(value_label_gap) + labelH(18) - labelGap(-2)
    // = prev_bottom + 10 + 18 - 2 = prev_bottom + 26
    // Value label H ≈ 18 (textH=14 + margin 2 + 2 extra)
    float sliderY0 = rowBottom + 26.0f;

    auto makeLetter = [&](float sx, const string& ch) {
        auto lb = make_shared<Label>(m_dialog.get(),
            SRect(pad, sx + (sliderH - 16.0f) / 2.0f, labelW, 16.0f),
            1.0f, 1.0f);
        lb->setCaption(ch);
        lb->setFontSize(ConstDef::COLORPICKER_HEX_FONT_SIZE);
        lb->setMargin({1, 1, 1, 1});
        lb->setTextNormalStateColor(ConstDef::DEFAULT_TEXT_NORMAL_COLOR);
        lb->setTextDisabledStateColor(ConstDef::DEFAULT_TEXT_NORMAL_COLOR);
        lb->setEnable(false);
        lb->create();
        m_dialog->addControl(lb);
    };

    auto makeSlider = [&](float yOff, const SColor& fillCol,
                          const string& letter) -> shared_ptr<Slider> {
        float sy = sliderY0 + yOff;
        makeLetter(sy, letter);
        auto sl = make_shared<Slider>(m_dialog.get(),
            SRect(trackX, sy, trackW, sliderH),
            1.0f, 1.0f);
        sl->setRange(0, 255);
        sl->setStep(1);
        sl->setValue(128);
        sl->setShowValueLabel(true);
        sl->setLabelGap(-8.0f);
        sl->setTrackColor(SColor(45, 45, 48, 255));
        sl->setTrackFillColor(fillCol);
        sl->setThumbBorderColor(fillCol);
        sl->create();
        m_dialog->addControl(sl);
        return sl;
    };

    float step = sliderH + 26.0f;

    m_sliderR = makeSlider(0, SColor(255, 68, 68, 255), "R");
    m_sliderR->setOnValueChanged([this](shared_ptr<Slider> s, float v) {
        onSliderRChanged(s, v);
    });

    m_sliderG = makeSlider(step, SColor(68, 255, 68, 255), "G");
    m_sliderG->setOnValueChanged([this](shared_ptr<Slider> s, float v) {
        onSliderGChanged(s, v);
    });

    m_sliderB = makeSlider(step * 2, SColor(68, 68, 255, 255), "B");
    m_sliderB->setOnValueChanged([this](shared_ptr<Slider> s, float v) {
        onSliderBChanged(s, v);
    });

    m_sliderA = makeSlider(step * 3, SColor(136, 136, 136, 255), "A");
    m_sliderA->setOnValueChanged([this](shared_ptr<Slider> s, float v) {
        onSliderAChanged(s, v);
    });
}

void ColorPicker::layoutButtons() {
    if (!m_dialog) return;
    float pad = ConstDef::COLORPICKER_POPUP_PADDING;
    float cellH = ConstDef::COLORPICKER_PRESET_CELL_H;
    float sliderH = ConstDef::COLORPICKER_SLIDER_H;
    float btnW = ConstDef::COLORPICKER_BTN_W;
    float btnH = ConstDef::COLORPICKER_BTN_H;
    float btnGap = ConstDef::COLORPICKER_BTN_GAP;
    float gridBottom = pad + ConstDef::COLORPICKER_PRESET_ROWS * (cellH + ConstDef::COLORPICKER_PRESET_GAP);
    float rowTop = gridBottom + 8.0f;
    float rowBottom = rowTop + cellH;
    float sliderY0 = rowBottom + 26.0f;
    float step = sliderH + 26.0f;
    float sliderABottom = sliderY0 + step * 3 + sliderH;
    float btnY = sliderABottom + 10.0f;

    float btnRightX = m_popupWidth - pad - btnW;

    m_dialog->setConfirmButtonRect(SRect(btnRightX - btnW - btnGap, btnY, btnW, btnH));
    m_dialog->setCancelButtonRect(SRect(btnRightX, btnY, btnW, btnH));
}

// ==================== Color ====================

void ColorPicker::setColor(const SColor& color) {
    m_color = color;
    syncUIFromColor();
}

void ColorPicker::setColor(const string& hex) {
    SColor c;
    if (SColor::fromHex(hex, c))
        setColor(c);
}

void ColorPicker::setPresetColors(const vector<SColor>& colors) {
    m_presetColors = colors;
    m_presetGridCreated = false;
    if (m_dialog && m_dialog->getVisible()) {
        recreatePopupContent();
        syncUIFromColor();
    }
}

void ColorPicker::setPresetLayout(int cols, int rows) {
    m_presetCols = (std::max)(1, cols);
    m_presetRows = (std::max)(1, rows);
    m_presetGridCreated = false;
    if (m_dialog) {
        recreatePopupContent();
        syncUIFromColor();
    }
}

// ==================== Sync ====================

void ColorPicker::syncUIFromColor() {
    string h8 = m_color.toRRGGBBAA();
    if (m_hexInput && m_hexInput->getText() != h8)
        m_hexInput->setText(h8);
    if (m_previewSwatch) {
        m_previewSwatch->setNormalStateBGColor(m_color);
        m_previewSwatch->setDisabledStateBGColor(m_color);
    }
    if (m_closedSwatch) {
        m_closedSwatch->setNormalStateBGColor(m_color);
        m_closedSwatch->setDisabledStateBGColor(m_color);
    }
    if (m_closedLabel)
        m_closedLabel->setCaption(m_color.toHex(true));
    m_isSyncing = true;
    if (m_sliderR) m_sliderR->setValue((float)m_color.redByte());
    if (m_sliderG) m_sliderG->setValue((float)m_color.greenByte());
    if (m_sliderB) m_sliderB->setValue((float)m_color.blueByte());
    if (m_sliderA) m_sliderA->setValue((float)m_color.alphaByte());
    m_isSyncing = false;
    for (auto& cell : m_presetCells) {
        cell->setSelected(cell->getColor() == m_color);
    }
}

void ColorPicker::syncColorFromSliders() {
    if (m_isSyncing || !m_sliderR || !m_sliderG || !m_sliderB || !m_sliderA) return;
    int r = (std::min)((std::max)((int)m_sliderR->getValue(), 0), 255);
    int g = (std::min)((std::max)((int)m_sliderG->getValue(), 0), 255);
    int b = (std::min)((std::max)((int)m_sliderB->getValue(), 0), 255);
    int a = (std::min)((std::max)((int)m_sliderA->getValue(), 0), 255);
    m_color = SColor(r, g, b, a);
    if (m_previewSwatch) {
        m_previewSwatch->setNormalStateBGColor(m_color);
        m_previewSwatch->setDisabledStateBGColor(m_color);
    }
    if (m_closedSwatch) {
        m_closedSwatch->setNormalStateBGColor(m_color);
        m_closedSwatch->setDisabledStateBGColor(m_color);
    }
    if (m_closedLabel)
        m_closedLabel->setCaption(m_color.toHex(true));
    for (auto& cell : m_presetCells) {
        cell->setSelected(cell->getColor() == m_color);
    }
    string h8 = m_color.toRRGGBBAA();
    if (m_hexInput && m_hexInput->getText() != h8)
        m_hexInput->setText(h8);
}

// ==================== Event Handlers ====================

void ColorPicker::onPresetClicked(const SColor& color) {
    setColor(color);
}

void ColorPicker::onHexInputChanged(const string& text) {
    string filtered;
    for (char c : text) {
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F'))
            filtered += c;
    }
    SColor c;
    if (SColor::fromRRGGBBAA(filtered, c)) {
        m_color = c;
    }
    syncUIFromColor();
}

void ColorPicker::onSliderRChanged(shared_ptr<Slider> s, float val) {
    syncColorFromSliders();
}

void ColorPicker::onSliderGChanged(shared_ptr<Slider> s, float val) {
    syncColorFromSliders();
}

void ColorPicker::onSliderBChanged(shared_ptr<Slider> s, float val) {
    syncColorFromSliders();
}

void ColorPicker::onSliderAChanged(shared_ptr<Slider> s, float val) {
    syncColorFromSliders();
}

void ColorPicker::onOK() {
    if (m_hexInput) onHexInputChanged(m_hexInput->getText());
    m_committedColor = m_color;
    if (m_onColorChanged)
        m_onColorChanged(std::dynamic_pointer_cast<ColorPicker>(getThis()), m_committedColor);
    ColorPayload cp = { m_committedColor.redByte(), m_committedColor.greenByte(),
                        m_committedColor.blueByte(), m_committedColor.alphaByte() };
    fireCCallback(PropertyNames::kEventColorChanged, CCallbackData::Color, &cp);
}

void ColorPicker::onCancel() {
    m_color = m_committedColor;
    syncUIFromColor();
}

// ── Property system overrides ──

// P0-26：文本/文本阴影族转发关闭态 Label（含四态对象与单态键；自身字段同步以保读回一致）
void ColorPicker::setTextStateColor(StateColor stateColor) {
    ControlImpl::setTextStateColor(stateColor);
    if (m_closedLabel) m_closedLabel->setTextStateColor(stateColor);
}
void ColorPicker::setTextShadowStateColor(StateColor stateColor) {
    ControlImpl::setTextShadowStateColor(stateColor);
    if (m_closedLabel) m_closedLabel->setTextShadowStateColor(stateColor);
}

int ColorPicker::setColorProperty(const char* prop, SColor color) {
    if (m_closedLabel && ControlImpl::isTextColorFamilyKey(prop)) {
        m_closedLabel->setColorProperty(prop, color);
        ControlImpl::setColorProperty(prop, color);
        return 1;
    }
    if (strcmp(prop, PropertyNames::kClosedText) == 0) { setClosedTextColor(color); return 1; }
    if (strcmp(prop, PropertyNames::kPopupBG) == 0)    { setPopupBGColor(color);    return 1; }
    return Panel::setColorProperty(prop, color);
}

int ColorPicker::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) {   // P0-26：关闭态文字阴影开关
        if (m_closedLabel) { m_closedLabel->setShadow(value != 0); return 1; }
        return 0;
    }
    return Panel::setBoolProperty(prop, value);
}

int ColorPicker::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0)       { setClosedFontSize(value);             return 1; }  // P0-26：通用字号→关闭态
    if (strcmp(prop, PropertyNames::kPresetCols) == 0)     { setPresetLayout(value, m_presetRows); return 1; }
    if (strcmp(prop, PropertyNames::kPresetRows) == 0)     { setPresetLayout(m_presetCols, value); return 1; }
    if (strcmp(prop, PropertyNames::kClosedFontSize) == 0) { setClosedFontSize(value);             return 1; }
    return Panel::setIntProperty(prop, value);
}

int ColorPicker::setFloatProperty(const char* prop, float value) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) {   // P0-26：关闭态文字阴影偏移
        if (m_closedLabel) { SPoint o = m_closedLabel->getShadowOffset(); o.x = value; m_closedLabel->setShadowOffset(o); return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) {
        if (m_closedLabel) { SPoint o = m_closedLabel->getShadowOffset(); o.y = value; m_closedLabel->setShadowOffset(o); return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kClosedSwatchSize) == 0) { setClosedSwatchSize(value); return 1; }
    return Panel::setFloatProperty(prop, value);
}

int ColorPicker::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kFont) == 0) { setClosedFont(FontNameFromString(value)); return 1; }  // P0-26：通用字体名→关闭态
    return Panel::setEnumProperty(prop, value);
}

int ColorPicker::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kColor) == 0) { setColor(string(value)); return 1; }
    return Panel::setStringProperty(prop, value);
}

// ── Getter property system overrides ──

int ColorPicker::getColorProperty(const char* prop, SColor& out) {
    if (strcmp(prop, PropertyNames::kClosedText) == 0) { out = m_closedTextColor; return 1; }
    if (strcmp(prop, PropertyNames::kPopupBG) == 0)    { out = m_popupBGColor;    return 1; }
    return Panel::getColorProperty(prop, out);
}

int ColorPicker::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kShadow) == 0) {
        if (m_closedLabel) { out = m_closedLabel->isShadowEnabled() ? 1 : 0; return 1; }
        return 0;
    }
    return Panel::getBoolProperty(prop, out);
}

int ColorPicker::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0)       { out = m_closedFontSize; return 1; }
    if (strcmp(prop, PropertyNames::kPresetCols) == 0)     { out = m_presetCols;    return 1; }
    if (strcmp(prop, PropertyNames::kPresetRows) == 0)     { out = m_presetRows;    return 1; }
    if (strcmp(prop, PropertyNames::kClosedFontSize) == 0) { out = m_closedFontSize; return 1; }
    return Panel::getIntProperty(prop, out);
}

int ColorPicker::getFloatProperty(const char* prop, float& out) {
    if (strcmp(prop, PropertyNames::kShadowOffsetX) == 0) {
        if (m_closedLabel) { out = m_closedLabel->getShadowOffset().x; return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kShadowOffsetY) == 0) {
        if (m_closedLabel) { out = m_closedLabel->getShadowOffset().y; return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kClosedSwatchSize) == 0) { out = m_swatchSize; return 1; }
    return Panel::getFloatProperty(prop, out);
}

int ColorPicker::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kFont) == 0) { out = FontNameToString(m_closedFontName); return 1; }
    return Panel::getEnumProperty(prop, out);
}

int ColorPicker::getStringProperty(const char* prop, const char*& out) {
    return Panel::getStringProperty(prop, out);
}

int ColorPicker::setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) {
    if (strcmp(event, PropertyNames::kEventColorChanged) == 0)
        return ControlImpl::setCallbackProperty(event, cb, userData);
    return ControlImpl::setCallbackProperty(event, cb, userData);
}

// ==================== ColorPickerBuilder ====================

ColorPickerBuilder::ColorPickerBuilder(Control* parent, SRect rect,
    float xScale, float yScale)
    : m_colorPicker(nullptr)
{
    m_colorPicker = make_shared<ColorPicker>(parent, rect, xScale, yScale);
}

ColorPickerBuilder& ColorPickerBuilder::setColor(const string& hex)
{ m_colorPicker->setColor(hex); return *this; }

ColorPickerBuilder& ColorPickerBuilder::setPresetColors(const vector<string>& hexList)
{
    vector<SColor> colors;
    for (auto& h : hexList) {
        SColor c;
        if (SColor::fromHex(h, c))
            colors.push_back(c);
    }
    m_colorPicker->setPresetColors(colors);
    return *this;
}

ColorPickerBuilder& ColorPickerBuilder::setPresetLayout(int cols, int rows)
{ m_colorPicker->setPresetLayout(cols, rows); return *this; }

ColorPickerBuilder& ColorPickerBuilder::setOnColorChanged(
    ColorPicker::OnColorChangedHandler handler)
{ m_colorPicker->setOnColorChanged(handler); return *this; }

ColorPickerBuilder& ColorPickerBuilder::setBackgroundStateColor(StateColor stateColor)
{ m_colorPicker->setBackgroundStateColor(stateColor); return *this; }

ColorPickerBuilder& ColorPickerBuilder::setBorderStateColor(StateColor stateColor)
{ m_colorPicker->setBorderStateColor(stateColor); return *this; }

ColorPickerBuilder& ColorPickerBuilder::setClosedSwatchSize(float size)
{ m_colorPicker->setClosedSwatchSize(size); return *this; }

ColorPickerBuilder& ColorPickerBuilder::setClosedFontSize(int size)
{ m_colorPicker->setClosedFontSize(size); return *this; }

ColorPickerBuilder& ColorPickerBuilder::setClosedTextColor(const string& hex)
{ SColor c; if (SColor::fromHex(hex, c)) m_colorPicker->setClosedTextColor(c); return *this; }

ColorPickerBuilder& ColorPickerBuilder::setPopupBGColor(const string& hex)
{ SColor c; if (SColor::fromHex(hex, c)) m_colorPicker->setPopupBGColor(c); return *this; }

shared_ptr<ColorPicker> ColorPickerBuilder::build(void)
{
    m_colorPicker->create();
    if (!m_colorPicker->getVisible())
        m_colorPicker->setVisible(true);
    return m_colorPicker;
}
