#include "Cursor.h"
#include "RaylibCompat.h"

class RaylibCursor : public Cursor {
public:
    explicit RaylibCursor(SystemCursorType type)
        : m_type(type), m_raylibType(mapType(type))
    {
    }

    int raylibType() const { return m_raylibType; }

private:
    static int mapType(SystemCursorType type) {
        switch (type) {
            case SystemCursorType::Default:    return MOUSE_CURSOR_DEFAULT;
            case SystemCursorType::Text:       return MOUSE_CURSOR_IBEAM;
            case SystemCursorType::Pointer:    return MOUSE_CURSOR_POINTING_HAND;
            case SystemCursorType::Crosshair:  return MOUSE_CURSOR_CROSSHAIR;
            case SystemCursorType::EW_Resize:  return MOUSE_CURSOR_RESIZE_EW;
            case SystemCursorType::NS_Resize:  return MOUSE_CURSOR_RESIZE_NS;
            case SystemCursorType::NWSE_Resize: return MOUSE_CURSOR_RESIZE_NWSE;
            case SystemCursorType::NESW_Resize: return MOUSE_CURSOR_RESIZE_NESW;
            case SystemCursorType::Move:       return MOUSE_CURSOR_RESIZE_ALL;
            case SystemCursorType::NotAllowed: return MOUSE_CURSOR_NOT_ALLOWED;
            // Fallback: use closest available or default
            case SystemCursorType::Wait:       return MOUSE_CURSOR_DEFAULT;
            case SystemCursorType::Progress:   return MOUSE_CURSOR_DEFAULT;
            case SystemCursorType::NW_Resize:  return MOUSE_CURSOR_RESIZE_NWSE;
            case SystemCursorType::N_Resize:   return MOUSE_CURSOR_RESIZE_NS;
            case SystemCursorType::NE_Resize:  return MOUSE_CURSOR_RESIZE_NESW;
            case SystemCursorType::E_Resize:   return MOUSE_CURSOR_RESIZE_EW;
            case SystemCursorType::SE_Resize:  return MOUSE_CURSOR_RESIZE_NWSE;
            case SystemCursorType::S_Resize:   return MOUSE_CURSOR_RESIZE_NS;
            case SystemCursorType::SW_Resize:  return MOUSE_CURSOR_RESIZE_NESW;
            case SystemCursorType::W_Resize:   return MOUSE_CURSOR_RESIZE_EW;
            default:                           return MOUSE_CURSOR_DEFAULT;
        }
    }

    SystemCursorType m_type;
    int m_raylibType;
};

static RaylibCursor s_defaultCursor(SystemCursorType::Default);

static Cursor* raylibCreateSystemCursor(SystemCursorType type) {
    return new RaylibCursor(type);
}

static Cursor* raylibGetDefaultCursor() {
    return &s_defaultCursor;
}

static void raylibSetCurrentCursor(Cursor* cursor) {
    if (!cursor) return;
    RaylibCursor* rlCursor = static_cast<RaylibCursor*>(cursor);
    SetMouseCursor(rlCursor->raylibType());
}

void RegisterRaylibCursorFactories() {
    Cursor::registerFactories(raylibCreateSystemCursor, raylibGetDefaultCursor, raylibSetCurrentCursor);
}
