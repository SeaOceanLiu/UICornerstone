# LuotiAni 动画开发手册

LuotiAni 是 UICornerstone 内置的关键帧动画引擎。它把一份 JSON 描述文件（`.jsonc`）在加载时**烘焙**为全部帧的贴图，播放时只做"按毫秒跳帧"——运行时零插值、零计算。本文从原理讲起，带你从 0 做出第一个动画。

- 设计文档（内部）：`design/LuotiAni_Design.md`
- 真实资源样本：`subModules/assets/animations/`（rotateBtn、bombBlock 等 11 个）
- 引擎实现：`src/LuotiAni.cpp` / `include/LuotiAni.h`

---

## 1. 原理：三段式流水线

```
  jsonc 描述           prepare() 烘焙               play() 播放
┌──────────────┐   ┌────────────────────────┐   ┌──────────────────┐
│ overview     │ → │ 每层贴图加载/缩放        │ → │ update() 按毫秒   │
│ layers       │   │ 关键帧→全帧 OpData 插值  │   │ 推进 m_frameToDraw│
│ keyFrames    │   │ 逐帧合成 canvas(贴图)    │ → │ draw() 直接贴帧图 │
│ operation    │   │ 帧 Actor 数组            │   │ loop / 结束事件   │
└──────────────┘   └────────────────────────┘   └──────────────────┘
```

### 1.1 描述期（JSON）

一份描述 = 画布（overview）+ 若干图层（layer）。每个图层 = 一张贴图（src）+ 一系列关键帧（keyFrames）；每个关键帧 = 一组属性操作（operation）：平移 / 缩放 / 旋转 / 透明度 / 可见性。

### 1.2 烘焙期（prepare）

一次 prepare 把描述变成**所有帧**的具体画面：

1. 每个图层加载贴图；`width/height` 与贴图原始尺寸不一致时拉伸到该尺寸；
2. 从关键帧推导每一帧的属性值（相邻关键帧间线性/缓动插值，得到 `OpData` 数组，`getFrameOpData(layer, frame)` 可读）；
3. 逐帧新建 `overview.view` 尺寸的画布，按图层顺序绘制：先旋转（rotate，按角度旋转贴图）→ 计算 `(位置 + translate) × scale` → 按透明度与混合模式贴到画布上，**超出画布的部分被裁剪**；
4. 每帧画布生成一张贴图，包成帧 Actor，共 `totalFrames` 个。

烘焙完成后 JSON 不再参与运行。内存与帧数、画布尺寸正比（60 帧 512×512 贴图 ≈ 63MB）。

### 1.3 播放期（play / update）

`m_frameMSDuration = 1000 / frameRate` 毫秒一帧。`update()` 用**绝对墙钟毫秒**推进：`下一帧 = 当前帧 + 流逝毫秒 / 帧毫秒`（源:LuotiAni.cpp:460）。因此播放速度只与真实时间有关，**与调用 update 的帧率无关**——帧率低会一次跳多帧，帧率高也按真实毫秒累计。

- `loop=true`：到最后一帧回绕第 0 帧；
- `loop=false`：到尾帧停在第 0 帧，并触发一次 `AnimationEnded` 自定义事件，播放标志置 0。

### 1.4 渲染链

LuotiAni 是 `LuotiAni → Material → ControlImpl → Control` 体系下的控件。`draw()` 把当前帧 Actor 的贴图画到控件矩形（`setRect` 同步所有帧 Actor 的矩形）。动画控件挂到按钮（Button + `"animation"` 属性）或独立创建时，随控件树缩放自动校准（帧 Actor 缩放与父控件一致，源:LuotiAni.cpp:485）。

---

## 2. 从 0 制作第一个动画

目标：做一个 128×128 的图标，"呼吸"效果（1 秒内透明度 100% ↔ 40% 来回）。

### 2.1 准备素材

