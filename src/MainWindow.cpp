#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "BackendPlugin.h"

// ============================================================
// Mode 1: Owned loop — delegates to the tick-based API below
// ============================================================
int MainWindow::run(AppCallbacks* app) {
    if (!init(app)) return 1;

    bool running = true;
    while (running) {
        running = processEvents(app);
        if (!running) break;
        update(app);
        render(app);
    }

    shutdown(app);
    return 0;
}

// ============================================================
// Mode 2: Tick-based API
// ============================================================
bool MainWindow::init(AppCallbacks* app) {
    return app->onInit();
}

bool MainWindow::processEvents(AppCallbacks* app) {
    auto* backend = BackendManager::instance();
    auto* inputBackend = backend->inputBackend();

    Event event;
    bool running = true;
    while (inputBackend->pollEvent(event)) {
        switch (event.m_type) {
            case EventType::WindowClose:
                running = false;
                break;
            case EventType::WindowResize:
                onWindowResized(event.resizeEvent.width, event.resizeEvent.height);
                BENCH->resized({0, 0, (float)event.resizeEvent.width, (float)event.resizeEvent.height});
                break;
            case EventType::WindowMoved:
                onWindowMoved(event.windowMoved.x, event.windowMoved.y);
                break;
            default:
                // Old-style dispatch (dual-run backward compatibility)
                if (event.m_eventName != static_cast<EventName>(0)) {
                    auto sharedEvent = make_shared<Event>(event);
                    BENCH->inputControl(sharedEvent);
                }
                // New-style event notification for app-specific handling
                app->onEvent(event);
                break;
        }
    }
    return running;
}

void MainWindow::update(AppCallbacks* app) {
    app->onUpdate();
}

void MainWindow::render(AppCallbacks* app) {
    app->onRender();
    BackendManager::instance()->renderDevice()->present();
}

void MainWindow::shutdown(AppCallbacks* app) {
    app->onQuit();
}
