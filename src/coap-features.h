//
// Created by malachi on 2/23/18.
//

#pragma once


// TODO: Also platform.h interacts with feature flags, need to clean all
// that up and consolidate

// output informational/instrumentation oriented data
#define FEATURE_MCCOAP_PROFILING

// output errors when boundary checks fail
#define FEATURE_MCCOAP_BOUNDS_CHECK

// If defined, use std::clog and std::cerr for outputs, otherwise
// use printf
#define FEATURE_MCCOAP_CLOG

// If defined, IncomingContext itself is manager of message token
// memory.  It also disables token pool within ContextDispatcherHandler
// NOTE: Has a number of problems, not ready yet
#define FEATURE_MCCOAP_INLINE_TOKEN

// If defined, IMessageObserver has an on_complete message too
// Experimental right now, but likely will be a committed feature
#define FEATURE_MCCOAP_COMPLETE_OBSERVER

// Enable/disable legacy netbuf encoder
#ifndef FEATURE_MCCOAP_NETBUF_ENCODER
#define FEATURE_MCCOAP_NETBUF_ENCODER 0
#endif
