//
// Created by malachi on 2/14/18.
//
#pragma once

namespace moducom { namespace coap { namespace experimental {

struct FnFactoryContext
{
    pipeline::MemoryChunk chunk;
};


template <class TKey>
struct FnFactoryKeyTraits
{
    static bool equals(TKey left, TKey right) { return left == right; }
};


template <>
struct FnFactoryKeyTraits<const char*>
{
    static bool equals(const char* left, const char* right)
    { return strcmp(left, right) == 0; }
};


template <class TValue>
struct FnFactoryValueTraits
{
    static TValue null_value() { return NULLPTR; }
};

template <>
struct FnFactoryValueTraits<int>
{
    static int null_value() { return -1; }
};


template <class TKey, class TValue>
struct FnFactoryTraits
{
    typedef FnFactoryContext context_t;
    typedef FnFactoryContext context_ref_t;

    typedef TValue (*factory_fn_t)(context_t context);

    typedef FnFactoryKeyTraits<TKey> key_traits_t;
    typedef FnFactoryValueTraits<TValue> value_traits_t;
};


template <class TKey, class TValue, class TTraits = FnFactoryTraits<TKey, TValue> >
struct FnFactoryItem
{
    typedef TKey key_t;
    typedef TValue value_t;
    typedef TTraits traits_t;
    typedef typename traits_t::context_t context_t;
    typedef TValue (*factory_fn_t)(context_t context);

    key_t key;
    factory_fn_t factory_fn;
};


template <class TKey, class TValue>
inline FnFactoryItem<TKey, TValue> factory_item_helper(TKey key,
        //, typename TTraits::factory_fn_t factory_fn)
       typename FnFactoryItem<TKey, TValue>::factory_fn_t factory_fn)
{
    FnFactoryItem<TKey, TValue> item;

    item.key = key;
    item.factory_fn = factory_fn;

    return item;
}

template <class TKey, class TValue>
inline FnFactoryItem<const char*, TValue> factory_item_helper(TKey key,
       TValue (*factory_fn)(typename FnFactoryTraits<TKey, TValue>::context_t))
{
    FnFactoryItem<TKey, TValue> item;

    item.key = key;
    item.factory_fn = factory_fn;

    return item;
}


template <class TKey, class TValue, class TTraits = FnFactoryTraits<TKey, TValue> >
class FnFactory
{
    typedef TKey key_t;
    typedef TTraits traits_t;
    typedef typename traits_t::key_traits_t key_traits_t;
    typedef typename traits_t::value_traits_t value_traits_t;
    typedef typename traits_t::context_t context_t;
    typedef FnFactoryItem<TKey, TValue, traits_t> item_t;

    const item_t* items;
    const int count;

public:
    template <class T, const size_t N>
    FnFactory(T (&t) [N]) :
            items(t),
            count(N)
    {
    }

    FnFactory(const item_t* items, int count)
            :items(items), count(count) {}

    TValue create(TKey key, context_t context)
    {
        for(int i = 0; i < count; i++)
        {
            const item_t& item = items[i];

            if(key_traits_t::equals(item.key, key))
            {
                return item.factory_fn(context);
            }
        }

        return value_traits_t::null_value();
    }
};


template <class TItem, size_t N>
FnFactory<typename TItem::key_t, typename TItem::value_t> factory_helper(TItem (&items) [N])
{
    FnFactory<typename TItem::key_t, typename TItem::value_t> factory(items, N);

    return factory;
}

}}}