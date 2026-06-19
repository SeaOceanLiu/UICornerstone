#include "CallbackAdapters.h"
#include "StateMachine.h"
#include <cstring>
#include <cstdio>
#include <string>

// ============================================================
// CallbackWindow
// ============================================================
CallbackWindow::CallbackWindow(const UIBackendCallbacks* cbs, UIWindowHandle handle)
    : m_handle(handle), m_cbs(cbs) {}
CallbackWindow::~CallbackWindow() {
    if (m_handle && m_cbs->destroyWindow) m_cbs->destroyWindow(m_handle);
}

SSize CallbackWindow::getSize() const {
    float w = 0, h = 0;
    if (m_cbs->getWindowSize) m_cbs->getWindowSize(m_handle, &w, &h);
    return SSize{w, h};
}
SPoint CallbackWindow::getPosition() const {
    float x = 0, y = 0;
    if (m_cbs->getWindowPosition) m_cbs->getWindowPosition(m_handle, &x, &y);
    return SPoint{x, y};
}
float CallbackWindow::getDisplayWidth() const {
    return m_cbs->getDisplayWidth ? m_cbs->getDisplayWidth(m_handle) : 0;
}
float CallbackWindow::getDisplayHeight() const {
    return m_cbs->getDisplayHeight ? m_cbs->getDisplayHeight(m_handle) : 0;
}
float CallbackWindow::getDpiScale() const {
    return m_cbs->getDpiScale ? m_cbs->getDpiScale(m_handle) : 1.0f;
}
void CallbackWindow::setTitle(const std::string& title) {
    if (m_cbs->setWindowTitle) m_cbs->setWindowTitle(m_handle, title.c_str());
}
void* CallbackWindow::nativeHandle() {
    return m_handle;
}
RenderDevice* CallbackWindow::renderDevice() {
    return nullptr; // 由 BackendManager 单独管理
}
bool CallbackWindow::getMousePosition(float& x, float& y) {
    return m_cbs->getMousePosition ? m_cbs->getMousePosition(m_handle, &x, &y) : false;
}

// ============================================================
// CallbackTexture
// ============================================================
CallbackTexture::CallbackTexture(const UIBackendCallbacks* cbs, UIRenderDeviceHandle dev, UITextureHandle tex)
    : m_deviceHandle(dev), m_cbs(cbs), m_handle(tex) {}
CallbackTexture::~CallbackTexture() {
    if (m_handle && m_cbs->destroyTexture) m_cbs->destroyTexture(m_deviceHandle, m_handle);
}
int CallbackTexture::width() const {
    if (m_cachedW < 0 && m_cbs->getTextureSize) m_cbs->getTextureSize(m_handle, &m_cachedW, &m_cachedH);
    return m_cachedW;
}
int CallbackTexture::height() const {
    if (m_cachedH < 0 && m_cbs->getTextureSize) m_cbs->getTextureSize(m_handle, &m_cachedW, &m_cachedH);
    return m_cachedH;
}
void CallbackTexture::setBlendMode(BlendMode) {}
void CallbackTexture::setAlphaMod(uint8_t) {}
BlendMode CallbackTexture::getBlendMode() const { return BlendMode::None; }
uint8_t CallbackTexture::getAlphaMod() const { return 255; }

// ============================================================
// CallbackRenderDevice
// ============================================================
CallbackRenderDevice::CallbackRenderDevice(const UIBackendCallbacks* cbs, UIRenderDeviceHandle handle)
    : m_handle(handle), m_cbs(cbs) {}
CallbackRenderDevice::~CallbackRenderDevice() {
    if (m_cbs->destroyRenderDevice) m_cbs->destroyRenderDevice(m_handle);
}

