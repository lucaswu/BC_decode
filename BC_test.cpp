#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include "BC_decode.h"
#include <string>
#include <algorithm>
#include "BC_define.h"

const int BLOCK_SIZE = 4;
const int width = 1000;
const int height = 784;

void testBC1()
{
	const int srcSize = 784000/2;
	uint8_t* pSrc = new uint8_t[srcSize];
	uint8_t* pDst = new uint8_t[width * height * 4];
#if DAMON_ANDROID
	FILE* fp = fopen("/sdcard/data/1000x784_392000_bc1_input.rgba", "rb");
#else
	FILE* fp = fopen("1000x784_392000_bc1_input.rgba", "rb");
#endif
	if (fp == nullptr) {
		printf("open 1000x784_392000_bc1_input.rgba file error!\n");
		return;
	}

	fread(pSrc, sizeof(uint8_t), srcSize, fp);
	fclose(fp);


	auto ptr = pSrc;
	auto output = pDst;
	for (uint32_t block_y = 0; block_y < height / 4; ++block_y) {
		for (uint32_t block_x = 0; block_x < width / 4; ++block_x) {
			uint8_t block[64];
			decompress_bc1(block, ptr);
			ptr += 8;
			for (uint32_t y = 0; y < BLOCK_SIZE; ++y) {
				for (uint32_t x = 0; x < BLOCK_SIZE; ++x) {

					const uint32_t linear_y = block_y * BLOCK_SIZE + y;
					const uint32_t linear_x = block_x * BLOCK_SIZE + x;

					const uint32_t offset_y = linear_y * width;
					const uint32_t offset_x = linear_x;
					const uint32_t output_offset = (offset_y + offset_x) * 4ULL;
					uint8_t* ptr2 = block + (y * 4 + x) * 4;
					output[output_offset + 0] = ptr2[0];
					output[output_offset + 1] = ptr2[1];
					output[output_offset + 2] = ptr2[2];
					output[output_offset + 3] = ptr2[3];
				}
			}
		}
	}
#if DAMON_ANDROID
	std::string name = std::string("/sdcard/data/output/") + std::to_string(width) + "x" + std::to_string(height) + "_android_bc1_out.rgba";
#else
	std::string name = std::to_string(width) + "x" + std::to_string(height) + "_x86_bc1_out.rgba";
#endif
	remove(name.c_str());
	fp = fopen(name.c_str(), "wb");
	fwrite(pDst, sizeof(uint8_t), width * height * 4, fp);
	fclose(fp);
}

void testBC2()
{
	const int srcSize = 784000;
	uint8_t* pSrc = new uint8_t[srcSize];
	uint8_t* pDst = new uint8_t[width * height * 4];

#if DAMON_ANDROID
	FILE* fp = fopen("/sdcard/data/input/1000x784_784000_bc2_input.rgba", "rb");
#else
	FILE* fp = fopen("1000x784_784000_bc2_input.rgba", "rb");
#endif
	if (fp == nullptr) {
		printf("open 1000x784_784000_bc2_input.rgba file error!\n");
		return;
	}

	fread(pSrc, sizeof(uint8_t), srcSize, fp);
	fclose(fp);


	auto ptr = pSrc;
	auto output = pDst;
	for (uint32_t block_y = 0; block_y < height / 4; ++block_y) {
		for (uint32_t block_x = 0; block_x < width / 4; ++block_x) {
			uint8_t block[64];
			decompress_bc2(block, ptr);
			ptr += 16;
			for (uint32_t y = 0; y < BLOCK_SIZE; ++y) {
				for (uint32_t x = 0; x < BLOCK_SIZE; ++x) {

					const uint32_t linear_y = block_y * BLOCK_SIZE + y;
					const uint32_t linear_x = block_x * BLOCK_SIZE + x;

					const uint32_t offset_y = linear_y * width;
					const uint32_t offset_x = linear_x;
					const uint32_t output_offset = (offset_y + offset_x) * 4ULL;
					uint8_t* ptr2 = block + (y * 4 + x) * 4;
					output[output_offset + 0] = ptr2[0];
					output[output_offset + 1] = ptr2[1];
					output[output_offset + 2] = ptr2[2];
					output[output_offset + 3] = ptr2[3];
				}
			}
		}
	}
#if DAMON_ANDROID
	std::string name = std::string("/sdcard/data/output") + std::to_string(width) + "x" + std::to_string(height) + "_android_bc2_out.rgba";
#else
	std::string name = std::to_string(width) + "x" + std::to_string(height) + "_x86_bc2_out.rgba";
#endif
	remove(name.c_str());
	fp = fopen(name.c_str(), "wb");
	fwrite(pDst, sizeof(uint8_t), width * height * 4, fp);
	fclose(fp);
}

