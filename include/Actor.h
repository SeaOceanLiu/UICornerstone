#ifndef ActorH
#define ActorH
#include "Material.h"
#include "Texture.h"
#include "ResourceLoader.h"

// 图片在目标矩形中的缩放方式
enum class ScaleType {
    STRETCH,      // 拉伸填满整个矩形（默认，可能变形）
    FIT_CENTER,   // 保持宽高比居中适配，不裁剪
    CENTER_CROP,  // 保持宽高比填满矩形，裁剪溢出部分
    NONE          // 保持原始尺寸，居中显示
};

class Actor: public Material{
friend class ActorBuilder;
private:
protected:
    bool m_matchParentRect; //是否强制使用目标矩形
    ScaleType m_scaleType;
public:
    Actor(Control *parent, float xScale=1.0f, float yScale=1.0f);
    Actor(Control *parent, bool matchParentRect=false, float xScale=1.0f, float yScale=1.0f);
    Actor(Control *parent, fs::path filePath, bool matchParentRect=false, float xScale=1.0f, float yScale=1.0f);
    Actor(Control *parent, string resourceId, bool matchParentRect=false, float xScale=1.0f, float yScale=1.0f);
    Actor(const Actor& other);
    Actor& operator=(const Actor& other);
    void loadFromFile(fs::path filePath) override;
    void loadFromResource(string resourceId) override;
    void setParent(Control *parent) override;   // 由于要考虑匹配父控件绘图区域大小，所以需要重载该函数，以使其在设备父控件时匹配父控件绘图区域大小
    void loadTextureFromSurface(Surface* surface);
    Texture* getTexture() const { return m_texture.get(); }
    void setTexture(SharedTexture texture) { m_texture = texture; }

    using Material::draw;
    void draw(float posx, float posy, uint8_t alpha=255) override;

    void setScaleType(ScaleType type) { m_scaleType = type; }
    ScaleType getScaleType() const { return m_scaleType; }
};

class ActorBuilder{
private:
    shared_ptr<Actor> m_actor;
public:
    ActorBuilder(Control *parent, float xScale=1.0f, float yScale=1.0f);
    ActorBuilder& loadFromFile(fs::path filePath);
    ActorBuilder& setMatchParentRect(bool matchParentRect);
    ActorBuilder& setScaleType(ScaleType type);
    shared_ptr<Actor> build(void);
};
#endif