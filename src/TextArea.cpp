// 由AI(MinMax V2.5)生成，可能不完整或有错误，请自行检查和修改
#define NOMINMAX
#include "TextArea.h"
#include "MainWindow.h"
#include <algorithm>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>

TextArea::TextArea(Control *parent, SRect rect, float xScale, float yScale)
    : EditBox(parent, rect, xScale, yScale)
    , m_scrollY(0)
    , m_scrollX(0)
    , m_wordWrap(false)
    , m_lineHeight(20)
    , m_vScrollBar(nullptr)
    , m_hScrollBar(nullptr)
    , m_autoScroll(true)
    , m_updatingScrollBar(false)
{
    m_lines.push_back("");

    SRect vScrollBarRect = SRect(
        rect.width - 16.0f,
        0.0f,
        16.0f,
        rect.height
    );

    m_vScrollBar = make_shared<ScrollBar>(this, vScrollBarRect, ScrollBarOrientation::Vertical, 1.0f, 1.0f);
    m_vScrollBar->setRange(0, 100);
    m_vScrollBar->setPageSize(rect.height);
    m_vScrollBar->setVisible(false);
    m_vScrollBar->setOnPositionChanged([this](shared_ptr<ScrollBar>, float, float newValue, float, float) {
        if (!m_updatingScrollBar) {
            setScrollY((int)newValue);
        }
    });

    addControl(m_vScrollBar);

    SRect hScrollBarRect = SRect(
        0.0f,
        rect.height - 16.0f,
        rect.width,
        16.0f
    );

    m_hScrollBar = make_shared<ScrollBar>(this, hScrollBarRect, ScrollBarOrientation::Horizontal, 1.0f, 1.0f);
    m_hScrollBar->setRange(0, 100);
    m_hScrollBar->setPageSize(rect.width);
    m_hScrollBar->setVisible(false);
    m_hScrollBar->setOnPositionChanged([this](shared_ptr<ScrollBar>, float, float newValue, float, float) {
        if (!m_updatingScrollBar) {
            setScrollX((int)newValue);
        }
    });

    addControl(m_hScrollBar);
}

float TextArea::getCharWidth(const std::string& text, int byteIndex) {
    if (byteIndex < 0 || byteIndex >= (int)text.length() || !getTextRenderer() || !getFont()) {
        return 0;
    }

    int charLen = getUtf8CharLength(text[byteIndex]);
    if (byteIndex + charLen > (int)text.length()) {
        charLen = text.length() - byteIndex;
    }

    SSize size = getTextRenderer()->measureText(getFont(), text.substr(byteIndex, charLen));
    return size.width;
}

int TextArea::getLinePixelWidth(const std::string& line) {
    if (line.empty() || !getTextRenderer() || !getFont()) {
        return 0;
    }

    SSize size = getTextRenderer()->measureText(getFont(), line);
    return (int)size.width;
}

int TextArea::getByteIndexFromPixelX(const std::string& text, float pixelX) {
    if (text.empty()) {
        return 0;
    }

    if (!getTextRenderer() || !getFont()) {
        return 0;
    }

    if (pixelX <= 0) {
        return 0;
    }

    int fullWidth = getLinePixelWidth(text);
    if (pixelX >= fullWidth) {
        return (int)text.length();
    }

    int byteIndex = 0;

    while (byteIndex < (int)text.length()) {
        int charLen = getUtf8CharLength((unsigned char)text[byteIndex]);
        if (byteIndex + charLen > (int)text.length()) {
            charLen = text.length() - byteIndex;
        }

        int prefixWidth = 0;
        if (byteIndex > 0) {
            SSize size = getTextRenderer()->measureText(getFont(), text.substr(0, byteIndex));
            prefixWidth = (int)size.width;
        }

        int nextPrefixWidth = 0;
        SSize nextSize = getTextRenderer()->measureText(getFont(), text.substr(0, byteIndex + charLen));
        nextPrefixWidth = (int)nextSize.width;

        float charCenterX = (prefixWidth + nextPrefixWidth) / 2.0f;

        if (pixelX < charCenterX) {
            return byteIndex;
        }

        byteIndex += charLen;
    }

    return (int)text.length();
}

void TextArea::rebuildLines() {
    m_lines.clear();
    m_lineStartPositions.clear();

    std::string text = getText();
    if (text.empty()) {
        m_lines.push_back("");
        m_lineStartPositions.push_back(0);
        return;
    }

    float availableWidth = getRect().width - 16.0f - 16.0f * getScaleXX();

    size_t start = 0;
    size_t newlinePos;
    int logicalLineStart = 0;

    while (start < text.length()) {
        newlinePos = text.find('\n', start);
        std::string line;
        int lineStartInText = (int)start;
        if (newlinePos == std::string::npos) {
            line = text.substr(start);
        } else {
            line = text.substr(start, newlinePos - start);
        }

        if (m_wordWrap && availableWidth > 0 && getTextRenderer() && getFont()) {
            std::string remaining = line;
            int wrapStartInLine = 0;
            while (!remaining.empty()) {
                m_lineStartPositions.push_back(lineStartInText + wrapStartInLine);

                int breakPos = (int)remaining.length();
                float lineWidth = 0;
                bool overflow = false;

                for (int i = 0; i <= (int)remaining.length(); ) {
                    int charLen = (i < (int)remaining.length()) ? getUtf8CharLength(remaining[i]) : 1;
                    std::string charStr = remaining.substr(i, charLen);

                    float charWidth = getCharWidth(remaining, i);

                    if (lineWidth + charWidth > availableWidth && i > 0) {
                        overflow = true;
                        breakPos = i;
                        for (int j = i - 1; j >= 0; --j) {
                            if (remaining[j] == ' ' || remaining[j] == '\t') {
                                breakPos = j;
                                break;
                            }
                        }
                        if (breakPos == i) {
                            breakPos = std::max(1, i - 1);
                        }
                        break;
                    }

                    lineWidth += charWidth;
                    i += charLen;
                }

                if (!overflow || breakPos == 0) {
                    m_lines.push_back(remaining);
                    break;
                } else {
                    m_lines.push_back(remaining.substr(0, breakPos));
                    wrapStartInLine += breakPos;
                    remaining = remaining.substr(breakPos);
                    while (!remaining.empty() && (remaining[0] == ' ' || remaining[0] == '\t')) {
                        remaining = remaining.substr(1);
                        wrapStartInLine++;
                    }
                }
            }
        } else {
            m_lines.push_back(line);
            m_lineStartPositions.push_back(lineStartInText);
        }

        if (newlinePos == std::string::npos) {
            break;
        }
        start = newlinePos + 1;
    }

    if (m_lines.empty()) {
        m_lines.push_back("");
        m_lineStartPositions.push_back(0);
    }

    std::string fullText = getText();
    if (!fullText.empty() && fullText.back() == '\n') {
        m_lines.push_back("");
        m_lineStartPositions.push_back((int)fullText.length() - 1);
    }
}

