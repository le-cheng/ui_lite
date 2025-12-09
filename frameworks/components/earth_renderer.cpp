#include "earth_renderer.h"
#include "engines/gfx/gfx_engine_manager.h"
#include "draw/draw_utils.h"
#include "gfx_utils/mem_api.h"
#include "gfx_utils/graphic_math.h"
#include "securec.h"
#include <cstdio>
#include <cmath>

namespace OHOS {

static uint32_t generate_sphere_mesh(float radius, uint32_t latSegments, uint32_t lonSegments, EarthMesh* mesh_out)
{
    if ((mesh_out == nullptr) || (latSegments == 0) || (lonSegments == 0)) {
        GRAPHIC_LOGE("%s: invalid parameter", __func__);
        return 1;
    }
    if (latSegments > 100) latSegments = 100;
    if (lonSegments > 200) lonSegments = 200;
    float twoPI = 2.0f * M_PI;
    float invLat = 1.0f / static_cast<float>(latSegments);
    float invLon = 1.0f / static_cast<float>(lonSegments);
    uint32_t stride = lonSegments + 1;
    uint32_t nverts = (latSegments + 1) * stride;
    uint32_t maxFaces = latSegments * lonSegments;
    EarthVertex* vertices = static_cast<EarthVertex*>(UIMalloc(sizeof(EarthVertex) * nverts));
    EarthFace* faces = static_cast<EarthFace*>(UIMalloc(sizeof(EarthFace) * maxFaces));

    if ((vertices == nullptr) || (faces == nullptr)) {
        if (vertices) UIFree(vertices);
        if (faces) UIFree(faces);
        return 2;
    }
    uint32_t vi = 0;
    for (uint32_t i = 0; i <= latSegments; i++) {
#if 0
        float coordY = static_cast<float>(i) * invLat;
#else // 动态细分
        float ratio = static_cast<float>(i) * invLat;
        // Use sine curve to distribute latitude segments: more dense at poles, less at equator
        float coordY = 0.5f - 0.5f * cosf(ratio * M_PI);
#endif
        if (coordY < 0.008f) coordY = 0.008f;
        else if (coordY >= 0.992f) coordY = 0.992f;
        float phi = M_PI * coordY;
        float cosPhi = cosf(phi);
        float sinPhi = sinf(phi);
        for (uint32_t j = 0; j <= lonSegments; j++) {
            float coordX = static_cast<float>(j) * invLon;
            float theta = twoPI * coordX;
            float cosTheta = cosf(theta);
            float sinTheta = sinf(theta);
            float x = -sinPhi * cosTheta * radius;
            float y = -cosPhi * radius;
            float z = sinPhi * sinTheta * radius;
            vertices[vi].position.x = x;
            vertices[vi].position.y = y;
            vertices[vi].position.z = z;
            vertices[vi].u = coordX;
            vertices[vi].v = coordY;
            vi++;
        }
    }
    uint32_t fi = 0;
    for (uint32_t i = 0; i < latSegments; i++) {
        for (uint32_t j = 0; j < lonSegments; j++) {
            uint32_t v1 = i * stride + j;
            uint32_t v2 = v1 + stride;
            faces[fi].v1 = v1;
            faces[fi].v2 = v2;
            faces[fi].v3 = v2 + 1;
            faces[fi].v4 = v1 + 1;
            fi++;
        }
    }
    mesh_out->vertices = vertices;
    mesh_out->faces = faces;
    mesh_out->vertex_count = nverts;
    mesh_out->face_count = fi;
    return 0;
}

static void CalculateUVBoundingBox(const EarthTile& tile, uint16_t imgW, uint16_t imgH, int16_t& minX, int16_t& maxX, int16_t& minY, int16_t& maxY)
{
    float fMinX = tile.srcQuad[0].x;
    float fMaxX = tile.srcQuad[0].x;
    float fMinY = tile.srcQuad[0].y;
    float fMaxY = tile.srcQuad[0].y;

    for (int i = 1; i < 4; i++) {
        if (tile.srcQuad[i].x < fMinX) fMinX = tile.srcQuad[i].x;
        if (tile.srcQuad[i].x > fMaxX) fMaxX = tile.srcQuad[i].x;
        if (tile.srcQuad[i].y < fMinY) fMinY = tile.srcQuad[i].y;
        if (tile.srcQuad[i].y > fMaxY) fMaxY = tile.srcQuad[i].y;
    }

    // Use rounding (round half up) as requested
    minX = static_cast<int16_t>(fMinX + 0.5f);
    maxX = static_cast<int16_t>(fMaxX + 0.5f);
    minY = static_cast<int16_t>(fMinY + 0.5f);
    maxY = static_cast<int16_t>(fMaxY + 0.5f);

    // Clip to image boundaries
    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX >= imgW) maxX = imgW - 1;
    if (maxY >= imgH) maxY = imgH - 1;
}

