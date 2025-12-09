/**
 * @file ui_notification_panel.h
 *
 * @brief Defines the attributes and common functions of a notification panel.
 *
 * The notification panel is inspired by Apple Watch notifications, supporting
 * pull-up gesture from bottom, scrollable message list, and swipe-to-clear functionality.
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef GRAPHIC_LITE_UI_NOTIFICATION_PANEL_H
#define GRAPHIC_LITE_UI_NOTIFICATION_PANEL_H

#include "components/ui_base_panel.h"
#include "components/ui_list6.h"
#include "components/ui_label.h"
#include "components/ui_label_button.h"
#include "components/ui_image_view.h"
#include "securec.h"

namespace OHOS {

/**
 * @brief Message type enumeration
 */
enum MessageType : uint8_t {
    MESSAGE_TYPE_NORMAL = 0,
    MESSAGE_TYPE_WARNING = 1,
    MESSAGE_TYPE_ERROR = 2,
    MESSAGE_TYPE_INFO = 3
};

/**
 * @brief Message data structure
 */
struct MessageData {
    static constexpr uint16_t MAX_TITLE_LEN = 64;
    static constexpr uint16_t MAX_CONTENT_LEN = 128;
    static constexpr uint16_t MAX_TIMESTAMP_LEN = 16;
    static constexpr uint16_t MAX_ICON_PATH_LEN = 128;
    static constexpr uint16_t MAX_APP_NAME_LEN = 16;

    char appName[MAX_APP_NAME_LEN];      // 应用名称（存储内容）
    char title[MAX_TITLE_LEN];           // 消息标题（存储内容）
    char content[MAX_CONTENT_LEN];       // 消息内容（存储内容）
    char timestamp[MAX_TIMESTAMP_LEN];   // 时间戳（存储内容）
    char iconPath[MAX_ICON_PATH_LEN];    // 图标路径（存储内容）
    uint32_t messageId;
    MessageType type;
    bool isRead;
    ColorType color;  // 消息背景颜色

    MessageData() : messageId(0), type(MESSAGE_TYPE_NORMAL), isRead(false), color(Color::White())
    {
        ClearStrings();
    }

    /**
     * @brief 构造函数，安全拷贝各字符串字段并初始化其他成员
     * @param app 应用名字符串，允许为 nullptr
     * @param t 标题字符串，允许为 nullptr
     * @param c 内容字符串，允许为 nullptr
     * @param ts 时间戳字符串，允许为 nullptr
     * @param id 消息ID
     * @param tp 消息类型
     * @param read 是否已读
     * @param col 背景颜色
     * @param icon 图标路径字符串，允许为 nullptr
     */
    MessageData(const char* app, const char* t, const char* c, const char* ts, uint32_t id,
               MessageType tp = MESSAGE_TYPE_NORMAL, bool read = false,
               ColorType col = Color::White(), const char* icon = nullptr)
        : messageId(id), type(tp), isRead(read), color(col)
    {
        SafeCopyString(appName, app, MAX_APP_NAME_LEN);
        SafeCopyString(title, t, MAX_TITLE_LEN);
        SafeCopyString(content, c, MAX_CONTENT_LEN);
        SafeCopyString(timestamp, ts, MAX_TIMESTAMP_LEN);
        SafeCopyString(iconPath, icon, MAX_ICON_PATH_LEN);
    }

    static void SafeCopyString(char* dest, const char* src, uint16_t maxLen)
    {
        if (dest == nullptr || maxLen == 0) {
            return;
        }
        if (src == nullptr) {
            dest[0] = '\0';
            return;
        }
        errno_t rc = strncpy_s(dest, static_cast<size_t>(maxLen), src, static_cast<size_t>(maxLen - 1));
        if (rc != 0) {
            dest[0] = '\0';
        }
    }

private:
    void ClearStrings()
    {
        appName[0] = '\0';
        title[0] = '\0';
        content[0] = '\0';
        timestamp[0] = '\0';
        iconPath[0] = '\0';
    }
};

/**
 * @brief 消息内容视图类
 * 负责显示消息的标题、内容和时间戳
 */
class MessageContentView : public UIViewGroup {
public:
    MessageContentView();
    virtual ~MessageContentView();

    bool CreateChilds();
    void ClearChilds();
    void UpdateContent(const MessageData& data);
    void LayoutLabels();
    void CopyFrom(const MessageContentView& src);

private:
    UILabel* titleLabel_;
    UILabel* contentLabel_;
    UILabel* timeLabel_;
    UILabel* appNameLabel_;
    UIImageView* iconView_;
};

class MessageStackView : public UIViewGroup {
public:
    MessageStackView();
    virtual ~MessageStackView();

