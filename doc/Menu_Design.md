# Menu 菜单控件设计文档（VSCode风格）

## 1. 概述

Menu 控件系统提供完整的菜单功能，包括菜单栏（MenuBar）、下拉菜单（DropdownMenu）和菜单项（MenuItem），视觉风格参考 VSCode 的菜单系统。

### 1.1 VSCode菜单风格特征

- **菜单栏**：深色背景，水平排列顶级菜单项，hover时浅色高亮
- **下拉菜单**：带圆角和阴影，垂直排列菜单项
- **菜单项**：左侧图标区 + 中间文字 + 右侧快捷键/子菜单箭头
- **分隔线**：细灰色水平线
- **子菜单**：向右展开，同样带圆角和阴影
- **hover高亮**：蓝色背景条
- **点击外部关闭**：点击菜单外部区域自动关闭所有菜单

### 1.2 架构设计

```
┌──────────────────────────────────────────────────┐
│                    MenuBar                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐         │
│  │  File(F)  │ │  Edit(E)  │ │  View(V)  │ ...   │
│  └─────┬────┘ └──────────┘ └──────────┘         │
│        │                                         │
│        ▼                                         │
│  ┌─────────────────────┐                         │
│  │  MenuPanel          │                         │
│  │  ┌─────────────────┐│                         │
│  │  │  New File   Ctrl+N │                       │
│  │  │  Open...    Ctrl+O │                       │
│  │  │  ───────────────── │  ← Separator          │
│  │  │  Recent Files   >  │  ← SubMenu            │
│  │  │  ───────────────── │                       │
│  │  │  Save       Ctrl+S │                       │
│  │  │  Exit              │                       │
│  │  └─────────────────┘│                         │
│  └─────────────────────┘                         │
└──────────────────────────────────────────────────┘
```

## 2. 类设计

### 2.1 类层次

```
ControlImpl
├── MenuBar          菜单栏（水平排列顶级菜单项）
├── MenuPanel        下拉菜单面板（垂直排列菜单项，带圆角+阴影）
├── MenuItem         菜单项（文字+快捷键+箭头）
└── MenuSeparator    分隔线
```

### 2.2 MenuBar 菜单栏

菜单栏是水平排列的顶级菜单容器，固定在窗口顶部。

```cpp
class MenuBar : public ControlImpl {
public:
    MenuBar(Control *parent, float xScale=1.0f, float yScale=1.0f);
    ~MenuBar() override;

    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;

    // 添加顶级菜单项
    void addMenu(const string& caption, shared_ptr<MenuPanel> panel);
    void removeMenu(const string& caption);

    // 关闭所有下拉菜单
    void closeAllMenus();

    // 设置菜单栏高度
    void setBarHeight(float height);
    float getBarHeight() const;

    // 菜单项高度与字体大小的比例系数（范围 1.0 ~ 3.0，默认 1.6）
    static void setItemHeightRatio(float ratio);

    // 菜单字体大小（默认 20.0，须在面板创建前设置）
    static void setFontSize(float size);
    static float getFontSize();

private:
    struct MenuEntry {
        string caption;              // 菜单标题，如 "文件(F)"
        SRect hitRect;               // 点击区域
        shared_ptr<MenuPanel> panel; // 下拉面板
    };

    vector<MenuEntry> m_entries;
    float m_barHeight;
    int m_hoveredIndex;     // 当前hover的菜单项索引，-1表示无
    int m_activeIndex;      // 当前激活（展开下拉）的菜单项索引，-1表示无
    bool m_menuMode;        // 是否处于菜单模式（点击展开后，鼠标移动自动切换菜单）

    // 绘制与布局
    void layoutEntries();
    int hitTest(float x, float y);
};
```

### 2.3 MenuPanel 菜单面板

下拉菜单面板，包含垂直排列的菜单项，带圆角和阴影。

