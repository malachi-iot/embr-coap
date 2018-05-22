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
#include "../coap-features.h"

// enables retry/reliable message capabilities.  Generally we want this,
// but for extremely constrained environments one might eschew this feature
// NOTE: eventually architecture goal is that datapump and retry code will
// be isolated enough from one another such that this flag won't be necessary
//#define FEATURE_MCCOAP_RELIABLE

// enables subject/observer pattern for outgoing transmit packets
#define FEATURE_MCCOAP_DATAPUMP_OBSERVABLE

// Do not enable features/options here, they are presented only for reference

// utilize inline TNetBuf instead of pointer
// NOTE: pointer is useful to avoid construction/destruction on non-active Items in the queue
//#define FEATURE_MCCOAP_DATAPUMP_INLINE

// +++ coap-dispatcher related:

// Doesn't really work, but will let us compile
// Expect memory corruption errors while using this -
// Specifically we get a problem during "CoAP dispatcher tests"::"Dispatcher Factory"
//#define FEATURE_MCCOAP_LEGACY_PREOBJSTACK

// This adds 8 bytes to IDispatcherHandler class when enabled
//#define FEATURE_IISINTERESTED

// This adds quite a few bytes for an IMessageObserver since each IObserver variant has its own
// vtable pointer
//#define FEATURE_DISCRETE_OBSERVERS

//#define FEATURE_MCCOAP_RESERVED_DISPATCHER

// ---
