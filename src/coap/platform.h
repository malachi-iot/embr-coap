#pragma once

#include "../platform/assert.h"
#include "../mc/opts-internal.h"
#include "internal/buffer.h"

#define FEATURE_MCCOAP_HEADER_LEGACY    0
#define FEATURE_MCCOAP_HEADER_CXX_03    1
#define FEATURE_MCCOAP_HEADER_CXX_11    2

#ifndef FEATURE_MCCOAP_HEADER_VERSION
#define FEATURE_MCCOAP_HEADER_VERSION   FEATURE_MCCOAP_HEADER_LEGACY
#endif