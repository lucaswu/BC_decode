#include "BC_decode.h"
#include <math.h>
#include <algorithm>

#define NUM_PIXELS_PER_BLOCK 16

// BC4U/BC5U
struct BC4_UNORM
{
    float R(size_t uOffset) const noexcept
    {
        const size_t uIndex = GetIndex(uOffset);
        return DecodeFromIndex(uIndex);
    }

    float DecodeFromIndex(size_t uIndex) const noexcept
    {
        if (uIndex == 0)
            return float(red_0) / 255.0f;
        if (uIndex == 1)
            return float(red_1) / 255.0f;
        const float fred_0 = float(red_0) / 255.0f;
        const float fred_1 = float(red_1) / 255.0f;
        if (red_0 > red_1)
        {
            uIndex -= 1;
            return (fred_0 * float(7u - uIndex) + fred_1 * float(uIndex)) / 7.0f;
        }
        else
        {
            if (uIndex == 6)
                return 0.0f;
            if (uIndex == 7)
                return 1.0f;
            uIndex -= 1;
            return (fred_0 * float(5u - uIndex) + fred_1 * float(uIndex)) / 5.0f;
        }
    }

    size_t GetIndex(size_t uOffset) const noexcept
    {
        return static_cast<size_t>((data >> (3 * uOffset + 16)) & 0x07);
    }
    union
    {
        struct
        {
            uint8_t red_0;
            uint8_t red_1;
            uint8_t indices[6];
        };
        uint64_t data;
    };
};


// BC4S/BC5S
struct BC4_SNORM
{
    float R(size_t uOffset) const noexcept
    {
        const size_t uIndex = GetIndex(uOffset);
        return DecodeFromIndex(uIndex);
    }

    float DecodeFromIndex(size_t uIndex) const noexcept
    {
        const int8_t sred_0 = (red_0 == -128) ? -127 : red_0;
        const int8_t sred_1 = (red_1 == -128) ? -127 : red_1;

        if (uIndex == 0)
            return float(sred_0) / 127.0f;
        if (uIndex == 1)
            return float(sred_1) / 127.0f;
        const float fred_0 = float(sred_0) / 127.0f;
        const float fred_1 = float(sred_1) / 127.0f;
        if (red_0 > red_1)
        {
            uIndex -= 1;
            return (fred_0 * float(7u - uIndex) + fred_1 * float(uIndex)) / 7.0f;
        }
        else
        {
            if (uIndex == 6)
                return -1.0f;
            if (uIndex == 7)
                return 1.0f;
            uIndex -= 1;
            return (fred_0 * float(5u - uIndex) + fred_1 * float(uIndex)) / 5.0f;
        }
    }

    size_t GetIndex(size_t uOffset) const noexcept
    {
        return static_cast<size_t>((data >> (3 * uOffset + 16)) & 0x07);
    }

    union
    {
        struct
        {
            int8_t red_0;
            int8_t red_1;
            uint8_t indices[6];
        };
        uint64_t data;
    };
};



void decompress_bc4S(uint8_t* out, const uint8_t* in)
{
    auto pBC4 = reinterpret_cast<const BC4_SNORM*>(in);
    auto ptr = reinterpret_cast<int8_t*>(out);
    for (auto i = 0; i < NUM_PIXELS_PER_BLOCK; ++i) {
        ptr[0] = static_cast<int8_t>(std::clamp(pBC4->R(i), -1.f, 1.f) * 127.f);
        ptr[1] = ptr[0];
        ptr[2] = ptr[0];
        ptr[3] = 127;

        ptr += 4;
    }
}

void decompress_bc4U(uint8_t* out, const uint8_t* in)
{
    auto pBC4 = reinterpret_cast<const BC4_UNORM*>(in);
    auto ptr = out;
    for (auto i = 0; i < NUM_PIXELS_PER_BLOCK; ++i) {
        auto value = std::clamp(pBC4->R(i) + 0.5f/255.f, 0.0f, 1.f);
        ptr[0] = static_cast<uint8_t>(value * 255.f);
        ptr[1] = ptr[0];
        ptr[2] = ptr[0];
        ptr[3] = 0xff;

        ptr += 4;
    }

    return ;
}

void decompress_bc5S(uint8_t* out, const uint8_t* in)
{
    auto pBCR = reinterpret_cast<const BC4_SNORM*>(in);
    auto pBCG = reinterpret_cast<const BC4_SNORM*>(in + sizeof(BC4_SNORM));
    auto ptr = reinterpret_cast<int8_t*>(out);
    for (auto i = 0; i < NUM_PIXELS_PER_BLOCK; ++i) {

        auto r = std::clamp(pBCR->R(i), -1.f, 1.f);
        auto g = std::clamp(pBCG->R(i), -1.f, 1.f);

        ptr[0] = static_cast<int8_t>(r*127.f);
        ptr[1] = static_cast<int8_t>(g*127.f);
        ptr[2] = 0;
        ptr[3] = 127;

        ptr += 4;
    }
}

void decompress_bc5U(uint8_t* out, const uint8_t* in)
{
    auto pBCR = reinterpret_cast<const BC4_UNORM*>(in);
    auto pBCG = reinterpret_cast<const BC4_UNORM*>(in + sizeof(BC4_SNORM));
    auto ptr = out;
    for (auto i = 0; i < NUM_PIXELS_PER_BLOCK; ++i) {
        ptr[0] = static_cast<uint8_t>(std::clamp(pBCR->R(i) + 0.5f / 255.f, -1.f, 1.f) * 255.f);
        ptr[1] = static_cast<uint8_t>(std::clamp(pBCG->R(i) + 0.5f / 255.f, -1.f, 1.f) * 255.f);
        ptr[2] = 0;
        ptr[3] = 0xff;

        ptr += 4;
    }
}