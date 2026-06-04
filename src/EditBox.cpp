// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#define NOMINMAX
#include <iostream>
#include "EditBox.h"
#include "MainWindow.h"
#include "ResourceLoader.h"
#include "EventQueue.h"
#include <algorithm>
#include <cstdint>

EditBox::EditBox(Control *parent, SRect rect, float xScale, float yScale)
    : ControlImpl(parent, xScale, yScale)
    , m_cursorPosition(0)
    , m_selectionStart(0)
    , m_selectionEnd(0)
    , m_passwordMode(false)
    , m_passwordChar('*')
    , m_focused(false)
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
{
    m_id = 0;
    m_visible = true;
    m_enable = true;
    m_isBorderVisible = true;
    m_isTransparent = false;
    m_state = ControlState::Normal;

    setRect(rect);

    m_margin = Margin(8.0f, 4.0f, 8.0f, 4.0f);

    loadFontInternal();
    updateTextOffset();

    InputBackend* ib = getInputBackend();
    if (ib) ib->startTextInput();
}

EditBox::~EditBox() {
}

void EditBox::loadFontInternal() {
    ResourceProvider* provider = getResourceProvider();
    if (provider == nullptr) {
        printf("EditBox::loadFontInternal: No resource provider\n");
        return;
    }

    m_fontData = provider->readFile(ResourceLoader::m_fontFiles[m_fontName]);
    if (m_fontData == nullptr || m_fontData->empty()) {
        printf("EditBox::loadFontInternal: Failed to load font\n");
        return;
    }

    int scaledFontSize = (int)(m_fontSize * getScaleXX());
    m_font = getTextRenderer()->loadFontFromMemory(m_fontData->data(), m_fontData->size(), scaledFontSize);
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
    float scale = getScaleXX();
    float scaledFontSize = m_fontSize * scale;
    float textHeight = scaledFontSize;
    m_textOffsetY = (drawRect.height - textHeight) / 2.0f;

    float textWidth = getTextWidth(getDisplayText());
    float visibleWidth = drawRect.width - (m_margin.left + m_margin.right) * scale;
    float margin = m_margin.left * scale;

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
    updateTextOffset();

    if (m_onTextChanged) {
        m_onTextChanged(getThis(), m_text);
    }
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
    if (m_focused) {
        m_cursorBlinkTime += 16;
        if (m_cursorBlinkTime >= 2000) {
            m_cursorVisible = !m_cursorVisible;
            m_cursorBlinkTime = 0;
        }
    }
}

