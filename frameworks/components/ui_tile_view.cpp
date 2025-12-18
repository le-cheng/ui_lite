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
#include "securec.h"
#if defined(ENABLE_ROTATE_INPUT) && ENABLE_ROTATE_INPUT
#include "events/rotate_event.h"
#endif
#define VERBOSE 0

namespace OHOS {

UITileView::UITileView()
    : maxCols_(1),
      maxRows_(1),
      curCol_(0),
      curRow_(0),
      curView_(nullptr),
      targetCol_(0),
      targetRow_(0),
      contentX_(0),
      contentY_(0),
      targetX_(0),
      targetY_(0),
      tileWidth_(0),
      tileHeight_(0),
      startValueX_(0),
      endValueX_(0),
      previousValueX_(0),
      startValueY_(0),
      endValueY_(0),
      previousValueY_(0),
      easingFunc_(EasingEquation::CubicEaseOut),
      scrollAnimator_(this, this, 350, false),
      direction_(TDIR_NONE),
      deltaIndex_(0),
      loopHor_(false),
      loopVer_(false),
      axisLocked_(false),
      isHorizontalDrag_(true),
      tileChangeListener_(nullptr),
      transitionZIndexActive_(false),
      transitionCurView_(nullptr),
      transitionTargetView_(nullptr),
      transitionCurZIndex_(0),
      transitionTargetZIndex_(0),
      enterEffect_(PAGE_EFFECT_AUTO),
      exitEffect_(PAGE_EFFECT_AUTO),
      oldDir_(0)
{
    isViewGroup_ = true;
    touchable_ = true;
    draggable_ = true;
    dragParentInstead_ = false;

    if (memset_s(tiles_, sizeof(tiles_), 0, sizeof(tiles_)) != EOK) {
        GRAPHIC_LOGE("UITileView memset_s failed");
    }
    for (uint16_t i = 0; i < MAX_TILES; i++) {
        tiles_[i].allowedDir = TDIR_NONE;
        tiles_[i].view = nullptr;
        for (uint8_t d = 0; d < 4; d++) {
            tiles_[i].enterEffects[d] = PAGE_EFFECT_NONE;
            tiles_[i].exitEffects[d] = PAGE_EFFECT_NONE;
        }
    }
    easingFunc_ = EasingEquation::CubicEaseOut;
    for (uint8_t d = 0; d < 4; d++) {
        globalEnterEffects_[d] = PAGE_EFFECT_NONE;
        globalExitEffects_[d] = PAGE_EFFECT_NONE;
    }

    ResetDragDelta();
}

UITileView::~UITileView()
{
    for (uint16_t i = 0; i < MAX_TILES; i++) {
        tiles_[i].view = nullptr;
    }
}

// ============================================================================
// Tile Management
// ============================================================================

uint16_t UITileView::GetTileIndex(uint16_t col, uint16_t row) const
{
    if (col >= MAX_COLS || row >= MAX_ROWS) {
        return MAX_TILES;
    }
    return row * MAX_COLS + col;
}

void UITileView::ExpandGridIfNeeded(uint16_t col, uint16_t row)
{
    if (col >= maxCols_) {
        maxCols_ = col + 1;
    }
    if (row >= maxRows_) {
        maxRows_ = row + 1;
    }
}

void UITileView::AddTile(UIView* view, uint16_t col, uint16_t row)
{
    if (view == nullptr || col >= MAX_COLS || row >= MAX_ROWS) {
        return;
    }

    uint16_t index = GetTileIndex(col, row);
    if (index >= MAX_TILES) {
        return;
    }

    if (tiles_[index].view != nullptr) {
        GRAPHIC_LOGE("AddTile: tile at (%d,%d) already exists", col, row);
        return;
    }

    ExpandGridIfNeeded(col, row);

    view->SetDragParentInstead(true);
    UIViewGroup::Add(view);
    tiles_[index].view = view;

    UpdateTileAndNeighbors(col, row);

#if VERBOSE
    for (uint16_t i = 0; i < MAX_TILES; i++) {
        if (tiles_[i].allowedDir != 0) {
            GRAPHIC_LOGI("allowedDir[%d]=%u", i, tiles_[i].allowedDir);
        }
    }
    GRAPHIC_LOGI("AddTile: col=%u, row=%u, index=%u", col, row, index);
#endif
    SetCurrentTile(col, row, false);
    // LayoutTiles();
    Invalidate();
}

void UITileView::RemoveTile(uint16_t col, uint16_t row)
{
    uint16_t index = GetTileIndex(col, row);
    if (index >= MAX_TILES || tiles_[index].view == nullptr) {
        return;
    }

    UIViewGroup::Remove(tiles_[index].view);
    tiles_[index].view = nullptr;

    UpdateTileAndNeighbors(col, row);
    Invalidate();
}

UITileView::TileInfo* UITileView::GetTile(uint16_t col, uint16_t row)
{
    uint16_t index = GetTileIndex(col, row);
    if (index >= MAX_TILES || tiles_[index].view == nullptr) { // TODO: check if it is safe to return nullptr
        return nullptr;
    }
    return &tiles_[index];
}

UIView* UITileView::GetTileView(uint16_t col, uint16_t row)
{
    TileInfo* tile = GetTile(col, row);
    if (tile == nullptr) {
        return nullptr;
    }
    return tile->view;
}

bool UITileView::HasTile(uint16_t col, uint16_t row) const
{
    uint16_t index = GetTileIndex(col, row);
    if (index >= MAX_TILES || tiles_[index].view == nullptr) {
        return false;
    }
    return true;
}

// ============================================================================
// Direction Management
// ============================================================================

void UITileView::SetValidDirection(uint16_t col, uint16_t row, uint8_t direction)
{
    uint16_t index = GetTileIndex(col, row);
    if (index >= MAX_TILES) {
        return;
    }
    tiles_[index].allowedDir = direction;
}

uint8_t UITileView::GetValidDirection(uint16_t col, uint16_t row) const
{
    uint16_t index = GetTileIndex(col, row);
    if (index >= MAX_TILES) {
        return TDIR_NONE;
    }
    return tiles_[index].allowedDir;
}

void UITileView::SetInvalidDirection(uint16_t col, uint16_t row, uint8_t direction)
{
    uint16_t index = GetTileIndex(col, row);
    if (index >= MAX_TILES) {
        return;
    }
    tiles_[index].allowedDir = tiles_[index].allowedDir & (~direction);
}

uint8_t UITileView::GetInvalidDirection(uint16_t col, uint16_t row) const
{
    uint16_t index = GetTileIndex(col, row);
    if (index >= MAX_TILES) {
        return TDIR_ALL;
    }
    return TDIR_ALL & (~tiles_[index].allowedDir);
}

uint8_t UITileView::GetAllowedDirection(uint16_t col, uint16_t row) const
{
    uint16_t index = GetTileIndex(col, row);
    if (index >= MAX_TILES) {
        return TDIR_NONE;
    }
    return tiles_[index].allowedDir;
}

void UITileView::UpdateTileDirection(uint16_t col, uint16_t row)
{
    if (!HasTile(col, row)) {
        return;
    }

    uint8_t allowed = TDIR_NONE;

    // Check right neighbor (allows dragging LEFT)
    bool hasRight = false;
    if (col < maxCols_ - 1) {
        hasRight = HasTile(col + 1, row);
    } else if (loopHor_ && maxCols_ > 1) {
        hasRight = HasTile(0, row);
    }
    if (hasRight) {
        allowed |= TDIR_LEFT;
    }

    // Check left neighbor (allows dragging RIGHT)
    bool hasLeft = false;
    if (col > 0) {
        hasLeft = HasTile(col - 1, row);
    } else if (loopHor_ && maxCols_ > 1) {
        hasLeft = HasTile(maxCols_ - 1, row);
    }
    if (hasLeft) {
        allowed |= TDIR_RIGHT;
    }

    // Check bottom neighbor (allows dragging UP/TOP)
    bool hasBottom = false;
    if (row < maxRows_ - 1) {
        hasBottom = HasTile(col, row + 1);
    } else if (loopVer_ && maxRows_ > 1) {
        hasBottom = HasTile(col, 0);
    }
    if (hasBottom) {
        allowed |= TDIR_TOP;
    }

    // Check top neighbor (allows dragging DOWN/BOTTOM)
    bool hasTop = false;
    if (row > 0) {
        hasTop = HasTile(col, row - 1);
    } else if (loopVer_ && maxRows_ > 1) {
        hasTop = HasTile(col, maxRows_ - 1);
    }
    if (hasTop) {
        allowed |= TDIR_BOTTOM;
    }

    SetValidDirection(col, row, allowed);
}

void UITileView::UpdateNeighborTile(uint16_t col, uint16_t row, int16_t deltaCol, int16_t deltaRow, bool isHorizontal)
{
    int16_t neighborCol = col + deltaCol;
    int16_t neighborRow = row + deltaRow;

    if (isHorizontal) {
        if (neighborCol < 0 || neighborCol >= static_cast<int16_t>(maxCols_)) {
            if (loopHor_ && maxCols_ > 1) {
                neighborCol = (neighborCol + maxCols_) % maxCols_;
            } else {
                return;
            }
        }
    } else {
        if (neighborRow < 0 || neighborRow >= static_cast<int16_t>(maxRows_)) {
            if (loopVer_ && maxRows_ > 1) {
                neighborRow = (neighborRow + maxRows_) % maxRows_;
            } else {
                return;
            }
        }
    }

    UpdateTileDirection(static_cast<uint16_t>(neighborCol), static_cast<uint16_t>(neighborRow));
}

void UITileView::UpdateTileAndNeighbors(uint16_t col, uint16_t row)
{
    UpdateTileDirection(col, row);
    UpdateNeighborTile(col, row, -1, 0, true);  // Left
    UpdateNeighborTile(col, row, 1, 0, true);   // Right
    UpdateNeighborTile(col, row, 0, -1, false); // Top
    UpdateNeighborTile(col, row, 0, 1, false);  // Bottom
}

void UITileView::UpdateAllTileDirections()
{
    for (uint16_t row = 0; row < maxRows_; row++) {
        for (uint16_t col = 0; col < maxCols_; col++) {
            UpdateTileDirection(col, row);
        }
    }
}

void UITileView::SetLoopHorizontal(bool loop)
{
    loopHor_ = loop;
    UpdateAllTileDirections();
}

void UITileView::SetLoopVertical(bool loop)
{
    loopVer_ = loop;
    UpdateAllTileDirections();
}

// ============================================================================
// Layout and Positioning
// ============================================================================

int16_t UITileView::NormalizeOffsetLoop(int16_t pos, int16_t itemSize, uint16_t count) const
{
    if (count <= 1 || itemSize == 0) {
        return pos;
    }

    int32_t grid = static_cast<int32_t>(itemSize) * static_cast<int32_t>(count);
    int32_t threshold = grid - itemSize;

    if (pos < -threshold) {
        pos += grid;
    } else if (pos > threshold) {
        pos -= grid;
    }

    return pos;
}

int16_t UITileView::NormalizeContentLoop(int16_t pos, int16_t itemSize, uint16_t count) const
{
    if (count <= 1 || itemSize == 0) {
        return pos;
    }

    int32_t grid = static_cast<int32_t>(itemSize) * static_cast<int32_t>(count);
    int32_t threshold = grid - itemSize;

    if (pos < -threshold) {
        pos += grid;
    } else if (pos > 0) {
        pos -= grid;
    }

    return pos;
}

void UITileView::LayoutTiles()
{
    int16_t tileWidth = tileWidth_;
    int16_t tileHeight = tileHeight_;

    for (uint16_t row = 0; row < maxRows_; row++) {
        for (uint16_t col = 0; col < maxCols_; col++) {
            uint16_t index = GetTileIndex(col, row);
            if (index < MAX_TILES && tiles_[index].view != nullptr) {
                int16_t x = contentX_ + col * tileWidth;
                int16_t y = contentY_ + row * tileHeight;

                if (loopHor_ && maxCols_ > 1) {
                    x = NormalizeOffsetLoop(x, tileWidth, maxCols_);
                }
                if (loopVer_ && maxRows_ > 1) {
                    y = NormalizeOffsetLoop(y, tileHeight, maxRows_);
                }

                tiles_[index].view->SetPosition(x, y, tileWidth, tileHeight);
            }
        }
    }
    ResetEffect();
    ApplyTransitionEffect();
}

static uint8_t GetDirectionIndex(uint8_t direction)
{
    if (direction == UITileView::TDIR_LEFT) {
        return 0;
    }
    if (direction == UITileView::TDIR_RIGHT) {
        return 1;
    }
    if (direction == UITileView::TDIR_TOP) {
        return 2;
    }
    if (direction == UITileView::TDIR_BOTTOM) {
        return 3;
    }
    return 0;
}

void UITileView::SetTileEnterEffect(uint16_t col, uint16_t row, uint8_t directionMask, PageEffectType effect)
{
    TileInfo* tile = GetTile(col, row);
    if (tile == nullptr) {
        return;
    }

    for (uint8_t d = 0; d < 4; d++) {
        uint8_t dirFlag = 0;
        if (d == 0) {
            dirFlag = TDIR_LEFT;
        } else if (d == 1) {
            dirFlag = TDIR_RIGHT;
        } else if (d == 2) {
            dirFlag = TDIR_TOP;
        } else {
            dirFlag = TDIR_BOTTOM;
        }
        if (directionMask & dirFlag) {
            tile->enterEffects[d] = effect;
        } else {
            // tile->enterEffects[d] = PAGE_EFFECT_NONE;
        }
    }
}

void UITileView::SetTileExitEffect(uint16_t col, uint16_t row, uint8_t directionMask, PageEffectType effect)
{
    TileInfo* tile = GetTile(col, row);
    if (tile == nullptr) {
        return;
    }
    for (uint8_t d = 0; d < 4; d++) {
        uint8_t dirFlag = 0;
        if (d == 0) {
            dirFlag = TDIR_LEFT;
        } else if (d == 1) {
            dirFlag = TDIR_RIGHT;
        } else if (d == 2) {
            dirFlag = TDIR_TOP;
        } else {
            dirFlag = TDIR_BOTTOM;
        }
        if (directionMask & dirFlag) {
            tile->exitEffects[d] = effect;
        } else {
            // tile->exitEffects[d] = PAGE_EFFECT_NONE;
        }
    }
}

UITileView::PageEffectType UITileView::GetTileEnterEffect(uint16_t col, uint16_t row, uint8_t direction)
{
    TileInfo* tile = GetTile(col, row);
    if (tile == nullptr) {
        return PAGE_EFFECT_NONE;
    }
    uint8_t dirIndex = GetDirectionIndex(direction);
    PageEffectType effect = tile->enterEffects[dirIndex];
    if (effect != PAGE_EFFECT_NONE) {
        return effect;
    }
    PageEffectType globalEffect = globalEnterEffects_[dirIndex];
    return globalEffect;
}

UITileView::PageEffectType UITileView::GetTileExitEffect(uint16_t col, uint16_t row, uint8_t direction)
{
    TileInfo* tile = GetTile(col, row);
    if (tile == nullptr) {
        return PAGE_EFFECT_NONE;
    }
    uint8_t dirIndex = GetDirectionIndex(direction);
    PageEffectType effect = tile->exitEffects[dirIndex];
    if (effect != PAGE_EFFECT_NONE) {
        return effect;
    }
    PageEffectType globalEffect = globalExitEffects_[dirIndex];
    return globalEffect;
}

void UITileView::SetGlobalEnterEffect(uint8_t directionMask, PageEffectType effect)
{
    for (uint8_t d = 0; d < 4; d++) {
        uint8_t dirFlag = 0;
        if (d == 0) {
            dirFlag = TDIR_LEFT;
        } else if (d == 1) {
            dirFlag = TDIR_RIGHT;
        } else if (d == 2) {
            dirFlag = TDIR_TOP;
        } else {
            dirFlag = TDIR_BOTTOM;
        }
        if (directionMask & dirFlag) {
            globalEnterEffects_[d] = effect;
        } else {
            globalEnterEffects_[d] = PAGE_EFFECT_NONE;
        }
    }
}

void UITileView::SetGlobalExitEffect(uint8_t directionMask, PageEffectType effect)
{
    for (uint8_t d = 0; d < 4; d++) {
        uint8_t dirFlag = 0;
        if (d == 0) {
            dirFlag = TDIR_LEFT;
        } else if (d == 1) {
            dirFlag = TDIR_RIGHT;
        } else if (d == 2) {
            dirFlag = TDIR_TOP;
        } else {
            dirFlag = TDIR_BOTTOM;
        }
        if (directionMask & dirFlag) {
            globalExitEffects_[d] = effect;
        } else {
            globalExitEffects_[d] = PAGE_EFFECT_NONE;
        }
    }
}

UITileView::TransitionConfig UITileView::GetTransitionConfig(PageEffectType type) const
{
    TransitionConfig config = {false, false};
    switch (type) {
        case PAGE_EFFECT_SCALE:
            config.enableScale = true;
            break;
        case PAGE_EFFECT_FADE:
            config.enableFade = true;
            break;
        case PAGE_EFFECT_SCALE_FADE:
            config.enableScale = true;
            config.enableFade = true;
            break;
        case PAGE_EFFECT_COVER:
            config.enableScale = true;
            break;
        case PAGE_EFFECT_AUTO:
        case PAGE_EFFECT_NONE:
        default:
            break;
    }
    return config;
}

bool UITileView::IsCoverLikeEffect(PageEffectType effect) const
{
    return (effect == PAGE_EFFECT_STATIC) || (effect == PAGE_EFFECT_STATIC_SCALE);
}

void UITileView::RestoreTransitionZIndex()
{
    if (!transitionZIndexActive_) {
        return;
    }
    if (transitionCurView_ != nullptr) {
        transitionCurView_->SetZIndex(transitionCurZIndex_);
    }
    if (transitionTargetView_ != nullptr) {
        transitionTargetView_->SetZIndex(transitionTargetZIndex_);
    }
    transitionZIndexActive_ = false;
    transitionCurView_ = nullptr;
    transitionTargetView_ = nullptr;
}

void UITileView::UpdateTransitionZIndex(UIView* curView,
                                       UIView* targetView,
                                       PageEffectType curEffect,
                                       PageEffectType targetEffect)
{
    if ((curView == nullptr) || (targetView == nullptr) || (curView == targetView)) {
        RestoreTransitionZIndex();
        return;
    }
    GRAPHIC_LOGD("%s: %p, %p, %d, %d", __func__, curView, targetView, curEffect, targetEffect);

    bool curStatic = IsCoverLikeEffect(curEffect);
    bool targetStatic = IsCoverLikeEffect(targetEffect);
    if (curStatic == targetStatic) {
        RestoreTransitionZIndex();
        return;
    }

    if (transitionZIndexActive_ && ((transitionCurView_ != curView) || (transitionTargetView_ != targetView))) {
        RestoreTransitionZIndex();
    }

    if (!transitionZIndexActive_) {
        transitionCurView_ = curView;
        transitionTargetView_ = targetView;
        transitionCurZIndex_ = curView->GetZIndex();
        transitionTargetZIndex_ = targetView->GetZIndex();
        transitionZIndexActive_ = true;
    }

    UIView* topView = nullptr;
    UIView* bottomView = nullptr;
    if (curStatic) {
        bottomView = curView;
        topView = targetView;
        GRAPHIC_LOGD("%s: %p, %p topView is targetView", __func__, bottomView, topView);
    } else {
        bottomView = targetView;
        topView = curView;
        GRAPHIC_LOGD("%s: %p, %p topView is curView", __func__, bottomView, topView);
    }

    int32_t base = MATH_MIN(transitionCurZIndex_, transitionTargetZIndex_);
    int16_t topZIndex = 0;
    int16_t bottomZIndex = 0;
    if (base >= 32767) {
        topZIndex = 32767;
        bottomZIndex = 32766;
    } else {
        bottomZIndex = static_cast<int16_t>(base);
        topZIndex = static_cast<int16_t>(base + 1);
    }
    GRAPHIC_LOGD("%s: %d, %d", __func__, bottomZIndex, topZIndex);
    bottomView->SetZIndex(bottomZIndex);
    topView->SetZIndex(topZIndex);
}

void UITileView::UpdateDirAndZIndex()
{
    GRAPHIC_LOGD("%s", __func__);
    if (curView_ == nullptr) {
        return;
    }
    if (direction_ & TDIR_HOR) {
        int curX = curView_->GetX();
        UpdateDirAndZIndex(curX);
    } else if (direction_ & TDIR_VER) {
        int curY = curView_->GetY();
        UpdateDirAndZIndex(curY);
    } else {
        return;
    }
}

bool UITileView::ShouldUpdateDir(int16_t pos)
{
    int8_t dir = 0;
    if (pos < 0) {
        dir = -1;
    } else if (pos > 0) {
        dir = 1;
    }
    if ((dir != 0) && (dir != oldDir_)) {
        oldDir_ = dir;
        return true;
    }
    return false;
}

void UITileView::ResetDragDelta()
{
    for (uint8_t i = 0; i < 3; i++) {
        lastDelta_[i] = 0;
    }
    deltaIndex_ = 0;
}

int16_t UITileView::GetMaxAbsLastDelta() const
{
    int16_t maxDelta = 0;
    for (uint8_t i = 0; i < 3; i++) {
        int16_t absDelta = MATH_ABS(lastDelta_[i]);
        if (maxDelta < absDelta) {
            maxDelta = absDelta;
        }
    }
    return maxDelta;
}

void UITileView::UpdateLastDelta(int16_t delta)
{
    lastDelta_[deltaIndex_ % 3] = delta;
    deltaIndex_ = (deltaIndex_ + 1) % 3;
}

bool UITileView::GetTransitionEnterInfo(int16_t pos,
                                       uint16_t& enterCol,
                                       uint16_t& enterRow,
                                       uint8_t& effectiveDirection) const
{
    enterCol = curCol_;
    enterRow = curRow_;
    effectiveDirection = TDIR_NONE;

    if (pos == 0) {
        return false;
    }

    if (direction_ & TDIR_HOR) {
        if (pos < 0) {
            effectiveDirection = TDIR_LEFT;
            enterCol = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curCol_) + 1,
                                                           static_cast<int16_t>(maxCols_),
                                                           loopHor_));
            return true;
        }
        if (pos > 0) {
            effectiveDirection = TDIR_RIGHT;
            enterCol = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curCol_) - 1,
                                                           static_cast<int16_t>(maxCols_),
                                                           loopHor_));
            return true;
        }
        return false;
    }

    else if (direction_ & TDIR_VER) {
        if (pos < 0) {
            effectiveDirection = TDIR_TOP;
            enterRow = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curRow_) + 1,
                                                           static_cast<int16_t>(maxRows_),
                                                           loopVer_));
            return true;
        }
        if (pos > 0) {
            effectiveDirection = TDIR_BOTTOM;
            enterRow = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curRow_) - 1,
                                                           static_cast<int16_t>(maxRows_),
                                                           loopVer_));
            return true;
        }
        return false;
    }

    return false;
}

