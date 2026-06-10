#include "InputBackend.h"
#include "Utility.h"
#include "EventQueue.h"
#include "Window.h"
#include "RaylibCompat.h"
#include <queue>
#include <cstdio>

// ============================================================
// RaylibInputBackend
// ============================================================
class RaylibInputBackend : public InputBackend {
public:
    explicit RaylibInputBackend(Window* window)
        : m_window(window)
        , m_textInputActive(false)
        , m_lastMouseX(0.0f)
        , m_lastMouseY(0.0f)
        , m_lastFrameTime(0.0)
        , m_phase(Phase::Done)
        , m_lastFocused(true)
        , m_consumedMouseButtons(0)
        , m_consumedMouseReleases(0)
        , m_wheelConsumed(false)
        , m_keyConsumed(false)
        , m_charConsumed(false)
    {
    }

    ~RaylibInputBackend() override = default;

    void startTextInput() override { m_textInputActive = true; }
    void stopTextInput() override { m_textInputActive = false; }
    bool isTextInputActive() const override { return m_textInputActive; }

    void setClipboardText(const std::string& text) override {
        SetClipboardText(text.c_str());
    }

    std::string getClipboardText() const override {
        const char* text = GetClipboardText();
        return text ? std::string(text) : std::string();
    }

    bool hasScreenKeyboard() const override {
        return false;
    }

    void newFrame() override {
        // Poll input events so raylib/GLFW processes window messages.
        // This is the ONLY PollInputEvents call per frame — present() no
        // longer calls EndDrawing() (which would double-poll and destroy
        // the just-pressed/released state).
        PollInputEvents();

        m_phase = Phase::Keyboard;
        m_consumedMouseButtons = 0;
        m_consumedMouseReleases = 0;
        m_wheelConsumed = false;
        m_keyConsumed = false;
        m_charConsumed = false;
        m_resizeDetected = false;
    }

    bool pollEvent(Event& event) override {
        // Detect frame boundary via GetTime() change
        double now = GetTime();
        if (now > m_lastFrameTime) {
            m_lastFrameTime = now;
            m_phase = Phase::Keyboard;
            // Reset per-frame consumed flags so multiple keys/chars can be
            // picked up one-per-call within a single frame of event processing.
            m_keyConsumed = false;
            m_charConsumed = false;
            // m_consumedMouseButtons is NOT reset here — it persists across
            // all pollEvent calls within a render frame, preventing infinite
            // MouseDown loops.  It is reset in newFrame() instead.
        }

        int loopCount = 0;
        while (true) {
            loopCount++;
            if (loopCount > 20) {
                // Safety: if we loop >20 times without reaching Done, something is wrong
                m_phase = Phase::Done;
                return false;
            }

            switch (m_phase) {
                case Phase::Keyboard: {
                    if (!m_keyConsumed) {
                        int key = GetKeyPressed();
                        if (key != 0) {
                            fillKeyEvent(event, key, false);
                            m_keyConsumed = true;
                            return true;
                        }
                    }
                    m_phase = Phase::CharInput;
                    break;
                }

                case Phase::CharInput: {
                    if (!m_charConsumed) {
                        int ch = GetCharPressed();
                        if (ch != 0) {
                            fillTextInput(event, ch);
                            m_charConsumed = true;
                            return true;
                        }
                    }
                    m_phase = Phase::WindowEvents;
                    break;
                }

                case Phase::WindowEvents: {
                    if (IsWindowResized()) {
                        m_resizeDetected = true;
                        event.m_type = EventType::WindowResize;
                        event.resizeEvent.width = GetScreenWidth();
                        event.resizeEvent.height = GetScreenHeight();
                        m_phase = Phase::Done;
                        return true;
                    }
                    if (WindowShouldClose()) {
                        event.m_type = EventType::WindowClose;
                        m_phase = Phase::Done;
                        return true;
                    }
                    bool focused = IsWindowFocused();
                    if (focused != m_lastFocused) {
                        m_lastFocused = focused;
                        event.m_type = focused ? EventType::FocusGained : EventType::FocusLost;
                        event.focusEvent.focused = focused;
                        m_phase = Phase::Done;
                        return true;
                    }
                    m_phase = Phase::MouseButton;
                    break;
                }

                case Phase::MouseButton: {
                    // Check pressed (transition from up to down)
                    for (int i = 0; i < 3; i++) {
                        if (!(m_consumedMouseButtons & (1 << i)) && IsMouseButtonPressed(i)) {
                            m_consumedMouseButtons |= (1 << i);
                            Vector2 pos = GetMousePosition();
                            m_lastMouseX = pos.x;
                            m_lastMouseY = pos.y;
                            fillMouseButton(event, EventType::MouseDown, i, pos);
                            return true;
                        }
                    }
                    // Check released.  Suppress MOUSE_UP when a WindowResize
                    // was detected in this same event batch — the release is
                    // the tail of a resize-edge drag, not a normal click.
                    for (int i = 0; i < 3; i++) {
                        if (!(m_consumedMouseReleases & (1 << i)) && IsMouseButtonReleased(i)) {
                            m_consumedMouseReleases |= (1 << i);
                            if (!m_resizeDetected) {
                                Vector2 pos = GetMousePosition();
                                fillMouseButton(event, EventType::MouseUp, i, pos);
                                return true;
                            }
                        }
                    }
                    m_phase = Phase::MouseMove;
                    break;
                }

                case Phase::MouseMove: {
                    Vector2 pos = GetMousePosition();
                    if (pos.x != m_lastMouseX || pos.y != m_lastMouseY) {
                        m_lastMouseX = pos.x;
                        m_lastMouseY = pos.y;
                        event.m_type = EventType::MouseMove;
                        event.mousePos.x = pos.x;
                        event.mousePos.y = pos.y;
                        return true;
                    }
                    m_phase = Phase::MouseWheel;
                    break;
                }

                case Phase::MouseWheel: {
                    if (!m_wheelConsumed) {
                        Vector2 wheel = GetMouseWheelMoveV();
                        if (wheel.x != 0.0f || wheel.y != 0.0f) {
                            m_wheelConsumed = true;
                            Vector2 pos = GetMousePosition();
                            event.m_type = EventType::MouseWheel;
                            event.mouseWheel.x = pos.x;
                            event.mouseWheel.y = pos.y;
                            event.mouseWheel.scrollX = wheel.x;
                            event.mouseWheel.scrollY = wheel.y;
                            return true;
                        }
                    }
                    m_phase = Phase::Done;
                    break;
                }

                case Phase::Done:
                    return false;
            }
        }
    }