static void BuildTilesFromMesh(EarthMesh& mesh,
                                EarthTile* tiles,
                                uint16_t imgW,
                                uint16_t imgH,
                                int16_t viewW,
                                int16_t viewH,
                                uint32_t& outCount)
{
    outCount = 0;
    if ((mesh.vertex_count == 0) || (mesh.face_count == 0) || (tiles == nullptr) || (mesh.vertices == nullptr)) {
        return;
    }

    float w = static_cast<float>(viewW);
    float h = static_cast<float>(viewH);
    auto toScreen = [&](const Vec3& p, PointF& out) {
        out.x = p.x + 0.5f * w;
        out.y = p.y + 0.5f * h;
    };

    uint32_t count = 0;
    for (uint32_t t = 0; t < mesh.face_count; t++) {
        EarthFace f = mesh.faces[t];

        Vec3 p1 = mesh.vertices[f.v1].transPos;
        Vec3 p2 = mesh.vertices[f.v2].transPos;
        Vec3 p3 = mesh.vertices[f.v3].transPos;

        float x1 = p2.x - p1.x;
        float y1 = p2.y - p1.y;
        float x2 = p3.x - p1.x;
        float y2 = p3.y - p1.y;
        float z = x1 * y2 - x2 * y1;
        if (z > 0) continue; // Back-face culling

        EarthTile tile;
        EarthVertex v1 = mesh.vertices[f.v1];
        EarthVertex v2 = mesh.vertices[f.v2];
        EarthVertex v3 = mesh.vertices[f.v3];
        EarthVertex v4 = mesh.vertices[f.v4];

        tile.srcQuad[0].x = v1.u * imgW;
        tile.srcQuad[0].y = v1.v * imgH;
        tile.srcQuad[1].x = v2.u * imgW;
        tile.srcQuad[1].y = v2.v * imgH;
        tile.srcQuad[2].x = v3.u * imgW;
        tile.srcQuad[2].y = v3.v * imgH;
        tile.srcQuad[3].x = v4.u * imgW;
        tile.srcQuad[3].y = v4.v * imgH;

        int16_t minX, maxX, minY, maxY;
        CalculateUVBoundingBox(tile, imgW, imgH, minX, maxX, minY, maxY);
        if (minX == maxX || minY == maxY) {
            // GRAPHIC_LOGE("Invalid UV bounding box: minX=%d, maxX=%d, minY=%d, maxY=%d", minX, maxX, minY, maxY);
            continue;
        }

        IRect r = {minX, minY, static_cast<uint16_t>(maxX - minX + 1), static_cast<uint16_t>(maxY - minY + 1)};
        tile.srcRect = r;

        // tile.srcQuad[0].x = 0;
        // tile.srcQuad[0].y = 0;
        // tile.srcQuad[1].x = 0;
        // tile.srcQuad[1].y = r.h;
        // tile.srcQuad[2].x = r.w;
        // tile.srcQuad[2].y = r.h;
        // tile.srcQuad[3].x = r.w;
        // tile.srcQuad[3].y = 0;

        toScreen(p1, tile.dstQuad[0]);
        toScreen(p2, tile.dstQuad[1]);
        toScreen(p3, tile.dstQuad[2]);
        toScreen(mesh.vertices[f.v4].transPos, tile.dstQuad[3]);

        tiles[count++] = tile;
    }

    outCount = count;
}

static Mat4 CalculateTransformMatrix(EarthTransformParams& params)
{
    // View Matrix
    if (params.rotationY > 89.0f) params.rotationY = 89.0f;
    if (params.rotationY < -89.0f) params.rotationY = -89.0f;
    // if (params.rotationX > 360) params.rotationX = 360;
    // if (params.rotationX < -360) params.rotationX = -360;

    float lon = toRad(params.rotationX);
    float lat = toRad(params.rotationY);

    Vec3 frontDir(
        cosf(lon) * cosf(lat),
        sinf(lat),
        sinf(lon) * cosf(lat)
    );

    float distance = 600.0f;
    Vec3 eye(frontDir.x * -distance, frontDir.y * -distance, frontDir.z * -distance);
    Vec3 center(0.0f, 0.0f, 0.0f);
    Vec3 up(0.0f, 1.0f, 0.0f);

    Mat4 view = Mat4::LookAt(eye, center, up);

    Mat4 modelScale = Mat4::Scale(params.scale, params.scale, params.scale);
    Mat4 modelTrans = Mat4::Translate(params.translateX, params.translateY, params.translateZ);
    Mat4 model = modelTrans * modelScale;

    return view * model;
}

