# UIEarth 组件设计文档

## 1. 概述
`UIEarth` 是一个基于 OpenHarmony UI Lite 框架的 3D 地球渲染组件。它采用 **C/C++ 混合架构**，将核心渲染算法下沉至 C 语言层实现，通过构建球体网格模型，将平面地图纹理映射到球面，实现了支持旋转、缩放和惯性拖拽的 3D 地球效果。

## 2. 核心设计

### 2.1 架构分层 (Architecture)
组件分为两层：
1.  **UI 层 (`UIEarth` - C++)**：负责生命周期管理、事件处理（拖拽、点击）、动画控制以及与 UI 框架的对接。
2.  **渲染核心层 (`EarthRenderer` - C)**：负责网格生成、矩阵变换、坐标计算和图元组装。该层不依赖 C++ 运行时，具有更高的移植性和独立性。

### 2.2 网格生成 (Mesh Generation)
地球模型在 C 层被离散化为一系列四边形面片（Quad）。通过经纬度分割生成顶点坐标和纹理坐标（UV）。组件支持通过 `SetSegments` 自定义行列数来平衡性能与渲染质量。

```c
// 伪代码：网格顶点生成逻辑 (C 实现)
float theta = lat * PI / 180.0f;
float phi = lon * PI / 180.0f;
EarthVertex* v = &mesh->vertices[i];
v->position.x = radius * cosf(theta) * sinf(phi);
v->position.y = radius * sinf(theta);
v->position.z = radius * cosf(theta) * cosf(phi);
// UV 映射
v->u = lon / 360.0f;
v->v = lat / 180.0f;
```

### 2.3 渲染管线 (Rendering Pipeline)
渲染过程主要在 `EarthRenderer_DrawEarth` 函数中完成，步骤如下：

1.  **变换计算**：
    根据 `EarthRenderer_Params` 中的旋转角 (`rotationX`, `rotationY`)、缩放 (`scale`) 和平移 (`translateX`, `translateY`, `translateZ`) 参数，计算 Model-View 变换矩阵。

2.  **顶点变换**：
    将所有网格顶点从模型空间批量变换到屏幕空间。

3.  **图元组装与剔除**：
    在 `BuildTilesFromMesh` 中处理：
    *   **背面剔除 (Back-face Culling)**：利用屏幕空间向量叉积判断面片朝向，剔除不可见的背面面片 (`z > 0`)。
    *   **UV 边界计算**：计算每个面片对应的纹理区域 (`srcRect`)。

4.  **光栅化桥接**：
    通过 `EarthRenderer_DrawQuad_Bridge` 回调 C++ 层的 `BaseGfxEngine::QuadToQuad` 接口，进行透视校正的纹理映射。

### 2.4 浮点精度优化
为了保证纹理映射的平滑度和准确性，内部计算全链路采用浮点数 (`float`)。C 层定义了专用结构体：

```c
typedef struct {
    float x, y;
} EarthPointF;

typedef struct {
    EarthPointF srcQuad[4]; // 源纹理坐标 (浮点)
    EarthPointF dstQuad[4]; // 屏幕目标坐标 (浮点)
    EarthRect srcRect;
} EarthTile;
```

### 2.5 性能优化
*   **C 语言核心**：核心数学运算（矩阵乘法、向量运算）使用 C 语言实现，减少了对象开销。
*   **内存复用**：`EarthRenderer` 结构体中复用 `mesh` 和 `tilesBuf` 内存，避免每帧重复申请与释放。
*   **批量处理**：顶点变换与图块构建分离，提高缓存命中率。

## 3. 交互设计
*   **拖拽交互**：`UIEarth::OnDragEvent` 处理用户触摸拖动，计算拖拽速度，并更新渲染参数。
*   **惯性动画**：集成 `Animator` (`inertiaAnimator_`) 实现拖拽结束后的惯性旋转效果，模拟真实的物理手感。
*   **过渡动画**：支持从普通状态到边缘状态的平滑过渡 (`transitionAnimator_`)，用于实现如聚焦、入场等特效。

## 4. 使用示例

```cpp
#include "components/ui_earth.h"

// 1. 创建组件
UIEarth* earth = new UIEarth();
earth->SetPosition(0, 0);
earth->SetWidth(400);
earth->SetHeight(400);

// 2. 设置纹理贴图 (需确保路径有效)
earth->SetTexture("test_resources/earth_map.jpg");

// 3. 设置细分粒度（行x列），越高越平滑但开销越大
earth->SetSegments(20, 20);

// 4. 设置初始视角
earth->SetCenter(116.4f, 39.9f); // 北京坐标
earth->SetScale(1.5f);

// 5. 添加到视图层级
rootView->Add(earth);
```