    KeyMod getModState() override {
        KeyMod mod = KeyMod::None;
        if (IsKeyDown(KEY_LEFT_SHIFT))   mod = mod | KeyMod::LShift;
        if (IsKeyDown(KEY_RIGHT_SHIFT))  mod = mod | KeyMod::RShift;
        if (IsKeyDown(KEY_LEFT_CONTROL)) mod = mod | KeyMod::LCtrl;
        if (IsKeyDown(KEY_RIGHT_CONTROL))mod = mod | KeyMod::RCtrl;
        if (IsKeyDown(KEY_LEFT_ALT))     mod = mod | KeyMod::LAlt;
        if (IsKeyDown(KEY_RIGHT_ALT))    mod = mod | KeyMod::RAlt;
        if (IsKeyDown(KEY_LEFT_SUPER))   mod = mod | KeyMod::LGui;
        if (IsKeyDown(KEY_RIGHT_SUPER))  mod = mod | KeyMod::RGui;
        if (IsKeyDown(KEY_NUM_LOCK))     mod = mod | KeyMod::NumLock;
        if (IsKeyDown(KEY_CAPS_LOCK))    mod = mod | KeyMod::CapsLock;
        return mod;
    }

private:
    enum class Phase {
        Keyboard, CharInput, WindowEvents, MouseButton, MouseMove, MouseWheel, Done
    };