bool UITileView::GetOnScreenTransitionViews(UIView*& exitView,
                                            UIView*& enterView,
                                            uint16_t& enterCol,
                                            uint16_t& enterRow,
                                            uint8_t& effectiveDirection)
{
    exitView = curView_;
    enterView = nullptr;
    enterCol = curCol_;
    enterRow = curRow_;
    effectiveDirection = TDIR_NONE;
    int16_t pos = 0;
    float fullSize = 0.0f;

    if (exitView == nullptr || tileWidth_ == 0 || tileHeight_ == 0) {
        return false;
    }

    if (direction_ & TDIR_HOR) {
        pos = exitView->GetX();
        fullSize = static_cast<float>(tileWidth_);
    } else if (direction_ & TDIR_VER) {
        pos = exitView->GetY();
        fullSize = static_cast<float>(tileHeight_);
    } else {
        return false;
    }

    if (pos < -fullSize || pos > fullSize || pos == 0) {
        return false;
    }

    if (!GetTransitionEnterInfo(pos, enterCol, enterRow, effectiveDirection)) {
        return false;
    }

    enterView = GetTileView(enterCol, enterRow);
    if (enterView == nullptr || enterView == exitView) {
        enterView = nullptr;
        return false;
    }
    return true;
}

void UITileView::UpdateDirAndZIndex(int pos)
{
    if (pos == 0) {
        return;
    }
    GRAPHIC_LOGD("%s: %d, %d, %d", __func__, curCol_, curRow_, direction_);

    uint16_t enterCol = curCol_;
    uint16_t enterRow = curRow_;
    uint8_t effectiveDirection = TDIR_NONE;
    if (!GetTransitionEnterInfo(pos, enterCol, enterRow, effectiveDirection)) {
        return;
    }

    direction_ = effectiveDirection;

    UIView* targetView = GetTileView(enterCol, enterRow);
    if (targetView == nullptr) {
        return;
    }

    enterEffect_ = GetTileEnterEffect(enterCol, enterRow, direction_);
    exitEffect_  = GetTileExitEffect(curCol_, curRow_, direction_);
    UpdateTransitionZIndex(curView_, targetView, exitEffect_, enterEffect_);
}