void CallbackRenderDevice::setDrawColor(SColor color) {
    m_cbs->setDrawColor(m_handle, UIColor{color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()});
}
void CallbackRenderDevice::setBlendMode(BlendMode mode) {
    if (m_cbs->setBlendMode) m_cbs->setBlendMode(m_handle, static_cast<int>(mode));
}
void CallbackRenderDevice::setClipRect(const SRect& rect) {
    if (m_cbs->setClipRect) m_cbs->setClipRect(m_handle, rect.left, rect.top, rect.width, rect.height);
}
void CallbackRenderDevice::clearClipRect() {
    if (m_cbs->clearClipRect) m_cbs->clearClipRect(m_handle);
}
void CallbackRenderDevice::fillRect(const SRect& rect) {
    m_cbs->fillRect(m_handle, rect.left, rect.top, rect.width, rect.height);
}
void CallbackRenderDevice::drawRect(const SRect& rect) {
    m_cbs->drawRect(m_handle, rect.left, rect.top, rect.width, rect.height);
}
void CallbackRenderDevice::drawLine(float x1, float y1, float x2, float y2) {
    m_cbs->drawLine(m_handle, x1, y1, x2, y2);
}
void CallbackRenderDevice::drawPoint(float x, float y) {
    m_cbs->drawPoint(m_handle, x, y);
}

void CallbackRenderDevice::drawTriangles(const Vertex* vertices, int count) {
    for (int i = 0; i + 2 < count; i += 3)
        drawTriangle(vertices[i].x, vertices[i].y, vertices[i+1].x, vertices[i+1].y, vertices[i+2].x, vertices[i+2].y, vertices[i].color);
}
void CallbackRenderDevice::drawTriangleStrip(const Vertex* vertices, int count) {
    for (int i = 0; i + 2 < count; i++)
        drawTriangle(vertices[i].x, vertices[i].y, vertices[i+1].x, vertices[i+1].y, vertices[i+2].x, vertices[i+2].y, vertices[i].color);
}
void CallbackRenderDevice::drawTriangleFan(const Vertex* vertices, int count) {
    for (int i = 1; i + 1 < count; i++)
        drawTriangle(vertices[0].x, vertices[0].y, vertices[i].x, vertices[i].y, vertices[i+1].x, vertices[i+1].y, vertices[0].color);
}

void CallbackRenderDevice::drawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, SColor color) {
    UIColor uc{color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()};
    if (m_cbs->fillTriangle) {
        m_cbs->fillTriangle(m_handle, x0, y0, x1, y1, x2, y2, uc);
    } else {
        setDrawColor(color);
        drawLine(x0, y0, x1, y1);
        drawLine(x1, y1, x2, y2);
        drawLine(x2, y2, x0, y0);
    }
}
void CallbackRenderDevice::drawQuad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, SColor color) {
    UIColor uc{color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()};
    if (m_cbs->fillQuad) {
        m_cbs->fillQuad(m_handle, x0, y0, x1, y1, x2, y2, x3, y3, uc);
    } else {
        // fallback: two triangles
        drawTriangle(x0, y0, x1, y1, x2, y2, color);
        drawTriangle(x0, y0, x2, y2, x3, y3, color);
    }
}

