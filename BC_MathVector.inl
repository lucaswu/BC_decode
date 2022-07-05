#pragma  once


inline VECTOR VectorMultiply(VECTOR V1, VECTOR V2)
{
#if DAMON_ANDROID
    return vmulq_f32(V1, V2);
#else
    VECTORF32 Result = { { {
                                   V1.vector4_f32[0] * V2.vector4_f32[0],
                                   V1.vector4_f32[1] * V2.vector4_f32[1],
                                   V1.vector4_f32[2] * V2.vector4_f32[2],
                                   V1.vector4_f32[3] * V2.vector4_f32[3]
                           } } };
    return Result.v;
#endif
}

inline VECTOR VectorSwizzle(VECTOR V, uint32_t E0, uint32_t E1, uint32_t E2, uint32_t E3)
{
#if DAMON_ANDROID
    static const uint32_t ControlElement[4] =
    {
        0x03020100, // XM_SWIZZLE_X
        0x07060504, // XM_SWIZZLE_Y
        0x0B0A0908, // XM_SWIZZLE_Z
        0x0F0E0D0C, // XM_SWIZZLE_W
    };

    uint8x8x2_t tbl;
    tbl.val[0] = vget_low_f32(V);
    tbl.val[1] = vget_high_f32(V);

    uint32x2_t idx = vcreate_u32(static_cast<uint64_t>(ControlElement[E0]) | (static_cast<uint64_t>(ControlElement[E1]) << 32));
    const uint8x8_t rL = vtbl2_u8(tbl, vreinterpret_u8_u32(idx));

    idx = vcreate_u32(static_cast<uint64_t>(ControlElement[E2]) | (static_cast<uint64_t>(ControlElement[E3]) << 32));
    const uint8x8_t rH = vtbl2_u8(tbl, vreinterpret_u8_u32(idx));

    return vcombine_f32(rL, rH);
#else
    VECTORF32 Result = { { {
                                   V.vector4_f32[E0],
                                   V.vector4_f32[E1],
                                   V.vector4_f32[E2],
                                   V.vector4_f32[E3]
                           } } };
    return Result.v;
#endif
}

inline VECTOR VectorSelect(VECTOR V1, VECTOR V2, VECTOR Control)
{
#if DAMON_ANDROID
    return vbslq_f32(Control, V2, V1);
#else
    VECTORU32 Result = { { {
                                   (V1.vector4_u32[0] & ~Control.vector4_u32[0]) | (V2.vector4_u32[0] & Control.vector4_u32[0]),
                                   (V1.vector4_u32[1] & ~Control.vector4_u32[1]) | (V2.vector4_u32[1] & Control.vector4_u32[1]),
                                   (V1.vector4_u32[2] & ~Control.vector4_u32[2]) | (V2.vector4_u32[2] & Control.vector4_u32[2]),
                                   (V1.vector4_u32[3] & ~Control.vector4_u32[3]) | (V2.vector4_u32[3] & Control.vector4_u32[3]),
                           } } };
    return Result.v;
#endif
}

inline VECTOR VectorMultiplyAdd(VECTOR V1, VECTOR V2, VECTOR V3) {
#if DAMON_ANDROID
    #if defined(_M_ARM64) ||  __aarch64__
    return vfmaq_f32( V3, V1, V2 );
#else
    return vmlaq_f32( V3, V1, V2 );
#endif
#else
    VECTORF32 Result = { { {
                                   V1.vector4_f32[0] * V2.vector4_f32[0] + V3.vector4_f32[0],
                                   V1.vector4_f32[1] * V2.vector4_f32[1] + V3.vector4_f32[1],
                                   V1.vector4_f32[2] * V2.vector4_f32[2] + V3.vector4_f32[2],
                                   V1.vector4_f32[3] * V2.vector4_f32[3] + V3.vector4_f32[3]
                           } } };
    return Result.v;
#endif
}
inline VECTOR VectorLerp(VECTOR V0, VECTOR V1, float t)
{
#if DAMON_ANDROID
    VECTOR L = vsubq_f32(V1, V0);
    return vmlaq_n_f32(V0, L, t);
#else
    VECTORF32 Scale;
    Scale.f[0] = t;
    Scale.f[1] = t;
    Scale.f[2] = t;
    Scale.f[3] = t;

    VECTORF32 Length = { { {
                                   V1.vector4_f32[0] - V0.vector4_f32[0],
                                   V1.vector4_f32[1] - V0.vector4_f32[1],
                                   V1.vector4_f32[2] - V0.vector4_f32[2],
                                   V1.vector4_f32[3] - V0.vector4_f32[3]
                           } } };

    return VectorMultiplyAdd(Length, Scale, V0);
#endif
}

