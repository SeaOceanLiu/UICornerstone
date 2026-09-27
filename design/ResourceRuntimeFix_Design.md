# ResourceRuntimeFix_Design — 资源运行时键 schema 补齐 + 四态图可见性/读回 + 动画分发健壮性

> 提出方：CornerstoneDesigner（UI 设计器）
> 需求来源：`requirements/UICornerstone_配合修改清单_第三批.md`（2026-09-22 追加，P0-17 ~ P0-21）
> 前置：C ABI 实证探针 `Temp/temp_actor_resource_probe.cpp`（SDL3 后端，2026-09-22 全项实测）+ 源码核实（行号见 §2）
> 状态：**已放行，已实施**（复核：`requirements/ResourceRuntimeFix_Design_复核意见.md`，2026-09-22 通过——§7 六问全部确认 + 设计器随批 1 项删除动作）
> **实施记录（2026-09-22）**：
> 1. **P0-20**：`Button` 四 setter 补 `setVisible(true)` + `if (isCreated() && !actor->isCreated()) actor->create()`；test_button 四态 CHECK 全 PASS。
> 2. **P0-21**：`Actor`/`LuotiAni` 增 `m_filePathStr` + `getFilePathStr()`（loadFromFile 存原值 / loadFromResource 清空）；`Button::getStringProperty` 四态图 + animation 原值读回；**延伸**：`LuotiAni::getStringProperty(kAnimation)` 同型补齐（独立动画控件读回，test_p0_getter §8 验证）。
> 3. **路径解析收敛**：basePath 解析统一到 `Actor::loadFromFile`/`LuotiAni::loadFromFile` 内部——Button 四态图/animation 分发、`LuotiAni::setStringProperty`、`CreateAnimation`、`CreateAnimatedButton`、LayoutParser（LuotiAni path/内嵌动画）共 6 处调用方改为**原值直传**（行为等价，读回得原值）。
> 4. **Bonus A**：animation 分发 try/catch + `setLuotiAni` 移至 loadFromFile 之后（失败返 0 / 保留旧动画 / 可重试）；无效路径负向用例 PASS（修复前 terminate）。
> 5. **Bonus B**：`getIntProperty` 转发 `total-frames`/`current-frame`（实测 total=60）。
> 6. **P0-19**：schema button def 补 5 键（`additionalProperties:false` 下必需）+ LayoutParser 字符串键（actors/luotiAni 对象优先）；探针实测 LoadLayout 读回正常、actors 优先成立；validate_layout --strict 三布局 + schema-only 全 PASS。
> 7. **附带**：test_property_cabi 3 处陈旧短键（`check`/`cross`/`box-border`）修为 kebab（92 键迁移遗留），76/0 全绿。
> 8. 回归：test_p0_getter / test_button / test_capture_cabi / test_animation / test_layout(_advanced) / test_luotiani / test_wheel* 等全绿；全量 ALL_BUILD 0 错误。`test_menu_cabi`（bar-height 25.6）/`test_treeview_cabi`（item-font）经 stash 基线对照确认为**预存失败**（与本次无关）。

---

## 1. 问题与目标

| # | 设计器报告 | 核实结论 | 本批处置 |
|---|---|---|---|
| P0-17 | Actor image 分发未拼 basePath，相对路径加载失败 | **不复现**：`loadFromFile` 已拼 basePath（2026-06 起）；探针 rect `0x0→24x24` + 576/576 像素绘制 | 不实施（§3.6，待复现） |
| P0-18 | providerName/resourceId 补运行时 setter | 解析期资源结构键；运行时 `image-resource` 已覆盖 | 不实施（§3.6，维持既有决策） |
| P0-19 | schema button def 补 5 个运行时资源键 | 引擎 setter 已支持、schema 缺声明、解析器不识别 | **实施**（§3.5） |
| P0-20 | 运行时设置四态状态图不显示 | **确认**（双重缺陷：m_visible 默认 false + 无 create） | **实施**（§3.1） |
| P0-21 | 四态图/animation 运行时读回 | **确认**（GetString 均 ret=0；Actor.h:71 悬垂理由需以稳定存储解除） | **实施**（§3.2） |
| Bonus A | （探针附带）animation 分发无异常保护 → 无效路径崩进程（VC 运行库弹窗） | **确认**：Button 无 try/catch，LuotiAni 有 | **实施**（§3.3） |
| Bonus B | （探针附带）内嵌动画 frames 读回未转发 | **确认**：getIntProperty 缺转发 | **实施**（§3.4） |

目标：资源类运行时配置（四态图/animation）的**可见性、读回、健壮性、schema 契约**四端对齐；schema 键与运行时属性名一致（92 键原则）。

## 2. 现状核实（源码事实 + 探针数据）