    static KeyCode mapKeycode(int rlKey) {
        if (rlKey >= 32 && rlKey <= 126) {
            return static_cast<KeyCode>(rlKey);
        }
        switch (rlKey) {
            case KEY_SPACE:          return KeyCode::Space;
            case KEY_ESCAPE:         return KeyCode::Escape;
            case KEY_ENTER:          return KeyCode::Return;
            case KEY_TAB:            return KeyCode::Tab;
            case KEY_BACKSPACE:      return KeyCode::Backspace;
            case KEY_INSERT:         return KeyCode::Insert;
            case KEY_DELETE:         return KeyCode::Del;
            case KEY_RIGHT:          return KeyCode::Right;
            case KEY_LEFT:           return KeyCode::Left;
            case KEY_DOWN:           return KeyCode::Down;
            case KEY_UP:             return KeyCode::Up;
            case KEY_PAGE_UP:        return KeyCode::PageUp;
            case KEY_PAGE_DOWN:      return KeyCode::PageDown;
            case KEY_HOME:           return KeyCode::Home;
            case KEY_END:            return KeyCode::End;
            case KEY_CAPS_LOCK:      return KeyCode::CapsLock;
            case KEY_SCROLL_LOCK:    return KeyCode::ScrollLock;
            case KEY_NUM_LOCK:       return KeyCode::NumLock;
            case KEY_PRINT_SCREEN:   return KeyCode::PrintScreen;
            case KEY_PAUSE:          return KeyCode::Pause;
            case KEY_F1:             return KeyCode::F1;
            case KEY_F2:             return KeyCode::F2;
            case KEY_F3:             return KeyCode::F3;
            case KEY_F4:             return KeyCode::F4;
            case KEY_F5:             return KeyCode::F5;
            case KEY_F6:             return KeyCode::F6;
            case KEY_F7:             return KeyCode::F7;
            case KEY_F8:             return KeyCode::F8;
            case KEY_F9:             return KeyCode::F9;
            case KEY_F10:            return KeyCode::F10;
            case KEY_F11:            return KeyCode::F11;
            case KEY_F12:            return KeyCode::F12;
            case KEY_LEFT_SHIFT:     return KeyCode::LShift;
            case KEY_LEFT_CONTROL:   return KeyCode::LCtrl;
            case KEY_LEFT_ALT:       return KeyCode::LAlt;
            case KEY_LEFT_SUPER:     return KeyCode::LGui;
            case KEY_RIGHT_SHIFT:    return KeyCode::RShift;
            case KEY_RIGHT_CONTROL:  return KeyCode::RCtrl;
            case KEY_RIGHT_ALT:      return KeyCode::RAlt;
            case KEY_RIGHT_SUPER:    return KeyCode::RGui;
            case KEY_KB_MENU:        return KeyCode::Menu;
            case KEY_KP_0:           return KeyCode::KP0;
            case KEY_KP_1:           return KeyCode::KP1;
            case KEY_KP_2:           return KeyCode::KP2;
            case KEY_KP_3:           return KeyCode::KP3;
            case KEY_KP_4:           return KeyCode::KP4;
            case KEY_KP_5:           return KeyCode::KP5;
            case KEY_KP_6:           return KeyCode::KP6;
            case KEY_KP_7:           return KeyCode::KP7;
            case KEY_KP_8:           return KeyCode::KP8;
            case KEY_KP_9:           return KeyCode::KP9;
            case KEY_KP_DECIMAL:     return KeyCode::KPDecimal;
            case KEY_KP_DIVIDE:      return KeyCode::KPDivide;
            case KEY_KP_MULTIPLY:    return KeyCode::KPMultiply;
            case KEY_KP_SUBTRACT:    return KeyCode::KPMinus;
            case KEY_KP_ADD:         return KeyCode::KPPlus;
            case KEY_KP_ENTER:       return KeyCode::KPEnter;
            case KEY_KP_EQUAL:       return KeyCode::KPEquals;
            default:                 return KeyCode::Unknown;
        }
    }

    static MouseButton mapMouseButton(int rlBtn) {
        // raylib: 0=LEFT, 1=RIGHT, 2=MIDDLE, 3=SIDE, 4=EXTRA, 5=FORWARD, 6=BACK
        switch (rlBtn) {
            case 0: return MouseButton::Left;
            case 1: return MouseButton::Right;
            case 2: return MouseButton::Middle;
            case 3: return MouseButton::X1;
            case 4: return MouseButton::X2;
            default: return MouseButton::Left;
        }
    }

    void fillKeyEvent(Event& event, int rlKey, bool repeat) {
        event.m_type = EventType::KeyDown;
        event.keyEvent.keycode = mapKeycode(rlKey);
        event.keyEvent.mod = getModState();
        event.keyEvent.scancode = rlKey;
        event.keyEvent.repeat = repeat;
    }

    void fillTextInput(Event& event, int codepoint) {
        event.m_type = EventType::TextInput;
        int n = encodeUTF8(static_cast<unsigned int>(codepoint), event.textInput.text);
        event.textInput.text[n] = '\0';
    }

    void fillMouseButton(Event& event, EventType type, int rlBtn, const Vector2& pos) {
        event.m_type = type;
        event.mouseButton.x = pos.x;
        event.mouseButton.y = pos.y;
        event.mouseButton.button = mapMouseButton(rlBtn);
    }

    static int encodeUTF8(unsigned int cp, char* buf) {
        if (cp <= 0x7F) {
            buf[0] = static_cast<char>(cp);
            return 1;
        }
        if (cp <= 0x7FF) {
            buf[0] = static_cast<char>(0xC0 | (cp >> 6));
            buf[1] = static_cast<char>(0x80 | (cp & 0x3F));
            return 2;
        }
        if (cp <= 0xFFFF) {
            buf[0] = static_cast<char>(0xE0 | (cp >> 12));
            buf[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            buf[2] = static_cast<char>(0x80 | (cp & 0x3F));
            return 3;
        }
        if (cp <= 0x10FFFF) {
            buf[0] = static_cast<char>(0xF0 | (cp >> 18));
            buf[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            buf[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            buf[3] = static_cast<char>(0x80 | (cp & 0x3F));
            return 4;
        }
        return 0;
    }

    Window* m_window;
    bool m_textInputActive;
    float m_lastMouseX, m_lastMouseY;
    double m_lastFrameTime;
    Phase m_phase;
    bool m_lastFocused;
    uint8_t m_consumedMouseButtons;
    uint8_t m_consumedMouseReleases;
    bool m_wheelConsumed;
    bool m_keyConsumed;
    bool m_charConsumed;
    bool m_resizeDetected = false;
};

// ============================================================
// Factory entry point
// ============================================================
InputBackend* CreateRaylibInputBackend(Window* window) {
    return new RaylibInputBackend(window);
}
