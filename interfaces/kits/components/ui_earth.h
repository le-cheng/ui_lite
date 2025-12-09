#ifndef GRAPHIC_LITE_UI_EARTH_H
#define GRAPHIC_LITE_UI_EARTH_H

#include "components/ui_view.h"
#include "gfx_utils/graphic_types.h"
#include "animator/animator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace OHOS {
struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const {
        float inv = 1.0f / s;
        return Vec3(x * inv, y * inv, z * inv);
    }

    float Dot(const Vec3& other) const { return x * other.x + y * other.y + z * other.z; }

    Vec3 Cross(const Vec3& other) const {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    float Length() const { return sqrt(x * x + y * y + z * z); }

    Vec3 Normalize() const {
        float len = Length();
        if (len > 1e-6f) {
            return *this / len;
        }
        return *this;
    }

    static Vec3 Cross(const Vec3& a, const Vec3& b) { return a.Cross(b); }
    static float Dot(const Vec3& a, const Vec3& b) { return a.Dot(b); }
};

struct Vec2 {
    float x, y;

    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}

    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2 operator/(float s) const {
        float inv = 1.0f / s;
        return Vec2(x * inv, y * inv);
    }

    float Dot(const Vec2& other) const { return x * other.x + y * other.y; }

    float Length() const { return sqrt(x * x + y * y); }

    Vec2 Normalize() const {
        float len = Length();
        if (len > 1e-6f) {
            return *this / len;
        }
        return *this;
    }

    static float Dot(const Vec2& a, const Vec2& b) { return a.Dot(b); }
};

static inline float toRad(float angleDeg) {
    return angleDeg * M_PI / 180.0f;
}

struct Mat4 {
    float m[4][4];

    Mat4() { Identity(); }

    void Identity() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                m[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }

    static Mat4 Translate(float x, float y, float z) {
        Mat4 res;
        res.m[0][3] = x;
        res.m[1][3] = y;
        res.m[2][3] = z;
        return res;
    }

    static Mat4 Scale(float sx, float sy, float sz) {
        Mat4 res;
        res.m[0][0] = sx;
        res.m[1][1] = sy;
        res.m[2][2] = sz;
        return res;
    }

    static Mat4 RotateX(float angleDeg) {
        Mat4 res;
        float rad = toRad(angleDeg);
        float c = cosf(rad);
        float s = sinf(rad);
        res.m[1][1] = c;
        res.m[1][2] = -s;
        res.m[2][1] = s;
        res.m[2][2] = c;
        return res;
    }

    static Mat4 RotateY(float angleDeg) {
        Mat4 res;
        float rad = toRad(angleDeg);
        float c = cosf(rad);
        float s = sinf(rad);
        res.m[0][0] = c;
        res.m[0][2] = s;
        res.m[2][0] = -s;
        res.m[2][2] = c;
        return res;
    }

    static Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Mat4 res;
        Vec3 f = (eye - center).Normalize();
        Vec3 r = Vec3::Cross(up, f).Normalize();
        Vec3 u = Vec3::Cross(f, r);

        res.m[0][0] = r.x; res.m[0][1] = r.y; res.m[0][2] = r.z;
        res.m[1][0] = u.x; res.m[1][1] = u.y; res.m[1][2] = u.z;
        res.m[2][0] = f.x; res.m[2][1] = f.y; res.m[2][2] = f.z;

        res.m[0][3] = -Vec3::Dot(r, eye);
        res.m[1][3] = -Vec3::Dot(u, eye);
        res.m[2][3] = -Vec3::Dot(f, eye);

        return res;
    }

    Mat4 operator*(const Mat4& other) const {
        Mat4 res;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                res.m[i][j] = 0.0f;
                for (int k = 0; k < 4; k++) {
                    res.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return res;
    }

    Vec3 operator*(const Vec3& v) const {
        float x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3];
        float y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3];
        float z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3];
        float w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3];

        if (w != 0.0f) {
             float invW = 1.0f / w;
             x *= invW;
             y *= invW;
             z *= invW;
        }
        return Vec3(x, y, z);
    }
};

struct EarthTransformParams {
    float rotationX;
    float rotationY;
    float scale;
    float translateX;
    float translateY;
    float translateZ;
    int16_t viewX;
    int16_t viewY;
    int16_t viewW;
    int16_t viewH;
};

class EarthRenderer;

class UIEarth : public UIView, public AnimatorCallback {
public:
    UIEarth();
    ~UIEarth() override;
    void OnDraw(BufferInfo& gfxDstBuffer, const Rect& invalidatedArea) override;
    void SetTexture(const char* path);
    void SetSegments(uint16_t cols, uint16_t rows);
    void SetCenter(float longitude, float latitude);
    void SetScale(float scale);
    void SetTranslation(float x, float y, float z);
    bool OnDragEvent(const DragEvent& event) override;
    bool OnDragEndEvent(const DragEvent& event) override;
    bool OnClickEvent(const ClickEvent& event) override;
    void Callback(UIView* view) override;
    void OnStop(UIView& view) override;

private:
    void SetupCenterAnimation();
    void SetupEdgeAnimation();

    enum EarthState {
        STATE_NORMAL,
        STATE_EDGE
    };

    enum EdgePosition {
        EDGE_BOTTOM = 0,
        EDGE_LEFT,
        EDGE_RIGHT,
        EDGE_TOP,
        EDGE_MAX
    };

    uint8_t* imageData_;

    Animator* inertiaAnimator_;
    float dragVelocityX_;
    float dragVelocityY_;
    Animator* transitionAnimator_;
    EarthState state_;

    EdgePosition nextEdgePos_;
    EarthTransformParams animStartParams_;
    EarthTransformParams animEndParams_;
    EarthTransformParams currentParams_;

    EarthRenderer* renderer_;
};
}
#endif