void testBC3()
{
	const int srcSize = 784000;
	uint8_t* pSrc = new uint8_t[srcSize];
	uint8_t* pDst = new uint8_t[width * height * 4];
#if DAMON_ANDROID
	FILE* fp = fopen("/sdcard/data/input/1000x784_784000_bc3_input.rgba", "rb");
#else
	FILE* fp = fopen("1000x784_784000_bc3_input.rgba", "rb");
#endif
	if (fp == nullptr) {
		printf("open 1000x784_784000_bc3_input.rgba file error!\n");
		return;
	}

	fread(pSrc, sizeof(uint8_t), srcSize, fp);
	fclose(fp);


	auto ptr = pSrc;
	auto output = pDst;
	for (uint32_t block_y = 0; block_y < height / 4; ++block_y) {
		for (uint32_t block_x = 0; block_x < width / 4; ++block_x) {
			uint8_t block[64];
			decompress_bc3(block, ptr);
			ptr += 16;
			for (uint32_t y = 0; y < BLOCK_SIZE; ++y) {
				for (uint32_t x = 0; x < BLOCK_SIZE; ++x) {

					const uint32_t linear_y = block_y * BLOCK_SIZE + y;
					const uint32_t linear_x = block_x * BLOCK_SIZE + x;

					const uint32_t offset_y = linear_y * width;
					const uint32_t offset_x = linear_x;
					const uint32_t output_offset = (offset_y + offset_x) * 4ULL;
					uint8_t* ptr2 = block + (y * 4 + x) * 4;
					output[output_offset + 0] = ptr2[0];
					output[output_offset + 1] = ptr2[1];
					output[output_offset + 2] = ptr2[2];
					output[output_offset + 3] = ptr2[3];
				}
			}
		}
	}
#if DAMON_ANDROID
	std::string name = std::string("/sdcard/data/output") + std::to_string(width) + "x" + std::to_string(height) + "_android_bc3_out.rgba";
#else
	std::string name = std::to_string(width) + "x" + std::to_string(height) + "_x86_bc3_out.rgba";
#endif
	remove(name.c_str());
	fp = fopen(name.c_str(), "wb");
	fwrite(pDst, sizeof(uint8_t), width * height * 4, fp);
	fclose(fp);
}

