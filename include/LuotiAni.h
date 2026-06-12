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
#include "MainWindow.h"
#include "Actor.h"
#include "Surface.h"
#include "Texture.h"
#include "Utility.h"

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
        if (str == "translate") return OPERATION_TYPE::TRANSLATE;
        else if (str == "scale") return OPERATION_TYPE::SCALE;
        else if (str == "rotate") return OPERATION_TYPE::ROTATE;
        else if (str == "opacity") return OPERATION_TYPE::OPACITY;
        else if (str == "visible") return OPERATION_TYPE::VISIBLE;
        else return OPERATION_TYPE::NULL_OPERATION;
    }
    static string operationTypeToStr(OPERATION_TYPE type){
        switch(type){
            case OPERATION_TYPE::TRANSLATE: return "translate";
            case OPERATION_TYPE::SCALE: return "scale";
            case OPERATION_TYPE::ROTATE: return "rotate";
            case OPERATION_TYPE::OPACITY: return "opacity";
            case OPERATION_TYPE::VISIBLE: return "visible";
            default: return "null_operation";
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
        if (str == "image") return LAYER_TYPE::IMAGE;
        else if (str == "shape") return LAYER_TYPE::SHAPE;
        else if (str == "text") return LAYER_TYPE::TEXT;
        else return LAYER_TYPE::NULL_LAYER;
    }
    static string layerTypeToStr(LAYER_TYPE type){
        switch(type){
            case LAYER_TYPE::IMAGE: return "image";
            case LAYER_TYPE::SHAPE: return "shape";
            case LAYER_TYPE::TEXT: return "text";
            default: return "null_layer";
        }
    }
    static BlendMode blendModeStrToBlendMode(string str){
        if (str == "normal") return BlendMode::None;
        else if (str == "additive") return BlendMode::Add;
        else if (str == "additivePremultiplied") return BlendMode::Add;
        else if (str == "modulate") return BlendMode::Mod;
        else if (str == "blend") return BlendMode::Blend;
        else if (str == "blendPremultiplied") return BlendMode::Blend;
        else if (str == "multiply") return BlendMode::Mul;
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
private:
    int m_id;

    uint64_t m_lastFrameMsTick;
    uint64_t m_frameMSDuration;
    bool m_isLoaded;
    bool m_isPrepared;
    bool m_isPlaying;

    shared_ptr<char[]>m_pJsonFileContent;
    json m_jsonAniDesc;

    string m_version;
    string m_name;
    SSize m_canvasSize;
    uint16_t m_frameRate;
    uint32_t m_totalFrames;
    bool m_loop;

    uint32_t m_frameToDraw;

    vector<shared_ptr<Layer>>m_layers;

    vector<shared_ptr<Actor>>m_frames;
    vector<SharedSurface> m_frameSurfaces;

    struct Matrix2D{
        float m[2][2];
    };
    static Matrix2D createRotationMatrix(float angle) {
        Matrix2D mat;
        float rad = angle * M_PI / 180.0f;
        float cos_angle = cosf(rad);
        float sin_angle = sinf(rad);

        mat.m[0][0] = cos_angle;
        mat.m[0][1] = -sin_angle;
        mat.m[1][0] = sin_angle;
        mat.m[1][1] = cos_angle;

        return mat;
    }
    static SPoint transformPoint(const Matrix2D *mat, SPoint point) {
        return SPoint{
            mat->m[0][0] * point.x + mat->m[0][1] * point.y,
            mat->m[1][0] * point.x + mat->m[1][1] * point.y
        };
    }

    static uint32_t getPixel(Surface *surface, int x, int y) {
        if (!surface || x < 0 || x >= surface->width() || y < 0 || y >= surface->height()) {
            return 0;
        }
        return static_cast<uint32_t*>(surface->pixels())[y * surface->width() + x];
    }

    static void setPixel(Surface *surface, int x, int y, uint32_t pixel) {
        if (!surface || x < 0 || x >= surface->width() || y < 0 || y >= surface->height()) {
            return;
        }
        static_cast<uint32_t*>(surface->pixels())[y * surface->width() + x] = pixel;
    }

    static uint32_t bilinearInterpolation(Surface *surface, float x, float y) {
        int x1 = std::floor(x);
        int y1 = std::floor(y);
        int x2 = x1 + 1;
        int y2 = y1 + 1;

        int w = surface->width();
        int h = surface->height();
        x1 = std::clamp(x1, 0, w - 1);
        y1 = std::clamp(y1, 0, h - 1);
        x2 = std::clamp(x2, 0, w - 1);
        y2 = std::clamp(y2, 0, h - 1);

        uint32_t p11 = getPixel(surface, x1, y1);
        uint32_t p12 = getPixel(surface, x1, y2);
        uint32_t p21 = getPixel(surface, x2, y1);
        uint32_t p22 = getPixel(surface, x2, y2);

        uint8_t* p11b = reinterpret_cast<uint8_t*>(&p11);
        uint8_t* p12b = reinterpret_cast<uint8_t*>(&p12);
        uint8_t* p21b = reinterpret_cast<uint8_t*>(&p21);
        uint8_t* p22b = reinterpret_cast<uint8_t*>(&p22);

        uint8_t r11 = p11b[0], g11 = p11b[1], b11 = p11b[2], a11 = p11b[3];
        uint8_t r12 = p12b[0], g12 = p12b[1], b12 = p12b[2], a12 = p12b[3];
        uint8_t r21 = p21b[0], g21 = p21b[1], b21 = p21b[2], a21 = p21b[3];
        uint8_t r22 = p22b[0], g22 = p22b[1], b22 = p22b[2], a22 = p22b[3];

        float dx = x - x1;
        float dy = y - y1;

        float r = (1 - dx) * (1 - dy) * r11 + dx * (1 - dy) * r21 + (1 - dx) * dy * r12 + dx * dy * r22;
        float g = (1 - dx) * (1 - dy) * g11 + dx * (1 - dy) * g21 + (1 - dx) * dy * g12 + dx * dy * g22;
        float b = (1 - dx) * (1 - dy) * b11 + dx * (1 - dy) * b21 + (1 - dx) * dy * b12 + dx * dy * b22;
        float a = (1 - dx) * (1 - dy) * a11 + dx * (1 - dy) * a21 + (1 - dx) * dy * a12 + dx * dy * a22;

        uint32_t result = 0;
        uint8_t* rb = reinterpret_cast<uint8_t*>(&result);
        rb[0] = static_cast<uint8_t>(r);
        rb[1] = static_cast<uint8_t>(g);
        rb[2] = static_cast<uint8_t>(b);
        rb[3] = static_cast<uint8_t>(a);
        return result;
    }

    SharedSurface getImageFromResource(string resourceId){
        ResourceProvider* provider = getResourceProvider();
        if (provider == nullptr) {
            printf("LuotiAni::getImageFromResource: No resource provider\n");
            throw "LuotiAni::getImageFromResource: No resource provider";
        }

        shared_ptr<vector<char>> imageData = provider->readFile(resourceId);
        if (imageData == nullptr || imageData->empty()) {
            printf("LuotiAni::getImageFromResource Error: '%s' not found\n", resourceId.c_str());
            throw "LuotiAni::getImageFromResource Error: resource not found";
        }

        SharedSurface surface = Surface::loadFromMemory(imageData->data(), imageData->size());
        if (surface == nullptr) {
            printf("LuotiAni::getImageFromResource Error: loadFromMemory failed.\n");
            throw "LuotiAni::getImageFromResource Error: loadFromMemory failed.";
        }
        return surface;
    }

    OpData keyFrameToOpData(shared_ptr<KeyFrame> keyFrame, OpData srcOpData){
        if (keyFrame == nullptr) throw "LuotiAni::keyFrameToOpData: No keyframe to convert.";

        OpData opData = srcOpData;

        for (size_t o = 0; o < keyFrame->size(); o++) {
            shared_ptr<Operation> operation = (*keyFrame)[o];
            if (operation == nullptr) continue;

            switch (operation->getType()) {
                case Operation::OPERATION_TYPE::TRANSLATE:
                    opData.translate = opData.translate + SPoint(operation->getP0(), operation->getP1());
                    break;
                case Operation::OPERATION_TYPE::SCALE:
                    opData.m = opData.m * SMultipleSize(operation->getP0(), operation->getP1());
                    break;
                case Operation::OPERATION_TYPE::ROTATE:
                    opData.rotate += operation->getP0();
                    opData.centerPos = {operation->getP1(), operation->getP2()};
                    break;
                case Operation::OPERATION_TYPE::OPACITY:
                    opData.opacity = (uint8_t)(operation->getP0() * 255 / 100);
                    break;
                case Operation::OPERATION_TYPE::VISIBLE:
                    opData.visible = operation->getP0() > 0 ? true : false;
                    break;
                default:
                    break;
            }
        }
        return opData;
    }


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
        m_id(-1)
    {
    }
    ~LuotiAni(){
        m_frameSurfaces.clear();
        m_frames.clear();
    };

    void loadFromFile(fs::path filePath) override {
        loadAniDesc(filePath);
    };
    void loadFromResource(string resourceId) override {
        loadAniDesc(resourceId);
    };
    void loadAniDesc(fs::path filePath){
        FILE* stream = fopen(filePath.string().c_str(), "rb");
        if (!stream) {
            printf("Open aniDesc json file error\n");
            throw "Open aniDesc json file error";
            return;
        }
        fseek(stream, 0, SEEK_END);
        long iFileLen = ftell(stream);
        if (iFileLen <= 0) {
            fclose(stream);
            throw "Open aniDesc json file error";
            return;
        }
        fseek(stream, 0, SEEK_SET);

        m_pJsonFileContent = shared_ptr<char[]>(new char[static_cast<size_t>(iFileLen) + 1]);
        size_t bytesRead = fread(m_pJsonFileContent.get(), 1, static_cast<size_t>(iFileLen), stream);
        m_pJsonFileContent[static_cast<size_t>(iFileLen)] = '\0';
        fclose(stream);

        if (bytesRead != static_cast<size_t>(iFileLen)) {
            m_pJsonFileContent = nullptr;
            throw "Read aniDesc json file error";
            return;
        }

        parseJsonDesc();
    }

    void loadAniDesc(string resourceId){
        ResourceProvider* provider = getResourceProvider();
        if (provider == nullptr) {
            printf("LuotiAni::loadAniDesc: No resource provider\n");
            return;
        }

        shared_ptr<vector<char>> fileData = provider->readFile(resourceId);
        if (fileData == nullptr || fileData->empty()) {
            printf("LuotiAni::loadAniDesc: '%s' not found\n", resourceId.c_str());
            return;
        }

        size_t iFileLen = fileData->size();
        m_pJsonFileContent = shared_ptr<char[]>(new char[iFileLen + 1]);
        memcpy(m_pJsonFileContent.get(), fileData->data(), iFileLen);
        m_pJsonFileContent[iFileLen] = '\0';

        parseJsonDesc();
    }

    void parseJsonDesc(){
        m_jsonAniDesc = json::parse(m_pJsonFileContent.get(), nullptr, false, true);

        json overview = m_jsonAniDesc["overview"];
        if (overview.is_null()) {
            printf("Animation Description json error: 'overview' section missing.\n");
            throw "Animation Description json error: 'overview' section missing.";
            return;
        }
        m_name = overview["name"].get<string>();
        m_version = overview["version"].get<string>();
        m_canvasSize.width = overview["view"].at("width").get<float>();
        m_canvasSize.height = overview["view"].at("height").get<float>();
        m_frameRate = overview["frameRate"].get<int>();
        if(m_frameRate == 0) {
            printf("Animation Description json error: 'frameRate' cannot be zero.\n");
            throw "Animation Description json error: 'frameRate' cannot be zero.";
            return;
        }
        m_frameMSDuration = 1000 / m_frameRate;
        m_totalFrames = overview["totalFrames"].get<uint32_t>();
        m_loop = overview.at("loop").get<bool>();

        for (const auto& layerData : m_jsonAniDesc["layers"]) {
            auto layer = make_shared<Layer>();
            layer->setName(layerData.at("name").get<string>())
                ->setType(Layer::strToLayerType(layerData.at("type").get<string>()))
                ->setSrc(layerData.at("src").get<string>())
                ->setSize(SSize(layerData.contains("width") && layerData.contains("height") ?
                                SSize(layerData.at("width").get<float>(), layerData.at("height").get<float>()) :
                                SSize(0, 0)))
                ->setOpacity(layerData.at("opacity").get<float>() / 100.0f)
                ->setBlendMode(Layer::blendModeStrToBlendMode(layerData.at("blendMode").get<string>()));
            for (const auto& keyFrameData : layerData["keyFrames"]) {
                auto keyFrame = make_shared<KeyFrame>();
                uint32_t frameNumber = keyFrameData.at("frame").get<uint32_t>();

                auto operationsData = keyFrameData.at("operation");
                for (const auto& operationData : operationsData) {
                    string type = operationData.at("type").get<string>();
                    Operation::OPERATION_TYPE opType = Operation::strToOperationType(type);

                    shared_ptr<Operation> operation = nullptr;
                    switch(opType) {
                        case Operation::OPERATION_TYPE::TRANSLATE:
                            operation = make_shared<Operation>(opType, operationData.at("tx").get<float>(), operationData.at("ty").get<float>());
                            break;
                        case Operation::OPERATION_TYPE::SCALE:
                            operation = make_shared<Operation>(opType, operationData.at("sx").get<float>(), operationData.at("sy").get<float>());
                            break;
                        case Operation::OPERATION_TYPE::ROTATE:
                            operation = make_shared<Operation>(opType, operationData.at("angle").get<float>(), operationData.at("cx").get<float>(), operationData.at("cy").get<float>());
                            break;
                        case Operation::OPERATION_TYPE::OPACITY:
                            operation = make_shared<Operation>(opType, operationData.at("opacity").get<float>());
                            break;
                        case Operation::OPERATION_TYPE::VISIBLE:
                            operation = make_shared<Operation>(opType, operationData.at("visible").get<bool>() ? 1.0f : 0.0f);
                            break;
                        default:
                            printf("KeyFrame Operation: Unknown operation type: %s\n", type.c_str());
                            continue;
                    }
                    if (operation == nullptr) continue;
                    keyFrame->addOperation(operation);
                }
                layer->addKeyFrame(frameNumber, keyFrame);
            }
            m_layers.push_back(layer);
        }

        m_isLoaded = true;
    }


    void update(void) override {
        if (!m_visible) return;
        if (!m_isPrepared || m_frames.empty()) return;

        if (m_isPlaying && m_totalFrames > 0) {
            uint64_t currentTick = getTicks();
            uint64_t deltaTick = currentTick - m_lastFrameMsTick;
            if (deltaTick >= m_frameMSDuration) {
                uint32_t m_nextFrameToDraw = (m_frameToDraw + deltaTick / m_frameMSDuration) % m_totalFrames;

                if (m_nextFrameToDraw < m_frameToDraw) {
                    if (!m_loop) {
                        m_frameToDraw = 0;
                        m_isPlaying = false;

                        { auto evt = make_shared<Event>(EventType::Custom); evt->customInt = static_cast<int>(EventName::AnimationEnded); evt->customPtr = reinterpret_cast<void*>(static_cast<intptr_t>(m_id)); triggerEvent(evt); }
                        return;
                    }
                }
                m_frameToDraw = m_nextFrameToDraw;
                m_lastFrameMsTick = currentTick;
            }
        }
    }

    void draw(float x=0, float y=0, uint8_t alpha=255) override{
        draw(m_frameToDraw, x, y, alpha);
    }

    void draw(uint32_t frameNo, float x=0, float y=0, uint8_t alpha=255) {
        if (!m_visible) return;
        if (!m_isPrepared || m_frames.empty()) return;

        m_frames[frameNo]->draw(x, y, alpha);
    }

    void setRect(SRect rect) override{
        Material::setRect(rect);
        for (size_t f = 0; f < m_frames.size(); f++) {
            m_frames[f]->setRect({0, 0, rect.width, rect.height});
        }
    }

    void play(void){
        if (!m_isPrepared) {
            printf("LuotiAni::play: Animation not prepared.\n");
            throw "LuotiAni::play: Animation not prepared.";
            return;
        }
        m_frameToDraw = 0;
        m_isPlaying = true;
        m_lastFrameMsTick = getTicks();
    }

    void setRenderDevice(RenderDevice* device) override {
        Material::setRenderDevice(device);

        if (device != nullptr) {
            for (size_t i = 0; i < m_frames.size() && i < m_frameSurfaces.size(); i++) {
                Actor* frameActor = dynamic_cast<Actor*>(m_frames[i].get());
                if (frameActor != nullptr && frameActor->getTexture() == nullptr && m_frameSurfaces[i] != nullptr) {
                    auto tex = m_frameSurfaces[i]->createTexture(device);
                    if (tex != nullptr) {
                        frameActor->setTexture(tex);
                    }
                }
            }
        }
    }

    void prepare(uint32_t startFrame = 0){
        if (!m_isLoaded) {
            printf("LuotiAni::prepare: Animation description not loaded.\n");
            throw "LuotiAni::prepare: Animation description not loaded.";
            return;
        }
        if (m_isPrepared) {
            printf("LuotiAni::prepare: Animation already prepared.\n");
            return;
        }

        m_frameToDraw = startFrame;

        if (m_rect.width <= 0 || m_rect.height <= 0) {
            m_rect.width = m_canvasSize.width;
            m_rect.height = m_canvasSize.height;
        }
        vector<OpData> frameOp;
        vector<vector<OpData>> allFrameOp;
        for (uint32_t l = 0; l < m_layers.size(); l++) {
            shared_ptr<Layer> layer = m_layers[l];
            if (layer == nullptr) continue;
            SharedSurface operationSurface;
            switch( layer->getType() ){
                case Layer::LAYER_TYPE::IMAGE:
                    operationSurface = getImageFromResource(layer->getSrc());

                    if(layer->getSize().width == 0) {
                        layer->setSize({(float)operationSurface->width(), layer->getSize().height});
                    }
                    if(layer->getSize().height == 0) {
                        layer->setSize({layer->getSize().width, (float)operationSurface->height()});
                    }
                    if(layer->getSize().width != operationSurface->width() || layer->getSize().height != operationSurface->height()){
                        SharedSurface scaledImageSurface = Surface::create(layer->getSize().width, layer->getSize().height);
                        if (scaledImageSurface == nullptr) {
                            printf("LuotiAni::prepare: Create scaled image surface error\n");
                            throw "LuotiAni::prepare: Create scaled image surface error";
                        }
                        scaledImageSurface->blit(operationSurface.get(), 0, 0, operationSurface->width(), operationSurface->height(),
                                                  0, 0, scaledImageSurface->width(), scaledImageSurface->height());
                        operationSurface = scaledImageSurface;
                    }
                    break;
                case Layer::LAYER_TYPE::SHAPE:
                    continue;
                case Layer::LAYER_TYPE::TEXT:
                    continue;
                default:
                    continue;
            }
            if (operationSurface == nullptr) {
                printf("LuotiAni::prepare: No surface for layer %d.\n", l);
                throw "LuotiAni::prepare: No surface for layer.";
                return;
            }

            OpData opData;
            opData.opacity = layer->getOpacity();
            opData.dRect = {0, 0, (float)operationSurface->width(), (float)operationSurface->height()};
            opData.m = {1, 1};
            opData.surface = operationSurface;

            shared_ptr<KeyFrame> keyFrame = (*layer)[0];
            if (keyFrame == nullptr) throw "LuotiAni::prepare: No keyframe for frame 0.";
            opData = keyFrameToOpData(keyFrame, opData);

            frameOp.push_back(opData);
            allFrameOp.push_back(frameOp);
            frameOp.clear();
        }

        for (uint32_t l = 0; l < m_layers.size(); l++) {
            shared_ptr<Layer> layer = m_layers[l];
            if (layer == nullptr) continue;

            frameOp = allFrameOp[l];

            uint32_t previousFrameNumber = 0;
            OpData previousOpData = frameOp[previousFrameNumber];

            uint32_t previousKeyFrame = 0;
            uint32_t nextKeyFrame = layer->nextKeyFrameNumber(previousKeyFrame);
            while (nextKeyFrame != 0) {
                shared_ptr<KeyFrame> keyFrame = (*layer)[nextKeyFrame];
                if (keyFrame == nullptr) break;
                OpData opData = keyFrameToOpData(keyFrame, previousOpData);

                for (uint32_t f = 1; f < nextKeyFrame - previousKeyFrame; f++){
                    OpData autoOpData;
                    float t = (float)f / (nextKeyFrame - previousKeyFrame);

                    autoOpData.dRect        = previousOpData.dRect;
                    autoOpData.translate.x  = previousOpData.translate.x    + (opData.translate.x   - previousOpData.translate.x)   * t;
                    autoOpData.translate.y  = previousOpData.translate.y    + (opData.translate.y   - previousOpData.translate.y)   * t;
                    autoOpData.m.scaleX     = previousOpData.m.scaleX       + (opData.m.scaleX      - previousOpData.m.scaleX)      * t;
                    autoOpData.m.scaleY     = previousOpData.m.scaleY       + (opData.m.scaleY      - previousOpData.m.scaleY)      * t;
                    autoOpData.rotate       = previousOpData.rotate         + (opData.rotate        - previousOpData.rotate)        * t;
                    autoOpData.centerPos.x  = previousOpData.centerPos.x    + (opData.centerPos.x   - previousOpData.centerPos.x)   * t;
                    autoOpData.centerPos.y  = previousOpData.centerPos.y    + (opData.centerPos.y   - previousOpData.centerPos.y)   * t;
                    autoOpData.opacity      = previousOpData.opacity        + (opData.opacity       - previousOpData.opacity)       * t;
                    autoOpData.visible      = previousOpData.visible;
                    autoOpData.surface      = previousOpData.surface;

                    frameOp.push_back(autoOpData);
                }
                frameOp.push_back(opData);

                previousKeyFrame = nextKeyFrame;
                previousOpData = opData;
                nextKeyFrame = layer->nextKeyFrameNumber(previousKeyFrame);

            }
            while (previousKeyFrame + 1 < m_totalFrames) {
                frameOp.push_back(previousOpData);
                previousKeyFrame++;
            }
            allFrameOp[l] = frameOp;
        }

        for (uint32_t f = 0; f < m_totalFrames; f++) {
            SharedSurface canvas = Surface::create(m_canvasSize.width, m_canvasSize.height);
            if (canvas == nullptr) {
                printf("LuotiAni::prepare: Create canvas surface error\n");
                throw "LuotiAni::prepare: Create canvas surface error";
                return;
            }

            for(uint32_t l = 0; l < m_layers.size(); l++) {
                shared_ptr<Layer> layer = m_layers[l];
                if (layer == nullptr) continue;

                OpData opData = allFrameOp[l][f];

                if(!opData.visible) {
                    continue;
                }

                if (opData.surface == nullptr) continue;
                Surface* srcSurface = opData.surface.get();

                SharedSurface rotatedHolder;
                if(opData.rotate != 0) {
                    rotatedHolder = srcSurface->rotate(opData.rotate, getRenderDevice());
                    if (rotatedHolder == nullptr) {
                        printf("LuotiAni::prepare: rotate surface error.\n");
                        throw "LuotiAni::prepare: rotate surface error.";
                    }
                    srcSurface = rotatedHolder.get();
                }

                SRect dRect = (opData.dRect + opData.translate) * opData.m;

                srcSurface->setAlphaMod(opData.opacity);
                srcSurface->setBlendMode(layer->getBlendMode());

                canvas->blit(srcSurface, 0, 0, srcSurface->width(), srcSurface->height(),
                            (int)dRect.left, (int)dRect.top, (int)dRect.width, (int)dRect.height);
            }
            shared_ptr<Actor> frame = make_shared<Actor>(this, true);
            frame->setRect({0, 0, m_rect.width, m_rect.height});
            auto tex = canvas->createTexture(getRenderDevice());
            frame->setTexture(tex);
            m_frames.push_back(frame);

            m_frameSurfaces.push_back(canvas);
        }
        for (uint32_t l = 0; l < m_layers.size(); l++) {
            OpData opData = allFrameOp[l][0];
            opData.surface = nullptr;
        }

        m_isPrepared = true;
    }
    void pause(void){
        m_isPlaying = false;
    }
    void resume(void) {
        if (!m_isPrepared) {
            printf("LuotiAni::resume: Animation not prepared.\n");
            throw "LuotiAni::resume: Animation not prepared.";
            return;
        }
        m_isPlaying = true;
        m_lastFrameMsTick = getTicks();
    }
    bool isPlaying(void) { return m_isPlaying; };
    bool isPrepared(void) { return m_isPrepared; };
    bool isLoaded(void) { return m_isLoaded; };
    uint64_t getFrameDuration(){ return m_frameMSDuration; };
    uint32_t getTotalFrames(void) { return m_totalFrames; };
    uint32_t getCurrentFrame(void) { return m_frameToDraw; };
    bool isLoop(void) { return m_loop; };
    void setLoop(bool loop) { m_loop = loop; };
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
    LuotiAniBuilder& loadAniDesc(fs::path filePath){
        m_luoAni->loadAniDesc(filePath);
        return *this;
    }
    LuotiAniBuilder& loadAniDesc(string resourceId){
        m_luoAni->loadAniDesc(resourceId);
        return *this;
    }
    LuotiAniBuilder& setRect(SRect rect){
        m_luoAni->setRect(rect);
        return *this;
    }
    LuotiAniBuilder& prepare(uint32_t startFrame = 0){
        m_luoAni->prepare(startFrame);
        return *this;
    }
    LuotiAniBuilder& setLoop(bool loop){
        m_luoAni->setLoop(loop);
        return *this;
    }
    LuotiAniBuilder& setAutoStart(){
        m_luoAni->play();
        return *this;
    }
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
    void update(void) override {
        if (!m_visible) return;
        if (!m_luoAni->isPrepared()) return;

        if (m_isPlaying) {
            uint64_t currentTick = getTicks();
            uint64_t deltaTick = currentTick - m_lastFrameMsTick;
            if (deltaTick >= m_luoAni->getFrameDuration()) {
                uint32_t m_nextFrameToDraw = (m_frameToDraw + deltaTick / m_luoAni->getFrameDuration()) % m_luoAni->getTotalFrames();

                if (m_nextFrameToDraw < m_frameToDraw) {
                    if (!m_luoAni->isLoop()) {
                        m_frameToDraw = 0;
                        m_isPlaying = false;

                        { auto evt = make_shared<Event>(EventType::Custom); evt->customInt = static_cast<int>(EventName::LuotiInstanceEnded); evt->customPtr = reinterpret_cast<void*>(static_cast<intptr_t>(m_userId)); triggerEvent(evt); }
                        return;
                    }
                }
                m_frameToDraw = m_nextFrameToDraw;
                m_lastFrameMsTick = currentTick;
            }
        }
    }

    void draw(float x=0, float y=0, uint8_t alpha=255) override{
        if (!m_visible) return;
        m_luoAni->draw(m_frameToDraw, x, y, alpha);
    }

    void play(void){
        if (!m_luoAni->isPrepared()) {
            printf("LuotiAni::play: Animation not prepared.\n");
            throw "LuotiAni::play: Animation not prepared.";
            return;
        }
        m_frameToDraw = 0;
        m_isPlaying = true;
        m_lastFrameMsTick = getTicks();
    }
    uint64_t getUserId(void) { return m_userId; };
    bool isPlaying(void) { return m_isPlaying; };
};

#endif // LuotiAniH
