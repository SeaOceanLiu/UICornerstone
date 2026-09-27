# ActorUnload_AniResourceFallback_Design — Actor 空路径/失败卸载语义 + LuotiAni 帧图资源文件回退

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-09-22 资源行联测反馈）——**P0-22**（空路径防御 + 卸载语义）、**P0-22 补充**（失败清 m_texture）、**P0-23**（帧图资源文件回退，高）
> 前置：源码核实（行号见 §2）+ 既有 basePath 解析链（ResourceRuntimeFix 批已收敛到 loadFromFile）
> 状态：**已放行，已实施**（复核：`requirements/ActorUnload_AniResourceFallback_Design_复核意见.md`，2026-09-22 通过——§7 五问全部确认 + 设计器随批零代码改动）
> **实施记录（2026-09-22）**：
> 1. **P0-22**：`Actor::clearImage()`（清 surface/texture/filePath/filePathStr/resourceId）；`loadFromFile` 空串早退卸载——test_p0_getter 空串读回 0 + 恢复有效路径往返 PASS。
> 2. **P0-22 补充**：`loadFromFile` 失败两分支统一清 surface/texture（路径字段保留）；test_capture_cabi 像素双步：换图前 4680 纹理像素 → 设坏路径后 0（无图）PASS。
> 3. **P0-23**：`getImageFromResource` provider 优先不变；miss 后文件回退双基准（`<exeDir>/assets` → `<exeDir>`）；`provider:` 前缀剥离；`loadFromMemory` 失败继续回退；仍失败 throw（错误含两基准）。探针：assets 外包（`<exeDir>/anims_probe`）SetString=1/playing=1/捕获 14400 像素 FALLBACK RENDERED；负向 SetString=0 不崩（错误信息含双基准）。
> 4. 回归：test_p0_getter / test_capture_cabi / test_animation / test_luotiani 全绿；全量 ALL_BUILD 0 错误。

---

## 1. 问题与目标