    UIViewType GetViewType() const override { return UI_VIEW_GROUP; }

    bool CreateChildViews();
    void ClearChildViews();
    void SetOffsetAndInset(int16_t offsetY, int16_t inset);
    void UpdateContent(const MessageData& data, int num);
    void LayoutStack();
    MessageContentView* GetMessageContentView() const { return messageContentView_; }

private:
    int16_t offsetY_ = 6;
    int16_t inset_ = 8;
    MessageContentView* messageContentView_ = nullptr;
    UIView* stackHintView_ = nullptr;
};

class UICircleList : public UIList6 {
public:
    explicit UICircleList(uint8_t direction = VERTICAL) : UIList6(direction){}

    ~UICircleList(){}

    void MoveChildByOffset(int16_t xOffset, int16_t yOffset) override;
    void SetScaleRange(float minScale, float maxScale);
    void SetFadeZoneRatio(float ratio);
protected:
    void PushBack(UIView* view) override;
private:
    static constexpr float DEFAULT_MIN_SCALE = 0.60f;   // 顶部最小缩放
    static constexpr float DEFAULT_MAX_SCALE = 1.15f;   // 底部最大缩放
    static constexpr float DEFAULT_FADE_RATIO = 0.12f;  // 顶部淡出区域比例
    static constexpr float EASE_EXPONENT = 1.4f;       // 位置到缩放的缓动指数

    void ApplyCircularTransform(UIView* view);
    float minScale_ = DEFAULT_MIN_SCALE;
    float maxScale_ = DEFAULT_MAX_SCALE;
    float fadeZoneRatio_ = DEFAULT_FADE_RATIO;
};

class OnMessageListener : public HeapBase {
public:
    virtual ~OnMessageListener() {}
    virtual void OnMessageClicked(UIView& item) {}
    virtual void OnMessageDeleted(UIView& item) {}
    virtual void OnExitAppMessages(UIView& item) {}
    virtual void OnClearMessages() {}
};

class INotificationDataProvider : public HeapBase {
public:
    virtual ~INotificationDataProvider() {}
    virtual uint16_t GetAppCount() const = 0;
    virtual List<MessageData>* GetMessagesByAppIndex(int16_t index) const = 0;
    virtual List<MessageData>* GetMessagesByAppName(const char* appName) const = 0;
    virtual uint16_t GetMessagesCountByAppName(const char* appName) const = 0;
    virtual MessageData* GetAppMessageByIndex(const char* appName, int16_t index) = 0;

    virtual void AddMessage(const MessageData& message) = 0;
    virtual uint32_t RemoveMessageById(const char* appName, uint32_t messageId) = 0;
    virtual void RemoveMessagesByAppName(const char* appName) = 0;
    virtual void ClearAll() = 0;
    virtual int16_t GetTotalMessageCount() const = 0;
};

class UINotificationItem : public UIViewGroup, public UIView::OnClickListener, public Animator, public AnimatorCallback {
public:
    UINotificationItem();
    virtual ~UINotificationItem();

    UIViewType GetViewType() const override { return UI_VIEW_GROUP; }

    uint32_t GetMessageId() const;
    void SetMessageId(uint32_t messageId);

    const char* GetAppName() const { return appName_; }
    void SetAppName(const char* appName);
    void SetRead(bool isRead);

    void SetMessageListener(OnMessageListener* listener) { itemListener_ = listener; }
    OnMessageListener* GetMessageListener() const { return itemListener_; }

    bool OnDragStartEvent(const DragEvent& event) override;
    bool OnDragEvent(const DragEvent& event) override;
    bool OnDragEndEvent(const DragEvent& event) override;
    bool OnClickEvent(const ClickEvent& event) override;
    bool OnClick(UIView& view, const ClickEvent& event) override;

    void UpdateChildPosition(int16_t offsetX);
    void SetChildPosition(int16_t x);
    void UpdateMessageItem(const MessageData& message);
    void SetAsFooter(bool isFooter);

    // 动画相关定义
    enum AnimationState : uint8_t {
        CONTENT_HIDDEN = 0,    // 向左隐藏（删除动画）
        CONTENT_SHOW_IN_BUTTON_LEFT,   // 显示在按钮左侧（展开状态）
        CONTENT_SHOW          // 向右显示（回弹到原位）
    };

    void StartAutoCompleteAnimationTo(int16_t targetX);

    void OnStop(UIView& view) override;
    void Callback(UIView* view) override;
    void StopAnimation();

    uint8_t GetParentListDirection(UIView* view);
    void LayoutChildViews();
    void SetMessageNumber(int number);
    MessageContentView* GetMessageContentView() const;

private:
    bool CreateChildViews();
    void ClearChildViews();

