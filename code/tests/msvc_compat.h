#pragma once

// Upstream uses GCC's packed attribute for its wire structs. For host tests,
// MSVC needs equivalent packing before the header is first included.
#ifdef _MSC_VER
#define __attribute__(attributes)
#pragma pack(push, 1)
#include "opendroneid.h"
#pragma pack(pop)
#endif
