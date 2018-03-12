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

template <class TValue>
struct FnFactoryValueTraits
{
    static TValue not_found_value() { return NULLPTR; }
};



template <class TKey, class TValue, class TTraits>
class Map
{
public:
    typedef FnFactoryKeyTraits<TKey> key_traits_t;
    typedef FnFactoryValueTraits<TValue> value_traits_t;

    struct Item
    {
        TKey key;
        TValue value;
    };

    typedef Item item_t;

protected:
    // TODO: switch this out to a layer3-specific entity
    //       and utilize layer3::Array in the process
    const Item* items;
    const int count;

public:
    template <const size_t N>
    Map(Item (&t) [N]) :
        items(t),
        count(N)
    {
    }

    const TValue find(TKey key)
    {
        for(int i = 0; i < count; i++)
        {
            const item_t& item = items[i];

            if(key_traits_t::equals(key, item.key))
            {
                return item.value;
            }
        }

        return value_traits_t::not_found_value();
    }
};

// FIX: shamelessly copy/pasted out of coap-uripath-dispatcher
// consolidated the starts_with code

// Specialized prefix checker which does not expect s to be
// null terminated, but does expect prefix to be null terminated
template <typename TChar>
inline bool starts_with(const TChar* s, int slen, const char* prefix)
{
    while(*prefix && slen--)
    {
        if(*prefix++ != *s++) return false;
    }

    return true;
}


inline bool starts_with(pipeline::MemoryChunk::readonly_t chunk, const char* prefix)
{
    return starts_with(chunk.data(), chunk.length(), prefix);
}



template <>
struct FnFactoryKeyTraits<const char*>
{
    static bool equals(const char* left, const char* right)
    { return strcmp(left, right) == 0; }

    static bool equals(pipeline::MemoryChunk::readonly_t& chunk, const char* right)
    {
        // TODO: This specialization lets us compare any key type with great detail
        // we don't have to rely on just an automatic operator conversion
        return starts_with(chunk, right);
    }
};


template <>
struct FnFactoryValueTraits<int>
{
    static int not_found_value() { return -1; }
};


template <class TKey, class TValue, class TContext = FnFactoryContext>
struct FnFactoryTraits
{
    typedef TKey key_t;
    typedef TValue value_t;
    typedef TContext context_t;
    typedef TContext context_ref_t;

    typedef TValue (*factory_fn_t)(context_t context);

    typedef FnFactoryKeyTraits<TKey> key_traits_t;
    typedef FnFactoryValueTraits<TValue> value_traits_t;
};


template <class TKey, class TValue, class TContext = typename FnFactoryTraits<TKey, TValue>::context_t>
struct FnFactoryItem
{
    typedef TKey key_t;
    typedef TValue value_t;
    typedef TContext context_t;
    typedef value_t (*factory_fn_t)(context_t context);

    key_t key;
    factory_fn_t factory_fn;
};


/*
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
*/

template <class TKey, class TValue, class TContext>
inline FnFactoryItem<TKey, TValue, TContext> factory_item_helper(TKey key,
       TValue (*factory_fn)(TContext context))
{
    FnFactoryItem<TKey, TValue, TContext> item;

    item.key = key;
    item.factory_fn = factory_fn;

    return item;
}


// TODO: merge FnFactory and FnFactoryHelper - one can use it either static or instantiated that way
// TODO: split out "Map" portion of FnFactory, and place it under layer3
template <class TKey, class TValue, class TTraits = FnFactoryTraits<TKey, TValue> >
class FnFactory
{
    typedef TKey key_t;
    typedef TValue value_t;
    typedef TTraits traits_t;
    typedef typename traits_t::key_traits_t key_traits_t;
    typedef typename traits_t::value_traits_t value_traits_t;
    typedef typename traits_t::context_t context_t;
    typedef typename traits_t::context_ref_t context_ref_t;
    typedef FnFactoryItem<key_t, value_t, context_t> item_t;

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

    template <class _TKey>
    static TValue create(const item_t* items, int count, _TKey key, context_ref_t context)
    {
        for(int i = 0; i < count; i++)
        {
            const item_t& item = items[i];

            if(key_traits_t::equals(key, item.key))
            {
                return item.factory_fn(context);
            }
        }

        return value_traits_t::not_found_value();
    }

    template <class _TKey>
    inline TValue create(_TKey key, context_t context)
    {
        return create(items, count, key, context);
    }
};


template <class TItem, size_t N>
FnFactory<typename TItem::key_t, typename TItem::value_t> factory_helper(TItem (&items) [N])
{
    FnFactory<typename TItem::key_t, typename TItem::value_t> factory(items, N);

    return factory;
}


template <class TTraits>
struct FnFactoryHelper
{
    typedef typename TTraits::key_t key_t;
    typedef typename TTraits::key_traits_t key_traits_t;
    typedef typename TTraits::value_t value_t;
    typedef typename TTraits::value_traits_t value_traits_t;
    typedef typename TTraits::factory_fn_t factory_fn_t;
    typedef typename TTraits::context_t context_t;
    typedef typename TTraits::context_ref_t context_ref_t;
    typedef FnFactoryItem<key_t, value_t, context_t> item_t;
    typedef FnFactory<key_t, value_t, TTraits> factory_t;

    static item_t item(key_t key, factory_fn_t factory_fn)
    {
        return factory_item_helper(key, factory_fn);
    };

    static context_t context() { context_t c; return c; }

    template <typename T, size_t N, class TKey>
    inline static value_t create(T (&items) [N], TKey key, context_ref_t context)
    {
        return factory_t::create(items, N, key, context);
    }
};



}}}
