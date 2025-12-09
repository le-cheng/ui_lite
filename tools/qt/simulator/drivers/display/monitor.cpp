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

#include "monitor.h"

#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QtGui/QTransform>
#include <QtCore/QDebug>
#include <cstdio>

#include "common/graphic_startup.h"
#include "common/image_decode_ability.h"
#include "common/input_device_manager.h"
#include "draw/draw_utils.h"
#include "font/ui_font.h"
#include "font/ui_font_header.h"
#if defined(ENABLE_VECTOR_FONT) && ENABLE_VECTOR_FONT
#include "font/ui_font_vector.h"
#else
#include "common/ui_text_language.h"
#include "font/ui_font_bitmap.h"
#endif
#include "key_input.h"
#include "mouse_input.h"
#include "mousewheel_input.h"
#include "windows.h"

namespace OHOS {
bool Monitor::isRegister_ = false;

Monitor* Monitor::GetInstance()
{
    static Monitor instance;
    if (!isRegister_) {
        BaseGfxEngine::InitGfxEngine(&instance);
        isRegister_ = true;
    }
    return &instance;
}

void Monitor::InitHal()
{
#if defined(USE_MOUSE) && USE_MOUSE
    MouseInput* mouse = MouseInput::GetInstance();
    InputDeviceManager::GetInstance()->Add(mouse);
#endif

#if defined(USE_MOUSEWHEEL) && defined(ENABLE_ROTATE_INPUT) && USE_MOUSEWHEEL && ENABLE_ROTATE_INPUT
    MousewheelInput* mousewheel = MousewheelInput::GetInstance();
    InputDeviceManager::GetInstance()->Add(mousewheel);
#endif

#if defined(USE_KEY) && USE_KEY
    KeyInput* key = KeyInput::GetInstance();
    InputDeviceManager::GetInstance()->Add(key);
#endif
}

BufferInfo* Monitor::GetFBBufferInfo()
{
    static BufferInfo* bufferInfo = nullptr;
    if (bufferInfo == nullptr) {
        bufferInfo = new BufferInfo;
        bufferInfo->rect = {0, 0, HORIZONTAL_RESOLUTION - 1, VERTICAL_RESOLUTION - 1};
        bufferInfo->mode = ARGB8888;
        bufferInfo->color = 0x44;
        bufferInfo->phyAddr = bufferInfo->virAddr = tftFb_;
        // 3: Shift right 3 bits
        bufferInfo->stride = HORIZONTAL_RESOLUTION * (DrawUtils::GetPxSizeByColorMode(bufferInfo->mode) >> 3);
        bufferInfo->width = HORIZONTAL_RESOLUTION;
        bufferInfo->height = VERTICAL_RESOLUTION;
    }
    return bufferInfo;
}

void Monitor::Flush(const Rect &rect)
{
    UpdatePaint(tftFb_, HORIZONTAL_RESOLUTION, VERTICAL_RESOLUTION);
}

// assuming below are the memory pool
static uint8_t g_fontMemBaseAddr[OHOS::MIN_FONT_PSRAM_LENGTH];
#if defined(ENABLE_ICU) && ENABLE_ICU
static uint8_t g_icuMemBaseAddr[OHOS::SHAPING_WORD_DICT_LENGTH];
#endif

void Monitor::InitFontEngine()
{
#if defined(ENABLE_VECTOR_FONT) && ENABLE_VECTOR_FONT
    GraphicStartUp::InitFontEngine(reinterpret_cast<uintptr_t>(g_fontMemBaseAddr), MIN_FONT_PSRAM_LENGTH,
                                   VECTOR_FONT_DIR, DEFAULT_VECTOR_FONT_FILENAME);
#else
    BitmapFontInit();
    std::string dPath(_pgmptr);
    size_t len = dPath.size();
    size_t pos = dPath.find_last_of('\\');
    dPath.replace((pos + 1), (len - pos), "..\\..\\simulator\\font\\font.bin");
    GraphicStartUp::InitFontEngine(reinterpret_cast<uintptr_t>(g_fontMemBaseAddr), MIN_FONT_PSRAM_LENGTH,
                                   dPath.c_str(), nullptr);
#endif

#if defined(ENABLE_ICU) && ENABLE_ICU
    GraphicStartUp::InitLineBreakEngine(reinterpret_cast<uintptr_t>(g_icuMemBaseAddr), SHAPING_WORD_DICT_LENGTH,
                                        VECTOR_FONT_DIR, DEFAULT_LINE_BREAK_RULE_FILENAME);
#endif
}

void Monitor::InitImageDecodeAbility()
{
    uint32_t imageType = IMG_SUPPORT_BITMAP | OHOS::IMG_SUPPORT_JPEG | OHOS::IMG_SUPPORT_PNG;
    ImageDecodeAbility::GetInstance().SetImageDecodeAbility(imageType);
}

void Monitor::GUILoopStart() const
{
    Sleep(GUI_REFR_PERIOD);
}

void Monitor::InitGUI()
{
    for (uint32_t i = 0; i < HORIZONTAL_RESOLUTION * VERTICAL_RESOLUTION; i++) {
        tftFb_[i] = defaultColor_;
    }
    UpdatePaint(tftFb_, HORIZONTAL_RESOLUTION, VERTICAL_RESOLUTION);
}

void Monitor::DrawPerspectiveTransform(BufferInfo& dst,
                                       const Rect& mask,
                                       const Point& position,
                                       ColorType color,
                                       OpacityType opacity,
                                       const Matrix3<float>& matrix,
                                       const TransformDataInfo& dataInfo)
{
    if ((dst.virAddr == nullptr) || (dataInfo.data == nullptr)) {
        return;
    }

    QImage::Format dstFormat = QImage::Format_ARGB32_Premultiplied;
    if (dst.mode == RGB565) {
        dstFormat = QImage::Format_RGB16;
    } else if (dst.mode == RGB888) {
        dstFormat = QImage::Format_RGB888;
    }

    QImage dstImg(reinterpret_cast<uchar*>(dst.virAddr), dst.width, dst.height, dst.stride, dstFormat);

    QImage::Format srcFormat = QImage::Format_ARGB32_Premultiplied;
    if (dataInfo.header.colorMode == RGB565) {
        srcFormat = QImage::Format_RGB16;
    } else if (dataInfo.header.colorMode == RGB888) {
        srcFormat = QImage::Format_RGB888;
    }

    QImage srcImg(const_cast<uchar*>(dataInfo.data), dataInfo.header.width, dataInfo.header.height, srcFormat);

    Matrix3<float> m = matrix;
    QTransform transform(m[0][0], m[0][1], m[0][2],
                         m[1][0], m[1][1], m[1][2],
                         m[2][0], m[2][1], m[2][2]);

    QTransform transPos;
    transPos.translate(position.x, position.y);
    transform = transPos * transform;

    QPainter painter(&dstImg);
    painter.setClipRect(mask.GetLeft(), mask.GetTop(), mask.GetWidth(), mask.GetHeight());

    if (opacity != OPA_OPAQUE) {
        painter.setOpacity(opacity / 255.0);
    }

    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setTransform(transform);
    painter.drawImage(0, 0, srcImg);
}

void Monitor::GUILoopQuit() const {}

void Monitor::QuadToQuad(BufferInfo& dst,
                         const Rect& mask,
                         BufferInfo& src,
                         const Rect& srcRect,
                         ColorType color,
                         OpacityType opacity,
                         const PointF srcQuad[4],
                         const PointF dstQuad[4])
{
    if ((dst.virAddr == nullptr) || (src.virAddr == nullptr)) {
        return;
    }

    QImage::Format dstFormat = QImage::Format_ARGB32_Premultiplied;
    if (dst.mode == RGB565) {
        dstFormat = QImage::Format_RGB16;
    } else if (dst.mode == RGB888) {
        dstFormat = QImage::Format_RGB888;
    }

    QImage dstImg(reinterpret_cast<uchar*>(dst.virAddr), dst.width, dst.height, dst.stride, dstFormat);

    QImage::Format srcFormat = QImage::Format_ARGB32_Premultiplied;
    if (src.mode == RGB565) {
        srcFormat = QImage::Format_RGB16;
    } else if (src.mode == RGB888) {
        srcFormat = QImage::Format_RGB888;
    }

    QImage srcFull(reinterpret_cast<uchar*>(src.virAddr), src.width, src.height, src.stride, srcFormat);

    QPolygonF srcQ;
    float sx = srcRect.GetLeft();
    float sy = srcRect.GetTop();
    srcQ << QPointF(srcQuad[0].x - sx, srcQuad[0].y - sy)
         << QPointF(srcQuad[1].x - sx, srcQuad[1].y - sy)
         << QPointF(srcQuad[2].x - sx, srcQuad[2].y - sy)
         << QPointF(srcQuad[3].x - sx, srcQuad[3].y - sy);

    QPolygonF dstQ;
    dstQ << QPointF(dstQuad[0].x, dstQuad[0].y)
         << QPointF(dstQuad[1].x, dstQuad[1].y)
         << QPointF(dstQuad[2].x, dstQuad[2].y)
         << QPointF(dstQuad[3].x, dstQuad[3].y);

    QTransform transform;
    if (!QTransform::quadToQuad(srcQ, dstQ, transform)) {
        return;
    }

    QPainter painter(&dstImg);
    // painter.setClipRect(mask.GetLeft(), mask.GetTop(), mask.GetWidth(), mask.GetHeight());
    if (opacity != OPA_OPAQUE) {
        painter.setOpacity(opacity / 255.0);
    }
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setTransform(transform);
    QRect srcCrop(srcRect.GetLeft(), srcRect.GetTop(), srcRect.GetWidth(), srcRect.GetHeight());
    QRectF dstRect(0, 0, srcCrop.width(), srcCrop.height());
    painter.drawImage(dstRect, srcFull, srcCrop);
}

} // namespace OHOS
