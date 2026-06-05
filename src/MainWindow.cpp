#include "MainWindow.h"
#include "Bench.h"
#include "AppCallbacks.h"
#include "BackendPlugin.h"

int MainWindow::run(AppCallbacks* app) {
    if (!app->onInit()) return 1;

    auto* backend = BackendManager::instance();
    auto* inputBackend = backend->inputBackend();
    auto* renderDevice = backend->renderDevice();

    bool running = true;
    while (running) {
        Event event;
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
                    if (event.m_eventName != static_cast<EventName>(0)) {
                        auto sharedEvent = make_shared<Event>(event);
                        BENCH->inputControl(sharedEvent);
                    }
                    break;
            }
        }
        if (!running) break;

        app->onUpdate();
        app->onRender();
        renderDevice->present();
    }

    app->onQuit();
    return 0;
}
