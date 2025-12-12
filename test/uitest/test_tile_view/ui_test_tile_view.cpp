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
    if (container_ == nullptr) return;

    tileView_ = new UITileView();
    int16_t screenW = 466;// Screen::GetInstance().GetWidth();
    int16_t screenH = 466;

    tileView_->SetPosition(0, 0, screenW, screenH);

    // Watch Face Layout Demo
    // (1,0) Control Panel (Top)
    // (0,1) Left Card <-> (1,1) Watch Face <-> (2,1) Right Card
    // (1,2) Notifications (Bottom)

    int R = screenW / 2;

    // 1. Control Panel (1,0)
    {
        UIViewGroup* view = new UIViewGroup();
        view->Resize(screenW, screenH);
        view->SetStyle(STYLE_BACKGROUND_COLOR, Color::Gray().full);
        // view 设置圆角
        view->SetStyle(STYLE_BORDER_RADIUS, R);
        UILabel* label = new UILabel();
        label->SetPosition(150, 150, 300, 50);
        label->SetText("Control Panel (Drag Up)");
        view->Add(label);

        // Only allow dragging UP (to go back to Watch Face at 1,1)
        tileView_->Add(view, 1, 0, UITileView::TDIR_TOP);
    }

    // 2. Notifications (1,2)
    {
        UIViewGroup* view = new UIViewGroup();
        view->Resize(screenW, screenH);
        view->SetStyle(STYLE_BACKGROUND_COLOR, Color::Maroon().full);
        view->SetStyle(STYLE_BORDER_RADIUS, R);
        UILabel* label = new UILabel();
        label->SetPosition(150, 150, 300, 50);
        label->SetText("Notifications (Drag Down)");
        label->SetStyle(STYLE_TEXT_COLOR, Color::White().full);
        view->Add(label);

        // Only allow dragging DOWN (to go back to Watch Face at 1,1)
        tileView_->Add(view, 1, 2, UITileView::TDIR_BOTTOM);
    }

    // 3. Watch Face (1,1) - Center
    {
        UIViewGroup* view = new UIViewGroup();
        view->Resize(screenW, screenH);
        view->SetStyle(STYLE_BACKGROUND_COLOR, Color::Orange().full); // Watch face usually black bg
        view->SetStyle(STYLE_BORDER_RADIUS, R);

        // Add a "Clock" (Simulated)
        UILabel* time = new UILabel();
        time->SetPosition(screenW/2 - 50, screenH/2 - 25, 100, 50);
        time->SetText("12:00");
        time->SetStyle(STYLE_TEXT_COLOR, Color::White().full);
        time->SetAlign(TEXT_ALIGNMENT_CENTER, TEXT_ALIGNMENT_CENTER);
        view->Add(time);

        UILabel* label = new UILabel();
        label->SetPosition(120, 120, 400, 50);
        label->SetText("Watch Face (Center)");
        label->SetStyle(STYLE_TEXT_COLOR, Color::Green().full);
        view->Add(label);

        // Allow all directions
        tileView_->Add(view, 1, 1, UITileView::TDIR_ALL);

        tileView_->SetTransitionEffect(1, 1, UITileView::TDIR_VER, &UITileView::defaultCoverTransition_);
        tileView_->SetTransitionEffect(1, 1, UITileView::TDIR_HOR, &UITileView::defaultSlideTransition_);
    }

    // 4. Left Card (0,1)
    {
        UIViewGroup* view = new UIViewGroup();
        view->Resize(screenW, screenH);
        view->SetStyle(STYLE_BACKGROUND_COLOR, Color::Blue().full);
        view->SetStyle(STYLE_BORDER_RADIUS, R);
        UILabel* label = new UILabel();
        label->SetPosition(150, 150, 300, 50);
        label->SetText("Left Card (Weather)");
        view->Add(label);

        // Allow Horizontal dragging
        tileView_->Add(view, 0, 1, UITileView::TDIR_HOR);

        tileView_->SetTransitionEffect(0, 1, UITileView::TDIR_RIGHT, &UITileView::defaultSlideTransition_);
        tileView_->SetTransitionEffect(0, 1, UITileView::TDIR_LEFT, &UITileView::defaultCoverTransition_);
    }

    // 5. Right Card (2,1)
    {
        UIViewGroup* view = new UIViewGroup();
        view->Resize(screenW, screenH);
        view->SetStyle(STYLE_BACKGROUND_COLOR, Color::Red().full);
        view->SetStyle(STYLE_BORDER_RADIUS, R);

        UILabel* label = new UILabel();
        label->SetPosition(150, 150, 300, 50);
        label->SetText("Right Card (Activity)");
        view->Add(label);

        // Allow Horizontal dragging
        tileView_->Add(view, 2, 1, UITileView::TDIR_HOR);

        tileView_->SetTransitionEffect(2, 1, UITileView::TDIR_LEFT, &UITileView::defaultSlideTransition_);
        tileView_->SetTransitionEffect(2, 1, UITileView::TDIR_RIGHT, &UITileView::defaultCoverTransition_);
    }

    // Set Initial Tile to Watch Face (1,1)
    tileView_->SetCurrentTile(1, 1, true);

    // Enable Horizontal Loop (Cards loop), Disable Vertical Loop
    tileView_->SetLoop(true, false);

    container_->Add(tileView_);
}

} // namespace OHOS
