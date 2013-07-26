#pragma once

#define __LINUX      1
#define __MICROBLAZE 2

#ifdef HARDWARE_ID
#define PLATFORM  __MICROBLAZE
#else
#define PLATFORM  __LINUX
#endif

#if PLATFORM == __LINUX
#include "mirdef-lnx2.h"
#elif PLATFORM == __MICROBLAZE
#include "mirdef-mblaze.h"
#endif
