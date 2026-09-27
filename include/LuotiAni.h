#ifndef LuotiAniH
#define LuotiAniH

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <math.h>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <map>

#include "nlohmann/json.hpp"
#include "Actor.h"
#include "Surface.h"
#include "Texture.h"
#include "Utility.h"
#include "PropertyNames.h"

using json = nlohmann::json;

inline uint64_t getTicks() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

class Operation{
public:
    enum OPERATION_TYPE: uint8_t{
        TRANSLATE,
        SCALE,
        ROTATE,
        OPACITY,
        VISIBLE,
        NULL_OPERATION
    };
    static OPERATION_TYPE strToOperationType(string str){
        if (str == PropertyNames::kOpTypeTranslate) return OPERATION_TYPE::TRANSLATE;
        else if (str == PropertyNames::kOpTypeScale) return OPERATION_TYPE::SCALE;
        else if (str == PropertyNames::kOpTypeRotate) return OPERATION_TYPE::ROTATE;
        else if (str == PropertyNames::kOpTypeOpacity) return OPERATION_TYPE::OPACITY;
        else if (str == PropertyNames::kOpTypeVisible) return OPERATION_TYPE::VISIBLE;
        else return OPERATION_TYPE::NULL_OPERATION;
    }
    static string operationTypeToStr(OPERATION_TYPE type){
        switch(type){
            case OPERATION_TYPE::TRANSLATE: return PropertyNames::kOpTypeTranslate;
            case OPERATION_TYPE::SCALE: return PropertyNames::kOpTypeScale;
            case OPERATION_TYPE::ROTATE: return PropertyNames::kOpTypeRotate;
            case OPERATION_TYPE::OPACITY: return PropertyNames::kOpTypeOpacity;
            case OPERATION_TYPE::VISIBLE: return PropertyNames::kOpTypeVisible;
            default: return PropertyNames::kOpTypeNull;
        }
    }
private:
    OPERATION_TYPE m_type;
    float m_param0;
    float m_param1;
    float m_param2;
public:
    OPERATION_TYPE getType(void) { return m_type; };
    float getP0(void) { return m_param0; };
    float getP1(void) { return m_param1; };
    float getP2(void) { return m_param2; };
    Operation():m_type(OPERATION_TYPE::NULL_OPERATION), m_param0(0), m_param1(0), m_param2(0) {}
    Operation(OPERATION_TYPE type, float p0, float p1=0, float p2=0):m_type(type), m_param0(p0), m_param1(p1), m_param2(p2) {};
    Operation(const Operation &other):m_type(other.m_type), m_param0(other.m_param0), m_param1(other.m_param1), m_param2(other.m_param2) {};
    Operation &operator=(const Operation &other){
        if (this != &other){
            m_type = other.m_type;
            m_param0 = other.m_param0;
            m_param1 = other.m_param1;
            m_param2 = other.m_param2;
        }
        return *this;
    }
    Operation(const Operation &&other):m_type(other.m_type), m_param0(other.m_param0), m_param1(other.m_param1), m_param2(other.m_param2) {};
    Operation &operator=(const Operation &&other){
        if (this != &other){
            m_type = other.m_type;
            m_param0 = other.m_param0;
            m_param1 = other.m_param1;
            m_param2 = other.m_param2;
        }
        return *this;
    }
};
class KeyFrame{
private:
    vector<shared_ptr<Operation>>m_operations;
public:
    KeyFrame(){};
    void addOperation(shared_ptr<Operation>operation){
        m_operations.push_back(operation);
    };
    shared_ptr<Operation> operator[](int index){
        if (index < 0 || index >= (int)m_operations.size()){
            return nullptr;
        }
        return m_operations[index];
    };
    size_t size(void) { return m_operations.size(); };
};
class Layer: public enable_shared_from_this<Layer>{
public:
    enum class LAYER_TYPE: uint8_t{
        IMAGE,
        SHAPE,
        TEXT,
        NULL_LAYER
    };
    static LAYER_TYPE strToLayerType(string str){
        if (str == PropertyNames::kLayerTypeImage) return LAYER_TYPE::IMAGE;
        else if (str == PropertyNames::kLayerTypeShape) return LAYER_TYPE::SHAPE;
        else if (str == PropertyNames::kLayerTypeText) return LAYER_TYPE::TEXT;
        else return LAYER_TYPE::NULL_LAYER;
    }
    static string layerTypeToStr(LAYER_TYPE type){
        switch(type){
            case LAYER_TYPE::IMAGE: return PropertyNames::kLayerTypeImage;
            case LAYER_TYPE::SHAPE: return PropertyNames::kLayerTypeShape;
            case LAYER_TYPE::TEXT: return PropertyNames::kLayerTypeText;
            default: return PropertyNames::kLayerTypeNull;
        }
    }
    static BlendMode blendModeStrToBlendMode(string str){
        if (str == PropertyNames::kBlendNormal) return BlendMode::None;
        else if (str == PropertyNames::kBlendAdditive) return BlendMode::Add;
        else if (str == PropertyNames::kBlendAdditivePremultiplied) return BlendMode::Add;
        else if (str == PropertyNames::kBlendModulate) return BlendMode::Mod;
        else if (str == PropertyNames::kBlendBlend) return BlendMode::Blend;
        else if (str == PropertyNames::kBlendBlendPremultiplied) return BlendMode::Blend;
        else if (str == PropertyNames::kBlendMultiply) return BlendMode::Mul;
        else return BlendMode::None;
    }
private:
    string m_name;
    LAYER_TYPE m_type;
    string m_src;
    SSize m_size;
    float m_opacity;
    BlendMode m_blendMode;

