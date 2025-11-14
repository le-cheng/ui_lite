#include "components/ui_notification_panel.h"
#include "math.h"

namespace OHOS {
namespace {
static constexpr uint16_t DEFAULT_PANEL_WIDTH = 466;
static constexpr uint16_t DEFAULT_PANEL_HEIGHT = 466;
static constexpr uint16_t HEADER_HEIGHT = 60;
static constexpr uint16_t HANDLE_WIDTH = 40;
static constexpr uint16_t HANDLE_HEIGHT = 4;
static constexpr uint16_t HANDLE_PADDING = 8;
static constexpr uint16_t PADDING = 16;
static constexpr uint16_t CLEAR_BUTTON_WIDTH = 100;
static constexpr uint16_t DEFAULT_ITEM_WIDTH = 466;
static constexpr uint16_t DEFAULT_ITEM_HEIGHT = 128;

// 列表项间距
static constexpr uint16_t ITEM_MARGIN_TOP = 6;
static constexpr uint16_t ITEM_MARGIN_LEFT = 33;
static constexpr uint16_t CONTENT_MARGIN_LEFT = 8;
static constexpr uint16_t BUTTON_MARGIN_RIGHT = 8;
static constexpr uint16_t DRAG_HIDE_POS = CLEAR_BUTTON_WIDTH + BUTTON_MARGIN_RIGHT;

// MessageContentView 布局常量
static constexpr int16_t CONTENT_PADDING = 15;
static constexpr int16_t LABEL_HEIGHT = 32;
static constexpr int16_t TEXT_HEIGHT = 32;
static constexpr int16_t ICON_SIZE = 32;
static constexpr int16_t ICON_PADDING = 10;
static constexpr int16_t TIME_LABEL_WIDTH = 94;
static constexpr int16_t TITLE_TIME_GAP = 4;
static constexpr uint8_t READ_OPACITY = 128;

static constexpr int16_t STACK_HINT_OFFSET_Y = 6;
static constexpr int16_t STACK_HINT_INSET = 14;
static constexpr uint16_t MAX_GROUP_MESSAGES = 64;
static constexpr int16_t EMPTY_ICON_SIZE = 48;
static constexpr int16_t EMPTY_LABEL_HEIGHT = 36;
} // namespace

#if 1 // UINotificationPanel
UINotificationPanel::UINotificationPanel()
    : UIBasePanel(),
      messageList_(nullptr),
      headerView_(nullptr),
      handleView_(nullptr),
      titleLabel_(nullptr),
      emptyView_(nullptr),
      emptyIconView_(nullptr),
      emptyLabel_(nullptr),
      externalMessageListener_(nullptr),
      isInitialized_(false)
{
    SetPanelDirection(static_cast<int16_t>(PanelDirection::BOTTOM_TO_TOP));
    Resize(DEFAULT_PANEL_WIDTH, DEFAULT_PANEL_HEIGHT);

    messageAdapter_.SetMessageListener(this);

    if (!InitializeComponents()) {
        GRAPHIC_LOGE("UINotificationPanel initialization failed");
        CleanupComponents();
    }
}

UINotificationPanel::~UINotificationPanel()
{
    messageAdapter_.SetMessageListener(nullptr);
    CleanupComponents();
}

void UINotificationPanel::GetTargetView(const Point& point, UIView** current, UIView** target)
{
    if ((current == nullptr) || (target == nullptr)) {
        return;
    }

    if (disallowIntercept_) {
        *current = nullptr;
        *target = nullptr;
        return;
    }

    if (!IsInExtendedTriggerArea(point)) {
        return;
    }

    if (!visible_) {
        return;
    }

    *target = this;
    if (touchable_) {
        *current = this;
    }

    if (isDragging_) {
        return;
    }

    if (IsInTriggerArea(point)) {
        return;
    }

    UIView* view = GetChildrenHead();
    while (view != nullptr) {
        if (!view->IsViewGroup()) {
            Rect rect = view->GetRect();
            if (rect.IsContains(point)) {
                view->GetTargetView(point, current, target);
            }
        } else {
            UIViewGroup* viewGroup = static_cast<UIViewGroup*>(view);
            viewGroup->GetTargetView(point, current, target);
        }
        view = view->GetNextSibling();
    }
}

void UINotificationPanel::CleanupComponents()
{
    isInitialized_ = false;

    if (messageList_ != nullptr) {
        Remove(messageList_);
        delete messageList_;
        messageList_ = nullptr;
    }

    if (headerView_ != nullptr) {
        Remove(headerView_);
        delete headerView_;
        headerView_ = nullptr;
    }

    handleView_ = nullptr;
    titleLabel_ = nullptr;

    if (emptyView_ != nullptr) {
        Remove(emptyView_);
        delete emptyView_;
        emptyView_ = nullptr;
        emptyIconView_ = nullptr;
        emptyLabel_ = nullptr;
    }
}

bool UINotificationPanel::InitializeComponents()
{
    headerView_ = new UIViewGroup();
    if (headerView_ == nullptr) {
        GRAPHIC_LOGE("UINotificationPanel::InitializeComponents headerView_ create failed");
        return false;
    }

    headerView_->SetPosition(0, 0, GetWidth(), HEADER_HEIGHT);
    headerView_->SetStyle(STYLE_BACKGROUND_OPA, OPA_TRANSPARENT);
    Add(headerView_);

    if (!CreateHandleView()) {
        CleanupComponents();
        return false;
    }
    // if (!CreateTitleLabel()) {
    //     CleanupComponents();
    //     return false;
    // }
    if (!CreateMessageList()) {
        CleanupComponents();
        return false;
    }
    if (!CreateEmptyView()) {
        CleanupComponents();
        return false;
    }

    isInitialized_ = true;
    UpdateEmptyStateVisibility();
    return true;
}

bool UINotificationPanel::CreateHandleView()
{
    handleView_ = new UIView();
    if (handleView_ == nullptr) {
        GRAPHIC_LOGE("UINotificationPanel::CreateHandleView failed");
        return false;
    }

    handleView_->SetPosition((GetWidth() - HANDLE_WIDTH) / 2, HANDLE_PADDING);
    handleView_->SetWidth(HANDLE_WIDTH);
    handleView_->SetHeight(HANDLE_HEIGHT);
    handleView_->SetStyle(STYLE_BACKGROUND_COLOR, Color::Red().full);
    handleView_->SetStyle(STYLE_BORDER_RADIUS, 2);

    headerView_->Add(handleView_);
    headerView_->SetHeight(HANDLE_HEIGHT + HANDLE_PADDING * 2);
    return true;
}

bool UINotificationPanel::CreateTitleLabel()
{
    titleLabel_ = new UILabel();
    if (titleLabel_ == nullptr) {
        GRAPHIC_LOGE("UINotificationPanel::CreateTitleLabel failed");
        return false;
    }

    titleLabel_->SetPosition(PADDING, HANDLE_HEIGHT + HANDLE_PADDING * 2);
    titleLabel_->SetWidth(GetWidth() - 2 * PADDING);
    titleLabel_->SetHeight(40);
    titleLabel_->SetText("通知");
    titleLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, 28);
    titleLabel_->SetAlign(TEXT_ALIGNMENT_CENTER);
    headerView_->SetHeight(HANDLE_HEIGHT + HANDLE_PADDING * 2 + titleLabel_->GetHeight());
    headerView_->Add(titleLabel_);
    return true;
}