SharedTexture CallbackRenderDevice::createTextureFromFile(const std::string& path) {
    if (!m_cbs->createTextureFromFile) return nullptr;
    UITextureHandle tex = m_cbs->createTextureFromFile(m_handle, path.c_str());
    if (!tex) return nullptr;
    return std::make_shared<CallbackTexture>(m_cbs, m_handle, tex);
}
SharedTexture CallbackRenderDevice::createTextureFromSurface(Surface*) { return nullptr; }
SharedTexture CallbackRenderDevice::createRenderTexture(int, int) { return nullptr; }
void CallbackRenderDevice::destroyTexture(Texture* texture) {
    auto* ct = dynamic_cast<CallbackTexture*>(texture);
    if (ct && ct->handle() && m_cbs->destroyTexture)
        m_cbs->destroyTexture(m_handle, ct->handle());
}
void CallbackRenderDevice::drawTexture(Texture* texture, const SRect* srcRect, const SRect* dstRect) {
    if (!m_cbs->drawTexture || !texture || !dstRect) {
        return;
    }
    auto* ct = dynamic_cast<CallbackTexture*>(texture);

    UIRect src = srcRect ? UIRect{srcRect->left, srcRect->top, srcRect->width, srcRect->height} : UIRect{0,0,0,0};
    UIRect dst = UIRect{dstRect->left, dstRect->top, dstRect->width, dstRect->height};

    if (ct && ct->handle()) {
        m_cbs->drawTexture(m_handle, ct->handle(), srcRect ? &src : nullptr, &dst);
    } else {
        // Non-CallbackTexture (e.g. SDL3Texture from inline backend):
        // wrap in a temporary non-owning shared_ptr so the bridge can use it
        std::shared_ptr<Texture> tempHolder(texture, [](Texture*){});
        UITextureHandle h = reinterpret_cast<UITextureHandle>(&tempHolder);
        m_cbs->drawTexture(m_handle, h, srcRect ? &src : nullptr, &dst);
    }
}
void CallbackRenderDevice::drawTextureRotated(Texture* texture, const SRect* srcRect, const SRect* dstRect, float) {
    drawTexture(texture, srcRect, dstRect);
}
void CallbackRenderDevice::setRenderTarget(Texture*) {}
void CallbackRenderDevice::resetRenderTarget() {}
void CallbackRenderDevice::readPixels(void*, const SRect&) {}
void CallbackRenderDevice::clear() {
    m_cbs->clear(m_handle, UIColor{0,0,0,255});
}
void CallbackRenderDevice::present() {
    m_cbs->present(m_handle);
}
void CallbackRenderDevice::flush() {
    if (m_cbs->flush) m_cbs->flush(m_handle);
}
void* CallbackRenderDevice::getNativeHandle() {
    return m_cbs->getNativeHandle ? m_cbs->getNativeHandle(m_handle) : nullptr;
}

// ============================================================
// CallbackInputBackend
// ============================================================
CallbackInputBackend::CallbackInputBackend(const UIBackendCallbacks* cbs, UIInputBackendHandle handle)
    : m_handle(handle), m_cbs(cbs) {}
CallbackInputBackend::~CallbackInputBackend() {
    if (m_cbs->destroyInputBackend) m_cbs->destroyInputBackend(m_handle);
}
void CallbackInputBackend::newFrame() {
    if (m_cbs->newFrame) m_cbs->newFrame(m_handle);
}
void CallbackInputBackend::startTextInput() {
    m_textInputActive = true;
    if (m_cbs->startTextInput) m_cbs->startTextInput(m_handle);
}
void CallbackInputBackend::stopTextInput() {
    m_textInputActive = false;
    if (m_cbs->stopTextInput) m_cbs->stopTextInput(m_handle);
}
void CallbackInputBackend::setClipboardText(const std::string& text) {
    if (m_cbs->setClipboardText) m_cbs->setClipboardText(m_handle, text.c_str());
}
std::string CallbackInputBackend::getClipboardText() const {
    if (!m_cbs->getClipboardText) return {};
    char buf[4096];
    int len = m_cbs->getClipboardText(m_handle, buf, (int)sizeof(buf));
    return len > 0 ? std::string(buf, (size_t)len) : "";
}

