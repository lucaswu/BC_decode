#include "BC_decode.h"
#include "BC_define.h"
#include <float.h>
#include <assert.h>
#include <math.h>
#include <algorithm>

#define NUM_PIXELS_PER_BLOCK 16
typedef uint16_t HALF;
#define SIGN_EXTEND(x,nb) ((((x)&(1<<((nb)-1)))?((~0)^((1<<(nb))-1)):0)|(x))

// Because these are used in SAL annotations, they need to remain macros rather than const values
#define BC6H_MAX_REGIONS 2
#define BC6H_MAX_INDICES 16
#define BC7_MAX_REGIONS 3
#define BC7_MAX_INDICES 16


static const VECTORF32 g_bias = { { { 0.5f / 255.f, 0.5f / 255.f, 0.5f / 255.f, 0.5f / 255.f } } };

struct XMUBYTE4
{
	union
	{
		struct
		{
			uint8_t x;
			uint8_t y;
			uint8_t z;
			uint8_t w;
		};
		uint32_t v;
	};
};

struct INT4 {
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t w;
};


constexpr uint16_t F16S_MASK = 0x8000;   // f16 sign mask
constexpr uint16_t F16EM_MASK = 0x7fff;   // f16 exp & mantissa mask
constexpr uint16_t F16MAX = 0x7bff;   // MAXFLT bit pattern for XMHALF

constexpr size_t BC6H_NUM_CHANNELS = 3;
constexpr size_t BC6H_MAX_SHAPES = 32;

constexpr size_t BC7_NUM_CHANNELS = 4;
//constexpr size_t BC7_MAX_SHAPES = 64;

constexpr int32_t BC67_WEIGHT_MAX = 64;
constexpr uint32_t BC67_WEIGHT_SHIFT = 6;
constexpr int32_t BC67_WEIGHT_ROUND = 32;

//constexpr float fEpsilon = (0.25f / 64.0f) * (0.25f / 64.0f);
//constexpr float pC3[] = { 2.0f / 2.0f, 1.0f / 2.0f, 0.0f / 2.0f };
//constexpr float pD3[] = { 0.0f / 2.0f, 1.0f / 2.0f, 2.0f / 2.0f };
//constexpr float pC4[] = { 3.0f / 3.0f, 2.0f / 3.0f, 1.0f / 3.0f, 0.0f / 3.0f };
//constexpr float pD4[] = { 0.0f / 3.0f, 1.0f / 3.0f, 2.0f / 3.0f, 3.0f / 3.0f };