bool UINotificationPanel::CreateMessageList()
{
    messageList_ = new UICircleList();
    if (messageList_ == nullptr) {
        GRAPHIC_LOGE("UINotificationPanel::CreateMessageList failed");
        return false;
    }

    messageList_->SetScaleRange(0.20f, 1.15f);
    messageList_->SetFadeZoneRatio(0.45f);
    messageList_->SetLoopState(false); // 非循环才有回弹
    // messageList_->SetScrollBlankSize(12); // 两端留白（可选）
    messageList_->SetReboundSize(200); // 开启回弹，40像素
    messageList_->SetThrowDrag(true);  // 抬手后有惯性滑动

    messageList_->SetPosition(0, headerView_->GetHeight());
    messageList_->SetWidth(GetWidth());
    messageList_->SetHeight(GetPanelHeight() - headerView_->GetHeight());
    messageList_->SetStyle(STYLE_BACKGROUND_OPA, OPA_TRANSPARENT);

    messageList_->SetAdapter(&messageAdapter_);

    Add(messageList_);
    return true;
}

bool UINotificationPanel::CreateEmptyView()
{
    emptyView_ = new UIViewGroup();
    if (emptyView_ == nullptr) {
        GRAPHIC_LOGE("UINotificationPanel::CreateEmptyView emptyView_ create failed");
        return false;
    }
    emptyView_->SetPosition(0, headerView_->GetHeight(), GetWidth(), GetPanelHeight() - headerView_->GetHeight());
    emptyView_->SetStyle(STYLE_BACKGROUND_OPA, OPA_TRANSPARENT);
    Add(emptyView_);

    emptyIconView_ = new UIView();
    if (emptyIconView_ == nullptr) {
        GRAPHIC_LOGE("UINotificationPanel::CreateEmptyView emptyIconView_ create failed");
        return false;
    }
    const int16_t iconX = (emptyView_->GetWidth() - EMPTY_ICON_SIZE) / 2;
    const int16_t iconY = (emptyView_->GetHeight() - EMPTY_ICON_SIZE) / 2 - 20; // 稍微上移
    emptyIconView_->SetPosition(iconX, iconY, EMPTY_ICON_SIZE, EMPTY_ICON_SIZE);
    emptyIconView_->SetStyle(STYLE_BACKGROUND_COLOR, Color::Red().full);
    emptyIconView_->SetStyle(STYLE_BACKGROUND_OPA, OPA_OPAQUE);
    emptyIconView_->SetStyle(STYLE_BORDER_RADIUS, EMPTY_ICON_SIZE / 2);
    emptyView_->Add(emptyIconView_);

    // 文案标签 “暂无消息”
    emptyLabel_ = new UILabel();
    if (emptyLabel_ == nullptr) {
        GRAPHIC_LOGE("UINotificationPanel::CreateEmptyView emptyLabel_ create failed");
        return false;
    }
    emptyLabel_->SetText("暂无消息");
    emptyLabel_->SetAlign(TEXT_ALIGNMENT_CENTER);
    emptyLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, 32);
    emptyLabel_->SetStyle(STYLE_TEXT_COLOR, Color::Gray().full);
    emptyLabel_->SetPosition(PADDING, iconY + EMPTY_ICON_SIZE + 10, emptyView_->GetWidth() - 2 * PADDING,
                             EMPTY_LABEL_HEIGHT);
    emptyView_->Add(emptyLabel_);

    emptyView_->SetVisible(false);
    return true;
}