inline VECTOR VectorZero()
{
#if DAMON_ANDROID
    return vdupq_n_f32(0);
#else
    VECTORF32 vResult = { { { 0.0f, 0.0f, 0.0f, 0.0f } } };
    return vResult.v;
#endif
}

inline VECTOR VectorAdd(VECTOR V1, VECTOR V2) {
#if DAMON_ANDROID
    return vaddq_f32(V1, V2);
#else
    VECTORF32 Result = { { {
                                   V1.vector4_f32[0] + V2.vector4_f32[0],
                                   V1.vector4_f32[1] + V2.vector4_f32[1],
                                   V1.vector4_f32[2] + V2.vector4_f32[2],
                                   V1.vector4_f32[3] + V2.vector4_f32[3]
                           } } };
    return Result.v;
#endif
}

inline VECTOR VectorMax(VECTOR V1, VECTOR V2)
{
#if DAMON_ANDROID
    return vmaxq_f32(V1, V2);
#else
    VECTORF32 Result = { { {
                                   (V1.vector4_f32[0] > V2.vector4_f32[0]) ? V1.vector4_f32[0] : V2.vector4_f32[0],
                                   (V1.vector4_f32[1] > V2.vector4_f32[1]) ? V1.vector4_f32[1] : V2.vector4_f32[1],
                                   (V1.vector4_f32[2] > V2.vector4_f32[2]) ? V1.vector4_f32[2] : V2.vector4_f32[2],
                                   (V1.vector4_f32[3] > V2.vector4_f32[3]) ? V1.vector4_f32[3] : V2.vector4_f32[3]
                           } } };
    return Result.v;
#endif
}

inline VECTOR VectorMin(VECTOR V1, VECTOR V2)
{
#if DAMON_ANDROID
    return vminq_f32(V1, V2);
#else
    VECTORF32 Result = { { {
                                   (V1.vector4_f32[0] < V2.vector4_f32[0]) ? V1.vector4_f32[0] : V2.vector4_f32[0],
                                   (V1.vector4_f32[1] < V2.vector4_f32[1]) ? V1.vector4_f32[1] : V2.vector4_f32[1],
                                   (V1.vector4_f32[2] < V2.vector4_f32[2]) ? V1.vector4_f32[2] : V2.vector4_f32[2],
                                   (V1.vector4_f32[3] < V2.vector4_f32[3]) ? V1.vector4_f32[3] : V2.vector4_f32[3]
                           } } };
    return Result.v;
#endif
}
inline VECTOR VectorClamp(VECTOR V, VECTOR Min, VECTOR Max)
{
#if DAMON_ANDROID
    VECTOR vResult;
    vResult = vmaxq_f32(Min, V);
    vResult = vminq_f32(Max, vResult);
    return vResult;
#else
    VECTOR Result;
    Result = VectorMax(Min, V);
    Result = VectorMin(Max, Result);
    return Result;
#endif
}
inline VECTOR VectorSaturate(VECTOR V)
{
#if DAMON_ANDROID
    // Set <0 to 0
    VECTOR vResult = vmaxq_f32(V, vdupq_n_f32(0));
    // Set>1 to 1
    return vminq_f32(vResult, vdupq_n_f32(1.0f));
#else
    const auto zero = VectorZero();
    VECTORF32 XMOne = { { { 1.0f, 1.0f, 1.0f, 1.0f } } };
    return VectorClamp(V, zero, XMOne.v);
#endif
}

