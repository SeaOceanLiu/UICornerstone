#ifndef FocusManagerH
#define FocusManagerH

#include <vector>
#include "SColor.h"

class Control;

enum class FocusRingStyle {
    Solid,
    Dashed
};

class FocusManager {
public:
    FocusManager();
    ~FocusManager();

    void registerControl(Control* ctl);
    void unregisterControl(Control* ctl);

    bool focusNext(Control* current);
    bool focusPrev(Control* current);

    Control* getCurrentFocused() const { return m_currentFocused; }

    void registerBoundary(Control* boundary);

    void unregisterBoundary(Control* boundary);

    void notifyControlFocused(Control* ctl, bool byKeyboard);
    void clearFocus();
    bool focusControl(Control* ctl);
    bool focusNextScope();
    bool focusPrevScope();
    bool focusFirstInScope(Control* scope);

private:
    std::vector<Control*> m_controls;
    std::vector<Control*> m_boundaries;
    Control* m_currentFocused = nullptr;

    Control* findFocusScope(Control* ctl);
    bool isInScope(Control* candidate, Control* scope);
    bool isDescendantOf(Control* ancestor, Control* candidate);
};

#endif
