#include "components/ui_earth.h"
#include "components/earth_renderer.h"
#include "engines/gfx/gfx_engine_manager.h"
#include "draw/draw_utils.h"
#include "gfx_utils/mem_api.h"
#include "gfx_utils/graphic_math.h"
#include "securec.h"
#include "jpeglib.h"
#include "animator/easing_equation.h"
#include <cstdio>
#include <cmath>
#include <csetjmp>

namespace OHOS {
namespace {
struct EarthJpegErrorMgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

METHODDEF(void) EarthJpegErrorExit(j_common_ptr cinfo)
{
    EarthJpegErrorMgr* myerr = (EarthJpegErrorMgr*)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

class EarthJpegLoader {
public:
    static uint8_t* Load(const char* path, uint16_t& width, uint16_t& height, uint16_t& stride) {
        struct jpeg_decompress_struct cinfo;
        EarthJpegErrorMgr jerr;
        FILE* infile = nullptr;
        uint8_t* data = nullptr;

        if ((infile = fopen(path, "rb")) == nullptr) {
            GRAPHIC_LOGE("EarthJpegLoader: can't open %s", path);
            return nullptr;
        }

        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = EarthJpegErrorExit;

        if (setjmp(jerr.setjmp_buffer)) {
            GRAPHIC_LOGE("EarthJpegLoader: JPEG error occurred");
            jpeg_destroy_decompress(&cinfo);
            if (infile) fclose(infile);
            if (data) UIFree(data);
            return nullptr;
        }

        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, infile);
        jpeg_read_header(&cinfo, TRUE);

        // Scale down to fit max texture size (2048x2048)
        if (cinfo.image_width > 2048 || cinfo.image_height > 2048) {
            cinfo.scale_num = 1;
            cinfo.scale_denom = 1;
            while ((cinfo.image_width / cinfo.scale_denom > 2048) ||
                   (cinfo.image_height / cinfo.scale_denom > 2048)) {
                cinfo.scale_denom *= 2;
            }
            GRAPHIC_LOGD("EarthJpegLoader: scaling by 1/%d", cinfo.scale_denom);
        }

        jpeg_start_decompress(&cinfo);

        width = cinfo.output_width;
        height = cinfo.output_height;
        int components = cinfo.output_components;
        uint32_t dataSize = width * height * 4; // ARGB8888

        data = static_cast<uint8_t*>(UIMalloc(dataSize));
        if (data == nullptr) {
            GRAPHIC_LOGE("EarthJpegLoader: Out of memory");
            jpeg_finish_decompress(&cinfo);
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);
            return nullptr;
        }

        int rowStride = width * components;
        JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, rowStride, 1);

        uint32_t offset = 0;
        while (cinfo.output_scanline < cinfo.output_height) {
            jpeg_read_scanlines(&cinfo, buffer, 1);
            uint8_t* p = buffer[0];
            if (components == 3) {
                for (int i = 0; i < width; i++) {
                    data[offset++] = p[i * 3 + 2]; // B
                    data[offset++] = p[i * 3 + 1]; // G
                    data[offset++] = p[i * 3];     // R
                    data[offset++] = 255;          // A
                }
            } else if (components == 1) {
                for (int i = 0; i < width; i++) {
                    uint8_t val = p[i];
                    data[offset++] = val;
                    data[offset++] = val;
                    data[offset++] = val;
                    data[offset++] = 255;
                }
            } else {
                 // Fallback for other formats
                 for (int i = 0; i < width; i++) {
                    data[offset++] = 0;
                    data[offset++] = 0;
                    data[offset++] = 0;
                    data[offset++] = 255;
                }
            }
        }

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);

        stride = width * 4;
        return data;
    }
};
}

UIEarth::UIEarth()
    : imageData_(nullptr),
      inertiaAnimator_(nullptr), dragVelocityX_(0.0f), dragVelocityY_(0.0f),
      transitionAnimator_(nullptr), state_(STATE_EDGE), nextEdgePos_(EDGE_BOTTOM), renderer_(nullptr)
{
    GRAPHIC_LOGD("UIEarth::UIEarth");
    renderer_ = new EarthRenderer();
    SetDraggable(false);
    SetTouchable(true);
    inertiaAnimator_ = new Animator(this, this, 0, true);
    transitionAnimator_ = new Animator(this, this, 1500, false); // 1500ms duration
    animStartParams_ = {0};
    animStartParams_.scale = 1.0f;
    currentParams_ = {0};
    currentParams_.scale = 1.0f;
    SetupEdgeAnimation();
    if (transitionAnimator_) {
        transitionAnimator_->Start();
    }
}

