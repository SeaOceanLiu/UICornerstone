#ifndef BenchH
#define BenchH
#include "MainWindow.h"
#include "Panel.h"
#include "Label.h"
#include "WinFrame.h"
#include "LuotiAni.h"

#define BENCH (Bench::getInstance())

class Bench: public Panel, public TopControl
{
    using OnInitialHandler = std::function<void (void)>;
protected:
private:
    bool m_isLoading;
    bool m_isInitialed;
    SRect m_defaultArenaRect;
    SRect m_defaultBGRect;

    float m_N; // multiple of Arena
    float m_M; // multiple of BG

    uint64_t m_nextTick;
    unordered_map<EventName, uint64_t> m_eventJitter; // jitter for each event
    uint64_t m_nextRepeatTick;
    shared_ptr<Event> m_lastAction;
    int m_isExiting;
    OnInitialHandler m_onInitial;

    Bench(Control *parent, SRect rect, float xScale=1.0f, float yScale=1.0f);

public:
    static Bench* getInstance(void){
        static Bench instance = Bench(nullptr, {0, 0, INITIAL_WIDTH, INITIAL_HEIGHT}); // 静态局部变量，程序运行期间只会被初始化一次
        instance.create(); // 确保在第一次获取实例时调用create方法进行初始化
        return &instance;
    }
    void initial(void);
    void inputControl(shared_ptr<Event> event);
    void repeatTrigger(void);
    void draw(void) override;
    void update(void) override;
    int isExiting(void);

    void setOnInitial(OnInitialHandler handler);

    void resized(SRect newRect) override;
    void addControl(shared_ptr<Control> control) override;
};
#endif // BenchH