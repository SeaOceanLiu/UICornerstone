#include "Cursor.h"
#include <SDL3/SDL_mouse.h>

class SDLCursor : public Cursor {
    SDL_Cursor* m_cursor;
    bool m_owned;
public:
    SDLCursor(SDL_Cursor* cursor, bool owned) : m_cursor(cursor), m_owned(owned) {}
    ~SDLCursor() override {
        if (m_owned && m_cursor) {
            SDL_DestroyCursor(m_cursor);
        }
    }
    SDL_Cursor* get() const { return m_cursor; }
};

Cursor* Cursor::createSystem(SystemCursorType type) {
    static const SDL_SystemCursor mapping[] = {
        SDL_SYSTEM_CURSOR_DEFAULT,
        SDL_SYSTEM_CURSOR_TEXT,
        SDL_SYSTEM_CURSOR_POINTER,
        SDL_SYSTEM_CURSOR_WAIT,
        SDL_SYSTEM_CURSOR_CROSSHAIR,
        SDL_SYSTEM_CURSOR_PROGRESS,
        SDL_SYSTEM_CURSOR_NWSE_RESIZE,
        SDL_SYSTEM_CURSOR_NESW_RESIZE,
        SDL_SYSTEM_CURSOR_EW_RESIZE,
        SDL_SYSTEM_CURSOR_NS_RESIZE,
        SDL_SYSTEM_CURSOR_MOVE,
        SDL_SYSTEM_CURSOR_NOT_ALLOWED,
        SDL_SYSTEM_CURSOR_NW_RESIZE,
        SDL_SYSTEM_CURSOR_N_RESIZE,
        SDL_SYSTEM_CURSOR_NE_RESIZE,
        SDL_SYSTEM_CURSOR_E_RESIZE,
        SDL_SYSTEM_CURSOR_SE_RESIZE,
        SDL_SYSTEM_CURSOR_S_RESIZE,
        SDL_SYSTEM_CURSOR_SW_RESIZE,
        SDL_SYSTEM_CURSOR_W_RESIZE,
    };
    int idx = static_cast<int>(type);
    if (idx < 0 || idx >= static_cast<int>(sizeof(mapping) / sizeof(mapping[0]))) {
        return nullptr;
    }
    SDL_Cursor* sdlCursor = SDL_CreateSystemCursor(mapping[idx]);
    if (!sdlCursor) return nullptr;
    return new SDLCursor(sdlCursor, true);
}

Cursor* Cursor::getDefault() {
    static SDLCursor defaultCursor(SDL_GetCursor(), false);
    return &defaultCursor;
}

void Cursor::setCurrent(Cursor* cursor) {
    if (!cursor) return;
    SDLCursor* sdlCursor = dynamic_cast<SDLCursor*>(cursor);
    if (sdlCursor && sdlCursor->get()) {
        SDL_SetCursor(sdlCursor->get());
    }
}
