# CoAP library for embedded targets

It may sound redundant to indicate this is a CoAP library for *embedded* targets, but 
it is my finding most CoAP libraries make curious and liberal use of dynamic allocation.
Last I checked, dynamic allocation was regarded as a practice to be avoided in most
embedded scenarios.  As such, I've developed this library which very specifically avoids
such shenanigans.

## library.json

This file is specifically to assist the very excellent platform.io tooling in utilizing
this lib.  Note the explicit inclusion of ext/estdlib/src and ext/moducom-memory/src. 
I haven't quite mastered platform.io's library depdendency techniques yet, so pio is
unable to (currently) resolve those paths without those helpers

## General Concepts

This library is taking a while to shape up, but here are some items which are getting pretty solid:

### coap::Decoder

A state machine which shepherds 3 sub-state machines: HeaderDecoder, OptionDecoder and TokenDecoder.  coap::Decoder via its state() call will indicate where it's at in that process.

Being memory efficient, only one mode is active at a time and only the last-decoded item is available at any given time.  What you do with that is up to you.  

#### constituent parts

HeaderDecoder and TokenDecoder are rather basic, while OptionDecoder takes on a little more responsibility.

https://github.com/moducom/mc-coap/wiki/Option-Decoder