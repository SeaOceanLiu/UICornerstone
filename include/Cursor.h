#ifndef CursorH
#define CursorH

enum class SystemCursorType {
    Default,
    Text,
    Pointer,
    Wait,
    Crosshair,
    Progress,
    NWSE_Resize,
    NESW_Resize,
    EW_Resize,
    NS_Resize,
    Move,
    NotAllowed,
    NW_Resize,
    N_Resize,
    NE_Resize,
    E_Resize,
    SE_Resize,
    S_Resize,
    SW_Resize,
    W_Resize,
};

class Cursor {
public:
    virtual ~Cursor() = default;
    static Cursor* createSystem(SystemCursorType type);
    static Cursor* getDefault();
    static void setCurrent(Cursor* cursor);
};

#endif // CursorH
