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

#include "ui_test_notification_panel.h"
#include "common/screen.h"
#include "gfx_utils/graphic_log.h"
#include <cmath>

namespace OHOS {

class NotiDataProvider : public INotificationDataProvider {
public:
    NotiDataProvider() {}
    ~NotiDataProvider() override { ClearAll(); }
    uint16_t GetAppCount() const override
    {
        return groups_.Size();
    }

    List<MessageData>* GetMessagesByAppIndex(int16_t index) const override
    {
        if (index < 0 || index >= groups_.Size()) {
            return nullptr;
        }
        Group* g = GetAppByIndex(index);
        return g ? &g->messages : nullptr;
    }

    List<MessageData>* GetMessagesByAppName(const char* appName) const override
    {
        ListNode<Group*>* node = GetAppNodeByName(appName);
        if (node == nullptr) {
            return nullptr;
        }
        Group* g = node->data_;
        return (g != nullptr) ? &g->messages : nullptr;
    }

    uint16_t GetMessagesCountByAppName(const char* appName) const override
    {
        List<MessageData>* messages = GetMessagesByAppName(appName);
        return (messages != nullptr) ? messages->Size() : 0;
    }

    MessageData* GetAppMessageByIndex(const char* appName, int16_t index) override
    {
        if (index < 0) {
            return nullptr;
        }
        List<MessageData>* messages = GetMessagesByAppName(appName);
        if (messages == nullptr || messages->Size() == 0 || index >= messages->Size()) {
            return nullptr;
        }
        ListNode<MessageData>* msg = messages->Begin();
        const ListNode<MessageData>* msgEnd = messages->End();
        for (int16_t i = 0; i < index && msg != msgEnd; i++) {
            msg = messages->Next(msg);
        }
        if (msg == msgEnd) {
            return nullptr;
        }
        return &msg->data_;
    }

    void AddMessage(const MessageData& message) override
    {
        Group* g = FindOrCreateGroup(message.appName);
        if (g != nullptr) {
            MessageData::SafeCopyString(g->appIconPath, message.iconPath, sizeof(g->appIconPath));
            g->messages.PushFront(message);
        }
    }

    uint32_t RemoveMessageById(const char* appName, uint32_t messageId) override
    {
        if (appName == nullptr) {
            return 0;
        }
        ListNode<Group*>* node = GetAppNodeByName(appName);
        if (node == nullptr) {
            return 0;
        }
        Group* g = node->data_;
        if (g == nullptr) {
            return 0;
        }
        if (g->messages.IsEmpty()) {
            RemoveGroupNode(node);
            return 0;
        }

        ListNode<MessageData>* msg = g->messages.Begin();
        const ListNode<MessageData>* end = g->messages.End();
        while (msg != end) {
            if (msg->data_.messageId == messageId) {
                g->messages.Remove(msg);
                if (g->messages.IsEmpty()) {
                    RemoveGroupNode(node);
                    return 0;
                }
                break;
            }
            msg = g->messages.Next(msg);
        }
        return g->messages.Size();
    }

    void RemoveMessagesByAppName(const char* appName) override
    {
        if (appName == nullptr) {
            return;
        }
        ListNode<Group*>* node = GetAppNodeByName(appName);
        if (node == nullptr) {
            return;
        }
        RemoveGroupNode(node);
    }

    void ClearAll() override
    {
        ListNode<Group*>* node = groups_.Begin();
        const ListNode<Group*>* end = groups_.End();
        while (node != end) {
            node = RemoveGroupNode(node);
        }
    }

    int16_t GetTotalMessageCount() const override
    {
        int16_t total = 0;
        ListNode<Group*>* node = const_cast<List<Group*>&>(groups_).Begin();
        const ListNode<Group*>* end = const_cast<List<Group*>&>(groups_).End();
        while (node != end) {
            Group* g = node->data_;
            if (g != nullptr) {
                total += g->messages.Size();
            }
            node = const_cast<List<Group*>&>(groups_).Next(node);
        }
        return total;
    }
private:
    struct Group {
        char appName[MessageData::MAX_APP_NAME_LEN];
        char appIconPath[MessageData::MAX_ICON_PATH_LEN];
        List<MessageData> messages;
        Group() { appName[0] = '\0'; appIconPath[0] = '\0'; }
    };

    List<Group*> groups_;

    Group* GetAppByIndex(int16_t index) const
    {
        if (index < 0) {
            return nullptr;
        }
        ListNode<Group*>* node = groups_.Begin();
        const ListNode<Group*>* end = groups_.End();
        for (int16_t i = 0; i < index && node != end; ++i) {
            node = groups_.Next(node);
        }
        if (node == nullptr || node == end) {
            return nullptr;
        }
        return node->data_;
    }