// Partition, Shape, Pixel (index into 4x4 block)
const uint8_t g_aPartitionTable[3][64][16] =
{
	{   // 1 Region case has no subsets (all 0)
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
	},

	{   // BC6H/BC7 Partition Set for 2 Subsets
		{ 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1 }, // Shape 0
		{ 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // Shape 1
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1 }, // Shape 2
		{ 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1 }, // Shape 3
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1 }, // Shape 4
		{ 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1 }, // Shape 5
		{ 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1 }, // Shape 6
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1 }, // Shape 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1 }, // Shape 8
		{ 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 9
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1 }, // Shape 10
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1 }, // Shape 11
		{ 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 12
		{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 13
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 14
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 }, // Shape 15
		{ 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1 }, // Shape 16
		{ 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, // Shape 17
		{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0 }, // Shape 18
		{ 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0 }, // Shape 19
		{ 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, // Shape 20
		{ 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0 }, // Shape 21
		{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0 }, // Shape 22
		{ 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1 }, // Shape 23
		{ 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0 }, // Shape 24
		{ 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0 }, // Shape 25
		{ 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0 }, // Shape 26
		{ 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0 }, // Shape 27
		{ 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0 }, // Shape 28
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, // Shape 29
		{ 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0 }, // Shape 30
		{ 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0 }, // Shape 31

															// BC7 Partition Set for 2 Subsets (second-half)
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // Shape 32
		{ 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1 }, // Shape 33
		{ 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0 }, // Shape 34
		{ 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0 }, // Shape 35
		{ 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0 }, // Shape 36
		{ 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0 }, // Shape 37
		{ 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1 }, // Shape 38
		{ 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1 }, // Shape 39
		{ 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0 }, // Shape 40
		{ 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0 }, // Shape 41
		{ 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0 }, // Shape 42
		{ 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0 }, // Shape 43
		{ 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 }, // Shape 44
		{ 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1 }, // Shape 45
		{ 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1 }, // Shape 46
		{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0 }, // Shape 47
		{ 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 }, // Shape 48
		{ 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0 }, // Shape 49
		{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0 }, // Shape 50
		{ 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0 }, // Shape 51
		{ 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1 }, // Shape 52
		{ 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1 }, // Shape 53
		{ 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0 }, // Shape 54
		{ 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0 }, // Shape 55
		{ 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1 }, // Shape 56
		{ 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1 }, // Shape 57
		{ 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 }, // Shape 58
		{ 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1 }, // Shape 59
		{ 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1 }, // Shape 60
		{ 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, // Shape 61
		{ 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // Shape 62
		{ 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1 }  // Shape 63
	},

	{   // BC7 Partition Set for 3 Subsets
		{ 0, 0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 1, 2, 2, 2, 2 }, // Shape 0
		{ 0, 0, 0, 1, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 2, 1 }, // Shape 1
		{ 0, 0, 0, 0, 2, 0, 0, 1, 2, 2, 1, 1, 2, 2, 1, 1 }, // Shape 2
		{ 0, 2, 2, 2, 0, 0, 2, 2, 0, 0, 1, 1, 0, 1, 1, 1 }, // Shape 3
		{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2 }, // Shape 4
		{ 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 2, 2 }, // Shape 5
		{ 0, 0, 2, 2, 0, 0, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 6
		{ 0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1 }, // Shape 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2 }, // Shape 8
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2 }, // Shape 9
		{ 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 10
		{ 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2 }, // Shape 11
		{ 0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1, 2 }, // Shape 12
		{ 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 2 }, // Shape 13
		{ 0, 0, 1, 1, 0, 1, 1, 2, 1, 1, 2, 2, 1, 2, 2, 2 }, // Shape 14
		{ 0, 0, 1, 1, 2, 0, 0, 1, 2, 2, 0, 0, 2, 2, 2, 0 }, // Shape 15
		{ 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 2, 1, 1, 2, 2 }, // Shape 16
		{ 0, 1, 1, 1, 0, 0, 1, 1, 2, 0, 0, 1, 2, 2, 0, 0 }, // Shape 17
		{ 0, 0, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1, 2, 2 }, // Shape 18
		{ 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 1, 1, 1, 1 }, // Shape 19
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2 }, // Shape 20
		{ 0, 0, 0, 1, 0, 0, 0, 1, 2, 2, 2, 1, 2, 2, 2, 1 }, // Shape 21
		{ 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 2, 2, 0, 1, 2, 2 }, // Shape 22
		{ 0, 0, 0, 0, 1, 1, 0, 0, 2, 2, 1, 0, 2, 2, 1, 0 }, // Shape 23
		{ 0, 1, 2, 2, 0, 1, 2, 2, 0, 0, 1, 1, 0, 0, 0, 0 }, // Shape 24
		{ 0, 0, 1, 2, 0, 0, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2 }, // Shape 25
		{ 0, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 1, 0, 1, 1, 0 }, // Shape 26
		{ 0, 0, 0, 0, 0, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 1 }, // Shape 27
		{ 0, 0, 2, 2, 1, 1, 0, 2, 1, 1, 0, 2, 0, 0, 2, 2 }, // Shape 28
		{ 0, 1, 1, 0, 0, 1, 1, 0, 2, 0, 0, 2, 2, 2, 2, 2 }, // Shape 29
		{ 0, 0, 1, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 0, 1, 1 }, // Shape 30
		{ 0, 0, 0, 0, 2, 0, 0, 0, 2, 2, 1, 1, 2, 2, 2, 1 }, // Shape 31
		{ 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 2, 2, 2 }, // Shape 32
		{ 0, 2, 2, 2, 0, 0, 2, 2, 0, 0, 1, 2, 0, 0, 1, 1 }, // Shape 33
		{ 0, 0, 1, 1, 0, 0, 1, 2, 0, 0, 2, 2, 0, 2, 2, 2 }, // Shape 34
		{ 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0 }, // Shape 35
		{ 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0 }, // Shape 36
		{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0 }, // Shape 37
		{ 0, 1, 2, 0, 2, 0, 1, 2, 1, 2, 0, 1, 0, 1, 2, 0 }, // Shape 38
		{ 0, 0, 1, 1, 2, 2, 0, 0, 1, 1, 2, 2, 0, 0, 1, 1 }, // Shape 39
		{ 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0, 1, 1 }, // Shape 40
		{ 0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 41
		{ 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 2, 1, 2, 1 }, // Shape 42
		{ 0, 0, 2, 2, 1, 1, 2, 2, 0, 0, 2, 2, 1, 1, 2, 2 }, // Shape 43
		{ 0, 0, 2, 2, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 1, 1 }, // Shape 44
		{ 0, 2, 2, 0, 1, 2, 2, 1, 0, 2, 2, 0, 1, 2, 2, 1 }, // Shape 45
		{ 0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 1 }, // Shape 46
		{ 0, 0, 0, 0, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1 }, // Shape 47
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2 }, // Shape 48
		{ 0, 2, 2, 2, 0, 1, 1, 1, 0, 2, 2, 2, 0, 1, 1, 1 }, // Shape 49
		{ 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 2 }, // Shape 50
		{ 0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1, 2 }, // Shape 51
		{ 0, 2, 2, 2, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2 }, // Shape 52
		{ 0, 0, 0, 2, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0, 2 }, // Shape 53
		{ 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 2, 2 }, // Shape 54
		{ 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 1, 2 }, // Shape 55
		{ 0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 56
		{ 0, 0, 2, 2, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 2, 2 }, // Shape 57
		{ 0, 0, 2, 2, 1, 1, 2, 2, 1, 1, 2, 2, 0, 0, 2, 2 }, // Shape 58
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2 }, // Shape 59
		{ 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1 }, // Shape 60
		{ 0, 2, 2, 2, 1, 2, 2, 2, 0, 2, 2, 2, 1, 2, 2, 2 }, // Shape 61
		{ 0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 62
		{ 0, 1, 1, 1, 2, 0, 1, 1, 2, 2, 0, 1, 2, 2, 2, 0 }  // Shape 63
	}
};

// Partition, Shape, Fixup
const uint8_t g_aFixUp[3][64][3] =
{
	{   // No fix-ups for 1st subset for BC6H or BC7
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
		{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 }
	},

	{   // BC6H/BC7 Partition Set Fixups for 2 Subsets
		{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
		{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
		{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
		{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
		{ 0,15, 0 },{ 0, 2, 0 },{ 0, 8, 0 },{ 0, 2, 0 },
		{ 0, 2, 0 },{ 0, 8, 0 },{ 0, 8, 0 },{ 0,15, 0 },
		{ 0, 2, 0 },{ 0, 8, 0 },{ 0, 2, 0 },{ 0, 2, 0 },
		{ 0, 8, 0 },{ 0, 8, 0 },{ 0, 2, 0 },{ 0, 2, 0 },

		// BC7 Partition Set Fixups for 2 Subsets (second-half)
		{ 0,15, 0 },{ 0,15, 0 },{ 0, 6, 0 },{ 0, 8, 0 },
		{ 0, 2, 0 },{ 0, 8, 0 },{ 0,15, 0 },{ 0,15, 0 },
		{ 0, 2, 0 },{ 0, 8, 0 },{ 0, 2, 0 },{ 0, 2, 0 },
		{ 0, 2, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0, 6, 0 },
		{ 0, 6, 0 },{ 0, 2, 0 },{ 0, 6, 0 },{ 0, 8, 0 },
		{ 0,15, 0 },{ 0,15, 0 },{ 0, 2, 0 },{ 0, 2, 0 },
		{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
		{ 0,15, 0 },{ 0, 2, 0 },{ 0, 2, 0 },{ 0,15, 0 }
	},

	{   // BC7 Partition Set Fixups for 3 Subsets
		{ 0, 3,15 },{ 0, 3, 8 },{ 0,15, 8 },{ 0,15, 3 },
		{ 0, 8,15 },{ 0, 3,15 },{ 0,15, 3 },{ 0,15, 8 },
		{ 0, 8,15 },{ 0, 8,15 },{ 0, 6,15 },{ 0, 6,15 },
		{ 0, 6,15 },{ 0, 5,15 },{ 0, 3,15 },{ 0, 3, 8 },
		{ 0, 3,15 },{ 0, 3, 8 },{ 0, 8,15 },{ 0,15, 3 },
		{ 0, 3,15 },{ 0, 3, 8 },{ 0, 6,15 },{ 0,10, 8 },
		{ 0, 5, 3 },{ 0, 8,15 },{ 0, 8, 6 },{ 0, 6,10 },
		{ 0, 8,15 },{ 0, 5,15 },{ 0,15,10 },{ 0,15, 8 },
		{ 0, 8,15 },{ 0,15, 3 },{ 0, 3,15 },{ 0, 5,10 },
		{ 0, 6,10 },{ 0,10, 8 },{ 0, 8, 9 },{ 0,15,10 },
		{ 0,15, 6 },{ 0, 3,15 },{ 0,15, 8 },{ 0, 5,15 },
		{ 0,15, 3 },{ 0,15, 6 },{ 0,15, 6 },{ 0,15, 8 },
		{ 0, 3,15 },{ 0,15, 3 },{ 0, 5,15 },{ 0, 5,15 },
		{ 0, 5,15 },{ 0, 8,15 },{ 0, 5,15 },{ 0,10,15 },
		{ 0, 5,15 },{ 0,10,15 },{ 0, 8,15 },{ 0,13,15 },
		{ 0,15, 3 },{ 0,12,15 },{ 0, 3,15 },{ 0, 3, 8 }
	}
};

//BPTC interpolation factors
const int g_aWeights2[] = { 0, 21, 43, 64 };
const int g_aWeights3[] = { 0, 9, 18, 27, 37, 46, 55, 64 };
const int g_aWeights4[] = { 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64 };

class LDRColorA;

inline VECTOR LoadUByte4(const XMUBYTE4* pSource) 
{
#if DAMON_ANDROID
	uint32x2_t vInt8 = vld1_dup_u32(reinterpret_cast<const uint32_t*>(pSource));
	uint16x8_t vInt16 = vmovl_u8(vreinterpret_u8_u32(vInt8));
	uint32x4_t vInt = vmovl_u16(vget_low_u16(vInt16));
	return vcvtq_f32_u32(vInt);
#else
	VECTORF32 vResult = { { {
			static_cast<float>(pSource->x),
			static_cast<float>(pSource->y),
			static_cast<float>(pSource->z),
			static_cast<float>(pSource->w)
		} } };
	return vResult.v;
#endif
}

inline VECTOR LoadSInt4(const INT4* pSource)
{
#if DAMON_ANDROID
	int32x4_t v = vld1q_s32(reinterpret_cast<const int32_t*>(pSource));
	return vcvtq_f32_s32(v);
#else
	VECTOR V;
	V.vector4_f32[0] = static_cast<float>(pSource->x);
	V.vector4_f32[1] = static_cast<float>(pSource->y);
	V.vector4_f32[2] = static_cast<float>(pSource->z);
	V.vector4_f32[3] = static_cast<float>(pSource->w);
	return V;
#endif
}


class HDRColorA
{
public:
	float r, g, b, a;

public:
	HDRColorA() = default;
	HDRColorA(float _r, float _g, float _b, float _a) noexcept : r(_r), g(_g), b(_b), a(_a) {}
	HDRColorA(const HDRColorA& c) noexcept : r(c.r), g(c.g), b(c.b), a(c.a) {}

	// binary operators
	HDRColorA operator + (const HDRColorA& c) const noexcept
	{
		return HDRColorA(r + c.r, g + c.g, b + c.b, a + c.a);
	}

	HDRColorA operator - (const HDRColorA& c) const noexcept
	{
		return HDRColorA(r - c.r, g - c.g, b - c.b, a - c.a);
	}

	HDRColorA operator * (float f) const noexcept
	{
		return HDRColorA(r * f, g * f, b * f, a * f);
	}

	HDRColorA operator / (float f) const noexcept
	{
		const float fInv = 1.0f / f;
		return HDRColorA(r * fInv, g * fInv, b * fInv, a * fInv);
	}

	float operator * (const HDRColorA& c) const noexcept
	{
		return r * c.r + g * c.g + b * c.b + a * c.a;
	}

	// HDRColorA operator = (const HDRColorA& c) const noexcept
	// {
	//     return HDRColorA(c.r, c.g, c.b, c.a);
	// }
	// assignment operators
	HDRColorA& operator += (const HDRColorA& c) noexcept
	{
		r += c.r;
		g += c.g;
		b += c.b;
		a += c.a;
		return *this;
	}

	HDRColorA& operator -= (const HDRColorA& c) noexcept
	{
		r -= c.r;
		g -= c.g;
		b -= c.b;
		a -= c.a;
		return *this;
	}

	HDRColorA& operator *= (float f) noexcept
	{
		r *= f;
		g *= f;
		b *= f;
		a *= f;
		return *this;
	}

	HDRColorA& operator /= (float f) noexcept
	{
		const float fInv = 1.0f / f;
		r *= fInv;
		g *= fInv;
		b *= fInv;
		a *= fInv;
		return *this;
	}

	HDRColorA& Clamp( float fMin,  float fMax) noexcept
	{
		r = std::min<float>(fMax, std::max<float>(fMin, r));
		g = std::min<float>(fMax, std::max<float>(fMin, g));
		b = std::min<float>(fMax, std::max<float>(fMin, b));
		a = std::min<float>(fMax, std::max<float>(fMin, a));
		return *this;
	}

	HDRColorA(const LDRColorA& c) noexcept;
	HDRColorA& operator = (const LDRColorA& c) noexcept;
	LDRColorA ToLDRColorA() const noexcept;
};

inline HDRColorA* HDRColorALerp( HDRColorA* pOut,  const HDRColorA* pC1,  const HDRColorA* pC2,  float s) noexcept
{
	pOut->r = pC1->r + s * (pC2->r - pC1->r);
	pOut->g = pC1->g + s * (pC2->g - pC1->g);
	pOut->b = pC1->b + s * (pC2->b - pC1->b);
	pOut->a = pC1->a + s * (pC2->a - pC1->a);
	return pOut;
}

class LDRColorA
{
public:
	uint8_t r, g, b, a;

	LDRColorA() = default;
	LDRColorA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) noexcept : r(_r), g(_g), b(_b), a(_a) {}

	const uint8_t& operator [] (size_t uElement) const noexcept
	{
		switch (uElement)
		{
		case 0: return r;
		case 1: return g;
		case 2: return b;
		case 3: return a;
		default:  return r;
		}
	}

	uint8_t& operator [] (size_t uElement) noexcept
	{
		switch (uElement)
		{
		case 0: return r;
		case 1: return g;
		case 2: return b;
		case 3: return a;
		default:  return r;
		}
	}

	LDRColorA operator = ( const HDRColorA& c) noexcept
	{
		LDRColorA ret;
		HDRColorA tmp(c);
		tmp = tmp.Clamp(0.0f, 1.0f) * 255.0f;
		ret.r = uint8_t(tmp.r + 0.001f);
		ret.g = uint8_t(tmp.g + 0.001f);
		ret.b = uint8_t(tmp.b + 0.001f);
		ret.a = uint8_t(tmp.a + 0.001f);
		return ret;
	}

	static void InterpolateRGB( const LDRColorA& c0,  const LDRColorA& c1,  size_t wc,   size_t wcprec,  LDRColorA& out) noexcept
	{
		const int* aWeights = nullptr;
		switch (wcprec)
		{
			case 2: aWeights = g_aWeights2; break;
			case 3: aWeights = g_aWeights3; break;
			case 4: aWeights = g_aWeights4; break;
			default: out.r = out.g = out.b = 0; return;
		}
		//interpolated value = ((64-weight)*E0 + weight*E1 + 32)>>6
		out.r = uint8_t((uint32_t(c0.r) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.r) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
		out.g = uint8_t((uint32_t(c0.g) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.g) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
		out.b = uint8_t((uint32_t(c0.b) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.b) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
	}

	static void InterpolateA( const LDRColorA& c0,  const LDRColorA& c1,  size_t wa,  size_t waprec,  LDRColorA& out) noexcept
	{
		const int* aWeights = nullptr;
		switch (waprec)
		{
		case 2: aWeights = g_aWeights2; break;
		case 3: aWeights = g_aWeights3; break;
		case 4: aWeights = g_aWeights4; break;
		default: out.a = 0; return;
		}
		out.a = uint8_t((uint32_t(c0.a) * uint32_t(BC67_WEIGHT_MAX - aWeights[wa]) + uint32_t(c1.a) * uint32_t(aWeights[wa]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
	}

	static void Interpolate( const LDRColorA& c0,  const LDRColorA& c1,  size_t wc,  size_t wa,  size_t wcprec,  size_t waprec,  LDRColorA& out) noexcept
	{
		InterpolateRGB(c0, c1, wc, wcprec, out);
		InterpolateA(c0, c1, wa, waprec, out);
	}
};

static_assert(sizeof(LDRColorA) == 4, "Unexpected packing");

struct LDREndPntPair
{
	LDRColorA A;
	LDRColorA B;
};

inline HDRColorA::HDRColorA(const LDRColorA& c) noexcept
{
	r = float(c.r) * (1.0f / 255.0f);
	g = float(c.g) * (1.0f / 255.0f);
	b = float(c.b) * (1.0f / 255.0f);
	a = float(c.a) * (1.0f / 255.0f);
}

inline HDRColorA& HDRColorA::operator = (const LDRColorA& c) noexcept
{
	r = static_cast<float>(c.r);
	g = static_cast<float>(c.g);
	b = static_cast<float>(c.b);
	a = static_cast<float>(c.a);
	return *this;
}

inline LDRColorA HDRColorA::ToLDRColorA() const noexcept
{
	return LDRColorA(static_cast<uint8_t>(r + 0.01f), static_cast<uint8_t>(g + 0.01f), static_cast<uint8_t>(b + 0.01f), static_cast<uint8_t>(a + 0.01f));
}

static inline float HalfToFloat(HALF Value)
{
#if DAMON_ANDROID
	uint16x4_t vHalf = vdup_n_u16(Value);
	float32x4_t vFloat = vcvt_f32_f16(vreinterpret_f16_u16(vHalf));
	return vgetq_lane_f32(vFloat, 0);
#else
	auto Mantissa = static_cast<uint32_t>(Value & 0x03FF);

	uint32_t Exponent = (Value & 0x7C00);
	if (Exponent == 0x7C00) // INF/NAN
	{
		Exponent = 0x8f;
	}
	else if (Exponent != 0)  // The value is normalized
	{
		Exponent = static_cast<uint32_t>((static_cast<int>(Value) >> 10) & 0x1F);
	}
	else if (Mantissa != 0)     // The value is denormalized
	{
		// Normalize the value in the resulting float
		Exponent = 1;

		do
		{
			Exponent--;
			Mantissa <<= 1;
		} while ((Mantissa & 0x0400) == 0);

		Mantissa &= 0x03FF;
	}
	else                        // The value is zero
	{
		Exponent = static_cast<uint32_t>(-112);
	}

	uint32_t Result = ((static_cast<uint32_t>(Value) & 0x8000) << 16) // Sign
		| ((Exponent + 112) << 23)                      // Exponent
		| (Mantissa << 13);                             // Mantissa

	return reinterpret_cast<float*>(&Result)[0];
#endif
}

static inline HALF FloatToHalf(float Value)
{
	uint32_t Result;

	auto IValue = reinterpret_cast<uint32_t*>(&Value)[0];
	uint32_t Sign = (IValue & 0x80000000U) >> 16U;
	IValue = IValue & 0x7FFFFFFFU;      // Hack off the sign

	if (IValue > 0x477FE000U)
	{
		// The number is too large to be represented as a half.  Saturate to infinity.
		if (((IValue & 0x7F800000) == 0x7F800000) && ((IValue & 0x7FFFFF) != 0))
		{
			Result = 0x7FFF; // NAN
		}
		else
		{
			Result = 0x7C00U; // INF
		}
	}
	else if (!IValue)
	{
		Result = 0;
	}
	else
	{
		if (IValue < 0x38800000U)
		{
			// The number is too small to be represented as a normalized half.
			// Convert it to a denormalized value.
			uint32_t Shift = 113U - (IValue >> 23U);
			IValue = (0x800000U | (IValue & 0x7FFFFFU)) >> Shift;
		}
		else
		{
			// Rebias the exponent to represent the value as a normalized half.
			IValue += 0xC8000000U;
		}

		Result = ((IValue + 0x0FFFU + ((IValue >> 13U) & 1U)) >> 13U) & 0x7FFFU;
	}
	return static_cast<HALF>(Result | Sign);
}

class INTColor
{
public:
	int r, g, b;
	int pad;

public:
	INTColor() = default;
	INTColor(int nr, int ng, int nb) noexcept : r(nr), g(ng), b(nb), pad(0) {}
	INTColor(const INTColor& c) noexcept : r(c.r), g(c.g), b(c.b), pad(0) {}

	INTColor& operator += ( const INTColor& c) noexcept
	{
		r += c.r;
		g += c.g;
		b += c.b;
		return *this;
	}

	INTColor& operator -= ( const INTColor& c) noexcept
	{
		r -= c.r;
		g -= c.g;
		b -= c.b;
		return *this;
	}

	INTColor& operator &= ( const INTColor& c) noexcept
	{
		r &= c.r;
		g &= c.g;
		b &= c.b;
		return *this;
	}

	int& operator [] ( uint8_t i) noexcept
	{
		return reinterpret_cast<int*>(this)[i];
	}

	void Set( const HDRColorA& c,  bool bSigned) noexcept
	{
		/*PackedVector::XMHALF4 aF16;

		const XMVECTOR v = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&c));
		XMStoreHalf4(&aF16, v);*/
		auto ptr = reinterpret_cast<const float*>(&c);
		float v[4] = { ptr[0],ptr[1],ptr[2],ptr[3] };

		HALF v_half[4];
		v_half[0] = FloatToHalf(v[0]);
		v_half[1] = FloatToHalf(v[1]);
		v_half[2] = FloatToHalf(v[2]);


		r = F16ToINT(v_half[0], bSigned);
		g = F16ToINT(v_half[1], bSigned);
		b = F16ToINT(v_half[2], bSigned);
	}

	INTColor& Clamp( int iMin,  int iMax) noexcept
	{
		r = std::min<int>(iMax, std::max<int>(iMin, r));
		g = std::min<int>(iMax, std::max<int>(iMin, g));
		b = std::min<int>(iMax, std::max<int>(iMin, b));
		return *this;
	}

	INTColor& SignExtend( const LDRColorA& Prec) noexcept
	{
		r = SIGN_EXTEND(r, int(Prec.r));
		g = SIGN_EXTEND(g, int(Prec.g));
		b = SIGN_EXTEND(b, int(Prec.b));
		return *this;
	}

	void ToF16(HALF aF16[3],  bool bSigned) const noexcept
	{
		aF16[0] = INT2F16(r, bSigned);
		aF16[1] = INT2F16(g, bSigned);
		aF16[2] = INT2F16(b, bSigned);
	}

private:
	static int F16ToINT( const HALF& f,  bool bSigned) noexcept
	{
		uint16_t input = *reinterpret_cast<const uint16_t*>(&f);
		int out, s;
		if (bSigned)
		{
			s = input & F16S_MASK;
			input &= F16EM_MASK;
			if (input > F16MAX) out = F16MAX;
			else out = input;
			out = s ? -out : out;
		}
		else
		{
			if (input & F16S_MASK) out = 0;
			else out = input;
		}
		return out;
	}

	static HALF INT2F16( int input,  bool bSigned) noexcept
	{
		HALF h;
		uint16_t out;
		if (bSigned)
		{
			int s = 0;
			if (input < 0)
			{
				s = F16S_MASK;
				input = -input;
			}
			out = uint16_t(s | input);
		}
		else
		{
			out = static_cast<uint16_t>(input);
		}

		*reinterpret_cast<uint16_t*>(&h) = out;
		return h;
	}
};

static_assert(sizeof(INTColor) == 16, "Unexpected packing");

struct INTEndPntPair
{
	INTColor A;
	INTColor B;
};

template< size_t SizeInBytes >
class CBits
{
public:
	uint8_t GetBit( size_t& uStartBit) const noexcept
	{
		const size_t uIndex = uStartBit >> 3;
		auto const ret = static_cast<uint8_t>((m_uBits[uIndex] >> (uStartBit - (uIndex << 3))) & 0x01);
		uStartBit++;
		return ret;
	}

	uint8_t GetBits( size_t& uStartBit,  size_t uNumBits) const noexcept
	{
		if (uNumBits == 0) return 0;
		uint8_t ret;
		const size_t uIndex = uStartBit >> 3;
		const size_t uBase = uStartBit - (uIndex << 3);
		if (uBase + uNumBits > 8)
		{
			const size_t uFirstIndexBits = 8 - uBase;
			const size_t uNextIndexBits = uNumBits - uFirstIndexBits;
			ret = static_cast<uint8_t>((unsigned(m_uBits[uIndex]) >> uBase) | ((unsigned(m_uBits[uIndex + 1]) & ((1u << uNextIndexBits) - 1)) << uFirstIndexBits));
		}
		else
		{
			ret = static_cast<uint8_t>((m_uBits[uIndex] >> uBase) & ((1 << uNumBits) - 1));
		}
		uStartBit += uNumBits;
		return ret;
	}

private:
	uint8_t m_uBits[SizeInBytes];
};


static void FillWithErrorColors(HDRColorA* pOut) noexcept
{
	for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
	{
		// In production use, default to black
		pOut[i] = HDRColorA(0.0f, 0.0f, 0.0f, 1.0f);

	}
}
static inline bool IsFixUpOffset(size_t uPartitions, size_t uShape, size_t uOffset) noexcept
{

	for (size_t p = 0; p <= uPartitions; p++)
	{
		if (uOffset == g_aFixUp[uPartitions][uShape][p])
		{
			return true;
		}
	}
	return false;
}

static inline void TransformForward(INTEndPntPair aEndPts[]) noexcept
{
	aEndPts[0].B -= aEndPts[0].A;
	aEndPts[1].A -= aEndPts[0].A;
	aEndPts[1].B -= aEndPts[0].A;
}

static inline void TransformInverse(INTEndPntPair aEndPts[],  const LDRColorA& Prec,  bool bSigned) noexcept
{
	const INTColor WrapMask((1 << Prec.r) - 1, (1 << Prec.g) - 1, (1 << Prec.b) - 1);
	aEndPts[0].B += aEndPts[0].A; aEndPts[0].B &= WrapMask;
	aEndPts[1].A += aEndPts[0].A; aEndPts[1].A &= WrapMask;
	aEndPts[1].B += aEndPts[0].A; aEndPts[1].B &= WrapMask;
	if (bSigned)
	{
		aEndPts[0].B.SignExtend(Prec);
		aEndPts[1].A.SignExtend(Prec);
		aEndPts[1].B.SignExtend(Prec);
	}
}

static inline float Norm( const INTColor& a,  const INTColor& b) noexcept
{
	const float dr = float(a.r) - float(b.r);
	const float dg = float(a.g) - float(b.g);
	const float db = float(a.b) - float(b.b);
	return dr * dr + dg * dg + db * db;
}

// return # of bits needed to store n. handle signed or unsigned cases properly
static inline int NBits( int n,  bool bIsSigned) noexcept
{
	int nb;
	if (n == 0)
	{
		return 0;	// no bits needed for 0, signed or not
	}
	else if (n > 0)
	{
		for (nb = 0; n; ++nb, n >>= 1);
		return nb + (bIsSigned ? 1 : 0);
	}
	else
	{
		for (nb = 0; n < -1; ++nb, n >>= 1);
		return nb + 1;
	}
}

static float ComputeError(
	 const LDRColorA& pixel,
	const LDRColorA aPalette[],
	uint8_t uIndexPrec,
	uint8_t uIndexPrec2,
	 size_t* pBestIndex = nullptr,
	 size_t* pBestIndex2 = nullptr) noexcept
{
	const size_t uNumIndices = size_t(1) << uIndexPrec;
	const size_t uNumIndices2 = size_t(1) << uIndexPrec2;
	float fTotalErr = 0;
	float fBestErr = FLT_MAX;

	if (pBestIndex)
		*pBestIndex = 0;
	if (pBestIndex2)
		*pBestIndex2 = 0;

	 const VECTOR vpixel = LoadUByte4(reinterpret_cast<const XMUBYTE4*>(&pixel));

	if (uIndexPrec2 == 0)
	{
		for (size_t i = 0; i < uNumIndices && fBestErr > 0; i++)
		{
			VECTOR tpixel = LoadUByte4(reinterpret_cast<const XMUBYTE4*>(&aPalette[i]));
			// Compute ErrorMetric
			tpixel = VectorSubtract(vpixel, tpixel);
			const float fErr = VectorGetX(Vector4Dot(tpixel, tpixel));
				break;
			if (fErr < fBestErr)
			{
				fBestErr = fErr;
				if (pBestIndex)
					*pBestIndex = i;
			}
		}
		fTotalErr += fBestErr;
	}
	else
	{
		for (size_t i = 0; i < uNumIndices && fBestErr > 0; i++)
		{
			 VECTOR tpixel = LoadUByte4(reinterpret_cast<const XMUBYTE4*>(&aPalette[i]));
			// Compute ErrorMetricRGB
			tpixel = VectorSubtract(vpixel, tpixel);
			const float fErr = VectorGetX(Vector3Dot(tpixel, tpixel));
			if (fErr > fBestErr)	// error increased, so we're done searching
				break;
			if (fErr < fBestErr)
			{
				fBestErr = fErr;
				if (pBestIndex)
					*pBestIndex = i;
			}
		}
		fTotalErr += fBestErr;
		fBestErr = FLT_MAX;
		for (size_t i = 0; i < uNumIndices2 && fBestErr > 0; i++)
		{
			// Compute ErrorMetricAlpha
			const float ea = float(pixel.a) - float(aPalette[i].a);
			const float fErr = ea * ea;
			if (fErr > fBestErr)	// error increased, so we're done searching
				break;
			if (fErr < fBestErr)
			{
				fBestErr = fErr;
				if (pBestIndex2)
					*pBestIndex2 = i;
			}
		}
		fTotalErr += fBestErr;
	}

	return fTotalErr;
}

class D3DX_BC7 : private CBits< 16 >
{
public:
	void Decode(HDRColorA* pOut) const noexcept;
private:
	struct ModeInfo
	{
		uint8_t uPartitions;
		uint8_t uPartitionBits; //PB: Partition selection bits
		uint8_t uPBits;        
		uint8_t uRotationBits;
		uint8_t uIndexModeBits; //ISB:index selection bits
		uint8_t uIndexPrec;   //IB: Index bits
		uint8_t uIndexPrec2; //IB2: Secondary index bits
		LDRColorA RGBAPrec;  //color bits
		LDRColorA RGBAPrecWithP; //color bits + alpha bits
	};


	static uint8_t Quantize( uint8_t comp,  uint8_t uPrec) noexcept
	{
		const uint8_t rnd = std::min<uint8_t>(255u, static_cast<uint8_t>(unsigned(comp) + (1u << (7 - uPrec))));
		return uint8_t(rnd >> (8u - uPrec));
	}

	static LDRColorA Quantize( const LDRColorA& c,  const LDRColorA& RGBAPrec) noexcept
	{
		LDRColorA q;
		q.r = Quantize(c.r, RGBAPrec.r);
		q.g = Quantize(c.g, RGBAPrec.g);
		q.b = Quantize(c.b, RGBAPrec.b);
		if (RGBAPrec.a)
			q.a = Quantize(c.a, RGBAPrec.a);
		else
			q.a = 255;
		return q;
	}

	static uint8_t Unquantize( uint8_t comp,  size_t uPrec) noexcept
	{
		comp = static_cast<uint8_t>(unsigned(comp) << (8 - uPrec));
		return uint8_t(comp | (comp >> uPrec));
	}

	static LDRColorA Unquantize( const LDRColorA& c,  const LDRColorA& RGBAPrec) noexcept
	{
		LDRColorA q;
		q.r = Unquantize(c.r, RGBAPrec.r);
		q.g = Unquantize(c.g, RGBAPrec.g);
		q.b = Unquantize(c.b, RGBAPrec.b);
		q.a = RGBAPrec.a > 0 ? Unquantize(c.a, RGBAPrec.a) : 255u;
		return q;
	}
private:
	static const ModeInfo ms_aInfo[];
};

const D3DX_BC7::ModeInfo D3DX_BC7::ms_aInfo[] =
{
	{2, 4, 6, 0, 0, 3, 0, LDRColorA(4,4,4,0), LDRColorA(5,5,5,0)},
	// Mode 0: Color only, 3 Subsets, RGBP 4441 (unique P-bit), 3-bit indecies, 16 partitions
	{1, 6, 2, 0, 0, 3, 0, LDRColorA(6,6,6,0), LDRColorA(7,7,7,0)},
	// Mode 1: Color only, 2 Subsets, RGBP 6661 (shared P-bit), 3-bit indecies, 64 partitions
	{2, 6, 0, 0, 0, 2, 0, LDRColorA(5,5,5,0), LDRColorA(5,5,5,0)},
	// Mode 2: Color only, 3 Subsets, RGB 555, 2-bit indecies, 64 partitions
	{1, 6, 4, 0, 0, 2, 0, LDRColorA(7,7,7,0), LDRColorA(8,8,8,0)},
	// Mode 3: Color only, 2 Subsets, RGBP 7771 (unique P-bit), 2-bits indecies, 64 partitions
	{0, 0, 0, 2, 1, 2, 3, LDRColorA(5,5,5,6), LDRColorA(5,5,5,6)},
	// Mode 4: Color w/ Separate Alpha, 1 Subset, RGB 555, A6, 16x2/16x3-bit indices, 2-bit rotation, 1-bit index selector
	{0, 0, 0, 2, 0, 2, 2, LDRColorA(7,7,7,8), LDRColorA(7,7,7,8)},
	// Mode 5: Color w/ Separate Alpha, 1 Subset, RGB 777, A8, 16x2/16x2-bit indices, 2-bit rotation
	{0, 0, 2, 0, 0, 4, 0, LDRColorA(7,7,7,7), LDRColorA(8,8,8,8)},
	// Mode 6: Color+Alpha, 1 Subset, RGBAP 77771 (unique P-bit), 16x4-bit indecies
	{1, 6, 4, 0, 0, 2, 0, LDRColorA(5,5,5,5), LDRColorA(6,6,6,6)}
	// Mode 7: Color+Alpha, 2 Subsets, RGBAP 55551 (unique P-bit), 2-bit indices, 64 partitions
};


//Reference: https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc7-format
void D3DX_BC7::Decode(HDRColorA* pOut) const noexcept
{
	size_t uFirst = 0;
	while (uFirst < 128 && !GetBit(uFirst)) {}
	const uint8_t uMode = uint8_t(uFirst - 1);

	if (uMode < 8)
	{
		const uint8_t uPartitions = ms_aInfo[uMode].uPartitions;

		auto const uNumEndPts = static_cast<const uint8_t>((unsigned(uPartitions) + 1u) << 1);
		const uint8_t uIndexPrec = ms_aInfo[uMode].uIndexPrec;
		const uint8_t uIndexPrec2 = ms_aInfo[uMode].uIndexPrec2;
		size_t i;
		size_t uStartBit = size_t(uMode) + 1;
		uint8_t P[6];
		const uint8_t uShape = GetBits(uStartBit, ms_aInfo[uMode].uPartitionBits);
		const uint8_t uRotation = GetBits(uStartBit, ms_aInfo[uMode].uRotationBits);
		const uint8_t uIndexMode = GetBits(uStartBit, ms_aInfo[uMode].uIndexModeBits);


		LDRColorA c[BC7_MAX_REGIONS << 1];
		const LDRColorA RGBAPrec = ms_aInfo[uMode].RGBAPrec;
		const LDRColorA RGBAPrecWithP = ms_aInfo[uMode].RGBAPrecWithP;



		// Red channel
		for (i = 0; i < uNumEndPts; i++)
		{
			if (uStartBit + RGBAPrec.r > 128)
			{

				FillWithErrorColors(pOut);
				return;
			}

			c[i].r = GetBits(uStartBit, RGBAPrec.r);
		}

		// Green channel
		for (i = 0; i < uNumEndPts; i++)
		{
			if (uStartBit + RGBAPrec.g > 128)
			{

				FillWithErrorColors(pOut);
				return;
			}

			c[i].g = GetBits(uStartBit, RGBAPrec.g);
		}

		// Blue channel
		for (i = 0; i < uNumEndPts; i++)
		{
			if (uStartBit + RGBAPrec.b > 128)
			{

				FillWithErrorColors(pOut);
				return;
			}

			c[i].b = GetBits(uStartBit, RGBAPrec.b);
		}

		// Alpha channel
		for (i = 0; i < uNumEndPts; i++)
		{
			if (uStartBit + RGBAPrec.a > 128)
			{

				FillWithErrorColors(pOut);
				return;
			}

			c[i].a = RGBAPrec.a ? GetBits(uStartBit, RGBAPrec.a) : 255u;
		}

		// P-bits
		for (i = 0; i < ms_aInfo[uMode].uPBits; i++)
		{
			if (uStartBit > 127)
			{

				FillWithErrorColors(pOut);
				return;
			}

			P[i] = GetBit(uStartBit);
		}

		if (ms_aInfo[uMode].uPBits)
		{
			for (i = 0; i < uNumEndPts; i++)
			{
				size_t pi = i * ms_aInfo[uMode].uPBits / uNumEndPts;
				for (uint8_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
				{
					if (RGBAPrec[ch] != RGBAPrecWithP[ch])
					{
						c[i][ch] = static_cast<uint8_t>((unsigned(c[i][ch]) << 1) | P[pi]);
					}
				}
			}
		}

		for (i = 0; i < uNumEndPts; i++)
		{
			c[i] = Unquantize(c[i], RGBAPrecWithP);
		}

		uint8_t w1[NUM_PIXELS_PER_BLOCK], w2[NUM_PIXELS_PER_BLOCK];

		// read color indices
		for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
		{
			const size_t uNumBits = IsFixUpOffset(ms_aInfo[uMode].uPartitions, uShape, i) ? uIndexPrec - 1u : uIndexPrec;
			if (uStartBit + uNumBits > 128)
			{
				FillWithErrorColors(pOut);
				return;
			}
			w1[i] = GetBits(uStartBit, uNumBits);
		}

		// read alpha indices
		if (uIndexPrec2)
		{
			for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
			{
				const size_t uNumBits = i ? uIndexPrec2 : uIndexPrec2 - 1u;
				if (uStartBit + uNumBits > 128)
				{
					FillWithErrorColors(pOut);
					return;
				}
				w2[i] = GetBits(uStartBit, uNumBits);
			}
		}

		for (i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
		{
			const uint8_t uRegion = g_aPartitionTable[uPartitions][uShape][i];
			LDRColorA outPixel;
			if (uIndexPrec2 == 0)
			{
				LDRColorA::Interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w1[i], w1[i], uIndexPrec, uIndexPrec, outPixel);
			}
			else
			{
				if (uIndexMode == 0)
				{
					LDRColorA::Interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w1[i], w2[i], uIndexPrec, uIndexPrec2, outPixel);
				}
				else
				{
					LDRColorA::Interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w2[i], w1[i], uIndexPrec2, uIndexPrec, outPixel);
				}
			}

			switch (uRotation)
			{
				case 1: std::swap(outPixel.r, outPixel.a); break;
				case 2: std::swap(outPixel.g, outPixel.a); break;
				case 3: std::swap(outPixel.b, outPixel.a); break;
			}

			pOut[i] = HDRColorA(outPixel);
		}
	}
	else
	{
		// Per the BC7 format spec, we must return transparent black
		memset(static_cast<void*>(pOut), 0, sizeof(HDRColorA) * NUM_PIXELS_PER_BLOCK);
	}
}

// BC6H compression (16 bits per texel)
class D3DX_BC6H : private CBits< 16 >
{
public:
	void Decode( bool bSigned, HDRColorA* pOut) const noexcept;

private:
	enum EField : uint8_t
	{
		NA,
		M,
		D,
		RW,
		RX,
		RY,
		RZ,
		GW,
		GX,
		GY,
		GZ,
		BW,
		BX,
		BY,
		BZ,
	};


	struct ModeDescriptor
	{
		EField m_eField;
		uint8_t   m_uBit;
	};

	struct ModeInfo
	{
		uint8_t uMode;
		uint8_t uPartitions;
		bool bTransformed;
		uint8_t uIndexPrec;
		LDRColorA RGBAPrec[BC6H_MAX_REGIONS][2];
	};

	static int Quantize( int iValue,  int prec,  bool bSigned) noexcept;
	static int Unquantize( int comp,  uint8_t uBitsPerComp,  bool bSigned) noexcept;
	static int FinishUnquantize( int comp,  bool bSigned) noexcept;

private:
	static const ModeDescriptor ms_aDesc[][82];
	static const ModeInfo ms_aInfo[];
	static const int ms_aModeToInfo[];
};

// BC6H Compression
const D3DX_BC6H::ModeDescriptor D3DX_BC6H::ms_aDesc[14][82] =
{
	{   // Mode 1 (0x00) - 10 5 5 5
		{ M, 0}, { M, 1}, {GY, 4}, {BY, 4}, {BZ, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
		{GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
		{BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
		{BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
		{BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
		{ D, 3}, { D, 4},
	},

	{   // Mode 2 (0x01) - 7 6 6 6
		{ M, 0}, { M, 1}, {GY, 5}, {GZ, 4}, {GZ, 5}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {BZ, 0}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {BY, 5}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BZ, 3}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
		{RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
		{GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
		{BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
		{RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
		{ D, 3}, { D, 4},
	},

	{   // Mode 3 (0x02) - 11 5 4 4
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
		{RW,10}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,10},
		{BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,10},
		{BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
		{BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
		{ D, 3}, { D, 4},
	},

	{   // Mode 4 (0x06) - 11 4 5 4
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,10},
		{GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
		{GW,10}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,10},
		{BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {BZ, 0},
		{BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {GY, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
		{ D, 3}, { D, 4},
	},

	{   // Mode 5 (0x0a) - 11 4 4 5
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,10},
		{BY, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,10},
		{BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
		{BW,10}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {BZ, 1},
		{BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {BZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
		{ D, 3}, { D, 4},
	},

	{   // Mode 6 (0x0e) - 9 5 5 5
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
		{GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
		{BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
		{BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
		{BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
		{ D, 3}, { D, 4},
	},

	{   // Mode 7 (0x12) - 8 6 5 5
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {GZ, 4}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {BZ, 3}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
		{RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
		{BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
		{BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
		{RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
		{ D, 3}, { D, 4},
	},

	{   // Mode 8 (0x16) - 8 5 6 5
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {BZ, 0}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {GY, 5}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {GZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
		{GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
		{GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
		{BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
		{BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
		{ D, 3}, { D, 4},
	},

	{   // Mode 9 (0x1a) - 8 5 5 6
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {BY, 5}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
		{GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
		{BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
		{BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
		{BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
		{ D, 3}, { D, 4},
	},

	{   // Mode 10 (0x1e) - 6 6 6 6
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {GZ, 4}, {BZ, 0}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GY, 5}, {BY, 5}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {GZ, 5}, {BZ, 3}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
		{RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
		{GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
		{BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
		{RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
		{ D, 3}, { D, 4},
	},

	{   // Mode 11 (0x03) - 10 10
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
		{RX, 5}, {RX, 6}, {RX, 7}, {RX, 8}, {RX, 9}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
		{GX, 5}, {GX, 6}, {GX, 7}, {GX, 8}, {GX, 9}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
		{BX, 5}, {BX, 6}, {BX, 7}, {BX, 8}, {BX, 9}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
		{NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
		{NA, 0}, {NA, 0},
	},

	{   // Mode 12 (0x07) - 11 9
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
		{RX, 5}, {RX, 6}, {RX, 7}, {RX, 8}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
		{GX, 5}, {GX, 6}, {GX, 7}, {GX, 8}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
		{BX, 5}, {BX, 6}, {BX, 7}, {BX, 8}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
		{NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
		{NA, 0}, {NA, 0},
	},

	{   // Mode 13 (0x0b) - 12 8
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
		{RX, 5}, {RX, 6}, {RX, 7}, {RW,11}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
		{GX, 5}, {GX, 6}, {GX, 7}, {GW,11}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
		{BX, 5}, {BX, 6}, {BX, 7}, {BW,11}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
		{NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
		{NA, 0}, {NA, 0},
	},

	{   // Mode 14 (0x0f) - 16 4
		{ M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
		{RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
		{GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
		{BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,15},
		{RW,14}, {RW,13}, {RW,12}, {RW,11}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,15},
		{GW,14}, {GW,13}, {GW,12}, {GW,11}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,15},
		{BW,14}, {BW,13}, {BW,12}, {BW,11}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
		{NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
		{NA, 0}, {NA, 0},
	},
};

// Mode, Partitions, Transformed, IndexPrec, RGBAPrec
const D3DX_BC6H::ModeInfo D3DX_BC6H::ms_aInfo[] =
{
	{0x00, 1, true,  3, { { LDRColorA(10,10,10,0), LDRColorA(5, 5, 5,0) }, { LDRColorA(5,5,5,0), LDRColorA(5,5,5,0) } } }, // Mode 1
	{0x01, 1, true,  3, { { LDRColorA(7, 7, 7,0), LDRColorA(6, 6, 6,0) }, { LDRColorA(6,6,6,0), LDRColorA(6,6,6,0) } } }, // Mode 2
	{0x02, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA(5, 4, 4,0) }, { LDRColorA(5,4,4,0), LDRColorA(5,4,4,0) } } }, // Mode 3
	{0x06, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA(4, 5, 4,0) }, { LDRColorA(4,5,4,0), LDRColorA(4,5,4,0) } } }, // Mode 4
	{0x0a, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA(4, 4, 5,0) }, { LDRColorA(4,4,5,0), LDRColorA(4,4,5,0) } } }, // Mode 5
	{0x0e, 1, true,  3, { { LDRColorA(9, 9, 9,0), LDRColorA(5, 5, 5,0) }, { LDRColorA(5,5,5,0), LDRColorA(5,5,5,0) } } }, // Mode 6
	{0x12, 1, true,  3, { { LDRColorA(8, 8, 8,0), LDRColorA(6, 5, 5,0) }, { LDRColorA(6,5,5,0), LDRColorA(6,5,5,0) } } }, // Mode 7
	{0x16, 1, true,  3, { { LDRColorA(8, 8, 8,0), LDRColorA(5, 6, 5,0) }, { LDRColorA(5,6,5,0), LDRColorA(5,6,5,0) } } }, // Mode 8
	{0x1a, 1, true,  3, { { LDRColorA(8, 8, 8,0), LDRColorA(5, 5, 6,0) }, { LDRColorA(5,5,6,0), LDRColorA(5,5,6,0) } } }, // Mode 9
	{0x1e, 1, false, 3, { { LDRColorA(6, 6, 6,0), LDRColorA(6, 6, 6,0) }, { LDRColorA(6,6,6,0), LDRColorA(6,6,6,0) } } }, // Mode 10
	{0x03, 0, false, 4, { { LDRColorA(10,10,10,0), LDRColorA(10,10,10,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 11
	{0x07, 0, true,  4, { { LDRColorA(11,11,11,0), LDRColorA(9, 9, 9,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 12
	{0x0b, 0, true,  4, { { LDRColorA(12,12,12,0), LDRColorA(8, 8, 8,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 13
	{0x0f, 0, true,  4, { { LDRColorA(16,16,16,0), LDRColorA(4, 4, 4,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 14
};

const int D3DX_BC6H::ms_aModeToInfo[] =
{
	 0, // Mode 1   - 0x00
	 1, // Mode 2   - 0x01
	 2, // Mode 3   - 0x02
	10, // Mode 11  - 0x03
	-1, // Invalid  - 0x04
	-1, // Invalid  - 0x05
	 3, // Mode 4   - 0x06
	11, // Mode 12  - 0x07
	-1, // Invalid  - 0x08
	-1, // Invalid  - 0x09
	 4, // Mode 5   - 0x0a
	12, // Mode 13  - 0x0b
	-1, // Invalid  - 0x0c
	-1, // Invalid  - 0x0d
	 5, // Mode 6   - 0x0e
	13, // Mode 14  - 0x0f
	-1, // Invalid  - 0x10
	-1, // Invalid  - 0x11
	 6, // Mode 7   - 0x12
	-1, // Reserved - 0x13
	-1, // Invalid  - 0x14
	-1, // Invalid  - 0x15
	 7, // Mode 8   - 0x16
	-1, // Reserved - 0x17
	-1, // Invalid  - 0x18
	-1, // Invalid  - 0x19
	 8, // Mode 9   - 0x1a
	-1, // Reserved - 0x1b
	-1, // Invalid  - 0x1c
	-1, // Invalid  - 0x1d
	 9, // Mode 10  - 0x1e
	-1, // Resreved - 0x1f
};


//-------------------------------------------------------------------------------------
// BC6H Compression
// Reference:https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc6h-format
//-------------------------------------------------------------------------------------
void D3DX_BC6H::Decode(bool bSigned, HDRColorA* pOut) const noexcept
{

	size_t uStartBit = 0;
	uint8_t uMode = GetBits(uStartBit, 2u);
	if (uMode != 0x00 && uMode != 0x01)
	{
		uMode = static_cast<uint8_t>((unsigned(GetBits(uStartBit, 3)) << 2) | uMode);
	}


	if (ms_aModeToInfo[uMode] >= 0)
	{
		const ModeDescriptor* desc = ms_aDesc[ms_aModeToInfo[uMode]];

		const ModeInfo& info = ms_aInfo[ms_aModeToInfo[uMode]];

		INTEndPntPair aEndPts[BC6H_MAX_REGIONS] = {};
		uint32_t uShape = 0;

		// Read header
		//if blocks has no partition bits, it's a single-subset block, else it's a two-subset block
		const size_t uHeaderBits = info.uPartitions > 0 ? 82u : 65u;
		while (uStartBit < uHeaderBits)
		{
			const size_t uCurBit = uStartBit;
			if (GetBit(uStartBit))
			{
				switch (desc[uCurBit].m_eField)
				{
				case D:  uShape |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case RW: aEndPts[0].A.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case RX: aEndPts[0].B.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case RY: aEndPts[1].A.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case RZ: aEndPts[1].B.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case GW: aEndPts[0].A.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case GX: aEndPts[0].B.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case GY: aEndPts[1].A.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case GZ: aEndPts[1].B.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case BW: aEndPts[0].A.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case BX: aEndPts[0].B.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case BY: aEndPts[1].A.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				case BZ: aEndPts[1].B.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
				default:
				{
					FillWithErrorColors(pOut);
					return;
				}
				}
			}
		}

		// Sign extend necessary end points
		if (bSigned)
		{
			aEndPts[0].A.SignExtend(info.RGBAPrec[0][0]);
		}
		if (bSigned || info.bTransformed)
		{
			for (size_t p = 0; p <= info.uPartitions; ++p)
			{
				if (p != 0)
				{
					aEndPts[p].A.SignExtend(info.RGBAPrec[p][0]);
				}
				aEndPts[p].B.SignExtend(info.RGBAPrec[p][1]);
			}
		}

		// Inverse transform the end points
		if (info.bTransformed)
		{
			TransformInverse(aEndPts, info.RGBAPrec[0][0], bSigned);
		}

		// Read indices
		for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
		{
			const size_t uNumBits = IsFixUpOffset(info.uPartitions, uShape, i) ? info.uIndexPrec - 1u : info.uIndexPrec;
			if (uStartBit + uNumBits > 128)
			{
				FillWithErrorColors(pOut);
				return;
			}
			const uint8_t uIndex = GetBits(uStartBit, uNumBits);

			if (uIndex >= ((info.uPartitions > 0) ? 8 : 16))
			{
				FillWithErrorColors(pOut);
				return;
			}

			const size_t uRegion = g_aPartitionTable[info.uPartitions][uShape][i];

			// Unquantize endpoints and interpolate
			const int r1 = Unquantize(aEndPts[uRegion].A.r, info.RGBAPrec[0][0].r, bSigned);
			const int g1 = Unquantize(aEndPts[uRegion].A.g, info.RGBAPrec[0][0].g, bSigned);
			const int b1 = Unquantize(aEndPts[uRegion].A.b, info.RGBAPrec[0][0].b, bSigned);
			const int r2 = Unquantize(aEndPts[uRegion].B.r, info.RGBAPrec[0][0].r, bSigned);
			const int g2 = Unquantize(aEndPts[uRegion].B.g, info.RGBAPrec[0][0].g, bSigned);
			const int b2 = Unquantize(aEndPts[uRegion].B.b, info.RGBAPrec[0][0].b, bSigned);
			const int* aWeights = info.uPartitions > 0 ? g_aWeights3 : g_aWeights4;
			INTColor fc;
			fc.r = FinishUnquantize((r1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + r2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);
			fc.g = FinishUnquantize((g1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + g2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);
			fc.b = FinishUnquantize((b1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + b2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);

			HALF rgb[3];
			fc.ToF16(rgb, bSigned);

			pOut[i].r = HalfToFloat(rgb[0]);
			pOut[i].g = HalfToFloat(rgb[1]);
			pOut[i].b = HalfToFloat(rgb[2]);
			pOut[i].a = 1.0f;
		}
	}
	else
	{
		// Per the BC6H format spec, we must return opaque black
		for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
		{
			pOut[i] = HDRColorA(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}
}


int D3DX_BC6H::Quantize(int iValue, int prec, bool bSigned) noexcept
{
	int q, s = 0;
	if (bSigned)
	{
		if (iValue < 0)
		{
			s = 1;
			iValue = -iValue;
		}
		q = (prec >= 16) ? iValue : (iValue << (prec - 1)) / (F16MAX + 1);
		if (s)
			q = -q;
	}
	else
	{
		q = (prec >= 15) ? iValue : (iValue << prec) / (F16MAX + 1);
	}

	return q;
}


int D3DX_BC6H::Unquantize(int comp, uint8_t uBitsPerComp, bool bSigned) noexcept
{
	int unq = 0, s = 0;
	if (bSigned)
	{
		if (uBitsPerComp >= 16)
		{
			unq = comp;
		}
		else
		{
			if (comp < 0)
			{
				s = 1;
				comp = -comp;
			}

			if (comp == 0) unq = 0;
			else if (comp >= ((1 << (uBitsPerComp - 1)) - 1)) unq = 0x7FFF;
			else unq = ((comp << 15) + 0x4000) >> (uBitsPerComp - 1);

			if (s) unq = -unq;
		}
	}
	else
	{
		if (uBitsPerComp >= 15) unq = comp;
		else if (comp == 0) unq = 0;
		else if (comp == ((1 << uBitsPerComp) - 1)) unq = 0xFFFF;
		else unq = ((comp << 16) + 0x8000) >> uBitsPerComp;
	}

	return unq;
}


int D3DX_BC6H::FinishUnquantize(int comp, bool bSigned) noexcept
{
	if (bSigned)
	{
		return (comp < 0) ? -(((-comp) * 31) >> 5) : (comp * 31) >> 5;  // scale the magnitude by 31/32
	}
	else
	{
		return (comp * 31) >> 6;                                        // scale the magnitude by 31/64
	}
}

void decompress_bc7(uint8_t* out, const uint8_t* in)
{
	VECTOR data[16];
	reinterpret_cast<const D3DX_BC7*>(in)->Decode(reinterpret_cast<HDRColorA*>(data));

	auto pDst = out;
	for (auto i = 0; i < NUM_PIXELS_PER_BLOCK; i++) {
		auto v = VectorAdd(data[i], g_bias);
		StoreUByteN4(pDst, v);
		pDst += 4;
	}
}

void decompress_bc6HU(uint8_t* out, const uint8_t* in)
{
	VECTOR data[16];
	reinterpret_cast<const D3DX_BC6H*>(in)->Decode(false, reinterpret_cast<HDRColorA*>(data));

	auto pDst = out;
	for (auto i = 0; i < NUM_PIXELS_PER_BLOCK; i++) {
		auto v = VectorAdd(data[i], g_bias);
		StoreUByteN4(pDst, v);
		pDst += 4;
	}
}

void decompress_bc6HS(uint8_t* out, const uint8_t* in)
{
	VECTOR data[16];
	reinterpret_cast<const D3DX_BC6H*>(in)->Decode(true, reinterpret_cast<HDRColorA*>(data));

	auto pDst = out;
	for (auto i = 0; i < NUM_PIXELS_PER_BLOCK; i++) {
		StoreByteN4(pDst, data[i]);
		pDst += 4;
	}
	
}