inline VECTOR VectorTruncate(VECTOR V)
{
#if DAMON_ANDROID
    #if defined(_M_ARM64) || __aarch64__
    return vrndq_f32(V);
#else
    float32x4_t vTest = vabsq_f32(V);
    vTest = vcltq_f32(vTest, g_XMNoFraction);

    int32x4_t vInt = vcvtq_s32_f32(V);
    XMVECTOR vResult = vcvtq_f32_s32(vInt);

    // All numbers less than 8388608 will use the round to int
    // All others, use the ORIGINAL value
    return vbslq_f32(vTest, vResult, V);
#endif
#else

#define XMISNAN(x)  ((*(const uint32_t*)&(x) & 0x7F800000) == 0x7F800000 && (*(const uint32_t*)&(x) & 0x7FFFFF) != 0)
#define XMISINF(x)  ((*(const uint32_t*)&(x) & 0x7FFFFFFF) == 0x7F800000)
    VECTOR Result;
    uint32_t     i;
    Result.vector4_f32[0] = 0.0f;

    for (i = 0; i < 4; i++)
    {
        if (XMISNAN(V.vector4_f32[i]))
        {
            Result.vector4_u32[i] = 0x7FC00000;
        }
        else if (fabsf(V.vector4_f32[i]) < 8388608.0f)
        {
            Result.vector4_f32[i] = static_cast<float>(static_cast<int32_t>(V.vector4_f32[i]));
        }
        else
        {
            Result.vector4_f32[i] = V.vector4_f32[i];
        }
    }
    return Result;

#undef XMISNAN
#undef XMISINF

#endif
}

inline void StoreFloat4A(VECTOR V, float* pDestination)
{
#if DAMON_ANDROID
    vst1q_f32(reinterpret_cast<float*>(pDestination), V);
#else
    pDestination[0] = V.vector4_f32[0];
    pDestination[1] = V.vector4_f32[1];
    pDestination[2] = V.vector4_f32[2];
    pDestination[3] = V.vector4_f32[3];
#endif

}
inline void StoreUByteN4(uint8_t* pDestination, VECTOR V) {

#if DAMON_ANDROID
    float32x4_t R = vmaxq_f32(V, vdupq_n_f32(0));
    R = vminq_f32(R, vdupq_n_f32(1.0f));
    R = vmulq_n_f32(R, 255.0f);
    uint32x4_t vInt32 = vcvtq_u32_f32(R);
    uint16x4_t vInt16 = vqmovn_u32(vInt32);
    uint8x8_t vInt8 = vqmovn_u16(vcombine_u16(vInt16, vInt16));
    vst1_lane_u32((uint32_t*)pDestination, vreinterpret_u32_u8(vInt8), 0);
#else
    VECTOR N = VectorSaturate(V);

    VECTORF32 UByteMax = { { { 255.0f, 255.0f, 255.0f, 255.0f } } };

    N = VectorMultiply(N, UByteMax);
    N = VectorTruncate(N);

    float tmp[4];
    StoreFloat4A(N, tmp);

    pDestination[0] = static_cast<uint8_t>(tmp[0]);
    pDestination[1] = static_cast<uint8_t>(tmp[1]);
    pDestination[2] = static_cast<uint8_t>(tmp[2]);
    pDestination[3] = static_cast<uint8_t>(tmp[3]);
#endif
}