    ListNode<Group*>* GetAppNodeByName(const char* appName) const
    {
        if (appName == nullptr) {
            return nullptr;
        }
        ListNode<Group*>* node = groups_.Begin();
        const ListNode<Group*>* end = groups_.End();
        while (node != end) {
            Group* g = node->data_;
            if (g != nullptr && strcmp(g->appName, appName) == 0) {
                return node;
            }
            node = groups_.Next(node);
        }
        return nullptr;
    }

    Group* FindOrCreateGroup(const char* appName)
    {
        if (appName == nullptr) {
            return nullptr;
        }
        ListNode<Group*>* node = groups_.Begin();
        const ListNode<Group*>* end = groups_.End();
        while (node != end) {
            Group* g = node->data_;
            if (g != nullptr && strcmp(g->appName, appName) == 0) {
                return g;
            }
            node = groups_.Next(node);
        }
        Group* ng = new Group();
        if (ng == nullptr) {
            return nullptr;
        }
        MessageData::SafeCopyString(ng->appName, appName, sizeof(ng->appName));
        groups_.PushFront(ng);
        return ng;
    }

    void RemoveEmptyGroups()
    {
        ListNode<Group*>* node = groups_.Begin();
        const ListNode<Group*>* end = groups_.End();
        while (node != end) {
            Group* g = node->data_;
            if (g != nullptr && g->messages.IsEmpty()) {
                node = RemoveGroupNode(node);
            } else {
                node = groups_.Next(node);
            }
        }
    }

    ListNode<Group*>* RemoveGroupNode(ListNode<Group*>* node)
    {
        if (node == nullptr) {
            return nullptr;
        }
        Group* g = node->data_;
        if (g != nullptr) {
            g->messages.Clear();
            delete g;
            g = nullptr;
        }
        return groups_.Remove(node);
    }
};

// UITestNotificationPanel implementation
void UITestNotificationPanel::SetUp()
{
    if (container_ == nullptr) {
        container_ = new UIViewGroup();
        container_->Resize(Screen::GetInstance().GetWidth(), Screen::GetInstance().GetHeight() - BACK_BUTTON_HEIGHT);
    }
}

void UITestNotificationPanel::TearDown()
{
    if (container_ != nullptr) {
        if (container_->IsViewGroup()) {
            DeleteChildren(static_cast<UIViewGroup*>(container_)->GetChildrenHead());
        }
        delete container_;
        container_ = nullptr;
    }
    watchFaceContainer_ = nullptr;
    watchFaceBackground_ = nullptr;
    notificationPanel_ = nullptr;
    showBtn_ = nullptr;
    hideBtn_ = nullptr;
    addMessageBtn_ = nullptr;
    clearAllBtn_ = nullptr;
    toggleAnimationBtn_ = nullptr;
    setHeight200Btn_ = nullptr;
    setHeight300Btn_ = nullptr;
    setThreshold30Btn_ = nullptr;
    setThreshold70Btn_ = nullptr;
    setTrigger30Btn_ = nullptr;
    setTrigger80Btn_ = nullptr;
    setColorRedBtn_ = nullptr;
    setColorGreenBtn_ = nullptr;
    setColorBlueBtn_ = nullptr;
    setColorYellowBtn_ = nullptr;
    setColorWhiteBtn_ = nullptr;
    setIcon1Btn_ = nullptr;
    setIcon2Btn_ = nullptr;
    setIcon3Btn_ = nullptr;
    noIconBtn_ = nullptr;
    stateLabel_ = nullptr;
    positionLabel_ = nullptr;
    configLabel_ = nullptr;
    messageCountLabel_ = nullptr;

    lastX_ = 0;
    lastY_ = 0;
    testMessageCounter_ = 0;
    if (dataProvider_ != nullptr) {
        delete dataProvider_;
        dataProvider_ = nullptr;
    }
}

const UIView* UITestNotificationPanel::GetTestView()
{
    CreateWatchFace();
    UIKitNotificationPanelTestBasic001();
    UIKitNotificationPanelTestAnimation002();
    UIKitNotificationPanelTestConfiguration003();
    UIKitNotificationPanelTestInteraction004();
    UIKitNotificationPanelTestSwipeGesture005();
    UIKitNotificationPanelTestColorSettings006();
    UIKitNotificationPanelTestIconSettings007();
    UIKitNotificationPanelTestAppExpansion008();
    return container_;
}

void UITestNotificationPanel::SetUpButton(UILabelButton* btn, const char* title, const char* id)
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
    lastX_ += btn->GetWidth() + NOTIFICATION_DEFAULT_GAP;
    if (lastX_ > Screen::GetInstance().GetWidth() - btn->GetWidth()) {
        lastX_ = watchFaceX_ + NOTIFICATION_WATCH_FACE_SIZE + 20;
        lastY_ += btn->GetHeight() + NOTIFICATION_DEFAULT_GAP;
    }
}