bool CallbackInputBackend::pollEvent(Event& event) {
    if (!m_cbs->pollEvent) return false;
    UIEvent ue;
    if (!m_cbs->pollEvent(m_handle, &ue)) return false;

    event = Event();
    event.customInt = 0;
    event.customPtr = nullptr;

    switch (ue.type) {
    case UI_EVENT_MOUSE_MOVE: {
        event.m_type = EventType::MouseMove;
        event.mousePos = EventMousePos{UI_EVENT_MOUSE_X(&ue), UI_EVENT_MOUSE_Y(&ue)};
        break;
    }
    case UI_EVENT_MOUSE_DOWN:
    case UI_EVENT_MOUSE_UP: {
        event.m_type = (ue.type == UI_EVENT_MOUSE_DOWN) ? EventType::MouseDown : EventType::MouseUp;
        event.mouseButton = EventMouseButton{
            UI_EVENT_MOUSE_X(&ue), UI_EVENT_MOUSE_Y(&ue),
            static_cast<MouseButton>(UI_EVENT_BUTTON(&ue))
        };
        break;
    }
    case UI_EVENT_MOUSE_WHEEL: {
        event.m_type = EventType::MouseWheel;
        event.mouseWheel = EventMouseWheel{0, 0, 0, UI_EVENT_WHEEL_DELTA(&ue)};
        break;
    }
    case UI_EVENT_KEY_DOWN:
    case UI_EVENT_KEY_UP: {
        event.m_type = (ue.type == UI_EVENT_KEY_DOWN) ? EventType::KeyDown : EventType::KeyUp;
        event.keyEvent = EventKey{static_cast<KeyCode>(UI_EVENT_KEY_CODE(&ue)), static_cast<KeyMod>(UI_EVENT_KEY_MOD(&ue)), 0, false};
        break;
    }
    case UI_EVENT_TEXT_INPUT: {
        event.m_type = EventType::TextInput;
        strncpy(event.textInput.text, UI_EVENT_TEXT(&ue), 31);
        event.textInput.text[31] = '\0';
        break;
    }
    case UI_EVENT_WINDOW_RESIZE: {
        event.m_type = EventType::WindowResize;
        event.resizeEvent = EventResize{UI_EVENT_RESIZE_W(&ue), UI_EVENT_RESIZE_H(&ue)};
        break;
    }
    case UI_EVENT_WINDOW_CLOSE:
        event.m_type = EventType::WindowClose;
        break;
    case UI_EVENT_FOCUS_GAINED:
        event.m_type = EventType::FocusGained;
        event.focusEvent = EventFocus{true};
        break;
    case UI_EVENT_FOCUS_LOST:
        event.m_type = EventType::FocusLost;
        event.focusEvent = EventFocus{false};
        break;
    default:
        return false;
    }
    return true;
}

KeyMod CallbackInputBackend::getModState() {
    if (!m_cbs->getModState) return KeyMod::None;
    return static_cast<KeyMod>(m_cbs->getModState(m_handle));
}

// ============================================================
// CallbackFont (内部辅助类)
// ============================================================
class CallbackFont : public Font {
    UITextRendererHandle m_trHandle;
    const UIBackendCallbacks* m_cbs;
    UIFontHandle m_handle;
    float m_size;
public:
    CallbackFont(const UIBackendCallbacks* cbs, UITextRendererHandle tr, UIFontHandle h, float size)
        : m_trHandle(tr), m_cbs(cbs), m_handle(h), m_size(size) {}
    ~CallbackFont() override {
        if (m_handle && m_cbs->destroyFont) m_cbs->destroyFont(m_trHandle, m_handle);
    }
    int getSize() const override { return static_cast<int>(m_size); }
    UIFontHandle handle() const { return m_handle; }
};

// ============================================================
// CallbackTextRenderer
// ============================================================
CallbackTextRenderer::CallbackTextRenderer(const UIBackendCallbacks* cbs, UITextRendererHandle handle, UIRenderDeviceHandle dev)
    : m_handle(handle), m_cbs(cbs), m_deviceHandle(dev) {}
CallbackTextRenderer::~CallbackTextRenderer() {
    if (m_cbs->destroyTextRenderer) m_cbs->destroyTextRenderer(m_handle);
}