void TextArea::updateVScrollBar() {
    if (!m_vScrollBar) return;

    SRect rect = getRect();
    if (rect.height <= 0) return;

    float scale = getScaleXX();
    Margin margin = getMargin();
    float vThickness = m_vScrollBar->getThickness();
    float hThickness = m_hScrollBar ? m_hScrollBar->getThickness() : 16.0f;

    int totalHeight = getTotalLines() * m_lineHeight;
    int availableHeight = (int)rect.height - (int)(margin.top + margin.bottom);
    bool hScrollVisible = m_hScrollBar && m_hScrollBar->getVisible();
    if (hScrollVisible) {
        availableHeight -= (int)hThickness;
    }

    bool wasVisible = m_vScrollBar->getVisible();

    if (totalHeight > availableHeight) {
        int maxScroll = totalHeight - availableHeight;
        if (maxScroll < 0) maxScroll = 0;
        if (m_scrollY > maxScroll) {
            m_scrollY = maxScroll;
        }

        m_vScrollBar->setVisible(true);
        m_vScrollBar->setRange(0, maxScroll);
        m_vScrollBar->setPageSize(availableHeight);
        m_updatingScrollBar = true;
        m_vScrollBar->setValue((float)m_scrollY);
        m_updatingScrollBar = false;
    } else {
        m_vScrollBar->setVisible(false);
        m_scrollY = 0;
    }

    if (m_vScrollBar) {
        bool hScrollVisible = m_hScrollBar && m_hScrollBar->getVisible();
        float hThickness = m_hScrollBar ? m_hScrollBar->getThickness() : 16.0f;
        SRect scrollBarRect = SRect(
            rect.width - vThickness,
            0.0f,
            vThickness,
            rect.height - (hScrollVisible ? hThickness : 0.0f)
        );
        m_vScrollBar->setRect(scrollBarRect);
    }

    if (m_hScrollBar) {
        bool vScrollVisible = m_vScrollBar && m_vScrollBar->getVisible();
        float hThickness = m_hScrollBar->getThickness();
        SRect hScrollBarRect = SRect(
            0.0f,
            rect.height - hThickness,
            rect.width - (vScrollVisible ? vThickness : 0.0f),
            hThickness
        );
        m_hScrollBar->setRect(hScrollBarRect);
    }

    if (wasVisible != m_vScrollBar->getVisible() && m_hScrollBar) {
        updateHScrollBar();
    }
}

void TextArea::updateHScrollBar() {
    if (!m_hScrollBar) return;

    SRect rect = getRect();
    if (rect.width <= 0) return;

    SRect drawRect = getDrawRect();
    if (drawRect.width <= 0) return;

    int maxLineWidth = 0;
    for (const auto& line : m_lines) {
        int lineWidth = getLinePixelWidth(line);
        if (lineWidth > maxLineWidth) {
            maxLineWidth = lineWidth;
        }
    }

    float scale = getScaleXX();
    Margin margin = getMargin();
    int marginX = (int)(margin.left * scale);
    int marginRight = (int)(margin.right * scale);
    float vThickness = m_vScrollBar ? m_vScrollBar->getThickness() : 16.0f;
    float hThickness = m_hScrollBar ? m_hScrollBar->getThickness() : 16.0f;
    int availableWidth = (int)drawRect.width - marginX - marginRight;

    bool vScrollVisible = m_vScrollBar && m_vScrollBar->getVisible();
    if (vScrollVisible) {
        availableWidth -= (int)(vThickness * scale);
    }

    bool wasVisible = m_hScrollBar->getVisible();

    int cursorWidth = (int)(2.0f * scale);
    bool needScrollBar = !m_wordWrap && (maxLineWidth + cursorWidth > availableWidth);

    if (!needScrollBar && m_scrollX > 0) {
        needScrollBar = maxLineWidth + cursorWidth > availableWidth;
    }

    if (needScrollBar) {
        m_hScrollBar->setVisible(true);

        int range = maxLineWidth + cursorWidth - availableWidth;
        if (range < 0) range = 0;
        m_hScrollBar->setRange(0, range);
        m_hScrollBar->setPageSize(availableWidth);
        if (m_scrollX > range) {
            m_scrollX = range;
        }
        m_updatingScrollBar = true;
        m_hScrollBar->setValue((float)m_scrollX);
        m_updatingScrollBar = false;
    } else {
        m_hScrollBar->setVisible(false);
        m_scrollX = 0;
    }

    if (m_hScrollBar) {
        SRect rect = getRect();
        float vThickness = m_vScrollBar ? m_vScrollBar->getThickness() : 16.0f;
        float hThickness = m_hScrollBar->getThickness();
        bool vScrollVisible = m_vScrollBar && m_vScrollBar->getVisible();
        float hWidth = vScrollVisible ? rect.width - vThickness : rect.width;
        SRect hScrollBarRect = SRect(
            0.0f,
            rect.height - hThickness,
            hWidth,
            hThickness
        );
        m_hScrollBar->setRect(hScrollBarRect);
    }

    if (wasVisible != m_hScrollBar->getVisible() && m_vScrollBar) {
        updateVScrollBar();
    }
}

void TextArea::setScrollX(int x) {
    if (!m_hScrollBar) return;

    int maxScroll = (int)m_hScrollBar->getMaxValue();

    m_scrollX = std::max(0, std::min(x, maxScroll));

    if (!m_updatingScrollBar) {
        m_updatingScrollBar = true;
        m_hScrollBar->setValue((float)m_scrollX);
        m_updatingScrollBar = false;
    }
}

