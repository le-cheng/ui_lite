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

#ifndef UI_TEST_NOTIFICATION_PANEL_H
#define UI_TEST_NOTIFICATION_PANEL_H

#include "components/ui_label_button.h"
#include "components/ui_scroll_view.h"
#include "components/ui_notification_panel.h"
#include "components/ui_label.h"
#include "graphic_config.h"
#include "ui_test.h"

namespace OHOS {
static constexpr int16_t NOTIFICATION_DEFAULT_GAP = 10;
static constexpr int16_t NOTIFICATION_WATCH_FACE_SIZE = 466;  // 表盘尺寸
static constexpr int16_t NOTIFICATION_WATCH_FACE_RADIUS = NOTIFICATION_WATCH_FACE_SIZE / 2; // 表盘半径
static constexpr int16_t NOTIFICATION_WATCH_FACE_CENTER_X = NOTIFICATION_WATCH_FACE_SIZE / 2; // 表盘中心X
static constexpr int16_t NOTIFICATION_WATCH_FACE_CENTER_Y = NOTIFICATION_WATCH_FACE_SIZE / 2; // 表盘中心Y

// 通知面板测试按钮常量
constexpr const char* UI_TEST_NOTIFICATION_SHOW_PANEL = "show_panel";
constexpr const char* UI_TEST_NOTIFICATION_HIDE_PANEL = "hide_panel";
constexpr const char* UI_TEST_NOTIFICATION_TOGGLE_PANEL = "toggle_panel";
constexpr const char* UI_TEST_NOTIFICATION_ADD_MESSAGE = "add_message";
constexpr const char* UI_TEST_NOTIFICATION_CLEAR_MESSAGES = "clear_messages";
constexpr const char* UI_TEST_NOTIFICATION_SET_HEIGHT_200 = "set_height_200";
constexpr const char* UI_TEST_NOTIFICATION_SET_HEIGHT_300 = "set_height_300";
constexpr const char* UI_TEST_NOTIFICATION_SET_THRESHOLD_30 = "set_threshold_30";
constexpr const char* UI_TEST_NOTIFICATION_SET_THRESHOLD_70 = "set_threshold_70";
constexpr const char* UI_TEST_NOTIFICATION_SET_TRIGGER_30 = "set_trigger_30";
constexpr const char* UI_TEST_NOTIFICATION_SET_TRIGGER_80 = "set_trigger_80";
constexpr const char* UI_TEST_NOTIFICATION_TOGGLE_ANIMATION = "toggle_animation";
constexpr const char* UI_TEST_NOTIFICATION_SET_COLOR_RED = "set_color_red";
constexpr const char* UI_TEST_NOTIFICATION_SET_COLOR_GREEN = "set_color_green";
constexpr const char* UI_TEST_NOTIFICATION_SET_COLOR_BLUE = "set_color_blue";
constexpr const char* UI_TEST_NOTIFICATION_SET_COLOR_YELLOW = "set_color_yellow";
constexpr const char* UI_TEST_NOTIFICATION_SET_COLOR_WHITE = "set_color_white";
constexpr const char* UI_TEST_NOTIFICATION_SET_ICON_1 = "set_icon_1";
constexpr const char* UI_TEST_NOTIFICATION_SET_ICON_2 = "set_icon_2";
constexpr const char* UI_TEST_NOTIFICATION_SET_ICON_3 = "set_icon_3";
constexpr const char* UI_TEST_NOTIFICATION_NO_ICON = "no_icon";

class UITestNotificationPanel : public UITest,
                               public UIView::OnClickListener,
                               public UIBasePanel::OnPanelListener,
                               public OnMessageListener {
public:
    UITestNotificationPanel() {}
    ~UITestNotificationPanel() {}
    void SetUp() override;
    void TearDown() override;
    const UIView* GetTestView() override;
    bool OnClick(UIView& view, const ClickEvent& event) override;

    void OnShow(UIBasePanel& panel) override;
    void OnHide(UIBasePanel& panel) override;
    void OnPositionChanged(UIBasePanel& panel, float position) override;
    void OnMessageClicked(UIView& item) override;
    void OnMessageDeleted(UIView& item) override;
    void OnExitAppMessages(UIView& item) override;

    void UIKitNotificationPanelTestBasic001();
    void UIKitNotificationPanelTestAnimation002();
    void UIKitNotificationPanelTestConfiguration003();
    void UIKitNotificationPanelTestInteraction004();
    void UIKitNotificationPanelTestSwipeGesture005();
    void UIKitNotificationPanelTestColorSettings006();
    void UIKitNotificationPanelTestIconSettings007();
    void UIKitNotificationPanelTestAppExpansion008();

private:
    void SetUpButton(UILabelButton* btn, const char* title, const char* id);
    void SetLastPos(UIView* view);
    void UpdateStatusInfo();
    void UpdateConfigInfo();
    void CreateWatchFace();
    void AddTestMessage();
    void AddTestMessageWithColor(ColorType color);
    void AddTestMessageWithIcon(const char* iconPath);

    UIViewGroup* container_ = nullptr;
    UIViewGroup* watchFaceContainer_ = nullptr; // 表盘容器
    UIView* watchFaceBackground_ = nullptr;     // 表盘背景
    UINotificationPanel* notificationPanel_ = nullptr;
    INotificationDataProvider* dataProvider_ = nullptr;

    // Control buttons
    UILabelButton* showBtn_ = nullptr;
    UILabelButton* hideBtn_ = nullptr;
    UILabelButton* addMessageBtn_ = nullptr;
    UILabelButton* clearAllBtn_ = nullptr;
    UILabelButton* toggleAnimationBtn_ = nullptr;
    UILabelButton* setHeight200Btn_ = nullptr;
    UILabelButton* setHeight300Btn_ = nullptr;
    UILabelButton* setThreshold30Btn_ = nullptr;
    UILabelButton* setThreshold70Btn_ = nullptr;
    UILabelButton* setTrigger30Btn_ = nullptr;
    UILabelButton* setTrigger80Btn_ = nullptr;

    // 颜色设置按钮
    UILabelButton* setColorRedBtn_ = nullptr;
    UILabelButton* setColorGreenBtn_ = nullptr;
    UILabelButton* setColorBlueBtn_ = nullptr;
    UILabelButton* setColorYellowBtn_ = nullptr;
    UILabelButton* setColorWhiteBtn_ = nullptr;

    // 图标设置按钮
    UILabelButton* setIcon1Btn_ = nullptr;
    UILabelButton* setIcon2Btn_ = nullptr;
    UILabelButton* setIcon3Btn_ = nullptr;
    UILabelButton* noIconBtn_ = nullptr;

    // Status display
    UILabel* stateLabel_ = nullptr;
    UILabel* positionLabel_ = nullptr;
    UILabel* configLabel_ = nullptr;
    UILabel* messageCountLabel_ = nullptr;

    // Panel content
    UILabel* panelContentLabel_ = nullptr;

    // Configuration
    bool animationEnabled_ = true;
    int16_t lastX_ = 0;
    int16_t lastY_ = 0;
    int16_t watchFaceX_ = 20;  // 表盘在屏幕中的X位置
    int16_t watchFaceY_ = 20;  // 表盘在屏幕中的Y位置
    uint16_t testMessageCounter_ = 0; // 测试消息计数器
};
} // namespace OHOS

#endif // UI_TEST_NOTIFICATION_PANEL_H