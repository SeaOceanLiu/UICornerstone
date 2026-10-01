#include "LuotiAni.h"
#include "PropertyNames.h"
#include "PlatformUtils.h"
#include "ConstDef.h"

LuotiAni::Matrix2D LuotiAni::createRotationMatrix(float angle) {
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

SPoint LuotiAni::transformPoint(const Matrix2D *mat, SPoint point) {
    return SPoint{
        mat->m[0][0] * point.x + mat->m[0][1] * point.y,
        mat->m[1][0] * point.x + mat->m[1][1] * point.y
    };
}

uint32_t LuotiAni::getPixel(Surface *surface, int x, int y) {
    if (!surface || x < 0 || x >= surface->width() || y < 0 || y >= surface->height()) {
        return 0;
    }
    return static_cast<uint32_t*>(surface->pixels())[y * surface->width() + x];
}

void LuotiAni::setPixel(Surface *surface, int x, int y, uint32_t pixel) {
    if (!surface || x < 0 || x >= surface->width() || y < 0 || y >= surface->height()) {
        return;
    }
    static_cast<uint32_t*>(surface->pixels())[y * surface->width() + x] = pixel;
}

uint32_t LuotiAni::bilinearInterpolation(Surface *surface, float x, float y) {
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

SharedSurface LuotiAni::getImageFromResource(string resourceId){
    std::string rid = resourceId;
    if (rid.rfind(PropertyNames::kProviderPrefix, 0) == 0)   // provider: 前缀：剥除后走同一回退链
        rid = rid.substr(strlen(PropertyNames::kProviderPrefix));

    // 1) 资源包优先（既有语义不变；provider null 不再直接 throw，走文件回退——P0-23）
    ResourceProvider* provider = getResourceProvider();
    if (provider != nullptr) {
        shared_ptr<vector<char>> imageData = provider->readFile(rid);
        if (imageData != nullptr && !imageData->empty()) {
            SharedSurface surface = Surface::loadFromMemory(imageData->data(), imageData->size());
            if (surface != nullptr) return surface;          // loadFromMemory 失败 → 继续文件回退
        }
    }

    // 2) 文件回退：<exeDir>/assets（jsonc src 实测根）→ <exeDir>（Actor 同款基准，兼容 src 自带 assets/ 前缀）
    const fs::path bases[2] = { ConstDef::pathPrefix, fs::path(Platform::GetBasePath()) };
    for (const auto& base : bases) {
        fs::path p = base / rid;
        if (!fs::exists(p)) continue;
        SharedSurface surface = Surface::loadFromFile(p.string());
        if (surface != nullptr) return surface;
    }

    // 3) 仍失败：throw（调用方 catch 链不变；错误信息含两基准，便于诊断）
    printf("LuotiAni::getImageFromResource Error: '%s' not found (provider + file fallback: '%s', '%s')\n",
           rid.c_str(), bases[0].string().c_str(), bases[1].string().c_str());
    throw "LuotiAni::getImageFromResource Error: resource not found";
}

LuotiAni::OpData LuotiAni::keyFrameToOpData(shared_ptr<KeyFrame> keyFrame, OpData srcOpData){
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


void LuotiAni::loadFromFile(fs::path filePath) {
    std::string p = filePath.string();
    if (p.rfind(PropertyNames::kProviderPrefix, 0) == 0) {  // 内存资源引用：剥前缀走资源 ID 路径
        loadFromResource(p.substr(strlen(PropertyNames::kProviderPrefix)));
        return;
    }
    m_filePathStr = p;                   // 设置原值（相对保持相对），供 getFilePathStr 读回
    if (filePath.is_relative()) {        // 统一在此解析 basePath（调用方传原值；绝对路径为 no-op）
        filePath = fs::path(Platform::GetBasePath()) / filePath;
    }
    loadAniDesc(filePath);
}
void LuotiAni::loadFromResource(string resourceId) {
    m_resourceId = resourceId;
    m_filePathStr.clear();               // 资源引用不冒充文件路径（P0-21：animation 键读回返回 0）
    loadAniDesc(resourceId);
}
void LuotiAni::loadAniDesc(fs::path filePath){
    std::string p = filePath.string();
    if (p.rfind(PropertyNames::kProviderPrefix, 0) == 0) {  // 兜底：任何 fopen 入口先分流
        loadAniDesc(p.substr(strlen(PropertyNames::kProviderPrefix)));
        return;
    }
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

void LuotiAni::loadAniDesc(string resourceId){
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

void LuotiAni::parseJsonDesc(){
    m_jsonAniDesc = json::parse(m_pJsonFileContent.get(), nullptr, false, true);
    m_layerSegs.clear();

    // 换动画状态重置（§6.5）：二次 loadAniDesc 必须清除上一动画的烘焙状态，
    // 否则 prepare() 因 m_isPrepared 早退、m_frames 保留旧帧而 m_totalFrames 已是新值
    m_isPrepared = false;
    m_frames.clear();
    m_frameSurfaces.clear();
    m_layers.clear();
    m_frameToDraw = 0;

    json overview = m_jsonAniDesc[PropertyNames::kJsonOverview];
    if (overview.is_null()) {
        printf("Animation Description json error: 'overview' section missing.\n");
        throw "Animation Description json error: 'overview' section missing.";
        return;
    }
    m_name = overview[PropertyNames::kJsonName].get<string>();
    m_version = overview[PropertyNames::kJsonVersion].get<string>();
    m_canvasSize.width = overview[PropertyNames::kJsonView].at(PropertyNames::kJsonWidth).get<float>();
    m_canvasSize.height = overview[PropertyNames::kJsonView].at(PropertyNames::kJsonHeight).get<float>();
    m_frameRate = overview[PropertyNames::kJsonFrameRate].get<int>();
    if(m_frameRate == 0) {
        printf("Animation Description json error: 'frameRate' cannot be zero.\n");
        throw "Animation Description json error: 'frameRate' cannot be zero.";
        return;
    }
    m_frameMSDuration = 1000 / m_frameRate;
    m_totalFrames = overview[PropertyNames::kJsonTotalFrames].get<uint32_t>();
    m_loop = overview.at(PropertyNames::kLoop).get<bool>();

    const auto& layersData = m_jsonAniDesc[PropertyNames::kJsonLayers];
    for (size_t l = 0; l < layersData.size(); l++) {
        const auto& layerData = layersData[l];
        auto layer = make_shared<Layer>();
        layer->setName(layerData.at(PropertyNames::kJsonName).get<string>())
            ->setType(Layer::strToLayerType(layerData.at(PropertyNames::kJsonType).get<string>()))
            ->setSrc(layerData.at(PropertyNames::kJsonSrc).get<string>())
            ->setSize(SSize(layerData.contains(PropertyNames::kJsonWidth) && layerData.contains(PropertyNames::kJsonHeight) ?
                            SSize(layerData.at(PropertyNames::kJsonWidth).get<float>(), layerData.at(PropertyNames::kJsonHeight).get<float>()) :
                            SSize(0, 0)))
            ->setOpacity(layerData.at(PropertyNames::kJsonOpacity).get<float>() / 100.0f)
            ->setBlendMode(Layer::blendModeStrToBlendMode(layerData.at(PropertyNames::kJsonBlendMode).get<string>()));
        m_layerSegs.push_back(map<uint32_t, SegmentInfo>());
        for (const auto& keyFrameData : layerData[PropertyNames::kJsonKeyFrames]) {
            auto keyFrame = make_shared<KeyFrame>();
            uint32_t frameNumber = keyFrameData.at(PropertyNames::kFrame).get<uint32_t>();

            auto operationsData = keyFrameData.at(PropertyNames::kJsonOperation);
            SegmentInfo keyFrameSegInfo;
            for (const auto& operationData : operationsData) {
                string type = operationData.at(PropertyNames::kJsonType).get<string>();
                Operation::OPERATION_TYPE opType = Operation::strToOperationType(type);

                if (operationData.contains(PropertyNames::kJsonEasing) && operationData.at(PropertyNames::kJsonEasing).is_string()) {
                    string easing = operationData.at(PropertyNames::kJsonEasing).get<string>();
                    if (parseEasing(easing, keyFrameSegInfo) != 0) {
                        printf("KeyFrame Operation: Unknown easing: %s, fallback to linear\n", easing.c_str());
                    }
                }
                if (opType == Operation::OPERATION_TYPE::TRANSLATE && operationData.contains(PropertyNames::kJsonPath)) {
                    if (!operationData.at(PropertyNames::kJsonPath).is_object()) {
                        printf("KeyFrame Operation: 'path' must be an object, fallback to linear\n");
                    } else if (parsePath(operationData.at(PropertyNames::kJsonPath), keyFrameSegInfo) != 0) {
                        printf("KeyFrame Operation: Unknown path type, fallback to linear\n");
                    }
                }

                shared_ptr<Operation> operation = nullptr;
                switch(opType) {
                    case Operation::OPERATION_TYPE::TRANSLATE:
                        operation = make_shared<Operation>(opType, operationData.at(PropertyNames::kJsonTx).get<float>(), operationData.at(PropertyNames::kJsonTy).get<float>());
                        break;
                    case Operation::OPERATION_TYPE::SCALE:
                        operation = make_shared<Operation>(opType, operationData.at(PropertyNames::kJsonSx).get<float>(), operationData.at(PropertyNames::kJsonSy).get<float>());
                        break;
                    case Operation::OPERATION_TYPE::ROTATE:
                        operation = make_shared<Operation>(opType, operationData.at(PropertyNames::kJsonAngle).get<float>(), operationData.at(PropertyNames::kJsonCx).get<float>(), operationData.at(PropertyNames::kJsonCy).get<float>());
                        break;
                    case Operation::OPERATION_TYPE::OPACITY:
                        operation = make_shared<Operation>(opType, operationData.at(PropertyNames::kJsonOpacity).get<float>());
                        break;
                    case Operation::OPERATION_TYPE::VISIBLE:
                        operation = make_shared<Operation>(opType, operationData.at(PropertyNames::kVisible).get<bool>() ? 1.0f : 0.0f);
                        break;
                    default:
                        printf("KeyFrame Operation: Unknown operation type: %s\n", type.c_str());
                        continue;
                }
                if (operation == nullptr) continue;
                keyFrame->addOperation(operation);
            }
            m_layerSegs[l][frameNumber] = keyFrameSegInfo;
            layer->addKeyFrame(frameNumber, keyFrame);
        }
        m_layers.push_back(layer);
    }

    m_isLoaded = true;
}

int LuotiAni::parseEasing(const string& easing, SegmentInfo& segInfo){
    if (easing == PropertyNames::kEasingLinear) { segInfo.easeType = 0; return 0; }
    if (easing == PropertyNames::kEasingEaseIn) { segInfo.easeType = 1; return 0; }
    if (easing == PropertyNames::kEasingEaseOut) { segInfo.easeType = 2; return 0; }
    if (easing == PropertyNames::kEasingEaseInOut) { segInfo.easeType = 3; return 0; }
    if (easing == PropertyNames::kEasingQuad) { segInfo.easeType = 4; return 0; }
    if (easing == PropertyNames::kEasingSine) { segInfo.easeType = 5; return 0; }
    if (easing.rfind(PropertyNames::kEasingCubicBezierPrefix, 0) == 0 && easing.back() == ')') {
        float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        if (sscanf(easing.c_str(), "cubic-bezier(%f,%f,%f,%f)", &x1, &y1, &x2, &y2) == 4) {
            if (x1 >= 0.0f && x1 <= 1.0f && x2 >= 0.0f && x2 <= 1.0f) {
                segInfo.easeType = 6;
                segInfo.eCx1 = x1;
                segInfo.eCy1 = y1;
                segInfo.eCx2 = x2;
                segInfo.eCy2 = y2;
                return 0;
            }
        }
    }
    return -1;
}

int LuotiAni::parsePath(const json& path, SegmentInfo& segInfo){
    string type = path.at(PropertyNames::kJsonType).get<string>();
    if (type == PropertyNames::kPathBezier) {
        segInfo.pathType = 1;
        segInfo.p1 = path.value(PropertyNames::kJsonC1x, 0.0f);
        segInfo.p2 = path.value(PropertyNames::kJsonC1y, 0.0f);
        if (path.contains(PropertyNames::kJsonC2x) && path.contains(PropertyNames::kJsonC2y)) {
            segInfo.bezierCubic = 1;
            segInfo.p3 = path.value(PropertyNames::kJsonC2x, 0.0f);
            segInfo.p4 = path.value(PropertyNames::kJsonC2y, 0.0f);
        }
        return 0;
    }
    if (type == PropertyNames::kPathParabola) {
        segInfo.pathType = 2;
        segInfo.vx = path.value(PropertyNames::kJsonVx, 0.0f);
        segInfo.vy = path.value(PropertyNames::kJsonVy, 0.0f);
        return 0;
    }
    if (type == PropertyNames::kPathCatmullRom) {
        segInfo.pathType = 3;
        if (path.contains(PropertyNames::kJsonPoints) && path.at(PropertyNames::kJsonPoints).is_array()) {
            for (const auto& p : path.at(PropertyNames::kJsonPoints)) {
                segInfo.points.push_back(SPoint(p.at(PropertyNames::kJsonX).get<float>(), p.at(PropertyNames::kJsonY).get<float>()));
            }
        }
        return 0;
    }
    return -1;
}

float LuotiAni::easeValue(const SegmentInfo& segInfo, float t){
    switch (segInfo.easeType) {
        case 1:
            return t * t;
        case 2:
        {
            float u = 1.0f - t;
            return 1.0f - u * u;
        }
        case 3:
            return 3.0f * t * t - 2.0f * t * t * t;
        case 4:
        {
            if (t < 0.5f) return 2.0f * t * t;
            float u = 1.0f - t;
            return 1.0f - 2.0f * u * u;
        }
        case 5:
            return 1.0f - cosf(t * M_PI / 2.0f);
        case 6:
        {
            float lo = 0.0f, hi = 1.0f;
            for (int i = 0; i < 24; i++) {
                float mid = (lo + hi) / 2.0f;
                float u = 1.0f - mid;
                float bx = 3.0f * u * u * mid * segInfo.eCx1 + 3.0f * u * mid * mid * segInfo.eCx2 + mid * mid * mid;
                if (bx < t) lo = mid; else hi = mid;
            }
            float tm = (lo + hi) / 2.0f;
            float u = 1.0f - tm;
            return 3.0f * u * u * tm * segInfo.eCy1 + 3.0f * u * tm * tm * segInfo.eCy2 + tm * tm * tm;
        }
        default:
            return t;
    }
}

SPoint LuotiAni::pathValue(const SegmentInfo& segInfo, SPoint start, SPoint end, float t){
    if (segInfo.pathType == 1) {
        float u = 1.0f - t;
        SPoint c1(start.x + segInfo.p1, start.y + segInfo.p2);
        if (segInfo.bezierCubic == 0) {
            return SPoint(u * u * start.x + 2.0f * u * t * c1.x + t * t * end.x,
                          u * u * start.y + 2.0f * u * t * c1.y + t * t * end.y);
        }
        SPoint c2(start.x + segInfo.p3, start.y + segInfo.p4);
        return SPoint(u * u * u * start.x + 3.0f * u * u * t * c1.x + 3.0f * u * t * t * c2.x + t * t * t * end.x,
                      u * u * u * start.y + 3.0f * u * u * t * c1.y + 3.0f * u * t * t * c2.y + t * t * t * end.y);
    }
    if (segInfo.pathType == 2) {
        float gx = 2.0f * (end.x - start.x - segInfo.vx);
        float gy = 2.0f * (end.y - start.y - segInfo.vy);
        return SPoint(start.x + segInfo.vx * t + 0.5f * gx * t * t,
                      start.y + segInfo.vy * t + 0.5f * gy * t * t);
    }
    if (segInfo.pathType == 3) {
        vector<SPoint> chain;
        chain.push_back(start);
        for (const auto& p : segInfo.points) chain.push_back(p);
        chain.push_back(end);
        int n = (int)chain.size();
        if (n < 3) {
            return SPoint(start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t);
        }
        float scaled = t * (float)(n - 1);
        int segIdx = (int)scaled;
        if (segIdx >= n - 1) segIdx = n - 2;
        float ft = scaled - (float)segIdx;
        SPoint p0 = chain[max(0, segIdx - 1)];
        SPoint p1 = chain[segIdx];
        SPoint p2 = chain[segIdx + 1];
        SPoint p3 = chain[min(n - 1, segIdx + 2)];
        float t2 = ft * ft;
        float t3 = t2 * ft;
        return SPoint(0.5f * (2.0f * p1.x + (-p0.x + p2.x) * ft + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
                      0.5f * (2.0f * p1.y + (-p0.y + p2.y) * ft + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3));
    }
    return SPoint(start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t);
}

LuotiAni::OpData LuotiAni::getFrameOpData(uint32_t layer, uint32_t frame) const {
    if (layer >= m_frameOpData.size() || layer >= m_layers.size()) {
        return OpData();
    }
    const vector<OpData>& layerData = m_frameOpData[layer];
    if (frame >= layerData.size()) {
        return OpData();
    }
    return layerData[frame];
}

SharedSurface LuotiAni::getFrameCanvas(uint32_t frame) const {
    if (frame >= m_frameSurfaces.size()) {
        return nullptr;
    }
    return m_frameSurfaces[frame];
}


void LuotiAni::update(void) {
    if (!m_visible) return;
    ControlImpl::update();   // P0-33④：hover 检测 + 子控件递归（原 override 未链基类）
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
                    fireCCallback(PropertyNames::kEventAnimationEnded, CCallbackData::Int, &m_id);
                    return;
                }
            }
            m_frameToDraw = m_nextFrameToDraw;
            m_lastFrameMsTick = currentTick;
        }
    }
}

void LuotiAni::draw(float x, float y, uint8_t alpha){
    (void)x; (void)y;  // 帧 Actor 经 getDrawRect 链已定位到本品绝对位置，透传会双重叠加父偏移
    draw(m_frameToDraw, 0, 0, alpha);
}

void LuotiAni::draw(uint32_t frameNo, float x, float y, uint8_t alpha) {
    if (!m_visible) return;
    if (!m_isPrepared || m_frames.empty()) return;

    ControlImpl::beforeDraw();   // P0-26：背景（四态经基类状态机；原手动边框块改走 afterDraw）
    m_frames[frameNo]->refreshScaleWith(m_xxScale, m_yyScale);  // 帧 Actor 缩放校准（§6.9：非控件树成员，快照会陈旧）
    m_frames[frameNo]->draw(x, y, alpha);
    afterDraw();                 // 边框/焦点环（统一基类路径）
}

void LuotiAni::setRect(SRect rect){
    Material::setRect(rect);
    for (size_t f = 0; f < m_frames.size(); f++) {
        m_frames[f]->setRect({0, 0, rect.width, rect.height});
    }
}

void LuotiAni::play(void){
    if (!m_isPrepared) {
        printf("LuotiAni::play: Animation not prepared.\n");
        throw "LuotiAni::play: Animation not prepared.";
        return;
    }
    m_frameToDraw = 0;
    m_isPlaying = true;
    m_lastFrameMsTick = getTicks();
}

void LuotiAni::setFrame(uint32_t frame) {
    if (!m_isPrepared) return;
    if (m_frames.empty() || frame >= m_totalFrames) return;
    m_frameToDraw = frame;
    m_lastFrameMsTick = getTicks();   // 跳帧后复位累计时间，避免首次 update 按旧 delta 连跳
}

Actor* LuotiAni::getFrameActor(uint32_t frameNo) {
    if (!m_isPrepared) return nullptr;
    if (frameNo >= m_frames.size()) return nullptr;
    return m_frames[frameNo].get();
}

void LuotiAni::setRenderDevice(RenderDevice* device) {
    Material::setRenderDevice(device);

    // 布局加载场景：parse 阶段 loadFromFile 仅解析描述（无设备），挂树时这里
    // 设备就绪 → 补 prepare（CreateAnimation 路径已在 addControl 后显式 prepare）
    if (device != nullptr && !m_isPrepared) {
        // 内存资源两阶段：parse 阶段 provider 未就绪（控件未挂树），此处补读描述
        if (!m_resourceId.empty() && m_totalFrames == 0) {
            loadAniDesc(m_resourceId);
        }
        if (m_totalFrames > 0) {
            try {
                prepare();
            } catch (...) {
                Platform::Log("LuotiAni::setRenderDevice: defer prepare failed\n");
            }
        }
    }

    if (device != nullptr) {
        for (size_t i = 0; i < m_frames.size() && i < m_frameSurfaces.size(); i++) {
            Actor* frameActor = dynamic_cast<Actor*>(m_frames[i].get());
            if (frameActor != nullptr && frameActor->getTexture() == nullptr && m_frameSurfaces[i] != nullptr) {
                auto tex = m_frameSurfaces[i]->createTexture(device);
                if (tex != nullptr) {
                    device->setTextureFilter(tex.get(), m_frameBilinearFilter);
                    frameActor->setTexture(tex);
                }
            }
        }
    }
}

void LuotiAni::prepare(uint32_t startFrame){
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
        // 图层级 opacity 存为 0~1（parseJsonDesc:239 /100），OpData::opacity 为 0~255 域——
        // 直接赋值会把 1=100% 截断成几乎透明（缺陷修复：手工×255 归一化，源 §5 陷阱 3）
        opData.opacity = static_cast<uint8_t>(layer->getOpacity() * 255.0f);
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

                SegmentInfo segInfo;
                const auto& layerSegs = m_layerSegs[l];
                auto segIt = layerSegs.find(nextKeyFrame);
                if (segIt != layerSegs.end()) {
                    segInfo = segIt->second;
                }
                float easedT = easeValue(segInfo, t);
                SPoint translated = pathValue(segInfo,
                                              SPoint(previousOpData.translate.x, previousOpData.translate.y),
                                              SPoint(opData.translate.x, opData.translate.y),
                                              easedT);

                autoOpData.dRect        = previousOpData.dRect;
                autoOpData.translate.x  = translated.x;
                autoOpData.translate.y  = translated.y;
                autoOpData.m.scaleX     = previousOpData.m.scaleX       + (opData.m.scaleX      - previousOpData.m.scaleX)      * easedT;
                autoOpData.m.scaleY     = previousOpData.m.scaleY       + (opData.m.scaleY      - previousOpData.m.scaleY)      * easedT;
                autoOpData.rotate       = previousOpData.rotate         + (opData.rotate        - previousOpData.rotate)        * easedT;
                autoOpData.centerPos.x  = previousOpData.centerPos.x    + (opData.centerPos.x   - previousOpData.centerPos.x)   * easedT;
                autoOpData.centerPos.y  = previousOpData.centerPos.y    + (opData.centerPos.y   - previousOpData.centerPos.y)   * easedT;
                autoOpData.opacity      = previousOpData.opacity        + (opData.opacity       - previousOpData.opacity)       * easedT;
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

    m_frameOpData = allFrameOp;

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
        if (getRenderDevice()) getRenderDevice()->setTextureFilter(tex.get(), m_frameBilinearFilter);
        frame->setTexture(tex);
        m_frames.push_back(frame);

        m_frameSurfaces.push_back(canvas);
    }
    for (uint32_t l = 0; l < m_layers.size(); l++) {
        OpData opData = allFrameOp[l][0];
        opData.surface = nullptr;
    }

    m_isPrepared = true;

    if (m_playAfterPrepare) {
        m_playAfterPrepare = false;
        play();                          // prepare 前收到 playing=1：补播放（不 throw）
    }
}
void LuotiAni::pause(void){
    m_isPlaying = false;
}
void LuotiAni::resume(void) {
    if (!m_isPrepared) {
        printf("LuotiAni::resume: Animation not prepared.\n");
        throw "LuotiAni::resume: Animation not prepared.";
        return;
    }
    m_isPlaying = true;
    m_lastFrameMsTick = getTicks();
}

LuotiAniBuilder& LuotiAniBuilder::loadAniDesc(fs::path filePath){
    m_luoAni->loadAniDesc(filePath);
    return *this;
}
LuotiAniBuilder& LuotiAniBuilder::loadAniDesc(string resourceId){
    m_luoAni->loadAniDesc(resourceId);
    return *this;
}
LuotiAniBuilder& LuotiAniBuilder::setRect(SRect rect){
    m_luoAni->setRect(rect);
    return *this;
}
LuotiAniBuilder& LuotiAniBuilder::prepare(uint32_t startFrame){
    m_luoAni->prepare(startFrame);
    return *this;
}
LuotiAniBuilder& LuotiAniBuilder::setLoop(bool loop){
    m_luoAni->setLoop(loop);
    return *this;
}
LuotiAniBuilder& LuotiAniBuilder::setAutoStart(){
    m_luoAni->play();
    return *this;
}

void LuotiInstance::update(void) {
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

void LuotiInstance::draw(float x, float y, uint8_t alpha){
    if (!m_visible) return;
    m_luoAni->draw(m_frameToDraw, x, y, alpha);
}

void LuotiInstance::play(void){
    if (!m_luoAni->isPrepared()) {
        printf("LuotiAni::play: Animation not prepared.\n");
        throw "LuotiAni::play: Animation not prepared.";
        return;
    }
    m_frameToDraw = 0;
    m_isPlaying = true;
    m_lastFrameMsTick = getTicks();
}

// ============================================================
// 属性系统重写（控件化 §6.3/§6.4）
// ============================================================
int LuotiAni::setStringProperty(const char* prop, const char* value) {
    // "path" 为 "animation" 别名（P0-24：布局键=运行时键；布局解析键 kJsonPath="path"）
    if (strcmp(prop, PropertyNames::kAnimation) == 0 || strcmp(prop, PropertyNames::kJsonPath) == 0) {
        if (!value || !value[0]) return 0;
        try {
            fs::path p(value);   // 原值直传：basePath 解析与 provider: 分流统一在 loadFromFile（P0-21 原值读回）
            bool wasPlaying = m_isPlaying;
            loadFromFile(p);     // 依赖 §6.5 换动画状态重置（parseJsonDesc 开头）
            prepare();
            if (wasPlaying) {    // §6.2：播放中换动画 → 从第 0 帧重播（prepare 已复位 frameToDraw=0）
                m_isPlaying = true;
                m_lastFrameMsTick = getTicks();
            }
        } catch (...) {          // §6.4-2：失败返回 0，控件保留，下一次设置仍可重试
            printf("LuotiAni::setStringProperty('animation'): load/prepare failed.\n");
            return 0;
        }
        return 1;
    }
    return ControlImpl::setStringProperty(prop, value);
}

int LuotiAni::getStringProperty(const char* prop, const char*& out) {
    // "path" 为 "animation" 别名（P0-24）
    if (strcmp(prop, PropertyNames::kAnimation) == 0 || strcmp(prop, PropertyNames::kJsonPath) == 0) {
        if (m_filePathStr.empty()) return 0;   // 未设置/资源引用：返回 0（键不同不越权）
        out = m_filePathStr.c_str();
        return 1;
    }
    return ControlImpl::getStringProperty(prop, out);
}

int LuotiAni::setBoolProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kPlaying) == 0) {
        if (value != 0) {
            if (!m_isPrepared) {
                // §6.4-3 两阶段：未 prepare 拒绝直接 play（会 throw）；
                // 记录请求，prepare 完成后自动播放（JSON 布局挂树前设 playing=1 的场景）
                m_playAfterPrepare = true;
                return 1;
            }
            play();                        // 每次置 1 均为 play()（帧复位 0 重播，§6.2 语义）
        } else {
            pause();
            m_playAfterPrepare = false;
        }
        return 1;
    }
    if (strcmp(prop, PropertyNames::kLoop) == 0) {
        setLoop(value != 0);
        return 1;
    }
    return ControlImpl::setBoolProperty(prop, value);
}

int LuotiAni::setIntProperty(const char* prop, int value) {
    if (strcmp(prop, PropertyNames::kFrame) == 0) {
        if (!m_isPrepared) return 0;       // §6.4-4：未 prepare 拒绝
        if (value < 0) return 0;
        setFrame(static_cast<uint32_t>(value));
        return 1;
    }
    return ControlImpl::setIntProperty(prop, value);
}

int LuotiAni::getBoolProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kPlaying) == 0) { out = m_isPlaying ? 1 : 0; return 1; }
    if (strcmp(prop, PropertyNames::kLoop) == 0)     { out = m_loop ? 1 : 0; return 1; }
    return ControlImpl::getBoolProperty(prop, out);
}

int LuotiAni::getIntProperty(const char* prop, int& out) {
    if (strcmp(prop, PropertyNames::kFrame) == 0) { out = static_cast<int>(m_frameToDraw); return 1; }
    if (strcmp(prop, PropertyNames::kTotalFrames) == 0)  { out = static_cast<int>(m_totalFrames); return 1; }
    if (strcmp(prop, PropertyNames::kCurrentFrame) == 0) { out = static_cast<int>(m_frameToDraw); return 1; }
    return ControlImpl::getIntProperty(prop, out);
}