int TextArea::getVisibleLines() {
    SRect rect = getRect();
    if (rect.height <= 0 || m_lineHeight <= 0) return 0;
    int availableHeight = (int)rect.height;
    float scale = getScaleXX();
    float hThickness = m_hScrollBar ? m_hScrollBar->getThickness() : 16.0f;
    if (m_hScrollBar && m_hScrollBar->getVisible()) {
        availableHeight -= (int)(hThickness * scale);
    }
    return (int)(availableHeight / m_lineHeight);
}

void TextArea::update(void) {
    EditBox::update();

    if (m_text != m_lastTextForRebuild) {
        m_lastTextForRebuild = m_text;
        rebuildLines();
        updateVScrollBar();
        updateHScrollBar();
    }

    if (m_vScrollBar && m_vScrollBar->getVisible()) {
        m_vScrollBar->update();
    }

    if (m_hScrollBar && m_hScrollBar->getVisible()) {
        m_hScrollBar->update();
    }
}

void TextArea::draw(void) {
    if (!m_visible) return;

    ControlImpl::preDraw();

    if (m_lines.empty() || m_lineStartPositions.empty()) {
        rebuildLines();
    }

    SRect drawRect = getDrawRect();
    float scale = getScaleXX();

    if (drawRect.width <= 0 || drawRect.height <= 0 || m_lineHeight <= 0) {
        return;
    }

    if (m_hScrollBar && m_hScrollBar->getVisible()) {
        m_hScrollBar->draw();
    }

    if (m_vScrollBar && m_vScrollBar->getVisible()) {
        m_updatingScrollBar = true;
        m_vScrollBar->setValue((float)m_scrollY);
        m_updatingScrollBar = false;
        m_vScrollBar->draw();
    }

    int clipWidth = (int)drawRect.width;
    int clipHeight = (int)drawRect.height;
    Margin margin = getMargin();
    float marginX = margin.left * scale;
    float marginY = margin.top * scale;
    float marginRight = margin.right * scale;
    float marginBottom = margin.bottom * scale;
    clipHeight -= (int)(marginY + marginBottom);
    float vThickness = m_vScrollBar ? m_vScrollBar->getThickness() : 16.0f;
    float hThickness = m_hScrollBar ? m_hScrollBar->getThickness() : 16.0f;
    if (m_vScrollBar && m_vScrollBar->getVisible()) {
        clipWidth -= (int)(vThickness * scale);
    }
    if (m_hScrollBar && m_hScrollBar->getVisible()) {
        clipHeight -= (int)(hThickness * scale);
    }
    SRect clipRect(drawRect.left + marginX, drawRect.top + marginY,
                   (float)(clipWidth - (int)(marginX + marginRight)), (float)clipHeight);
    GET_RENDERDEVICE->setClipRect(clipRect);

    int startLine = 0;
    int endLine = getTotalLines();
    int availableHeight = (int)drawRect.height;
    availableHeight -= (int)(marginY + marginBottom);
    if (m_hScrollBar && m_hScrollBar->getVisible()) {
        availableHeight -= (int)(hThickness * scale);
    }
    if (m_lineHeight > 0) {
        startLine = m_scrollY / m_lineHeight;
        int visibleLines = availableHeight / m_lineHeight;
        endLine = std::min(startLine + visibleLines + 1, getTotalLines());
    }

    if (hasSelection()) {
        int selStart = std::min(m_selectionStart, m_selectionEnd);
        int selEnd = std::max(m_selectionStart, m_selectionEnd);

        GET_RENDERDEVICE->setBlendMode(BlendMode::Blend);
        for (int i = startLine; i < endLine && i < getTotalLines(); ++i) {
            float y = drawRect.top + (i * m_lineHeight) * scale - m_scrollY * scale + marginY;

            if (i >= (int)m_lineStartPositions.size()) continue;
            int lineStartByte = m_lineStartPositions[i];
            int lineEndByte = lineStartByte + (int)m_lines[i].length();

            if (selEnd <= lineStartByte || selStart > lineEndByte) {
                continue;
            }

            int selStartInLine = std::max(0, selStart - lineStartByte);
            int selEndInLine = std::min((int)m_lines[i].length(), selEnd - lineStartByte);

            if (selStartInLine >= selEndInLine) continue;

            float selStartX = drawRect.left + marginX - (float)m_scrollX;
            std::string textBeforeSel = m_lines[i].substr(0, selStartInLine);
            if (!textBeforeSel.empty() && getTextRenderer() && getFont()) {
                SSize size = getTextRenderer()->measureText(getFont(), textBeforeSel);
                selStartX += size.width;
            }

            float selEndX = selStartX;
            std::string selectedText = m_lines[i].substr(selStartInLine, selEndInLine - selStartInLine);
            if (!selectedText.empty() && getTextRenderer() && getFont()) {
                SSize size = getTextRenderer()->measureText(getFont(), selectedText);
                selEndX += size.width;
            }

            SRect selRect(selStartX, y - marginY / 2, selEndX - selStartX, m_fontSize * scale + marginY);
            GET_RENDERDEVICE->setDrawColor(SColor(173, 214, 255, 128));
            GET_RENDERDEVICE->fillRect(selRect);
        }
        GET_RENDERDEVICE->setBlendMode(BlendMode::None);
    }

    for (int i = startLine; i < endLine; ++i) {
        if (i >= getTotalLines()) break;

        float y = drawRect.top + (i * m_lineHeight) * scale - m_scrollY * scale + marginY;

        if (!m_lines[i].empty()) {
            std::string displayLine = m_lines[i];
            if (isPasswordMode()) {
                displayLine = std::string(displayLine.length(), '*');
            }

            if (getTextRenderer() && getFont()) {
                SColor textColor = m_textColor.getNormal();
                float textStartX = drawRect.left + marginX - (float)m_scrollX;
                getTextRenderer()->drawText(getFont(), displayLine, textStartX, y, textColor);
            }
        }
    }

    int cursorByteIndex = getCursorPosition();

    int currentLine = 0;
    int byteIndexInLine = 0;

    if (!m_lines.empty() && !m_lineStartPositions.empty() && getTotalLines() > 0) {
        for (int i = 0; i < getTotalLines() && i < (int)m_lineStartPositions.size(); ++i) {
            int lineStart = m_lineStartPositions[i];
            int lineEnd;
            if (i + 1 < getTotalLines() && i + 1 < (int)m_lineStartPositions.size()) {
                lineEnd = m_lineStartPositions[i + 1];
            } else {
                lineEnd = (int)getText().length();
            }

            if (cursorByteIndex >= lineStart && cursorByteIndex < lineEnd) {
                currentLine = i;
                byteIndexInLine = cursorByteIndex - lineStart;
                break;
            }

            if (cursorByteIndex == lineEnd) {
                if (cursorByteIndex > 0 && m_text[cursorByteIndex - 1] == '\n') {
                    if (i + 1 < getTotalLines()) {
                        currentLine = i + 1;
                        byteIndexInLine = 0;
                    } else {
                        currentLine = i;
                        byteIndexInLine = (int)m_lines[i].length();
                    }
                } else {
                    currentLine = i;
                    if (i < (int)m_lines.size()) {
                        byteIndexInLine = (int)m_lines[i].length();
                    }
                }
                break;
            }
        }

        if (cursorByteIndex > (int)getText().length()) {
            currentLine = getTotalLines() - 1;
            byteIndexInLine = 0;
        }
    }

    float cursorY = drawRect.top + (currentLine * m_lineHeight) * scale - m_scrollY * scale + marginY;
    float cursorH = m_fontSize * scale;

    std::string textOnCurrentLine;
    if (currentLine < getTotalLines() && currentLine < (int)m_lines.size()) {
        textOnCurrentLine = m_lines[currentLine].substr(0, std::min(byteIndexInLine, (int)m_lines[currentLine].length()));
    }

    int cursorX = (int)(drawRect.left + marginX - m_scrollX);
    if (!textOnCurrentLine.empty() && getTextRenderer() && getFont()) {
        SSize size = getTextRenderer()->measureText(getFont(), textOnCurrentLine);
        cursorX += (int)size.width;
    }

    if (m_focused && m_cursorVisible && cursorY >= drawRect.top && cursorY + cursorH <= drawRect.top + drawRect.height) {
        SRect cursorRect((float)cursorX, cursorY, 2.0f * scale, cursorH);
        GET_RENDERDEVICE->setDrawColor(m_textColor.getNormal());
        GET_RENDERDEVICE->fillRect(cursorRect);
    }

    GET_RENDERDEVICE->clearClipRect();
}