void testBC4U()
{
	const int srcSize = 784000/2;
	uint8_t* pSrc = new uint8_t[srcSize];
	uint8_t* pDst = new uint8_t[width * height * 4];
#if DAMON_ANDROID
	FILE* fp = fopen("/sdcard/data/input/1000x784_392000_bc4U_input.rgba", "rb");
#else
	FILE* fp = fopen("1000x784_392000_bc4U_input.rgba", "rb");
#endif
	if (fp == nullptr) {
		printf("open 1000x784_392000_bc4U_input.rgba file error!\n");
		return;
	}

	fread(pSrc, sizeof(uint8_t), srcSize, fp);
	fclose(fp);


	auto ptr = pSrc;
	auto output = pDst;
	for (uint32_t block_y = 0; block_y < height / 4; ++block_y) {
		for (uint32_t block_x = 0; block_x < width / 4; ++block_x) {
			uint8_t block[64] = { 0 };
			decompress_bc4U(block, ptr);
			ptr += 8;
			for (uint32_t y = 0; y < BLOCK_SIZE; ++y) {
				for (uint32_t x = 0; x < BLOCK_SIZE; ++x) {

					const uint32_t linear_y = block_y * BLOCK_SIZE + y;
					const uint32_t linear_x = block_x * BLOCK_SIZE + x;

					const uint32_t offset_y = linear_y * width;
					const uint32_t offset_x = linear_x;
					const uint32_t output_offset = (offset_y + offset_x) * 4ULL;
					uint8_t* ptr2 = block + (y * 4 + x) * 4;
					output[output_offset + 0] = ptr2[0];
					output[output_offset + 1] = ptr2[1];
					output[output_offset + 2] = ptr2[2];
					output[output_offset + 3] = ptr2[3];

				}
			}
		}
	}

#if DAMON_ANDROID
	std::string name = std::string("/sdcard/data/output/") + std::to_string(width) + "x" + std::to_string(height) + "_android_bc4U_out.rgba";
#else
	std::string name = std::to_string(width) + "x" + std::to_string(height) + "_x86_bc4U_out.rgba";
#endif
	remove(name.c_str());
	fp = fopen(name.c_str(), "wb");
	fwrite(pDst, sizeof(uint8_t), width * height * 4, fp);
	fclose(fp);
}

void testBC4S()
{
	const int srcSize = 784000/2;
	uint8_t* pSrc = new uint8_t[srcSize];
	uint8_t* pDst = new uint8_t[width * height * 4];
#if DAMON_ANDROID
	FILE* fp = fopen("/sdcard/data/input/1000x784_392000_bc4S_input.rgba", "rb");
#else
	FILE* fp = fopen("1000x784_392000_bc4S_input.rgba", "rb");
#endif
	if (fp == nullptr) {
		printf("open 1000x784_392000_bc4S_input.rgba file error!\n");
		return;
}

	fread(pSrc, sizeof(uint8_t), srcSize, fp);
	fclose(fp);


	auto ptr = pSrc;
	auto output = pDst;
	for (uint32_t block_y = 0; block_y < height / 4; ++block_y) {
		for (uint32_t block_x = 0; block_x < width / 4; ++block_x) {
			uint8_t block[64] = {0};
			decompress_bc4S(block, ptr);
			ptr += 8;
			for (uint32_t y = 0; y < BLOCK_SIZE; ++y) {
				for (uint32_t x = 0; x < BLOCK_SIZE; ++x) {

					const uint32_t linear_y = block_y * BLOCK_SIZE + y;
					const uint32_t linear_x = block_x * BLOCK_SIZE + x;

					const uint32_t offset_y = linear_y * width;
					const uint32_t offset_x = linear_x;
					const uint32_t output_offset = (offset_y + offset_x) * 4ULL;
					int8_t* ptr2 = (int8_t*)block + (y * 4 + x) * 4;
					output[output_offset + 0] = std::clamp(ptr2[0] + 127, 0, 255);
					output[output_offset + 1] = std::clamp(ptr2[1] + 127, 0, 255);
					output[output_offset + 2] = std::clamp(ptr2[2] + 127, 0, 255);
					output[output_offset + 3] = std::clamp(ptr2[3] + 127, 0, 255);
				}
			}
		}
	}
#if DAMON_ANDROID
	std::string name = std::string("/sdcard/data/output/") + std::to_string(width) + "x" + std::to_string(height) + "_android_bc4S_out.rgba";
#else
	std::string name = std::to_string(width) + "x" + std::to_string(height) + "_x86_bc4S_out.rgba";
#endif
	remove(name.c_str());
	fp = fopen(name.c_str(), "wb");
	fwrite(pDst, sizeof(uint8_t), width * height * 4, fp);
	fclose(fp);
}