void UINotificationPanel::UpdateEmptyStateVisibility()
{
    if (!isInitialized_) {
        return;
    }
    const uint16_t total = messageAdapter_.GetTotalMessageCount();
    const bool showEmpty = (total == 0);

    if (emptyView_ != nullptr) {
        emptyView_->SetVisible(showEmpty);
        emptyView_->Invalidate();
    }
    if (messageList_ != nullptr) {
        messageList_->SetVisible(!showEmpty);
        if (!showEmpty) {
            messageList_->Invalidate();
        }
    }
}

void UINotificationPanel::RefreshMessages()
{
    if (!isInitialized_ || messageList_ == nullptr) {
        GRAPHIC_LOGE("UINotificationPanel not initialized or messageList_ is null");
        return;
    }

    messageList_->RefreshList();
    messageList_->Invalidate();
    UpdateEmptyStateVisibility();
}

void UINotificationPanel::AddMessage(const MessageData& message)
{
    messageAdapter_.AddMessage(message);
    RefreshMessages();
}

void UINotificationPanel::RemoveMessage(uint32_t messageId)
{
    messageAdapter_.RemoveMessageById(messageId);
    RefreshMessages();
}

void UINotificationPanel::ClearAllMessages()
{
    messageAdapter_.ClearAllGroupData();
    RefreshMessages();
}

uint16_t UINotificationPanel::GetMessageCount()
{
    return messageAdapter_.GetTotalMessageCount();
}

void UINotificationPanel::OnMessageClicked(UIView& item)
{
    __attribute__((unused)) UINotificationItem& itemRef = static_cast<UINotificationItem&>(item);
    GRAPHIC_LOGI("UINotificationPanel::OnMessageClicked messageId=%u", itemRef.GetMessageId());

    if (externalMessageListener_ != nullptr) {
        externalMessageListener_->OnMessageClicked(item);
    }
}

void UINotificationPanel::OnMessageDeleted(UIView& item)
{
    if (externalMessageListener_ != nullptr) {
        externalMessageListener_->OnMessageDeleted(item);
    }

    UINotificationItem& itemRef = static_cast<UINotificationItem&>(item);
    GRAPHIC_LOGI("UINotificationPanel::OnMessageDeleted messageId=%u", itemRef.GetMessageId());
    RemoveMessage(itemRef.GetMessageId());
}
#endif

// ============================================================================
// MessageAdapter Implementation
// ============================================================================
#if 1 // MessageAdapter
MessageAdapter::MessageAdapter() : groups_(), itemListener_(nullptr) {}

MessageAdapter::~MessageAdapter()
{
    ClearAllGroupData();
}

uint16_t MessageAdapter::GetCount()
{
    return groups_.Size();
}

UIView* MessageAdapter::GetView(UIView* inView, int16_t index)
{
    const uint16_t count = GetCount();
    if (index < 0 || static_cast<uint16_t>(index) >= count) {
        GRAPHIC_LOGE("MessageAdapter::%s invalid index: %d, total groups: %d", __func__, index, count);
        return nullptr;
    }

    NotificationGroupData_t* groupData = GetGroupDataByIndex(static_cast<uint16_t>(index));
    if (groupData == nullptr) {
        GRAPHIC_LOGE("MessageAdapter::%s group data not found at index: %d", __func__, index);
        return nullptr;
    }

    UINotificationItem* item = CreateMessageItem(inView);
    if (item == nullptr) {
        GRAPHIC_LOGE("MessageAdapter::%s failed to create item", __func__);
        return nullptr;
    }

    item->SetMessageNumber(groupData->messages.Size());
    item->UpdateMessageItem(groupData->messages.Front());
    item->LayoutChildViews();
    return item;
}

UINotificationItem* MessageAdapter::CreateMessageItem(UIView* inView)
{
    UINotificationItem* item = nullptr;

    if (inView != nullptr) {
        item = static_cast<UINotificationItem*>(inView);
        GRAPHIC_LOGI("MessageAdapter::%s use existing item", __func__);
    } else {
        item = new UINotificationItem();
        if (item == nullptr) {
            GRAPHIC_LOGE("MessageAdapter::%s failed to allocate memory", __func__);
            return nullptr;
        }
        GRAPHIC_LOGI("MessageAdapter::%s create new item", __func__);
    }

    if (itemListener_ != nullptr) {
        item->SetMessageListener(itemListener_);
    }
    return item;
}

