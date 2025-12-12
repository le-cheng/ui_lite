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

#ifndef GRAPHIC_LITE_UI_TILE_VIEW_H
#define GRAPHIC_LITE_UI_TILE_VIEW_H

#include "components/ui_view_group.h"
#include "animator/animator.h"
#include "gfx_utils/list.h"
#include "animator/easing_equation.h"

namespace OHOS {
class UITileView : public UIViewGroup, public AnimatorCallback {
public:
    UITileView();
    virtual ~UITileView();

    /**
     * @brief Enumeration for drag directions.
     */
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

    /**
     * @brief Adds a view to the specified tile position.
     *
     * @param view The view to add.
     * @param col The column index.
     * @param row The row index.
     * @param direction The allowed drag direction for this tile (Bitmask of DragDirection).
     */
    void Add(UIView* view, uint8_t col, uint8_t row, uint8_t direction = TDIR_ALL);

    /**
     * @brief Adds a view to the default position (0, 0).
     *
     * @param view The view to add.
     */
    void Add(UIView* view) override;

    /**
     * @brief Sets the current active tile.
     *
     * @param col The column index.
     * @param row The row index.
     * @param animate Whether to animate the transition.
     */
    void SetCurrentTile(uint8_t col, uint8_t row, bool animate = true);

    /**
     * @brief Sets whether the tile view supports looping.
     *
     * @param loop Whether to enable looping (applies to both axes).
     */
    void SetLoop(bool loop);

    /**
     * @brief Sets whether the tile view supports looping for specific axes.
     *
     * @param hor Whether to enable horizontal looping.
     * @param ver Whether to enable vertical looping.
     */
    void SetLoop(bool hor, bool ver);

    /**
     * @brief Base class for transition effects.
     */
    class TileTransition {
    public:
        virtual ~TileTransition() {}
        /**
         * @brief Apply transition effect.
         *
         * @param srcView The current active view (staying or moving out).
         * @param dstView The target view (entering).
         * @param offset The drag offset (negative if dragging left/up, positive if dragging right/down).
         * @param range The total range of movement (width for horizontal, height for vertical).
         * @param isHorizontal True if the drag is horizontal.
         */
        virtual void Apply(UIView* srcView, UIView* dstView, int offset, int range, bool isHorizontal) = 0;
    };

    class SlideTransition : public TileTransition {
    public:
        void Apply(UIView* srcView, UIView* dstView, int offset, int range, bool isHorizontal) override;
    };

    class CoverTransition : public TileTransition {
    public:
        void Apply(UIView* srcView, UIView* dstView, int offset, int range, bool isHorizontal) override;
    };

    static SlideTransition defaultSlideTransition_;
    static CoverTransition defaultCoverTransition_;

    /**
     * @brief Sets the transition effect for a specific tile and direction.
     *
     * @param col The column index of the tile.
     * @param row The row index of the tile.
     * @param direction The drag direction to apply this transition (TDIR_LEFT, TDIR_RIGHT, TDIR_TOP, TDIR_BOTTOM).
     * @param transition Pointer to the transition effect.
     */
    void SetTransitionEffect(uint8_t col, uint8_t row, uint8_t direction, TileTransition* transition);

    /**
     * @brief 获取指定瓦片允许的拖拽方向（接口封装，避免遍历泄漏到调用处）
     */
    uint8_t GetAllowedDirection(uint8_t col, uint8_t row) const;

    /**
     * @brief Move a child view to the front (top) of the view group.
     * @param view The view to move.
     */
    void MoveChildToFront(UIView* view);

    bool OnDragStartEvent(const DragEvent& event) override;
    bool OnDragEvent(const DragEvent& event) override;
    bool OnDragEndEvent(const DragEvent& event) override;

    void Callback(UIView* view) override;
    void OnStop(UIView& view) override;

private:
    struct TileInfo {
        UIView* view;
        uint8_t col;
        uint8_t row;
        uint8_t direction; // Bitmask of DragDirection
        TileTransition* transitions[4]; // 0:Left, 1:Right, 2:Top, 3:Bottom
    };

    List<TileInfo*> tileList_;

    uint8_t curCol_;
    uint8_t curRow_;
    uint8_t maxCol_;
    uint8_t maxRow_;
    bool loopHor_;
    bool loopVer_;
    bool isLoop_; // Deprecated, kept for compatibility, effectively logic OR or maps to both? I'll replace usage.

    Animator animator_;

    bool isDragging_;

    // For axis locking during drag
    bool lockHorizontal_;

    UIView* GetTile(uint8_t col, uint8_t row);
    TileInfo* FindTileInfo(uint8_t col, uint8_t row) const;

    // Animation targets
    int targetX_;
    int targetY_;
    int startX_; // Current virtual offset of the container content
    int startY_;

    // Current virtual offset (scroll position)
    int contentX_;
    int contentY_;

    int animSrcCol_;
    int animSrcRow_;

    int pendingNextCol_;
    int pendingNextRow_;

    // 网格总跨度（像素），用于循环模式的坐标归一化
    uint32_t totalW_;
    uint32_t totalH_;

    void UpdateChildrenPosition();
    void RecomputeTotalSpan();
    void PlaceTileAtGrid(UIView* view, uint8_t col, uint8_t row,
                         uint32_t width, uint32_t height,
                         int contentX, int contentY);

    bool DragXInner(int16_t distance);
    bool DragYInner(int16_t distance);
    void MoveChildByOffset(int16_t offsetX, int16_t offsetY);

    void StopAnimation();
    EasingFunc easingFunc_;
};
} // namespace OHOS
#endif // GRAPHIC_LITE_UI_TILE_VIEW_H