EarthRenderer::EarthRenderer()
    : cols_(20), rows_(20), imgWidth_(0), imgHeight_(0), meshBuilt_(false), tilesBuf_(nullptr)
{
    memset_s(&mesh_, sizeof(EarthMesh), 0, sizeof(EarthMesh));
    memset_s(&srcBuf_, sizeof(BufferInfo), 0, sizeof(BufferInfo));
}

EarthRenderer::~EarthRenderer()
{
    FreeMesh();
}

void EarthRenderer::SetTexture(uint8_t* data, uint16_t width, uint16_t height, uint16_t stride, ColorMode mode)
{
    if (data == nullptr) return;

    srcBuf_.mode = mode;
    srcBuf_.color = 0;
    srcBuf_.phyAddr = data;
    srcBuf_.virAddr = data;
    srcBuf_.stride = stride;
    srcBuf_.width = width;
    srcBuf_.height = height;
    srcBuf_.rect = {0, 0, static_cast<int16_t>(width - 1), static_cast<int16_t>(height - 1)};

    imgWidth_ = width;
    imgHeight_ = height;
}

void EarthRenderer::SetSegments(uint16_t cols, uint16_t rows)
{
    if (cols > 0) cols_ = cols;
    if (rows > 0) rows_ = rows;
    FreeMesh();
    meshBuilt_ = false;
}

void EarthRenderer::FreeMesh()
{
    if (meshBuilt_) {
        if (mesh_.vertices) UIFree(mesh_.vertices);
        if (mesh_.faces) UIFree(mesh_.faces);
        if (tilesBuf_) UIFree(tilesBuf_);

        mesh_.vertices = nullptr;
        mesh_.faces = nullptr;
        tilesBuf_ = nullptr;
        mesh_.vertex_count = 0;
        mesh_.face_count = 0;
        meshBuilt_ = false;
    }
}

bool EarthRenderer::BuildMesh()
{
    if (meshBuilt_) return true;

    uint32_t ret = generate_sphere_mesh(200.0f, rows_, cols_, &mesh_);
    if (ret != 0) return false;

    if (mesh_.face_count > 0) {
        tilesBuf_ = UIMalloc(sizeof(EarthTile) * mesh_.face_count);
        if (tilesBuf_ == nullptr) {
            FreeMesh();
            return false;
        }
    }

    meshBuilt_ = true;
    return true;
}

void EarthRenderer::DrawEarth(BufferInfo& gfxDstBuffer, const Rect& invalidatedArea, EarthTransformParams& params)
{
    if (!BuildMesh()) return;

    Mat4 vp = CalculateTransformMatrix(params);

    if ((mesh_.face_count == 0) || (mesh_.vertices == nullptr) || (tilesBuf_ == nullptr)) {
        return;
    }

    // 1. Transform all vertices once
    for (uint32_t i = 0; i < mesh_.vertex_count; i++) {
        Vec3 p = mesh_.vertices[i].position;
        Vec3 res = vp * p;
        mesh_.vertices[i].transPos.x = res.x;
        mesh_.vertices[i].transPos.y = res.y;
        mesh_.vertices[i].transPos.z = res.z;
    }

    // 2. Build tiles using transformed vertices
    EarthTile* tiles = static_cast<EarthTile*>(tilesBuf_);
    uint32_t tileCount = 0;
    BuildTilesFromMesh(mesh_, tiles, imgWidth_, imgHeight_, params.viewW, params.viewH, tileCount);

    for (uint32_t k = 0; k < tileCount; k++) {
        EarthTile tile = tiles[k];
        Rect r(tile.srcRect.x, tile.srcRect.y, tile.srcRect.x + tile.srcRect.w - 1, tile.srcRect.y + tile.srcRect.h - 1);
        BaseGfxEngine::GetInstance()->QuadToQuad(
            gfxDstBuffer,
            invalidatedArea,
            srcBuf_,
            r,
            Color::White(),
            OPA_OPAQUE,
            tile.srcQuad,
            tile.dstQuad
        );
    }
}

} // namespace OHOS
