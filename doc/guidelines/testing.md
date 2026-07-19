# 测试用例规范

新控件或新功能的测试应包含以下三个层次，缺一不可：

| 层次 | 文件模式 | 用途 | 验收标准 |
|------|---------|------|---------|
| **标准 C++ 测试** | `test/test_xxxx.cpp` | 测试控件的 C++ 直接 API（Builder、setter、getter、回调）、UI 布局 | SDL3 后端编译通过、运行无崩溃、所有功能可交互验证 |
| **集成测试** | `test/test_fromsource_cabi.cpp` 追加 | 在 DLL 模式下的 fromsource 集成环境中验证控件的基本存在和交互 | 三后端均能在 DLL 模式下创建控件并可见 |
| **C ABI 测试** | `test/test_xxxx_cabi.cpp` | 独立的 fromsource C ABI 测试文件，覆盖工厂函数、全部 C ABI setter/getter、事件回调 | SDL3 后端编译通过，控件可见可交互 |

## C ABI 测试构建方式

C ABI 测试（`test_xxxx_cabi.cpp`）**必须使用 JSON 布局方式构建控件**，而非仅通过 C ABI 工厂函数直接创建。理由：

1. **LayoutParser 集成验证** — JSON 路径经过了 `parseControl` → `parseXxxx` → 属性解析 → 事件绑定的完整链路，确保 LayoutParser 对新控件的注册和解析正确。
2. **与 ComboBox 模式一致** — `test_combobox_cabi.cpp` 已使用 JSON 布局方式。
3. **运行时属性修改** — JSON 构建后，可在帧循环中通过 C ABI setter 修改运行时属性，验证 C ABI 函数的双向正确性。

```c
// ✓ 正确：使用 JSON 布局加载控件
const char* layoutJson = R"json({
    "version": "1.0",
    "controls": [
        {
            "type": "NumericUpDown",
            "id": "myNud",
            "rect": { "x": 20, "y": 80, "w": 200, "h": 26 },
            "value": 50,
            "range": { "min": 0, "max": 100 },
            ...
        }
    ]
})json";
uiLoadLayout(layoutJson);
void* nud = uiFindControl("myNud");
double v = uiGetNumericUpDownValue(nud);
```

```c
// ✗ 不推荐：仅通过工厂函数创建（不验证 LayoutParser）
UIControlHandle nud = UICornerstone_CreateNumericUpDown(10, 50, 120, 24);
```

## 执行顺序

1. 先实现**标准 C++ 测试**（单个控件、SDL3 后端），功能稳定后提交审核。
2. 审核通过后扩展 **test_fromsource_cabi.cpp 集成**（追加 ~20 行代码，验证 DLL 模式）。
3. 最后创建**独立的 `test_xxxx_cabi.cpp`**（完整覆盖 C ABI 接口）。
4. SFML/Raylib 后端的测试在 SDL3 确认通过后再启动。
