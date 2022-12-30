#pragma once

#include "../platform.h"

#if FEATURE_MCCOAP_HEADER_LEGACY_COMPILE
#include "header/legacy.h"
#endif
#include "header/c++03.h"
#if __cplusplus >= 201103L
#include "header/c++11.h"
#endif
