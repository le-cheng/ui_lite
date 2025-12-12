# UITileView 设计文档

## 1. 概述 (Overview)
UITileView 是一个容器组件，允许用户以 2D 网格（行和列）的形式组织子视图。用户可以通过滑动手势（拖拽）在不同的 "Tile"（瓦片/页面）之间进行导航。该组件的设计理念参考了 LVGL 的 Tile View 组件。

主要特性：
- 支持 2D 网格布局（任意行/列组合）。
- 支持平滑的拖拽手势导航。
- 支持拖拽轴锁定（一次只能水平或垂直移动，不能斜向移动）。
- 支持松手后自动吸附（Snapping）到最近的 Tile。
- 提供 API 动态设置当前显示的 Tile。


## 4. 核心逻辑实现 (Implementation Details)
### 4.1 布局管理 (Layout)
### 4.2 拖拽处理 (Drag Handling)
### 4.3 自动吸附 (Snapping)
### 4.4 动画实现


## 6. 设计需求概要

- 功能目标
  - 支持 2D 网格分页导航（列/行），平滑拖拽与吸附切换。
  - 同时支持水平与垂直两个轴的独立循环模式（`loopHor_`/`loopVer_`）。
  - 支持tail切换时的转场动画, 包括滑动时和松手到目标位置的动画
  - 支持设置每个子页面拖拽的禁止方向

- 拖拽与定位

- 吸附与动画
  - 拖拽结束：`OnDragEndEvent` 估算目标瓦片并设置 `targetX_`/`targetY_`。
  - 动画插值：`Callback` 使用 `EasingFunc`（默认 `QuintEaseOut`）对 `contentX_`/`contentY_` 做插值并实时刷新布局。

- 转场效果
  - 每瓦片可按方向配置转场：`SetTransitionEffect(col,row,direction,transition)`。
  - 默认提供滑动与覆盖两类转场，方向支持位掩码：`TDIR_ALL`、`TDIR_HOR`、`TDIR_VER` 以及具体四向。
  - 允许方向查询：`GetAllowedDirection(col,row)` 用于在拖拽时约束移动方向。
  enum DragDirection : uint8_t {
        TDIR_NONE = 0,
        TDIR_LEFT = 1,
        TDIR_RIGHT = 2,
        TDIR_TOP = 4,
        TDIR_BOTTOM = 8,
        TDIR_HOR = TDIR_LEFT | TDIR_RIGHT,
        TDIR_VER = TDIR_TOP | TDIR_BOTTOM,
        TDIR_ALL = TDIR_HOR | TDIR_VER
    };

- 可靠性与性能
  - 空指针与边界检查；循环索引采用求模与半跨度归一化，避免大位移累积。
  - 消除重复计算，公共逻辑收敛到统一接口，保持代码简洁高效。