inline void StoreByteN4(uint8_t* pDestination, VECTOR V)
{
#if DAMON_ANDROID
    float32x4_t R = vmaxq_f32(V, vdupq_n_f32(-1.f));
    R = vminq_f32(R, vdupq_n_f32(1.0f));
    R = vmulq_n_f32(R, 127.0f);
    int32x4_t vInt32 = vcvtq_s32_f32(R);
    int16x4_t vInt16 = vqmovn_s32(vInt32);
    int8x8_t vInt8 = vqmovn_s16(vcombine_s16(vInt16, vInt16));
    vst1_lane_u32(pDestination, vreinterpret_u32_s8(vInt8), 0);
#else
    VECTORF32 negativeOne = { { { -1.0f, -1.0f, -1.0f, -1.0f } } };
    VECTORF32 one = { { { 1.0f, 1.0f, 1.0f, 1.0f } } };
    VECTORF32 byteMax = { { { 127.0f, 127.0f, 127.0f, 127.0f } } };

    VECTOR N = VectorClamp(V, negativeOne.v, one.v);
    N = VectorMultiply(V, byteMax);
    N = VectorTruncate(N);

    float tmp[4];
    StoreFloat4A(N, tmp);

    pDestination[0] = static_cast<int8_t>(tmp[0]);
    pDestination[1] = static_cast<int8_t>(tmp[1]);
    pDestination[2] = static_cast<int8_t>(tmp[2]);
    pDestination[3] = static_cast<int8_t>(tmp[3]);
#endif
}

inline VECTOR VectorSetW(VECTOR V, float w)
{
#if DAMON_ANDROID
    return vsetq_lane_f32(w, V, 3);
#else
    VECTORF32 U = { { {
                              V.vector4_f32[0],
                              V.vector4_f32[1],
                              V.vector4_f32[2],
                              w
                      } } };
    return U.v;
#endif
}

inline VECTOR VectorSubtract(VECTOR V1, VECTOR V2)
{
#if DAMON_ANDROID
    return vsubq_f32(V1, V2);
#else
    VECTORF32 Result = { { {
                                   V1.vector4_f32[0] - V2.vector4_f32[0],
                                   V1.vector4_f32[1] - V2.vector4_f32[1],
                                   V1.vector4_f32[2] - V2.vector4_f32[2],
                                   V1.vector4_f32[3] - V2.vector4_f32[3]
                           } } };
    return Result.v;
#endif
}

inline VECTOR Vector4Dot(VECTOR V1, VECTOR V2)
{
#if DAMON_ANDROID
    float32x4_t vTemp = vmulq_f32(V1, V2);
    float32x2_t v1 = vget_low_f32(vTemp);
    float32x2_t v2 = vget_high_f32(vTemp);
    v1 = vadd_f32(v1, v2);
    v1 = vpadd_f32(v1, v1);
    return vcombine_f32(v1, v1);
#else
    VECTORF32 Result;
    Result.f[0] =
    Result.f[1] =
    Result.f[2] =
    Result.f[3] = V1.vector4_f32[0] * V2.vector4_f32[0] + V1.vector4_f32[1] * V2.vector4_f32[1] + V1.vector4_f32[2] * V2.vector4_f32[2] + V1.vector4_f32[3] * V2.vector4_f32[3];
    return Result.v;
#endif
}

inline VECTOR Vector3Dot(VECTOR V1, VECTOR V2)
{
#if DAMON_ANDROID
    float32x4_t vTemp = vmulq_f32(V1, V2);
    float32x2_t v1 = vget_low_f32(vTemp);
    float32x2_t v2 = vget_high_f32(vTemp);
    v1 = vpadd_f32(v1, v1);
    v2 = vdup_lane_f32(v2, 0);
    v1 = vadd_f32(v1, v2);
    return vcombine_f32(v1, v1);
#else
    float fValue = V1.vector4_f32[0] * V2.vector4_f32[0] + V1.vector4_f32[1] * V2.vector4_f32[1] + V1.vector4_f32[2] * V2.vector4_f32[2];
    VECTORF32 vResult;
    vResult.f[0] =
    vResult.f[1] =
    vResult.f[2] =
    vResult.f[3] = fValue;
    return vResult.v;
#endif
}

inline float VectorGetX(VECTOR V)
{
#if DAMON_ANDROID
    return vgetq_lane_f32(V, 0);
#else
    return V.vector4_f32[0];
#endif
}