    // 子视图组件
    MessageStackView* messageStackView_;
    UILabelButton* clearButton_;
    OnMessageListener* itemListener_;

    int messageNumber_;
    uint32_t messageId_;
    char appName_[MessageData::MAX_APP_NAME_LEN];
    bool isFooter_ = false;

    // 动画相关成员
    EasingFunc easingFunc_;
    int16_t targetPositionX_;
    int16_t startPositionX_;

    // 状态标志
    AnimationState currentAnimationState_;
    uint8_t dragDirection_;
    bool needConsumeEvent_;

    static constexpr uint16_t DEFAULT_ANIMATION_DURATION = 300;
};

class MessageAdapter : public AbstractAdapter {
public:
    MessageAdapter();
    virtual ~MessageAdapter();

    uint16_t GetCount() override;
    UIView* GetView(UIView* inView, int16_t index) override;

    void SetMessageListener(OnMessageListener* listener)
    {
        itemListener_ = listener;
    }

    OnMessageListener* GetMessageListener() const
    {
        return itemListener_;
    }

    void SetDataProvider(INotificationDataProvider* provider)
    {
        dataProvider_ = provider;
    }

    enum ViewMode : uint8_t { GROUP_SUMMARY = 0, APP_DETAIL = 1 };
    void SetMode(ViewMode mode) { mode_ = mode; }
    ViewMode GetMode() const { return mode_; }
    void SetAppName(const char* appName)
    {
        MessageData::SafeCopyString(appName_, appName, MessageData::MAX_APP_NAME_LEN);
    }
    const char* GetAppName() const { return appName_; }

    UINotificationItem* CreateMessageItem(UIView* inView);
    void DeleteView(UIView*& view) override;
private:
    uint16_t GetOriginCount();
    OnMessageListener* itemListener_;
    INotificationDataProvider* dataProvider_ = nullptr;
    ViewMode mode_ = GROUP_SUMMARY;
    char appName_[MessageData::MAX_APP_NAME_LEN];
};

class UINotificationPanel : public UIBasePanel, public OnMessageListener, public UIView::OnClickListener {
public:
    UINotificationPanel();
    virtual ~UINotificationPanel();

    UIViewType GetViewType() const override
    {
        return UI_NOTIFICATION_PANEL;
    }

    // ============================================================================
    // 消息管理接口
    // ============================================================================
    uint32_t RemoveMessage(const char* appName, uint32_t messageId);
    void RefreshMessages();
    void SetMessageListener(OnMessageListener* listener)
    {
        externalMessageListener_ = listener;
    }

    void OnMessageClicked(UIView& item) override;
    void OnMessageDeleted(UIView& item) override;
    void OnExitAppMessages(UIView& item) override;
    void OnClearMessages() override;
    void GetTargetView(const Point& point, UIView** current, UIView** target) override;
    bool OnClick(UIView& view, const ClickEvent& event) override;

    void ShowAppMessageList(const char* appName);
    void ShowMainMessageList();
    void HideMessageDetail();
    void HideMessageDetailAnimated();
    void HideMessageDetailAnimatedDown();
    void ShowMessageDetail(UINotificationItem& item);

    void SetDataProvider(INotificationDataProvider* provider)
    {
        dataProvider_ = provider;
        messageAdapter_.SetDataProvider(provider);
    }
    void RemoveAppMessages(const char* appName);

protected:
    bool InitializeComponents();

private:
    bool CreateHandleView();
    bool CreateMessageList();
    bool CreateEmptyView();
    void UpdateEmptyStateVisibility();
    void CleanupComponents();
    void UpdateDetailOverlayGeometry(const Rect& rect);
    void SetDetailOverlayOpacity(uint8_t opa);

    UICircleList* messageList_;
    UIViewGroup* headerView_;
    UIView* handleView_;

    // 空状态视图组件
    UIViewGroup* emptyView_;
    UIView* emptyIconView_;
    UILabel* emptyLabel_;
    MessageAdapter messageAdapter_;
    OnMessageListener* externalMessageListener_;
    bool isInitialized_;
    INotificationDataProvider* dataProvider_ = nullptr;
    UIViewGroup* detailOverlay_ = nullptr;
    MessageContentView* detailContentView_ = nullptr;
    class DetailExpandAnimator;
    DetailExpandAnimator* detailAnimator_ = nullptr;
    UILabelButton* detailDeleteButton_ = nullptr;
    Rect detailStartRect_;
    bool hasDetailStartRect_ = false;
    char detailAppName_[MessageData::MAX_APP_NAME_LEN];
    uint32_t detailMessageId_ = 0;
};

} // namespace OHOS
#endif // GRAPHIC_LITE_UI_NOTIFICATION_PANEL_H
