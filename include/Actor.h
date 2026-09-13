#ifndef ActorH
#define ActorH
#include "Material.h"
#include "Texture.h"

// 图片在目标矩形中的缩放方式
enum class ScaleType {
    STRETCH,      // 拉伸填满整个矩形（默认，可能变形）
    FIT_CENTER,   // 保持宽高比居中适配，不裁剪
    CENTER_CROP,  // 保持宽高比填满矩形，裁剪溢出部分
    NONE,         // 保持原始尺寸，居中显示
    TILE          // 按纹理原尺寸平铺填满目标矩形（末行/末列边缘裁剪；v1.1.1）
};

class Actor: public Material{
friend class ActorBuilder;
private:
    uint8_t m_alpha = 255;
    bool m_explicitSize = false;   // setRect 显式设置过尺寸（w/h>0）→ 换图不再跟随自然尺寸
protected:
    bool m_matchParentRect; //是否强制使用目标矩形
    ScaleType m_scaleType;
    SRect m_sourceRect{0, 0, 0, 0};   // 源矩形（纹理子区域，空=整图）
    fs::path m_filePath;        // 延迟加载（两阶段创建）：挂树前保存加载参数
    string m_resourceId;
public:
    Actor(Control *parent, float xScale=1.0f, float yScale=1.0f);
    Actor(Control *parent, bool matchParentRect=false, float xScale=1.0f, float yScale=1.0f);
    Actor(Control *parent, fs::path filePath, bool matchParentRect=false, float xScale=1.0f, float yScale=1.0f);
    Actor(Control *parent, string resourceId, bool matchParentRect=false, float xScale=1.0f, float yScale=1.0f);
    Actor(const Actor& other);
    Actor& operator=(const Actor& other);
    void setRect(SRect rect) override;   // 记录显式尺寸（w/h>0），供纹理加载时判断是否跟随自然尺寸
    void loadFromFile(fs::path filePath) override;
    void loadFromResource(string resourceId) override;
    void create() override;   // 两阶段创建：context 就绪后由 setContext 触发加载
    void setParent(Control *parent) override;   // 由于要考虑匹配父控件绘图区域大小，所以需要重载该函数，以使其在设备父控件时匹配父控件绘图区域大小
    void loadTextureFromSurface(Surface* surface);
    Texture* getTexture() const { return m_texture.get(); }
    void setTexture(SharedTexture texture) { m_texture = texture; }

    using Material::draw;
    void draw(float posx, float posy, uint8_t alpha=255) override;

    void setScaleType(ScaleType type) { m_scaleType = type; }
    ScaleType getScaleType() const { return m_scaleType; }

    // 源矩形（纹理子区域；空 = 整图）。对 STRETCH 与 TILE 生效
    // （FIT_CENTER/CENTER_CROP/NONE 忽略，v1.1.1）
    void setSourceRect(SRect src) { m_sourceRect = src; }
    SRect getSourceRect() const { return m_sourceRect; }
    void clearSourceRect() { m_sourceRect = {}; }

    void setMatchParentRect(bool match) { m_matchParentRect = match; }
    void setAlpha(uint8_t alpha) { m_alpha = alpha; }
    uint8_t getAlpha() const { return m_alpha; }

    // 纯显示控件不参与事件命中与遮挡检测（Image 控件语义）
    bool isContainsPoint(float x, float y) override { return false; }

    // 属性系统重写（C ABI 属性分发，实现见 src/Actor.cpp）
    int setStringProperty(const char* prop, const char* value) override;  // "image" / "image-resource"
    int setBoolProperty(const char* prop, int value) override;            // "match-parent-rect"
    int setIntProperty(const char* prop, int value) override;             // "alpha"
    int setFloatProperty(const char* prop, float value) override;         // "anchor-x" / "anchor-y"
    int setEnumProperty(const char* prop, const char* value) override;    // "scale-type" / "anchor"
    int getEnumProperty(const char* prop, const char*& out) override;     // "scale-type" / "anchor"
    int getBoolProperty(const char* prop, int& out) override;             // "match-parent-rect"
    int getIntProperty(const char* prop, int& out) override;              // "alpha"
    int getFloatProperty(const char* prop, float& out) override;          // "anchor-x" / "anchor-y"
    // image/image-resource 只写不读（m_filePath 为 fs::path，string() 临时对象会悬垂）

    void draw(void) override;   // 使用成员 alpha（原走 Material::draw 默认 255）
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