```cpp
class MenuPanel : public ControlImpl {
public:
    MenuPanel(Control *parent, float xScale=1.0f, float yScale=1.0f);
    ~MenuPanel() override;

    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;

    // 添加菜单项
    void addItem(shared_ptr<MenuItem> item);
    void addSeparator();
    void removeItem(shared_ptr<MenuItem> item);

    // 显示/隐藏（带动画预留）
    void show();
    void hide();

    // 设置位置（相对于触发菜单项）
    void setPosition(float x, float y);

    // 获取菜单项
    shared_ptr<MenuItem> getItemAt(float x, float y);

    // 关闭本面板及子面板
    void closeWithChildren();

    // 面板尺寸计算
    void recalculateSize();

private:
    vector<shared_ptr<MenuItem>> m_items;
    float m_itemHeight;        // 统一菜单项高度
    float m_minWidth;          // 最小宽度
    float m_iconAreaWidth;     // 左侧图标区宽度
    float m_shortcutAreaWidth; // 右侧快捷键区宽度
    int m_hoveredIndex;        // 当前hover的菜单项索引
    shared_ptr<MenuPanel> m_subMenuPanel; // 当前展开的子菜单面板

    void layoutItems();
    int hitTest(float x, float y);
};
```

### 2.4 MenuItem 菜单项

单个菜单项，支持文字、快捷键、子菜单箭头。

```cpp
enum class MenuItemType {
    Normal,     // 普通菜单项
    Separator,  // 分隔线
    SubMenu     // 子菜单项（有下级菜单）
};

class MenuItem : public ControlImpl {
public:
    using OnClickHandler = function<void(shared_ptr<MenuItem>)>;

    MenuItem(Control *parent, MenuItemType type = MenuItemType::Normal,
             float xScale=1.0f, float yScale=1.0f);
    ~MenuItem() override;

    void create() override;
    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;

    // 属性设置
    void setCaption(const string& caption);
    string getCaption() const;
    void setShortcut(const string& shortcut);  // 如 "Ctrl+N"
    string getShortcut() const;
    void setEnabled(bool enabled);
    bool getEnabled() const;
    void setChecked(bool checked);
    bool getChecked() const;

    // 点击回调
    void setOnClick(OnClickHandler handler);

    // 子菜单
    void setSubMenu(shared_ptr<MenuPanel> panel);
    shared_ptr<MenuPanel> getSubMenu() const;
    bool hasSubMenu() const;

    MenuItemType getType() const;

private:
    MenuItemType m_type;
    string m_caption;
    string m_shortcut;
    bool m_enabled;
    bool m_checked;
    OnClickHandler m_onClick;
    shared_ptr<MenuPanel> m_subMenu;  // 子菜单面板
};
```

## 3. 交互逻辑

### 3.1 菜单模式

VSCode菜单的核心交互是"菜单模式"：

1. **进入菜单模式**：点击菜单栏上的某个菜单项，展开下拉菜单
2. **菜单模式中**：鼠标在菜单栏上移动，自动切换到对应的下拉菜单（无需点击）
3. **退出菜单模式**：
   - 点击菜单项执行操作
   - 点击菜单外部区域
   - 按ESC键

### 3.2 控件生命周期

```
MenuItem::create()  →  ControlImpl::create()  →  createLabels()
```

`create()` 中调用 `createLabels()` 创建所有 label 控件（文字、快捷键、箭头）。
label 在创建时读取 `MenuColors::g_menuTextSize` 和 `MenuColors::MENU_FONT`，
因此字体大小和字体必须在 create 调用之前设置。推荐在构建 MenuPanel 之前调用：

```cpp
MenuBar::setFontSize(16);                       // 先设字体大小
auto panel = MenuPanelBuilder().addItem(...).build();  // 再构建面板
```

**注意**：`setFontSize` 和 `setItemHeightRatio` 对已创建的 label 不生效，
只有下次调用 `create()` 时才会读取新值。

### 3.3 事件处理流程

```
SDL_EVENT_MOUSE_BUTTON_DOWN:
  ├─ 在菜单栏上 → 进入菜单模式，展开对应下拉菜单
  ├─ 在菜单项上 → 执行操作/展开子菜单，退出菜单模式
  └─ 在菜单外部 → 退出菜单模式，关闭所有菜单

SDL_EVENT_MOUSE_MOTION:
  ├─ 菜单模式中，在菜单栏上移动 → 切换下拉菜单
  ├─ 在菜单项上hover → 高亮该菜单项
  └─ 在子菜单项上hover → 延迟展开子菜单

SDL_EVENT_KEY_DOWN:
  └─ ESC → 关闭当前子菜单，或退出菜单模式
```

### 3.4 子菜单展开

