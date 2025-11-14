/**
 * @file ui_status_panel.h
 *
 * @brief Defines the attributes and common functions of a status panel.
 *
 * The status panel is hidden at the top of the screen by default and can be pulled down by gesture.
 * It follows finger position during dragging and automatically completes animation when released.
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef GRAPHIC_LITE_UI_STATUS_PANEL_H
#define GRAPHIC_LITE_UI_STATUS_PANEL_H

#include "components/ui_base_panel.h"

namespace OHOS {
class UIStatusPanel : public UIBasePanel {
public:
    UIStatusPanel();
    virtual ~UIStatusPanel();

    UIViewType GetViewType() const override
    {
        return UI_STATUS_PANEL;
    }

};
} // namespace OHOS
#endif // GRAPHIC_LITE_UI_STATUS_PANEL_H