    map<uint32_t, shared_ptr<KeyFrame>>m_keyFrames;
public:
    Layer():
        m_name(""),
        m_type(LAYER_TYPE::NULL_LAYER),
        m_src(""),
        m_size(SSize(0,0)),
        m_opacity(1.0f),
        m_blendMode(BlendMode::None)
    {
    }
    shared_ptr<Layer> setName(string name){
        m_name = name;
        return shared_from_this();
    };
    shared_ptr<Layer> setType(LAYER_TYPE type){
        m_type = type;
        return shared_from_this();
    };
    shared_ptr<Layer> setSrc(string src){
        m_src = src;
        return shared_from_this();
    };
    shared_ptr<Layer> setSize(SSize size){
        m_size = size;
        return shared_from_this();
    };
    shared_ptr<Layer> setOpacity(float opacity){
        m_opacity = opacity;
        return shared_from_this();
    };
    shared_ptr<Layer> setBlendMode(BlendMode blendMode){
        m_blendMode = blendMode;
        return shared_from_this();
    };
    shared_ptr<Layer> addKeyFrame(uint32_t frameNumber, shared_ptr<KeyFrame> keyFrame){
        m_keyFrames[frameNumber] = keyFrame;
        return shared_from_this();
    };
    string getName(void) { return m_name; };
    LAYER_TYPE getType(void) { return m_type; };
    string getSrc(void) { return m_src; };
    SSize getSize(void) { return m_size; };
    float getOpacity(void) { return m_opacity; };
    BlendMode getBlendMode(void) { return m_blendMode; };
    shared_ptr<KeyFrame> operator[](uint32_t frameNumber) {
        if (m_keyFrames.find(frameNumber) != m_keyFrames.end()) {
            return m_keyFrames[frameNumber];
        } else {
            return nullptr;
        }
    };
    size_t size(void) { return m_keyFrames.size(); };
    uint32_t nextKeyFrameNumber(uint32_t currentKeyFrame) {
        auto it = m_keyFrames.upper_bound(currentKeyFrame);
        if (it != m_keyFrames.end()) {
            return it->first;
        } else {
            return 0;
        }
    };
};

class LuotiAni: public Material{
friend class LuotiAniBuilder;
public:
    class OpData{
    public:
        SRect dRect;
        SPoint translate;
        SMultipleSize m;
        float rotate;
        SPoint centerPos;
        uint8_t opacity;
        bool visible;

        SharedSurface surface;

