#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "BackendPlugin.h"
#include "PlatformUtils.h"

// ============================================================
// Mode 1: Owned loop — delegates to the tick-based API below
// ============================================================
int MainWindow::run(AppCallbacks* app) {
    if (!init(app)) return 1;

    bool running = true;
    while (running) {
        if (!processEvents(app))
            break;
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
    inputBackend->newFrame();

    Event event;
    bool running = true;
    while (inputBackend->pollEvent(event)) {
        switch (event.m_type) {
            case EventType::WindowClose:
                running = false;
                break;
            case EventType::WindowResize:
                onWindowResized(event.resizeEvent.width, event.resizeEvent.height);
                BackendManager::instance()->window()->onResized(
                    event.resizeEvent.width, event.resizeEvent.height);
                BENCH->resized({0, 0, (float)event.resizeEvent.width, (float)event.resizeEvent.height});
                break;
            case EventType::WindowMoved:
                onWindowMoved(event.windowMoved.x, event.windowMoved.y);
                break;
            default:
                {
                    // Dispatch all events (new-style and old-style) to controls
                    auto sharedEvent = make_shared<Event>(event);
                    BENCH->inputControl(sharedEvent);
                }
                // Notify app of events via new-style API
                app->onEvent(event);
                break;
        }
    }
    // Check for programmatic quit request (e.g. from "Exit" menu item)
    if (m_quitRequested) {
        m_quitRequested = false;
        running = false;
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
