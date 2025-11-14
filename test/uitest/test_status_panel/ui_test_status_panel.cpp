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

#include "ui_test_status_panel.h"
#include "common/screen.h"
#include "gfx_utils/graphic_log.h"
#include <cmath> // 添加数学函数支持

namespace OHOS {
void UITestStatusPanel::SetUp()
{
    if (container_ == nullptr) {
        container_ = new UIViewGroup();
        container_->Resize(Screen::GetInstance().GetWidth(), Screen::GetInstance().GetHeight() - BACK_BUTTON_HEIGHT);
        // container_->SetHorizontalScrollState(false);
        // container_->SetVerticalScrollState(true);
    }
}

void UITestStatusPanel::TearDown()
{
    DeleteChildren(container_);
    container_ = nullptr;
    watchFaceContainer_ = nullptr;
    watchFaceBackground_ = nullptr;
    statusPanel_ = nullptr;
    showBtn_ = nullptr;
    hideBtn_ = nullptr;
    toggleAnimationBtn_ = nullptr;
    setHeight200Btn_ = nullptr;
    setHeight300Btn_ = nullptr;
    setThreshold30Btn_ = nullptr;
    setThreshold70Btn_ = nullptr;
    setTrigger30Btn_ = nullptr;
    setTrigger80Btn_ = nullptr;
    stateLabel_ = nullptr;
    positionLabel_ = nullptr;
    configLabel_ = nullptr;
    panelContentLabel_ = nullptr;
    lastX_ = 0;
    lastY_ = 0;
}

const UIView* UITestStatusPanel::GetTestView()
{
    CreateWatchFace();
    UIKitStatusPanelTestBasic001();
    UIKitStatusPanelTestAnimation002();
    UIKitStatusPanelTestConfiguration003();
    UIKitStatusPanelTestInteraction004();
    return container_;
}

void UITestStatusPanel::SetUpButton(UILabelButton* btn, const char* title, const char* id)
{
    if (btn == nullptr) {
        return;
    }
    container_->Add(btn);
    btn->SetPosition(lastX_, lastY_, BUTTON_WIDHT2, BUTTON_HEIGHT2);
    btn->SetText(title);
    btn->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    btn->SetOnClickListener(this);
    btn->SetViewId(id);
    lastX_ += btn->GetWidth() + DEFAULT_GAP;
    if (lastX_ > Screen::GetInstance().GetWidth() - btn->GetWidth()) {
        lastX_ = watchFaceX_ + WATCH_FACE_SIZE + 20;
        lastY_ += btn->GetHeight() + DEFAULT_GAP;
    }
}

void UITestStatusPanel::SetLastPos(UIView* view)
{
    if (view == nullptr) {
        return;
    }
    lastX_ = view->GetX();
    lastY_ = view->GetY() + view->GetHeight() + DEFAULT_GAP;
}

void UITestStatusPanel::UIKitStatusPanelTestBasic001()
{
    if (container_ == nullptr) {
        return;
    }

    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(lastX_, lastY_, Screen::GetInstance().GetWidth() - lastX_, TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("表盘下拉状态面板测试");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);

    // Create control buttons
    showBtn_ = new UILabelButton();
    SetUpButton(showBtn_, "显示面板", UI_TEST_SHOW_PANEL);

    hideBtn_ = new UILabelButton();
    SetUpButton(hideBtn_, "隐藏面板", UI_TEST_HIDE_PANEL);

    toggleAnimationBtn_ = new UILabelButton();
    SetUpButton(toggleAnimationBtn_, "切换动画", UI_TEST_TOGGLE_ANIMATION);

    // Create status display labels
    lastX_ = watchFaceX_ + WATCH_FACE_SIZE + 20;
    lastY_ += toggleAnimationBtn_->GetHeight() + DEFAULT_GAP;

    stateLabel_ = new UILabel();
    container_->Add(stateLabel_);
    stateLabel_->SetPosition(lastX_, lastY_, Screen::GetInstance().GetWidth() - lastX_, TITLE_LABEL_DEFAULT_HEIGHT);
    stateLabel_->SetText("状态: HIDDEN");
    stateLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);

    positionLabel_ = new UILabel();
    container_->Add(positionLabel_);
    positionLabel_->SetPosition(lastX_ + 200, lastY_, Screen::GetInstance().GetWidth() - lastX_,
                                TITLE_LABEL_DEFAULT_HEIGHT);
    positionLabel_->SetText("位置: 0.00");
    positionLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(positionLabel_);
    lastX_ = watchFaceX_ + WATCH_FACE_SIZE + 20;
}