void MessageAdapter::SetData(List<MessageData>* messages)
{
    if (messages == nullptr) {
        GRAPHIC_LOGE("MessageAdapter::SetData messages is null");
        return;
    }

    ClearAllGroupData();

    ListNode<MessageData>* node = messages->Begin();
    while (node != messages->End()) {
        AddMessage(node->data_);
        node = messages->Next(node);
    }
}

void MessageAdapter::AddMessage(const MessageData& message)
{
    GRAPHIC_LOGI("MessageAdapter::AddMessage messageId=%u", message.messageId);
    NotificationGroupData_t* group = FindOrCreateGroupData(message.appName, message.iconPath);
    if (group == nullptr) {
        GRAPHIC_LOGE("MessageAdapter::%s FindOrCreateGroupData failed", __func__);
        return;
    }
    group->messages.PushFront(message);
}

void MessageAdapter::RemoveMessageById(uint32_t messageId)
{
    ListNode<NotificationGroupData_t*>* groupNode = groups_.Begin();
    const ListNode<NotificationGroupData_t*>* end = groups_.End();
    while (groupNode != end) {
        NotificationGroupData_t* group = groupNode->data_;
        if (group != nullptr) {
            ListNode<MessageData>* msgNode = group->messages.Begin();
            const ListNode<MessageData>* msgEnd = group->messages.End();
            while (msgNode != msgEnd) {
                if (msgNode->data_.messageId == messageId) {
                    group->messages.Remove(msgNode);
                    RemoveEmptyGroups();
                    return;
                }
                msgNode = group->messages.Next(msgNode);
            }
        }
        groupNode = groups_.Next(groupNode);
    }
}

void MessageAdapter::ClearAllGroupData()
{
    ListNode<NotificationGroupData_t*>* node = groups_.Begin();
    const ListNode<NotificationGroupData_t*>* end = groups_.End();
    while (node != end) {
        NotificationGroupData_t* group = node->data_;
        if (group != nullptr) {
            delete group;
        }
        node = groups_.Next(node);
    }
    groups_.Clear();
}

MessageAdapter::NotificationGroupData_t* MessageAdapter::FindOrCreateGroupData(const char* appName,
                                                                               const char* appIconPath)
{
    if (appName == nullptr) {
        GRAPHIC_LOGE("MessageAdapter::%s appName is null", __func__);
        return nullptr;
    }

    ListNode<NotificationGroupData_t*>* node = groups_.Begin();
    const ListNode<NotificationGroupData_t*>* end = groups_.End();
    while (node != end) {
        NotificationGroupData_t* data = node->data_;
        if (data != nullptr && strcmp(data->appName, appName) == 0) {
            return data;
        }
        node = groups_.Next(node);
    }

    NotificationGroupData_t* newData = new NotificationGroupData_t();
    if (newData == nullptr) {
        GRAPHIC_LOGE("MessageAdapter::%s failed to allocate group data", __func__);
        return nullptr;
    }
    // 初始化分组的应用名与应用图标路径（图标路径仅在创建时设置）
    MessageData::SafeCopyString(newData->appName, appName, sizeof(newData->appName));
    MessageData::SafeCopyString(newData->appIconPath, appIconPath, sizeof(newData->appIconPath));
    groups_.PushFront(newData);
    return newData;
}

void MessageAdapter::RemoveEmptyGroups()
{
    ListNode<NotificationGroupData_t*>* node = groups_.Begin();
    while (node != groups_.End()) {
        NotificationGroupData_t* group = node->data_;
        if (group != nullptr && group->messages.Size() == 0) {
            ListNode<NotificationGroupData_t*>* nextNode = groups_.Next(node);
            delete group;
            groups_.Remove(node);
            node = nextNode;
        } else {
            node = groups_.Next(node);
        }
    }
}

MessageAdapter::NotificationGroupData_t* MessageAdapter::GetGroupDataByIndex(uint16_t index)
{
    ListNode<NotificationGroupData_t*>* node = groups_.Begin();
    for (uint16_t i = 0; i < index && node != groups_.End(); ++i) {
        node = groups_.Next(node);
    }
    if (node == nullptr || node == groups_.End()) {
        return nullptr;
    }
    return node->data_;
}

uint16_t MessageAdapter::GetTotalMessageCount() const
{
    uint16_t total = 0;
    ListNode<NotificationGroupData_t*>* node = const_cast<List<NotificationGroupData_t*>&>(groups_).Begin();
    const ListNode<NotificationGroupData_t*>* end = const_cast<List<NotificationGroupData_t*>&>(groups_).End();
    while (node != end) {
        NotificationGroupData_t* group = node->data_;
        if (group != nullptr) {
            total += group->messages.Size();
        }
        node = const_cast<List<NotificationGroupData_t*>&>(groups_).Next(node);
    }
    return total;
}
#endif

// ============================================================================
// MessageContentView Implementation - 消息内容展示区域
// ============================================================================
#if 1 // MessageContentView
MessageContentView::MessageContentView()
    : titleLabel_(nullptr), contentLabel_(nullptr), timeLabel_(nullptr), appNameLabel_(nullptr), iconView_(nullptr)
{
    SetTouchable(false);

    if (!CreateChilds()) {
        GRAPHIC_LOGE("MessageContentView: Failed to create childs");
        ClearChilds();
    }
}

