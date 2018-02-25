//
// Created by malachi on 2/25/18.
//

#pragma once

namespace moducom { namespace experimental {

// Splitting this out since we don't always need count embedded in
// it
template <class T>
class ArrayHelperBase
{
    T* items;

public:
    ArrayHelperBase(T* items) : items(items) {}

    inline static void construct(T* items, size_t count)
    {
        while(count--)
        {
            new (items++) T();
        }
    }

    inline static void destruct(T* items, size_t count)
    {
        while(count--)
        {
            // NOTE: I thought one had to call the destructor
            // explicitly , I thought there was no placement
            // delete but here it is...
            delete (items++);
        }
    }

    inline void construct(size_t count) { construct(items, count);}
    inline void destruct(size_t count) { destruct(items, count); }

    operator T* () const { return items; }
};

// Apparently placement-new-array is problematic in that it has a platform
// specific addition of count to the buffer - but no way to ascertain how
// many bytes that count actually uses.  So, use this helper instead
//
// Also does double duty and carries count around in its instance mode
// since so often we need to know that also
template <class T, typename TSize = size_t>
class ArrayHelper : public ArrayHelperBase<T>
{
    const TSize _count;
    typedef ArrayHelperBase<T> base_t;

public:
    ArrayHelper(T* items, TSize count)
            : base_t(items), _count(count) {}

    TSize count() const {return _count;}

    void construct() { base_t::construct(_count);}
    void destruct() { base_t::destruct(_count); }
};


}}