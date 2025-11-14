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

#ifndef UI_TEST_STATUS_PANEL_H
#define UI_TEST_STATUS_PANEL_H

#include "components/ui_label_button.h"
#include "components/ui_scroll_view.h"
#include "components/ui_status_panel.h"
#include "components/ui_label.h"
#include "ui_test.h"

namespace OHOS {
static constexpr int16_t DEFAULT_GAP = 10;
static constexpr int16_t WATCH_FACE_SIZE = 400;  // 表盘尺寸
static constexpr int16_t WATCH_FACE_RADIUS = WATCH_FACE_SIZE / 2; // 表盘半径
static constexpr int16_t WATCH_FACE_CENTER_X = WATCH_FACE_SIZE / 2; // 表盘中心X
static constexpr int16_t WATCH_FACE_CENTER_Y = WATCH_FACE_SIZE / 2; // 表盘中心Y

constexpr const char* UI_TEST_SHOW_PANEL = "show_panel";
constexpr const char* UI_TEST_HIDE_PANEL = "hide_panel";
constexpr const char* UI_TEST_TOGGLE_ANIMATION = "toggle_animation";
constexpr const char* UI_TEST_SET_HEIGHT_200 = "set_height_200";
constexpr const char* UI_TEST_SET_HEIGHT_300 = "set_height_300";
constexpr const char* UI_TEST_SET_THRESHOLD_30 = "set_threshold_30";
constexpr const char* UI_TEST_SET_THRESHOLD_70 = "set_threshold_70";
constexpr const char* UI_TEST_SET_TRIGGER_30 = "set_trigger_30";
constexpr const char* UI_TEST_SET_TRIGGER_80 = "set_trigger_80";

class UITestStatusPanel : public UITest,
                         public UIView::OnClickListener,
                         public UIBasePanel::OnPanelListener {
public:
    UITestStatusPanel() {}
    ~UITestStatusPanel() {}
    void SetUp() override;
    void TearDown() override;
    const UIView* GetTestView() override;
    bool OnClick(UIView& view, const ClickEvent& event) override;

    void OnShow(UIBasePanel& panel) override;
    void OnHide(UIBasePanel& panel) override;
    void OnPositionChanged(UIBasePanel& panel, float position) override;

    void UIKitStatusPanelTestBasic001();
    void UIKitStatusPanelTestAnimation002();
    void UIKitStatusPanelTestConfiguration003();
    void UIKitStatusPanelTestInteraction004();

private:
    void SetUpButton(UILabelButton* btn, const char* title, const char* id);
    void SetLastPos(UIView* view);
    void UpdateStatusInfo();
    void CreateWatchFace(); // 创建表盘模拟区域

    UIViewGroup* container_ = nullptr;
    UIViewGroup* watchFaceContainer_ = nullptr; // 表盘容器
    UIView* watchFaceBackground_ = nullptr;     // 表盘背景
    UIStatusPanel* statusPanel_ = nullptr;

    // Control buttons
    UILabelButton* showBtn_ = nullptr;
    UILabelButton* hideBtn_ = nullptr;
    UILabelButton* toggleAnimationBtn_ = nullptr;
    UILabelButton* setHeight200Btn_ = nullptr;
    UILabelButton* setHeight300Btn_ = nullptr;
    UILabelButton* setThreshold30Btn_ = nullptr;
    UILabelButton* setThreshold70Btn_ = nullptr;
    UILabelButton* setTrigger30Btn_ = nullptr;
    UILabelButton* setTrigger80Btn_ = nullptr;

    // Status display
    UILabel* stateLabel_ = nullptr;
    UILabel* positionLabel_ = nullptr;
    UILabel* configLabel_ = nullptr;

    // Panel content
    UILabel* panelContentLabel_ = nullptr;

    // Configuration
    bool animationEnabled_ = true;
    int16_t lastX_ = 0;
    int16_t lastY_ = 0;
    int16_t watchFaceX_ = 20;  // 表盘在屏幕中的X位置
    int16_t watchFaceY_ = 20;  // 表盘在屏幕中的Y位置
};
} // namespace OHOS
#endif // UI_TEST_STATUS_PANEL_H