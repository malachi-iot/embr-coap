//
// Created by malachi on 2/14/18.
//
#pragma once

namespace moducom { namespace coap { namespace experimental {

struct FnFactoryContext
{
    pipeline::MemoryChunk chunk;

#ifndef FEATURE_MCCOAP_REWRITABLE_MEMCHUNK
    FnFactoryContext() : chunk(NULLPTR, 0) {}
#endif
};


template <class TKey>
struct KeyTraits
{
    static bool equals(TKey left, TKey right) { return left == right; }
};

template <class TValue>
struct ValueTraits
{
    static TValue not_found_value() { return NULLPTR; }
};



template <class TKey, class TValue>
struct KeyValuePair
{
    typedef TValue value_t;
    typedef TKey key_t;

    key_t key;
    value_t value;

    /*
    KeyValuePair() {}

    KeyValuePair(key_t key, value_t value) :
        key(key),
        value(value)
    {} */
};


template <class TKeyValuePair,
    class TKeyTraits,
    class TValueTraits,
    class TKey>
inline static typename TKeyValuePair::value_t find(
    const TKeyValuePair* items, int count,
    TKey key)
{
    typedef TKeyValuePair item_t;
    typedef TKeyTraits key_traits_t;
    typedef TValueTraits value_traits_t;

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


template <class TKeyValuePair, class TKey>
inline static typename TKeyValuePair::value_t find2(
    const TKeyValuePair* items, int count,
    TKey key)
{
    return find<TKeyValuePair,
        KeyTraits<typename TKeyValuePair::key_t>,
        ValueTraits<typename TKeyValuePair::value_t>,
        TKey>(items, count, key);
}

template <class TKey, class TValue, class TTraits>
class Map
{
public:
    typedef KeyTraits<TKey> key_traits_t;
    typedef ValueTraits<TValue> value_traits_t;

    typedef KeyValuePair<TKey, TValue> item_t;

protected:
    // TODO: switch this out to a layer3-specific entity
    //       and utilize layer3::Array in the process
    const item_t* items;
    const int count;

public:
    template <const size_t N>
    Map(item_t (&t) [N]) :
        items(t),
        count(N)
    {
    }

    TValue find(TKey key)
    {
        return experimental::find<item_t, key_traits_t, value_traits_t>(items, count, key);
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

    // If we haven't reached end of prefix, then s was too short and
    // didn't match prefix
    if(*prefix) return false;

    return true;
}


inline bool starts_with(const pipeline::MemoryChunk::readonly_t& chunk, const char* prefix)
{
    return starts_with(chunk.data(), chunk.length(), prefix);
}



template <>
struct KeyTraits<const char*>
{
    static bool equals(const char* left, const char* right)
    { return strcmp(left, right) == 0; }

    static bool equals(const pipeline::MemoryChunk::readonly_t& chunk, const char* right)
    {
        // TODO: This specialization lets us compare any key type with great detail
        // we don't have to rely on just an automatic operator conversion
        return starts_with(chunk, right);
    }
};


template <>
struct ValueTraits<int>
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

    typedef KeyTraits<TKey> key_traits_t;
    typedef ValueTraits<TValue> value_traits_t;
};


template <class TKey, class TValue, class TContext = typename FnFactoryTraits<TKey, TValue>::context_t>
struct FnFactoryItem : public KeyValuePair<TKey, TValue (*)(TContext)>
{
    typedef KeyValuePair<TKey, TValue (*)(TContext)> base_t;
    typedef TContext context_t;
    typedef TValue (*factory_fn_t)(context_t context);

    /*
     * Not a terrible idea but ignites a requirement for C++11
    FnFactoryItem() {}

    FnFactoryItem(TKey key, TValue value) : base_t(key, value) {} */

    TValue factory_fn(context_t context) const { return this->value(context); };
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
    item.value = factory_fn;

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
    typedef typename traits_t::factory_fn_t factory_fn_t;
    typedef ValueTraits<factory_fn_t> factory_fn_traits_t;
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
        //value_t found = find<item_t, key_traits_t, value_traits_t>(items, count, key);
        //factory_fn_t found = find<item_t, key_traits_t, factory_fn_traits_t>(items, count, key);
        factory_fn_t found = find2(items, count, key);

        if(factory_fn_traits_t::not_found_value() == found)
            return value_traits_t::not_found_value();

        return found(context);

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


// FIX: Move this and the item_experimental out of here
class IDispatcherHandler;

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

#if defined(__CPP11__) || __cplusplus >= 201103L
    // We would rather this be somewhere more like AggregateUriPathObserver
    // it's too specific to be living here
    template <class TObserver>
    static item_t item_experimental(key_t key)
    {
        return item(key, [](context_t& c)
        {
            TObserver observer = new (c.objstack) TObserver;

            observer->set_context(c.context);

            return static_cast<IDispatcherHandler*>(observer);
        });
    }
#endif

    static context_t context() { context_t c; return c; }

    template <typename T, size_t N, class TKey>
    inline static value_t create(T (&items) [N], TKey key, context_ref_t context)
    {
        return factory_t::create(items, N, key, context);
    }
};



}}}
