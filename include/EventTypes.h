#ifndef EVENTTYPES_H
#define EVENTTYPES_H

#include <cstdint>

// Event type enum - cross-DLL safe, no std::any dependency
enum class EventType : uint8_t {
    None,
    MouseMove, MouseDown, MouseUp, MouseWheel,
    KeyDown, KeyUp, TextInput,
    WindowResize, WindowMoved, WindowClose,
    FocusGained, FocusLost,
};

// ========== Canonical KeyCode ==========
// Printable characters (0x20-0x7E) use ASCII values — universal across all backends.
// Extended keys (cursors, F-keys, etc.) use our own range starting at 0x100.
// Each InputBackend maps its backend-specific keycodes to these canonical values.
enum class KeyCode : uint16_t {
    Unknown = 0,

    // ASCII control codes (commonly used)
    Return = 0x0D,
    Escape = 0x1B,
    Backspace = 0x08,
    Tab = 0x09,

    // ASCII printable (0x20-0x7E)
    Space       = 0x20,
    Exclaim     = 0x21,
    DblApostrophe = 0x22,
    Hash        = 0x23,
    Dollar      = 0x24,
    Percent     = 0x25,
    Ampersand   = 0x26,
    Apostrophe  = 0x27,
    LeftParen   = 0x28,
    RightParen  = 0x29,
    Asterisk    = 0x2A,
    Plus        = 0x2B,
    Comma       = 0x2C,
    Minus       = 0x2D,
    Period      = 0x2E,
    Slash       = 0x2F,
    Num0 = 0x30, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Colon       = 0x3A,
    Semicolon   = 0x3B,
    Less        = 0x3C,
    Equals      = 0x3D,
    Greater     = 0x3E,
    Question    = 0x3F,
    At          = 0x40,
    A = 0x41, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket  = 0x5B,
    Backslash    = 0x5C,
    RightBracket = 0x5D,
    Caret       = 0x5E,
    Underscore  = 0x5F,
    Grave       = 0x60,
    LeftBrace   = 0x7B,
    Bar         = 0x7C,
    RightBrace  = 0x7D,
    Tilde       = 0x7E,
    Del         = 0x7F,

    // ========== Extended keys (our own range) ==========
    // Cursor keys
    Left = 0x100, Right, Up, Down,
    Home, End, PageUp, PageDown, Insert,

    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,

    // Modifier keys
    LShift, RShift, LCtrl, RCtrl, LAlt, RAlt, LGui, RGui,

    // Lock keys
    CapsLock, NumLock, ScrollLock,

    // Keypad
    KP0, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
    KPDecimal, KPDivide, KPMultiply, KPMinus, KPPlus, KPEnter, KPEquals,

    // Print / Pause
    PrintScreen, Pause,

    // Application
    Application, Menu,

    // International
    Intl1, Intl2, Intl3, Intl4, Intl5, Intl6, Intl7, Intl8, Intl9,

    // Media
    AudioMute, AudioVolDown, AudioVolUp,
    AudioPlay, AudioStop, AudioPrev, AudioNext,

    // Browser
    BrowserBack, BrowserForward, BrowserRefresh, BrowserStop,
    BrowserSearch, BrowserFavorites, BrowserHome,

    // System
    Power, Sleep, WakeUp,

    // Misc
    Help, Undo, Redo, Cut, Copy, Paste, Find,
    Mail, MediaSelect,
    BrightnessDown, BrightnessUp,
};

// ========== Canonical KeyMod (bitmask) ==========
// Values match SDL_Keymod for zero-cost SDL3InputBackend mapping.
enum class KeyMod : uint16_t {
    None    = 0x0000,
    LShift  = 0x0001,
    RShift  = 0x0002,
    LCtrl   = 0x0040,
    RCtrl   = 0x0080,
    LAlt    = 0x0100,
    RAlt    = 0x0200,
    LGui    = 0x0400,
    RGui    = 0x0800,
    NumLock = 0x1000,
    CapsLock = 0x2000,
    AltGr   = 0x4000,

    Shift = LShift | RShift,
    Ctrl  = LCtrl | RCtrl,
    Alt   = LAlt | RAlt,
    Gui   = LGui | RGui,
};

inline KeyMod operator|(KeyMod a, KeyMod b) {
    return static_cast<KeyMod>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
inline KeyMod operator&(KeyMod a, KeyMod b) {
    return static_cast<KeyMod>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}
inline bool isModSet(KeyMod value, KeyMod mask) {
    return (static_cast<uint16_t>(value) & static_cast<uint16_t>(mask)) != 0;
}

// ========== Canonical MouseButton ==========
// Values match SDL button numbering for zero-cost SDL3InputBackend mapping.
enum class MouseButton : uint8_t {
    None   = 0,
    Left   = 1,
    Right  = 2,
    Middle = 3,
    X1     = 4,
    X2     = 5,
};

// ========== Event data structs (trivial / union-friendly) ==========
struct EventMousePos     { float x, y; };
struct EventMouseWheel   { float x, y; float scrollX, scrollY; };
struct EventKey          { KeyCode keycode; KeyMod mod; bool repeat; };
struct EventTextInput    { char text[32]; };
struct EventResize       { int width, height; };
struct EventWindowMoved  { int x, y; };
struct EventFocus        { bool focused; };

#endif // EVENTTYPES_H
