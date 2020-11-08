//
// Created by Malachi Burke on 12/26/17.
//

#include "coap-dispatcher.h"
#include "coap/context.h"
// One day subject/observer code might work in C++98, but not today
#if __cplusplus >= 201103L
#include "coap/decoder/subject.hpp"
#endif

#ifdef ESP_DEBUG
#include <stdio.h>
#endif


namespace moducom { namespace coap {

namespace experimental {

}

}}
