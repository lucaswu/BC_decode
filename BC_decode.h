#pragma once

#include <stdint.h>

//Reference: 
// 1. https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html#s3tc_bc1_noalpha
// 2. https://docs.microsoft.com/zh-cn/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression?redirectedfrom=MSDN
// 3. https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc6h-format
// 4. https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc7-format

void decompress_bc1(uint8_t* out, const uint8_t* in);
void decompress_bc2(uint8_t* out, const uint8_t* in);
void decompress_bc3(uint8_t* out, const uint8_t* in);
void decompress_bc4U(uint8_t* out, const uint8_t* in);
void decompress_bc4S(uint8_t* out, const uint8_t* in);
void decompress_bc5U(uint8_t* out, const uint8_t* in);
void decompress_bc5S(uint8_t* out, const uint8_t* in);
void decompress_bc6HU(uint8_t* out, const uint8_t* in);
void decompress_bc6HS(uint8_t* out, const uint8_t* in);
void decompress_bc7(uint8_t* out, const uint8_t* in);



