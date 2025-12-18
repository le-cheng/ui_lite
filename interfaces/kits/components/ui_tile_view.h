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

#include "animator/animator.h"
#include "animator/easing_equation.h"
#include "components/ui_view_group.h"

namespace OHOS {
/**
 * @brief Represents a tile view for 2D grid navigation.
 *
 * UITileView allows organizing child views in a 2D grid (rows and columns).
 * Users can navigate between tiles through drag gestures with axis locking
 * and automatic snapping to the nearest tile.
 *
 * @since 1.0
 * @version 1.0
 */
class UITileView : public UIViewGroup, public AnimatorCallback {
public:
    enum PageEffectType : uint8_t {
        PAGE_EFFECT_NONE = 0,
        PAGE_EFFECT_SCALE,
        PAGE_EFFECT_FADE,
        PAGE_EFFECT_SCALE_FADE,
        PAGE_EFFECT_COVER,
        PAGE_EFFECT_STATIC,
        PAGE_EFFECT_STATIC_SCALE,
        PAGE_EFFECT_AUTO
    };
    /**
     * @brief Drag direction flags for tiles.
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
     * @brief Listener for tile change events.
     */
    class OnTileChangeListener : public HeapBase {
    public:
        virtual void OnTileChange(UITileView& view, uint16_t col, uint16_t row) = 0;
        virtual ~OnTileChangeListener() {}
    };

    struct TileInfo {
        UIView* view;
        uint8_t allowedDir;
        PageEffectType enterEffects[4];
        PageEffectType exitEffects[4];
    };

    UITileView();
    virtual ~UITileView();

    UIViewType GetViewType() const override
    {
        return UI_TILE_VIEW;
    }

    /**
     * @brief Gets the number of columns in the grid.
     */
    uint16_t GetCols() const { return maxCols_; }

    /**
     * @brief Gets the number of rows in the grid.
     */
    uint16_t GetRows() const { return maxRows_; }

    /**
     * @brief Adds a tile at specified grid position.
     * Grid size will automatically expand to accommodate the tile.
     * @param view The view to add as a tile.
     * @param col Column index (0-based).
     * @param row Row index (0-based).
     */
    void AddTile(UIView* view, uint16_t col, uint16_t row);

    /**
     * @brief Removes a tile at specified grid position.
     */
    void RemoveTile(uint16_t col, uint16_t row);

    struct TransitionConfig {
        bool enableScale;
        bool enableFade;
    };

    TransitionConfig GetTransitionConfig(PageEffectType type) const;

    /**
     * @brief Gets the tile info at specified position.
     */
    void DoTransition(UIView* curView,
                      UIView* targetView,
                      PageEffectType curEffect,
                      PageEffectType targetEffect,
                      uint8_t direction);

    TileInfo* GetTile(uint16_t col, uint16_t row);

    /**
     * @brief Sets the current tile with optional animation.
     */
    void SetCurrentTile(uint16_t col, uint16_t row, bool needAnimator = false);

    /**
     * @brief Sets valid (allowed) drag directions for a specific tile.
     */
    void SetValidDirection(uint16_t col, uint16_t row, uint8_t direction);

    /**
     * @brief Gets valid drag directions for a specific tile.
     */
    uint8_t GetValidDirection(uint16_t col, uint16_t row) const;

    /**
     * @brief Sets invalid (forbidden) drag directions for a specific tile.
     */
    void SetInvalidDirection(uint16_t col, uint16_t row, uint8_t direction);

    /**
     * @brief Gets invalid drag directions for a specific tile.
     */
    uint8_t GetInvalidDirection(uint16_t col, uint16_t row) const;

    /**
     * @brief Sets horizontal loop mode.
     */
    void SetLoopHorizontal(bool loop);
    bool GetLoopHorizontal() const { return loopHor_; }

    /**
     * @brief Sets vertical loop mode.
     */
    void SetLoopVertical(bool loop);
    bool GetLoopVertical() const { return loopVer_; }

    /**
     * @brief Sets the tile change listener.
     */
    void SetOnTileChangeListener(OnTileChangeListener* listener) { tileChangeListener_ = listener; }
    OnTileChangeListener* GetOnTileChangeListener() const { return tileChangeListener_; }

    void SetTileEnterEffect(uint16_t col, uint16_t row, uint8_t directionMask, PageEffectType effect);
    void SetTileExitEffect(uint16_t col, uint16_t row, uint8_t directionMask, PageEffectType effect);

    PageEffectType GetTileEnterEffect(uint16_t col, uint16_t row, uint8_t direction);
    PageEffectType GetTileExitEffect(uint16_t col, uint16_t row, uint8_t direction);

    void SetGlobalEnterEffect(uint8_t directionMask, PageEffectType effect);
    void SetGlobalExitEffect(uint8_t directionMask, PageEffectType effect);

    /**
     * @brief Sets the animation duration.
     */
    void SetAnimatorTime(uint16_t time);

    void SetWidth(int16_t width) override
    {
        tileWidth_ = width;
        UIViewGroup::SetWidth(width);
    }

    void SetHeight(int16_t height) override
    {
        tileHeight_ = height;
        UIViewGroup::SetHeight(height);
    }