UIEarth::~UIEarth()
{
    GRAPHIC_LOGD("UIEarth::~UIEarth");
    if (renderer_) {
        delete renderer_;
        renderer_ = nullptr;
    }
    if (imageData_) {
        UIFree(imageData_);
        imageData_ = nullptr;
    }
    if (inertiaAnimator_) {
        delete inertiaAnimator_;
        inertiaAnimator_ = nullptr;
    }
    if (transitionAnimator_) {
        delete transitionAnimator_;
        transitionAnimator_ = nullptr;
    }
}

void UIEarth::SetTexture(const char* path)
{
    GRAPHIC_LOGD("UIEarth::SetTexture %s", path);
    if (imageData_) {
        UIFree(imageData_);
        imageData_ = nullptr;
    }

    uint16_t w = 0, h = 0, stride = 0;
    imageData_ = EarthJpegLoader::Load(path, w, h, stride);

    if (renderer_ && imageData_) {
        renderer_->SetTexture(imageData_, w, h, stride, ARGB8888);
    }
}

void UIEarth::SetSegments(uint16_t cols, uint16_t rows)
{
    GRAPHIC_LOGD("UIEarth::SetSegments %d %d", cols, rows);
    if (renderer_) {
        renderer_->SetSegments(cols, rows);
    }
}

void UIEarth::OnDraw(BufferInfo& gfxDstBuffer, const Rect& invalidatedArea)
{
    UIView::OnDraw(gfxDstBuffer, invalidatedArea);

    if (!renderer_) return;

    currentParams_.viewX = GetX();
    currentParams_.viewY = GetY();
    currentParams_.viewW = GetWidth();
    currentParams_.viewH = GetHeight();

    renderer_->DrawEarth(gfxDstBuffer, invalidatedArea, currentParams_);
}

void UIEarth::SetCenter(float longitude, float latitude)
{
    currentParams_.rotationX = longitude;
    currentParams_.rotationY = latitude;
    Invalidate();
}

void UIEarth::SetScale(float scale)
{
    currentParams_.scale = scale;
    Invalidate();
}

void UIEarth::SetTranslation(float x, float y, float z)
{
    currentParams_.translateX = x;
    currentParams_.translateY = y;
    currentParams_.translateZ = z;
    Invalidate();
}

bool UIEarth::OnDragEvent(const DragEvent& event)
{
    if (inertiaAnimator_ && inertiaAnimator_->GetState() != Animator::STOP) {
        inertiaAnimator_->Stop();
    }
    dragVelocityX_ = event.GetDeltaX();
    dragVelocityY_ = event.GetDeltaY();
    float& rotationX = currentParams_.rotationX;
    float& rotationY = currentParams_.rotationY;
    const float dragFactor = 0.5f;
    const float maxRotationY = 89.0f;
    const float rotationCycle = 360.0f;

    rotationX += dragVelocityX_ * dragFactor;
    rotationY += dragVelocityY_ * dragFactor;

    if (rotationY > maxRotationY) {
        rotationY = maxRotationY;
    }
    if (rotationY < -maxRotationY) {
        rotationY = -maxRotationY;
    }
    if (rotationX > rotationCycle) {
        rotationX -= rotationCycle;
    }
    if (rotationX < -rotationCycle) {
        rotationX += rotationCycle;
    }
    GRAPHIC_LOGD("UIEarth::OnDragEvent %f %f %f %f", rotationX, dragVelocityX_, rotationY, dragVelocityY_);
    Invalidate();
    return true;
}

bool UIEarth::OnDragEndEvent(const DragEvent& event)
{
    if (inertiaAnimator_) {
        inertiaAnimator_->Start();
    }
    return true;
}

bool UIEarth::OnClickEvent(const ClickEvent& event)
{
    GRAPHIC_LOGD("UIEarth::%s", __func__);
    if (transitionAnimator_ == nullptr) {
        GRAPHIC_LOGD("UIEarth::%s transitionAnimator_ is null", __func__);
        return false;
    }
    if (transitionAnimator_->GetState() != Animator::STOP) {
        return true; // Ignore clicks during animation
    }
    GRAPHIC_LOGD("UIEarth::%s state_=%d", __func__, state_);

    // Current state params
    animStartParams_ = currentParams_;

    if (state_ == STATE_EDGE) {
        // Animate to Center (Normal)
        SetupCenterAnimation();
        state_ = STATE_NORMAL;
    } else {
        if (inertiaAnimator_ != nullptr && inertiaAnimator_->GetState() == Animator::START) {
            inertiaAnimator_->Stop();
        }
        // Animate to Edge
        SetupEdgeAnimation();
        state_ = STATE_EDGE;
        SetDraggable(false); // Disable drag immediately
    }
    transitionAnimator_->Start();
    return true;
}