void UITestNotificationPanel::SetLastPos(UIView* view)
{
    if (view == nullptr) {
        return;
    }
    lastX_ = view->GetX();
    lastY_ = view->GetY() + view->GetHeight() + NOTIFICATION_DEFAULT_GAP;
}

void UITestNotificationPanel::UIKitNotificationPanelTestBasic001()
{
    if (container_ == nullptr) {
        return;
    }

    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(watchFaceX_ + NOTIFICATION_WATCH_FACE_SIZE + 20, watchFaceY_,
                       Screen::GetInstance().GetWidth() - watchFaceX_ - NOTIFICATION_WATCH_FACE_SIZE - 40,
                       TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("消息面板基础功能测试");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);

    // 显示/隐藏按钮
    showBtn_ = new UILabelButton();
    SetUpButton(showBtn_, "显示面板", UI_TEST_NOTIFICATION_SHOW_PANEL);

    hideBtn_ = new UILabelButton();
    SetUpButton(hideBtn_, "隐藏面板", UI_TEST_NOTIFICATION_HIDE_PANEL);

    lastX_ = watchFaceX_ + NOTIFICATION_WATCH_FACE_SIZE + 20;
    lastY_ += BUTTON_HEIGHT2 + NOTIFICATION_DEFAULT_GAP;
}

void UITestNotificationPanel::UIKitNotificationPanelTestAnimation002()
{
    if (container_ == nullptr) {
        return;
    }

    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(lastX_, lastY_, 200, TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("动画控制测试");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);

    toggleAnimationBtn_ = new UILabelButton();
    SetUpButton(toggleAnimationBtn_, "切换动画", UI_TEST_NOTIFICATION_TOGGLE_PANEL);

    lastX_ = watchFaceX_ + NOTIFICATION_WATCH_FACE_SIZE + 20;
    lastY_ += BUTTON_HEIGHT2 + NOTIFICATION_DEFAULT_GAP;
}

void UITestNotificationPanel::UIKitNotificationPanelTestConfiguration003()
{
    if (container_ == nullptr) {
        return;
    }

    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(lastX_, lastY_, 200, TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("配置参数测试");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);

    setHeight200Btn_ = new UILabelButton();
    SetUpButton(setHeight200Btn_, "高度200", UI_TEST_NOTIFICATION_SET_HEIGHT_200);

    setHeight300Btn_ = new UILabelButton();
    SetUpButton(setHeight300Btn_, "高度300", UI_TEST_NOTIFICATION_SET_HEIGHT_300);

    setThreshold30Btn_ = new UILabelButton();
    SetUpButton(setThreshold30Btn_, "阈值30%", UI_TEST_NOTIFICATION_SET_THRESHOLD_30);

    setThreshold70Btn_ = new UILabelButton();
    SetUpButton(setThreshold70Btn_, "阈值70%", UI_TEST_NOTIFICATION_SET_THRESHOLD_70);

    // 添加触发区域配置按钮
    setTrigger30Btn_ = new UILabelButton();
    SetUpButton(setTrigger30Btn_, "触发30%", UI_TEST_NOTIFICATION_SET_TRIGGER_30);

    setTrigger80Btn_ = new UILabelButton();
    SetUpButton(setTrigger80Btn_, "触发80%", UI_TEST_NOTIFICATION_SET_TRIGGER_80);

    // 添加配置信息显示标签
    configLabel_ = new UILabel();
    container_->Add(configLabel_);
    configLabel_->SetPosition(lastX_, lastY_, 400, TITLE_LABEL_DEFAULT_HEIGHT);
    configLabel_->SetText("配置: 高度=250, 阈值=50%, 触发=60%");
    configLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(configLabel_);
}