void UITileView::DoTransition(UIView* curView,
                              UIView* targetView,
                              PageEffectType curEffect,
                              PageEffectType targetEffect,
                              uint8_t direction)
{
    if (curView == nullptr || targetView == nullptr) {
        return;
    }
    // GRAPHIC_LOGD("%s: direction=%d", __func__, direction);
    int16_t curPos = 0;
    int16_t targetPos = 0;
    float fullSize = 0.0f;
    if (direction & TDIR_HOR) {
        targetPos = targetView->GetX();
        curPos = curView->GetX();
        fullSize = static_cast<float>(tileWidth_);
    } else {
        targetPos = targetView->GetY();
        curPos = curView->GetY();
        fullSize = static_cast<float>(tileHeight_);
    }

    float ratio = static_cast<float>(curPos) / fullSize;
    float absRatio = MATH_ABS(ratio);

    float targetRatio = 0.0f;
    if (ratio < 0) {
        targetRatio = ratio + 1.0f;
    } else {
        targetRatio = ratio - 1.0f;
    }
    float targetAbsRatio = MATH_ABS(targetRatio);

    GRAPHIC_LOGD(" %f %f", ratio, targetRatio);

    curView->ResetTransParameter();
    targetView->ResetTransParameter();

    // 1. Current Tile (Exiting)
    switch (curEffect) {
        case PAGE_EFFECT_SCALE:
        {
            GRAPHIC_LOGD("Current Tile PAGE_EFFECT_SCALE: pos=%d", curPos);
            float minScale = 0.5f;
            float scale = minScale + (1.0f - minScale) * (1.0f - absRatio);
            float pivotX = fullSize * 0.5f;
            float pivotY = fullSize * 0.5f;
            curView->Scale(Vector2<float>(scale, scale), Vector2<float>(pivotX, pivotY));
            break;
        }

        case PAGE_EFFECT_FADE:
        {
            GRAPHIC_LOGD("Current Tile PAGE_EFFECT_FADE: pos=%d", curPos);
            float minFactor = 0.3f;
            float factor = minFactor + (1.0f - minFactor) * (1.0f - absRatio);
            curView->SetOpaScale(static_cast<uint8_t>(factor * OPA_OPAQUE));
            break;
        }
        case PAGE_EFFECT_STATIC:
        {
            // GRAPHIC_LOGD("Current Tile PAGE_EFFECT_COVER: pos=%d", curPos);
            int16_t offsetX = 0;
            int16_t offsetY = 0;
            if (direction & TDIR_HOR) {
                offsetX = -curPos;
            } else {
                offsetY = -curPos;
            }
            curView->Translate(Vector2<int16_t>(offsetX, offsetY));
            break;
        }
        case PAGE_EFFECT_STATIC_SCALE:
        {
            // GRAPHIC_LOGD("Current Tile PAGE_EFFECT_STATIC_SCALE: pos=%d", curPos);
            float minScale = 0.8f;
            float scale = minScale + (1.0f - minScale) * (1.0f - absRatio);
            float pivotX = fullSize * 0.5f;
            float pivotY = fullSize * 0.5f;
            curView->Scale(Vector2<float>(scale, scale), Vector2<float>(pivotX, pivotY));

            int16_t offsetX = 0;
            int16_t offsetY = 0;
            if (direction & TDIR_HOR) {
                offsetX = -curPos;
            } else {
                offsetY = -curPos;
            }
            curView->Translate(Vector2<int16_t>(offsetX, offsetY));
            break;
        }
        case PAGE_EFFECT_SCALE_FADE:
        {
            GRAPHIC_LOGD("Current Tile PAGE_EFFECT_SCALE_FADE: pos=%d", curPos);
            float minScale = 0.8f;
            float scale = minScale + (1.0f - minScale) * (1.0f - absRatio);
            float pivotX = fullSize * 0.5f;
            float pivotY = fullSize * 0.5f;
            curView->Scale(Vector2<float>(scale, scale), Vector2<float>(pivotX, pivotY));

            float minFactor = 0.3f;
            float factor = minFactor + (1.0f - minFactor) * (1.0f - absRatio);
            curView->SetOpaScale(static_cast<uint8_t>(factor * OPA_OPAQUE));
            break;
        }
        case PAGE_EFFECT_AUTO:
            GRAPHIC_LOGD("Current Tile PAGE_EFFECT_AUTO: pos=%d", curPos);
        default:
            break;
    }

    // 2. Target Tile (Entering)
    switch (targetEffect) {
        case PAGE_EFFECT_SCALE:
        {
            GRAPHIC_LOGD("Target Tile PAGE_EFFECT_SCALE: pos=%d", targetPos);
            float minScale = 0.5f;
            float scale = minScale + (1.0f - minScale) * (1.0f - targetAbsRatio);
            float pivotX = fullSize * 0.5f;
            float pivotY = fullSize * 0.5f;
            targetView->Scale(Vector2<float>(scale, scale), Vector2<float>(pivotX, pivotY));
            break;
        }
        case PAGE_EFFECT_FADE:
        {
            GRAPHIC_LOGD("Target Tile PAGE_EFFECT_FADE: pos=%d", targetPos);
            float minFactor = 0.3f;
            float factor = minFactor + (1.0f - minFactor) * (1.0f - targetAbsRatio);
            targetView->SetOpaScale(static_cast<uint8_t>(factor * OPA_OPAQUE));
            break;
        }
        case PAGE_EFFECT_STATIC:
        {
            // GRAPHIC_LOGD("Target Tile PAGE_EFFECT_COVER: pos=%d", targetPos);
            int16_t offsetX = 0;
            int16_t offsetY = 0;
            if (direction & TDIR_HOR) {
                offsetX = -targetPos;
            } else {
                offsetY = -targetPos;
            }
            targetView->Translate(Vector2<int16_t>(offsetX, offsetY));
            break;
        }
        case PAGE_EFFECT_STATIC_SCALE:
        {
            // GRAPHIC_LOGD("Target Tile PAGE_EFFECT_STATIC_SCALE: pos=%d", targetPos);
            float minScale = 0.8f;
            float scale = minScale + (1.0f - minScale) * (1.0f - targetAbsRatio);
            float pivotX = fullSize * 0.5f;
            float pivotY = fullSize * 0.5f;
            targetView->Scale(Vector2<float>(scale, scale), Vector2<float>(pivotX, pivotY));

            int16_t offsetX = 0;
            int16_t offsetY = 0;
            if (direction & TDIR_HOR) {
                offsetX = -targetPos;
            } else {
                offsetY = -targetPos;
            }
            targetView->Translate(Vector2<int16_t>(offsetX, offsetY));
            break;
        }
        case PAGE_EFFECT_SCALE_FADE:
        {
            GRAPHIC_LOGD("Target Tile PAGE_EFFECT_SCALE_FADE: pos=%d", targetPos);
            float minScale = 0.8f;
            float scale = minScale + (1.0f - minScale) * (1.0f - targetAbsRatio);
            float pivotX = fullSize * 0.5f;
            float pivotY = fullSize * 0.5f;
            targetView->Scale(Vector2<float>(scale, scale), Vector2<float>(pivotX, pivotY));

            float minFactor = 0.3f;
            float factor = minFactor + (1.0f - minFactor) * (1.0f - targetAbsRatio);
            targetView->SetOpaScale(static_cast<uint8_t>(factor * OPA_OPAQUE));
            break;
        }
        case PAGE_EFFECT_AUTO:
            GRAPHIC_LOGD("Target Tile PAGE_EFFECT_AUTO: pos=%d", targetPos);
        default:
            break;
    }
}