void EditBox::draw(void) {
    if (!m_visible) return;

    ControlImpl::preDraw();

    SRect drawRect = getDrawRect();
    float scaledFontSize = m_fontSize * getScaleXX();

    float scale = getScaleXX();
    float marginX = m_margin.left * scale;
    float marginY = m_margin.top * scale;
    float marginRight = m_margin.right * scale;
    float marginBottom = m_margin.bottom * scale;
    SRect clipRect(drawRect.left + marginX, drawRect.top + marginY,
                   drawRect.width - marginX - marginRight, drawRect.height - marginY - marginBottom);
    GET_RENDERDEVICE->setClipRect(clipRect);

    if (hasSelection() && m_focused) {
        int selStart = std::min(m_selectionStart, m_selectionEnd);
        int selEnd = std::max(m_selectionStart, m_selectionEnd);

        std::string displayText = getDisplayText();
        std::string prefixForStart = getUtf8Substr(displayText, 0, selStart);
        std::string prefixForEnd = getUtf8Substr(displayText, 0, selEnd);

        float startX = m_textOffsetX + getTextWidth(prefixForStart);
        float endX = m_textOffsetX + getTextWidth(prefixForEnd);

        SRect selRect(drawRect.left + startX, drawRect.top + m_textOffsetY,
                      endX - startX, scaledFontSize);
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

    GET_RENDERDEVICE->clearClipRect();

    if (m_focused && m_cursorVisible && m_selectionStart == m_selectionEnd) {
        float cursorX = getCursorX(m_cursorPosition);

        SRect cursorRect(drawRect.left + cursorX, drawRect.top + m_textOffsetY,
                         2.0f, scaledFontSize);

        GET_RENDERDEVICE->setDrawColor(m_textColor.getNormal());
        GET_RENDERDEVICE->fillRect(cursorRect);
    }
}

bool EditBox::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;

    if (event->m_eventName == EventName::MOUSE_LBUTTON_DOWN) {
        if (!event->m_eventParam.has_value()) return false;
        try {
            auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;
            if (isContainsPoint(pos->x, pos->y)) {
                setFocused(true);

                int newCursor = getCursorFromPosition(pos->x - getDrawRect().left);

                SDL_Keymod mod = SDL_GetModState();
                bool shiftPressed = (mod & SDL_KMOD_SHIFT) != 0;

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

                return true;
            } else {
                setFocused(false);
            }
        } catch (...) {
            return false;
        }
    }

    if (event->m_eventName == EventName::MOUSE_MOVING) {
        if (m_focused && m_isDragging) {
            if (!event->m_eventParam.has_value()) return false;
            try {
                auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
                if (!pos) return false;
                int newCursor = getCursorFromPosition(pos->x - getDrawRect().left);

                int start = std::min(m_dragStartPosition, newCursor);
                int end = std::max(m_dragStartPosition, newCursor);
                m_selectionStart = start;
                m_selectionEnd = end;
                m_cursorPosition = newCursor;

                updateTextOffset();
                return true;
            } catch (...) {
                return false;
            }
        }
    }

    if (event->m_eventName == EventName::MOUSE_LBUTTON_UP) {
        m_isDragging = false;
    }

    if (event->m_eventName == EventName::TEXT_INPUT) {
        if (m_focused) {
            if (!event->m_eventParam.has_value()) return false;
            try {
                auto data = std::any_cast<TextInputEventData>(event->m_eventParam);
                std::string filtered;
                for (char c : data.text) {
                    if (c == '\n' || c == '\r') continue;
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
            } catch (...) {
                return false;
            }
        }
    }

    if (event->m_eventName == EventName::KEY_DOWN) {
        if (!m_focused) return false;
        if (!event->m_eventParam.has_value()) return false;
        try {
            auto keyData = std::any_cast<KeyEventData>(event->m_eventParam);

            m_shiftPressed = (keyData.mod & SDL_KMOD_SHIFT) != 0;
            m_ctrlPressed = (keyData.mod & SDL_KMOD_CTRL) != 0;

            if (m_ctrlPressed) {
                if (keyData.keycode == SDLK_A) {
                    selectAll();
                    return true;
                } else if (keyData.keycode == SDLK_C) {
                    copy();
                    return true;
                } else if (keyData.keycode == SDLK_V) {
                    paste();
                    return true;
                } else if (keyData.keycode == SDLK_X) {
                    cut();
                    return true;
                }
            } else if (keyData.keycode == SDLK_BACKSPACE) {
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
                    }
                }
                return true;
            } else if (keyData.keycode == SDLK_DELETE) {
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
                    }
                }
                return true;
            } else if (keyData.keycode == SDLK_LEFT) {
                int newPos = m_cursorPosition;
                while (newPos > 0) {
                    newPos--;
                    if ((m_text[newPos] & 0xC0) != 0x80) break;
                }
                newPos = std::max(0, newPos);

                if ((keyData.mod & SDL_KMOD_SHIFT) != 0) {
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
            } else if (keyData.keycode == SDLK_RIGHT) {
                int newPos = m_cursorPosition;
                while (newPos < (int)m_text.length()) {
                    int charLen = getUtf8CharLength((unsigned char)m_text[newPos]);
                    newPos += charLen;
                    break;
                }
                newPos = std::min((int)m_text.length(), newPos);

                if ((keyData.mod & SDL_KMOD_SHIFT) != 0) {
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
            } else if (keyData.keycode == SDLK_HOME) {
                m_cursorPosition = 0;
                clearSelection();
                m_cursorVisible = true;
                m_cursorBlinkTime = 0;
                updateTextOffset();
                return true;
            } else if (keyData.keycode == SDLK_END) {
                int maxPos = (int)m_text.length();
                m_cursorPosition = maxPos;
                clearSelection();
                m_cursorVisible = true;
                m_cursorBlinkTime = 0;
                updateTextOffset();
                return true;
            } else if (keyData.keycode == SDLK_RETURN || keyData.keycode == SDLK_RETURN2) {
                if (m_onEnter) {
                    m_onEnter(getThis());
                }
                return true;
            }
        } catch (...) {
            return false;
        }
    }

    if (event->m_eventName == EventName::KEY_UP) {
        if (!m_focused) return false;
        if (!event->m_eventParam.has_value()) return false;
        try {
            auto keyData = std::any_cast<KeyEventData>(event->m_eventParam);

            if (keyData.keycode == SDLK_LSHIFT || keyData.keycode == SDLK_RSHIFT) {
                m_shiftPressed = false;
            } else if (keyData.keycode == SDLK_LCTRL || keyData.keycode == SDLK_RCTRL) {
                m_ctrlPressed = false;
            }
        } catch (...) {
            return false;
        }
    }

    return false;
}

void EditBox::setRect(SRect rect) {
    ControlImpl::setRect(rect);
    updateTextOffset();
}

void EditBox::onMouseEnter(float x, float y) {
    m_mouseInside = true;
}

void EditBox::onMouseLeave(float x, float y) {
    m_mouseInside = false;
}

void EditBox::setText(const std::string& text) {
    m_text = text;
    m_cursorPosition = (int)m_text.length();
    clearSelection();
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

void EditBox::setFocused(bool focused) {
    if (m_focused == focused) return;

    if (focused) {
        if (!m_focusWatcherRegistered) {
            EventQueue::getInstance()->addBeforeEventHandlingWatcher(EventName::ON_FOCUS, getThis());
            m_focusWatcherRegistered = true;
        }

        FocusEventData focusData;
        focusData.controlPtr = this;
        focusData.focused = true;

        shared_ptr<Event> event = make_shared<Event>(EventName::ON_FOCUS, focusData);
        EventQueue::getInstance()->pushEventIntoQueue(event);

        m_focused = true;
        m_cursorVisible = true;
        m_cursorBlinkTime = 0;
    } else {
        m_focused = false;
        m_cursorVisible = false;
        clearSelection();
    }

    updateTextOffset();
}

void EditBox::setMargin(const Margin& margin) {
    m_margin = margin;
    updateTextOffset();
}

bool EditBox::beforeEventHandlingWatcher(shared_ptr<Event> event) {
    if (event->m_eventName == EventName::ON_FOCUS && m_focused) {
        try {
            FocusEventData focusData = std::any_cast<FocusEventData>(event->m_eventParam);
            if (focusData.controlPtr != this) {
                setFocused(false);
            }
        } catch (const std::bad_any_cast& e) {
        }
    }
    return false;
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
