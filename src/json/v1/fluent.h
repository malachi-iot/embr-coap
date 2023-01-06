#pragma once

#include "encoder.h"

namespace embr { namespace json {

inline namespace v1 {

namespace minij {

enum modes
{
    core = 0,
    array,
    normal,
    begin
};

}

template <class TOut, class TOptions, int mode = minij::core>
struct fluent;


template <class TStreambuf, class TBase, class TOptions, int mode_>
struct fluent<estd::detail::basic_ostream<TStreambuf, TBase>, TOptions, mode_>
{
    typedef estd::detail::basic_ostream <TStreambuf, TBase> out_type;
    typedef TOptions options_type;

    typedef fluent<out_type, options_type> default_type;
    typedef fluent<out_type, options_type, mode_> this_type;
    typedef fluent<out_type, options_type, minij::array> array_type;
    typedef fluent<out_type, options_type, minij::normal> mode2_type;
    typedef fluent<out_type, options_type, minij::begin> begin_type;

    static ESTD_CPP_CONSTEXPR_RET int mode() { return mode_; }

    out_type& out;
    encoder<TOptions>& json;

    fluent(out_type& out, encoder<TOptions>& json) :
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
    fluent& operator,(T value)
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
fluent& operator=(const char* key)
{
    return *this;
} */
};

// TODO: Look into that fnptr-like magic that ostream uses for its endl
template <class TOut, class TOptions, int mode>
fluent<TOut, TOptions, mode>& end(fluent<TOut, TOptions, mode>& j)
{
    return j.end();
}

template <class TStreambuf, class TBase, class TOptions>
struct fluent<estd::detail::basic_ostream<TStreambuf, TBase>, TOptions, minij::array> :
    fluent<estd::detail::basic_ostream<TStreambuf, TBase>, TOptions>
{
    typedef fluent<estd::detail::basic_ostream<TStreambuf, TBase>, TOptions>
            base_type;
    typedef fluent<estd::detail::basic_ostream<TStreambuf, TBase>, TOptions,
            minij::array>
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

template <class TStreambuf, class TBase, class TOptions>
struct fluent<estd::detail::basic_ostream<TStreambuf, TBase>, TOptions, minij::begin> :
    fluent<estd::detail::basic_ostream<TStreambuf, TBase>, TOptions>
{
    fluent& operator=(const char* key)
    {
        return *this;
    }
};

/*
template <class TOut>
fluent <TOut>& operator<(fluent <TOut>& j, const char* key)
{
    return j.begin(key);
}

// doesn't work
template <class TOut>
fluent <TOut>& operator>(fluent <TOut>& j, fluent <TOut>&)
{
    return j.end();
}
*/

// TODO: Rework this so that encoder can be inline inside the fluent mechanism

template <class TStreambuf, class TBase, class TOptions>
auto make_fluent(encoder<TOptions>& json, estd::detail::basic_ostream<TStreambuf, TBase>& out)
{
    return fluent<estd::detail::basic_ostream<TStreambuf, TBase>, TOptions> (out, json);
}

}

}}