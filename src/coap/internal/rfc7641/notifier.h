#pragma once

#include "enum.h"
#include "keys.h"

namespace embr { namespace coap {

namespace internal {

// DEBT: In fact, NotifyHelperBsae is truly the registrar

// DEBT: Consider removing 'Base' and instead specializing on void transport -
// note for that to be aesthetic (which is the whole point), we'd need to swap
// TTransport and TRegistrar order
template <class TRegistrar>
struct NotifyHelperBase
{
    typedef typename estd::remove_reference<TRegistrar>::type registrar_type;
    typedef typename registrar_type::endpoint_type endpoint_type;
    typedef typename registrar_type::handle_type handle_type;

    // DEBT: exposing this so that nvs/serialization can get to it
public:
//protected:
    TRegistrar registrar_;

public:
    ESTD_CPP_CONSTEXPR_RET NotifyHelperBase(const registrar_type& registrar) :
        registrar_(registrar)
    {}

    ESTD_CPP_DEFAULT_CTOR(NotifyHelperBase)

    registrar_type& registrar() { return registrar_; }
};

}

}}
