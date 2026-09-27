// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#define NOMINMAX
#include "EditBox.h"
#include <iostream>
#include "MainWindow.h"
#include "EventQueue.h"
#include "PropertyNames.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

EditBox::EditBox(Control *parent, SRect rect, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
    , m_cursorPosition(0)
    , m_selectionStart(0)
    , m_selectionEnd(0)
    , m_passwordMode(false)
    , m_passwordChar('*')
    , m_cursorBlinkTime(0)
    , m_cursorVisible(true)
    , m_shiftPressed(false)
    , m_ctrlPressed(false)
    , m_isDragging(false)
    , m_dragStartPosition(0)
    , m_textOffsetX(8.0f)
    , m_textOffsetY(0)
    , m_font(nullptr)
    , m_fontSize(16)
    , m_fontName(FontName::HarmonyOS_Sans_SC_Regular)
    , m_focusWatcherRegistered(false)
    , m_AlignmentMode(AlignmentMode::AM_MID_LEFT)
    , m_fontScaleDirty(false)
{
    m_ctlType = ControlType::EditBox;
    m_id = 0;
    m_visible = true;
    m_enable = true;
    m_isBorderVisible = true;
    m_isTransparent = false;
    m_state = ControlState::Normal;

    setRect(rect);

    m_margin = Margin(8.0f, 4.0f, 8.0f, 4.0f);

    setFocusable(true);

    if (GET_CONTEXT != nullptr) loadFontInternal();  // 两阶段：挂树后由 create() 加载
    updateTextOffset();

    InputBackend* ib = getInputBackend();
    if (ib) ib->startTextInput();
}

EditBox::~EditBox() {
}

void EditBox::create() {
    if (m_isCreated) return;
    if (GET_CONTEXT == nullptr) return;  // 未挂入实例上下文：延迟创建（字体依赖 context）

    ControlImpl::create();
    loadFontInternal();
    updateTextOffset();
    // 激活文本输入：构造时可能无 context（未挂树）导致构造中的 startTextInput 未执行，
    // 挂树后 create() 由 setContext→recreate() 重跑，在此补齐
    InputBackend* ib = getInputBackend();
    if (ib) ib->startTextInput();
}

void EditBox::loadFontInternal() {
    if (GET_CONTEXT == nullptr) return;  // 两阶段：挂树后由 create() 加载
    ResourceProvider* provider = getResourceProvider();
    if (provider == nullptr) {
        printf("EditBox::loadFontInternal: No resource provider\n");
        return;
    }

    m_fontData = provider->readFile(ConstDef::fontFiles.at(m_fontName));
    if (m_fontData == nullptr || m_fontData->empty()) {
        printf("EditBox::loadFontInternal: Failed to load font\n");
        return;
    }

    int scaledFontSize = (int)(m_fontSize * getScaleXX());
    m_font = getTextRenderer()->loadFontFromMemoryWithText(m_fontData->data(), m_fontData->size(), scaledFontSize, m_text);
    if (!m_font) {
        printf("Failed to load font for EditBox\n");
    }
}

std::string EditBox::getDisplayText() const {
    if (m_passwordMode && !m_text.empty()) {
        return std::string(m_text.length(), m_passwordChar);
    }
    return m_text;
}

int EditBox::getUtf8CharLength(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

std::string EditBox::getUtf8Substr(const std::string& str, int startByte, int byteCount) const {
    if (startByte < 0 || startByte >= (int)str.length()) return "";
    if (byteCount <= 0) return "";

    while (startByte > 0 && (str[startByte] & 0xC0) == 0x80) {
        startByte--;
    }

    int endByte = startByte;
    int remaining = byteCount;
    while (remaining > 0 && endByte < (int)str.length()) {
        int charLen = getUtf8CharLength((unsigned char)str[endByte]);
        if (endByte + charLen > startByte + byteCount) break;
        endByte += charLen;
        remaining -= charLen;
    }

    return str.substr(startByte, endByte - startByte);
}

float EditBox::getTextWidth(const std::string& text) {
    if (!m_font || text.empty()) return 0;

    SSize size = getTextRenderer()->measureText(m_font.get(), text);
    return size.width;
}

int EditBox::getCursorFromPosition(float x) {
    std::string displayText = getDisplayText();
    float textX = m_textOffsetX;

    int bestOffset = 0;
    float bestDist = 99999.0f;

    for (int i = 0; i <= (int)displayText.length(); ) {
        std::string prefix = getUtf8Substr(displayText, 0, i);
        float charX = textX + getTextWidth(prefix);
        float dist = std::abs(x - charX);

        if (dist < bestDist) {
            bestDist = dist;
            bestOffset = i;
        }

        if (i >= (int)displayText.length()) break;

        int charLen = getUtf8CharLength((unsigned char)displayText[i]);
        i += charLen;
    }

    return bestOffset;
}

float EditBox::getCursorX(int cursorPos) {
    std::string displayText = getDisplayText();
    std::string prefix = getUtf8Substr(displayText, 0, cursorPos);
    return m_textOffsetX + getTextWidth(prefix);
}

void EditBox::updateTextOffset() {
    SRect rect = getRect();
    SRect drawRect = getDrawRect();
    float scaleX = getScaleXX();
    float scaleY = getScaleYY();
    float fontHeight = (getTextRenderer() && getFont())
        ? (float)getTextRenderer()->getFontHeight(getFont()) : m_fontSize * scaleY;
    m_textOffsetY = (drawRect.height - fontHeight) / 2.0f;

    float textWidth = getTextWidth(getDisplayText());
    float visibleWidth = drawRect.width - (m_margin.left + m_margin.right) * scaleX;
    float margin = m_margin.left * scaleX;

    if (textWidth <= visibleWidth) {
        m_textOffsetX = margin;
        return;
    }

    std::string textBeforeCursor = getUtf8Substr(getDisplayText(), 0, m_cursorPosition);
    float cursorPixelX = getTextWidth(textBeforeCursor);

    float minVisibleX = margin;
    float maxVisibleX = visibleWidth - margin;
    float cursorDisplayX = cursorPixelX + m_textOffsetX;

    if (cursorDisplayX >= minVisibleX && cursorDisplayX <= maxVisibleX) {
        return;
    }

    if (cursorDisplayX > maxVisibleX) {
        float targetOffset = cursorPixelX - maxVisibleX + margin;
        float maxOffset = textWidth - visibleWidth + margin;
        if (targetOffset > maxOffset) targetOffset = maxOffset;
        m_textOffsetX = margin - targetOffset;
    } else if (cursorDisplayX < minVisibleX) {
        m_textOffsetX = margin - cursorPixelX;
    }

    if (m_textOffsetX > margin) m_textOffsetX = margin;
    float minOffset = margin - (textWidth - visibleWidth);
    if (m_textOffsetX < minOffset) m_textOffsetX = minOffset;
}

void EditBox::insertText(const std::string& text) {
    deleteSelectedText();

    int pos = m_cursorPosition;
    m_text.insert(pos, text);
    m_cursorPosition += (int)text.length();

    clearSelection();

    // Reload font with new text to pick up any new codepoints (e.g., CJK)
    loadFontInternal();

    updateTextOffset();

    if (m_onTextChanged) {
        m_onTextChanged(getThis(), m_text);
    }
    fireCCallback(PropertyNames::kEventTextChanged, CCallbackData::String, m_text.c_str());
}

void EditBox::deleteSelectedText() {
    if (m_selectionStart == m_selectionEnd) return;

    int start = std::min(m_selectionStart, m_selectionEnd);
    int endVal = std::max(m_selectionStart, m_selectionEnd);

    m_text.erase(start, endVal - start);
    m_cursorPosition = start;
    clearSelection();
}

void EditBox::update(void) {
    if (!getEnable()) return;   // enable 守卫（同 Label/Panel；ComboBox/TextArea/NumericUpDown 继承受益）
    if (m_fontScaleDirty && m_visible) {
        m_fontScaleDirty = false;
        loadFontInternal();
        updateTextOffset();
    }
    if (m_focused) {
        m_cursorBlinkTime += 16;
        if (m_cursorBlinkTime >= 500) {
            m_cursorVisible = !m_cursorVisible;
            m_cursorBlinkTime = 0;
        }
    }
    ControlImpl::update();   // P0-30：hover 检测 + 子控件递归（Panel 同款约定；原缺失致 EditBox 族 hover 不可达）
}

void EditBox::draw(void) {
    if (!m_visible) return;

    ControlImpl::beforeDraw();

    SRect drawRect = getDrawRect();
    float fontHeight = (getTextRenderer() && getFont())
        ? (float)getTextRenderer()->getFontHeight(getFont()) : m_fontSize * getScaleYY();

    float scaleX = getScaleXX();
    float scaleY = getScaleYY();
    float marginX = m_margin.left * scaleX;
    float marginY = m_margin.top * scaleY;
    float marginRight = m_margin.right * scaleX;
    float marginBottom = m_margin.bottom * scaleY;
    SRect clipRect(drawRect.left + marginX, drawRect.top + marginY,
                   drawRect.width - marginX - marginRight, drawRect.height - marginY - marginBottom);
    GET_RENDERDEVICE->pushClipRect(clipRect);

    if (hasSelection() && m_focused) {
        int selStart = std::min(m_selectionStart, m_selectionEnd);
        int selEnd = std::max(m_selectionStart, m_selectionEnd);

        std::string displayText = getDisplayText();
        std::string prefixForStart = getUtf8Substr(displayText, 0, selStart);
        std::string prefixForEnd = getUtf8Substr(displayText, 0, selEnd);

        float startX = m_textOffsetX + getTextWidth(prefixForStart);
        float endX = m_textOffsetX + getTextWidth(prefixForEnd);

        SRect selRect(drawRect.left + startX, drawRect.top + m_textOffsetY,
                      endX - startX, fontHeight);
        GET_RENDERDEVICE->setDrawColor(SColor(100, 149, 237, 128));
        GET_RENDERDEVICE->fillRect(selRect);
    }

    if (!m_text.empty() && m_font) {
        SColor textColor = m_textColor.getNormal();
        getTextRenderer()->drawText(m_font.get(), getDisplayText(),
            drawRect.left + m_textOffsetX, drawRect.top + m_textOffsetY, textColor);
    } else if (!m_placeholderText.empty() && m_font) {
        SColor placeholderColor(128, 128, 128, 255);
        getTextRenderer()->drawText(m_font.get(), m_placeholderText,
            drawRect.left + m_textOffsetX, drawRect.top + m_textOffsetY, placeholderColor);
    }

    GET_RENDERDEVICE->popClipRect();

    if (m_focused && m_cursorVisible && m_selectionStart == m_selectionEnd) {
        float cursorX = getCursorX(m_cursorPosition);

        SRect cursorRect(drawRect.left + cursorX, drawRect.top + m_textOffsetY,
                         2.0f, fontHeight);

        GET_RENDERDEVICE->setDrawColor(m_textColor.getNormal());
        GET_RENDERDEVICE->fillRect(cursorRect);
    }

    afterDraw();
}

bool EditBox::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;

    if (event->m_type == EventType::MouseDown && event->mouseButton.button == MouseButton::Left) {
        if (isContainsPoint(event->mouseButton.x, event->mouseButton.y)) {
            setFocused(true);

            int newCursor = getCursorFromPosition(event->mouseButton.x - getDrawRect().left);

            KeyMod mod = getInputBackend() ? getInputBackend()->getModState() : KeyMod::None;
            bool shiftPressed = isModSet(mod, KeyMod::Shift);

            if (shiftPressed) {
                m_selectionEnd = newCursor;
            } else {
                m_cursorPosition = newCursor;
                clearSelection();
            }

            m_cursorVisible = true;
            m_cursorBlinkTime = 0;
            m_isDragging = true;
            m_dragStartPosition = newCursor;
            updateTextOffset();
            applyPressState(true);   // P0-30：按下切态（EditBox 自处理点击不链基类）

            return true;
        } else {
            setFocused(false);
        }
    }

    if (event->m_type == EventType::MouseMove) {
        if (m_focused && m_isDragging) {
            int newCursor = getCursorFromPosition(event->mousePos.x - getDrawRect().left);

            int start = std::min(m_dragStartPosition, newCursor);
            int end = std::max(m_dragStartPosition, newCursor);
            m_selectionStart = start;
            m_selectionEnd = end;
            m_cursorPosition = newCursor;

            updateTextOffset();
            return true;
        }
    }

    if (event->m_type == EventType::MouseUp && event->mouseButton.button == MouseButton::Left) {
        m_isDragging = false;
    }

    if (event->m_type == EventType::TextInput) {
        if (m_focused) {
            std::string data(event->textInput.text);
            std::string filtered;
            for (char c : data) {
                if (c == '\n' || c == '\r') continue;
                if (static_cast<unsigned char>(c) < 0x20 && c != '\t') continue;
                filtered += c;
            }
                if (m_passwordMode) {
                    std::string pwdFiltered;
                    for (char c : filtered) {
                        if ((unsigned char)c < 128 && c >= 32) {
                            pwdFiltered += c;
                        }
                    }
                    filtered = pwdFiltered;
                }
                if (!filtered.empty()) {
                    insertText(filtered);
                }
                return true;
        }
    }

    if (event->m_type == EventType::KeyDown) {
        if (!m_focused) return false;
        const auto& key = event->keyEvent;

        m_shiftPressed = isModSet(key.mod, KeyMod::Shift);
        m_ctrlPressed = isModSet(key.mod, KeyMod::Ctrl);

        if (m_ctrlPressed) {
            if (key.keycode == KeyCode::A) {
                selectAll();
                return true;
            } else if (key.keycode == KeyCode::C) {
                copy();
                return true;
            } else if (key.keycode == KeyCode::V) {
                paste();
                return true;
            } else if (key.keycode == KeyCode::X) {
                cut();
                return true;
            }
        } else if (key.keycode == KeyCode::Backspace) {
            if (hasSelection()) {
                deleteSelectedText();
            } else if (m_cursorPosition > 0 && !m_text.empty()) {
                int charPos = m_cursorPosition - 1;
                while (charPos > 0 && (m_text[charPos] & 0xC0) == 0x80) {
                    charPos--;
                }
                int charLen = m_cursorPosition - charPos;
                if (charLen > 0) {
                    m_text.erase(charPos, charLen);
                    m_cursorPosition = charPos;
                    updateTextOffset();
                    if (m_onTextChanged) {
                        m_onTextChanged(getThis(), m_text);
                    }
                    fireCCallback(PropertyNames::kEventTextChanged, CCallbackData::String, m_text.c_str());
                }
            }
            return true;
        } else if (key.keycode == KeyCode::Del) {
            if (hasSelection()) {
                deleteSelectedText();
            } else if (m_cursorPosition < (int)m_text.length()) {
                int charStart = m_cursorPosition;
                while (charStart < (int)m_text.length() && (m_text[charStart] & 0xC0) == 0x80) {
                    charStart++;
                }
                if (charStart < (int)m_text.length()) {
                    int charLen = getUtf8CharLength((unsigned char)m_text[charStart]);
                    m_text.erase(m_cursorPosition, charLen);
                    if (m_onTextChanged) {
                        m_onTextChanged(getThis(), m_text);
                    }
                    fireCCallback(PropertyNames::kEventTextChanged, CCallbackData::String, m_text.c_str());
                }
            }
            return true;
        } else if (key.keycode == KeyCode::Left) {
            int newPos = m_cursorPosition;
            while (newPos > 0) {
                newPos--;
                if ((m_text[newPos] & 0xC0) != 0x80) break;
            }
            newPos = std::max(0, newPos);

            if (isModSet(key.mod, KeyMod::Shift)) {
                if (!hasSelection()) {
                    m_selectionStart = m_cursorPosition;
                }
                m_cursorPosition = newPos;
                m_selectionEnd = m_cursorPosition;
            } else {
                m_cursorPosition = newPos;
                clearSelection();
            }
            m_cursorVisible = true;
            m_cursorBlinkTime = 0;
            updateTextOffset();
            return true;
        } else if (key.keycode == KeyCode::Right) {
            int newPos = m_cursorPosition;
            while (newPos < (int)m_text.length()) {
                int charLen = getUtf8CharLength((unsigned char)m_text[newPos]);
                newPos += charLen;
                break;
            }
            newPos = std::min((int)m_text.length(), newPos);

            if (isModSet(key.mod, KeyMod::Shift)) {
                if (!hasSelection()) {
                    m_selectionStart = m_cursorPosition;
                }
                m_cursorPosition = newPos;
                m_selectionEnd = m_cursorPosition;
            } else {
                m_cursorPosition = newPos;
                clearSelection();
            }
            m_cursorVisible = true;
            m_cursorBlinkTime = 0;
            updateTextOffset();
            return true;
        } else if (key.keycode == KeyCode::Home) {
            m_cursorPosition = 0;
            clearSelection();
            m_cursorVisible = true;
            m_cursorBlinkTime = 0;
            updateTextOffset();
            return true;
        } else if (key.keycode == KeyCode::End) {
            int maxPos = (int)m_text.length();
            m_cursorPosition = maxPos;
            clearSelection();
            m_cursorVisible = true;
            m_cursorBlinkTime = 0;
            updateTextOffset();
            return true;
        } else if (key.keycode == KeyCode::Return || key.keycode == KeyCode::KPEnter) {
            if (m_onEnter) {
                m_onEnter(getThis());
            }
            fireCCallback(PropertyNames::kEventEnter, CCallbackData::None, nullptr);
            return true;
        }
    }

    if (event->m_type == EventType::KeyUp) {
        if (!m_focused) return false;
        const auto& key = event->keyEvent;

        if (key.keycode == KeyCode::LShift || key.keycode == KeyCode::RShift) {
            m_shiftPressed = false;
        } else if (key.keycode == KeyCode::LCtrl || key.keycode == KeyCode::RCtrl) {
            m_ctrlPressed = false;
        }
    }

    return false;
}

void EditBox::setRect(SRect rect) {
    ControlImpl::setRect(rect);
    updateTextOffset();
}

// 父链缩放变更时重建字体：字号随复合（loadFontInternal 用 getScaleXX），
// 避免 resize 后字号滞后（原仅在 create/setText 等时机加载）。
// 不可见控件延后到可见帧（update）再重建（TextArea 继承本路径，行高懒检测同步生效）。
void EditBox::refreshScaleWith(float parentXX, float parentYY){
    float oldScaleX = getScaleXX();
    float oldScaleY = getScaleYY();
    ControlImpl::refreshScaleWith(parentXX, parentYY);
    if (oldScaleX != getScaleXX() || oldScaleY != getScaleYY()) {
        if (m_visible) {
            loadFontInternal();
            updateTextOffset();
        } else {
            m_fontScaleDirty = true;
        }
    }
}

void EditBox::onMouseEnter(float x, float y) {
    m_mouseInside = true;
    ControlImpl::onMouseEnter(x, y);   // P0-30：EditBox 族（含 NUD/ComboBox/TextArea）获得 hover 态
}

void EditBox::onMouseLeave(float x, float y) {
    m_mouseInside = false;
    ControlImpl::onMouseLeave(x, y);
}

void EditBox::setText(const std::string& text) {
    m_text = text;
    m_cursorPosition = (int)m_text.length();
    clearSelection();
    loadFontInternal();
    updateTextOffset();
}

std::string EditBox::getText() const {
    return m_text;
}

void EditBox::setPlaceholder(const std::string& placeholder) {
    m_placeholderText = placeholder;
}

std::string EditBox::getPlaceholder() const {
    return m_placeholderText;
}

void EditBox::setPasswordMode(bool enable) {
    m_passwordMode = enable;
    updateTextOffset();
}

void EditBox::setPasswordChar(char c) {
    m_passwordChar = c;
}

void EditBox::selectAll() {
    m_selectionStart = 0;
    m_selectionEnd = (int)m_text.length();
    m_cursorPosition = m_selectionEnd;
    updateTextOffset();
}

void EditBox::setSelection(int start, int endVal) {
    m_selectionStart = std::max(0, start);
    m_selectionEnd = std::min((int)m_text.length(), endVal);
    m_cursorPosition = m_selectionEnd;
    updateTextOffset();
}

void EditBox::clearSelection() {
    m_selectionStart = m_cursorPosition;
    m_selectionEnd = m_cursorPosition;
}

std::string EditBox::getSelectedText() const {
    if (!hasSelection()) return "";

    int start = std::min(m_selectionStart, m_selectionEnd);
    int endVal = std::max(m_selectionStart, m_selectionEnd);

    return getUtf8Substr(m_text, start, endVal - start);
}

void EditBox::copy() {
    if (m_passwordMode) return;

    std::string selectedText = getSelectedText();
    if (!selectedText.empty()) {
        InputBackend* ib = getInputBackend();
        if (ib) ib->setClipboardText(selectedText);
    }
}

void EditBox::cut() {
    if (!hasSelection() || m_passwordMode) return;

    copy();
    deleteSelectedText();
}

void EditBox::paste() {
    InputBackend* ib = getInputBackend();
    if (!ib) return;
    std::string text = ib->getClipboardText();
    if (!text.empty()) {
        insertText(text);
    }
}

void EditBox::setFont(FontName fontName) {
    m_fontName = fontName;
    loadFontInternal();
    updateTextOffset();
}

void EditBox::setFontSize(int size) {
    m_fontSize = size;
    loadFontInternal();
    updateTextOffset();
}

void EditBox::setAlignmentMode(AlignmentMode mode) {
    if (m_AlignmentMode == mode) return;
    m_AlignmentMode = mode;
    updateTextOffset();
}

void EditBox::setOnTextChanged(OnTextChangedHandler handler) {
    m_onTextChanged = handler;
}

void EditBox::setOnEnter(OnEnterHandler handler) {
    m_onEnter = handler;
}

void EditBox::setFocused(bool focused, bool byKeyboard) {
    if (m_focused == focused) return;

    if (focused) {
        if (!m_focusWatcherRegistered) {
            m_context->eventQueue->addBeforeEventHandlingWatcher(EventType::Custom, getThis());
            m_focusWatcherRegistered = true;
        }

        auto event = make_shared<Event>(EventType::Custom);
        event->customInt = static_cast<int>(EventName::ON_FOCUS);
        event->customPtr = this;
        m_context->eventQueue->pushEventIntoQueue(event);
    }

    ControlImpl::setFocused(focused, byKeyboard);
}

void EditBox::onFocusGained(bool byKeyboard) {
    m_cursorVisible = true;
    m_cursorBlinkTime = 0;
    updateTextOffset();
}

void EditBox::onFocusLost() {
    m_cursorVisible = false;
    clearSelection();
    updateTextOffset();
}

void EditBox::setMargin(const Margin& margin) {
    m_margin = margin;
    updateTextOffset();
}

bool EditBox::beforeEventHandlingWatcher(shared_ptr<Event> event) {
    if (event->m_type == EventType::Custom && event->customInt == static_cast<int>(EventName::ON_FOCUS) && m_focused) {
        if (event->customPtr != this) {
            setFocused(false);
        }
    }
    return false;
}

// ── Property system overrides ──
int EditBox::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kPasswordMode) == 0) { setPasswordMode(value != 0); return 1; }
    return ControlImpl::setBoolProperty(prop, value);
}
int EditBox::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) { setFontSize(value); return 1; }
    if (strcmp(prop, PropertyNames::kPasswordChar) == 0) { setPasswordChar(static_cast<char>(value)); return 1; }
    return ControlImpl::setIntProperty(prop, value);
}
int EditBox::setStringProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kTextContent) == 0)  { setText(value);      return 1; }
    if (strcmp(prop, PropertyNames::kPlaceholder) == 0)  { setPlaceholder(value); return 1; }
    return ControlImpl::setStringProperty(prop, value);
}
int EditBox::setEnumProperty(const char* prop, const char* value) {
    if (strcmp(prop, PropertyNames::kAlign) == 0) {
        if (_stricmp(value, PropertyNames::kAlignLowerTopLeft) == 0)      { setAlignmentMode(AlignmentMode::AM_TOP_LEFT);      return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerMidLeft) == 0)      { setAlignmentMode(AlignmentMode::AM_MID_LEFT);      return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerBottomLeft) == 0)   { setAlignmentMode(AlignmentMode::AM_BOTTOM_LEFT);   return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerTopRight) == 0)     { setAlignmentMode(AlignmentMode::AM_TOP_RIGHT);     return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerMidRight) == 0)     { setAlignmentMode(AlignmentMode::AM_MID_RIGHT);     return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerBottomRight) == 0)  { setAlignmentMode(AlignmentMode::AM_BOTTOM_RIGHT);  return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerTopCenter) == 0)    { setAlignmentMode(AlignmentMode::AM_TOP_CENTER);    return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerCenter) == 0)        { setAlignmentMode(AlignmentMode::AM_CENTER);        return 1; }
        if (_stricmp(value, PropertyNames::kAlignLowerBottomCenter) == 0) { setAlignmentMode(AlignmentMode::AM_BOTTOM_CENTER); return 1; }
        return 0;
    }
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        setFont(FontNameFromString(value));
        return 1;
    }
    return ControlImpl::setEnumProperty(prop, value);
}
int EditBox::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kPasswordMode) == 0) { out = m_passwordMode ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}
int EditBox::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kFontSize) == 0) { out = m_fontSize; return 1; }
    if (strcmp(prop, PropertyNames::kPasswordChar) == 0) { out = static_cast<int>(m_passwordChar); return 1; }
    return ControlImpl::getIntProperty(prop, out);
}
int EditBox::getStringProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kTextContent) == 0)  { out = m_text.c_str();         return 1; }
    if (strcmp(prop, PropertyNames::kPlaceholder) == 0)  { out = m_placeholderText.c_str(); return 1; }
    return ControlImpl::getStringProperty(prop, out);
}
int EditBox::getEnumProperty(const char* prop, const char*& out) {
    if (strcmp(prop, PropertyNames::kAlign) == 0) {
        switch (m_AlignmentMode) {
            case AlignmentMode::AM_TOP_LEFT:      out = PropertyNames::kAlignLowerTopLeft;      break;
            case AlignmentMode::AM_MID_LEFT:      out = PropertyNames::kAlignLowerMidLeft;      break;
            case AlignmentMode::AM_BOTTOM_LEFT:   out = PropertyNames::kAlignLowerBottomLeft;   break;
            case AlignmentMode::AM_TOP_RIGHT:     out = PropertyNames::kAlignLowerTopRight;     break;
            case AlignmentMode::AM_MID_RIGHT:     out = PropertyNames::kAlignLowerMidRight;     break;
            case AlignmentMode::AM_BOTTOM_RIGHT:  out = PropertyNames::kAlignLowerBottomRight;  break;
            case AlignmentMode::AM_TOP_CENTER:    out = PropertyNames::kAlignLowerTopCenter;    break;
            case AlignmentMode::AM_CENTER:        out = PropertyNames::kAlignLowerCenter;        break;
            case AlignmentMode::AM_BOTTOM_CENTER: out = PropertyNames::kAlignLowerBottomCenter; break;
            default: out = PropertyNames::kAlignLowerMidLeft; break;
        }
        return 1;
    }
    if (strcmp(prop, PropertyNames::kFont) == 0) {
        out = FontNameToString(m_fontName);
        return 1;
    }
    return ControlImpl::getEnumProperty(prop, out);
}
int EditBox::setCallbackProperty(const char* event, void (*cb)(void*, const void*, void*), void* userData) {
    if (strcmp(event, PropertyNames::kEventTextChanged) == 0 ||
        strcmp(event, PropertyNames::kEventEnter) == 0) {
        return ControlImpl::setCallbackProperty(event, cb, userData);
    }
    return ControlImpl::setCallbackProperty(event, cb, userData);
}