void UITestNotificationPanel::UIKitNotificationPanelTestInteraction004()
{
    if (container_ == nullptr) {
        return;
    }

    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(lastX_, lastY_, 200, TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("交互功能测试");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);

    addMessageBtn_ = new UILabelButton();
    SetUpButton(addMessageBtn_, "添加消息", UI_TEST_NOTIFICATION_ADD_MESSAGE);

    clearAllBtn_ = new UILabelButton();
    SetUpButton(clearAllBtn_, "清空所有", UI_TEST_NOTIFICATION_CLEAR_MESSAGES);

    lastX_ = watchFaceX_ + NOTIFICATION_WATCH_FACE_SIZE + 20;
    lastY_ += BUTTON_HEIGHT2 + NOTIFICATION_DEFAULT_GAP;

    // 状态显示标签
    stateLabel_ = new UILabel();
    container_->Add(stateLabel_);
    stateLabel_->SetPosition(lastX_, lastY_, 300, TITLE_LABEL_DEFAULT_HEIGHT);
    stateLabel_->SetText("状态: 隐藏");
    stateLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(stateLabel_);

    positionLabel_ = new UILabel();
    container_->Add(positionLabel_);
    positionLabel_->SetPosition(lastX_, lastY_, 300, TITLE_LABEL_DEFAULT_HEIGHT);
    positionLabel_->SetText("位置: 0.0");
    positionLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(positionLabel_);

    messageCountLabel_ = new UILabel();
    container_->Add(messageCountLabel_);
    messageCountLabel_->SetPosition(lastX_, lastY_, 300, TITLE_LABEL_DEFAULT_HEIGHT);
    messageCountLabel_->SetText("消息数量: 3");
    messageCountLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(messageCountLabel_);
}

void UITestNotificationPanel::UIKitNotificationPanelTestSwipeGesture005()
{
    if (container_ == nullptr) {
        return;
    }

    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(lastX_, lastY_, 200, TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("左滑手势测试");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);

    UILabel* instructionLabel = new UILabel();
    container_->Add(instructionLabel);
    instructionLabel->SetPosition(lastX_, lastY_, 400, TITLE_LABEL_DEFAULT_HEIGHT);
    instructionLabel->SetText("在消息项上左滑可显示删除按钮");
    instructionLabel->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(instructionLabel);
}

void UITestNotificationPanel::UIKitNotificationPanelTestColorSettings006()
{
    if (container_ == nullptr) {
        return;
    }

    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(lastX_, lastY_, 200, TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("颜色设置测试");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);

    setColorRedBtn_ = new UILabelButton();
    SetUpButton(setColorRedBtn_, "红色消息", UI_TEST_NOTIFICATION_SET_COLOR_RED);

    setColorGreenBtn_ = new UILabelButton();
    SetUpButton(setColorGreenBtn_, "绿色消息", UI_TEST_NOTIFICATION_SET_COLOR_GREEN);

    setColorBlueBtn_ = new UILabelButton();
    SetUpButton(setColorBlueBtn_, "蓝色消息", UI_TEST_NOTIFICATION_SET_COLOR_BLUE);

    setColorYellowBtn_ = new UILabelButton();
    SetUpButton(setColorYellowBtn_, "黄色消息", UI_TEST_NOTIFICATION_SET_COLOR_YELLOW);

    setColorWhiteBtn_ = new UILabelButton();
    SetUpButton(setColorWhiteBtn_, "白色消息", UI_TEST_NOTIFICATION_SET_COLOR_WHITE);

    lastX_ = watchFaceX_ + NOTIFICATION_WATCH_FACE_SIZE + 20;
    lastY_ += BUTTON_HEIGHT2 + NOTIFICATION_DEFAULT_GAP;
}

void UITestNotificationPanel::UIKitNotificationPanelTestAppExpansion008()
{
    if (container_ == nullptr) {
        return;
    }

    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(lastX_, lastY_, 200, TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("应用消息展开测试");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);

    UILabel* instructionLabel = new UILabel();
    container_->Add(instructionLabel);
    instructionLabel->SetPosition(lastX_, lastY_, 400, TITLE_LABEL_DEFAULT_HEIGHT);
    instructionLabel->SetText("点击消息项可展开显示该应用的所有消息");
    instructionLabel->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(instructionLabel);

    UILabel* backButtonLabel = new UILabel();
    container_->Add(backButtonLabel);
    backButtonLabel->SetPosition(lastX_, lastY_, 400, TITLE_LABEL_DEFAULT_HEIGHT);
    backButtonLabel->SetText("展开后点击返回按钮可回到主列表");
    backButtonLabel->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(backButtonLabel);
}

void UITestNotificationPanel::UIKitNotificationPanelTestIconSettings007()
{
    if (container_ == nullptr) {
        return;
    }

    UILabel* label = new UILabel();
    container_->Add(label);
    label->SetPosition(lastX_, lastY_, 200, TITLE_LABEL_DEFAULT_HEIGHT);
    label->SetText("图标设置测试");
    label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, FONT_DEFAULT_SIZE);
    SetLastPos(label);

    setIcon1Btn_ = new UILabelButton();
    SetUpButton(setIcon1Btn_, "图标1消息", UI_TEST_NOTIFICATION_SET_ICON_1);

    setIcon2Btn_ = new UILabelButton();
    SetUpButton(setIcon2Btn_, "图标2消息", UI_TEST_NOTIFICATION_SET_ICON_2);

    setIcon3Btn_ = new UILabelButton();
    SetUpButton(setIcon3Btn_, "图标3消息", UI_TEST_NOTIFICATION_SET_ICON_3);

    noIconBtn_ = new UILabelButton();
    SetUpButton(noIconBtn_, "无图标消息", UI_TEST_NOTIFICATION_NO_ICON);

    lastX_ = watchFaceX_ + NOTIFICATION_WATCH_FACE_SIZE + 20;
    lastY_ += BUTTON_HEIGHT2 + NOTIFICATION_DEFAULT_GAP;
}