        OpData():dRect(), translate(0,0), m(1, 1), rotate(0), centerPos({0, 0}), opacity(255), visible(true), surface(nullptr) {}
    };
    OpData getFrameOpData(uint32_t layer, uint32_t frame) const;
    SharedSurface getFrameCanvas(uint32_t frame) const;
private:
    struct SegmentInfo{
        int easeType;
        float eCx1, eCy1, eCx2, eCy2;
        int pathType;
        int bezierCubic;
        float p1, p2, p3, p4;
        float vx, vy;
        vector<SPoint> points;

        SegmentInfo():
            easeType(0),
            eCx1(0), eCy1(0), eCx2(0), eCy2(0),
            pathType(0),
            bezierCubic(0),
            p1(0), p2(0), p3(0), p4(0),
            vx(0), vy(0)
        {}
    };
private:
    int m_id;

    uint64_t m_lastFrameMsTick;
    uint64_t m_frameMSDuration;
    bool m_isLoaded;
    bool m_isPrepared;
    bool m_isPlaying;
    bool m_playAfterPrepare = false;   // 两阶段：prepare 前收到 playing=1 → 记录请求，prepare 完成后自动播放

    shared_ptr<char[]>m_pJsonFileContent;
    json m_jsonAniDesc;

    string m_version;
    string m_name;
    string m_resourceId;              // 内存资源引用：parse 阶段 provider 未就绪时记忆，挂树后由 setRenderDevice 补读
    string m_filePathStr;             // 文件路径设置原值（稳定存储，P0-21 读回；resource 路径清空）
    SSize m_canvasSize;
    uint16_t m_frameRate;
    uint32_t m_totalFrames;
    bool m_loop;

    uint32_t m_frameToDraw;

    vector<shared_ptr<Layer>>m_layers;

    vector<map<uint32_t, SegmentInfo>>m_layerSegs;
    vector<vector<OpData>>m_frameOpData;

    vector<shared_ptr<Actor>>m_frames;
    vector<SharedSurface> m_frameSurfaces;
    bool m_frameBilinearFilter;

    struct Matrix2D{
        float m[2][2];
    };
    static Matrix2D createRotationMatrix(float angle);
    static SPoint transformPoint(const Matrix2D *mat, SPoint point);
    static uint32_t getPixel(Surface *surface, int x, int y);
    static void setPixel(Surface *surface, int x, int y, uint32_t pixel);
    static uint32_t bilinearInterpolation(Surface *surface, float x, float y);
    SharedSurface getImageFromResource(string resourceId);
    OpData keyFrameToOpData(shared_ptr<KeyFrame> keyFrame, OpData srcOpData);

    static int parseEasing(const string& easing, SegmentInfo& segInfo);
    static int parsePath(const json& path, SegmentInfo& segInfo);
    static float easeValue(const SegmentInfo& segInfo, float t);
    static SPoint pathValue(const SegmentInfo& segInfo, SPoint start, SPoint end, float t);


public:
    LuotiAni(Control *parent, float xScale=1.0f, float yScale=1.0f):
        Material(parent, xScale, yScale),
        m_pJsonFileContent(nullptr),
        m_jsonAniDesc(nullptr),
        m_version("1.0"),
        m_name(""),
        m_canvasSize({0, 0}),
        m_frameRate(24),
        m_totalFrames(0),
        m_loop(false),
        m_lastFrameMsTick(0),
        m_frameMSDuration(0),
        m_isLoaded(false),
        m_isPrepared(false),
        m_isPlaying(false),
        m_frameBilinearFilter(false),
        m_id(-1)
    {
        m_ctlType = ControlType::Animation;
        setTransparent(true);   // P0-26：缺省无底色；显式设背景色自动取消透明
    }
    ~LuotiAni(){
        m_frameSurfaces.clear();
        m_frames.clear();
    };

