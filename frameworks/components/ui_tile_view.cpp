/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "components/ui_tile_view.h"
#include "gfx_utils/graphic_log.h"
#include <stdlib.h>
#include <math.h>

namespace OHOS {

UITileView::SlideTransition UITileView::defaultSlideTransition_;
UITileView::CoverTransition UITileView::defaultCoverTransition_;

UITileView::UITileView()
    : curCol_(0), curRow_(0), maxCol_(0), maxRow_(0), loopHor_(false), loopVer_(false), isLoop_(false),
      animator_(this, this, 250, false),
      isDragging_(false),
      lockHorizontal_(false),
      targetX_(0), targetY_(0),
      startX_(0), startY_(0),
      contentX_(0), contentY_(0),
      animSrcCol_(-1), animSrcRow_(-1),
      pendingNextCol_(-1), pendingNextRow_(-1),
      totalW_(0), totalH_(0),
      easingFunc_(EasingEquation::QuintEaseOut)
{
    touchable_ = true;
    draggable_ = true;
}

UITileView::~UITileView()
{
    ListNode<TileInfo*>* node = tileList_.Begin();
    while (node != tileList_.End()) {
        TileInfo* info = node->data_;
        node = node->next_;
        delete info;
    }
    tileList_.Clear();
}

void UITileView::Add(UIView* view)
{
    Add(view, 0, 0);
}

void UITileView::Add(UIView* view, uint8_t col, uint8_t row, uint8_t direction)
{
    if (view == nullptr) {
        return;
    }

    TileInfo* info = new TileInfo();
    if (info == nullptr) {
        return;
    }

    info->view = view;
    info->col = col;
    info->row = row;
    info->direction = direction;
    for (int i = 0; i < 4; i++) {
        info->transitions[i] = &defaultSlideTransition_;
    }
    tileList_.PushBack(info);

    if (col > maxCol_) {
        maxCol_ = col;
    }
    if (row > maxRow_) {
        maxRow_ = row;
    }
    RecomputeTotalSpan();

    view->SetDragParentInstead(true);
    view->SetTouchable(true);
    view->SetDraggable(true);
    UIViewGroup::Add(view);
    UpdateChildrenPosition();
}

void UITileView::SetCurrentTile(uint8_t col, uint8_t row, bool animate)
{
    uint32_t width = static_cast<uint32_t>(GetWidth());
    uint32_t height = static_cast<uint32_t>(GetHeight());

    targetX_ = -static_cast<int>(col) * static_cast<int>(width);
    targetY_ = -static_cast<int>(row) * static_cast<int>(height);

    if (loopHor_) {
        int totalW = static_cast<int>(totalW_);
        if (totalW > 0) {
            int diff = targetX_ - contentX_;
            while (diff > totalW / 2) { targetX_ -= totalW; diff -= totalW; }
            while (diff < -totalW / 2) { targetX_ += totalW; diff += totalW; }
        }
    }
    if (loopVer_) {
        int totalH = static_cast<int>(totalH_);
        if (totalH > 0) {
            int diff = targetY_ - contentY_;
            while (diff > totalH / 2) { targetY_ -= totalH; diff -= totalH; }
            while (diff < -totalH / 2) { targetY_ += totalH; diff += totalH; }
        }
    }

    if (animate) {
        pendingNextCol_ = static_cast<int>(col);
        pendingNextRow_ = static_cast<int>(row);
        startX_ = contentX_;
        startY_ = contentY_;
        animator_.Start();
    } else {
        contentX_ = targetX_;
        contentY_ = targetY_;
        curCol_ = col;
        curRow_ = row;
        pendingNextCol_ = -1;
        pendingNextRow_ = -1;
        UpdateChildrenPosition();
    }
}

void UITileView::SetLoop(bool loop)
{
    SetLoop(loop, loop);
}

void UITileView::SetLoop(bool hor, bool ver)
{
    loopHor_ = hor;
    loopVer_ = ver;
    isLoop_ = hor || ver;
    RecomputeTotalSpan();
    UpdateChildrenPosition();
}

// ==================== 转场效果配置 ====================

/**
 * @brief 为指定瓦片的特定方向设置转场效果
 *
 * 使用说明：
 * 1. 每个瓦片可以为4个方向（左、右、上、下）分别设置不同的转场效果
 * 2. direction 参数支持位掩码组合，例如：
 *    - TDIR_LEFT | TDIR_RIGHT：同时设置左右方向
 *    - TDIR_ALL：设置所有方向
 * 3. 内置转场效果：
 *    - defaultSlideTransition_：滑动转场（默认）
 *    - defaultCoverTransition_：覆盖转场
 *
 * 示例代码：
 * @code
 *   // 为第(0,0)个瓦片的左侧方向设置覆盖转场
 *   tileView->SetTransitionEffect(0, 0, TDIR_LEFT, &UITileView::defaultCoverTransition_);
 *
 *   // 为第(1,0)个瓦片的所有方向设置滑动转场
 *   tileView->SetTransitionEffect(1, 0, TDIR_ALL, &UITileView::defaultSlideTransition_);
 * @endcode
 *
 * @param col 瓦片的列索引
 * @param row 瓦片的行索引
 * @param direction 方向位掩码（TDIR_LEFT/RIGHT/TOP/BOTTOM 或其组合）
 * @param transition 转场效果对象指针
 */
void UITileView::SetTransitionEffect(uint8_t col, uint8_t row, uint8_t direction, TileTransition* transition)
{
    if (transition == nullptr) {
        return;
    }

    // 使用接口查找 TileInfo，避免在调用处显式遍历链表，保持简洁
    TileInfo* info = FindTileInfo(col, row);
    if (info == nullptr) {
        return;
    }

    if (direction == TDIR_ALL) {
        info->transitions[0] = transition;
        info->transitions[1] = transition;
        info->transitions[2] = transition;
        info->transitions[3] = transition;
        return;
    }
    if (direction == TDIR_HOR) {
        info->transitions[0] = transition;
        info->transitions[1] = transition;
        return;
    }
    if (direction == TDIR_VER) {
        info->transitions[2] = transition;
        info->transitions[3] = transition;
        return;
    }
    // transitions 数组索引：0=左, 1=右, 2=上, 3=下
    if (direction & TDIR_LEFT) {
        info->transitions[0] = transition;
    }
    if (direction & TDIR_RIGHT) {
        info->transitions[1] = transition;
    }
    if (direction & TDIR_TOP) {
        info->transitions[2] = transition;
    }
    if (direction & TDIR_BOTTOM) {
        info->transitions[3] = transition;
    }
}

// ==================== 转场效果实现 ====================

/**
 * @brief 滑动转场：源页面和目标页面同时移动
 *
 * 效果说明：
 * - 源页面随拖拽移动
 * - 目标页面从屏幕外滑入
 * - 两个页面保持相邻，无缝衔接
 *
 * @param srcView 当前显示的页面
 * @param dstView 即将显示的页面
 * @param offset 拖拽偏移量（负值=向左/上拖，正值=向右/下拖）
 * @param range 页面尺寸（宽度或高度）
 * @param isHorizontal 是否为水平方向拖拽
 */
void UITileView::SlideTransition::Apply(UIView* srcView, UIView* dstView, int offset, int range, bool isHorizontal)
{
    if (srcView == nullptr || dstView == nullptr) {
        return;
    }

    if (isHorizontal) {
        // 水平滑动
        // 源页面位置 = 拖拽偏移量
        srcView->SetPosition(static_cast<int16_t>(offset), 0);

        // 目标页面位置计算：
        // - 向左拖（offset < 0）：目标页面从右侧滑入，初始位置 = offset + range
        // - 向右拖（offset > 0）：目标页面从左侧滑入，初始位置 = offset - range
        int dstX = offset + (offset < 0 ? range : -range);
        dstView->SetPosition(static_cast<int16_t>(dstX), 0);
    } else {
        // 垂直滑动
        srcView->SetPosition(0, static_cast<int16_t>(offset));
        int dstY = offset + (offset < 0 ? range : -range);
        dstView->SetPosition(0, static_cast<int16_t>(dstY));
    }
}

/**
 * @brief 覆盖转场：源页面保持不动，目标页面从上方滑入覆盖
 *
 * 效果说明：
 * - 源页面固定在原位（0, 0）
 * - 目标页面从屏幕外滑入，逐渐覆盖源页面
 * - 目标页面始终在源页面上层（通过 MoveChildToFront 实现）
 *
 * 适用场景：
 * - 类似 iOS 的 push 动画
 * - 强调新内容的进入感
 *
 * @param srcView 当前显示的页面（保持不动）
 * @param dstView 即将显示的页面（滑入覆盖）
 * @param offset 拖拽偏移量
 * @param range 页面尺寸
 * @param isHorizontal 是否为水平方向拖拽
 */
void UITileView::CoverTransition::Apply(UIView* srcView, UIView* dstView, int offset, int range, bool isHorizontal)
{
    if (srcView == nullptr || dstView == nullptr) {
        return;
    }

    // 源页面固定在原位
    srcView->SetPosition(0, 0);

    // 计算目标页面的滑入位置
    // 目标页面从屏幕外开始，随拖拽逐渐移动到 (0, 0)
    if (isHorizontal) {
        // 水平覆盖
        // - 向左拖（offset < 0）：目标页面从右侧（range）滑入到当前位置（range + offset）
        //   当 offset = -range 时，目标页面完全覆盖（位置 = 0）
        // - 向右拖（offset > 0）：目标页面从左侧（-range）滑入
        int dstX = (offset < 0 ? range : -range) + offset;
        dstView->SetPosition(static_cast<int16_t>(dstX), 0);
    } else {
        // 垂直覆盖
        int dstY = (offset < 0 ? range : -range) + offset;
        dstView->SetPosition(0, static_cast<int16_t>(dstY));
    }

    // 确保目标页面在源页面上层，实现覆盖效果
    UIView* parent = dstView->GetParent();
    if (parent != nullptr) {
        UITileView* tileView = static_cast<UITileView*>(parent);
        tileView->MoveChildToFront(dstView);
    }
}

/**
 * @brief 将指定子视图移到最前端（Z轴顺序）
 *
 * 实现原理：
 * - 先从视图组中移除该视图
 * - 再重新添加到视图组（会被添加到末尾，即最上层）
 *
 * @param view 要移到前端的视图
 */
void UITileView::MoveChildToFront(UIView* view)
{
    if (view == nullptr) {
        return;
    }
    UIViewGroup::Remove(view);
    UIViewGroup::Add(view);
}

bool UITileView::OnDragStartEvent(const DragEvent& event)
{
    StopAnimation();

    int dx = event.GetDeltaX();
    int dy = event.GetDeltaY();

    if (dx != 0 || dy != 0) {
        if (abs(dx) > abs(dy)) {
            lockHorizontal_ = true;
        } else {
            lockHorizontal_ = false;
        }
    }
    return UIView::OnDragStartEvent(event);
}

bool UITileView::OnDragEvent(const DragEvent& event)
{
    StopAnimation();

    if (lockHorizontal_) {
        DragXInner(static_cast<int16_t>(event.GetDeltaX()));
    } else {
        DragYInner(static_cast<int16_t>(event.GetDeltaY()));
    }

    return UIView::OnDragEvent(event);
}

bool UITileView::OnDragEndEvent(const DragEvent& event)
{
    uint32_t width = static_cast<uint32_t>(GetWidth());
    uint32_t height = static_cast<uint32_t>(GetHeight());
    if (width == 0u || height == 0u) {
        return UIView::OnDragEndEvent(event);
    }

    int estimatedCol = 0;
    int estimatedRow = 0;

    if (loopHor_ || loopVer_) {
        estimatedCol = static_cast<int16_t>(round(static_cast<float>(-contentX_) / static_cast<float>(width)));
        estimatedRow = static_cast<int16_t>(round(static_cast<float>(-contentY_) / static_cast<float>(height)));

        int normCol = estimatedCol;
        if (loopHor_) {
            int maxC = maxCol_ + 1;
            normCol = estimatedCol % maxC;
            if (normCol < 0) {
                normCol += maxC;
            }
        } else {
            if (normCol < 0) {
                normCol = 0;
            }
            if (normCol > maxCol_) {
                normCol = maxCol_;
            }
        }

        int normRow = estimatedRow;
        if (loopVer_) {
            int maxR = maxRow_ + 1;
            normRow = estimatedRow % maxR;
            if (normRow < 0) {
                normRow += maxR;
            }
        } else {
            if (normRow < 0) {
                normRow = 0;
            }
            if (normRow > maxRow_) {
                normRow = maxRow_;
            }
        }

        if (GetTile(static_cast<uint8_t>(normCol), static_cast<uint8_t>(normRow)) != nullptr) {
            animSrcCol_ = curCol_;
            animSrcRow_ = curRow_;
            SetCurrentTile(static_cast<uint8_t>(normCol), static_cast<uint8_t>(normRow), true);
        } else {
            animSrcCol_ = curCol_;
            animSrcRow_ = curRow_;
            SetCurrentTile(curCol_, curRow_, true);
        }
    } else {
        estimatedCol = (-contentX_ + static_cast<int>(width / 2)) / static_cast<int>(width);
        estimatedRow = (-contentY_ + static_cast<int>(height / 2)) / static_cast<int>(height);

        if (estimatedCol < 0) {
            estimatedCol = 0;
        }
        if (estimatedRow < 0) {
            estimatedRow = 0;
        }

        // 检查是否存在精确匹配
        if (GetTile(static_cast<uint8_t>(estimatedCol), static_cast<uint8_t>(estimatedRow)) != nullptr) {
            animSrcCol_ = curCol_;
            animSrcRow_ = curRow_;
            SetCurrentTile(static_cast<uint8_t>(estimatedCol), static_cast<uint8_t>(estimatedRow), true);
        } else {
            animSrcCol_ = curCol_;
            animSrcRow_ = curRow_;
            SetCurrentTile(curCol_, curRow_, true);
        }
    }

    return UIView::OnDragEndEvent(event);
}

// ==================== 位置更新核心逻辑 ====================

/**
 * @brief 更新所有子视图（瓦片）的位置
 *
 * 该方法根据当前内容偏移量和拖拽状态来计算并设置每个瓦片的显示位置。
 *
 * 工作模式：
 * 1. 静止状态（dragX/dragY ≈ 0）：所有瓦片按网格布局排列
 * 2. 拖拽状态：应用转场效果，仅更新源瓦片和目标瓦片
 */
void UITileView::UpdateChildrenPosition()
{
    int16_t width = GetWidth();
    int16_t height = GetHeight();

    int dragX = contentX_ - targetX_;
    int dragY = contentY_ - targetY_;
    // GRAPHIC_LOGI("UITileView::UpdateChildrenPosition dragX: %d, dragY: %d, contentX_: %d, contentY_: %d", dragX, dragY, contentX_, contentY_);
    // ========== 模式1：静止状态 ==========
    if (abs(dragX) <= 1 && abs(dragY) <= 1) {
        GRAPHIC_LOGI("UITileView::UpdateChildrenPosition dragX: %d, dragY: %d, contentX_: %d, contentY_: %d", dragX, dragY, contentX_, contentY_);
        // 所有瓦片按标准网格布局
        ListNode<TileInfo*>* node = tileList_.Begin();
        while (node != tileList_.End()) {
            TileInfo* info = node->data_;
            if (info && info->view) {
                PlaceTileAtGrid(info->view, info->col, info->row, width, height, contentX_, contentY_);
            }
            node = node->next_;
        }
        Invalidate();
        return;
    }

    // ========== 模式2：拖拽状态 ==========

    // 判断拖拽方向
    bool isHor = abs(dragX) > abs(dragY);
    int offset = isHor ? dragX : dragY;
    int range = isHor ? static_cast<int>(width) : static_cast<int>(height);

    // 计算目标瓦片坐标
    int dstCol = curCol_;
    int dstRow = curRow_;
    int transitionIdx = 0;

    if (isHor) {
        if (dragX < 0) {
            dstCol++;
            transitionIdx = 0;  // TDIR_LEFT
        } else {
            dstCol--;
            transitionIdx = 1;  // TDIR_RIGHT
        }
    } else {
        if (dragY < 0) {
            dstRow++;
            transitionIdx = 2;  // TDIR_TOP
        } else {
            dstRow--;
            transitionIdx = 3;  // TDIR_BOTTOM
        }
    }

    // 循环模式下的索引归一化
    if (loopHor_) {
        int maxC = maxCol_ + 1;
        if (maxC > 0) {
            dstCol = dstCol % maxC;
            if (dstCol < 0) {
                dstCol += maxC;
            }
        }
    }
    if (loopVer_) {
        int maxR = maxRow_ + 1;
        if (maxR > 0) {
            dstRow = dstRow % maxR;
            if (dstRow < 0) {
                dstRow += maxR;
            }
        }
    }

    // 查找源瓦片、目标瓦片和转场效果
    UIView* srcView = nullptr;
    UIView* dstView = nullptr;
    TileTransition* transition = nullptr;

    ListNode<TileInfo*>* node = tileList_.Begin();
    while (node != tileList_.End()) {
        TileInfo* info = node->data_;
        if (info && info->view) {
            bool isSrc = (info->col == curCol_ && info->row == curRow_);
            bool isDst = (info->col == dstCol && info->row == dstRow);

            if (isSrc) {
                srcView = info->view;
            } else if (isDst) {
                dstView = info->view;
            } else {
                PlaceTileAtGrid(info->view, info->col, info->row, width, height, contentX_, contentY_);
            }
            if (animSrcCol_ >= 0 && animSrcRow_ >= 0 && info->col == animSrcCol_ && info->row == animSrcRow_) {
                transition = info->transitions[transitionIdx];
            } else if (animSrcCol_ < 0 && animSrcRow_ < 0 && isSrc) {
                transition = info->transitions[transitionIdx];
            }
        }
        node = node->next_;
    }

    // 应用转场效果
    if (srcView != nullptr && dstView != nullptr && transition != nullptr) {
        transition->Apply(srcView, dstView, offset, range, isHor);
    }

    Invalidate();
}

void UITileView::Callback(UIView* view)
{
    (void)view;
    uint32_t runTime = animator_.GetRunTime();
    uint32_t duration = animator_.GetTime();
    if (duration == 0) {
        GRAPHIC_LOGE("UITileView::%s: invalid duration", __FUNCTION__);
        return;
    }
    if (runTime >= duration) {
        contentX_ = targetX_;
        contentY_ = targetY_;
    } else {
        contentX_ = easingFunc_(static_cast<int16_t>(startX_), static_cast<int16_t>(targetX_),
                                 static_cast<uint16_t>(runTime), static_cast<uint16_t>(duration));
        contentY_ = easingFunc_(static_cast<int16_t>(startY_), static_cast<int16_t>(targetY_),
                                 static_cast<uint16_t>(runTime), static_cast<uint16_t>(duration));
    }
    UpdateChildrenPosition();

    if (contentX_ == targetX_ && contentY_ == targetY_) {
        StopAnimation();
    }
}

bool UITileView::DragXInner(int16_t distance)
{
    if (distance == 0) {
        return true;
    }

    uint8_t allowedDir = GetAllowedDirection(curCol_, curRow_);
    if ((distance < 0 && (allowedDir & TDIR_LEFT) == 0) ||
        (distance > 0 && (allowedDir & TDIR_RIGHT) == 0)) {
        return false;
    }

    int newContentX = contentX_ + static_cast<int>(distance);
    if (!loopHor_) {
        int width = static_cast<int>(GetWidth());
        int minX = -static_cast<int>(maxCol_) * width;
        int maxX = 0;
        if (newContentX > maxX) {
            distance = static_cast<int16_t>(maxX - contentX_);
        } else if (newContentX < minX) {
            distance = static_cast<int16_t>(minX - contentX_);
        }
    }

    MoveChildByOffset(distance, 0);
    return true;
}

bool UITileView::DragYInner(int16_t distance)
{
    if (distance == 0) {
        return true;
    }

    uint8_t allowedDir = GetAllowedDirection(curCol_, curRow_);
    if ((distance < 0 && (allowedDir & TDIR_TOP) == 0) ||
        (distance > 0 && (allowedDir & TDIR_BOTTOM) == 0)) {
        return false;
    }

    int newContentY = contentY_ + static_cast<int>(distance);
    if (!loopVer_) {
        int height = static_cast<int>(GetHeight());
        int minY = -static_cast<int>(maxRow_) * height;
        int maxY = 0;
        if (newContentY > maxY) {
            distance = static_cast<int16_t>(maxY - contentY_);
        } else if (newContentY < minY) {
            distance = static_cast<int16_t>(minY - contentY_);
        }
    }

    MoveChildByOffset(0, distance);
    return true;
}

void UITileView::MoveChildByOffset(int16_t offsetX, int16_t offsetY)
{
    if (offsetX == 0 && offsetY == 0) {
        return;
    }
    contentX_ += static_cast<int>(offsetX);
    contentY_ += static_cast<int>(offsetY);

    if (loopHor_ && totalW_ > 0) {
        int totalW = static_cast<int>(totalW_);
        int halfW = totalW / 2;
        int mx = contentX_ % totalW;
        if (mx > halfW) {
            mx -= totalW;
        } else if (mx < -halfW) {
            mx += totalW;
        }
        contentX_ = mx;
    }
    if (loopVer_ && totalH_ > 0) {
        int totalH = static_cast<int>(totalH_);
        int halfH = totalH / 2;
        int my = contentY_ % totalH;
        if (my > halfH) {
            my -= totalH;
        } else if (my < -halfH) {
            my += totalH;
        }
        contentY_ = my;
    }

    UpdateChildrenPosition();
}

void UITileView::OnStop(UIView& view)
{
    (void)view;
    contentX_ = targetX_;
    contentY_ = targetY_;
    animSrcCol_ = -1;
    animSrcRow_ = -1;
    if (pendingNextCol_ >= 0 && pendingNextRow_ >= 0) {
        curCol_ = static_cast<uint8_t>(pendingNextCol_);
        curRow_ = static_cast<uint8_t>(pendingNextRow_);
        pendingNextCol_ = -1;
        pendingNextRow_ = -1;
    }
    UpdateChildrenPosition();
}

void UITileView::StopAnimation()
{
    if (animator_.GetState() != Animator::STOP) {
        animator_.Stop();
    }
}

UIView* UITileView::GetTile(uint8_t col, uint8_t row)
{
    ListNode<TileInfo*>* node = tileList_.Begin();
    while (node != tileList_.End()) {
        TileInfo* info = node->data_;
        if (info && info->col == col && info->row == row) {
            return info->view;
        }
        node = node->next_;
    }
    return nullptr;
}

uint8_t UITileView::GetAllowedDirection(uint8_t col, uint8_t row) const
{
    // 封装后的接口：返回指定瓦片的允许方向，默认允许全部
    TileInfo* info = const_cast<UITileView*>(this)->FindTileInfo(col, row);
    return (info == nullptr) ? TDIR_ALL : info->direction;
}

UITileView::TileInfo* UITileView::FindTileInfo(uint8_t col, uint8_t row) const
{
    // 统一的内部查找接口，集中处理链表遍历，调用方无需关心实现细节
    ListNode<TileInfo*>* node = const_cast<UITileView*>(this)->tileList_.Begin();
    while (node != const_cast<UITileView*>(this)->tileList_.End()) {
        TileInfo* info = node->data_;
        if (info && info->col == col && info->row == row) {
            return info;
        }
        node = node->next_;
    }
    return nullptr;
}

void UITileView::RecomputeTotalSpan()
{
    uint32_t w = static_cast<uint32_t>(GetWidth());
    uint32_t h = static_cast<uint32_t>(GetHeight());
    totalW_ = static_cast<uint32_t>(static_cast<uint32_t>(maxCol_ + 1) * w);
    totalH_ = static_cast<uint32_t>(static_cast<uint32_t>(maxRow_ + 1) * h);
}

void UITileView::PlaceTileAtGrid(UIView* view, uint8_t col, uint8_t row,
                                 uint32_t width, uint32_t height,
                                 int contentX, int contentY)
{
    if (view == nullptr) {
        return;
    }
    int rx = static_cast<int>(col) * static_cast<int>(width) + contentX;
    int ry = static_cast<int>(row) * static_cast<int>(height) + contentY;

    if (loopHor_ && totalW_ > 0) {
        int halfW = static_cast<int>(totalW_ / 2);
        int mx = rx % static_cast<int>(totalW_);
        if (mx > halfW) {
            mx -= static_cast<int>(totalW_);
        } else if (mx < -halfW) {
            mx += static_cast<int>(totalW_);
        }
        rx = mx;
    }
    if (loopVer_ && totalH_ > 0) {
        int halfH = static_cast<int>(totalH_ / 2);
        int my = ry % static_cast<int>(totalH_);
        if (my > halfH) {
            my -= static_cast<int>(totalH_);
        } else if (my < -halfH) {
            my += static_cast<int>(totalH_);
        }
        ry = my;
    }
    view->SetPosition(static_cast<int16_t>(rx), static_cast<int16_t>(ry));
}
} // 命名空间 OHOS
