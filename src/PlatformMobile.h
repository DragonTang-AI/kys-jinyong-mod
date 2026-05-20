#pragma once

#if defined(__ANDROID__)
#define KYS_MOBILE_EXTRACT_ASSETS 1
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#define KYS_MOBILE_EXTRACT_ASSETS 1
#endif
#endif