bool TextArea::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;

    if (event->m_eventName == EventName::TEXT_INPUT) {
        if (m_focused) {
            if (!event->m_eventParam.has_value()) return false;
            try {
                auto data = std::any_cast<TextInputEventData>(event->m_eventParam);
                for (char c : data.text) {
                    if (c == '\r') continue;
                    insertText(std::string(1, c));
                }
                rebuildLines();
                updateVScrollBar();
                updateHScrollBar();
                ensureCursorVisible();
                ensureCursorHorizontalVisible();
                return true;
            } catch (...) {
                return false;
            }
        }
    }

    if (event->m_eventName == EventName::KEY_DOWN && !m_focused) {
        return false;
    }

    if (event->m_eventName == EventName::KEY_DOWN) {
        try {
            auto keyData = std::any_cast<KeyEventData>(event->m_eventParam);
            if (keyData.keycode == SDLK_RETURN || keyData.keycode == SDLK_RETURN2) {
                int oldCursorPos = m_cursorPosition;

                std::string newText = m_text;
                newText.insert(m_cursorPosition, 1, '\n');
                m_text = newText;
                m_cursorPosition++;

                rebuildLines();

                for (int i = 0; i < getTotalLines() && i < (int)m_lineStartPositions.size(); ++i) {
                    int lineStart = m_lineStartPositions[i];
                    if (oldCursorPos + 1 == lineStart) {
                        m_cursorPosition = lineStart;
                        break;
                    }
                }

                updateVScrollBar();
                updateHScrollBar();
                ensureCursorVisible();
                ensureCursorHorizontalVisible();
                updateVScrollBar();
                ensureCursorVisible();
                if (m_onTextChanged) {
                    m_onTextChanged(getThis(), m_text);
                }
                return true;
            }
        } catch (...) {
        }
    }

    if (m_vScrollBar && m_vScrollBar->getVisible()) {
        bool isMouseEvent = event->m_eventName == EventName::MOUSE_LBUTTON_DOWN ||
                           event->m_eventName == EventName::MOUSE_LBUTTON_UP ||
                           event->m_eventName == EventName::MOUSE_MOVING;
        if (isMouseEvent && event->m_eventParam.has_value()) {
            bool isDragging = m_vScrollBar->isDragging();
            try {
                auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
                if (isDragging || (pos && m_vScrollBar->isContainsPoint(pos->x, pos->y))) {
                    if (m_vScrollBar->handleEvent(event)) {
                        return true;
                    }
                }
            } catch (...) {
            }
        }
    }

    if (m_hScrollBar && m_hScrollBar->getVisible()) {
        bool isMouseEvent = event->m_eventName == EventName::MOUSE_LBUTTON_DOWN ||
                           event->m_eventName == EventName::MOUSE_LBUTTON_UP ||
                           event->m_eventName == EventName::MOUSE_MOVING;
        if (isMouseEvent && event->m_eventParam.has_value()) {
            bool isDragging = m_hScrollBar->isDragging();
            try {
                auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
                if (isDragging || (pos && m_hScrollBar->isContainsPoint(pos->x, pos->y))) {
                    if (m_hScrollBar->handleEvent(event)) {
                        return true;
                    }
                }
            } catch (...) {
            }
        }
    }

    if (event->m_eventName == EventName::MOUSE_LBUTTON_DOWN) {
        if (!event->m_eventParam.has_value()) return false;
        try {
            auto pos = std::any_cast<shared_ptr<SPoint>>(event->m_eventParam);
            if (!pos) return false;
            if (isContainsPoint(pos->x, pos->y)) {
                setFocused(true);

                SRect drawRect = getDrawRect();
                float scale = getScaleXX();
                Margin margin = getMargin();
                float marginX = margin.left * scale;
                float baseX = drawRect.left + marginX - (float)m_scrollX;
                float relX = pos->x - baseX;
                float relY = (pos->y - drawRect.top) / scale + m_scrollY;
                int targetLine = (int)(relY / m_lineHeight);
                targetLine = std::max(0, std::min(targetLine, getTotalLines() - 1));

                int targetByteIndex = 0;
                if (targetLine < getTotalLines() && targetLine < (int)m_lines.size()) {
                    std::string& targetLineText = m_lines[targetLine];
                    targetByteIndex = getByteIndexFromPixelX(targetLineText, relX);
                }

                int targetPos = 0;
                if (targetLine < (int)m_lineStartPositions.size()) {
                    targetPos = m_lineStartPositions[targetLine];
                }
                targetPos += targetByteIndex;

                SDL_Keymod mod = SDL_GetModState();
                bool shiftPressed = (mod & SDL_KMOD_SHIFT) != 0;

                if (shiftPressed) {
                    m_selectionEnd = targetPos;
                    m_cursorPosition = targetPos;
                } else {
                    m_cursorPosition = targetPos;
                    clearSelection();
                }

                m_cursorVisible = true;
                m_cursorBlinkTime = 0;
                m_isDragging = true;
                m_dragStartPosition = targetPos;
                updateTextOffset();
                ensureCursorVisible();

                return true;
            } else {
                setFocused(false);
            }
        } catch (...) {
            return false;
        }
    }

    if (event->m_eventName == EventName::MOUSE_WHEEL) {
        if (!event->m_eventParam.has_value()) return false;
        try {
            MouseWheelEventData wheelData = std::any_cast<MouseWheelEventData>(event->m_eventParam);
            if (!isContainsPoint(wheelData.mouseX, wheelData.mouseY)) {
                return false;
            }
            int scrollAmount = (int)(wheelData.y * m_lineHeight * 3);
            setScrollY(m_scrollY - scrollAmount);
            return true;
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
                SRect drawRect = getDrawRect();
                float scale = getScaleXX();
                Margin margin = getMargin();
                float marginX = margin.left * scale;
                float baseX = drawRect.left + marginX - (float)m_scrollX;
                float relX = pos->x - baseX;
                float relY = (pos->y - drawRect.top) / scale + m_scrollY;
                int targetLine = (int)(relY / m_lineHeight);
                targetLine = std::max(0, std::min(targetLine, getTotalLines() - 1));

                int targetByteIndex = 0;
                if (targetLine < getTotalLines() && targetLine < (int)m_lines.size()) {
                    std::string& targetLineText = m_lines[targetLine];
                    targetByteIndex = getByteIndexFromPixelX(targetLineText, relX);
                }

                int targetPos = 0;
                if (targetLine < (int)m_lineStartPositions.size()) {
                    targetPos = m_lineStartPositions[targetLine];
                }
                targetPos += targetByteIndex;

                m_cursorPosition = targetPos;
                m_selectionStart = m_dragStartPosition;
                m_selectionEnd = targetPos;
                if (m_selectionEnd < m_selectionStart) {
                    m_selectionEnd = m_selectionStart;
                    m_selectionStart = targetPos;
                }
                ensureCursorVisible();
                ensureCursorHorizontalVisible();
                return true;
            } catch (...) {
                return false;
            }
        }
    }

    if (event->m_eventName == EventName::MOUSE_LBUTTON_UP) {
        m_isDragging = false;
    }

    if (event->m_eventName == EventName::KEY_DOWN) {
        if (isFocused()) {
            auto keyData = std::any_cast<KeyEventData>(event->m_eventParam);

            if ((keyData.mod & SDL_KMOD_CTRL) && (keyData.keycode == SDLK_C || keyData.keycode == SDLK_V || keyData.keycode == SDLK_X)) {
                if (keyData.keycode == SDLK_C) {
                    copy();
                    return true;
                }
                if (keyData.keycode == SDLK_V) {
                    paste();
                    rebuildLines();
                    updateVScrollBar();
                    updateHScrollBar();
                    ensureCursorVisible();
                    return true;
                }
                if (keyData.keycode == SDLK_X) {
                    cut();
                    rebuildLines();
                    updateVScrollBar();
                    updateHScrollBar();
                    return true;
                }
            }

            if (keyData.keycode == SDLK_UP) {
                int currentLine = 0;
                int byteIndexInLine = 0;
                for (int i = 0; i < getTotalLines() && i < (int)m_lineStartPositions.size(); ++i) {
                    int lineStart = m_lineStartPositions[i];
                    int lineEnd;
                    if (i + 1 < getTotalLines()) {
                        lineEnd = m_lineStartPositions[i + 1];
                    } else {
                        lineEnd = (int)m_text.length();
                    }

                    if (m_cursorPosition >= lineStart && m_cursorPosition < lineEnd) {
                        currentLine = i;
                        byteIndexInLine = m_cursorPosition - lineStart;
                        break;
                    }

                    if (m_cursorPosition == lineEnd) {
                        if (m_cursorPosition > 0 && m_cursorPosition <= (int)m_text.length() && m_text[m_cursorPosition - 1] == '\n') {
                            if (i + 1 < getTotalLines()) {
                                currentLine = i + 1;
                                byteIndexInLine = 0;
                            } else {
                                currentLine = i;
                                byteIndexInLine = (int)m_lines[i].length();
                            }
                        } else {
                            currentLine = i;
                            byteIndexInLine = (int)m_lines[i].length();
                        }
                        break;
                    }
                }

                if (currentLine <= 0) return true;

                float scale = getScaleXX();
                Margin margin = getMargin();
                float marginX = margin.left * scale;
                float baseX = getDrawRect().left + marginX;

                std::string currentLineText = (currentLine < (int)m_lines.size()) ? m_lines[currentLine] : "";
                float cursorX = baseX;
                for (int i = 0; i < byteIndexInLine && i < (int)currentLineText.length(); ++i) {
                    cursorX += getCharWidth(currentLineText, i);
                }

                std::string targetLine = m_lines[currentLine - 1];
                int bestByteIndex = 0;
                float bestDiff = FLT_MAX;
                float x = baseX;
                for (int i = 0; i <= (int)targetLine.length(); ++i) {
                    float diff = fabs(x - cursorX);
                    if (diff < bestDiff) {
                        bestDiff = diff;
                        bestByteIndex = i;
                    }
                    if (i < (int)targetLine.length()) {
                        x += getCharWidth(targetLine, i);
                    }
                }

                int newCursorPos = m_lineStartPositions[currentLine - 1] + bestByteIndex;

                bool shiftPressed = (keyData.mod & SDL_KMOD_SHIFT) != 0;
                if (shiftPressed) {
                    if (!hasSelection()) {
                        m_selectionStart = m_cursorPosition;
                    }
                    m_selectionEnd = newCursorPos;
                }
                m_cursorPosition = newCursorPos;
                ensureCursorVisible();
                return true;
            }

            if (keyData.keycode == SDLK_DOWN) {
                int currentLine = 0;
                int byteIndexInLine = 0;
                for (int i = 0; i < getTotalLines() && i < (int)m_lineStartPositions.size(); ++i) {
                    int lineStart = m_lineStartPositions[i];
                    int lineEnd;
                    if (i + 1 < getTotalLines()) {
                        lineEnd = m_lineStartPositions[i + 1];
                    } else {
                        lineEnd = (int)m_text.length();
                    }

                    if (m_cursorPosition >= lineStart && m_cursorPosition < lineEnd) {
                        currentLine = i;
                        byteIndexInLine = m_cursorPosition - lineStart;
                        break;
                    }

                    if (m_cursorPosition == lineEnd) {
                        if (m_cursorPosition > 0 && m_cursorPosition <= (int)m_text.length() && m_text[m_cursorPosition - 1] == '\n') {
                            if (i + 1 < getTotalLines()) {
                                currentLine = i + 1;
                                byteIndexInLine = 0;
                            } else {
                                currentLine = i;
                                byteIndexInLine = (int)m_lines[i].length();
                            }
                        } else {
                            currentLine = i;
                            byteIndexInLine = (int)m_lines[i].length();
                        }
                        break;
                    }
                }

                if (currentLine == 0 && m_cursorPosition >= (int)m_text.length()) {
                    currentLine = getTotalLines() - 1;
                    byteIndexInLine = 0;
                }

                if (currentLine == 0 && m_cursorPosition == (int)m_text.length() &&
                    (!m_text.empty() && m_text.back() != '\n')) {
                    currentLine = getTotalLines() - 1;
                    byteIndexInLine = (int)m_lines[currentLine].length();
                }

                if (currentLine >= getTotalLines() - 1) return true;

                float scale = getScaleXX();
                Margin margin = getMargin();
                float marginX = margin.left * scale;
                float baseX = getDrawRect().left + marginX;

                std::string currentLineText = (currentLine < (int)m_lines.size()) ? m_lines[currentLine] : "";
                float cursorX = baseX;
                for (int i = 0; i < byteIndexInLine && i < (int)currentLineText.length(); ++i) {
                    cursorX += getCharWidth(currentLineText, i);
                }

                std::string targetLine = m_lines[currentLine + 1];
                int bestByteIndex = 0;
                float bestDiff = FLT_MAX;
                float x = baseX;
                for (int i = 0; i <= (int)targetLine.length(); ++i) {
                    float diff = fabs(x - cursorX);
                    if (diff < bestDiff) {
                        bestDiff = diff;
                        bestByteIndex = i;
                    }
                    if (i < (int)targetLine.length()) {
                        x += getCharWidth(targetLine, i);
                    }
                }

                int newCursorPos = m_lineStartPositions[currentLine + 1] + bestByteIndex;

                bool shiftPressed = (keyData.mod & SDL_KMOD_SHIFT) != 0;
                if (shiftPressed) {
                    if (!hasSelection()) {
                        m_selectionStart = m_cursorPosition;
                    }
                    m_selectionEnd = newCursorPos;
                }
                m_cursorPosition = newCursorPos;
                ensureCursorVisible();
                return true;
            }

            if (keyData.keycode == SDLK_HOME) {
                bool shiftPressed = (keyData.mod & SDL_KMOD_SHIFT) != 0;
                int oldCursorPosition = m_cursorPosition;
                int currentLine = 0;
                for (int i = 0; i < getTotalLines() && i < (int)m_lineStartPositions.size(); ++i) {
                    int lineStart = m_lineStartPositions[i];
                    int lineEnd;
                    if (i + 1 < getTotalLines()) {
                        lineEnd = m_lineStartPositions[i + 1];
                    } else {
                        lineEnd = (int)m_text.length();
                    }

                    if (m_cursorPosition >= lineStart && m_cursorPosition < lineEnd) {
                        currentLine = i;
                        break;
                    }
                    if (m_cursorPosition == lineEnd) {
                        currentLine = i;
                        break;
                    }
                }
                m_cursorPosition = m_lineStartPositions[currentLine];
                if (shiftPressed) {
                    if (!hasSelection()) {
                        m_selectionStart = oldCursorPosition;
                    }
                    m_selectionEnd = m_cursorPosition;
                } else {
                    clearSelection();
                }
                ensureCursorVisible();
                ensureCursorHorizontalVisible();
                return true;
            }

            if (keyData.keycode == SDLK_END) {
                bool shiftPressed = (keyData.mod & SDL_KMOD_SHIFT) != 0;
                int oldCursorPosition = m_cursorPosition;
                int currentLine = 0;
                for (int i = 0; i < getTotalLines() && i < (int)m_lineStartPositions.size(); ++i) {
                    int lineStart = m_lineStartPositions[i];
                    int lineEnd;
                    if (i + 1 < getTotalLines()) {
                        lineEnd = m_lineStartPositions[i + 1];
                    } else {
                        lineEnd = (int)m_text.length();
                    }

                    if (m_cursorPosition >= lineStart && m_cursorPosition <= lineEnd) {
                        currentLine = i;
                        break;
                    }
                }
                if (currentLine < getTotalLines() && currentLine < (int)m_lineStartPositions.size()) {
                    int lineStart = m_lineStartPositions[currentLine];
                    int lineEnd;
                    if (currentLine + 1 < getTotalLines()) {
                        lineEnd = m_lineStartPositions[currentLine + 1];
                    } else {
                        lineEnd = (int)m_text.length();
                    }
                    m_cursorPosition = lineStart + (currentLine < (int)m_lines.size() ? (int)m_lines[currentLine].length() : 0);
                }
                if (shiftPressed) {
                    if (!hasSelection()) {
                        m_selectionStart = oldCursorPosition;
                    }
                    m_selectionEnd = m_cursorPosition;
                } else {
                    clearSelection();
                }
                ensureCursorVisible();
                ensureCursorHorizontalVisible();
                return true;
            }

            if (keyData.keycode == SDLK_LEFT) {
                bool shiftPressed = (keyData.mod & SDL_KMOD_SHIFT) != 0;
                int oldCursorPosition = m_cursorPosition;
                if (m_cursorPosition > 0) {
                    if (m_cursorPosition > 0 && m_text[m_cursorPosition - 1] == '\n') {
                        for (int i = 0; i < getTotalLines() && i < (int)m_lineStartPositions.size(); ++i) {
                            int lineStart = m_lineStartPositions[i];
                            if (lineStart == m_cursorPosition - 1) {
                                if (i > 0) {
                                    m_cursorPosition = m_lineStartPositions[i - 1] + m_lines[i - 1].length();
                                    if (shiftPressed) {
                                        if (!hasSelection()) {
                                            m_selectionStart = oldCursorPosition;
                                        }
                                        m_selectionEnd = m_cursorPosition;
                                    } else {
                                        clearSelection();
                                    }
                                    ensureCursorVisible();
                                    ensureCursorHorizontalVisible();
                                    return true;
                                }
                            }
                        }
                    }
                    int newPos = m_cursorPosition;
                    while (newPos > 0 && (m_text[newPos - 1] & 0xC0) == 0x80) {
                        newPos--;
                    }
                    newPos--;
                    m_cursorPosition = std::max(0, newPos);
                    if (shiftPressed) {
                        if (!hasSelection()) {
                            m_selectionStart = oldCursorPosition;
                        }
                        m_selectionEnd = m_cursorPosition;
                    } else {
                        clearSelection();
                    }
                    ensureCursorVisible();
                    ensureCursorHorizontalVisible();
                }
                return true;
            }

            if (keyData.keycode == SDLK_RIGHT) {
                bool shiftPressed = (keyData.mod & SDL_KMOD_SHIFT) != 0;
                int oldCursorPosition = m_cursorPosition;
                if (m_cursorPosition < (int)m_text.length()) {
                    if (m_text[m_cursorPosition] == '\n') {
                        for (int i = 0; i < getTotalLines() && i < (int)m_lineStartPositions.size(); ++i) {
                            int lineStart = m_lineStartPositions[i];
                            if (lineStart == m_cursorPosition) {
                                if (i + 1 < getTotalLines()) {
                                    m_cursorPosition = m_lineStartPositions[i + 1];
                                    if (shiftPressed) {
                                        if (!hasSelection()) {
                                            m_selectionStart = oldCursorPosition;
                                        }
                                        m_selectionEnd = m_cursorPosition;
                                    } else {
                                        clearSelection();
                                    }
                                    ensureCursorVisible();
                                    ensureCursorHorizontalVisible();
                                    return true;
                                }
                            }
                        }
                    }
                    int charLen = getUtf8CharLength((unsigned char)m_text[m_cursorPosition]);
                    m_cursorPosition = std::min((int)m_text.length(), m_cursorPosition + charLen);
                    if (shiftPressed) {
                        if (!hasSelection()) {
                            m_selectionStart = oldCursorPosition;
                        }
                        m_selectionEnd = m_cursorPosition;
                    } else {
                        clearSelection();
                    }
                    ensureCursorVisible();
                    ensureCursorHorizontalVisible();
                }
                return true;
            }
        }
    }

    return EditBox::handleEvent(event);
}

