// FIX: Reaches out to mc/mem/platform.h - however that's strongly not desired
// Additionally, it's broken for Blackfin and the ASSERT code I'm not sure where
// to put it, so in the short term patch it up for modern use.  Code disabled 
// until that's done
#include <coap/header.h>