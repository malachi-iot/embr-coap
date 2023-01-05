#pragma once

#include "encoder.h"

namespace embr { namespace json {

namespace minij {

enum modes
{
    core = 0,
    array,
    normal,
    begin
};

}


inline namespace v1 {

template <class TOut, int mode = minij::core>
struct minijson_fluent;


template <class TStreambuf, class TBase, int mode_>
struct minijson_fluent<estd::detail::basic_ostream < TStreambuf, TBase>, mode_> {
    typedef estd::detail::basic_ostream <TStreambuf, TBase> out_type;

    typedef minijson_fluent<out_type> default_type;
    typedef minijson_fluent<out_type, mode_> this_type;
    typedef minijson_fluent<out_type, minij::array> array_type;
    typedef minijson_fluent<out_type, minij::normal> mode2_type;
    typedef minijson_fluent<out_type, minij::begin> begin_type;

    static ESTD_CPP_CONSTEXPR_RET int mode() { return mode_; }

    out_type& out;
    encoder& json;

    minijson_fluent(out_type& out, encoder& json) :
            out(out),
            json(json) {}

    template <typename T>
    this_type& add(const char* key, T value)
    {
        json.add(out, key, value);
        return *this;
    }

    this_type& begin()
    {
        json.begin(out);
        return *this;
    }

    this_type& begin(const char* key)
    {
        json.begin(out, key);
        return *this;
    }

    this_type& array(const char* key)
    {
        json.array(out, key);
        return *this;
    }

    template <class T>
    this_type& array_item(T value)
    {
        json.array_item(out, value);
        return *this;
    }

    this_type& end()
    {
        json.end(out);
        return *this;
    }

    this_type& operator()(const char* key)
    {
        return begin(key);
    }

    this_type& operator()()
    {
        return end();
    }

    template <typename T>
    this_type& operator()(const char* key, T value)
    {
        return add(key, value);
    }

    array_type& operator[](const char* key)
    {
        return (array_type&) array(key);
    }

    this_type& operator--(int)
    {
        return end();
    }

/*
 - fascinating, but deeper overloading of () is better
    template <typename T>
    minijson_fluent& operator,(T value)
    {
        json.array_item(out, value);
        return *this;
    } */

// almost but not quite
/*
begin_type& operator++(int)
{
    return (begin_type&) *this;
} */


/*
    begin_type* operator->()
    {
        return (begin_type*) this;
    } */

/*
minijson_fluent& operator=(const char* key)
{
    return *this;
} */
};

// TODO: Look into that fnptr-like magic that ostream uses for its endl
template <class TOut, int mode>
minijson_fluent<TOut, mode>& end(minijson_fluent < TOut, mode > &j)
{
    return j.end();
}

template <class TStreambuf, class TBase>
struct minijson_fluent<estd::internal::basic_ostream < TStreambuf, TBase>, minij::array> :
        minijson_fluent<estd::internal::basic_ostream < TStreambuf, TBase> > {
    typedef minijson_fluent<estd::internal::basic_ostream < TStreambuf, TBase> >
            base_type;
    typedef minijson_fluent<estd::internal::basic_ostream < TStreambuf, TBase>, minij::array>
            this_type;

    template <class T>
    void array_items(T item)
    {
        base_type::array_item(item);
    }

    template <class T, class ... TArgs>
    void array_items(T item, TArgs...args)
    {
        base_type::array_item(item);
        array_items(std::forward<TArgs>(args)...);
    }

    template <class ... TArgs>
    base_type& operator()(TArgs...args)
    {
        array_items(std::forward<TArgs>(args)...);
        base_type::end();
        return *this;
    }
};

template <class TStreambuf, class TBase>
struct minijson_fluent<estd::internal::basic_ostream < TStreambuf, TBase>, minij::begin> :
        minijson_fluent<estd::internal::basic_ostream < TStreambuf, TBase> > {
    minijson_fluent& operator=(const char* key)
    {
        return *this;
    }
};


template <class TOut>
minijson_fluent <TOut>& operator<(minijson_fluent <TOut>& j, const char* key)
{
    return j.begin(key);
}

// doesn't work
template <class TOut>
minijson_fluent <TOut>& operator>(minijson_fluent <TOut>& j, minijson_fluent <TOut>&)
{
    return j.end();
}


template <class TStreambuf, class TBase>
auto make_fluent(encoder& json, estd::internal::basic_ostream <TStreambuf, TBase>& out)
{
    return minijson_fluent<estd::internal::basic_ostream<TStreambuf, TBase> > (out, json);
}

}

}}