| 机制 | 位置 | 状态 |
|---|---|---|
| 可见性默认 | `ControlImpl` 构造 `m_visible(false)`（ControlBase.h:19）；`ControlImpl::create()` 内 `setVisible(true)`（ControlBase.cpp:146-151） | 创建即可见（正常路径） |
| 四态 setter | `Button::setNormalState/Hover/Pressed/DisabledActor`（Button.cpp:246-273）：setRect + setParent + 存成员，**无 setVisible / create** | **P0-20 缺陷** |
| 状态 Actor 补建 | `ensureActor` 仅 `Button::create()` 调用一次（Button.cpp:25-36） | 运行时新建 Actor 无人补建 |
| 绘制路径 | `Button::draw` 直调 `actor->draw()`（Button.cpp:92）；`Actor::draw` 守卫 `!m_visible`（Actor.cpp:296-298） | m_visible=false 即不绘制 |
| 动画分发 | `Button::setStringProperty(kAnimation)`（Button.cpp:437-447）：loadFromFile + setLuotiAni + prepare + play，**无 try/catch** | **Bonus A 缺陷**；`setLuotiAni` 有 setVisible（:370）故可见性正常 |
| 异常源 | `LuotiAni::loadAniDesc` 打不开文件即 **throw**（LuotiAni.cpp:158-160）；`prepare/play` 未就绪亦 throw | 未捕获 → std::terminate |
| 对照实现 | `LuotiAni::setStringProperty(kAnimation)` 有 try/catch，失败返回 0 且控件保留（LuotiAni.cpp:857-874） | Button 应镜像 |
| 读回现状 | `Button::getStringProperty` 仅 caption（Button.cpp:480-483）；`getIntProperty` 仅 font-size（:470-473）；`getBoolProperty` 已转发 kPlaying（:389） | P0-21 + Bonus B |
| 路径存储 | Actor：`fs::path m_filePath` + `string m_resourceId`（Actor.h:24-25），image 键**只写不读**（Actor.h:71：`string()` 临时对象悬垂）；LuotiAni：仅 `m_resourceId`（LuotiAni.h:267） | 读回需稳定 string 成员 |
| schema | button def 含 `actors/luotiAni`（解析期对象形式）与 `image/image-resource`，**缺** normal-image/hover-image/pressed-image/disabled-image/animation | P0-19 |
| 解析器 | LayoutParser 无这 5 键处理（grep 零命中）；`actors` 对象形式在建 Actor 时同样走 `make_shared<Actor>` + setter（:953-975）——**安全**（后续 `Button::create → ensureActor` 补建） | P0-19 需决策解析支持 |
| 同型排查 | `CreateImageButton`（UICornerstoneAPI.cpp:1257-1283）：Actor 在 `ctl->create()` 前设置 → ensureActor 补建 ✓；LayoutParser actors ✓；LuotiAni 有 setVisible ✓ | 仅 Button 运行时 setter 缺陷 |
| basePath | `Actor::loadFromFile` 相对路径拼 `Platform::GetBasePath()`（Actor.cpp:81-83，ef5c975 2026-06-07 起）；provider: 前缀分流（:75-77） | P0-17 不复现 |

**探针数据**（`Temp/temp_actor_resource_probe.cpp`）：

```
P0-17 SetString ret=1 after rect=24x24 -> LOADED（捕获 576/576 像素 DRAWN）
P0-17b 工厂相对路径 -> LOADED
P0-20 SetString(normal-image) ret=1 -> 捕获 green=0 blue=0 NOT VISIBLE
P0-20b SetString(animation) ret=1 -> GetBool(playing)=1，捕获 14400/14400 ANIM RENDERED（非同型）
P0-21 GetString(normal-image) ret=0；GetString(animation) ret=0
Bonus A 无效 animation 路径 -> Open aniDesc json file error -> 未捕获异常 -> 进程崩溃（VC 运行库弹窗）
```

## 3. 架构选择与关键设计决策

### 3.1 P0-20：四态 setter 统一补"可见 + 条件补建"（决策：采纳设计器方案）

```cpp
// Button.cpp 四个 setter 统一追加（示意 Normal，Hover/Pressed/Disabled 同）：
void Button::setNormalStateActor(shared_ptr<Actor> actor){
    if (actor == nullptr) return;
    actor->setRect({0, 0, m_rect.width, m_rect.height});
    actor->setParent(this);
    actor->setVisible(true);                                   // ① 运行时构造的 Actor m_visible 默认 false
    if (isCreated() && !actor->isCreated()) actor->create();   // ② ensureActor 只在 Button::create 跑一次
    m_actor = actor;
}
```

