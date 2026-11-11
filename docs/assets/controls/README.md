# 控件截图

本目录保存用户手册控件章节的截图，统一为 720×340（部分为实际控件尺寸；已裁掉章节标题区）；LuotiAni 使用 260×260 透明 APNG 动画、深色背景，并由引擎实际渲染生成。

- PNG/APNG：页面引用文件（LuotiAni 为透明背景动画 APNG），命名 `<控件>-<场景>.png`；当前使用 `<控件>-overview.png` 表示默认外观/属性对比总览。
- `layouts/`：生成对应截图的声明式布局 JSON（含主题、控件、探针用 `_states`/`_select`/`_visible` 等元数据）。
  - `_states`/`_select`/`_visible`/`_clicks`/`_expandAll`/`_contextMenus` 仅由 `manual_shots` 读取，渲染前剥除；不是运行时 JSON 键。对布局做 `validate_layout --strict` 时需先剥除这些探针元数据（等价于校验实际加载内容）。
- 生成器：`testing_scripts/manual_shots.cpp`（读取布局 → `CaptureBench` → BMP）；BMP 经脚本转为 PNG。
- 特殊控件（ContextMenu、HandleControl、LuotiAni）由专用探针生成；其截图同样来自引擎渲染。

刷新流程：修改控件视觉/属性后，先更新对应 `layouts/*.json`，构建核心库并编译 `manual_shots.cpp`，逐个渲染布局；
用图像工具将 BMP 转为 PNG 并覆盖本目录文件，最后检查手册中的截图与当前实现一致。
