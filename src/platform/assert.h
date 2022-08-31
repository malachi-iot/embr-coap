// Intermediate ASSERT stand in for old moducom-memory code.  Yanked in
// directly from there

#include <estd/internal/platform.h>

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
// TODO: Identify a better way to identify presence of C++ std::cerr
#if __ADSPBLACKFIN__
// blackfin doesn't have cerr, and probably others don't also
#define MCMEM_CERR  ::std::cout
#else
#define MCMEM_CERR  ::std::cerr
#endif
#define FEATURE_MCMEM_ASSERT_ENABLE 1
#endif


#if FEATURE_MCMEM_ASSERT_ENABLE
#include <iostream>
#define ASSERT(expected, actual) if((expected) != (actual)) \
{ MCMEM_CERR << "ASSERT: (" << __func__ << ") " << ::std::endl; }
// This will generate warnings but the program will keep going
#define ASSERT_WARN(expected, actual, message) if((expected) != (actual)) \
{ MCMEM_CERR << "WARN: (" << __func__ << ") " << message << ::std::endl; }
// This will generate a warning and issue a 'return' statement immediately
#define ASSERT_ABORT(expected, actual, message)
// This will halt the program with a message
// One may *carefully* use << syntax within message
#define ASSERT_ERROR(expected, actual, message) if((expected) != (actual)) \
{ MCMEM_CERR << "ERROR: (" << __func__ << ") " << message << ::std::endl; }
#else
#define ASSERT(expected, actual)
// This will generate warnings but the program will keep going
#define ASSERT_WARN(expected, actual, message)
// This will generate a warning and issue a 'return' statement immediately
#define ASSERT_ABORT(expected, actual, message)
// This will halt the program with a message
#define ASSERT_ERROR(expected, actual, message)
#endif