MessageContentView::~MessageContentView()
{
    ClearChilds();
}

bool MessageContentView::CreateChilds()
{
    iconView_ = new UIImageView();
    if (iconView_ == nullptr) {
        GRAPHIC_LOGE("MessageContentView: Failed to create iconView_");
        return false;
    }
    iconView_->SetAutoEnable(false);
    iconView_->SetResizeMode(UIImageView::ImageResizeMode::FILL);
    Add(iconView_);

    titleLabel_ = new UILabel();
    if (titleLabel_ == nullptr) {
        GRAPHIC_LOGE("MessageContentView: Failed to create titleLabel_");
        return false;
    }
    titleLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, 29);
    titleLabel_->SetStyle(STYLE_TEXT_COLOR, Color::Black().full);
    Add(titleLabel_);

    contentLabel_ = new UILabel();
    if (contentLabel_ == nullptr) {
        GRAPHIC_LOGE("MessageContentView: Failed to create contentLabel_");
        return false;
    }
    contentLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, 28);
    contentLabel_->SetStyle(STYLE_TEXT_COLOR, Color::GetColorFromRGB(0xbd, 0xc1, 0xc6).full);
    Add(contentLabel_);

    timeLabel_ = new UILabel();
    if (timeLabel_ == nullptr) {
        GRAPHIC_LOGE("MessageContentView: Failed to create timeLabel_");
        return false;
    }
    timeLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, 20);
    timeLabel_->SetStyle(STYLE_TEXT_COLOR, Color::Gray().full);
    timeLabel_->SetAlign(TEXT_ALIGNMENT_RIGHT);
    Add(timeLabel_);

    appNameLabel_ = new UILabel();
    if (appNameLabel_ == nullptr) {
        GRAPHIC_LOGE("MessageContentView: Failed to create appNameLabel_");
        return false;
    }
    appNameLabel_->SetFont(DEFAULT_VECTOR_FONT_FILENAME, 28);
    appNameLabel_->SetStyle(STYLE_TEXT_COLOR, Color::Black().full);
    Add(appNameLabel_);

    return true;
}

void MessageContentView::ClearChilds()
{
    if (iconView_ != nullptr) {
        Remove(iconView_);
        delete iconView_;
        iconView_ = nullptr;
    }
    if (titleLabel_ != nullptr) {
        Remove(titleLabel_);
        delete titleLabel_;
        titleLabel_ = nullptr;
    }
    if (contentLabel_ != nullptr) {
        Remove(contentLabel_);
        delete contentLabel_;
        contentLabel_ = nullptr;
    }
    if (timeLabel_ != nullptr) {
        Remove(timeLabel_);
        delete timeLabel_;
        timeLabel_ = nullptr;
    }
    if (appNameLabel_ != nullptr) {
        Remove(appNameLabel_);
        delete appNameLabel_;
        appNameLabel_ = nullptr;
    }
}

void MessageContentView::UpdateContent(const MessageData& data)
{
    if (titleLabel_ != nullptr) {
        titleLabel_->SetText(data.title);
    }
    if (contentLabel_ != nullptr) {
        contentLabel_->SetText(data.content);
    }
    if (timeLabel_ != nullptr) {
        timeLabel_->SetText(data.timestamp);
    }
    if (iconView_ != nullptr && data.iconPath[0] != '\0') {
        iconView_->SetSrc(data.iconPath);
    }
    if (appNameLabel_ != nullptr) {
        appNameLabel_->SetText(data.appName);
    }

    SetStyle(STYLE_BACKGROUND_COLOR, static_cast<int64_t>(data.color.full));
    SetStyle(STYLE_BACKGROUND_OPA, data.isRead ? READ_OPACITY : static_cast<uint8_t>(OPA_OPAQUE));

    Invalidate();
}

void MessageContentView::LayoutLabels()
{
    if (titleLabel_ == nullptr || contentLabel_ == nullptr || timeLabel_ == nullptr || iconView_ == nullptr ||
        appNameLabel_ == nullptr) {
        return;
    }

    int16_t X = CONTENT_PADDING;
    int16_t Y = CONTENT_PADDING;
    iconView_->SetPosition(X, Y, ICON_SIZE, ICON_SIZE);

    X += ICON_SIZE + ICON_PADDING;
    int16_t contentWidth = GetWidth() - X - CONTENT_PADDING - TIME_LABEL_WIDTH;
    appNameLabel_->SetPosition(X, Y, contentWidth, LABEL_HEIGHT);

    X += contentWidth;
    Y += TITLE_TIME_GAP;
    timeLabel_->SetPosition(X, Y, TIME_LABEL_WIDTH, LABEL_HEIGHT);

    X = CONTENT_PADDING;
    Y += LABEL_HEIGHT;
    contentWidth = GetWidth() - CONTENT_PADDING - CONTENT_PADDING;
    titleLabel_->SetPosition(X, Y, contentWidth, TEXT_HEIGHT);

    Y += TEXT_HEIGHT;
    contentLabel_->SetPosition(X, Y);
    contentLabel_->SetWidth(contentWidth);
    contentLabel_->SetHeight(GetHeight() - CONTENT_PADDING - Y);
}
#endif