- ① `setVisible(true)`：与工厂/ActorBuilder/解析期路径语义一致（设置状态图即期望显示）；`create()` 成功时亦会 setVisible（ControlBase.cpp:146-151），双保险覆盖 `GET_CONTEXT==nullptr` 的早退分支。
- ② 条件补建：仅当 Button 已创建（上下文就绪）；未创建时由既有 `Button::create → ensureActor` 兜底（parse 期路径不变）。
- 同型排查结论：`CreateImageButton`、LayoutParser `actors`、animation 分发均**无此缺陷**（见 §2），无需同批修改。

### 3.2 P0-21：读回以"目标对象稳定存储"为准（决策：Actor/LuotiAni 增路径字符串 + Button 转发）

```cpp
// include/Actor.h
std::string m_filePathStr;                                  // loadFromFile 存设置原值（稳定存储，解除悬垂顾虑）
const std::string& getFilePathStr() const { return m_filePathStr; }

// include/LuotiAni.h —— 同型新增（loadFromFile 存原值；loadFromResource 清空）

// Button::getStringProperty 追加：
if (strcmp(prop, PropertyNames::kNormalImage) == 0)  { return actorPath(m_actor, out); }
if (strcmp(prop, PropertyNames::kHoverImage) == 0)   { return actorPath(m_hoverActor, out); }
if (strcmp(prop, PropertyNames::kPressedImage) == 0) { return actorPath(m_pressedActor, out); }
if (strcmp(prop, PropertyNames::kDisabledImage) == 0){ return actorPath(m_disabledActor, out); }
if (strcmp(prop, PropertyNames::kAnimation) == 0) {
    if (m_luotiAni && !m_luotiAni->getFilePathStr().empty()) { out = m_luotiAni->getFilePathStr().c_str(); return 1; }
    return 0;
}
```

- **数据源为目标对象**（Actor/LuotiAni）而非 Button 缓存：`setNormalStateActor(shared_ptr)` 直接注入路径（Builder/解析路径）也能正确读回；避免双份状态不同步。
- **返回设置原值**（相对保持相对）：与设计器输入一致，避免回填路径被改写（绝对化）。
- **资源引用不冒充文件路径**：经 `image-resource`/`provider:` 建立的 Actor，其文件路径串为空 → 对应 image 键读回返回 0（键不同不越权）。
- **生命周期**：返回指针指向 Actor/LuotiAni 成员 string（随控件存活，至下次 set 前有效）——与 `Button::getStringProperty` caption 现有约定（m_captionText.c_str()）一致；Actor.h:71 的"只写"注释同步修订为"读写（稳定存储）"。
- Bonus B 同批：`Button::getIntProperty` 转发 `kTotalFrames/kCurrentFrame` → `m_luotiAni`（读回与独立 LuotiAni 一致）。

### 3.3 Bonus A：Button animation 分发补异常保护（决策：镜像 LuotiAni 语义）

```cpp
if (strcmp(prop, PropertyNames::kAnimation) == 0) {
    if (!value) return 0;
    fs::path p(value);
    if (p.is_relative() && p.string().rfind(PropertyNames::kProviderPrefix, 0) != 0)
        p = fs::path(Platform::GetBasePath()) / p;
    try {
        auto ani = make_shared<LuotiAni>(this);
        ani->loadFromFile(p);
        setLuotiAni(ani);      // 成功才替换（失败保留旧动画）
        ani->prepare();
        ani->play();
    } catch (...) {
        return 0;              // 失败返回 0，控件保留，可重试
    }
    return 1;
}
```

- 失败语义与 `LuotiAni::setStringProperty` 完全一致（返回 0 / 保留旧动画 / 可重试）；不再产生进程级崩溃。
- 顺序调整：`setLuotiAni` 移至 `loadFromFile` 之后、`prepare` 之前——加载失败时不替换旧动画（原顺序先替换，失败会留下未就绪的新动画）。

### 3.4 Bonus B：frames 读回转发

`Button::getIntProperty` 追加：`kTotalFrames`/`kCurrentFrame` → `m_luotiAni->getIntProperty`（无动画返回 0）。

### 3.5 P0-19：schema 5 键 + 解析器字符串支持（决策：两者同批，避免"校验通过但布局无效"）

1. **schema**：button def 增 `normal-image`/`hover-image`/`pressed-image`/`disabled-image`/`animation`（type string；描述注明运行时资源键/等价 `actors` 对象形式）。
2. **LayoutParser**：button 解析段对 5 键做字符串处理，**复用运行时分发**（`btn->setStringProperty(kNormalImage, value)` 等）——解析期 Button 未创建，Actor 由后续 `Button::create → ensureActor` 补建（与 `actors` 路径同构，已证安全）。
3. **优先级**：`actors` 对象形式优先——字符串键仅在该态未被 `actors` 声明时应用（保守，不破坏既有布局语义）。
4. `image` def 无需改动（`image`/`image-resource` 已具备）。