void TextArea::deleteSelectedText() {
    if (m_selectionStart == m_selectionEnd) return;

    int start = std::min(m_selectionStart, m_selectionEnd);
    int endVal = std::max(m_selectionStart, m_selectionEnd);

    m_text.erase(start, endVal - start);
    m_cursorPosition = start;
    clearSelection();
    rebuildLines();
    updateVScrollBar();
    updateHScrollBar();
    ensureCursorVisible();

    if (m_onTextChanged) {
        m_onTextChanged(getThis(), m_text);
    }
}

void TextArea::setScrollY(int y) {
    if (!m_vScrollBar) return;

    int totalHeight = getTotalLines() * m_lineHeight;
    if (totalHeight <= 0) return;

    int maxScroll = (int)m_vScrollBar->getMaxValue();

    m_scrollY = std::max(0, std::min(y, maxScroll));

    m_updatingScrollBar = true;
    m_vScrollBar->setValue((float)m_scrollY);
    m_updatingScrollBar = false;
}

void TextArea::scrollToBottom() {
    SRect rect = getRect();
    int totalHeight = getTotalLines() * m_lineHeight;
    int targetScroll = totalHeight - (int)rect.height;

    setScrollY(targetScroll);
}

void TextArea::ensureCursorHorizontalVisible() {
    if (!m_hScrollBar) return;

    SRect drawRect = getDrawRect();
    if (drawRect.width <= 0) return;

    float scale = getScaleXX();
    Margin margin = getMargin();
    int marginX = (int)(margin.left * scale);
    int marginRight = (int)(margin.right * scale);
    int availableWidth = (int)drawRect.width - marginX - marginRight;

    bool vScrollVisible = m_vScrollBar && m_vScrollBar->getVisible();

    if (vScrollVisible) {
        float vThickness = m_vScrollBar ? m_vScrollBar->getThickness() : 16.0f;
        availableWidth -= (int)(vThickness * scale);
    }

    if (availableWidth <= 0) return;

    int cursorLine = 0;
    int cursorByteIndex = getCursorPosition();
    int totalLines = getTotalLines();

    if (totalLines > 0 && !m_lineStartPositions.empty()) {
        for (int i = 0; i < totalLines && i < (int)m_lineStartPositions.size(); ++i) {
            int lineStart = m_lineStartPositions[i];
            int lineEnd = (i + 1 < totalLines) ? m_lineStartPositions[i + 1] : (int)m_text.length();

            if (cursorByteIndex >= lineStart && cursorByteIndex < lineEnd) {
                cursorLine = i;
                break;
            }
            if (cursorByteIndex == lineEnd) {
                cursorLine = i;
                break;
            }
        }
    }

    if (cursorLine >= totalLines || cursorLine >= (int)m_lines.size()) {
        return;
    }
    if (cursorLine < 0) return;

    int byteIndexInLine = cursorByteIndex - m_lineStartPositions[cursorLine];
    if (byteIndexInLine < 0) byteIndexInLine = 0;

    std::string textBeforeCursor = m_lines[cursorLine].substr(0, std::min(byteIndexInLine, (int)m_lines[cursorLine].length()));
    int cursorPixelX = getLinePixelWidth(textBeforeCursor);

    int maxScroll = (int)m_hScrollBar->getMaxValue();
    int targetScrollX = m_scrollX;

    int cursorWidth = (int)(2.0f * scale);

    int visibleStart = m_scrollX;
    int visibleEnd = m_scrollX + availableWidth;

    if (cursorPixelX + cursorWidth > visibleEnd) {
        targetScrollX = cursorPixelX + cursorWidth - availableWidth;
    } else if (cursorPixelX < visibleStart) {
        targetScrollX = cursorPixelX;
    }

    if (targetScrollX > maxScroll) {
        targetScrollX = maxScroll;
    }
    if (targetScrollX < 0) {
        targetScrollX = 0;
    }

    if (targetScrollX != m_scrollX) {
        setScrollX(targetScrollX);
    }
}