- 鼠标hover到子菜单项时，延迟200ms展开子菜单
- 鼠标从子菜单项移向子菜单面板时，不关闭子菜单
- 子菜单面板位置：在父菜单项的右侧展开

## 4. 视觉规格

### 4.1 颜色方案与字体（VSCode Dark主题）

| 元素 | 值 |
|------|-----|
| 菜单栏背景 | #3C3C3C |
| 菜单栏文字 | #CCCCCC |
| 菜单栏hover背景 | #505050 |
| 下拉面板背景 | #252526 |
| 下拉面板边框 | #454545 |
| 下拉面板圆角 | 5px |
| 下拉面板阴影 | rgba(0,0,0,0.4) |
| 菜单项文字 | #CCCCCC |
| 菜单项hover背景 | #094771 |
| 菜单项禁用文字 | #5A5A5A |
| 分隔线颜色 | #454545 |
| 子菜单箭头 | #CCCCCC |
| 快捷键文字 | #858585 |
| **字体** | MapleMono_NF_CN_Regular（含 ▶ 等 Nerd Font 图标） |
| **缺省字体大小** | 20px（运行时可通过 `MenuBar::setFontSize()` 修改） |

### 4.2 尺寸规格

菜单栏高度和菜单项高度不再使用固定常量，而是**基于字体大小 × 比例系数**动态计算。

| 元素 | 尺寸 |
|------|------|
| 菜单栏高度 | `g_menuTextSize × g_heightRatio`（默认 20 × 1.6 = 32px） |
| 菜单项高度 | `g_menuTextSize × g_heightRatio`（默认 20 × 1.6 = 32px） |
| 菜单项左侧padding | 28px |
| 菜单项右侧padding | 20px |
| 图标区域宽度 | 20px |
| 快捷键区域最小宽度 | 60px |
| 子菜单箭头宽度 | 20px |
| 分隔线高度 | 1px |
| 分隔线左右margin | 10px |

**运行时调整：**

- `MenuBar::setFontSize(16)` → 菜单栏/项高度变为 16 × 1.6 = 25.6px
- `MenuBar::setItemHeightRatio(2.0)` → 菜单栏/项高度变为 当前字体大小 × 2.0
- 字体大小和比例系数必须在 `MenuPanel` 创建（`createLabels()` 调用）**之前**设置，生效于 label 重建时。
- 比例系数范围：1.0 ~ 3.0，超出自动钳制。

## 5. Builder模式

```cpp
// 创建菜单栏
auto menuBar = MenuBarBuilder(parent)
    .addMenu("File(F)", MenuPanelBuilder()
        .addItem(MenuItemBuilder("New File")
            .setShortcut("Ctrl+N")
            .setOnClick([](auto item) { newFile(); })
            .build())
        .addItem(MenuItemBuilder("Open...")
            .setShortcut("Ctrl+O")
            .setOnClick([](auto item) { openFile(); })
            .build())
        .addSeparator()
        .addItem(MenuItemBuilder("Recent Files")
            .setSubMenu(MenuPanelBuilder()
                .addItem(MenuItemBuilder("file1.txt")
                    .setOnClick([](auto item) { openRecent("file1.txt"); })
                    .build())
                .build()))
        .addSeparator()
        .addItem(MenuItemBuilder("Save")
            .setShortcut("Ctrl+S")
            .setOnClick([](auto item) { saveFile(); })
            .build())
        .addItem(MenuItemBuilder("Exit")
            .setOnClick([](auto item) { exitApp(); })
            .build())
        .build())
    .addMenu("Edit(E)", MenuPanelBuilder()
        .addItem(MenuItemBuilder("Undo")
            .setShortcut("Ctrl+Z")
            .setOnClick([](auto item) { undo(); })
            .build())
        .addItem(MenuItemBuilder("Redo")
            .setShortcut("Ctrl+Y")
            .setOnClick([](auto item) { redo(); })
            .build())
        .build())
    .build();
```

## 6. 与旧版Menu的兼容性

### 6.1 保留的类名

- `MenuBar` - 保留，但内部实现完全重写
- `MenuItem` - 保留，接口简化
- `MenuSeparator` - 保留，合并到MenuItem的Separator类型

### 6.2 移除的类

- `MenuContainer` - 替换为 `MenuPanel`
- `MenuBase` - 功能合并到各子类
- `MenuClassLevel` 枚举 - 不再需要
- `SubMenuState` 枚举 - 不再需要