void UITileView::ApplyTransitionEffect()
{
    if (curView_ == nullptr || tileWidth_ == 0 || tileHeight_ == 0) {
        return;
    }

    UIView* exitView = nullptr;
    UIView* enterView = nullptr;
    uint16_t enterCol = curCol_;
    uint16_t enterRow = curRow_;
    uint8_t effectiveDirection = TDIR_NONE;
    if (!GetOnScreenTransitionViews(exitView, enterView, enterCol, enterRow, effectiveDirection)) {
        return;
    }

    PageEffectType enterEffect = GetTileEnterEffect(enterCol, enterRow, effectiveDirection);
    PageEffectType exitEffect = GetTileExitEffect(curCol_, curRow_, effectiveDirection);
    DoTransition(exitView, enterView, exitEffect, enterEffect, effectiveDirection);
}

// ============================================================================
// Navigation and Animation
// ============================================================================

int16_t UITileView::NormalizeIndex(int16_t index, int16_t max, bool loop) const
{
    if (loop) {
        index = ((index % max) + max) % max;
    } else {
        if (index < 0) {
            index = 0;
        } else if (index >= max) {
            index = max - 1;
        }
    }
    return index;
}

void UITileView::SetCurrentTile(uint16_t col, uint16_t row, bool needAnimator)
{
    if (col >= maxCols_ || row >= maxRows_) {
        return;
    }
    SwitchToTile(col, row, needAnimator);
    Invalidate();
}