bool UITestNotificationPanel::OnClick(UIView& view, const ClickEvent& event)
{
    (void)event; // 避免未使用参数警告
    const char* id = view.GetViewId();
    if (id == nullptr || notificationPanel_ == nullptr) {
        return false;
    }

    if (strcmp(id, UI_TEST_NOTIFICATION_SHOW_PANEL) == 0) {
        notificationPanel_->ShowPanel();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_HIDE_PANEL) == 0) {
        notificationPanel_->HidePanel();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_ADD_MESSAGE) == 0) {
        AddTestMessage();
        notificationPanel_->RefreshMessages();
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_CLEAR_MESSAGES) == 0) {
        dataProvider_->ClearAll();
        notificationPanel_->RefreshMessages();
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_TOGGLE_PANEL) == 0) {
        notificationPanel_->TogglePanel();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_HEIGHT_200) == 0) {
        notificationPanel_->SetPanelHeight(200);
        UpdateConfigInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_HEIGHT_300) == 0) {
        notificationPanel_->SetPanelHeight(300);
        UpdateConfigInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_THRESHOLD_30) == 0) {
        notificationPanel_->SetAutoCompleteThreshold(0.3f);
        UpdateConfigInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_THRESHOLD_70) == 0) {
        notificationPanel_->SetAutoCompleteThreshold(0.7f);
        UpdateConfigInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_TRIGGER_30) == 0) {
        // 设置触发区域高度为30像素
        notificationPanel_->SetTriggerHeight(30);
        UpdateConfigInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_TRIGGER_80) == 0) {
        // 设置触发区域高度为80像素
        notificationPanel_->SetTriggerHeight(80);
        UpdateConfigInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_COLOR_RED) == 0) {
        AddTestMessageWithColor(Color::Red());
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_COLOR_GREEN) == 0) {
        AddTestMessageWithColor(Color::Green());
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_COLOR_BLUE) == 0) {
        AddTestMessageWithColor(Color::Blue());
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_COLOR_YELLOW) == 0) {
        AddTestMessageWithColor(Color::Yellow());
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_COLOR_WHITE) == 0) {
        AddTestMessageWithColor(Color::White());
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_ICON_1) == 0) {
        AddTestMessageWithIcon("../test_notification_panel/aa-aa.png");
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_ICON_2) == 0) {
        AddTestMessageWithIcon("../test_notification_panel/iconfont-27.png");
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_SET_ICON_3) == 0) {
        AddTestMessageWithIcon("../test_notification_panel/iconfont-28.png");
        UpdateStatusInfo();
    } else if (strcmp(id, UI_TEST_NOTIFICATION_NO_ICON) == 0) {
        AddTestMessageWithIcon(nullptr);
        UpdateStatusInfo();
    }

    return true;
}

void UITestNotificationPanel::OnShow(UIBasePanel& panel)
{
    GRAPHIC_LOGI("UITestNotificationPanel::OnShow - Notification panel is now shown");

    if (stateLabel_ != nullptr) {
        stateLabel_->SetText("状态: SHOWN");
        stateLabel_->Invalidate();
    }
}

void UITestNotificationPanel::OnHide(UIBasePanel& panel)
{
    GRAPHIC_LOGI("UITestNotificationPanel::OnHide - Notification panel is now hidden");

    if (stateLabel_ != nullptr) {
        stateLabel_->SetText("状态: HIDDEN");
        stateLabel_->Invalidate();
    }

    UpdateStatusInfo();
}

void UITestNotificationPanel::OnPositionChanged(UIBasePanel& panel, float position)
{
    if (positionLabel_ != nullptr) {
        char posText[64];
        if (snprintf(posText, sizeof(posText), "位置: %.2f", position) > 0) {
            positionLabel_->SetText(posText);
            positionLabel_->Invalidate();
        }
    }

    // Update panel content based on position
    if (panelContentLabel_ != nullptr && position > 0.3f && notificationPanel_ != nullptr) {
        char contentText[256];
        const int messageCount = dataProvider_->GetTotalMessageCount();
        snprintf(contentText, sizeof(contentText), "通知面板\n新消息: %d条\n位置: %.1f%%\n状态: %s\n时间: 12:34",
                 messageCount, position * 100, panel.IsVisible() ? "显示中" : "隐藏");
        panelContentLabel_->SetText(contentText);
        panelContentLabel_->Invalidate();
    }

    GRAPHIC_LOGI("UITestNotificationPanel::OnNotificationPanelPositionChanged position=%f", position);
}