把素材（SVG 或图片）放进资源目录，路径形如 `animations/<你的动画名>/<文件名>.svg`。资源根目录默认为 `subModules/assets`（可在 `UIInstanceConfig.resourceRoot` 覆盖）。素材的引用路径在 JSON 里写**相对资源根的路径**。

### 2.2 编写 jsonc

在同一个目录建 `<动画名>.jsonc`（jsonc 允许注释）：

```jsonc
{
    "overview": {
        "version": "0.0.1",
        "name": "breathing icon",
        "view": { "width": 128, "height": 128 },
        "frameRate": 30,
        "totalFrames": 30,
        "loop": true
    },
    "layers": [
        {
            "name": "icon",
            "type": "image",
            "src": "animations/breath/breath.svg",
            "width": 128,
            "height": 128,
            "opacity": 100,
            "blendMode": "blend",
            "keyFrames": [
                {
                    "frame": 0,
                    "operation": [
                        { "type": "opacity", "opacity": 100 }
                    ]
                },
                {
                    "frame": 15,
                    "operation": [
                        { "type": "opacity", "opacity": 40, "easing": "ease-in-out" }
                    ]
                },
                {
                    "frame": 30,
                    "operation": [
                        { "type": "opacity", "opacity": 100, "easing": "ease-in-out" }
                    ]
                }
            ]
        }
    ]
}
```

关键点：

- **第 0 帧关键帧必须有**（引擎在 frame 0 找关键帧，找不到直接抛错）；
- 未列出的属性沿用上一关键帧到达值（这里只有透明度在动）；
- `opacity` 是**百分比** 0~100，`easing` 写在目标关键帧的操作上，作用于从上一关键帧到本关键帧的整段；
- `loop` 字段**必填**（缺失抛异常）。

### 2.3 跑起来

引擎级验证（无 UI 依赖，可断点/打印）：

```cpp
auto ani = LuotiAniBuilder(BENCH)
    .loadAniDesc("animations/breath/breath.jsonc")
    .prepare()
    .build();
ani->play();
```

C ABI（独立动画控件，w/h 传 0 时自动取 `view` 画布尺寸）：

```c
UIControlHandle h = UICornerstone_CreateAnimation(inst,
    "animations/breath/breath.jsonc", 20, 20, 0, 0);
UICornerstone_SetBoolProperty(h, "playing", 1);
UICornerstone_SetBoolProperty(h, "loop", 1);
```

绑定到按钮（用按钮当动画载体，布局里用 `"animation"` 属性或代码 `SetStringProperty`）：

```cpp
btn->setStringProperty("animation", "animations/breath/breath.jsonc");
```

### 2.4 验证效果

调试接口直接读中间帧数据，断言/打印都方便：

```cpp
LuotiAni::OpData d = ani->getFrameOpData(0 /*layer*/, 7 /*frame*/);
float alpha = d.opacity;              // 0~255 域
SharedSurface canvas = ani->getFrameCanvas(7);  // 该帧合成后的画布
```

运行现成测试观察视觉：`test_button`（真实资源动画）、`test_animation`（C ABI 集成 A1-A12）、`test_luotiani`（引擎语义 L1-L11 + 全量资源回归 L10）。

---

## 3. 实操样例

### 3.1 上下浮动 + 弧形轨迹（translate + path）

图标在 1 秒内浮起 20px，沿贝塞尔弧线；后 1 秒回到原位。注意 **translate 是相对增量**：第 3 个关键帧 `tx:0,ty:0` 表示"回到上一关键帧的偏移上再加 0"。

```jsonc
"keyFrames": [
    { "frame": 0, "operation": [ { "type": "translate", "tx": 0, "ty": 0 } ] },
    { "frame": 15, "operation": [
        { "type": "translate", "tx": 0, "ty": -20,
          "easing": "ease-in-out",
          "path": { "type": "bezier", "c1x": 30, "c1y": -30 } }   // 控制点相对段起点
    ] },
    { "frame": 30, "operation": [ { "type": "translate", "tx": 0, "ty": 0,
          "easing": "ease-in-out" } ] }
]
```