// ============================================================================
// UINotificationItem Implementation - 主容器，负责拖拽方向判断
// ============================================================================
#if 1
UINotificationItem::UINotificationItem()
    : Animator(this, this, DEFAULT_ANIMATION_DURATION, false),
      messageStackView_(nullptr),
      clearButton_(nullptr),
      itemListener_(nullptr),
      messageNumber_(0),
      messageId_(0),
      easingFunc_(EasingEquation::QuintEaseOut),
      targetPositionX_(0),
      startPositionX_(0),
      currentAnimationState_(CONTENT_SHOW),
      dragDirection_(UIList::HORIZONTAL),
      needConsumeEvent_(true)
{
    appName_[0] = '\0';
    SetTouchable(true);
    SetDraggable(true);
    SetDragParentInsteadAllowed(false);
    Resize(DEFAULT_ITEM_WIDTH, DEFAULT_ITEM_HEIGHT);
    SetStyle(STYLE_MARGIN_TOP, ITEM_MARGIN_TOP);
    SetStyle(STYLE_BACKGROUND_OPA, OPA_TRANSPARENT);

    if (!CreateChildViews()) {
        GRAPHIC_LOGE("UINotificationItem: Failed to create child views");
        ClearChildViews();
        return;
    }
}

UINotificationItem::~UINotificationItem()
{
    StopAnimation();
    ClearChildViews();
}

bool UINotificationItem::CreateChildViews()
{
    clearButton_ = new UILabelButton();
    if (clearButton_ != nullptr) {
        clearButton_->SetText("删除");
        clearButton_->SetStyle(STYLE_BACKGROUND_COLOR, Color::Red().full);
        clearButton_->SetStyle(STYLE_TEXT_COLOR, Color::White().full);
        clearButton_->SetOnClickListener(this);
        Add(clearButton_);
    }

    messageStackView_ = new MessageStackView();
    if (messageStackView_ != nullptr) {
        Add(messageStackView_);
    }
    return true;
}

void UINotificationItem::ClearChildViews()
{
    if (messageStackView_ != nullptr) {
        Remove(messageStackView_);
        delete messageStackView_;
        messageStackView_ = nullptr;
    }
    if (clearButton_ != nullptr) {
        clearButton_->SetOnClickListener(nullptr);
        Remove(clearButton_);
        delete clearButton_;
        clearButton_ = nullptr;
    }
    appName_[0] = '\0';
}

void UINotificationItem::LayoutChildViews()
{
    if (messageStackView_ == nullptr || clearButton_ == nullptr) {
        return;
    }

    const int16_t itemWidth = DEFAULT_ITEM_WIDTH;
    const int16_t itemHeight = DEFAULT_ITEM_HEIGHT;
    Resize(itemWidth, itemHeight);

    // 消息内容视图布局
    messageStackView_->SetStyle(STYLE_MARGIN_LEFT, ITEM_MARGIN_LEFT);
    messageStackView_->SetPosition(0, 0, itemWidth-2*ITEM_MARGIN_LEFT, itemHeight);
    messageStackView_->SetOffsetAndInset(STACK_HINT_OFFSET_Y, STACK_HINT_INSET);
    messageStackView_->SetStyle(STYLE_BACKGROUND_OPA, OPA_TRANSPARENT);
    messageStackView_->LayoutStack();

    // 清除按钮布局（初始在右侧可视区域外）
    int16_t btnY = (itemHeight - CLEAR_BUTTON_WIDTH) / 2;
    int16_t btnX = messageStackView_->GetWidthWithMargin() - itemHeight;
    clearButton_->SetStyle(STYLE_MARGIN_TOP, btnY);
    clearButton_->SetStyle(STYLE_MARGIN_LEFT, btnY);
    clearButton_->SetPosition(btnX, 0, CLEAR_BUTTON_WIDTH, CLEAR_BUTTON_WIDTH);
    clearButton_->SetStyle(STYLE_BORDER_RADIUS, CLEAR_BUTTON_WIDTH/2.0);
    clearButton_->SetVisible(false);
}

void UINotificationItem::UpdateMessageItem(const MessageData& data)
{
    SetMessageId(data.messageId);
    SetAppName(data.appName);
    if (messageStackView_ != nullptr) {
        messageStackView_->UpdateContent(data, messageNumber_);
    }
}

void UINotificationItem::SetMessageNumber(int number)
{
    messageNumber_ = number;
}

uint32_t UINotificationItem::GetMessageId() const
{
    return messageId_;
}

void UINotificationItem::SetMessageId(uint32_t messageId)
{
    messageId_ = messageId;
}

void UINotificationItem::SetAppName(const char* appName)
{
    if (appName == nullptr) {
        return;
    }
    MessageData::SafeCopyString(appName_, appName, MessageData::MAX_APP_NAME_LEN);
}

void UINotificationItem::UpdateChildPosition(int16_t offsetX)
{
    if (offsetX == 0 || messageStackView_ == nullptr) {
        return;
    }

    const int16_t newX = messageStackView_->GetX() + offsetX;
    SetChildPosition(newX);
}