void testBC5U()
{
	const int srcSize = 784000;
	uint8_t* pSrc = new uint8_t[srcSize];
	uint8_t* pDst = new uint8_t[width * height * 4];
#if DAMON_ANDROID
	FILE* fp = fopen("/sdcard/data/input/1000x784_784000_bc5U_input.rgba", "rb");
#else
	FILE* fp = fopen("1000x784_784000_bc5U_input.rgba", "rb");
#endif
	if (fp == nullptr) {
		printf("open 1000x784_784000_bc5U_input.rgba file error!\n");
		return;
}

	fread(pSrc, sizeof(uint8_t), srcSize, fp);
	fclose(fp);


	auto ptr = pSrc;
	auto output = pDst;
	for (uint32_t block_y = 0; block_y < height / 4; ++block_y) {
		for (uint32_t block_x = 0; block_x < width / 4; ++block_x) {
			uint8_t block[64];
			decompress_bc5U(block, ptr);
			ptr += 16;
			for (uint32_t y = 0; y < BLOCK_SIZE; ++y) {
				for (uint32_t x = 0; x < BLOCK_SIZE; ++x) {

					const uint32_t linear_y = block_y * BLOCK_SIZE + y;
					const uint32_t linear_x = block_x * BLOCK_SIZE + x;

					const uint32_t offset_y = linear_y * width;
					const uint32_t offset_x = linear_x;
					const uint32_t output_offset = (offset_y + offset_x) * 4ULL;
					uint8_t* ptr2 = block + (y * 4 + x) * 4;
					output[output_offset + 0] = ptr2[0];
					output[output_offset + 1] = ptr2[1];
					output[output_offset + 2] = ptr2[2];
					output[output_offset + 3] = ptr2[3];
				}
			}
		}
	}
#if DAMON_ANDROID
	std::string name = std::string("/sdcard/data/output/") + std::to_string(width) + "x" + std::to_string(height) + "_android_bc5U_out.rgba";
#else
	std::string name = std::to_string(width) + "x" + std::to_string(height) + "_x86_bc5U_out.rgba";
#endif
	remove(name.c_str());
	fp = fopen(name.c_str(), "wb");
	fwrite(pDst, sizeof(uint8_t), width * height * 4, fp);
	fclose(fp);
}

void testBC5S()
{
	const int srcSize = 784000;
	uint8_t* pSrc = new uint8_t[srcSize];
	uint8_t* pDst = new uint8_t[width * height * 4];
#if DAMON_ANDROID
	FILE* fp = fopen("/sdcard/data/input/1000x784_784000_bc5S_input.rgba", "rb");
#else
	FILE* fp = fopen("1000x784_784000_bc5S_input.rgba", "rb");
#endif
	if (fp == nullptr) {
		printf("open 1000x784_784000_bc5S_input.rgba file error!\n");
		return;
	}

	fread(pSrc, sizeof(uint8_t), srcSize, fp);
	fclose(fp);


	auto ptr = pSrc;
	auto output = pDst;
	for (uint32_t block_y = 0; block_y < height / 4; ++block_y) {
		for (uint32_t block_x = 0; block_x < width / 4; ++block_x) {
			uint8_t block[64];
			decompress_bc5S(block, ptr);
			ptr += 16;
			for (uint32_t y = 0; y < BLOCK_SIZE; ++y) {
				for (uint32_t x = 0; x < BLOCK_SIZE; ++x) {

					const uint32_t linear_y = block_y * BLOCK_SIZE + y;
					const uint32_t linear_x = block_x * BLOCK_SIZE + x;

					const uint32_t offset_y = linear_y * width;
					const uint32_t offset_x = linear_x;
					const uint32_t output_offset = (offset_y + offset_x) * 4ULL;
					int8_t* ptr2 = (int8_t*)block + (y * 4 + x) * 4;
					output[output_offset + 0] = std::clamp(ptr2[0] + 127, 0, 255);
					output[output_offset + 1] = std::clamp(ptr2[1] + 127, 0, 255);
					output[output_offset + 2] = std::clamp(ptr2[2] + 127, 0, 255);
					output[output_offset + 3] = std::clamp(ptr2[3] + 127, 0, 255);

				}
			}
		}
	}

#if DAMON_ANDROID
	std::string name = std::string("/sdcard/data/output/") + std::to_string(width) + "x" + std::to_string(height) + "_android_bc5S_out.rgba";
#else
	std::string name = std::to_string(width) + "x" + std::to_string(height) + "_x86_bc5S_out.rgba";
#endif
	remove(name.c_str());
	fp = fopen(name.c_str(), "wb");
	fwrite(pDst, sizeof(uint8_t), width * height * 4, fp);
	fclose(fp);
}