void TextArea::ensureCursorVisible() {
    SRect drawRect = getDrawRect();
    SRect rect = getRect();
    if (drawRect.height <= 0) return;

    float scale = getScaleXX();
    Margin margin = getMargin();
    int marginY = (int)(margin.top * scale);
    int marginBottom = (int)(margin.bottom * scale);
    int availableHeightScaled = (int)drawRect.height - marginY - marginBottom;

    if (m_hScrollBar && m_hScrollBar->getVisible()) {
        float hThickness = m_hScrollBar ? m_hScrollBar->getThickness() : 16.0f;
        availableHeightScaled -= (int)(hThickness * scale);
    }

    int availableHeight = (int)(availableHeightScaled / scale);

    int totalHeight = getTotalLines() * m_lineHeight;
    int maxScroll = totalHeight - availableHeight;
    if (maxScroll < 0) maxScroll = 0;

    if (m_scrollY > maxScroll) {
        m_scrollY = maxScroll;
    }

    int cursorLine = 0;
    int cursorByteIndex = getCursorPosition();
    int totalLines = getTotalLines();

    if (totalLines > 0 && !m_lineStartPositions.empty()) {
        for (int i = 0; i < totalLines && i < (int)m_lineStartPositions.size(); ++i) {
            int lineStart = m_lineStartPositions[i];
            int lineEnd = (i + 1 < totalLines) ? m_lineStartPositions[i + 1] : (int)m_text.length();

            if (cursorByteIndex >= lineStart && cursorByteIndex < lineEnd) {
                cursorLine = i;
                break;
            }
            if (cursorByteIndex == lineEnd) {
                cursorLine = i;
                break;
            }
        }
    }

    if (m_autoScroll && cursorLine >= totalLines - 1) {
        scrollToBottom();
        return;
    }

    int cursorY = cursorLine * m_lineHeight;

    if (cursorY < m_scrollY) {
        m_scrollY = cursorY;
    } else if (cursorY + m_lineHeight > m_scrollY + availableHeight) {
        m_scrollY = cursorY + m_lineHeight - availableHeight;
    }

    if (m_scrollY > maxScroll) {
        m_scrollY = maxScroll;
    }
    if (m_scrollY < 0) {
        m_scrollY = 0;
    }

    if (m_autoScroll) {
        m_scrollY = maxScroll;
    }
}

