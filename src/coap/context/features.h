#pragma once

// DEBT: I think GCC gets mad when we use 'features.h' filename, so be careful
// if that ISN'T the case, briefly document that to eliminate debt

// Some compilers will "reach forward" with warnings to allow us to compile
// anyway, but technically we really need c++20 features at the moment for
// subcontext support
#ifndef FEATURE_EMBR_COAP_SUBCONTEXT
#if __cpp_generic_lambdas >= 201707L
#define FEATURE_EMBR_COAP_SUBCONTEXT 1
#else
#define FEATURE_EMBR_COAP_SUBCONTEXT 0
#endif
#endif

#define FEATURE_MCCOAP_SUBCONTEXT FEATURE_EMBR_COAP_SUBCONTEXT
