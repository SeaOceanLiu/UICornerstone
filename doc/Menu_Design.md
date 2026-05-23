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

### 3.2 事件处理流程

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

### 3.3 子菜单展开

- 鼠标hover到子菜单项时，延迟200ms展开子菜单
- 鼠标从子菜单项移向子菜单面板时，不关闭子菜单
- 子菜单面板位置：在父菜单项的右侧展开

## 4. 视觉规格

### 4.1 颜色方案（VSCode Dark主题）

| 元素 | 颜色 |
|------|------|
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

### 4.2 尺寸规格

| 元素 | 尺寸 |
|------|------|
| 菜单栏高度 | 30px |
| 菜单项高度 | 28px |
| 菜单项左侧padding | 20px |
| 菜单项右侧padding | 20px |
| 图标区域宽度 | 20px |
| 快捷键区域最小宽度 | 60px |
| 子菜单箭头宽度 | 16px |
| 分隔线高度 | 1px |
| 分隔线左右margin | 10px |

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

## 7. 实现计划

1. **Phase 1**: 创建新的 Menu.h 头文件，定义所有类接口
2. **Phase 2**: 实现 MenuBar（菜单栏布局、hover高亮、菜单模式切换）
3. **Phase 3**: 实现 MenuPanel（下拉面板绘制、圆角阴影、菜单项布局）
4. **Phase 4**: 实现 MenuItem（文字绘制、快捷键、子菜单箭头、hover高亮）
5. **Phase 5**: 实现交互逻辑（菜单模式、子菜单延迟展开、点击外部关闭）
6. **Phase 6**: 实现 Builder 模式
7. **Phase 7**: 更新测试文件 test_menu.cpp
8. **Phase 8**: 编译验证
