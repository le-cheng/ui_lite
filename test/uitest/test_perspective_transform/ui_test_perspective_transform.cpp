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

#include "ui_test_perspective_transform.h"
#include "common/screen.h"
#include "components/ui_label.h"
#include "components/ui_scroll_view.h"
#include "components/ui_slider.h"
#include "engines/gfx/gfx_engine_manager.h"
#include "draw/draw_utils.h"
#include "test_resource_config.h"
#include "components/ui_earth.h"
#include <cstdio>

namespace OHOS {

namespace {
    enum EarthParamType {
        PARAM_ROT_X,
        PARAM_ROT_Y,
        PARAM_SCALE,
        PARAM_TRANS_X,
        PARAM_TRANS_Y,
        PARAM_TRANS_Z
    };

    struct EarthState {
        float rotX = 65.0f;
        float rotY = 33.0f;
        float scale = 1.0f;
        float transX = 0.0f;
        float transY = 0.0f;
        float transZ = 0.0f;
    };

    static EarthState g_earthState;

    class EarthConfigListener : public UISlider::UISliderEventListener {
    public:
        EarthConfigListener(UIEarth* earth, EarthParamType type, UILabel* label)
            : earth_(earth), type_(type), label_(label) {}

        void OnChange(int32_t value) override {
            if (!earth_ || !label_) return;

            char buf[64] = {0};

            switch (type_) {
                case PARAM_ROT_X: // Range 0-360 -> -180 to 180
                    g_earthState.rotX = static_cast<float>(value - 180);
                    sprintf_s(buf, sizeof(buf), "RotX: %.1f", g_earthState.rotX);
                    earth_->SetCenter(g_earthState.rotX, g_earthState.rotY);
                    break;
                case PARAM_ROT_Y: // Range 0-180 -> -90 to 90
                    g_earthState.rotY = static_cast<float>(value - 90);
                    sprintf_s(buf, sizeof(buf), "RotY: %.1f", g_earthState.rotY);
                    earth_->SetCenter(g_earthState.rotX, g_earthState.rotY);
                    break;
                case PARAM_SCALE: // Range 1-50 -> 0.1 to 5.0
                    g_earthState.scale = static_cast<float>(value) / 10.0f;
                    sprintf_s(buf, sizeof(buf), "Scale: %.1f", g_earthState.scale);
                    earth_->SetScale(g_earthState.scale);
                    break;
                case PARAM_TRANS_X: // Range 0-1000 -> -500 to 500
                    g_earthState.transX = static_cast<float>(value - 500);
                    sprintf_s(buf, sizeof(buf), "TransX: %.1f", g_earthState.transX);
                    earth_->SetTranslation(g_earthState.transX, g_earthState.transY, g_earthState.transZ);
                    break;
                case PARAM_TRANS_Y: // Range 0-1000 -> -500 to 500
                    g_earthState.transY = static_cast<float>(value - 500);
                    sprintf_s(buf, sizeof(buf), "TransY: %.1f", g_earthState.transY);
                    earth_->SetTranslation(g_earthState.transX, g_earthState.transY, g_earthState.transZ);
                    break;
                case PARAM_TRANS_Z: // Range 0-1000 -> -500 to 500
                    g_earthState.transZ = static_cast<float>(value - 500);
                    sprintf_s(buf, sizeof(buf), "TransZ: %.1f", g_earthState.transZ);
                    earth_->SetTranslation(g_earthState.transX, g_earthState.transY, g_earthState.transZ);
                    break;
            }
            label_->SetText(buf);
        }

    private:
        UIEarth* earth_;
        EarthParamType type_;
        UILabel* label_;
    };
}

class UIPerspectiveView : public UIView {
public:
    UIPerspectiveView() {}
    ~UIPerspectiveView() {}
    uint8_t* data = nullptr;