void TextArea::insertTextAtCursor(const std::string& text) {
    m_text.insert(m_cursorPosition, text);
    m_cursorPosition += (int)text.length();
    rebuildLines();
    updateVScrollBar();
    updateHScrollBar();
}

void TextArea::insertText(const std::string& text) {
    EditBox::insertText(text);
    rebuildLines();
    updateVScrollBar();
    updateHScrollBar();
    ensureCursorVisible();
}

void TextArea::setRect(SRect rect) {
    EditBox::setRect(rect);
    updateVScrollBar();
    updateHScrollBar();
}

void TextArea::setText(const std::string& text) {
    EditBox::setText(text);
    rebuildLines();
    updateVScrollBar();
    updateHScrollBar();
}

void TextArea::setWordWrap(bool enable) {
    m_wordWrap = enable;
    rebuildLines();
    updateVScrollBar();
    updateHScrollBar();
}

void TextArea::setLineHeight(int height) {
    m_lineHeight = height;
    updateVScrollBar();
    updateHScrollBar();
}

int TextArea::getMaxLinePixelWidth() {
    int maxWidth = 0;
    for (const auto& line : m_lines) {
        int width = getLinePixelWidth(line);
        if (width > maxWidth) maxWidth = width;
    }
    return maxWidth;
}