void UITestStatusPanel::UIKitStatusPanelTestAnimation002()
{
    if (container_ == nullptr) {
        return;
    }

    // Create title label
    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(lastX_, lastY_, Screen::GetInstance().GetWidth(), TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("动画配置测试");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);
}

void UITestStatusPanel::UIKitStatusPanelTestConfiguration003()
{
    if (container_ == nullptr) {
        return;
    }

    // Create title label
    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(lastX_, lastY_, Screen::GetInstance().GetWidth() - lastX_, TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("表盘参数配置测试");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);

    // Height configuration buttons - 适合表盘尺寸
    setHeight200Btn_ = new UILabelButton();
    SetUpButton(setHeight200Btn_, "高度150", UI_TEST_SET_HEIGHT_200);

    setHeight300Btn_ = new UILabelButton();
    SetUpButton(setHeight300Btn_, "高度250", UI_TEST_SET_HEIGHT_300);

    // Threshold configuration buttons
    setThreshold30Btn_ = new UILabelButton();
    SetUpButton(setThreshold30Btn_, "阈值30%", UI_TEST_SET_THRESHOLD_30);

    setThreshold70Btn_ = new UILabelButton();
    SetUpButton(setThreshold70Btn_, "阈值70%", UI_TEST_SET_THRESHOLD_70);

    // Trigger area configuration buttons
    setTrigger30Btn_ = new UILabelButton();
    SetUpButton(setTrigger30Btn_, "触发区20", UI_TEST_SET_TRIGGER_30);

    setTrigger80Btn_ = new UILabelButton();
    SetUpButton(setTrigger80Btn_, "触发区50", UI_TEST_SET_TRIGGER_80);

    // Configuration display
    lastX_ = watchFaceX_ + WATCH_FACE_SIZE + 20;
    lastY_ += setTrigger80Btn_->GetHeight() + DEFAULT_GAP;

    configLabel_ = new UILabel();
    container_->Add(configLabel_);
    configLabel_->SetPosition(lastX_, lastY_, Screen::GetInstance().GetWidth() - lastX_,
                              TITLE_LABEL_DEFAULT_HEIGHT * 2);
    UpdateStatusInfo();
    configLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(configLabel_);
}

void UITestStatusPanel::UIKitStatusPanelTestInteraction004()
{
    if (container_ == nullptr) {
        return;
    }

    // Create title label
    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(lastX_, lastY_, Screen::GetInstance().GetWidth() - lastX_, TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("表盘交互说明");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);

    // Create instruction label
    UILabel* instructionLabel = new UILabel();
    container_->Add(instructionLabel);
    instructionLabel->SetPosition(lastX_, lastY_, Screen::GetInstance().GetWidth() - lastX_,
                                  TITLE_LABEL_DEFAULT_HEIGHT * 5);
    instructionLabel->SetText(
        "表盘下拉面板使用说明:\n1. 在表盘顶部触发区域下拉显示状态面板\n2. 面板显示时间、日期、电量等信息\n3. "
        "拖动过程中面板实时跟随手指\n4. 释放时根据阈值自动完成动画\n5. 可通过右侧按钮控制显示/隐藏和配置参数");
    instructionLabel->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    instructionLabel->SetAlign(TEXT_ALIGNMENT_LEFT, TEXT_ALIGNMENT_TOP);
    SetLastPos(instructionLabel);
}

bool UITestStatusPanel::OnClick(UIView& view, const ClickEvent& /* event */)
{
    if (statusPanel_ == nullptr) {
        return true;
    }

    const char* id = view.GetViewId();
    if (id == nullptr) {
        return true;
    }

    if (strcmp(id, UI_TEST_SHOW_PANEL) == 0) {
        statusPanel_->ShowPanel(true);
    } else if (strcmp(id, UI_TEST_HIDE_PANEL) == 0) {
        statusPanel_->HidePanel(true);
    } else if (strcmp(id, UI_TEST_TOGGLE_ANIMATION) == 0) {
        animationEnabled_ = !animationEnabled_;
        if (toggleAnimationBtn_ != nullptr) {
            toggleAnimationBtn_->SetText(animationEnabled_ ? "关闭动画" : "开启动画");
            toggleAnimationBtn_->Invalidate();
        }
    } else if (strcmp(id, UI_TEST_SET_HEIGHT_200) == 0) {
        statusPanel_->SetPanelHeight(150); // 适合表盘的高度
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_SET_HEIGHT_300) == 0) {
        statusPanel_->SetPanelHeight(250); // 适合表盘的高度
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_SET_THRESHOLD_30) == 0) {
        statusPanel_->SetAutoCompleteThreshold(0.3f);
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_SET_THRESHOLD_70) == 0) {
        statusPanel_->SetAutoCompleteThreshold(0.7f);
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_SET_TRIGGER_30) == 0) {
        statusPanel_->SetTriggerHeight(20); // 适合表盘的触发区域
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_SET_TRIGGER_80) == 0) {
        statusPanel_->SetTriggerHeight(50); // 适合表盘的触发区域
        UpdateStatusInfo();
    }

    return true;
}

