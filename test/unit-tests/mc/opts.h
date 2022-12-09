//#include_next <mc/opts.h>
#include <estd/internal/platform.h>

//#define FEATURE_MCCOAP_RELIABLE

//#if __cplusplus >= 201103L
#if defined(FEATURE_CPP_MOVESEMANTIC)
//#define FEATURE_EMBR_DATAPUMP_INLINE
#endif

// uses move semantic so that someone always owns the netbuf, somewhat
// sidestepping shared_ptr/ref counters (even though implementations
// like PBUF have inbuilt ref counters)
// 09DEC22 Only applies to pre-v4-retry code and relies on obsolete datapump
//#define FEATURE_MCCOAP_RETRY_INLINE
