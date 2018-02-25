//
// Created by malachi on 2/25/18.
//

// this is a *traditional* memory pool, i.e. array of fixed size chunks vs memory_pool.h which is our
// advanced virtual memory engine

#include <stdlib.h>
#include "array-helper.h"

#pragma once

namespace moducom { namespace dynamic {


// lean heavily on placement new, for smarter pool items
template <class T>
struct DefaultPoolItemTrait
{
    static bool is_allocated(const T& item)
    {
        return item.is_active();
    }


    static bool is_free(const T& item)
    {
        return !item.is_active();
    }

    template <class TArg1>
    static void allocate(T& item, TArg1 arg1)
    {
        new (&item) T(arg1);
    }


    static void free(T& item)
    {
        item.~T();
    }

    static void initialize(T& item)
    {
        item.~T();
    }
};


template <class T>
struct ExplicitPoolItemTrait
{
    static bool is_allocated(const T& item)
    {
        return item.is_active();
    }


    static bool is_free(const T& item)
    {
        return !item.is_active();
    }


    template <class TArg1>
    static void allocate(T& item, TArg1 arg1)
    {
        item.allocate(arg1);
    }


    // unallocate item, returning it to a free status
    static void free(T& item)
    {
        item.free();
    }

    // zero out item, initializing it to a free status
    static void initialize(T& item)
    {
        item.initialize();
    }
};


template <class T, class TTraits>
inline size_t count_allocated(const T* items, size_t max_count)
{
    size_t c = 0;

    for(int i = 0; i < max_count; i++)
    {
        const T& item = items[i];

        if(TTraits::is_allocated(item))
            c++;
    }

    return c;
}


template <class T, size_t max_count, class TTraits = DefaultPoolItemTrait<T > >
class PoolBase
{
    typedef TTraits traits_t;
    typedef experimental::ArrayHelperBase<T> array_helper_t;

    // pool items themselves
    T items[max_count];

public:
    // TODO: complete once I determine how useful this is in a pre-C++11 environment
    size_t begin()
    {
        return 0;
    }


    size_t end() const
    {
        return max_count;
    }

    PoolBase()
    {
        // Not needed because explicit rigid arrays do an auto constructor call on
        // their items
        //array_helper_t::construct(items, max_count);
        // however, many items are "dumb" and have no inherent knowledge of their own
        // validity, so we do need to signal somehow that they are unallocated
        for(int i = 0; i < max_count; i++)
            traits_t::initialize(items[i]);
    }

#ifdef __CPP11__
    template <class TArgs... args>
    T& allocate(TArgs...args1)
    {
        for(int i = 0; i < count; i++)
        {
            T& candidate = items[i];

            if(traits_t::is_free(candidate))
            {
                new (&candidate) T(args1...);
                return candidate;
            }
        }
    }
#else
    template <class TArg1>
    T& allocate(TArg1 arg1)
    {
        for(int i = 0; i < max_count; i++)
        {
            T& candidate = items[i];

            if(traits_t::is_free(candidate))
            {
                new (&candidate) T(arg1);
                return candidate;
            }
        }
    }

    T& allocate()
    {
        for(int i = 0; i < max_count; i++)
        {
            T& candidate = items[i];

            if(traits_t::is_free(candidate))
            {
                new (&candidate) T();
                return candidate;
            }
        }
    }
#endif


    void free(T& item)
    {
        for(int i = 0; i < max_count; i++)
        {
            T& candidate = items[i];

            if(&item == &candidate)
            {
                traits_t::free(candidate);
                return;
            }
        }
    }

    // returns number of allocated items
    size_t count() const
    {
        return count_allocated<T, traits_t>(items, max_count);
    }

    // returns number of free slots
    size_t free() const
    {
        return max_count - count();
    }
};

// To replace non fixed-array pools, but still are traditional pools
// i.e. at RUNTIME are fixed count and in a fixed position, once
// the class is initialized
// TODO: Combine with FnFactory also
template <class T, class TTraits = DefaultPoolItemTrait<T > >
class OutOfBandPool
{
    typedef TTraits traits_t;
    const T* items;
    const int max_count;

public:
    // returns number of allocated items
    size_t count() const
    {
        return count_allocated<T, traits_t>(items, max_count);
    }

    // returns number of free slots
    size_t free() const
    {
        return max_count - count();
    }
};

}}