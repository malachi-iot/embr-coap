#pragma once

#include "../platform/assert.h"
#include "../mc/opts-internal.h"
#include "internal/buffer.h"
#include "context/features.h"

// DEBT: Rename (while retaining compat with old names) to FEATURE_EMBR_COAP_xxx
// DEBT: Move these out to a particular features file

#define FEATURE_MCCOAP_HEADER_LEGACY    0
#define FEATURE_MCCOAP_HEADER_CXX_03    1
#define FEATURE_MCCOAP_HEADER_CXX_11    2

#ifndef FEATURE_MCCOAP_HEADER_VERSION
#define FEATURE_MCCOAP_HEADER_VERSION   FEATURE_MCCOAP_HEADER_LEGACY
#endif

#ifndef FEATURE_MCCOAP_HEADER_LEGACY_COMPILE
#define FEATURE_MCCOAP_HEADER_LEGACY_COMPILE 1
#endif