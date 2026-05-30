// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#define NOMINMAX
#include <iostream>
#include "EditBox.h"
#include "MainWindow.h"
#include "ResourceLoader.h"
#include "EventQueue.h"
#include <algorithm>
#include <cstdint>

static void EnsureTextInputStarted() {
    static bool started = false;
    if (!started) {
        SDL_StartTextInput(MainWindow::getInstance()->getWindow());
        started = true;
    }
}

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
    , m_textEngine(nullptr)
    , m_textObj(nullptr)
    , m_placeholderTextObj(nullptr)
    , m_fontSize(16)
    , m_fontName(FontName::HarmonyOS_Sans_SC_Regular)
    , m_focusWatcherRegistered(false)
    , m_AlignmentMode(AlignmentMode::AM_MID_LEFT)
{
    EnsureTextInputStarted();
    m_id = 0;
    m_visible = true;
    m_enable = true;
    m_isBorderVisible = true;
    m_isTransparent = false;
    m_state = ControlState::Normal;

    setRect(rect);

    m_margin = Margin(8.0f, 4.0f, 8.0f, 4.0f);

    fs::path fontPath = ConstDef::pathPrefix / ResourceLoader::m_fontFiles[m_fontName];
    int scaledFontSize = (int)(m_fontSize * getScaleXX());
    m_font = TTF_OpenFont(fontPath.string().c_str(), scaledFontSize);
    if (!m_font) {
        SDL_Log("Failed to load font for EditBox: %s", SDL_GetError());
    } else {
        createTextEngine();
        createTextObjects();
    }
    updateTextOffset();
}

EditBox::~EditBox() {
    destroyTextObjects();
    if (m_textEngine) {
        TTF_DestroyRendererTextEngine(m_textEngine);
        m_textEngine = nullptr;
    }
    if (m_font) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
}

void EditBox::createTextEngine() {
    SDL_Log("EditBox::createTextEngine called, renderer=%p", (void*)getRenderer());
    if (m_textEngine) {
        TTF_DestroyRendererTextEngine(m_textEngine);
    }
    m_textEngine = TTF_CreateRendererTextEngine(getRenderer());
    if (!m_textEngine) {
        SDL_Log("EditBox::createTextEngine FAILED: %s", SDL_GetError());
    } else {
        SDL_Log("EditBox::createTextEngine succeeded, engine=%p", (void*)m_textEngine);
    }
}

void EditBox::createTextObjects() {
    if (!m_textEngine || !m_font) {
        return;
    }

    destroyTextObjects();

    std::string displayText = getDisplayText();
    if (!displayText.empty()) {
        m_textObj = TTF_CreateText(m_textEngine, m_font, displayText.c_str(), (Uint32)displayText.length());
    }

    if (!m_placeholderText.empty()) {
        m_placeholderTextObj = TTF_CreateText(m_textEngine, m_font, m_placeholderText.c_str(), (Uint32)m_placeholderText.length());
    }
}