`path` 决定段内位置**轨迹**（缺省直线），`easing` 决定沿轨迹的**速度**。三种轨迹：`bezier`（c1x/c1y 二次，c2x/c2y 三次）、`parabola`（vx/vy 初速向量，抛体）、`catmull-rom`（`points` 过点数组，平滑曲线）。

### 3.2 旋转一圈（rotate）

```jsonc
"keyFrames": [
    { "frame": 0, "operation": [ { "type": "rotate", "angle": 0, "cx": 64, "cy": 64 } ] },
    { "frame": 60, "operation": [ { "type": "rotate", "angle": 360, "cx": 64, "cy": 64,
          "easing": "ease-in-out" } ] }
]
```

- `angle` 同样**累加**：上一到达角 + 本关键帧角；
- `cx/cy` 是旋转中心（画布内坐标，一般取素材中心）。素材在动画运行时被预旋转，所以换中心点要自己换算。

### 3.3 缩放脉冲（scale）

```jsonc
"keyFrames": [
    { "frame": 0, "operation": [ { "type": "scale", "sx": 1, "sy": 1 } ] },
    { "frame": 10, "operation": [ { "type": "scale", "sx": 1.2, "sy": 1.2, "easing": "ease-out" } ] },
    { "frame": 20, "operation": [ { "type": "scale", "sx": 0.8, "sy": 0.8, "easing": "ease-in-out" } ] }
]
```

`scale` 是**累乘**：第 3 个关键帧到达值 = 1.2 × 0.8 = 0.96。要想"回到 1"，写 `sx: 1/1.2 ≈ 0.8333`。

---

## 4. 速查表（字段参考）

### overview

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `name` / `version` | string | ✓ | 名称与格式版本（当前 `0.0.1`） |
| `view.width/height` | number | ✓ | 画布尺寸；控件未给尺寸时即控件尺寸 |
| `frameRate` | int | ✓ | 帧率，**不得为 0** |
| `totalFrames` | int | ✓ | 总帧数 |
| `loop` | bool | ✓ | 循环播放；**缺失抛异常**（严格解析） |

### layer

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `name` | string | ✓ | 图层名 |
| `type` | string | ✓ | `image`（当前仅 image 走通；shape/text 待扩展，跳过） |
| `src` | string | ✓ | 贴图路径：资源包 ID（相对资源根）或**同 jsonc 目录相对短名**（P0-102） |
| `width/height` | number | — | 显式拉伸尺寸；都缺省保持贴图原始尺寸 |
| `opacity` | number | ✓ | 图层级不透明度百分比。**见 §5 陷阱**：建议关键帧始终显式定义透明度 |
| `blendMode` | string | ✓ | 见下 |
| `keyFrames` | array | ✓ | 关键帧数组 |

`blendMode` 取值：`normal` / `additive` / `additivePremultiplied` / `modulate` / `blend` / `blendPremultiplied` / `multiply`；未知回退 `normal`。

### keyFrames → operation

| type | 字段 | 语义 |
|------|------|------|
| `translate` | `tx`, `ty` | 平移**增量**（到达值累加） |
| `scale` | `sx`, `sy` | 缩放**累乘**因子 |
| `rotate` | `angle`, `cx`, `cy` | 旋转角度**累加**、旋转中心 |
| `opacity` | `opacity` | 不透明度百分比（0~100，覆盖到 0~255 域） |
| `visible` | `visible` | 可见性布尔（缺省 true） |

通用可选字段：`easing`（段内所有属性的插值缓动，定义在目标关键帧）、`path`（仅 translate，段内位置轨迹）。

### easing 值

`linear`（缺省）· `ease-in` · `ease-out` · `ease-in-out` · `quad` · `sine` · `cubic-bezier(x1,y1,x2,y2)`（控制点 x ∈[0,1]）。未知值回退 linear + 警告。

---

## 5. 语义与陷阱（必读）

