# ResourceLoader 设计文档（已废弃）

> **⚠️ 该模块已于 Phase 9（2026-06-04）移除，被 `ResourceProvider` 抽象接口替代。**
>
> **替代方案**：`include/ResourceProvider.h` + `src/ResourceProvider.cpp`
> - `FilesystemResourceProvider` 提供 `readFile()` / `openFileStream()` / `exists()` 接口
> - `FontName` 枚举和 `fontFiles` 映射迁至 `include/ConstDef.h`
> - 字体数据生命周期通过 `Label::m_fontData` / `EditBox::m_fontData` 成员管理
>
> 以下内容保留仅供参考，不再适用于当前代码库。

## 概述

ResourceLoader 是 UIControls 库的旧版资源加载器，负责管理所有游戏资源（字体、图片、音频、配置文件等）的加载、缓存和版本控制（已移除）。

## 核心特性

### 1. 单例模式

```cpp
static ResourceLoader* getInstance(void){
    static ResourceLoader instance;
    return &instance;
}
```

### 2. 资源类型

| 类型常量 | 说明 |
|---------|------|
| RT_CONFIG | 配置文件 |
| RT_FONTS | 字体 |
| RT_IMAGES | 图片 |
| RT_BACKGROUND | 背景图 |
| RT_ANIMATIONS | 动画配置 |
| RT_MUSIC | 背景音乐 |
| RT_SOUNDS | 音效 |
| RT_SAVED | 保存文件 |

## 资源版本管理

### 工作流程

```
1. loadConfig() 
   ├── 读取工作目录 config.jsonc (currentResourceVersion)
   └── 读取资源目录 config.jsonc (targetResourceVersion)
   
2. 版本比较
   ├── 不匹配 → 从 pathPrefix 加载，标记 m_isRewriteNeeded = true
   └── 匹配 → 从 workforldPath 加载（本地缓存更快）
```

### 版本控制机制

- 通过 `config.jsonc` 中的 `resourceVersion` 字段判断资源是否需要更新
- 版本不一致时，会自动将新资源保存到本地工作目录
- 下次启动时，如果版本一致，直接从本地缓存加载，提升加载速度

## 资源加载

### 1. 配置加载 (loadConfigFile)

```cpp
void loadConfigFile(fs::path configPath){
    // 1. 打开 JSON 配置文件
    SDL_IOStream *jsonFIleStream = SDL_IOFromFile(path, "r");
    
    // 2. 读取文件内容到内存
    shared_ptr<char[]> m_pJsonFileContent = new char[size + 1];
    SDL_ReadIO(stream, buffer, size);
    
    // 3. 解析 JSON
    m_jsonObject = json::parse(content);
    
    // 4. 提取资源版本号
    m_resourceVersion = m_jsonObject["resourceVersion"];
}
```

### 2. 资源路径收集 (getResourcePath)

从配置文件中读取各类资源的路径列表，存入 `m_resourceMap`：

```cpp
void getResourcePath(json jsonBlockGroup, string resourceType){
    for(auto& resourceItem : jsonBlockGroup[resourceType]){
        m_resourceMap[resourceItem] = make_shared<Resource>(type, 0, nullptr);
    }
}
```

### 3. 异步加载 (loadingThread)

使用 SDL 线程在后台异步加载所有资源：

```cpp
int SDLCALL loadingThread(void *rlInstance){
    for(auto kv : m_resourceMap){
        // 1. 打开文件
        SDL_IOStream *stream = SDL_IOFromFile(path, "r");
        
        // 2. 读取内容到内存
        shared_ptr<char[]> content(new char[fileSize]);
        SDL_ReadIO(stream, content.get(), fileSize);
        
        // 3. 存入资源映射
        kv.second->pMem = content;
        kv.second->resourceSize = fileSize;
        
        m_loadedResourceCount++;
    }
}
```

## 资源结构

### Resource 类

```cpp
class Resource {
    string resourceType;           // 资源类型
    size_t resourceSize;           // 资源大小
    shared_ptr<char[]> pMem;       // 内存中的资源数据
};
```

### 资源映射

```cpp
unordered_map<string, shared_ptr<Resource>> m_resourceMap;
// Key: 资源ID (如 "fonts/Muyao-Softbrush.ttf")
// Value: Resource 对象
```

## 字体管理

### FontName 枚举

支持 44 种字体，通过 `m_fontFiles` 映射表管理：

```cpp
unordered_map<FontName, string> m_fontFiles = {
    {FontName::Asul_Bold, "fonts/Asul-Bold.ttf"},
    {FontName::Muyao_Softbrush, "fonts/Muyao-Softbrush.ttf"},
    // ...
};
```

## 资源保存

### 保存单个资源

```cpp
void saveResourceToPrefPath(string resourceId){
    // 1. 创建目录
    SDL_CreateDirectory(directory);
    
    // 2. 写入文件
    SDL_SaveFile(filePath, pMem, size);
}
```

### 批量保存

```cpp
void saveAllResourceToPrefPath(){
    for(auto kv : m_resourceMap){
        saveResourceToPrefPath(kv.first);
    }
}
```

## 临时存档

### 文件操作

```cpp
SDL_IOStream* openTempSavedFile(char mode);  // mode: 'r' 或 'w'
void closeTempSavedFile(SDL_IOStream *fileStream);
```

存档路径：`workforldPath/saved/tempSaved.jsonc`

## 加载进度

```cpp
float getLoadingProgress(void){
    if(m_totalResourceCount == 0) return 1.0f;
    return (float)m_loadedResourceCount / m_totalResourceCount;
}
```

## 线程安全

- 使用 SDL_CreateThread 创建后台加载线程
- 加载完成后可分离线程：`SDL_DetachThread(m_loadingThread)`
- 主线程可以通过 getLoadingProgress() 查询加载进度

## 配置示例

config.jsonc 结构：

```jsonc
{
    "resourceVersion": "1.2.0",
    "blockDefine": "config/RBlock.jsonc",
    "fonts": [
        "fonts/Muyao-Softbrush.ttf",
        "fonts/HarmonyOS_Sans_SC_Regular.ttf"
    ],
    "images": [
        "images/icon.png",
        "images/play.png"
    ],
    "music": [
        "music/Take the Ride.wav"
    ]
}
```

## 设计优点

1. **版本控制**：自动检测资源更新，避免每次重新加载
2. **异步加载**：后台线程加载，不阻塞主界面
3. **本地缓存**：版本匹配时从本地加载，速度更快
4. **统一管理**：所有资源通过同一个接口访问
5. **内存管理**：使用 shared_ptr 自动管理资源内存

## 潜在改进

1. 添加资源卸载接口，释放不常用资源内存
2. 增加资源预加载优先级机制
3. 添加资源加载失败的重试逻辑
4. 支持资源压缩包格式