void UIEarth::SetupCenterAnimation()
{
    animEndParams_.scale = 1.0f;
    animEndParams_.translateX = 0.0f;
    animEndParams_.translateY = 0.0f;
    animEndParams_.translateZ = 0.0f;
    // shanghai
    animEndParams_.rotationX = 65;
    animEndParams_.rotationY = 33;
}

void UIEarth::SetupEdgeAnimation()
{
    switch (nextEdgePos_) {
        case EDGE_BOTTOM:
            animEndParams_.rotationX = 73.0f;
            animEndParams_.rotationY = -11.0f;
            animEndParams_.scale = 2.2f;
            animEndParams_.translateX = 0.0f;
            animEndParams_.translateY = 396.0f;
            animEndParams_.translateZ = 0.0f;
            nextEdgePos_ = EDGE_LEFT;
            break;
        case EDGE_LEFT:
            animEndParams_.rotationX = 116.0f;
            animEndParams_.rotationY = 42.0f;
            animEndParams_.scale = 2.5f;
            animEndParams_.translateX = 468.0f;
            animEndParams_.translateY = -104.0f;
            animEndParams_.translateZ = 132.0f;
            nextEdgePos_ = EDGE_TOP;
            break;
        case EDGE_TOP:
            animEndParams_.rotationX = 70.0f;
            animEndParams_.rotationY = 72.0f;
            animEndParams_.scale = 2.0f;
            animEndParams_.translateX = 108.0f;
            animEndParams_.translateY = -256.0f;
            animEndParams_.translateZ = 288.0f;
        //     nextEdgePos_ = EDGE_RIGHT;
        //     break;
        // case EDGE_RIGHT:
        //     // Mirrored from EDGE_LEFT for symmetry
        //     animEndParams_.rotationX = -116.0f;
        //     animEndParams_.rotationY = 42.0f;
        //     animEndParams_.scale = 2.5f;
        //     animEndParams_.translateX = -468.0f;
        //     animEndParams_.translateY = -104.0f;
        //     animEndParams_.translateZ = 132.0f;
            nextEdgePos_ = EDGE_BOTTOM;
            break;
        default:
            nextEdgePos_ = EDGE_BOTTOM;
            break;
    }
}

void UIEarth::Callback(UIView* view)
{
    if (view != this) {
        return;
    }

    if (transitionAnimator_ != nullptr && transitionAnimator_->GetState() == Animator::START) {
        uint32_t runTime = transitionAnimator_->GetRunTime();
        uint32_t duration = transitionAnimator_->GetTime();
        if (runTime > duration) {
            runTime = duration;
        }

        constexpr int16_t RANGE = 10000;
        constexpr float INV_RANGE = 1.0f / RANGE;

        int16_t val = EasingEquation::CubicEaseInOut(0, RANGE, static_cast<uint16_t>(runTime), static_cast<uint16_t>(duration));
        float smoothT = val * INV_RANGE;

        auto UpdateParam = [&](float& current, float start, float end) {
            current = start + (end - start) * smoothT;
        };

        UpdateParam(currentParams_.scale, animStartParams_.scale, animEndParams_.scale);
        UpdateParam(currentParams_.translateX, animStartParams_.translateX, animEndParams_.translateX);
        UpdateParam(currentParams_.translateY, animStartParams_.translateY, animEndParams_.translateY);
        UpdateParam(currentParams_.translateZ, animStartParams_.translateZ, animEndParams_.translateZ);
        UpdateParam(currentParams_.rotationX, animStartParams_.rotationX, animEndParams_.rotationX);
        UpdateParam(currentParams_.rotationY, animStartParams_.rotationY, animEndParams_.rotationY);

        Invalidate();

        if (runTime >= duration) {
             transitionAnimator_->Stop();
        }
        return;
    }

    // Inertia Logic (Only runs if transition is not running)
    if (inertiaAnimator_ != nullptr && inertiaAnimator_->GetState() == Animator::START) {
        const float friction = 0.92f;
        const float stopThreshold = 0.1f;
        const float dragFactor = 0.5f;

        // Apply friction
        dragVelocityX_ *= friction;
        dragVelocityY_ *= friction;

        if (std::abs(dragVelocityX_) < stopThreshold && std::abs(dragVelocityY_) < stopThreshold) {
            inertiaAnimator_->Stop();
            return;
        }

        currentParams_.rotationX += dragVelocityX_ * dragFactor;
        currentParams_.rotationY += dragVelocityY_ * dragFactor;
        Invalidate();
    }
}

void UIEarth::OnStop(UIView& view)
{
    if (state_ == STATE_NORMAL) {
        SetDraggable(true);
    }
}

} // namespace OHOS
