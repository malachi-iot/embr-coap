#pragma once

// TODO: find "most" standardized version of this
#ifdef __CPP11__
#define OVERRIDE override
#define CONSTEXPR constexpr
#define NULLPTR nullptr
#else
#define OVERRIDE
#define CONSTEXPR const
#define NULLPTR NULL
#endif

#if __ADSPBLACKFIN__ || defined(_MSC_VER)
#define PACKED
#define USE_PRAGMA_PACK
#else
#define PACKED __attribute__ ((packed))
#endif

// TODO: Make this generate log warnings or something
#define ASSERT(expected, actual)

// This will generate warnings but the program will keep going
#define ASSERT_WARN(expected, actual, message)
// This will generate a warning and issue a 'return' statement immediately
#define ASSERT_ABORT(expected, actual, message)
// This will halt the program with a message
#define ASSERT_ERROR(expected, actual, message)