EditBoxBuilder::EditBoxBuilder(Control *parent, SRect rect, float xScale, float yScale)
    : m_editBox(make_shared<EditBox>(parent, rect, xScale, yScale))
{
}

EditBoxBuilder& EditBoxBuilder::setBackgroundStateColor(StateColor stateColor) {
    m_editBox->setBackgroundStateColor(stateColor);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setBorderStateColor(StateColor stateColor) {
    m_editBox->setBorderStateColor(stateColor);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setTextStateColor(StateColor stateColor) {
    m_editBox->setTextStateColor(stateColor);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setText(const std::string& text) {
    m_editBox->setText(text);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setPlaceholder(const std::string& placeholder) {
    m_editBox->setPlaceholder(placeholder);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setPasswordMode(bool enable) {
    m_editBox->setPasswordMode(enable);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setPasswordChar(char c) {
    m_editBox->setPasswordChar(c);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setFont(FontName fontName) {
    m_editBox->setFont(fontName);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setFontSize(int size) {
    m_editBox->setFontSize(size);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setAlignmentMode(AlignmentMode mode) {
    m_editBox->setAlignmentMode(mode);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setOnTextChanged(EditBox::OnTextChangedHandler handler) {
    m_editBox->setOnTextChanged(handler);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setOnEnter(EditBox::OnEnterHandler handler) {
    m_editBox->setOnEnter(handler);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setId(int id) {
    m_editBox->setId(id);
    return *this;
}

EditBoxBuilder& EditBoxBuilder::setTransparent(bool isTransparent) {
    m_editBox->setTransparent(isTransparent);
    return *this;
}

shared_ptr<EditBox> EditBoxBuilder::build(void) {
    m_editBox->create();
    return m_editBox;
}