void UINotificationItem::SetChildPosition(int16_t x)
{
    if (messageStackView_ == nullptr || clearButton_ == nullptr) {
        return;
    }

    const int16_t currentX = messageStackView_->GetX();
    if (x == currentX) {
        return;
    }

    messageStackView_->SetX(x);
    clearButton_->SetVisible(x < 0);

    float start = 0.6f;
    float progress = (- (float)x / DEFAULT_ITEM_HEIGHT);
    float scale = progress * progress * (1 - start) + start;
    if (scale < start) {
        scale = start;
    }
    if (scale > 1.0f) {
        scale = 1.0f;
    }

    const int16_t pivotY = CLEAR_BUTTON_WIDTH / 2;
    clearButton_->Scale(Vector2<float>(scale, scale), Vector2<float>(pivotY, pivotY));
    Invalidate();
}

uint8_t UINotificationItem::GetParentListDirection(UIView* view)
{
    UIView* parent = view ? view->GetParent() : nullptr;
    while (parent) {
        if (parent->GetViewType() == UI_LIST) {
            UIList* list = reinterpret_cast<UIList*>(parent);
            return list->GetDirection();
        }
        parent = parent->GetParent();
    }
    return UIList::VERTICAL;
}

bool UINotificationItem::OnDragStartEvent(const DragEvent& event)
{
    const uint8_t dragDir = event.GetDragDirection();
    const uint8_t listDir = GetParentListDirection(this);

    if (dragDir == DragEvent::DIRECTION_LEFT_TO_RIGHT || dragDir == DragEvent::DIRECTION_RIGHT_TO_LEFT) {
        dragDirection_ = UIList::HORIZONTAL;
    } else if (dragDir == DragEvent::DIRECTION_TOP_TO_BOTTOM || dragDir == DragEvent::DIRECTION_BOTTOM_TO_TOP) {
        dragDirection_ = UIList::VERTICAL;
    }

    StopAnimation();
    needConsumeEvent_ = (listDir != dragDirection_);
    return needConsumeEvent_;
}

bool UINotificationItem::OnDragEvent(const DragEvent& event)
{
    if (!needConsumeEvent_) {
        return false;
    }

    if (dragDirection_ == UIList::HORIZONTAL) {
        UpdateChildPosition(event.GetDeltaX());
    }
    return true;
}

bool UINotificationItem::OnDragEndEvent(const DragEvent& event)
{
    if (!needConsumeEvent_) {
        return false;
    }

    if (dragDirection_ == UIList::HORIZONTAL && messageStackView_ != nullptr) {
        const int16_t currentX = messageStackView_->GetX();
        const int16_t h = messageStackView_->GetHeight();
        const int16_t itemWidth = messageStackView_->GetWidthWithMargin();
        const int16_t threshold = h / 2;

        if (currentX < -h * 1.2) {
            currentAnimationState_ = CONTENT_HIDDEN;
            StartAutoCompleteAnimationTo(-itemWidth);
        } else if (currentX <= -threshold) {
            currentAnimationState_ = CONTENT_SHOW_IN_BUTTON_LEFT;
            StartAutoCompleteAnimationTo(-h);
        } else {
            currentAnimationState_ = CONTENT_SHOW;
            StartAutoCompleteAnimationTo(0);
        }
    }

    return true;
}

bool UINotificationItem::OnClick(UIView& view, const ClickEvent& event)
{
    if (&view == clearButton_ && itemListener_ != nullptr) {
        itemListener_->OnMessageDeleted(*this);
        return true;
    }
    return false;
}

bool UINotificationItem::OnClickEvent(const ClickEvent& event)
{
    if (itemListener_ != nullptr) {
        itemListener_->OnMessageClicked(*this);
        return true;
    }
    return false;
}

void UINotificationItem::StartAutoCompleteAnimationTo(int16_t targetX)
{
    StopAnimation();
    startPositionX_ = (messageStackView_ != nullptr) ? messageStackView_->GetX() : 0;
    targetPositionX_ = targetX;
    Start();
}

void UINotificationItem::Callback(UIView* view)
{
    if (view == nullptr) {
        return;
    }

    const uint16_t runTime = GetRunTime();
    const uint16_t duration = GetTime();

    if (duration == 0) {
        GRAPHIC_LOGE("UINotificationItem::Callback: invalid duration");
        return;
    }

    int16_t currentPosX;
    if (runTime >= duration) {
        currentPosX = targetPositionX_;
    } else {
        const float currentPos = easingFunc_(startPositionX_, targetPositionX_, runTime, duration);
        currentPosX = static_cast<int16_t>(currentPos);
    }

    SetChildPosition(currentPosX);

    if (currentPosX == targetPositionX_) {
        StopAnimation();
    }
}

void UINotificationItem::OnStop(UIView& view)
{
    GRAPHIC_LOGE("UINotificationItem::OnStop");
    if (clearButton_ == nullptr) {
        return;
    }

    switch (currentAnimationState_) {
        case CONTENT_HIDDEN:
            if (messageStackView_ != nullptr) {
                const int16_t x = messageStackView_->GetX();
                const int16_t width = messageStackView_->GetWidthWithMargin();
                GRAPHIC_LOGE("UINotificationItem::OnStop: x = %d, width = %d", x, width);
                if ((x <= -width || x >= width) && itemListener_ != nullptr) {
                    itemListener_->OnMessageDeleted(*this);
                }
            }
            clearButton_->SetVisible(false);
            break;
        case CONTENT_SHOW_IN_BUTTON_LEFT:
            clearButton_->SetVisible(true);
            break;
        case CONTENT_SHOW:
            clearButton_->SetVisible(false);
            break;
        default:
            break;
    }
}

