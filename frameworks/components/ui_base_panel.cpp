#include "components/ui_base_panel.h"

namespace OHOS {

UIBasePanel::UIBasePanel()
    : Animator(this, this, DEFAULT_ANIMATION_DURATION, false),
      panelHeight_(DEFAULT_PANEL_HEIGHT), triggerHeight_(DEFAULT_TRIGGER_HEIGHT),
      autoCompleteThreshold_(DEFAULT_AUTO_COMPLETE_THRESHOLD),
      panelDirection_(TOP_TO_BOTTOM), panelState_(PANEL_HIDDEN),
      progress_(0.0f), isDragging_(false), isDragEnabled_(true),
      isAnimationEnabled_(true), initialPositionY_(0),
      startPositionY_(0.0f), endPositionY_(0.0f),
      easingFunc_(EasingEquation::QuintEaseOut),
      inverseInitialPositionY_(0.0f), lastDelta_(0),
      deltaIndex_(0), panelListener_(nullptr)
{
    SetTouchable(true);
    SetDraggable(true);
    InitDelta();
    SetInitialPosition();
}

UIBasePanel::~UIBasePanel()
{
    StopAnimation();
}

// ============================================================================
// Panel Configuration Methods
// ============================================================================

void UIBasePanel::SetPanelHeight(int16_t height)
{
    if (height <= 0) {
        GRAPHIC_LOGE("UIBasePanel::SetPanelHeight invalid height: %d", height);
        return;
    }

    panelHeight_ = height;
    SetHeight(panelHeight_);
    SetInitialPosition();
}

void UIBasePanel::SetTriggerHeight(int16_t height)
{
    if (height <= 0) {
        GRAPHIC_LOGE("UIBasePanel::SetTriggerHeight invalid height: %d", height);
        return;
    }

    triggerHeight_ = height;
}

void UIBasePanel::SetAutoCompleteThreshold(float threshold)
{
    if (threshold < 0.0f || threshold > 1.0f) {
        GRAPHIC_LOGE("UIBasePanel::SetAutoCompleteThreshold invalid threshold: %f", threshold);
        return;
    }

    autoCompleteThreshold_ = threshold;
}

void UIBasePanel::SetAnimationDuration(uint16_t duration)
{
    if (duration == 0) {
        GRAPHIC_LOGE("UIBasePanel::SetAnimationDuration invalid duration: %d", duration);
        return;
    }

    SetTime(duration);
}

void UIBasePanel::SetPanelDirection(int16_t direction)
{
    if (panelDirection_ == direction) {
        return;
    }

    panelDirection_ = static_cast<PanelDirection>(direction);
    SetInitialPosition();
}

// ============================================================================
// Panel Control Methods
// ============================================================================

void UIBasePanel::ShowPanel(bool animated)
{
    if (panelState_ == PANEL_SHOWN) {
        return;
    }

    if (animated && isAnimationEnabled_) {
        StartAutoCompleteAnimation(PANEL_SHOWN);
    } else {
        SetPanelPosition(0);
    }
}

void UIBasePanel::HidePanel(bool animated)
{
    if (panelState_ == PANEL_HIDDEN) {
        return;
    }

    if (animated && isAnimationEnabled_) {
        StartAutoCompleteAnimation(PANEL_HIDDEN);
    } else {
        SetInitialPosition();
    }
}

void UIBasePanel::TogglePanel(bool animated)
{
    if (panelState_ == PANEL_SHOWN) {
        HidePanel(animated);
    } else {
        ShowPanel(animated);
    }
}

// ============================================================================
// Event Handling Methods
// ============================================================================

void UIBasePanel::GetTargetView(const Point& point, UIView** current, UIView** target)
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

bool UIBasePanel::OnDragStartEvent(const DragEvent& event)
{
    if (!IsDragEnabled()) {
        return false;
    }

    if (!IsInTriggerArea(event.GetStartPoint())) {
        return false;
    }

    isDragging_ = true;
    StopAnimation();
    return UIView::OnDragStartEvent(event);
}

bool UIBasePanel::OnDragEvent(const DragEvent& event)
{
    if (!isDragging_) {
        return false;
    }

    RefreshDelta(event.GetDeltaY());
    UpdatePanelPosition(event.GetDeltaY());
    return UIView::OnDragEvent(event);
}

bool UIBasePanel::OnDragEndEvent(const DragEvent& event)
{
    (void)event;
    if (!isDragging_) {
        return false;
    }

    isDragging_ = false;

    bool shouldAutoComplete = (progress_ >= autoCompleteThreshold_);
    if (ShouldThrowComplete()) {
        shouldAutoComplete = IsDraggingToShow();
    }

    if (shouldAutoComplete) {
        ShowPanel(isAnimationEnabled_);
    } else {
        HidePanel(isAnimationEnabled_);
    }

    InitDelta();
    return UIView::OnDragEndEvent(event);
}

// ============================================================================
// Trigger Area Methods
// ============================================================================

bool UIBasePanel::IsInTriggerArea(const Point& point)
{
    Rect triggerRect = GetRect();

    switch (panelDirection_) {
        case TOP_TO_BOTTOM:
            triggerRect.SetY(triggerRect.GetY() + GetHeight() - triggerHeight_);
            triggerRect.SetHeight(triggerHeight_ * 2);
            break;
        case BOTTOM_TO_TOP:
            triggerRect.SetY(triggerRect.GetY() - triggerHeight_);
            triggerRect.SetHeight(triggerHeight_ * 2);
            break;
        case LEFT_TO_RIGHT:
            triggerRect.SetX(triggerRect.GetX() + GetWidth() - triggerHeight_);
            triggerRect.SetWidth(triggerHeight_ * 2);
            break;
        case RIGHT_TO_LEFT:
            triggerRect.SetX(triggerRect.GetX() - triggerHeight_);
            triggerRect.SetWidth(triggerHeight_ * 2);
            break;
        default:
            return false;
    }

    return triggerRect.IsContains(point);
}

bool UIBasePanel::IsInExtendedTriggerArea(const Point& point)
{
    Rect triggerRect = GetRect();
    switch (panelDirection_) {
        case TOP_TO_BOTTOM:
            triggerRect.SetHeight(triggerHeight_ + GetHeight());
            break;
        case BOTTOM_TO_TOP:
            triggerRect.SetHeight(triggerHeight_ + GetHeight());
            triggerRect.SetY(triggerRect.GetY() - triggerHeight_);
            break;
        case LEFT_TO_RIGHT:
            triggerRect.SetWidth(triggerHeight_ + GetWidth());
            break;
        case RIGHT_TO_LEFT:
            triggerRect.SetWidth(triggerHeight_ + GetWidth());
            triggerRect.SetX(triggerRect.GetX() - triggerHeight_);
            break;
        default:
            break;
    }
    return triggerRect.IsContains(point);
}

// ============================================================================
// Animation Callback Methods
// ============================================================================

void UIBasePanel::Callback(UIView* view)
{
    if (view == nullptr) {
        GRAPHIC_LOGE("UIBasePanel::Callback: view is null");
        return;
    }

    const uint16_t runTime = GetRunTime();
    const uint16_t duration = GetTime();

    if (duration == 0) {
        GRAPHIC_LOGE("UIBasePanel::Callback: invalid duration");
        return;
    }

    if (runTime >= duration) {
        SetPanelPosition(endPositionY_);
    } else {
        const float currentPos = easingFunc_(startPositionY_, endPositionY_, runTime, duration);
        SetPanelPosition(currentPos);
    }
}

void UIBasePanel::OnStop(UIView& view)
{
    (void)view;
}

// ============================================================================
// Core Panel Methods
// ============================================================================

void UIBasePanel::UpdatePanelPosition(int16_t deltaY)
{
    if (deltaY == 0) {
        return;
    }

    int16_t newPos = GetY() + deltaY;

    // Constrain position based on panel direction
    switch (panelDirection_) {
        case TOP_TO_BOTTOM:
            newPos = MATH_MAX(initialPositionY_, MATH_MIN(0, newPos));
            break;
        case BOTTOM_TO_TOP:
            newPos = MATH_MAX(0, MATH_MIN(initialPositionY_, newPos));
            break;
        default:
            // For horizontal panels, override in subclass
            break;
    }

    SetPanelPosition(newPos);
}

void UIBasePanel::SetPanelPosition(int16_t targetY)
{
    if (GetY() == targetY) {
        return;
    }

    const Rect preRect = GetRect();
    SetY(targetY);

    // Update progress
    const float newProgress = MATH_MAX(0.0f, MATH_MIN(1.0f,
        1.0f - static_cast<float>(targetY) * inverseInitialPositionY_));

    if (!MATH_FLT_EQUAL(newProgress, progress_)) {
        progress_ = newProgress;
        NotifyPositionChanged();
    }

    UpdatePanelState(progress_);

    // Invalidate affected area
    Rect invalidated;
    invalidated.Join(preRect, GetRect());
    InvalidateRect(invalidated);
}

void UIBasePanel::SetInitialPosition()
{
    switch (panelDirection_) {
        case TOP_TO_BOTTOM:
            initialPositionY_ = -panelHeight_;
            break;
        case BOTTOM_TO_TOP:
            initialPositionY_ = panelHeight_;
            break;
        default:
            // For horizontal panels, override in subclass
            initialPositionY_ = 0;
            break;
    }

    SetPanelPosition(initialPositionY_);

    if (initialPositionY_ != 0) {
        inverseInitialPositionY_ = 1.0f / static_cast<float>(initialPositionY_);
    } else {
        inverseInitialPositionY_ = 0.0f;
    }
}

void UIBasePanel::StartAutoCompleteAnimation(PanelState targetState)
{
    StopAnimation();
    startPositionY_ = GetY();
    endPositionY_ = (targetState == PANEL_SHOWN) ? 0.0f : static_cast<float>(initialPositionY_);
    Start();
}

void UIBasePanel::StopAnimation()
{
    if (GetState() != Animator::STOP) {
        Stop();
    }
}

void UIBasePanel::UpdatePanelState(float progress)
{
    PanelState newState = panelState_;
    if (MATH_FLT_EQUAL(progress, 0.0f)) {
        newState = PANEL_HIDDEN;
    } else if (MATH_FLT_EQUAL(progress, 1.0f)) {
        newState = PANEL_SHOWN;
    } else {
        newState = PANEL_SHOWING;
    }

    if (newState != panelState_) {
        panelState_ = newState;
        NotifyStateChanged();
    }
}

void UIBasePanel::NotifyStateChanged()
{
    if (panelListener_ == nullptr) {
        return;
    }

    switch (panelState_) {
        case PANEL_SHOWN:
            panelListener_->OnShow(*this);
            break;
        case PANEL_HIDDEN:
            panelListener_->OnHide(*this);
            break;
        default:
            break;
    }
}

void UIBasePanel::NotifyPositionChanged()
{
    if (panelListener_ == nullptr) {
        return;
    }
    panelListener_->OnPositionChanged(*this, progress_);
}

// ============================================================================
// Velocity Detection Methods
// ============================================================================

void UIBasePanel::InitDelta()
{
    for (uint8_t i = 0; i < MAX_DELTA_SIZE; ++i) {
        recentDelta_[i] = 0;
    }
    deltaIndex_ = 0;
    lastDelta_ = 0;
}

void UIBasePanel::RefreshDelta(int16_t distance)
{
    recentDelta_[deltaIndex_] = distance;
    deltaIndex_ = (deltaIndex_ + 1) % MAX_DELTA_SIZE;
    lastDelta_ = distance;
}

int16_t UIBasePanel::GetMaxDelta() const
{
    int16_t maxDelta = 0;
    for (uint8_t i = 0; i < MAX_DELTA_SIZE; ++i) {
        const int16_t absDelta = MATH_ABS(recentDelta_[i]);
        maxDelta = MATH_MAX(maxDelta, absDelta);
    }
    return maxDelta;
}

bool UIBasePanel::IsDraggingToShow() const
{
    switch (panelDirection_) {
        case TOP_TO_BOTTOM:
        case LEFT_TO_RIGHT:
            return lastDelta_ > 0;
        case BOTTOM_TO_TOP:
        case RIGHT_TO_LEFT:
            return lastDelta_ < 0;
        default:
            return false;
    }
}

bool UIBasePanel::ShouldThrowComplete() const
{
    return GetMaxDelta() >= THROW_THRESHOLD;
}

} // namespace OHOS
