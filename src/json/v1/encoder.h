#pragma once

#include <estd/iosfwd.h>

namespace embr { namespace json {

inline namespace v1 {

struct textfmt
{
    bool use_eol() { return true; }
    bool use_tabs() { return true; }
    bool use_spaces() { return true; }
    bool brace_on_newline() { return true; }
    ESTD_CPP_CONSTEXPR_RET bool use_doublequotes() const { return true; }
};

struct textfmt2
{
    bool use_eol() { return false; }
    bool use_tabs() { return false; }
    bool use_spaces() { return false; }
    bool brace_on_newline() { return false; }
    ESTD_CPP_CONSTEXPR_RET bool use_doublequotes() const { return false; }
};


struct encoder : textfmt2
{
    struct
    {
        uint32_t has_items_: 8;
        uint32_t level_: 3;
        uint32_t in_array_: 1;
    };

    encoder() : has_items_(0), level_(0), in_array_(0)
    {

    }

    template <class TStreambuf, class TBase>
    inline void do_tabs(estd::internal::basic_ostream <TStreambuf, TBase>& out)
    {
        if (use_tabs())
        {
            for (unsigned i = level_; i > 0; --i)
                out << "  ";
        }
    }

    template <class TStreambuf, class TBase>
    inline void do_eol(estd::internal::basic_ostream <TStreambuf, TBase>& out);

    ESTD_CPP_CONSTEXPR_RET char quote(bool key = true) const
    {
        return use_doublequotes() ? '"' : '\'';
    }

    bool has_items() const { return has_items_ >> level_; }
    bool in_array() const { return in_array_; }

    void set_has_items()
    {
        has_items_ |= 1 << level_;
    }

    // clear out indicator that this level has items
    // (do this when moving back up a level)
    void clear_has_items()
    {
        has_items_ &= ~(1 << level_);
    }

    // Prints comma and eol if items preceded this
    template <class TStreambuf, class TBase>
    void do_has_items(estd::internal::basic_ostream <TStreambuf, TBase>& out)
    {
        if (has_items())
        {
            out << ',';
            do_eol(out);
        }
        else
            set_has_items();
    }

    template <class TStreambuf, class TBase>
    void begin(estd::internal::basic_ostream <TStreambuf, TBase>& out)
    {
        do_tabs(out);
        do_has_items(out);

        out << '{';
        do_eol(out);
        ++level_;
    }

    template <class TStreambuf, class TBase>
    void add_key(estd::internal::basic_ostream <TStreambuf, TBase>& out, const char* key)
    {
        do_has_items(out);

        do_tabs(out);

        out << quote() << key << quote() << ':';

        if (use_spaces()) out << ' ';
    }

    template <class TStreambuf, class TBase>
    void begin(estd::internal::basic_ostream <TStreambuf, TBase>& out, const char* key)
    {
        add_key(out, key);

        if (brace_on_newline())
        {
            do_eol(out);
            do_tabs(out);
        }

        out << '{';
        do_eol(out);

        ++level_;
    }

    template <class TStreambuf, class TBase>
    void array(estd::internal::basic_ostream <TStreambuf, TBase>& out, const char* key)
    {
        in_array_ = 1;

        add_key(out, key);

        out << '[';
        ++level_;
    }

    template <class TStreambuf, class TBase>
    void raw(estd::internal::basic_ostream <TStreambuf, TBase>& out, const char* value)
    {
        out << quote(false) << value << quote(false);
    }

    template <class TStreambuf, class TBase>
    void raw(estd::internal::basic_ostream <TStreambuf, TBase>& out, int value)
    {
        out << value;
    }

    template <class TStreambuf, class TBase, typename T>
    void array_item(estd::internal::basic_ostream <TStreambuf, TBase>& out, T value)
    {
        if (has_items())
        {
            out << ',';
            if (use_spaces()) out << ' ';
        }
        else
            set_has_items();

        raw(out, value);
    }

    template <class TStreambuf, class TBase>
    void end_array(estd::internal::basic_ostream <TStreambuf, TBase>& out)
    {
        out << ']';
        --level_;
        in_array_ = 0;
    }

    template <class TStreambuf, class TBase>
    void end(estd::internal::basic_ostream <TStreambuf, TBase>& out)
    {
        clear_has_items();

        if (in_array())
        {
            end_array(out);
            return;
        }

        --level_;

        do_eol(out);
        do_tabs(out);

        out << '}';
    }

    template <class TStreambuf, class TBase, typename T>
    void add(estd::internal::basic_ostream <TStreambuf, TBase>& out, const char* key, T value)
    {
        add_key(out, key);
        raw(out, value);
    }
};


}

}}