void UITestStatusPanel::OnShow(UIBasePanel& /* panel */)
{
    GRAPHIC_LOGI("UITestStatusPanel::OnShow - Status panel is now shown\n");
    if (stateLabel_ != nullptr) {
        stateLabel_->SetText("状态: 显示");
        stateLabel_->Invalidate();
    }
}

void UITestStatusPanel::OnHide(UIBasePanel& /* panel */)
{
    GRAPHIC_LOGI("UITestStatusPanel::OnHide - Status panel is now hidden\n");
    if (stateLabel_ != nullptr) {
        stateLabel_->SetText("状态: 隐藏");
        stateLabel_->Invalidate();
    }
}

void UITestStatusPanel::OnPositionChanged(UIBasePanel& panel, float position)
{
    if (positionLabel_ != nullptr) {
        char posText[64];
        if (snprintf(posText, sizeof(posText), "位置: %.2f", position) > 0) {
            positionLabel_->SetText(posText);
            positionLabel_->Invalidate();
        }
    }

    // Update state based on position
    if (stateLabel_ != nullptr) {
        const char* stateText = "状态: ";
        switch (panel.GetPanelState()) {
            case UIStatusPanel::PANEL_HIDDEN:
                stateText = "状态: HIDDEN";
                break;
            case UIStatusPanel::PANEL_SHOWING:
                stateText = "状态: SHOWING";
                break;
            case UIStatusPanel::PANEL_SHOWN:
                stateText = "状态: SHOWN";
                break;
            case UIStatusPanel::PANEL_HIDING:
                stateText = "状态: HIDING";
                break;
            default:
                stateText = "状态: UNKNOWN";
                break;
        }
        stateLabel_->SetText(stateText);
        stateLabel_->Invalidate();
    }
}

void UITestStatusPanel::UpdateStatusInfo()
{
    if (configLabel_ == nullptr || statusPanel_ == nullptr) {
        return;
    }

    char configText[256];
    if (snprintf(configText, sizeof(configText), "配置: 高度=%d, 阈值=%.1f%%, 触发区=%d",
                 statusPanel_->GetPanelHeight(), statusPanel_->GetAutoCompleteThreshold() * 100,
                 statusPanel_->GetTriggerHeight()) > 0) {
        configLabel_->SetText(configText);
        configLabel_->Invalidate();
    }
}

