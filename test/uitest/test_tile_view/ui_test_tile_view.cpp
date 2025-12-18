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

#include "ui_test_tile_view.h"
#include "common/screen.h"
#include "components/ui_label.h"
#include "components/ui_label_button.h"

namespace OHOS {
namespace {
static constexpr int16_t TILE_VIEW_SIZE = 466;

static UIView* CreateTileContent(const char* text, ColorType bgColor)
{
    UIViewGroup* tile = new UIViewGroup();
    tile->SetStyle(STYLE_BACKGROUND_COLOR, bgColor.full);
    tile->SetStyle(STYLE_BACKGROUND_OPA, OPA_OPAQUE);
    // 设置圆角
    tile->SetStyle(STYLE_BORDER_RADIUS, TILE_VIEW_SIZE/2);

    UILabel* label = new UILabel();
    label->SetText(text);
    label->SetAlign(TEXT_ALIGNMENT_CENTER, TEXT_ALIGNMENT_CENTER);
    label->SetPosition(0, TILE_VIEW_SIZE / 2 - 20, TILE_VIEW_SIZE, 40);
    label->SetStyle(STYLE_TEXT_COLOR, Color::White().full);
    tile->Add(label);

    return tile;
}
} // namespace

void UITestTileView::SetUp()
{
    if (container_ == nullptr) {
        container_ = new UIScrollView();
        container_->Resize(Screen::GetInstance().GetWidth(), Screen::GetInstance().GetHeight());
    }
}

void UITestTileView::TearDown()
{
    if (container_ != nullptr) {
        delete container_;
        container_ = nullptr;
    }
    tileView_ = nullptr;
}

const UIView* UITestTileView::GetTestView()
{
    UIKitTileViewTestDisplay001();
    return container_;
}

void UITestTileView::UIKitTileViewTestDisplay001()
{
    if (container_ == nullptr) {
        return;
    }

    tileView_ = new UITileView();
    tileView_->SetPosition(0, 0, TILE_VIEW_SIZE, TILE_VIEW_SIZE);
    tileView_->SetStyle(STYLE_BACKGROUND_COLOR, Color::Black().full);
    tileView_->SetStyle(STYLE_BACKGROUND_OPA, OPA_OPAQUE);

    // Create 3x3 grid tiles - Row 0
    // tileView_->AddTile(CreateTileContent("(0,0) Top-Left", Color::GetColorFromRGB(0x21, 0x96, 0xF3)), 0, 0);
    tileView_->AddTile(CreateTileContent("(1,0) Top-Center", Color::GetColorFromRGB(0x4C, 0xAF, 0x50)), 1, 0);
    // tileView_->AddTile(CreateTileContent("(2,0) Top-Right", Color::GetColorFromRGB(0xFF, 0x98, 0x00)), 2, 0);

    // Row 1
    tileView_->AddTile(CreateTileContent("(0,1) Left", Color::GetColorFromRGB(0x9C, 0x27, 0xB0)), 0, 1);
    tileView_->AddTile(CreateTileContent("(1,1) Center", Color::GetColorFromRGB(0xF4, 0x43, 0x36)), 1, 1);
    tileView_->AddTile(CreateTileContent("(2,1) Right", Color::GetColorFromRGB(0x00, 0xBC, 0xD4)), 2, 1);
    tileView_->AddTile(CreateTileContent("(3,1) Right", Color::GetColorFromRGB(0x00, 0xBC, 0x09)), 3, 1);

    // Row 2
    // tileView_->AddTile(CreateTileContent("(0,2) Bottom-Left", Color::GetColorFromRGB(0x79, 0x55, 0x48)), 0, 2);
    tileView_->AddTile(CreateTileContent("(1,2) Bottom-Center", Color::GetColorFromRGB(0x60, 0x7D, 0x8B)), 1, 2);
    // tileView_->AddTile(CreateTileContent("(2,2) Bottom-Right", Color::GetColorFromRGB(0xE9, 0x1E, 0x63)), 2, 2);

    // Tile directions are automatically set based on neighbors:
    // - (1,0) Top-Center: can drag DOWN, LEFT (no tile above or to the right)
    // - (0,1) Left: can drag RIGHT, UP, DOWN (no tile to the left)
    // - (1,1) Center: can drag in ALL directions (has neighbors on all sides)
    // - (2,1) Right: can drag LEFT, UP, DOWN (has neighbor to the right at 3,1)
    // - (3,1) Right-most: can drag LEFT, UP, DOWN (no tile to the right)
    // - (1,2) Bottom-Center: can drag UP, LEFT (no tile below or to the right)

    tileView_->SetLoopHorizontal(true);
    tileView_->SetCurrentTile(1, 1, false);

    tileView_->SetGlobalEnterEffect(UITileView::TDIR_ALL, UITileView::PAGE_EFFECT_SCALE_FADE);
    tileView_->SetGlobalExitEffect(UITileView::TDIR_ALL, UITileView::PAGE_EFFECT_SCALE_FADE);
    GRAPHIC_LOGD("Global Enter Effect: %d, %d, %d, %d", tileView_->globalEnterEffects_[0],
    tileView_->globalEnterEffects_[1],
    tileView_->globalEnterEffects_[2],
    tileView_->globalEnterEffects_[3]);
    GRAPHIC_LOGD("Global Exit Effect: %d, %d, %d, %d", tileView_->globalExitEffects_[0],
    tileView_->globalExitEffects_[1],
    tileView_->globalExitEffects_[2],
    tileView_->globalExitEffects_[3]);

    // (1,1) Center and (2,1) Right (0,1) Left
    tileView_->SetTileEnterEffect(1, 1,
        UITileView::TDIR_HOR,
        UITileView::PAGE_EFFECT_STATIC_SCALE);
    tileView_->SetTileExitEffect(1, 1,
        UITileView::TDIR_HOR,
        UITileView::PAGE_EFFECT_STATIC_SCALE);

    tileView_->SetTileEnterEffect(0, 1,
        UITileView::TDIR_RIGHT,
        UITileView::PAGE_EFFECT_AUTO);
    tileView_->SetTileExitEffect(0, 1,
        UITileView::TDIR_LEFT,
        UITileView::PAGE_EFFECT_AUTO);

    tileView_->SetTileEnterEffect(2, 1,
        UITileView::TDIR_LEFT,
        UITileView::PAGE_EFFECT_AUTO);
    tileView_->SetTileExitEffect(2, 1,
        UITileView::TDIR_RIGHT,
        UITileView::PAGE_EFFECT_AUTO);

    // (1,1) Center and (1,0) Top (1,2) Bottom
    tileView_->SetTileEnterEffect(1, 1,
        UITileView::TDIR_VER,
        UITileView::PAGE_EFFECT_STATIC);
    tileView_->SetTileExitEffect(1, 1,
        UITileView::TDIR_VER,
        UITileView::PAGE_EFFECT_STATIC);

    tileView_->SetTileEnterEffect(1, 0,
        UITileView::TDIR_BOTTOM,
        UITileView::PAGE_EFFECT_AUTO);
    tileView_->SetTileExitEffect(1, 0,
        UITileView::TDIR_TOP,
        UITileView::PAGE_EFFECT_AUTO);

    tileView_->SetTileEnterEffect(1, 2,
        UITileView::TDIR_TOP,
        UITileView::PAGE_EFFECT_AUTO);
    tileView_->SetTileExitEffect(1, 2,
        UITileView::TDIR_BOTTOM,
        UITileView::PAGE_EFFECT_AUTO);
#if 0
    // 打印所有tile的enter和exit效果
    for (uint16_t col = 0; col < tileView_->GetCols(); col++) {
        for (uint16_t row = 0; row < tileView_->GetRows(); row++) {
            GRAPHIC_LOGD("Tile (%d, %d) Enter Effect: %d, %d, %d, %d", col, row, tileView_->GetTileEnterEffect(col, row, UITileView::TDIR_LEFT),
            tileView_->GetTileEnterEffect(col, row, UITileView::TDIR_RIGHT),
            tileView_->GetTileEnterEffect(col, row, UITileView::TDIR_TOP),
            tileView_->GetTileEnterEffect(col, row, UITileView::TDIR_BOTTOM));
            GRAPHIC_LOGD("Tile (%d, %d) Exit Effect: %d, %d, %d, %d", col, row, tileView_->GetTileExitEffect(col, row, UITileView::TDIR_LEFT),
            tileView_->GetTileExitEffect(col, row, UITileView::TDIR_RIGHT),
            tileView_->GetTileExitEffect(col, row, UITileView::TDIR_TOP),
            tileView_->GetTileExitEffect(col, row, UITileView::TDIR_BOTTOM));
        }
    }
#endif

    container_->Add(tileView_);
}

} // namespace OHOS
