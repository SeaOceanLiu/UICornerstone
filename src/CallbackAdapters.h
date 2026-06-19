#ifndef CALLBACKADAPTERS_H
#define CALLBACKADAPTERS_H

#include "UICornerstoneAPI.h"
#include "Window.h"
#include "RenderDevice.h"
#include "InputBackend.h"
#include "TextRenderer.h"
#include "ResourceProvider.h"
#include "Font.h"
#include "Texture.h"
#include "EventTypes.h"

// ============================================================
// CallbackAdapter 层：将 UIBackendCallbacks 包装为 C++ 抽象接口
// ============================================================

// ---------- CallbackWindow ----------
class CallbackWindow : public Window {
    UIWindowHandle m_handle;
    const UIBackendCallbacks* m_cbs;
public:
    CallbackWindow(const UIBackendCallbacks* cbs, UIWindowHandle handle);
    ~CallbackWindow() override;
    SSize getSize() const override;
    SPoint getPosition() const override;
    float getDisplayWidth() const override;
    float getDisplayHeight() const override;
    float getDpiScale() const override;
    void setTitle(const std::string& title) override;
    void* nativeHandle() override;
    RenderDevice* renderDevice() override;
    bool getMousePosition(float& x, float& y) override;
};

// ---------- CallbackTexture ----------
class CallbackTexture : public Texture {
    UIRenderDeviceHandle m_deviceHandle;
    const UIBackendCallbacks* m_cbs;
    UITextureHandle m_handle;
    mutable int m_cachedW = -1;
    mutable int m_cachedH = -1;
public:
    CallbackTexture(const UIBackendCallbacks* cbs, UIRenderDeviceHandle dev, UITextureHandle tex);
    ~CallbackTexture() override;
    int width() const override;
    int height() const override;
    void setBlendMode(BlendMode mode) override;
    void setAlphaMod(uint8_t alpha) override;
    BlendMode getBlendMode() const override;
    uint8_t getAlphaMod() const override;
    UITextureHandle handle() const { return m_handle; }
};

// ---------- CallbackRenderDevice ----------
class CallbackRenderDevice : public RenderDevice {
    UIRenderDeviceHandle m_handle;
    const UIBackendCallbacks* m_cbs;
public:
    CallbackRenderDevice(const UIBackendCallbacks* cbs, UIRenderDeviceHandle handle);
    ~CallbackRenderDevice() override;
    void setDrawColor(SColor color) override;
    void setBlendMode(BlendMode mode) override;
    void setClipRect(const SRect& rect) override;
    void clearClipRect() override;
    void fillRect(const SRect& rect) override;
    void drawRect(const SRect& rect) override;
    void drawLine(float x1, float y1, float x2, float y2) override;
    void drawPoint(float x, float y) override;
    void drawTriangles(const Vertex* vertices, int count) override;
    void drawTriangleStrip(const Vertex* vertices, int count) override;
    void drawTriangleFan(const Vertex* vertices, int count) override;
    void drawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, SColor color) override;
    void drawQuad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, SColor color) override;
    SharedTexture createTextureFromFile(const std::string& path) override;
    SharedTexture createTextureFromSurface(Surface* surface) override;
    SharedTexture createRenderTexture(int width, int height) override;
    void destroyTexture(Texture* texture) override;
    void drawTexture(Texture* texture, const SRect* srcRect, const SRect* dstRect) override;
    void drawTextureRotated(Texture* texture, const SRect* srcRect, const SRect* dstRect, float angle) override;
    void setRenderTarget(Texture* texture) override;
    void resetRenderTarget() override;
    void readPixels(void* buffer, const SRect& rect) override;
    void clear() override;
    void present() override;
    void flush() override;
    void* getNativeHandle() override;
    UIRenderDeviceHandle handle() const { return m_handle; }
};

// ---------- CallbackInputBackend ----------
class CallbackInputBackend : public InputBackend {
    UIInputBackendHandle m_handle;
    const UIBackendCallbacks* m_cbs;
public:
    CallbackInputBackend(const UIBackendCallbacks* cbs, UIInputBackendHandle handle);
    ~CallbackInputBackend() override;
    void newFrame() override;
    void startTextInput() override;
    void stopTextInput() override;
    bool isTextInputActive() const override { return m_textInputActive; }
    void setClipboardText(const std::string& text) override;
    std::string getClipboardText() const override;
    bool hasScreenKeyboard() const override { return false; }
    bool pollEvent(Event& event) override;
    KeyMod getModState() override;
    mutable bool m_textInputActive = false;
};

// ---------- CallbackTextRenderer ----------
class CallbackTextRenderer : public TextRenderer {
    UITextRendererHandle m_handle;
    const UIBackendCallbacks* m_cbs;
    UIRenderDeviceHandle m_deviceHandle;
public:
    CallbackTextRenderer(const UIBackendCallbacks* cbs, UITextRendererHandle handle, UIRenderDeviceHandle dev);
    ~CallbackTextRenderer() override;
    SharedFont loadFont(const std::string& path, int size) override;
    SharedFont loadFontFromMemory(const void* data, size_t len, int size) override;
    int getFontHeight(Font* font) override;
    void* createText(Font* font, const std::string& text) override;
    void destroyText(void* text) override;
    SSize measureText(void* text) override;
    void drawText(void* text, float x, float y, SColor color) override;
    void drawText(void* text, float x, float y, float wrapWidth, SColor color) override;
    SSize measureText(Font* font, const std::string& text) override;
    void drawText(Font* font, const std::string& text, float x, float y, SColor color) override;
    void drawText(Font* font, const std::string& text, float x, float y, float wrapWidth, SColor color) override;
};

// ---------- CallbackResourceProvider ----------
class CallbackResourceProvider : public ResourceProvider {
    UIResourceProviderHandle m_handle;
    const UIBackendCallbacks* m_cbs;
public:
    CallbackResourceProvider(const UIBackendCallbacks* cbs, UIResourceProviderHandle handle);
    ~CallbackResourceProvider() override;
    std::shared_ptr<std::vector<char>> readFile(const std::string& path) override;
    bool exists(const std::string& path) override;
};

#endif // CALLBACKADAPTERS_H