void testBC6U()
{
	const int srcSize = 784000;
	uint8_t* pSrc = new uint8_t[srcSize];
	uint8_t* pDst = new uint8_t[width * height * 4];
#if DAMON_ANDROID
	FILE* fp = fopen("/sdcard/data/input/1000x784_784000_bc6U_input.rgba", "rb");
#else
	FILE* fp = fopen("1000x784_784000_bc6U_input.rgba", "rb");
#endif
	if (fp == nullptr) {
		printf("open 1000x784_784000_bc6U_input.rgba file error!\n");
		return;
	}

	fread(pSrc, sizeof(uint8_t), srcSize, fp);
	fclose(fp);


	auto ptr = pSrc;
	auto output = pDst;
	for (uint32_t block_y = 0; block_y < height / 4; ++block_y) {
		for (uint32_t block_x = 0; block_x < width / 4; ++block_x) {
			uint8_t block[64];
			decompress_bc6HU(block, ptr);
			ptr += 16;
			for (uint32_t y = 0; y < BLOCK_SIZE; ++y) {
				for (uint32_t x = 0; x < BLOCK_SIZE; ++x) {

					const uint32_t linear_y = block_y * BLOCK_SIZE + y;
					const uint32_t linear_x = block_x * BLOCK_SIZE + x;

					const uint32_t offset_y = linear_y * width;
					const uint32_t offset_x = linear_x;
					const uint32_t output_offset = (offset_y + offset_x) * 4ULL;
					uint8_t* ptr2 = block + (y * 4 + x) * 4;
					output[output_offset + 0] = ptr2[0];
					output[output_offset + 1] = ptr2[1];
					output[output_offset + 2] = ptr2[2];
					output[output_offset + 3] = ptr2[3];
				}
			}
		}
	}
#if DAMON_ANDROID
	std::string name = std::string("/sdcard/data/input") + std::to_string(width) + "x" + std::to_string(height) + "_android_bc6U_out.rgba";
#else
	std::string name = std::to_string(width) + "x" + std::to_string(height) + "_x86_bc6U_out.rgba";
#endif
	remove(name.c_str());
	fp = fopen(name.c_str(), "wb");
	fwrite(pDst, sizeof(uint8_t), width * height * 4, fp);
	fclose(fp);
}

