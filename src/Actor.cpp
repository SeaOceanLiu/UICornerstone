#include "Actor.h"

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
    m_surface = IMG_Load(filePath.string().c_str());
    if (m_surface == nullptr) {
        SDL_Log("LoadFromFile Error: %s\n", SDL_GetError());
        return;
    }

    loadTextureFromSurface(m_surface);
}

void Actor::loadFromResource(string resourceId) {
    shared_ptr<Resource> resource = ResourceLoader::getInstance()->getResource(resourceId);
    if (resource == nullptr || (resource->resourceType != ResourceLoader::RT_IMAGES
                            && resource->resourceType != ResourceLoader::RT_ANIMATIONS
                            && resource->resourceType != ResourceLoader::RT_BACKGROUND)
        || resource->pMem == nullptr) {

        SDL_Log("LoadFromResource Error: '%s' is not an images\n", resourceId.c_str());
        return;
    }

    SDL_IOStream *resourceStream = SDL_IOFromConstMem(resource->pMem.get(), resource->resourceSize);
    if( resourceStream == nullptr){
        SDL_Log("Create IOStream Error: %s\n", SDL_GetError());
        return;
    }
    m_surface = IMG_Load_IO(resourceStream, true);  // 这里传递了true给closeio参数，所以完成图片载入后，会自动关闭resourceStream
    if (m_surface == nullptr) {
        SDL_Log("loadFromResource Error: %s\n", SDL_GetError());
        return;
    }

    loadTextureFromSurface(m_surface);
}

void Actor::loadTextureFromSurface(SDL_Surface *surface) {
    m_rect.left = 0;
    m_rect.top = 0;
    if (!m_matchParentRect) {
        m_rect.width = (float)surface->w;
        m_rect.height = (float)surface->h;
    } else {
        m_rect.width = getParent()->getRect().width;
        m_rect.height = getParent()->getRect().height;
    }

    m_texture = getRenderDevice()->createTextureFromSDLSurface(surface);
    if (!m_texture) {
        SDL_Log("createTextureFromSDLSurface failed\n");
        return;
    }
}

void Actor::setParent(Control *parent){
    Material::setParent(parent);

    if (m_matchParentRect) {
        m_rect.width = getParent()->getRect().width;
        m_rect.height = getParent()->getRect().height;

        if (m_surface != nullptr) {
            m_texture = getRenderDevice()->createTextureFromSDLSurface(m_surface);
            if (!m_texture) {
                SDL_Log("createTextureFromSDLSurface failed\n");
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
