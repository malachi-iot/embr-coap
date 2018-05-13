#pragma once

// only for observing class/method signatures, no expectation of functional code
namespace experimental { namespace prototype {

class DecoderObserver;

class URI
{
public:
    URI(const char*, DecoderObserver&) {}
};


class DecoderObserver
{
public:
    template <class TArray>
    DecoderObserver(TArray array) {}

    DecoderObserver() {}
};


}}