| # | 内容 |
|---|------|
| 1 | **第 0 帧关键帧必须存在**（`prepare` 直接 throw，源:LuotiAni.cpp:600）。最简写法也是两个关键帧（0 + 终点） |
| 2 | **translate/rotate 是增量累加，scale 是累乘，opacity/visible 是覆盖**——写"回到原值"时按累计量反算（§3.3） |
| 3 | **opacity 域（已修复）**：关键帧 opacity 按百分比×255 转换；图层级 `layer.opacity` 曾走 0~1 浮点路径导致第 0 帧关键帧未写 opacity 时接近透明，现已在 prepare 归一化（×255）。**建议仍显式写 `opacity`**（现有 11 个正式资源都这么做，语义明确） |
| 4 | **loop 必填**，缺失抛 json 异常（strict at） |
| 5 | **最后一段 carry-over**：最后一个关键帧之后的帧保持到达值，不回落到初始值 |
| 6 | **未列出的属性继承**：某段只写部分操作时，其余属性保持上一关键帧到达值（自动补默认） |
| 7 | **画布裁剪**：合成时超出 `view` 范围的绘制被裁剪（越界内容不可见，不是报错） |
| 8 | **图层顺序**：`layers` 数组顺序即绘制顺序，后绘覆盖先绘 |
| 9 | **异常风格**：加载/烘焙失败 throw C 字符串（fopen、JSON、资源缺失）。C ABI 入口已捕获（返回 nullptr / 属性返回 0）；C++ 直接使用时注意 try/catch |
| 10 | **内存**：烘焙 = 全帧贴图。长动画/大画布注意（60 帧 512×512 ≈ 63MB） |
| 11 | **尺寸**：控件 w/h 传 0 → 回退 `view` 尺寸；不是贴图自然尺寸 |
| 12 | **资源路径**：`src` 解析链（P0-102）= 资源包（provider）优先 → **jsonc 所在目录**（相对短名）→ `<exeDir>/assets` → `<exeDir>`；加载路径相对资源根（默认 `subModules/assets`），代码侧传相对路径自动拼 `GetBasePath`；全程 UTF-8 感知（中文/超长路径可用） |

---

## 6. 测试与调试

| 测试 | 覆盖 | 跑法 |
|------|------|------|
| `test_luotiani` | 引擎语义 L1-L11：关键帧/多段/缓动/路径/向后兼容/全量资源回归 L10/帧 Actor 缩放校准 | `build_test.bat test_luotiani` |
| `test_animation` | C ABI 集成 A1-A12：工厂、playing/loop/frame 属性、尺寸回退、异常边界、easing+path 端到端 | `build_test.bat test_animation` |
| `test_button` | 真实资源动画按钮（g_button6 2x 缩放动画）人工观察 | `build_test.bat test_button` |
| `test_scale_json` / `test_scale_cabi` | 缩放对照组（JSON 布局 / C ABI 编程式）：1x/2x 按钮/图片按钮/动画按钮 rect 断言 + 自动点击注入（frame==12 向 ani1x 中心 (1210,190) 注入 MouseDown+MouseUp，验证内嵌动画按钮 CLICK） | `build_test.bat test_scale_json` / `test_scale_cabi` |

调试接口：`getFrameOpData(layer, frame)` 读任意帧的解析后属性；`getFrameCanvas(frame)` 读烘焙后画布；`getFrameActor(frame)` 读帧 Actor（断言缩放/矩形）。

## 7. 进一步

- 控件化属性：`animation`（重载动画，播放中重载从第 0 帧重播）、`playing`（1=play 帧复位重播）、`loop`、`frame`（跳帧，只读当前帧经 getInt）；
- 换动画自动重置烘焙状态（无需手动释放旧帧）；
- 布局 JSON 中的 luotiAni 节点（LayoutParser 已支持动画描述节点）；若需把动画作为独立控件放进布局 JSON，属正交扩展，当前未接入。