void UITileView::SwitchToTile(uint16_t targetCol, uint16_t targetRow, bool needAnimator)
{
    if (targetCol >= maxCols_ || targetRow >= maxRows_) {
        return;
    }

    int16_t tileWidth = tileWidth_;
    int16_t tileHeight = tileHeight_;
    int32_t baseX = -static_cast<int32_t>(targetCol) * tileWidth;
    int32_t baseY = -static_cast<int32_t>(targetRow) * tileHeight;

    // Handle loop wrapping for shortest path
    if (loopHor_ && maxCols_ > 1) {
        if (curCol_ == 0 && targetCol == maxCols_ - 1) {
            baseX = tileWidth;
        } else if (curCol_ == maxCols_ - 1 && targetCol == 0) {
            baseX = -static_cast<int32_t>(maxCols_) * tileWidth;
        }
    }

    if (loopVer_ && maxRows_ > 1) {
        if (curRow_ == 0 && targetRow == maxRows_ - 1) {
            baseY = tileHeight;
        } else if (curRow_ == maxRows_ - 1 && targetRow == 0) {
            baseY = -static_cast<int32_t>(maxRows_) * tileHeight;
        }
    }

    targetX_ = static_cast<int16_t>(baseX);
    targetY_ = static_cast<int16_t>(baseY);
    targetCol_ = targetCol;
    targetRow_ = targetRow;

    StopAnimator();

    if (needAnimator) {
        SetDragStartValue(contentX_, contentY_);
        SetDragEndValue(targetX_, targetY_);
        scrollAnimator_.Start();
    } else {
        contentX_ = targetX_;
        contentY_ = targetY_;
        curCol_ = targetCol;
        curRow_ = targetRow;
        UIView* curView = GetTileView(curCol_, curRow_);
        if (curView == nullptr) {
            return;
        }
        curView_ = curView;
        LayoutTiles();
        RestoreTransitionZIndex();
        ResetEffect();
    }
}