### 6.3 新增的类

- `MenuPanel` - 替代MenuContainer，负责下拉菜单面板的绘制和布局
- `MenuBarBuilder` / `MenuPanelBuilder` / `MenuItemBuilder` - Builder模式

### 6.4 API 变更

| API | 变更说明 |
|------|---------|
| `MenuBar::setFontSize(float)` | **新增** - 运行时修改菜单字体大小 |
| `MenuBar::getFontSize()` | **新增** - 获取当前字体大小 |
| `MenuBar::setItemHeightRatio(float)` | **新增** - 设置菜单项/栏高度与字体大小的比例 |
| `MenuItem::create()` | **新增** - 调用 `createLabels()`，确保字体大小生效 |
| `MenuColors::BAR_HEIGHT` / `ITEM_HEIGHT` | **移除** - 替换为 `getItemHeight()` / `getBarHeight()` 动态计算 |
| `MenuColors::MENU_TEXT_SIZE` | **移除** - 替换为运行时变量 `g_menuTextSize` |
| 字体 | **变更** - `HarmonyOS_Sans_SC_Regular` → `MapleMono_NF_CN_Regular` |
| 箭头编码 | **修复** - `"\u25B6"` → `u8"\u25B6"`（避免 CP936 乱码） |

## 7. LayoutParser 集成

### 7.1 JSON Schema

MenuBar 已集成到 LayoutParser 中，支持通过 JSON 配置文件定义完整的菜单结构（含多级子菜单）。详见 `LayoutSystem_Design.md` 第 4.5 节的 MenuBar 章节。

### 7.2 解析流程

```
parseControl(j, parent)
  └─ type == "MenuBar"
       → parseMenuBar(j, parent)
            ├─ 创建 MenuBar(parent, xScale, yScale)
            ├─ parseCommonProperties (visible/enabled)
            ├─ font.size → MenuBar::setFontSize()
            ├─ barHeight → setBarHeight()
            ├─ menus 数组遍历
            │    └─ 对每个菜单：
            │         ├─ 创建 MenuPanel(nullptr, xScale, yScale)
            │         ├─ populateMenuPanel(panel, items)
            │         │    └─ 遍历 items 数组：
            │         │         ├─ Separator → panel->addSeparator()
            │         │         ├─ Normal → create MenuItem + setCaption/setShortcut/setChecked
            │         │         ├─ SubMenu → create MenuItem + 递归 populateMenuPanel → setSubMenu
            │         │         └─ events.onClick → parseEvents → setOnClick
            │         └─ menuBar->addMenu(caption, panel)
            └─ 注册 ID
```

### 7.3 事件绑定

菜单项的点击事件通过标准 LayoutParser 事件系统处理：

```cpp
// C++ 注册事件处理器
g_parser.registerHandler("onMenuNew", [](shared_ptr<Control> c) {
    SDL_Log("Menu: New file");
});

g_parser.registerHandler("onMenuExit", [](shared_ptr<Control> c) {
    SDL_Log("Menu: Exit");
    SDL_Event quitEvent;
    quitEvent.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quitEvent);
});
```

```jsonc
// JSON 绑定
{ "caption": "新建", "shortcut": "Ctrl+N", "events": { "onClick": "onMenuNew" } }
```

- 处理器函数接收 `shared_ptr<Control>` 参数（可 `dynamic_pointer_cast<MenuItem>` 获取原类型）
- 未注册的 handler 名称被静默忽略

### 7.4 注意事项

- `font.size` 设置影响所有菜单（`MenuBar::setFontSize()` 是静态全局方法）
- MenuBar 不需要 `rect` 字段，其宽度继承自父容器，高度由 `barHeight` 或 font 自动计算
- 菜单项与菜单栏不可独立在 JSON 中使用，必须作为 MenuBar 的子项
- 子菜单使用递归解析，嵌套层级不限

## 8. 后续优化方向

- **动态字体切换**：为不同 DPI 场景自动调整字体大小
- **动画过渡**：下拉菜单展开/收起动画
- **键盘导航**：上下箭头选择、回车执行、左右切换菜单
- **无障碍支持**：屏幕阅读器、高对比度模式
- **菜单栏溢出**：窗口缩小时自动收起溢出菜单项到 "..." 按钮