void TextArea::setScrollBarThickness(float thickness) {
    if (m_vScrollBar) {
        m_vScrollBar->setThickness(thickness);
    }
    if (m_hScrollBar) {
        m_hScrollBar->setThickness(thickness);
    }
    updateVScrollBar();
    updateHScrollBar();
}

float TextArea::getScrollBarThickness() const {
    if (m_vScrollBar) {
        return m_vScrollBar->getThickness();
    }
    return 16.0f;
}

void TextArea::setOnTextChangedHandler(OnTextChangedHandler handler) {
    m_onTextChanged = handler;
}

TextAreaBuilder::TextAreaBuilder(Control *parent, SRect rect, float xScale, float yScale)
    : m_textArea(make_shared<TextArea>(parent, rect, xScale, yScale))
{
}

TextAreaBuilder& TextAreaBuilder::setBackgroundStateColor(StateColor stateColor) {
    m_textArea->setBackgroundStateColor(stateColor);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setBorderStateColor(StateColor stateColor) {
    m_textArea->setBorderStateColor(stateColor);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setTextStateColor(StateColor stateColor) {
    m_textArea->setTextStateColor(stateColor);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setText(const std::string& text) {
    m_textArea->setText(text);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setPlaceholder(const std::string& placeholder) {
    m_textArea->setPlaceholder(placeholder);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setPasswordMode(bool enable) {
    m_textArea->setPasswordMode(enable);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setPasswordChar(char c) {
    m_textArea->setPasswordChar(c);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setFont(FontName fontName) {
    m_textArea->setFont(fontName);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setFontSize(int size) {
    m_textArea->setFontSize(size);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setWordWrap(bool enable) {
    m_textArea->setWordWrap(enable);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setLineHeight(int height) {
    m_textArea->setLineHeight(height);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setOnTextChanged(TextArea::OnTextChangedHandler handler) {
    m_textArea->setOnTextChangedHandler(handler);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setOnEnter(EditBox::OnEnterHandler handler) {
    m_textArea->setOnEnter(handler);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setId(int id) {
    m_textArea->setId(id);
    return *this;
}

TextAreaBuilder& TextAreaBuilder::setTransparent(bool isTransparent) {
    m_textArea->setTransparent(isTransparent);
    return *this;
}

shared_ptr<TextArea> TextAreaBuilder::build(void) {
    m_textArea->create();
    return m_textArea;
}