void UITileView::SetAnimatorTime(uint16_t time)
{
    scrollAnimator_.SetTime(time);
}

void UITileView::StopAnimator()
{
    if (scrollAnimator_.GetState() != Animator::STOP) {
        scrollAnimator_.Stop();
    }
    axisLocked_ = false;
}

// ============================================================================
// Animator Callback
// ============================================================================

void UITileView::SetDragStartValue(int16_t startValueX, int16_t startValueY)
{
    startValueX_ = startValueX;
    previousValueX_ = startValueX;
    startValueY_ = startValueY;
    previousValueY_ = startValueY;
}

void UITileView::SetDragEndValue(int16_t endValueX, int16_t endValueY)
{
    endValueX_ = endValueX;
    endValueY_ = endValueY;
}

void UITileView::ResetCallback()
{
    startValueX_ = 0;
    endValueX_ = 0;
    startValueY_ = 0;
    endValueY_ = 0;
}

void UITileView::ResetEffect()
{
    for (uint16_t i = 0; i < MAX_TILES; i++) {
        TileInfo* tile = &tiles_[i];
        if (tile == nullptr) {
            continue;
        }
        UIView* view = tile->view;
        if (view == nullptr) {
            continue;
        }
        view->ResetTransParameter();
        view->SetOpaScale(OPA_OPAQUE);
    }
}