void UITestStatusPanel::CreateWatchFace()
{
    if (container_ == nullptr) {
        return;
    }

    // 创建表盘容器
    watchFaceContainer_ = new UIViewGroup();
    container_->Add(watchFaceContainer_);
    watchFaceContainer_->SetPosition(watchFaceX_, watchFaceY_, WATCH_FACE_SIZE, WATCH_FACE_SIZE);
    watchFaceContainer_->SetStyle(STYLE_BACKGROUND_COLOR, Color::Black().full);
    watchFaceContainer_->SetStyle(STYLE_BORDER_RADIUS, WATCH_FACE_RADIUS);
    watchFaceContainer_->SetStyle(STYLE_BORDER_WIDTH, 2);
    watchFaceContainer_->SetStyle(STYLE_BORDER_COLOR, Color::Silver().full);

    // 创建表盘背景
    watchFaceBackground_ = new UIView();
    watchFaceContainer_->Add(watchFaceBackground_);
    watchFaceBackground_->SetPosition(10, 10, WATCH_FACE_SIZE - 20, WATCH_FACE_SIZE - 20);
    watchFaceBackground_->SetStyle(STYLE_BACKGROUND_COLOR, Color::Blue().full);
    watchFaceBackground_->SetStyle(STYLE_BORDER_RADIUS, WATCH_FACE_RADIUS - 10);
    watchFaceBackground_->SetDraggable(true);
    watchFaceBackground_->SetTouchable(true);

    // 添加表盘刻度标记
    for (int i = 0; i < 12; i++) {
        UIView* hourMark = new UIView();
        watchFaceContainer_->Add(hourMark);

        // 计算刻度位置
        float angle = i * 30.0f * 3.14159f / 180.0f; // 每30度一个刻度
        int16_t markX = WATCH_FACE_CENTER_X + (int16_t)((WATCH_FACE_RADIUS - 30) * std::sin(angle)) - 2;
        int16_t markY = WATCH_FACE_CENTER_Y - (int16_t)((WATCH_FACE_RADIUS - 30) * std::cos(angle)) - 10;

        hourMark->SetPosition(markX, markY, 4, 20);
        hourMark->SetStyle(STYLE_BACKGROUND_COLOR, Color::White().full);
    }

    // 添加数字标记
    for (int i = 1; i <= 12; i++) {
        UILabel* numberLabel = new UILabel();
        watchFaceContainer_->Add(numberLabel);

        // 计算数字位置
        float angle = (i - 3) * 30.0f * 3.14159f / 180.0f; // 12点为0度
        int16_t numX = WATCH_FACE_CENTER_X + (int16_t)((WATCH_FACE_RADIUS - 50) * std::cos(angle)) - 10;
        int16_t numY = WATCH_FACE_CENTER_Y + (int16_t)((WATCH_FACE_RADIUS - 50) * std::sin(angle)) - 10;

        numberLabel->SetPosition(numX, numY, 40, 20);
        char numText[3];
        snprintf(numText, sizeof(numText), "%d", i);
        numberLabel->SetText(numText);
        numberLabel->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
        numberLabel->SetStyle(STYLE_TEXT_COLOR, Color::White().full);
        numberLabel->SetAlign(TEXT_ALIGNMENT_CENTER, TEXT_ALIGNMENT_CENTER);
    }

    // 添加中心点
    UIView* centerDot = new UIView();
    watchFaceContainer_->Add(centerDot);
    centerDot->SetPosition(WATCH_FACE_CENTER_X - 5, WATCH_FACE_CENTER_Y - 5, 10, 10);
    centerDot->SetStyle(STYLE_BACKGROUND_COLOR, Color::Red().full);
    centerDot->SetStyle(STYLE_BORDER_RADIUS, 5);

    // 更新布局位置，将后续控件放在表盘右侧
    lastX_ = watchFaceX_ + WATCH_FACE_SIZE + 20;
    lastY_ = watchFaceY_;

    // Create status panel within watch face area
    statusPanel_ = new UIStatusPanel();
    watchFaceContainer_->Add(statusPanel_);
    statusPanel_->SetPosition(0, -WATCH_FACE_SIZE, WATCH_FACE_SIZE, WATCH_FACE_SIZE);
    statusPanel_->SetPanelHeight(WATCH_FACE_SIZE); // 适合表盘的高度
    statusPanel_->SetTriggerHeight(50);            // 触发区域高度
    statusPanel_->SetAutoCompleteThreshold(0.5f);
    statusPanel_->SetOnPanelListener(this);
    statusPanel_->SetStyle(STYLE_BACKGROUND_COLOR, Color::Gray().full);
    statusPanel_->SetStyle(STYLE_BORDER_RADIUS, 10);
    statusPanel_->SetAnimationDuration(500);
    statusPanel_->SetPanelDirection(
        (int16_t)UIStatusPanel::PanelDirection::TOP_TO_BOTTOM); // TOP_TO_BOTTOM  BOTTOM_TO_TOP

    // Add content to status panel
    panelContentLabel_ = new UILabel();
    statusPanel_->Add(panelContentLabel_);
    panelContentLabel_->SetPosition(20, 50, WATCH_FACE_SIZE - 40, 120);
    panelContentLabel_->SetText("表盘状态面板\n时间: 12:30\n日期: 2024-01-15\n电量: 85%\n步数: 8,520");
    panelContentLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    panelContentLabel_->SetStyle(STYLE_TEXT_COLOR, Color::White().full);
    panelContentLabel_->SetAlign(TEXT_ALIGNMENT_LEFT, TEXT_ALIGNMENT_TOP);
}

} // namespace OHOS