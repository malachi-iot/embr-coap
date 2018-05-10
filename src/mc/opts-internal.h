#pragma once

// If you aren't interested in providing an mc/opts file, that's OK
// set the FEATURE_MC_INHIBIT_OPTS compiler flag
//#ifndef FEATURE_MC_INHIBIT_OPTS

// experimenting with our dummy ambient opts file.  It's a little flaky, any cached compiler output
// can give false positives (won't pick up user opts.h sometimes).  However, once a clean compile
// is done with a user opts.h present, I expect it will pick it up reliably after that
#include <mc/opts.h>
//#endif

// TODO: phase this into mc/coap-opts.h
#include <coap-features.h>

// Do not enable features/options here, they are presented only for reference

// utilize inline TNetBuf instead of pointer
// NOTE: pointer is useful to avoid construction/destruction on non-active Items in the queue
//#define FEATURE_MCCOAP_DATAPUMP_INLINE