void UITileView::Callback(UIView* view)
{
    if (view == nullptr) {
        return;
    }

    uint32_t runTime = scrollAnimator_.GetRunTime();
    uint32_t duration = scrollAnimator_.GetTime();

    if (duration == 0) {
        GRAPHIC_LOGE("UITileView::Callback: invalid duration");
        return;
    }

    if (runTime <= duration) {
        int16_t currentX = easingFunc_(static_cast<float>(startValueX_),
                                       static_cast<float>(endValueX_),
                                       runTime,
                                       duration);
        int16_t currentY = easingFunc_(static_cast<float>(startValueY_),
                                       static_cast<float>(endValueY_),
                                       runTime,
                                       duration);

        DragXInner(currentX - previousValueX_);
        DragYInner(currentY - previousValueY_);
        previousValueX_ = currentX;
        previousValueY_ = currentY;
    } else {
        StopAnimator();
        contentX_ = endValueX_;
        contentY_ = endValueY_;
        LayoutTiles();
        Invalidate();
    }
}

void UITileView::OnStop(UIView& view)
{
    (void)view;
    bool tileChanged = (targetCol_ != curCol_) || (targetRow_ != curRow_);

    UIView* targetView = GetTileView(targetCol_, targetRow_);
    if (targetView == nullptr) {
        RestoreTransitionZIndex();
        ResetCallback();
        ResetEffect();
        return;
    }

    curCol_ = targetCol_;
    curRow_ = targetRow_;
    curView_ = targetView;
    RestoreTransitionZIndex();

    // Normalize content position in loop mode
    contentX_ = NormalizeContentLoop(contentX_, tileWidth_, maxCols_);
    contentY_ = NormalizeContentLoop(contentY_, tileHeight_, maxRows_);

    ResetCallback();
    ResetEffect();

    if (tileChanged) {
        NotifyTileChange();
    }
}

void UITileView::NotifyTileChange()
{
    if (tileChangeListener_ != nullptr) {
        tileChangeListener_->OnTileChange(*this, curCol_, curRow_);
    }
}

// ============================================================================
// Drag Handling
// ============================================================================

bool UITileView::DragXInner(int16_t distance)
{
    if (distance == 0) {
        return true;
    }

    uint8_t allowedDir = GetAllowedDirection(curCol_, curRow_);

    if (distance > 0 && !(allowedDir & TDIR_RIGHT)) {
        return false;
    }
    if (distance < 0 && !(allowedDir & TDIR_LEFT)) {
        return false;
    }

    if (!loopHor_) {
        int16_t tileWidth = tileWidth_;
        int16_t minX = -static_cast<int16_t>(maxCols_ - 1) * tileWidth;
        int16_t maxX = 0;

        if (contentX_ + distance > maxX) {
            distance = maxX - contentX_;
        } else if (contentX_ + distance < minX) {
            distance = minX - contentX_;
        }
    }

    contentX_ += distance;
    LayoutTiles();
    Invalidate();
    return true;
}