void UITestNotificationPanel::OnMessageClicked(UIView& item)
{
    UINotificationItem& itemRef = static_cast<UINotificationItem&>(item);
    GRAPHIC_LOGI("UITestNotificationPanel::OnMessageClicked messageId=%u, appName=%s",
                 itemRef.GetMessageId(), itemRef.GetAppName());

    if (stateLabel_ != nullptr) {
        char statusText[128];
        snprintf(statusText, sizeof(statusText), "状态: 展开应用 %s", itemRef.GetAppName());
        stateLabel_->SetText(statusText);
        stateLabel_->Invalidate();
    }
}

void UITestNotificationPanel::OnMessageDeleted(UIView& item)
{
    UINotificationItem& itemRef = static_cast<UINotificationItem&>(item);
    GRAPHIC_LOGI("UITestNotificationPanel::%s messageId=%u", __func__, itemRef.GetMessageId());

    // UINotificationPanel 已经内部处理删除，这里只需要更新显示
    UpdateStatusInfo();
}

void UITestNotificationPanel::OnExitAppMessages(UIView& item)
{
    if (stateLabel_ != nullptr) {
        stateLabel_->SetText("状态: 返回主列表");
        stateLabel_->Invalidate();
    }
}

void UITestNotificationPanel::UpdateStatusInfo()
{
    if (notificationPanel_ == nullptr) {
        return;
    }

    if (stateLabel_ != nullptr) {
        const char* state = notificationPanel_->IsVisible() ? "显示" : "隐藏";
        stateLabel_->SetText(state);
        stateLabel_->Invalidate();
    }

    if (positionLabel_ != nullptr) {
        char posText[32];
        snprintf(posText, sizeof(posText), "位置: %.2f", notificationPanel_->GetProgress());
        positionLabel_->SetText(posText);
        positionLabel_->Invalidate();
    }

    if (messageCountLabel_ != nullptr) {
        char countText[32];
        const int messageCount = dataProvider_->GetTotalMessageCount();
        snprintf(countText, sizeof(countText), "消息数量: %d", messageCount);
        messageCountLabel_->SetText(countText);
        messageCountLabel_->Invalidate();
    }

    UpdateConfigInfo();
}

void UITestNotificationPanel::UpdateConfigInfo()
{
    if (configLabel_ == nullptr || notificationPanel_ == nullptr) {
        return;
    }

    char configText[256];
    if (snprintf(configText, sizeof(configText), "配置: 高度=%d, 阈值=%.1f%%, 触发高度=%d",
                 notificationPanel_->GetPanelHeight(), notificationPanel_->GetAutoCompleteThreshold() * 100,
                 notificationPanel_->GetTriggerHeight()) > 0) {
        configLabel_->SetText(configText);
        configLabel_->Invalidate();
    }
}