    void OnDraw(BufferInfo& gfxDstBuffer, const Rect& invalidatedArea) override
    {
        uint16_t width = 120;
        uint16_t height = 120;
        uint32_t dataSize = width * height * 4;

        if (data == nullptr) {
            data = new uint8_t[dataSize];
            if (data == nullptr) {
                return;
            }
        }

        for (uint32_t i = 0; i < dataSize; i += 4) {
            data[i] = 0;
            data[i + 1] = 0;
            data[i + 2] = 255;
            data[i + 3] = 255;
        }

        BufferInfo srcBuf;
        srcBuf.rect = {0, 0, static_cast<int16_t>(width - 1), static_cast<int16_t>(height - 1)};
        srcBuf.mode = ARGB8888;
        srcBuf.color = 0;
        srcBuf.phyAddr = srcBuf.virAddr = data;
        srcBuf.stride = width * (DrawUtils::GetPxSizeByColorMode(srcBuf.mode) >> 3);
        srcBuf.width = width;
        srcBuf.height = height;
        Rect srcRect(0, 0, width - 1, height - 1);

        PointF srcQuad[4] = {{0, 0},
                            {width - 1, 0},
                            {width - 1, height - 1},
                            {0, height - 1}};
        int16_t indent = 30;
        PointF dstQuad[4] = {{0, 0},
                            {width - 50, 0},
                            {width - 50 + indent, height - 1},
                            {0, height - 1}};

        BaseGfxEngine::GetInstance()->QuadToQuad(gfxDstBuffer, invalidatedArea, srcBuf, srcRect,
                                                 Color::White(), OPA_OPAQUE, srcQuad, dstQuad);
    }
};

void UITestPerspectiveTransform::SetUp()
{
    if (container_ == nullptr) {
        container_ = new UIScrollView();
        container_->Resize(Screen::GetInstance().GetWidth(), Screen::GetInstance().GetHeight());
    }
}

void UITestPerspectiveTransform::TearDown()
{
    DeleteChildren(container_);
    container_ = nullptr;
}

const UIView* UITestPerspectiveTransform::GetTestView()
{
    UIKitPerspectiveTransformTest001();
    return container_;
}

void UITestPerspectiveTransform::UIKitPerspectiveTransformTest001()
{
    if (container_ != nullptr) {
        g_earthState = EarthState(); // Reset state
        // UILabel* label = new UILabel();
        // container_->Add(label);
        // label->SetPosition(10, 10, 400, 50);
        // label->SetText("Perspective Transform Test (Red Box Rotated)");
        // label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, 20);
#if 0
        UIPerspectiveView* view = new UIPerspectiveView();
        view->SetPosition(100, 100, 400, 400);
        container_->Add(view);
#endif

        // UILabel* label2 = new UILabel();
        // container_->Add(label2);
        // label2->SetPosition(10, 200, 500, 50);
        // label2->SetText("Textured Earth (Quad ToQuad)");
        // label2->SetFont(DEFAULT_VECTOR_FONT_FILENAME, 20);

        UIEarth* earth = new UIEarth();
        earth->SetSegments(40, 20);
        earth->SetTexture(
            // "C:\\Users\\cheng\\work\\OpenHarmony-v6.0\\foundation\\arkui\\ui_lite\\test\\uitest\\test_perspective_transform\\8k_earth_daymap.jpg"
            "C:\\Users\\cheng\\work\\OpenHarmony-v6.0\\foundation\\arkui\\ui_lite\\test\\uitest\\test_perspective_transform\\Earth_Map.jpg"
        );
        // earth->SetOverTexture(
        //     "C:\\Users\\cheng\\work\\OpenHarmony-v6.0\\foundation\\arkui\\ui_lite\\test\\uitest\\test_perspective_transform\\css_globe_halo.png"
        // );
        earth->SetPosition(0, 0, 466, 466);
        // earth->SetCenter(65, 33);
        container_->Add(earth);

        // --- Control Sliders ---
        int16_t startX = 480;
        int16_t startY = 10;
        int16_t gapY = 70;
        int16_t sliderW = 250;
        int16_t sliderH = 20;

        auto createSlider = [&](const char* title, EarthParamType type, int32_t min, int32_t max, int32_t initial) {
            UILabel* label = new UILabel();
            label->SetPosition(startX, startY, sliderW, 30);
            label->SetText(title);
            label->SetFont(DEFAULT_VECTOR_FONT_FILENAME, 20);
            container_->Add(label);

            UISlider* slider = new UISlider();
            slider->SetPosition(startX, startY + 35, sliderW, sliderH);
            slider->SetRange(max, min);
            slider->SetValue(initial);
            slider->SetValidWidth(sliderW);
            slider->SetValidHeight(sliderH);
            slider->SetKnobWidth(30);
            slider->SetBackgroundStyle(STYLE_BACKGROUND_COLOR, Color::Gray().full);
            slider->SetForegroundStyle(STYLE_BACKGROUND_COLOR, Color::Blue().full);
            slider->SetKnobStyle(STYLE_BACKGROUND_COLOR, Color::White().full);
            slider->SetKnobStyle(STYLE_BORDER_RADIUS, 15);
            slider->SetBackgroundStyle(STYLE_BORDER_RADIUS, 10);
            slider->SetForegroundStyle(STYLE_BORDER_RADIUS, 10);

            EarthConfigListener* listener = new EarthConfigListener(earth, type, label);
            slider->SetSliderEventListener(listener);
            container_->Add(slider);

            startY += gapY;
        };

        createSlider("RotX: 65", PARAM_ROT_X, 0, 360, 245); // 65 + 180
        createSlider("RotY: 33", PARAM_ROT_Y, 0, 180, 123); // 33 + 90
        createSlider("Scale: 1.0", PARAM_SCALE, 1, 50, 10); // 1.0 * 10
        createSlider("TransX: 0", PARAM_TRANS_X, 0, 1000, 500); // 0 + 500
        createSlider("TransY: 0", PARAM_TRANS_Y, 0, 1000, 500); // 0 + 500
        createSlider("TransZ: 0", PARAM_TRANS_Z, 0, 1000, 500); // 0 + 500

        UIView* cropBox = new UIView();
        cropBox->SetPosition(0, -99, 466, 466); // Center 200x200 box in 466x466 earth view
        cropBox->SetStyle(STYLE_BORDER_COLOR, Color::Red().full);
        cropBox->SetStyle(STYLE_BORDER_WIDTH, 3);
        cropBox->SetStyle(STYLE_BACKGROUND_OPA, OPA_TRANSPARENT);
        container_->Add(cropBox);
    }
}

} // namespace OHOS
