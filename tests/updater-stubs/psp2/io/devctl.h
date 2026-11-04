#pragma once
#include <cstdint>
struct SceIoDevInfo { int64_t max_size, free_size; uint32_t cluster_size; void* unk; };
int sceIoDevctl(const char*, int, void*, unsigned, void*, unsigned);