void UITestNotificationPanel::CreateWatchFace()
{
    if (container_ == nullptr) {
        return;
    }

    // 创建表盘容器
    watchFaceContainer_ = new UIViewGroup();
    container_->Add(watchFaceContainer_);
    watchFaceContainer_->SetPosition(watchFaceX_, watchFaceY_, NOTIFICATION_WATCH_FACE_SIZE,
                                     NOTIFICATION_WATCH_FACE_SIZE);
    watchFaceContainer_->SetStyle(STYLE_BACKGROUND_COLOR, Color::Black().full);
    // 设置完全透明
    watchFaceContainer_->SetStyle(STYLE_BACKGROUND_OPA, OPA_TRANSPARENT);
    watchFaceContainer_->SetStyle(STYLE_BORDER_RADIUS, NOTIFICATION_WATCH_FACE_RADIUS);
    watchFaceContainer_->SetStyle(STYLE_BORDER_WIDTH, 2);
    watchFaceContainer_->SetStyle(STYLE_BORDER_COLOR, Color::Silver().full);

    // 创建表盘背景
    watchFaceBackground_ = new UIView();
    watchFaceContainer_->Add(watchFaceBackground_);
    watchFaceBackground_->SetPosition(10, 10, NOTIFICATION_WATCH_FACE_SIZE - 20, NOTIFICATION_WATCH_FACE_SIZE - 20);
    watchFaceBackground_->SetStyle(STYLE_BACKGROUND_COLOR, Color::Blue().full);
    watchFaceBackground_->SetStyle(STYLE_BORDER_RADIUS, NOTIFICATION_WATCH_FACE_RADIUS - 10);
    watchFaceBackground_->SetDraggable(true);
    watchFaceBackground_->SetTouchable(true);

    // 添加时间显示
    UILabel* timeLabel = new UILabel();
    watchFaceContainer_->Add(timeLabel);
    timeLabel->SetPosition(NOTIFICATION_WATCH_FACE_CENTER_X - 50, NOTIFICATION_WATCH_FACE_CENTER_Y - 80, 100, 40);
    timeLabel->SetText("12:34");
    timeLabel->SetFont(DEFAULT_VECTOR_FONT_FILENAME, 24);
    timeLabel->SetStyle(STYLE_TEXT_COLOR, Color::Olive().full);
    timeLabel->SetAlign(TEXT_ALIGNMENT_CENTER, TEXT_ALIGNMENT_CENTER);

    // 添加表盘刻度标记
    for (int i = 0; i < 12; i++) {
        UIView* hourMark = new UIView();
        watchFaceContainer_->Add(hourMark);

        // 计算刻度位置
        float angle = i * 30.0f * 3.14159f / 180.0f; // 每30度一个刻度
        int16_t markX =
            NOTIFICATION_WATCH_FACE_CENTER_X + (int16_t)((NOTIFICATION_WATCH_FACE_RADIUS - 30) * std::sin(angle)) - 2;
        int16_t markY =
            NOTIFICATION_WATCH_FACE_CENTER_Y - (int16_t)((NOTIFICATION_WATCH_FACE_RADIUS - 30) * std::cos(angle)) - 10;

        hourMark->SetPosition(markX, markY, 4, 20);
        hourMark->SetStyle(STYLE_BACKGROUND_COLOR, Color::White().full);
    }

    // 添加数字标记
    for (int i = 1; i <= 12; i++) {
        UILabel* numberLabel = new UILabel();
        watchFaceContainer_->Add(numberLabel);

        // 计算数字位置
        float angle = (i - 3) * 30.0f * 3.14159f / 180.0f; // 12点为0度
        int16_t numX =
            NOTIFICATION_WATCH_FACE_CENTER_X + (int16_t)((NOTIFICATION_WATCH_FACE_RADIUS - 50) * std::cos(angle)) - 10;
        int16_t numY =
            NOTIFICATION_WATCH_FACE_CENTER_Y + (int16_t)((NOTIFICATION_WATCH_FACE_RADIUS - 50) * std::sin(angle)) - 10;

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
    centerDot->SetPosition(NOTIFICATION_WATCH_FACE_CENTER_X - 5, NOTIFICATION_WATCH_FACE_CENTER_Y - 5, 10, 10);
    centerDot->SetStyle(STYLE_BACKGROUND_COLOR, Color::Red().full);
    centerDot->SetStyle(STYLE_BORDER_RADIUS, 5);

    // 更新布局位置，将后续控件放在表盘右侧
    lastX_ = watchFaceX_ + NOTIFICATION_WATCH_FACE_SIZE + 20;
    lastY_ = watchFaceY_;

    // Create notification panel within watch face area
    notificationPanel_ = new UINotificationPanel();
    watchFaceContainer_->Add(notificationPanel_);
    notificationPanel_->SetPosition(0, NOTIFICATION_WATCH_FACE_SIZE, NOTIFICATION_WATCH_FACE_SIZE,
                                    NOTIFICATION_WATCH_FACE_SIZE);
    notificationPanel_->SetPanelHeight(NOTIFICATION_WATCH_FACE_SIZE); // 适合表盘的高度
    notificationPanel_->SetTriggerHeight(70);                         // 触发区域高度
    notificationPanel_->SetAutoCompleteThreshold(0.5f);
    notificationPanel_->SetOnPanelListener(this);
    notificationPanel_->SetMessageListener(this); // 设置消息监听器

    // notificationPanel_->SetStyle(STYLE_BACKGROUND_COLOR, Color::Gray().full);
    notificationPanel_->SetStyle(STYLE_BACKGROUND_OPA, OPA_TRANSPARENT);
    notificationPanel_->SetStyle(STYLE_BORDER_RADIUS, NOTIFICATION_WATCH_FACE_RADIUS);
    notificationPanel_->SetAnimationDuration(500);

    if (dataProvider_ == nullptr) {
        dataProvider_ = new NotiDataProvider();
    }
    notificationPanel_->SetDataProvider(dataProvider_);

    // 添加一些测试消息
    for (int i = 0; i < 80; ++i) {
        AddTestMessage();
    }
    notificationPanel_->RefreshMessages();
}

void UITestNotificationPanel::AddTestMessage()
{
    if (notificationPanel_ == nullptr) {
        GRAPHIC_LOGE("notificationPanel is null");
        return;
    }

    testMessageCounter_++;

    // 生成测试消息内容（现在可以安全使用局部变量，MessageData会拷贝内容）
    char title[64];
    char content[128];

    // 创建不同的应用名称以测试应用展开功能
    const char* appNames[] = {"微信", "支付宝", "钉钉", "淘宝", "京东"};
    const char* currentApp = appNames[testMessageCounter_ % 5];

    snprintf(title, sizeof(title), "%s消息 %d", currentApp, testMessageCounter_);
    snprintf(content, sizeof(content), "这是来自 %s 的第 %d 条测试消息", currentApp, testMessageCounter_);

    ColorType colorTable[] = {Color::Red(), Color::Green(), Color::Blue(), Color::Yellow(), Color::White()};
    ColorType color = colorTable[testMessageCounter_ % 5];


    const char* imagePath;
    switch (testMessageCounter_ % 5) {
        case 0:
            imagePath =
                "C:/Users/cheng/work/OpenHarmony-v6.0/foundation/arkui/ui_lite/test/uitest/test_notification_panel/aa-aa.png";
            break;
        case 1:
            imagePath =
                "C:/Users/cheng/work/OpenHarmony-v6.0/foundation/arkui/ui_lite/test/uitest/test_notification_panel/iconfont-27.png";
            break;
        case 2:
            imagePath =
                "C:/Users/cheng/work/OpenHarmony-v6.0/foundation/arkui/ui_lite/test/uitest/test_notification_panel/iconfont-error.png";
            break;
        case 3:
            imagePath =
                "C:/Users/cheng/work/OpenHarmony-v6.0/foundation/arkui/ui_lite/test/uitest/test_notification_panel/iconfont_yellow-icon.png";
            break;
        default:
            imagePath =
                "C:/Users/cheng/work/OpenHarmony-v6.0/foundation/arkui/ui_lite/test/uitest/test_notification_panel/iconfont-time.png";
            break;
    }

    MessageData message(currentApp, title, content, "12:34", testMessageCounter_,
                        static_cast<MessageType>(testMessageCounter_ % 4),
                        false, color, imagePath);
    GRAPHIC_LOGI("AddTestMessage: %s", currentApp);
    dataProvider_->AddMessage(message);
    GRAPHIC_LOGI(" ");
}

void UITestNotificationPanel::AddTestMessageWithColor(ColorType color)
{
    if (notificationPanel_ == nullptr) {
        GRAPHIC_LOGE("notificationPanel is null");
        return;
    }

    testMessageCounter_++;

    // 生成测试消息内容
    char title[64];
    char content[128];
    const char* colorName = "未知";

    // 根据颜色设置消息内容
    if (color.full == Color::Red().full) {
        colorName = "红色";
    } else if (color.full == Color::Green().full) {
        colorName = "绿色";
    } else if (color.full == Color::Blue().full) {
        colorName = "蓝色";
    } else if (color.full == Color::Yellow().full) {
        colorName = "黄色";
    } else if (color.full == Color::White().full) {
        colorName = "白色";
    }

    snprintf(title, sizeof(title), "%s消息 %d", colorName, testMessageCounter_);
    snprintf(content, sizeof(content), "这是第 %d 条%s测试消息的内容", testMessageCounter_, colorName);

    char appName[16];
    snprintf(appName, sizeof(appName), "App%d", (testMessageCounter_ % 3) + 1);

    MessageData message(appName, title, content, "12:34", testMessageCounter_,
                        static_cast<MessageType>(testMessageCounter_ % 4),
                        false, color, nullptr);

    dataProvider_->AddMessage(message);
    notificationPanel_->RefreshMessages();
}

void UITestNotificationPanel::AddTestMessageWithIcon(const char* iconPath)
{
    if (notificationPanel_ == nullptr) {
        GRAPHIC_LOGE("notificationPanel is null");
        return;
    }

    testMessageCounter_++;

    // 生成测试消息内容
    char title[64];
    char content[128];
    const char* iconName = iconPath ? "图标" : "无图标";

    snprintf(title, sizeof(title), "%s消息 %d", iconName, testMessageCounter_);
    snprintf(content, sizeof(content), "这是第 %d 条带%s的测试消息内容", testMessageCounter_, iconName);

    char appName[16];
    snprintf(appName, sizeof(appName), "App%d", (testMessageCounter_ % 3) + 1);

    MessageData message(appName, title, content, "12:34", testMessageCounter_,
                        static_cast<MessageType>(testMessageCounter_ % 4),
                        false, Color::White(), iconPath);

    dataProvider_->AddMessage(message);
    notificationPanel_->RefreshMessages();
}

} // namespace OHOS