| # | 优先级 | 设计器报告 | 核实结论 | 本批处置 |
|---|---|---|---|---|
| P0-22 | 低 | `SetString("image","")` → 拼 basePath 打开 `Debug\` 失败刷日志 | **确认**：空串非 null，走完整加载链（basePath/"" = 目录）→ 两次加载失败 + 日志 | **实施**（§3.1） |
| P0-22 补充 | 低 | 加载失败应清 m_texture（"失败=无图"） | **确认**：Surface 失败置空 m_surface 但旧 m_texture 保留（绘制用 m_texture）→ 删除/换图失败旧图仍显示；device 回退失败反而会清（不一致） | **实施**（§3.2） |
| P0-23 | 高 | `getImageFromResource` 仅查 provider，未注册资源包宿主（设计器）动画帧必然 not found | **确认**：provider null 直接 throw；readFile miss 直接 throw；jsonc `src`（`animations/xxx/xxx.svg`）在独立运行/设计器场景无回退 | **实施**（§3.3） |

目标：资源加载语义三对齐——**空=卸载、失败=无图、帧图零配置可回退**；资源包优先语义不变。

## 2. 现状核实（源码事实）

| 机制 | 位置 | 状态 |
|---|---|---|
| 空路径链 | `Actor::loadFromFile`（Actor.cpp:73-107）：`p.empty()` 无防御 → `is_relative()==true` → `basePath / ""` = 目录 → Surface 失败 → device 回退失败 → `Platform::Log("...failed for '<exeDir>\\'")` | P0-22 缺陷（日志刷屏） |
| 失败保留旧纹理 | 同上：`m_surface = Surface::loadFromFile(...)` 失败置空 surface；`m_texture` 仅在 device 回退分支被赋值（失败即置空）——**device 为 null 时旧纹理保留**；两分支不一致 | P0-22 补充缺陷 |
| 绘制取图 | `Material::draw`（Material.cpp:53-58）：`if (!m_texture) return;` → 清 m_texture 即"无图" | 卸载语义锚点 |
| 纹理成员 | `ControlImpl::m_surface`（ControlBase.h:330）/ `m_texture`（:335，SharedTexture） | clearImage 目标 |
| 帧图解析 | `LuotiAni::getImageFromResource`（LuotiAni.cpp:85-104）：provider null → throw；`readFile` miss → throw；`Surface::loadFromMemory` 失败 → throw | P0-23 缺陷 |
| 帧图调用点 | `LuotiAni::prepare`（LuotiAni.cpp:608）：`operationSurface = getImageFromResource(layer->getSrc())` | 异常经 prepare 冒泡（Button/LuotiAni/CreateAnimation 均有 catch） |
| 默认资源基准 | `MainWindow` provider basePath = `ctx->resourceRoot` 或 `ConstDef::pathPrefix`（= `<exeDir>/assets`，ConstDef.cpp:9）；jsonc `src` 即相对该根（实测 `animations/rotateBtn/rotateBtn.svg`） | 回退基准依据 |
| 既有回退先例 | `Actor::loadFromFile`：provider: 前缀分流 + `Platform::GetBasePath()` 解析 | P0-23 对照 |

## 3. 架构选择与关键设计决策

### 3.1 P0-22：空路径 = 卸载（决策：早退 + 全量清理）

```cpp
// Actor::loadFromFile 开头
std::string p = filePath.string();
if (p.empty()) {          // 空路径：卸载（清空=删除），不拼 basePath、不刷日志
    clearImage();
    return;
}
```

`clearImage()`（Actor 私有新增）：
```cpp
void Actor::clearImage() {
    m_surface.reset();
    m_texture.reset();     // Material::draw 守卫：无纹理即无图
    m_filePath.clear();
    m_filePathStr.clear(); // P0-21 读回语义：卸载后 GetString 返回 0
    m_resourceId.clear();  // 资源引用同清：避免 recreate 把资源图重新加载回来
}
```

- 设计器"清空=删除"全链成立：写 `""` → 无图 + 读回 0；再写有效路径 → 恢复。
- `create()` 语义不变：`m_resourceId`/`m_filePath` 均空 → 跳过加载（`Actor.cpp:64-68` 分支天然成立）。

### 3.2 P0-22 补充：加载失败 = 无图（决策：统一清纹理/表面，路径字段保留）

```cpp
// 两处失败点统一（Surface 失败后 device 回退前 + device 回退失败后）：
m_surface.reset();
m_texture.reset();         // 失败=无图：旧图不残留（换图失败/删除无效问题根除）
Platform::Log("Actor::loadFromFile failed for '%s'\n", filePath.string().c_str());
```

- **路径字段保留**（`m_filePath`/`m_filePathStr` 不清）：读回=尝试值（设计器输入保持显示）；recreate 可重试（资源晚挂载场景）；与"空串卸载"路径互补。
- `loadFromResource` 失败分支**不改**（资源包语义：provider 未就绪/晚注册时保留 m_resourceId 供补读，既有两阶段设计）。

### 3.3 P0-23：帧图 provider miss → 文件回退（决策：资源包优先，双基准回退）

```cpp
SharedSurface LuotiAni::getImageFromResource(string resourceId) {
    std::string rid = resourceId;
    if (rid.rfind(PropertyNames::kProviderPrefix, 0) == 0)   // provider: 前缀：剥除后走同一回退链
        rid = rid.substr(strlen(PropertyNames::kProviderPrefix));

    // 1) 资源包优先（既有语义不变）
    ResourceProvider* provider = getResourceProvider();
    if (provider != nullptr) {
        auto data = provider->readFile(rid);
        if (data && !data->empty()) {
            auto surface = Surface::loadFromMemory(data->data(), data->size());
            if (surface) return surface;                     // loadFromMemory 失败 → 继续文件回退
        }
    }

    // 2) 文件回退：exe/assets（与 provider 默认基准一致）→ exe（Actor 同款基准）
    const fs::path bases[2] = { ConstDef::pathPrefix, fs::path(Platform::GetBasePath()) };
    for (const auto& base : bases) {
        fs::path p = base / rid;
        if (!fs::exists(p)) continue;
        SharedSurface surface = Surface::loadFromFile(p.string());
        if (surface) return surface;
    }

    // 3) 仍失败：throw（调用方 catch 链不变，错误信息含两基准）
    printf("LuotiAni::getImageFromResource Error: '%s' not found (provider + file fallback)\n", rid.c_str());
    throw "LuotiAni::getImageFromResource Error: resource not found";
}
```

- **回退双基准**：`ConstDef::pathPrefix`（`<exeDir>/assets`，jsonc src 的实际根——实测 `animations/rotateBtn/...` 相对它解析）优先；`Platform::GetBasePath()`（`<exeDir>`，Actor 同款，兼容 src 自带 `assets/` 前缀或 exe 旁布局）兜底。
- provider null 不再直接 throw（走文件回退）——内存 provider / 无 provider 宿主零配置可用。
- 资源包命中时行为与旧版完全一致（含 `loadFromMemory` 失败 → 旧版直接 throw，新版继续回退，属增强）。
- 依赖：`LuotiAni.cpp` 增 `#include "ConstDef.h"`。

