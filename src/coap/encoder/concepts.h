#pragma once

#include "../internal/raw/header.h"

#if __cpp_concepts
namespace embr { namespace coap { namespace concepts { inline namespace encoder { inline namespace v1 {

template <class T>
concept StreambufEncoder = requires(T e)
{
    e.payload();
};

}}}}}
#endif