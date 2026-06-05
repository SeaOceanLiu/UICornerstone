#ifndef EVENTTYPES_H
#define EVENTTYPES_H

#include <cstdint>

// Event type enum - cross-DLL safe, no std::any dependency
enum class EventType : uint8_t {
    None,
    MouseMove, MouseDown, MouseUp, MouseWheel,
    KeyDown, KeyUp, TextInput,
    WindowResize, WindowClose,
    FocusGained, FocusLost,
};

// Event data structs - all trivial types for union safety
struct EventMousePos     { float x, y; };
struct EventMouseWheel   { float x, y; float scrollX, scrollY; };
struct EventKey          { int keycode; int mod; bool repeat; };
struct EventTextInput    { char text[32]; };
struct EventResize       { int width, height; };
struct EventFocus        { bool focused; };

#endif // EVENTTYPES_H