    void loadFromFile(fs::path filePath) override;
    void loadFromResource(string resourceId) override;
    void loadAniDesc(fs::path filePath);
    void loadAniDesc(string resourceId);
    // 文件路径设置原值（稳定存储，供 getFilePathStr 读回；resource 路径清空；P0-21）
    const std::string& getFilePathStr() const { return m_filePathStr; }
    void parseJsonDesc();
    void update(void) override;
    void draw(float x=0, float y=0, uint8_t alpha=255) override;
    void draw(uint32_t frameNo, float x=0, float y=0, uint8_t alpha=255);
    void setRect(SRect rect) override;
    void play(void);
    void setRenderDevice(RenderDevice* device) override;
    void prepare(uint32_t startFrame = 0);
    void pause(void);
    void resume(void);
    void setFrame(uint32_t frame);
    Actor* getFrameActor(uint32_t frameNo);   // 帧 Actor 调试/断言访问（未 prepare 或越界返回 nullptr）
    bool isPlaying(void) { return m_isPlaying; };
    bool isPrepared(void) { return m_isPrepared; };
    bool isLoaded(void) { return m_isLoaded; };
    uint64_t getFrameDuration(){ return m_frameMSDuration; };
    uint32_t getTotalFrames(void) { return m_totalFrames; };
    uint32_t getCurrentFrame(void) { return m_frameToDraw; };
    bool isLoop(void) { return m_loop; };
    void setLoop(bool loop) { m_loop = loop; };

    // 帧位图纹理采样过滤开关（true=双线性，false=最近邻）；在 prepare/纹理创建时生效
    void setFrameFilter(bool bilinear) { m_frameBilinearFilter = bilinear; }
    bool getFrameFilter() const { return m_frameBilinearFilter; }

    // ── 属性系统重写（控件化 §6.3，分发惯例同 Button.cpp:335-353）──
    int setStringProperty(const char* prop, const char* value) override;  // "animation"
    int getStringProperty(const char* prop, const char*& out) override;   // "animation"（原值回读；P0-21 同型）
    int setBoolProperty(const char* prop, int value) override;            // "playing" / "loop"
    int setIntProperty(const char* prop, int value) override;             // "frame"
    int getBoolProperty(const char* prop, int& out) override;             // "playing" / "loop"
    int getIntProperty(const char* prop, int& out) override;              // "frame"
    bool isContainsPoint(float x, float y) override { return false; }     // 纯显示控件不参与事件命中
};

class LuotiAniBuilder{
private:
    shared_ptr<LuotiAni> m_luoAni;
public:
    LuotiAniBuilder(Control *parent, float xScale=1.0f, float yScale=1.0f):
        m_luoAni(nullptr)
    {
        m_luoAni = make_shared<LuotiAni>(parent, xScale, yScale);
    }
    LuotiAniBuilder& loadAniDesc(fs::path filePath);
    LuotiAniBuilder& loadAniDesc(string resourceId);
    LuotiAniBuilder& setRect(SRect rect);
    LuotiAniBuilder& prepare(uint32_t startFrame = 0);
    LuotiAniBuilder& setLoop(bool loop);
    LuotiAniBuilder& setAutoStart();
    shared_ptr<LuotiAni> build(void){
        return m_luoAni;
    }
};

class LuotiInstance: public Material{
private:
    uint64_t m_userId;
    bool m_isPlaying;
    uint32_t m_frameToDraw;
    uint64_t m_lastFrameMsTick;
    shared_ptr<LuotiAni> m_luoAni;
public:
    LuotiInstance(Control *parent, shared_ptr<LuotiAni> luoAni, uint64_t userId, float xScale=1.0f, float yScale=1.0f):
        Material(parent, xScale, yScale),
        m_userId(userId),
        m_isPlaying(false),
        m_frameToDraw(0),
        m_lastFrameMsTick(0),
        m_luoAni(luoAni)
    {}
    void loadFromFile(fs::path filePath) override {};
    void loadFromResource(string resourceId) override {};
    void update(void) override;
    void draw(float x=0, float y=0, uint8_t alpha=255) override;
    void play(void);
    uint64_t getUserId(void) { return m_userId; };
    bool isPlaying(void) { return m_isPlaying; };
};

#endif // LuotiAniH