void testBC6S()
{
	const int srcSize = 784000;
	uint8_t* pSrc = new uint8_t[srcSize];
	uint8_t* pDst = new uint8_t[width * height * 4];
#if DAMON_ANDROID
	FILE* fp = fopen("/sdcard/data/input/1000x784_784000_bc6S_input.rgba", "rb");
#else
	FILE* fp = fopen("1000x784_784000_bc6HS-2_input.rgba", "rb");
#endif
	if (fp == nullptr) {
		printf("open 1000x784_784000_bc6S_input.rgba file error!\n");
		return;
	}

	fread(pSrc, sizeof(uint8_t), srcSize, fp);
	fclose(fp);


	auto ptr = pSrc;
	auto output = pDst;
	for (uint32_t block_y = 0; block_y < height / 4; ++block_y) {
		for (uint32_t block_x = 0; block_x < width / 4; ++block_x) {
			uint8_t block[64];
			decompress_bc6HS(block, ptr);
			ptr += 16;
			for (uint32_t y = 0; y < BLOCK_SIZE; ++y) {
				for (uint32_t x = 0; x < BLOCK_SIZE; ++x) {

					const uint32_t linear_y = block_y * BLOCK_SIZE + y;
					const uint32_t linear_x = block_x * BLOCK_SIZE + x;

					const uint32_t offset_y = linear_y * width;
					const uint32_t offset_x = linear_x;
					const uint32_t output_offset = (offset_y + offset_x) * 4ULL;

					int8_t* ptr2 = (int8_t*)block + (y * 4 + x) * 4;
					output[output_offset + 0] = std::clamp(ptr2[0] + 128, 0, 255);
					output[output_offset + 1] = std::clamp(ptr2[1] + 128, 0, 255);
					output[output_offset + 2] = std::clamp(ptr2[2] + 128, 0, 255);
					output[output_offset + 3] = std::clamp(ptr2[3] + 128, 0, 255);
				}
			}
		}
	}
#if DAMON_ANDROID
	std::string name = std::string("/sdcard/data/output/") + std::to_string(width) + "x" + std::to_string(height) + "_android_bc6S_out.rgba";
#else
	std::string name = std::to_string(width) + "x" + std::to_string(height) + "_x86_bc6S_out.rgba";
#endif
	remove(name.c_str());
	fp = fopen(name.c_str(), "wb");
	fwrite(pDst, sizeof(uint8_t), width * height * 4, fp);
	fclose(fp);
}

void testBC7()
{
	const int srcSize = 784000;
	uint8_t* pSrc = new uint8_t[srcSize];
	uint8_t* pDst = new uint8_t[width * height * 4];
#if DAMON_ANDROID
	FILE* fp = fopen("/sdcard/data/input/1000x784_784000_bc7_input.rgba", "rb");
#else
	FILE* fp = fopen("1000x784_784000_bc7_input.rgba", "rb");
#endif
	if (fp == nullptr) {
		printf("open 1000x784_784000_bc7_input.rgba file error!\n");
		return;
	}

	fread(pSrc, sizeof(uint8_t), srcSize, fp);
	fclose(fp);


	auto ptr = pSrc;
	auto output = pDst;
	for (uint32_t block_y = 0; block_y < height / 4; ++block_y) {
		for (uint32_t block_x = 0; block_x < width / 4; ++block_x) {
			uint8_t block[64];
			decompress_bc7(block, ptr);
			ptr += 16;
			for (uint32_t y = 0; y < BLOCK_SIZE; ++y) {
				for (uint32_t x = 0; x < BLOCK_SIZE; ++x) {

					const uint32_t linear_y = block_y * BLOCK_SIZE + y;
					const uint32_t linear_x = block_x * BLOCK_SIZE + x;

					const uint32_t offset_y = linear_y * width;
					const uint32_t offset_x = linear_x;
					const uint32_t output_offset = (offset_y + offset_x) * 4ULL;
					uint8_t* ptr2 = block + (y * 4 + x) * 4;
					output[output_offset + 0] = ptr2[0];
					output[output_offset + 1] = ptr2[1];
					output[output_offset + 2] = ptr2[2];
					output[output_offset + 3] = ptr2[3];
				}
			}
		}
	}
#if DAMON_ANDROID
	std::string name = std::string("/sdcard/data/output/") + std::to_string(width) + "x" + std::to_string(height) + "_android_bc7_out.rgba";
#else
	std::string name = std::to_string(width) + "x" + std::to_string(height) + "_x86_bc7_out.rgba";
#endif
	remove(name.c_str());
	fp = fopen(name.c_str(), "wb");
	fwrite(pDst, sizeof(uint8_t), width * height * 4, fp);
	fclose(fp);
}

#if !DAMON_ANDROID
int main()
{
	testBC1();
	testBC2();
	testBC3();
	testBC4U();
	testBC4S();
	testBC5U();
	testBC5S();
	testBC6S();
	testBC6U();
	testBC7();
	return 0;
}
#endif
