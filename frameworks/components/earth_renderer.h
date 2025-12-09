#ifndef GRAPHIC_LITE_EARTH_RENDERER_H
#define GRAPHIC_LITE_EARTH_RENDERER_H

#include "gfx_utils/graphic_types.h"
#include "gfx_utils/rect.h"
#include "gfx_utils/geometry2d.h"
#include "components/ui_earth.h"

namespace OHOS {
typedef struct {
    int16_t x, y;
    uint16_t w, h;
} IRect;

struct EarthVertex { Vec3 position; Vec3 transPos; float u, v; };
struct EarthFace { uint32_t v1, v2, v3, v4; };
struct EarthMesh {
    EarthVertex* vertices;
    EarthFace* faces;
    uint32_t vertex_count;
    uint32_t face_count;
};

struct EarthTile {
    PointF srcQuad[4];
    PointF dstQuad[4];
    IRect srcRect;
};

class EarthRenderer {
public:
    EarthRenderer();
    ~EarthRenderer();

    void SetTexture(uint8_t* data, uint16_t width, uint16_t height, uint16_t stride, ColorMode mode);
    void SetSegments(uint16_t cols, uint16_t rows);
    bool BuildMesh();
    void DrawEarth(BufferInfo& gfxDstBuffer, const Rect& invalidatedArea, EarthTransformParams& params);
    void FreeMesh();

private:
    uint16_t cols_;
    uint16_t rows_;
    uint16_t imgWidth_;
    uint16_t imgHeight_;

    BufferInfo srcBuf_;

    EarthMesh mesh_;
    bool meshBuilt_;
    void* tilesBuf_;
};

} // namespace OHOS

#endif // GRAPHIC_LITE_EARTH_RENDERER_H
