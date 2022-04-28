#pragma once

#include <estd/utility.h>
#include <estd/tuple.h>

namespace embr { namespace coap {

namespace experimental {

template <class TKey, class TValue>
struct Factory
{
};

//template <class TKey, class ...TFactories>
template <class TKey, class TFactoriesTuple>
struct FactoryAggregator
{
    ESTD_FN_HAS_METHOD(bool, can_create, TKey)

    //typedef estd::tuple<TFactories...> tuple_type;
    typedef typename estd::remove_reference<TFactoriesTuple>::type tuple_type;
    TFactoriesTuple factories;

    FactoryAggregator(tuple_type& factories) : factories(factories) {}

    template <int index, class = estd::enable_if_t<!(index >= 0)> >
    void create_helper(TKey) const {}

    template <int index, class = estd::enable_if_t<(index >= 0)> >
    void create_helper(const TKey& key, bool = true)
    {
        typedef estd::tuple_element_t<index, tuple_type> factory_type;

        static_assert (has_can_create_method<factory_type>::value,
                       "Needs a can create (and create) method");

        factory_type& factory = estd::get<index>(factories);

        if(factory.can_create(key))
        {
            // NOTE: Still gonna need some memory management like objstack
            factory.create(key);
        }

        create_helper<index - 1>(key);
    }



    void create(const TKey& key)
    {
        constexpr int sz = estd::tuple_size<tuple_type>();
        //estd::apply(svc, factories);
        create_helper<sz - 1>(key);
    }
};



}

}}