### 3.6 P0-17 / P0-18：不实施结论

- **P0-17**：`Actor::loadFromFile` 已含 basePath 解析（含 provider: 分流），探针相对路径 LOADED + 像素绘制。请设计器如仍复现，提供具体路径与创建序列；另提示一个真实陷阱——先 `image-resource` 后 `image` 时 `create()` 中 `m_resourceId` 优先（Actor.cpp:64-68），重建会把文件图覆盖回资源图（如系其场景，另立需求）。
- **P0-18**：`providerName`/`resourceId` 为解析期资源结构键（LayoutParser.cpp:793-797 三别名之一），引擎此前已答复"有意保留、无对应运行时属性"（引擎回复 §9）；运行时资源切换由 `image-resource` 覆盖。维持现状，设计器保留过滤。

## 4. API 设计

### 4.1 核心库（无公开新 API）

```cpp
// include/Actor.h      ：新增 std::string m_filePathStr + const std::string& getFilePathStr() const
// include/LuotiAni.h   ：同型新增（loadFromFile 存原值 / loadFromResource 清空）
// src/Button.cpp       ：四 setter 补可见+条件 create；getStringProperty 四态图+animation；getIntProperty frames 转发；
//                        animation 分发 try/catch + 替换顺序
// src/LayoutParser.cpp ：button 段 5 键字符串支持（actors 优先）
```

### 4.2 C ABI / Binding

零新增：读回走既有 `UICornerstone_GetString/GetInt`（Get* 对称），Binding 走 `Control::GetString/GetInt` 通用通道。

## 5. 实现要点与验收

| # | 验收 | 测试 |
|---|---|---|
| 1 | 运行时 `SetString("normal-image", 相对路径)` → Actor created=1 visible=1 且像素可捕获 | test_button.cpp probe A 转 CHECK；test_capture_cabi 增运行时按钮像素用例（纹理绿/蓝 >0） |
| 2 | 四态 setter 全验（hover/pressed/disabled 同） | test_button 扩展四态循环 |
| 3 | `GetString` 四态图往返 = 设置原值（相对保持相对）；未设置返回 0 | test_p0_getter.c 扩展（或新 test_resource_getter.c） |
| 4 | `GetString("animation")` 往返；`GetInt("total-frames")>0`、`current-frame` 可推进 | 同上 |
| 5 | 无效 animation 路径：`SetString` 返回 0、进程存活、旧动画保留 | 负向用例（修复前必崩） |
| 6 | `"normal-image": path` 布局加载生效（像素验证）+ `actors` 优先 | validate_layout --strict + LoadLayout 用例 |
| 7 | 三后端编译 0 错误；全量回归 0 FAIL | 既有回归脚本 |

## 6. 影响面与风险

- **P0-20 setVisible(true) 覆盖显式隐藏**：设置状态图即期望显示（与工厂/解析路径一致）；如调用方需隐藏应经 visible 属性——语义明确，接受。
- **P0-21 语义变更**：image 键由"只写"→"读写"（原值回读）；docs 同步（properties.html 四态图行、button.html、animation 行）。
- **P0-19 解析优先级**：actors 对象形式优先，字符串键仅补空缺——对既有布局零影响；新增键的布局在旧引擎上校验失败（schema 版本化问题，随 release 同步解决）。
- **Bonus A**：失败路径从"崩溃"改为"返回 0 保留旧动画"，与 LuotiAni 既有语义一致，无回归面。
- 三后端无涉（纯属性/资源层）。

## 7. 待审核问题

1. **P0-19 解析支持**：schema + LayoutParser 同批（推荐，避免校验通过但无效）还是 schema-only？
2. **P0-21 读回值**：设置原值（相对保持相对，推荐）vs 解析后绝对路径？
3. **P0-21 资源场景**：`normal-image` 经 `image-resource`/provider 建立时，读回返回 0（推荐）还是 `provider:<id>`？
4. **P0-20 可见性**：setVisible(true) 覆盖显式隐藏——接受？（推荐接受）
5. **Bonus A 失败语义**：返回 0 + 保留旧动画 + 可重试（与 LuotiAni 对齐）——确认？
6. **P0-17**：如仍复现，请提供具体路径/创建序列（引擎侧已证相对路径可用）。

## 8. 待提交配套改动（实施时随批）

- docs：`appendix/properties.html`（四态图行"只写"→"读写（原值回读）"、animation 行）、`controls/button.html`（状态图运行时设置/读回说明）、schema 刷新 + validate_layout --strict。
- tests：test_button.cpp（probe→CHECK）、test_capture_cabi（运行时四态图像素）、test_p0_getter.c（读回往返 + 负向）。
- requirements/ 归档（已完成：2026-09-22 新版第三批清单）；复核意见归档后 make_release 同步 CornerstoneDesigner。