SharedFont CallbackTextRenderer::loadFont(const std::string& path, int size) {
    if (!m_cbs->loadFont) return nullptr;
    UIFontHandle fh = m_cbs->loadFont(m_handle, path.c_str(), static_cast<float>(size));
    if (!fh) return nullptr;
    return std::make_shared<CallbackFont>(m_cbs, m_handle, fh, static_cast<float>(size));
}
SharedFont CallbackTextRenderer::loadFontFromMemory(const void* data, size_t len, int size) {
    if (!m_cbs->loadFontFromMemory) return nullptr;
    UIFontHandle fh = m_cbs->loadFontFromMemory(m_handle, data, static_cast<int>(len), static_cast<float>(size));
    if (!fh) return nullptr;
    return std::make_shared<CallbackFont>(m_cbs, m_handle, fh, static_cast<float>(size));
}
int CallbackTextRenderer::getFontHeight(Font* font) {
    auto* cf = dynamic_cast<CallbackFont*>(font);
    if (!cf || !m_cbs->getFontHeight) return 0;
    return static_cast<int>(m_cbs->getFontHeight(m_handle, cf->handle()));
}
// Internal cached text handle: stores (font, text) for delegation to backend
struct CachedText {
    CallbackFont* font;
    std::string   text;
};
void* CallbackTextRenderer::createText(Font* font, const std::string& text) {
    auto* cf = dynamic_cast<CallbackFont*>(font);
    if (!cf) return nullptr;
    auto* ct = new CachedText{cf, text};
    return ct;
}
void CallbackTextRenderer::destroyText(void* h) {
    delete static_cast<CachedText*>(h);
}
SSize CallbackTextRenderer::measureText(void* h) {
    auto* ct = static_cast<CachedText*>(h);
    if (!ct || !m_cbs->measureTextWidth) return SSize{0,0};
    float w = m_cbs->measureTextWidth(m_handle, ct->font->handle(), ct->text.c_str());
    float hh = m_cbs->getFontHeight ? m_cbs->getFontHeight(m_handle, ct->font->handle()) : 0;
    return SSize{w, hh};
}
void CallbackTextRenderer::drawText(void* h, float x, float y, SColor color) {
    auto* ct = static_cast<CachedText*>(h);
    if (!ct || !m_cbs->drawText) return;
    m_cbs->drawText(m_handle, ct->font->handle(), ct->text.c_str(), x, y,
                     UIColor{color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()});
}
void CallbackTextRenderer::drawText(void* h, float x, float y, float wrapWidth, SColor color) {
    auto* ct = static_cast<CachedText*>(h);
    if (!ct || !m_cbs->drawTextWrapped) return;
    m_cbs->drawTextWrapped(m_handle, ct->font->handle(), ct->text.c_str(), x, y, wrapWidth,
                            UIColor{color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()});
}

SSize CallbackTextRenderer::measureText(Font* font, const std::string& text) {
    auto* cf = dynamic_cast<CallbackFont*>(font);
    if (!cf || !m_cbs->measureTextWidth) return SSize{0,0};
    float w = m_cbs->measureTextWidth(m_handle, cf->handle(), text.c_str());
    float h = m_cbs->getFontHeight ? m_cbs->getFontHeight(m_handle, cf->handle()) : 0;
    return SSize{w, h};
}
void CallbackTextRenderer::drawText(Font* font, const std::string& text, float x, float y, SColor color) {
    auto* cf = dynamic_cast<CallbackFont*>(font);
    if (!cf || !m_cbs->drawText) return;
    m_cbs->drawText(m_handle, cf->handle(), text.c_str(), x, y,
                     UIColor{color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()});
}
void CallbackTextRenderer::drawText(Font* font, const std::string& text, float x, float y, float wrapWidth, SColor color) {
    auto* cf = dynamic_cast<CallbackFont*>(font);
    if (!cf || !m_cbs->drawTextWrapped) return;
    m_cbs->drawTextWrapped(m_handle, cf->handle(), text.c_str(), x, y, wrapWidth,
                            UIColor{color.redByte(), color.greenByte(), color.blueByte(), color.alphaByte()});
}

// ============================================================
// CallbackResourceProvider
// ============================================================
CallbackResourceProvider::CallbackResourceProvider(const UIBackendCallbacks* cbs, UIResourceProviderHandle handle)
    : m_handle(handle), m_cbs(cbs) {}
CallbackResourceProvider::~CallbackResourceProvider() {
    if (m_cbs->destroyResourceProvider) m_cbs->destroyResourceProvider(m_handle);
}

std::shared_ptr<std::vector<char>> CallbackResourceProvider::readFile(const std::string& path) {
    if (!m_cbs->readFile) return nullptr;
    auto buf = std::make_shared<std::vector<char>>();
    buf->resize(1024 * 1024); // 1MB max
    int len = m_cbs->readFile(m_handle, path.c_str(), buf->data(), static_cast<int>(buf->size()));
    if (len <= 0) return nullptr;
    buf->resize(len);
    return buf;
}
bool CallbackResourceProvider::exists(const std::string& path) {
    return m_cbs->fileExists ? m_cbs->fileExists(m_handle, path.c_str()) != 0 : false;
}
