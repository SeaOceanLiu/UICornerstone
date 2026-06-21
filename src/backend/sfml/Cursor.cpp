#include "Cursor.h"
#include <SFML/Window/Cursor.hpp>
#include <SFML/Window/Window.hpp>
#include <optional>

class SFMLCursor : public Cursor {
    std::optional<sf::Cursor> m_cursor;
    bool m_hasCursor;
public:
    SFMLCursor() : m_hasCursor(false) {}
    SFMLCursor(sf::Cursor&& cursor) : m_cursor(std::move(cursor)), m_hasCursor(true) {}
    bool hasCursor() const { return m_hasCursor; }
    const sf::Cursor* get() const { return m_hasCursor ? &*m_cursor : nullptr; }
};

static sf::Window* g_cursorWindow = nullptr;

void SFMLSetCursorWindow(sf::Window* window) {
    g_cursorWindow = window;
}

static sf::Cursor::Type systemCursorTypeToSFML(SystemCursorType type) {
    switch (type) {
        case SystemCursorType::Default:     return sf::Cursor::Type::Arrow;
        case SystemCursorType::Text:        return sf::Cursor::Type::Text;
        case SystemCursorType::Pointer:     return sf::Cursor::Type::Hand;
        case SystemCursorType::Wait:        return sf::Cursor::Type::Wait;
        case SystemCursorType::Crosshair:   return sf::Cursor::Type::Cross;
        case SystemCursorType::Progress:    return sf::Cursor::Type::ArrowWait;
        case SystemCursorType::NWSE_Resize: return sf::Cursor::Type::SizeTopLeftBottomRight;
        case SystemCursorType::NESW_Resize: return sf::Cursor::Type::SizeBottomLeftTopRight;
        case SystemCursorType::EW_Resize:   return sf::Cursor::Type::SizeHorizontal;
        case SystemCursorType::NS_Resize:   return sf::Cursor::Type::SizeVertical;
        case SystemCursorType::Move:        return sf::Cursor::Type::SizeAll;
        case SystemCursorType::NotAllowed:  return sf::Cursor::Type::NotAllowed;
        case SystemCursorType::NW_Resize:   return sf::Cursor::Type::SizeTopLeft;
        case SystemCursorType::N_Resize:    return sf::Cursor::Type::SizeTop;
        case SystemCursorType::NE_Resize:   return sf::Cursor::Type::SizeTopRight;
        case SystemCursorType::E_Resize:    return sf::Cursor::Type::SizeRight;
        case SystemCursorType::SE_Resize:   return sf::Cursor::Type::SizeBottomRight;
        case SystemCursorType::S_Resize:    return sf::Cursor::Type::SizeBottom;
        case SystemCursorType::SW_Resize:   return sf::Cursor::Type::SizeBottomLeft;
        case SystemCursorType::W_Resize:    return sf::Cursor::Type::SizeLeft;
    }
    return sf::Cursor::Type::Arrow;
}

static Cursor* sfmlCreateSystemCursor(SystemCursorType type) {
    auto optCursor = sf::Cursor::createFromSystem(systemCursorTypeToSFML(type));
    if (!optCursor) {
        return nullptr;
    }
    return new SFMLCursor(std::move(*optCursor));
}

static Cursor* sfmlGetDefaultCursor() {
    static SFMLCursor defaultCursor(
        std::move(*sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow))
    );
    return &defaultCursor;
}

static void sfmlSetCurrentCursor(Cursor* cursor) {
    if (!cursor || !g_cursorWindow) return;
    SFMLCursor* sfmlCursor = dynamic_cast<SFMLCursor*>(cursor);
    if (sfmlCursor && sfmlCursor->get()) {
        g_cursorWindow->setMouseCursor(*sfmlCursor->get());
    }
}

void RegisterSFMLCursorFactories() {
    Cursor::registerFactories(sfmlCreateSystemCursor, sfmlGetDefaultCursor, sfmlSetCurrentCursor);
}