void UINotificationItem::StopAnimation()
{
    if (GetState() != Animator::STOP) {
        Stop();
    }
}
#endif

MessageStackView::MessageStackView()
{
    SetTouchable(false);
    CreateChildViews();
}

MessageStackView::~MessageStackView()
{
    ClearChildViews();
}

bool MessageStackView::CreateChildViews()
{
    stackHintView_ = new UIView();
    if (stackHintView_ != nullptr) {
        Add(stackHintView_);
    }

    messageContentView_ = new MessageContentView();
    if (messageContentView_ != nullptr) {
        Add(messageContentView_);
    }
    return true;
}

void MessageStackView::ClearChildViews()
{
    if (stackHintView_ != nullptr) {
        Remove(stackHintView_);
        delete stackHintView_;
        stackHintView_ = nullptr;
    }
    if (messageContentView_ != nullptr) {
        Remove(messageContentView_);
        delete messageContentView_;
        messageContentView_ = nullptr;
    }
}

void MessageStackView::SetOffsetAndInset(int16_t offsetY, int16_t inset)
{
    offsetY_ = offsetY;
    inset_ = inset;
    LayoutStack();
}

void MessageStackView::UpdateContent(const MessageData& data, int num)
{
    messageContentView_->UpdateContent(data);

    if (num > 1) {
        stackHintView_->SetVisible(true);
    } else {
        stackHintView_->SetVisible(false);
    }
}

void MessageStackView::LayoutStack()
{
    const int16_t w = GetWidth();
    const int16_t h = GetHeight();

    int16_t height = 0;
    messageContentView_->SetPosition(0, 0, w, h);
    messageContentView_->SetStyle(STYLE_BORDER_RADIUS, 40);
    messageContentView_->LayoutLabels();
    height += h;
    if (stackHintView_->IsVisible()) {
        stackHintView_->SetPosition(inset_, offsetY_, w - 2 * inset_, h);
        stackHintView_->SetStyle(STYLE_BACKGROUND_COLOR, Color::Red().full);
        stackHintView_->SetStyle(STYLE_BORDER_RADIUS, 40);
        height += offsetY_;
    }
    SetHeight(height);
    GetParent()->SetHeight(height);

    Invalidate();
}

void UICircleList::SetScaleRange(float minScale, float maxScale)
{
    if (minScale > 0.05f && minScale < 1.0f) {
        minScale_ = minScale;
    }
    if (maxScale >= 1.0f && maxScale < 2.0f) {
        maxScale_ = maxScale;
    }
}

void UICircleList::SetFadeZoneRatio(float ratio)
{
    if (ratio >= 0.0f && ratio <= 0.5f) {
        fadeZoneRatio_ = ratio;
    }
}

void UICircleList::MoveChildByOffset(int16_t xOffset, int16_t yOffset)
{
    UIList::MoveChildByOffset(xOffset, yOffset);

    UIView* view = GetChildrenHead();
    while (view != nullptr) {
        ApplyCircularTransform(view);
        view = view->GetNextSibling();
    }
}

void UICircleList::PushBack(UIView* view)
{
    if (view == nullptr) {
        return;
    }
    UIList::PushBack(view);
    ApplyCircularTransform(view);
}

void UICircleList::ApplyCircularTransform(UIView* view)
{
    if (view == nullptr) {
        return;
    }

    const Rect& rect = view->GetRelativeRect();
    const int16_t itemBottom = rect.GetBottom();
    const int16_t itemWidth = rect.GetWidth();
    const int16_t itemHeight = view->GetHeight();

    const int16_t listTop = 0;
    const int16_t listH = GetHeight();
    const int16_t threshold = static_cast<int16_t>(listTop + fadeZoneRatio_ * listH);

    if (itemBottom < threshold) {
        float t = static_cast<float>(threshold - itemBottom) / static_cast<float>(threshold - listTop);
        if (t < 0.f)
            t = 0.f;
        if (t > 1.f)
            t = 1.f;

        float easedT = powf(t, EASE_EXPONENT);
        float scale = minScale_ + (1.0f - minScale_) * (1.0f - easedT);
        const float pivotX = static_cast<float>(itemWidth) * 0.5f;
        const float pivotY = static_cast<float>(itemHeight);

        view->ResetTransParameter();
        view->Scale(Vector2<float>(scale, scale), Vector2<float>(pivotX, pivotY));
#if 0
        uint8_t opa = static_cast<uint8_t>(OPA_OPAQUE * (1.0f - easedT));
        view->SetOpaScale(opa);
#endif
    } else {
        view->ResetTransParameter();
#if 0
        view->SetOpaScale(OPA_OPAQUE);
#endif
    }
}

} // namespace OHOS
