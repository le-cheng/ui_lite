/**
 * @file ui_base_panel.h
 *
 * @brief Defines the base class for all panel components.
 *
 * This base class provides common functionality for panels that can be shown/hidden
 * with gesture support and animation. It reduces code duplication between different
 * panel implementations like UIStatusPanel and UINotificationPanel.
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef GRAPHIC_LITE_UI_BASE_PANEL_H
#define GRAPHIC_LITE_UI_BASE_PANEL_H

#include "animator/animator.h"
#include "components/ui_view_group.h"
#include "animator/easing_equation.h"

namespace OHOS {

/**
 * @brief Base class for all panel components
 */
class UIBasePanel : public UIViewGroup,
                   public AnimatorCallback,
                   public Animator {
public:
    class OnPanelListener : public HeapBase {
    public:
        virtual void OnShow(UIBasePanel& panel) {}
        virtual void OnHide(UIBasePanel& panel) {}
        virtual void OnPositionChanged(UIBasePanel& panel, float position) {}
        virtual ~OnPanelListener() = default;
    };

    enum PanelState : uint8_t {
        PANEL_HIDDEN,    /**< Panel is hidden */
        PANEL_SHOWING,   /**< Panel is being shown */
        PANEL_SHOWN,     /**< Panel is fully shown */
        PANEL_HIDING     /**< Panel is being hidden */
    };

    enum PanelDirection : uint8_t {
        LEFT_TO_RIGHT = 0,  /**< Panel slides from left to right */
        RIGHT_TO_LEFT = 1,  /**< Panel slides from right to left */
        TOP_TO_BOTTOM = 2,  /**< Panel slides from top to bottom */
        BOTTOM_TO_TOP = 3   /**< Panel slides from bottom to top */
    };

    UIBasePanel();
    virtual ~UIBasePanel();

    UIViewType GetViewType() const override
    {
        return UI_VIEW_GROUP;
    }

    // ============================================================================
    // Panel Configuration Methods
    // ============================================================================

    /**
     * @brief Set panel height
     * @param height Panel height in pixels
     */
    virtual void SetPanelHeight(int16_t height);

    /**
     * @brief Get panel height
     * @return Panel height in pixels
     */
    int16_t GetPanelHeight() const
    {
        return panelHeight_;
    }

    /**
     * @brief Set trigger area height
     * @param height Trigger area height in pixels
     */
    virtual void SetTriggerHeight(int16_t height);

    /**
     * @brief Get trigger area height
     * @return Trigger area height in pixels
     */
    int16_t GetTriggerHeight() const
    {
        return triggerHeight_;
    }

    /**
     * @brief Set auto-complete threshold
     * @param threshold Threshold value (0.0 - 1.0)
     */
    virtual void SetAutoCompleteThreshold(float threshold);

    /**
     * @brief Get auto-complete threshold
     * @return Threshold value
     */
    float GetAutoCompleteThreshold() const
    {
        return autoCompleteThreshold_;
    }

    /**
     * @brief Set animation duration
     * @param duration Duration in milliseconds
     */
    virtual void SetAnimationDuration(uint16_t duration);

    /**
     * @brief Get animation duration
     * @return Duration in milliseconds
     */
    uint16_t GetAnimationDuration() const
    {
        return GetTime();
    }

    /**
     * @brief Set panel direction
     * @param direction Panel slide direction
     */
    virtual void SetPanelDirection(int16_t direction);

    /**
     * @brief Get panel direction
     * @return Panel slide direction
     */
    int16_t GetPanelDirection() const
    {
        return panelDirection_;
    }

    /**
     * @brief Set easing function for animations
     * @param func Easing function
     */
    void SetEasingFunction(EasingFunc func) { easingFunc_ = func; }

    /**
     * @brief Get easing function
     * @return Easing function
     */
    EasingFunc GetEasingFunction() const
    {
        return easingFunc_;
    }

    // ============================================================================
    // Panel Control Methods
    // ============================================================================

    /**
     * @brief Show the panel
     * @param animated Whether to use animation
     */
    virtual void ShowPanel(bool animated = true);

    /**
     * @brief Hide the panel
     * @param animated Whether to use animation
     */
    virtual void HidePanel(bool animated = true);

    /**
     * @brief Toggle panel visibility
     * @param animated Whether to use animation
     */
    virtual void TogglePanel(bool animated = true);

    // ============================================================================
    // Panel State Methods
    // ============================================================================

    /**
     * @brief Get current panel state
     * @return Current panel state
     */
    PanelState GetPanelState() const
    {
        return panelState_;
    }

    /**
     * @brief Get current progress (0.0 = hidden, 1.0 = shown)
     * @return Current progress
     */
    float GetProgress() const
    {
        return progress_;
    }

    /**
     * @brief Check if panel is dragging
     * @return True if dragging
     */
    bool IsDragging() const
    {
        return isDragging_;
    }

    // ============================================================================
    // Listener Methods
    // ============================================================================

    /**
     * @brief Set panel listener
     * @param listener Panel event listener
     */
    void SetOnPanelListener(OnPanelListener* listener)
    {
        panelListener_ = listener;
    }

    /**
     * @brief Get panel listener
     * @return Panel event listener
     */
    OnPanelListener* GetOnPanelListener() const
    {
        return panelListener_;
    }

    // ============================================================================
    // Drag Control Methods
    // ============================================================================

    /**
     * @brief Enable or disable drag functionality
     * @param enabled True to enable drag
     */
    void SetDragEnabled(bool enabled)
    {
        isDragEnabled_ = enabled;
    }

    /**
     * @brief Check if drag is enabled
     * @return True if drag is enabled
     */
    bool IsDragEnabled() const
    {
        return isDragEnabled_;
    }

    // ============================================================================
    // Event Handling Methods (Override from UIView)
    // ============================================================================

    void GetTargetView(const Point& point, UIView** current, UIView** target) override;
    bool OnDragStartEvent(const DragEvent& event) override;
    bool OnDragEvent(const DragEvent& event) override;
    bool OnDragEndEvent(const DragEvent& event) override;

    // ============================================================================
    // Trigger Area Methods
    // ============================================================================

    /**
     * @brief Check if point is in trigger area
     * @param point Point to check
     * @return True if in trigger area
     */
    virtual bool IsInTriggerArea(const Point& point);

    /**
     * @brief Check if point is in extended trigger area
     * @param point Point to check
     * @return True if in extended trigger area
     */
    virtual bool IsInExtendedTriggerArea(const Point& point);

    /**
     * @brief Sets the drag function.
     *
     * @param func Indicates the easing function to set.
     * @since 1.0
     * @version 1.0
     */
    void SetDragFunc(EasingFunc func)
    {
        easingFunc_ = func;
    }

    /**
     * @brief Gets the drag function.
     *
     * @return Returns the easing function.
     * @since 1.0
     * @version 1.0
     */
    EasingFunc GetDragFunc() const
    {
        return easingFunc_;
    }

protected:
    // ============================================================================
    // Animation Callback Methods (Override from AnimatorCallback)
    // ============================================================================

    void Callback(UIView* view) override;
    void OnStop(UIView& view) override;

    // ============================================================================
    // Core Panel Methods (Template Method Pattern)
    // ============================================================================

    /**
     * @brief Update panel position during drag or animation
     * @param deltaY Y-axis movement delta
     */
    virtual void UpdatePanelPosition(int16_t deltaY);

    /**
     * @brief Set panel to specific position
     * @param targetY Target Y position
     */
    virtual void SetPanelPosition(int16_t targetY);

    /**
     * @brief Calculate initial position based on direction and height
     */
    virtual void SetInitialPosition();

    /**
     * @brief Start auto-complete animation to target state
     * @param targetState Target panel state
     */
    virtual void StartAutoCompleteAnimation(PanelState targetState);

    /**
     * @brief Stop current animation
     */
    virtual void StopAnimation();

    /**
     * @brief Update panel state based on progress
     * @param progress Current progress (0.0 - 1.0)
     */
    virtual void UpdatePanelState(float progress);

    /**
     * @brief Notify listeners of state change
     */
    virtual void NotifyStateChanged();

    /**
     * @brief Notify listeners of position change
     */
    virtual void NotifyPositionChanged();

    // ============================================================================
    // Velocity Detection Methods
    // ============================================================================

    /**
     * @brief Initialize velocity detection arrays
     */
    void InitDelta();

    /**
     * @brief Update velocity detection with new delta
     * @param distance Current drag distance
     */
    void RefreshDelta(int16_t distance);

    /**
     * @brief Get maximum delta from recent history
     * @return Maximum delta value
     */
    int16_t GetMaxDelta() const;

    /**
     * @brief Check if user is dragging to show panel
     * @return True if dragging to show
     */
    virtual bool IsDraggingToShow() const;

    /**
     * @brief Check if velocity is high enough for throw completion
     * @return True if should complete based on velocity
     */
    virtual bool ShouldThrowComplete() const;

    // ============================================================================
    // Protected Member Variables
    // ============================================================================

    // Panel configuration
    int16_t panelHeight_;
    int16_t triggerHeight_;
    float autoCompleteThreshold_;
    PanelDirection panelDirection_;

    // Panel state
    PanelState panelState_;
    float progress_;
    bool isDragging_;
    bool isDragEnabled_;
    bool isAnimationEnabled_;

    // Constants
    static constexpr uint8_t MAX_DELTA_SIZE = 3;
    static constexpr int16_t THROW_THRESHOLD = 10;
    static constexpr uint16_t DEFAULT_ANIMATION_DURATION = 300;
    static constexpr float DEFAULT_AUTO_COMPLETE_THRESHOLD = 0.5f;
    static constexpr int16_t DEFAULT_TRIGGER_HEIGHT = 70;
    static constexpr int16_t DEFAULT_PANEL_HEIGHT = 466;

    // Position and animation
    int16_t initialPositionY_;
    float startPositionY_;
    float endPositionY_;
    EasingFunc easingFunc_;
    float inverseInitialPositionY_;

    // Velocity detection
    int16_t recentDelta_[MAX_DELTA_SIZE];
    int16_t lastDelta_;
    uint8_t deltaIndex_;

    // Listener
    OnPanelListener* panelListener_;
};

} // namespace OHOS

#endif // GRAPHIC_LITE_UI_BASE_PANEL_H