## 4. API 设计

### 4.1 核心库（无公开新 API）

```cpp
// include/Actor.h        ：新增 private void clearImage();
// src/Actor.cpp          ：loadFromFile 空路径早退 + 两处失败清纹理/表面
// src/LuotiAni.cpp       ：getImageFromResource provider miss 文件回退（双基准）+ ConstDef.h
```

### 4.2 C ABI / Binding

零新增：`SetString("image", "")` 经既有 Set 通道；读回经 GetString（P0-21 已通）。

## 5. 实现要点与验收

| # | 验收 | 测试 |
|---|---|---|
| 1 | `SetString("image","")`：无失败日志、控件无图、`GetString` 返回 0；再设有效路径恢复 | test_p0_getter 扩展（空串往返 + 有效恢复） |
| 2 | 先有效图 → 再无效路径：旧图消失（像素验证）、无崩溃、读回=无效路径 | test_capture_cabi 像素用例（或探针） |
| 3 | 帧图 provider miss → 文件回退可加载（`<exeDir>/assets` 基准） | 探针：临时动画包 src 指向 exe/assets 下文件 + 屏蔽 provider 命中场景 |
| 4 | 帧图双基准兜底（`<exeDir>` 基准） | 探针：包放 `<exeDir>/anims_probe/`（assets 外）→ 帧图加载渲染 |
| 5 | 帧图两基准均 miss → throw（调用方返回 0/失败，不崩） | 负向用例 |
| 6 | 资源包命中优先语义不变 | 既有 test_animation / test_luotiani 全绿 |
| 7 | 三后端编译 0 错误；全量回归（预存失败除外） | ALL_BUILD + 全量测试 |

## 6. 影响面与风险

- **空串语义变化**（P0-22）：`image=""` 从"无效加载+旧图保留"变为"卸载"——设计器已放行（"清空=删除"）；其他调用方无空串依赖（引擎内部工厂不传空串）。
- **失败清纹理**（补充）：换图失败时旧图消失（无图）——即需求语义；对"失败后希望保留旧图"的使用方属行为变化（无已知依赖）。
- **帧图回退**（P0-23）：资源包命中路径不变；miss 时多两次文件探测（`fs::exists` 短路），prepare 一次性成本可忽略；`loadFromMemory` 失败由 throw 变为继续回退（更宽容）。
- `clearImage` 不改 `m_explicitSize`/rect 语义（尺寸跟随逻辑仅在成功加载时触发）。

## 7. 待审核问题

1. **P0-22 卸载范围**：`clearImage` 一并清 `m_resourceId`（推荐，避免 recreate 复活资源图）——确认？
2. **P0-22 补充 路径字段**：失败保留路径（读回=尝试值、可重试）vs 一并清空（读回 0）？（推荐保留）
3. **P0-23 回退基准顺序**：`<exeDir>/assets` → `<exeDir>`（推荐，贴合 jsonc src 实测根 + Actor 兼容）——确认？
4. **P0-23 `provider:` 前缀**：剥前缀后走同一回退链（推荐）——确认？
5. **P0-23 `loadFromMemory` 失败**：继续文件回退（推荐）vs 维持 throw？

## 8. 待提交配套改动（实施时随批）

- 探针/测试：test_p0_getter（空串卸载+恢复）、test_capture_cabi（失败无图像素）、Temp 探针（帧图回退双基准+负向）。
- docs：properties.html `image`/`image-resource` 行补"空串=卸载；失败=无图"说明；animation 帧图资源说明（独立运行回退）——视实现结论同步。
- requirements/ 归档（已完成：2026-09-22 新版第三批清单）；复核意见归档后 make_release 同步 CornerstoneDesigner。