void EditBox::destroyTextObjects() {
    if (m_textObj) {
        TTF_DestroyText(m_textObj);
        m_textObj = nullptr;
    }
    if (m_placeholderTextObj) {
        TTF_DestroyText(m_placeholderTextObj);
        m_placeholderTextObj = nullptr;
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

float EditBox::getTextWidth(const std::string& text) const {
    if (!m_textEngine || !m_font || text.empty()) return 0;

    TTF_Text *ttfText = TTF_CreateText(m_textEngine, m_font, text.c_str(), (Uint32)text.length());
    if (!ttfText) return 0;

    int width = 0;
    int height = 0;
    TTF_GetTextSize(ttfText, &width, &height);
    TTF_DestroyText(ttfText);

    return (float)width;
}

int EditBox::getCursorFromPosition(float x) const {
    std::string displayText = getDisplayText();
    float textX = m_textOffsetX;

    int bestOffset = 0;
    float bestDist = 99999.0f;

    for (int i = 0; i <= (int)displayText.length(); ) {
        std::string prefix = getUtf8Substr(displayText, 0, i);
        float charX = textX + getTextWidth(prefix);
        float dist = SDL_abs(x - charX);

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

float EditBox::getCursorX(int cursorPos) const {
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
    createTextObjects();
    updateTextOffset();

    if (m_onTextChanged) {
        m_onTextChanged(m_text);
    }
}

void EditBox::deleteSelectedText() {
    if (m_selectionStart == m_selectionEnd) return;

    int start = std::min(m_selectionStart, m_selectionEnd);
    int endVal = std::max(m_selectionStart, m_selectionEnd);

    m_text.erase(start, endVal - start);
    m_cursorPosition = start;
    clearSelection();
    createTextObjects();
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

    SDL_Renderer *renderer = getRenderer();
    if (!renderer) return;

    SRect drawRect = getDrawRect();
    float scaledFontSize = m_fontSize * getScaleXX();

    // drawBackground(&drawRect);
    // drawBorder(&drawRect);

    float scale = getScaleXX();
    float marginX = m_margin.left * scale;
    float marginY = m_margin.top * scale;
    float marginRight = m_margin.right * scale;
    float marginBottom = m_margin.bottom * scale;
    SDL_Rect clipRect = {(int)(drawRect.left + marginX), (int)(drawRect.top + marginY),
                         (int)(drawRect.width - marginX - marginRight), (int)(drawRect.height - marginY - marginBottom)};
    SDL_SetRenderClipRect(renderer, &clipRect);

    if (hasSelection() && m_focused) {
        int selStart = std::min(m_selectionStart, m_selectionEnd);
        int selEnd = std::max(m_selectionStart, m_selectionEnd);

        std::string displayText = getDisplayText();
        std::string prefixForStart = getUtf8Substr(displayText, 0, selStart);
        std::string prefixForEnd = getUtf8Substr(displayText, 0, selEnd);

        float startX = m_textOffsetX + getTextWidth(prefixForStart);
        float endX = m_textOffsetX + getTextWidth(prefixForEnd);

        SDL_FRect selRect = {
            drawRect.left + startX,
            drawRect.top + m_textOffsetY,
            endX - startX,
            scaledFontSize
        };

        SDL_SetRenderDrawColor(renderer, 100, 149, 237, 128);
        SDL_RenderFillRect(renderer, &selRect);
    }

    if (!m_text.empty() && m_textObj) {
        SDL_Color textColor = m_textColor.getNormal();
        TTF_SetTextColor(m_textObj, textColor.r, textColor.g, textColor.b, textColor.a);

        SDL_FPoint position = {drawRect.left + m_textOffsetX, drawRect.top + m_textOffsetY};
        TTF_DrawRendererText(m_textObj, position.x, position.y);
    } else if (!m_placeholderText.empty() && m_placeholderTextObj) {
        SDL_Color placeholderColor = {128, 128, 128, 255};
        TTF_SetTextColor(m_placeholderTextObj, placeholderColor.r, placeholderColor.g, placeholderColor.b, placeholderColor.a);

        SDL_FPoint position = {drawRect.left + m_textOffsetX, drawRect.top + m_textOffsetY};
        TTF_DrawRendererText(m_placeholderTextObj, position.x, position.y);
    }

    SDL_SetRenderClipRect(renderer, nullptr);

    if (m_focused && m_cursorVisible && m_selectionStart == m_selectionEnd) {
        float cursorX = getCursorX(m_cursorPosition);

        SDL_FRect cursorRect = {
            drawRect.left + cursorX,
            drawRect.top + m_textOffsetY,
            2.0f,
            scaledFontSize
        };

        SDL_Color cursorColor = m_textColor.getNormal();
        SDL_SetRenderDrawColor(renderer, cursorColor.r, cursorColor.g, cursorColor.b, cursorColor.a);
        SDL_RenderFillRect(renderer, &cursorRect);
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
                        createTextObjects();
                        updateTextOffset();
                        if (m_onTextChanged) {
                            m_onTextChanged(m_text);
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
                        createTextObjects();
                        if (m_onTextChanged) {
                            m_onTextChanged(m_text);
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
                    m_onEnter();
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

void EditBox::setRenderer(SDL_Renderer *renderer) {
    if (m_renderer == renderer) return;

    ControlImpl::setRenderer(renderer);

    if (renderer && m_font) {
        recreateTextObjects();
    }
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
    createTextObjects();
    updateTextOffset();
}

std::string EditBox::getText() const {
    return m_text;
}

void EditBox::setPlaceholder(const std::string& placeholder) {
    m_placeholderText = placeholder;
    createTextObjects();
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

void EditBox::copy() const {
    if (m_passwordMode) return;

    std::string selectedText = getSelectedText();
    if (!selectedText.empty()) {
        SDL_SetClipboardText(selectedText.c_str());
    }
}

void EditBox::cut() {
    if (!hasSelection() || m_passwordMode) return;

    copy();
    deleteSelectedText();
}

void EditBox::paste() {
    char *clipboardText = SDL_GetClipboardText();
    if (clipboardText && strlen(clipboardText) > 0) {
        insertText(std::string(clipboardText));
    }
    SDL_free(clipboardText);
}

void EditBox::setFont(FontName fontName) {
    m_fontName = fontName;
    if (m_font) {
        TTF_CloseFont(m_font);
    }
    fs::path fontPath = ConstDef::pathPrefix / ResourceLoader::m_fontFiles[m_fontName];
    int scaledFontSize = (int)(m_fontSize * getScaleXX());
    m_font = TTF_OpenFont(fontPath.string().c_str(), scaledFontSize);
    createTextObjects();
    updateTextOffset();
}

void EditBox::setFontSize(int size) {
    m_fontSize = size;
    if (m_font) {
        TTF_CloseFont(m_font);
    }
    fs::path fontPath = ConstDef::pathPrefix / ResourceLoader::m_fontFiles[m_fontName];
    int scaledFontSize = (int)(m_fontSize * getScaleXX());
    m_font = TTF_OpenFont(fontPath.string().c_str(), scaledFontSize);
    createTextObjects();
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

void EditBox::recreateTextObjects() {
    SDL_Log("EditBox::recreateTextObjects called");
    createTextEngine();
    createTextObjects();
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
