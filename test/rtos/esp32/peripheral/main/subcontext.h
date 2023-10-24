#pragma once

#include <coap/platform/lwip/context.h>
#include <coap/context/sub.h>   // DEBT: Temporary as we phase into API supported flavor

namespace concepts = embr::coap::concepts;

// Looking into https://stackoverflow.com/questions/64694218/how-to-express-concepts-over-variadic-template
// But this isn't quite there
//template <class ...Args>
//concept Substates = (Substate(Args) && ...)

