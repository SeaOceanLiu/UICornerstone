#include "Actor.h"
#include "Surface.h"
#include "PlatformUtils.h"

Actor::Actor(Control *parent, float xScale, float yScale):
    Material(parent, xScale, yScale),
    m_matchParentRect(false),
    m_scaleType(ScaleType::STRETCH)
{
    setParent(parent);
}

Actor::Actor(Control *parent, bool matchParentRect, float xScale, float yScale):
    Material(parent, xScale, yScale),
    m_matchParentRect(matchParentRect),
    m_scaleType(ScaleType::STRETCH)
{
    // setParent(parent);
}

Actor::Actor(Control *parent, fs::path filePath, bool matchParentRect, float xScale, float yScale):
    Material(parent, filePath, xScale, yScale),
    m_matchParentRect(matchParentRect),
    m_scaleType(ScaleType::STRETCH)
{
    setParent(parent);
    loadFromFile(filePath);
}

Actor::Actor(Control *parent, string resourceId, bool matchParentRect, float xScale, float yScale):
    Material(parent, resourceId, xScale, yScale),
    m_matchParentRect(matchParentRect),
    m_scaleType(ScaleType::STRETCH)
{
    setParent(parent);
    loadFromResource(resourceId);
}

Actor::Actor(const Actor& other):
    Material(other),
    m_matchParentRect(other.m_matchParentRect),
    m_scaleType(other.m_scaleType)
{
}

Actor& Actor::operator=(const Actor& other) {
    Material::operator=(other);
    m_matchParentRect = other.m_matchParentRect;
    m_scaleType = other.m_scaleType;
    return *this;
}

void Actor::loadFromFile(fs::path filePath) {
    m_surface = Surface::loadFromFile(filePath.string());
    if (!m_surface) {
        Platform::Log("LoadFromFile Error\n");
        return;
    }

    loadTextureFromSurface(m_surface.get());
}

void Actor::loadFromResource(string resourceId) {
    ResourceProvider* provider = getResourceProvider();
    if (provider == nullptr) {
        Platform::Log("Actor::loadFromResource: No resource provider\n");
        return;
    }

    shared_ptr<vector<char>> imageData = provider->readFile(resourceId);
    if (imageData == nullptr || imageData->empty()) {
        Platform::Log("Actor::loadFromResource: '%s' not found\n", resourceId.c_str());
        return;
    }

    m_surface = Surface::loadFromMemory(imageData->data(), imageData->size());
    if (!m_surface) {
        Platform::Log("Actor::loadFromResource Error\n");
        return;
    }

    loadTextureFromSurface(m_surface.get());
}

void Actor::loadTextureFromSurface(Surface* surface) {
    m_rect.left = 0;
    m_rect.top = 0;
    if (!m_matchParentRect) {
        m_rect.width = (float)surface->width();
        m_rect.height = (float)surface->height();
    } else {
        m_rect.width = getParent()->getRect().width;
        m_rect.height = getParent()->getRect().height;
    }

    m_texture = surface->createTexture(getRenderDevice());
    if (!m_texture) {
        Platform::Log("Surface::createTexture failed\n");
        return;
    }
}

void Actor::setParent(Control *parent){
    Material::setParent(parent);

    if (m_matchParentRect) {
        m_rect.width = getParent()->getRect().width;
        m_rect.height = getParent()->getRect().height;

        if (m_surface) {
            m_texture = m_surface->createTexture(getRenderDevice());
            if (!m_texture) {
                Platform::Log("Surface::createTexture failed\n");
                return;
            }
        }
    }
}

void Actor::draw(float posx, float posy, Uint8 alpha) {
    inheritRenderer();
    if (!m_texture) return;

    SRect targetRect = getRect();
    targetRect.left = posx - m_anchorPoint.x;
    targetRect.top = posy - m_anchorPoint.y;

    SRect drawRect = mapToDrawRect(targetRect);

    m_texture->setBlendMode(BlendMode::Blend);
    m_texture->setAlphaMod(alpha);

    float texW = (float)m_texture->width();
    float texH = (float)m_texture->height();
    if (texW <= 0 || texH <= 0) return;

    if (m_scaleType == ScaleType::STRETCH) {
        getRenderDevice()->drawTexture(m_texture.get(), nullptr, &drawRect);
        return;
    }

    switch (m_scaleType) {
        case ScaleType::FIT_CENTER: {
            float scale = min(drawRect.width / texW, drawRect.height / texH);
            float w = texW * scale;
            float h = texH * scale;
            SRect fitRect(
                drawRect.left + (drawRect.width - w) * 0.5f,
                drawRect.top + (drawRect.height - h) * 0.5f,
                w, h
            );
            getRenderDevice()->drawTexture(m_texture.get(), nullptr, &fitRect);
            break;
        }
        case ScaleType::CENTER_CROP: {
            float scale = max(drawRect.width / texW, drawRect.height / texH);
            float cropW = drawRect.width / scale;
            float cropH = drawRect.height / scale;
            SRect srcRect(
                (texW - cropW) * 0.5f,
                (texH - cropH) * 0.5f,
                cropW, cropH
            );
            getRenderDevice()->drawTexture(m_texture.get(), &srcRect, &drawRect);
            break;
        }
        case ScaleType::NONE: {
            SRect naturalRect(
                drawRect.left + (drawRect.width - texW) * 0.5f,
                drawRect.top + (drawRect.height - texH) * 0.5f,
                texW, texH
            );
            getRenderDevice()->drawTexture(m_texture.get(), nullptr, &naturalRect);
            break;
        }
        default:
            break;
    }
}

ActorBuilder::ActorBuilder(Control *parent, float xScale, float yScale):
    m_actor(nullptr)
{
    m_actor = make_shared<Actor>(parent, xScale, yScale);
}
ActorBuilder& ActorBuilder::loadFromFile(fs::path filePath){
    m_actor->loadFromFile(filePath);
    return *this;
}
ActorBuilder& ActorBuilder::setMatchParentRect(bool matchParentRect){
    m_actor->m_matchParentRect = matchParentRect;
    return *this;
}
ActorBuilder& ActorBuilder::setScaleType(ScaleType type){
    m_actor->m_scaleType = type;
    return *this;
}
shared_ptr<Actor> ActorBuilder::build(void){
    return m_actor;
}