bool UITileView::DragYInner(int16_t distance)
{
    if (distance == 0) {
        return true;
    }

    uint8_t allowedDir = GetAllowedDirection(curCol_, curRow_);

    if (distance > 0 && !(allowedDir & TDIR_BOTTOM)) {
        return false;
    }
    if (distance < 0 && !(allowedDir & TDIR_TOP)) {
        return false;
    }

    if (!loopVer_) {
        int16_t tileHeight = tileHeight_;
        int16_t minY = -static_cast<int16_t>(maxRows_ - 1) * tileHeight;
        int16_t maxY = 0;

        if (contentY_ + distance > maxY) {
            distance = maxY - contentY_;
        } else if (contentY_ + distance < minY) {
            distance = minY - contentY_;
        }
    }

    contentY_ += distance;
    LayoutTiles();
    Invalidate();
    return true;
}

bool UITileView::OnDragStartEvent(const DragEvent& event)
{
    StopAnimator();
    oldDir_ = 0;
    ResetDragDelta();

    uint8_t dragDir = event.GetDragDirection();
    switch (dragDir) {
        case DragEvent::DIRECTION_LEFT_TO_RIGHT:
            direction_ = TDIR_RIGHT;
            UpdateDirAndZIndex(1);
            break;
        case DragEvent::DIRECTION_RIGHT_TO_LEFT:
            direction_ = TDIR_LEFT;
            UpdateDirAndZIndex(-1);
            break;
        case DragEvent::DIRECTION_TOP_TO_BOTTOM:
            direction_ = TDIR_BOTTOM;
            UpdateDirAndZIndex(-1);
            break;
        case DragEvent::DIRECTION_BOTTOM_TO_TOP:
            direction_ = TDIR_TOP;
            UpdateDirAndZIndex(1);
            break;
        default:
            direction_ = TDIR_NONE;
            break;
    }
    return UIView::OnDragStartEvent(event);
}

bool UITileView::OnDragEvent(const DragEvent& event)
{
    StopAnimator();
    if (curView_ == nullptr) {
        return UIView::OnDragEvent(event);
    }

    if (direction_ & TDIR_HOR) {
        int16_t deltaX = event.GetDeltaX();
        DragXInner(deltaX);
        UpdateLastDelta(deltaX);

        int16_t curX = curView_->GetX();
        if (ShouldUpdateDir(curX)) {
            UpdateDirAndZIndex();
        }
    } else if (direction_ & TDIR_VER) {
        int16_t deltaY = event.GetDeltaY();
        DragYInner(deltaY);
        UpdateLastDelta(deltaY);

        int16_t curY = curView_->GetY();
        if (ShouldUpdateDir(curY)) {
            UpdateDirAndZIndex();
        }
    }

    return UIView::OnDragEvent(event);
}

bool UITileView::OnDragEndEvent(const DragEvent& event)
{
    int16_t distanceX = 0;
    int16_t distanceY = 0;

    if (direction_ & TDIR_HOR) {
        distanceX = event.GetCurrentPos().x - event.GetPreLastPoint().x;
    } else if (direction_ & TDIR_VER) {
        distanceY = event.GetCurrentPos().y - event.GetPreLastPoint().y;
    }

    uint16_t targetCol = curCol_;
    uint16_t targetRow = curRow_;
    UpdateCurrentTileByThrow(distanceX, distanceY, targetCol, targetRow);
    SwitchToTile(targetCol, targetRow, true);

    Invalidate();

    return UIView::OnDragEndEvent(event);
}

void UITileView::UpdateCurrentTileByThrow(int16_t distanceX,
                                          int16_t distanceY,
                                          uint16_t& targetCol,
                                          uint16_t& targetRow) const
{
    int16_t tileWidth = tileWidth_;
    int16_t tileHeight = tileHeight_;

    targetCol = curCol_;
    targetRow = curRow_;

    if ((direction_ & TDIR_HOR) && tileWidth > 0) {
        int16_t threshold = tileWidth / UITileView::SNAP_THRESHOLD_RATIO;
        int16_t offset = contentX_ + static_cast<int16_t>(curCol_) * tileWidth;

        if (MATH_ABS(offset) >= threshold) {
            if (offset < 0) {
                targetCol = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curCol_) + 1,
                                                                 static_cast<int16_t>(maxCols_),
                                                                 loopHor_));
            } else if (offset > 0) {
                targetCol = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curCol_) - 1,
                                                                 static_cast<int16_t>(maxCols_),
                                                                 loopHor_));
            }
        } else {
            if (GetMaxAbsLastDelta() >= UITileView::THROW_THRESHOLD) {
                if (offset < 0 && distanceX < 0) {
                    targetCol = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curCol_) + 1,
                                                                     static_cast<int16_t>(maxCols_),
                                                                     loopHor_));
                } else if (offset > 0 && distanceX > 0) {
                    targetCol = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curCol_) - 1,
                                                                     static_cast<int16_t>(maxCols_),
                                                                     loopHor_));
                }
            }
        }
    } else if ((direction_ & TDIR_VER) && tileHeight > 0) {
        int16_t threshold = tileHeight / UITileView::SNAP_THRESHOLD_RATIO;
        int16_t offset = contentY_ + static_cast<int16_t>(curRow_) * tileHeight;

        if (MATH_ABS(offset) >= threshold) {
            if (offset < 0) {
                targetRow = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curRow_) + 1,
                                                                 static_cast<int16_t>(maxRows_),
                                                                 loopVer_));
            } else if (offset > 0) {
                targetRow = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curRow_) - 1,
                                                                 static_cast<int16_t>(maxRows_),
                                                                 loopVer_));
            }
        } else {
            if (GetMaxAbsLastDelta() >= UITileView::THROW_THRESHOLD) {
                if (offset < 0 && distanceY < 0) {
                    targetRow = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curRow_) + 1,
                                                                     static_cast<int16_t>(maxRows_),
                                                                     loopVer_));
                } else if (offset > 0 && distanceY > 0) {
                    targetRow = static_cast<uint16_t>(NormalizeIndex(static_cast<int16_t>(curRow_) - 1,
                                                                     static_cast<int16_t>(maxRows_),
                                                                     loopVer_));
                }
            }
        }
    }
}

} // namespace OHOS
