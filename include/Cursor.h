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

// Factory function pointer types — set by backend plugin
class Cursor;
using CursorCreateSystemFn = Cursor*(*)(SystemCursorType type);
using CursorGetDefaultFn   = Cursor*(*)();
using CursorSetCurrentFn   = void(*)(Cursor* cursor);

class Cursor {
public:
    virtual ~Cursor() = default;
    static Cursor* createSystem(SystemCursorType type);
    static Cursor* getDefault();
    static void setCurrent(Cursor* cursor);

    // Backend plugin registration (called from UIBackend_xxx.dll during init)
    // Exported via CORE_API so backend plugin can call it
#ifndef UICORNERSTONE_CORE_API_DEFINED
#define UICORNERSTONE_CORE_API_DEFINED
#if defined(UICORNERSTONE_CORE_API_EXPORT)
  #define CORE_API __declspec(dllexport)
#elif defined(UICORNERSTONE_BUILD_SHARED)
  #define CORE_API __declspec(dllimport)
#else
  #define CORE_API
#endif
#endif
    static CORE_API void registerFactories(CursorCreateSystemFn c, CursorGetDefaultFn g, CursorSetCurrentFn s);
};

#endif // CursorH
