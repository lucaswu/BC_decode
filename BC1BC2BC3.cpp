#include "BC_decode.h"
#include <string.h>
#include <algorithm>
#include "BC_define.h"

#define NUM_PIXELS_PER_BLOCK 16

static const VECTORF32 g_bias = { { { 0.5f / 255.f, 0.5f / 255.f, 0.5f / 255.f, 0.5f / 255.f } } };

struct XMU565
{
    union
    {
        struct
        {
            uint16_t x : 5;    
            uint16_t y : 6;    
            uint16_t z : 5;    
        };
        uint16_t v;
    };
};


struct D3DX_BC1
{
    uint16_t    rgb[2]; // 565 colors
    uint32_t    bitmap; // 2bpp rgb bitmap
};

struct D3DX_BC2
{
    uint32_t    bitmap[2];  // 4bpp alpha bitmap
    D3DX_BC1    bc1;        // BC1 rgb data
};


struct D3DX_BC3
{
    uint8_t     alpha[2];   // alpha values
    uint8_t     bitmap[6];  // 3bpp alpha bitmap
    D3DX_BC1    bc1;        // BC1 rgb data
};


inline VECTOR LoadU565(const XMU565* pSource) {

#if DAMON_ANDROID
    static const VECTORI32 U565And = { { { 0x1F, 0x3F << 5, 0x1F << 11, 0 } } };
    static const VECTORF32 U565Mul = { { { 1.0f, 1.0f / 32.0f, 1.0f / 2048.f, 0 } } };
    uint16x4_t vInt16 = vld1_dup_u16(reinterpret_cast<const uint16_t*>(pSource));
    uint32x4_t vInt = vmovl_u16(vInt16);
    vInt = vandq_u32(vInt, U565And);
    float32x4_t R = vcvtq_f32_u32(vInt);
    return vmulq_f32(R, U565Mul);
#else
    VECTORF32 result = { { {
            float(pSource->v & 0x1F),
            float((pSource->v >> 5) & 0x3F),
            float((pSource->v >> 11) & 0x1F),
            0.f,
        } } };
    return result.v;
#endif
}


inline void DecodecBC1(VECTOR* pColor, const D3DX_BC1* pBC,bool isbc1)
{
    //5:6:5: RGB B[0:4] G[5:10] R[11:15]
    VECTOR clr0 = LoadU565(reinterpret_cast<const XMU565*>(&pBC->rgb[0]));
    VECTOR clr1 = LoadU565(reinterpret_cast<const XMU565*>(&pBC->rgb[1]));


    VECTORF32 s_Scale = {  1.f / 31.f, 1.f / 63.f, 1.f / 31.f, 1.f  };

    clr0 = VectorMultiply(clr0, s_Scale);
    clr1 = VectorMultiply(clr1, s_Scale);

    //BGR->RGB
    clr0 = VectorSwizzle(clr0, 2, 1, 0, 3);
    clr1 = VectorSwizzle(clr1, 2, 1, 0, 3);


    VECTORF32 XMIdentityR3 = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    VECTORU32 XMSelect1110 = { { { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 } } };

    //alpha = 1;
    clr0 = VectorSelect(XMIdentityR3, clr0, XMSelect1110);
    clr1 = VectorSelect(XMIdentityR3, clr1, XMSelect1110);



    VECTOR clr2, clr3;
    if (isbc1 && (pBC->rgb[0] <= pBC->rgb[1])) {
        clr2 = VectorLerp(clr0, clr1, 0.5f);
        // Alpha of 0
        clr3 = VectorZero();   
    }
    else {
        clr2 = VectorLerp(clr0, clr1, 1.f / 3.f);
        clr3 = VectorLerp(clr0, clr1, 2.f / 3.f);
    }

    uint32_t dw = pBC->bitmap;
    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i, dw >>= 2)
    {
        switch (dw & 3)
        {
            case 0: pColor[i] = clr0; break;   
            case 1: pColor[i] = clr1; break;
            case 2: pColor[i] = clr2; break;
            case 3:
            default: pColor[i] = clr3; break;
        }

    }

}

void decompress_bc1(uint8_t* out, const uint8_t* in)
{
    auto pBC1 = reinterpret_cast<const D3DX_BC1*>(in);

    VECTOR data[16];
    DecodecBC1(data, pBC1, true);

    auto pDst = out;
    for (auto i = 0; i < NUM_PIXELS_PER_BLOCK; i++) {
        auto v = VectorAdd(data[i], g_bias);
        StoreUByteN4(pDst,v);
        pDst += 4;
    }
}


void decompress_bc2(uint8_t* out, const uint8_t* in)
{
    auto pBC2 = reinterpret_cast<const D3DX_BC2*>(in);


    VECTOR data[16];
    DecodecBC1(data, &pBC2->bc1, false);
    

    uint32_t dw = pBC2->bitmap[0];
    for (size_t i = 0; i < 8; ++i, dw >>= 4) {
        data[i] = VectorSetW(data[i], static_cast<float>(dw & 0xf) * (1.0f / 15.0f));
    }

    dw = pBC2->bitmap[0];
    for (size_t i = 8; i < NUM_PIXELS_PER_BLOCK; ++i, dw >>= 4) {
        data[i] = VectorSetW(data[i], static_cast<float>(dw & 0xf) * (1.0f / 15.0f));
    }

    auto pDst = out;
    for (auto i = 0; i < NUM_PIXELS_PER_BLOCK; i++) {
        auto v = VectorAdd(data[i], g_bias);
        StoreUByteN4(pDst, v);
        pDst += 4;
    }
}


void decompress_bc3(uint8_t* out, const uint8_t* in)
{
    auto pBC3 = reinterpret_cast<const D3DX_BC3*>(in);
    
    VECTOR data[16];
    DecodecBC1(data, &pBC3->bc1, false);

    // Adaptive 3-bit alpha part
    float fAlpha[8];

    fAlpha[0] = static_cast<float>(pBC3->alpha[0]) * (1.0f / 255.0f);
    fAlpha[1] = static_cast<float>(pBC3->alpha[1]) * (1.0f / 255.0f);

    if (pBC3->alpha[0] > pBC3->alpha[1])
    {
        for (size_t i = 1; i < 7; ++i)
            fAlpha[i + 1] = (fAlpha[0] * float(7u - i) + fAlpha[1] * float(i)) * (1.0f / 7.0f);
    }
    else
    {
        for (size_t i = 1; i < 5; ++i)
            fAlpha[i + 1] = (fAlpha[0] * float(5u - i) + fAlpha[1] * float(i)) * (1.0f / 5.0f);

        fAlpha[6] = 0.0f;
        fAlpha[7] = 1.0f;
    }

    uint32_t dw = uint32_t(pBC3->bitmap[0]) | uint32_t(pBC3->bitmap[1] << 8) | uint32_t(pBC3->bitmap[2] << 16);
    for (size_t i = 0; i < 8; ++i, dw >>= 3) {
        data[i] = VectorSetW(data[i],fAlpha[dw & 0x7]);
    }
       
    dw = uint32_t(pBC3->bitmap[3]) | uint32_t(pBC3->bitmap[4] << 8) | uint32_t(pBC3->bitmap[5] << 16);
    for (size_t i = 8; i < NUM_PIXELS_PER_BLOCK; ++i, dw >>= 3) {
        data[i] = VectorSetW(data[i], fAlpha[dw & 0x7]);
    }

    auto pDst = out;
    for (auto i = 0; i < NUM_PIXELS_PER_BLOCK; i++) {
        auto v = VectorAdd(data[i], g_bias);
        StoreUByteN4(pDst, v);
        pDst += 4;
    }
}