    bool OnDragStartEvent(const DragEvent& event) override;
    bool OnDragEvent(const DragEvent& event) override;
    bool OnDragEndEvent(const DragEvent& event) override;
    PageEffectType globalEnterEffects_[4];
    PageEffectType globalExitEffects_[4];

protected:
    static constexpr uint16_t MAX_COLS = 8;
    static constexpr uint16_t MAX_ROWS = 8;
    static constexpr uint16_t MAX_TILES = MAX_COLS * MAX_ROWS;
    static constexpr int16_t SNAP_THRESHOLD_RATIO = 2;
    static constexpr int16_t THROW_THRESHOLD = 20;
    static constexpr uint8_t HORIZONTAL = 0;
    static constexpr uint8_t VERTICAL = 1;
    static constexpr uint8_t HORIZONTAL_AND_VERTICAL = 2;

    bool DragXInner(int16_t distance);
    bool DragYInner(int16_t distance);
    void StopAnimator();


private:
    // Animator callback methods
    void SetDragStartValue(int16_t startValueX, int16_t startValueY);
    void SetDragEndValue(int16_t endValueX, int16_t endValueY);
    void ResetCallback();
    void ResetEffect();
    void Callback(UIView* view) override;
    void OnStop(UIView& view) override;

    // Tile management
    uint16_t GetTileIndex(uint16_t col, uint16_t row) const;
    void ExpandGridIfNeeded(uint16_t col, uint16_t row);
    bool HasTile(uint16_t col, uint16_t row) const;
    UIView* GetTileView(uint16_t col, uint16_t row);

    // Direction management
    uint8_t GetAllowedDirection(uint16_t col, uint16_t row) const;
    void UpdateTileDirection(uint16_t col, uint16_t row);
    void UpdateNeighborTile(uint16_t col, uint16_t row, int16_t deltaCol, int16_t deltaRow, bool isHorizontal);
    void UpdateTileAndNeighbors(uint16_t col, uint16_t row);
    void UpdateAllTileDirections();

    // Layout and positioning
    void LayoutTiles();
    void ApplyTransitionEffect();
    int16_t NormalizeOffsetLoop(int16_t pos, int16_t itemSize, uint16_t count) const;
    int16_t NormalizeContentLoop(int16_t pos, int16_t itemSize, uint16_t count) const;
    int16_t NormalizeIndex(int16_t index, int16_t max, bool loop) const;

    // Navigation
    void SwitchToTile(uint16_t col, uint16_t row, bool needAnimator = true);
    void UpdateCurrentTileByThrow(int16_t distanceX, int16_t distanceY, uint16_t& targetCol, uint16_t& targetRow) const;
    void NotifyTileChange();
    void UpdateTransitionZIndex(UIView* curView, UIView* targetView, PageEffectType curEffect, PageEffectType targetEffect);
    void RestoreTransitionZIndex();
    bool IsCoverLikeEffect(PageEffectType effect) const;
    void UpdateDirAndZIndex();
    void UpdateDirAndZIndex(int pos);
    bool ShouldUpdateDir(int16_t pos);
    void UpdateLastDelta(int16_t delta);
    void ResetDragDelta();
    int16_t GetMaxAbsLastDelta() const;
    bool GetTransitionEnterInfo(int16_t pos, uint16_t& enterCol, uint16_t& enterRow, uint8_t& effectiveDirection) const;
    bool GetOnScreenTransitionViews(UIView*& exitView,
                                   UIView*& enterView,
                                   uint16_t& enterCol,
                                   uint16_t& enterRow,
                                   uint8_t& effectiveDirection);

    // Member variables
    TileInfo tiles_[MAX_TILES];
    uint16_t maxCols_;
    uint16_t maxRows_;
    uint16_t curCol_;
    uint16_t curRow_;
    UIView* curView_;
    uint16_t targetCol_;
    uint16_t targetRow_;
    int16_t contentX_;
    int16_t contentY_;
    int16_t targetX_;
    int16_t targetY_;
    int16_t tileWidth_;
    int16_t tileHeight_;
    int16_t lastDelta_[3];
    int16_t startValueX_;
    int16_t endValueX_;
    int16_t previousValueX_;
    int16_t startValueY_;
    int16_t endValueY_;
    int16_t previousValueY_;
    EasingFunc easingFunc_;
    Animator scrollAnimator_;
    uint8_t direction_;
    uint8_t deltaIndex_;
    bool loopHor_;
    bool loopVer_;
    bool axisLocked_;
    bool isHorizontalDrag_;
    OnTileChangeListener* tileChangeListener_;
    bool transitionZIndexActive_;
    UIView* transitionCurView_;
    UIView* transitionTargetView_;
    int16_t transitionCurZIndex_;
    int16_t transitionTargetZIndex_;
    PageEffectType enterEffect_;
    PageEffectType exitEffect_;

    int8_t oldDir_;


#if defined(ENABLE_ROTATE_INPUT) && ENABLE_ROTATE_INPUT
    int16_t lastRotate_[3];
    float rotateFactor_;
    bool isRotating_;
#endif
};
} // namespace OHOS
#endif // GRAPHIC_LITE_UI_TILE_VIEW_H
