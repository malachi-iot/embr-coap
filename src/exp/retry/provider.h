#pragma once

#include <estd/algorithm.h>

namespace embr { namespace internal {

// DEBT: Needs unit test
// DEBT: Probably wants to live in estd value_evaporator area
// DEBT: Name feels clumsy

template <class T, bool is_reference = estd::is_reference<T>::value>
struct instance_or_reference_provider;

template <class T>
struct instance_or_reference_provider<T, true>
{
    typedef typename estd::remove_reference<T>::type value_type;
    typedef T reference;
    typedef const value_type& const_reference;

    reference r;

    reference value() { return r; }
    const_reference value() const { return r; }

    instance_or_reference_provider(reference r) : r{r} {}
};

template <class T>
struct instance_or_reference_provider<T, false> : T
{
    typedef T base_type;

    typedef T value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;

    reference value() { return *this; }
    const_reference value() const { return *this; }

    ESTD_CPP_FORWARDING_CTOR(instance_or_reference_provider);
};

}}