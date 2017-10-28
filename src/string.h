//
// Created by malachi on 10/20/17.
//

#ifndef SRC_STRING_H
#define SRC_STRING_H

#include "memory.h"
#include <string.h>
#include <stdint.h>

#define FEATURE_TOSTRING_STDIO
#ifdef FEATURE_TOSTRING_STDIO
#include <stdio.h>
#endif

// FEATURE_STRING_CONST is not compatible with FEATURE_STRING_SIZE_FIELD
#define FEATURE_STRING_CONST const

#ifndef FEATURE_STRING_CONST
#define FEATURE_STRING_CONST_DISABLED
#define FEATURE_STRING_CONST
#endif


namespace moducom { namespace std {

// replacement for std::string class which instead uses handle-based memory
class string {
    typedef dynamic::Memory memory_t;

    memory_t::SmartHandle handle;
#ifdef FEATURE_STRING_SIZE_FIELD
    // TODO: optimize size out - perhaps smarthandle itself can track it?
    size_t size;
#endif

private:
    inline char* lock() FEATURE_STRING_CONST
    {
#ifdef FEATURE_STRING_CONST_DISABLED
        return handle.lock<char>();
#else
        // cheating and creating side affects in a const
        memory_t::SmartHandle& _h = (memory_t::SmartHandle&) handle;

        return _h.lock<char>();
#endif
    }
    inline void unlock() FEATURE_STRING_CONST
    {
#ifdef FEATURE_STRING_CONST_DISABLED
        handle.unlock();
#else
        // cheating and creating side affects in a const
        memory_t::SmartHandle& _h = (memory_t::SmartHandle&) handle;

        _h.unlock();
#endif
    }

    // TODO: make this smart enough to not allocate memory until someone actually
    // changes the string
    // TODO: Make final decision about +1 for trailing null or not, or make it a
    // traits kinda thing
protected:
    string(const memory_t::SmartHandle& handle
#ifdef FEATURE_STRING_SIZE_FIELD
            , size_t size
#endif
            ) :
            handle(handle)
#ifdef FEATURE_STRING_SIZE_FIELD
            ,
            size(size + 1)
#endif
            {}
public:
    string(const char* src, memory_t& pool = memory_t::default_pool) :
#ifdef FEATURE_STRING_SIZE_FIELD
            size(strlen(src)),
#endif
            // FIX: somehow size isn't assigned and have to use strlen(src) again
            handle(pool.allocate(src, strlen(src) + 1), pool)
    {
    }

    // FIX: default constructor allocates 10 bytes, but make this configurable
    string(memory_t& pool = memory_t::default_pool) :
            // FIX: we are left with "undiscoverable" bytes which may or may not 
            // cause inefficiencies for reallocation (FEATURE_STRING_SIZE_FIELD sorta trying to address that)
            handle(pool.allocate("", 10, 1), pool) {}

    // TODO: Do also && move operation for C++11
    string(const string& copy_from) :
#ifdef FEATURE_STRING_SIZE_FIELD
            size(copy_from.size),
#endif
            handle(copy_from.handle
#ifdef FEATURE_STRING_SIZE_FIELD
                    , copy_from.size
#endif
            )
    {

    }

    string(const char* src, size_t length, memory_t& pool = memory_t::default_pool) :
#ifdef FEATURE_STRING_SIZE_FIELD
        size(length),
#endif
        handle(pool.allocate(src, length + 1), pool)
    {
        char* data = lock();

        data[length] = 0;

        unlock();
    }

    // experimental
    template <class T>
    class auto_locking_ptr
    {
        memory_t::SmartHandle& handle;
        T* ptr;

    public:
        auto_locking_ptr(memory_t::SmartHandle& handle) : handle(handle)
        {
            ptr = handle.lock<T>();
        }

        ~auto_locking_ptr()
        {
            handle.unlock();
        }

        inline operator T* () { return ptr; }
    };

    class auto_ptr_t : public auto_locking_ptr<char>
    {
    public:
        auto_ptr_t(FEATURE_STRING_CONST memory_t::SmartHandle& handle) :
                auto_locking_ptr<char>((memory_t::SmartHandle&)handle)
        {
        }

        auto_ptr_t(string& s) : auto_locking_ptr<char>(s.handle)
        {
        }
    };


#ifndef FEATURE_STRING_SIZE_FIELD
    size_t length() FEATURE_STRING_CONST
    {
        char* str = lock();
        size_t len = strlen(str);
        unlock();
        return len;
    }
#else
    size_t length() { return size; }
#endif

    /// experimental.  Safer than leaving string locked open and
    /// I imagine 95% of the time this is the operation the caller
    /// ultimately wants also
    void populate(char* output) FEATURE_STRING_CONST
    {
        auto_ptr_t input(handle);

#ifdef FEATURE_STRING_SIZE_FIELD
        memcpy(output, input, size);
        output[size] = 0;
#else
        strcpy(output, input);
#endif
    }

    string& operator += (const char* src)
    {
        size_t src_len = strlen(src);
#ifndef FEATURE_STRING_SIZE_FIELD
        // TODO: optimize this somehow
        size_t size = length();
        handle.resize(size + src_len + 1);
        strcat(lock(), src);
#else
        handle.resize(size + src_len + 1);
        char* dest = handle.lock<char>();
        memcpy(dest + size, src, src_len);
        size += src_len;
#endif
        unlock();
        return *this;
    }

    string& operator += (FEATURE_STRING_CONST string& append_from)
    {
        const char* _append_from = append_from.lock();
        *this += _append_from;
        append_from.unlock();
        return *this;
    }

    inline string operator +(const char *src) FEATURE_STRING_CONST
    {
        size_t src_len = strlen(src);
#ifndef FEATURE_STRING_SIZE_FIELD
        size_t size = length();
#endif
        memory_t::SmartHandle new_handle =
                handle.append_into_new_experimental(src, size, src_len + 1);
        return string (new_handle
#ifdef FEATURE_STRING_SIZE_FIELD
                , size + src_len + 1
#endif
        );
    }

    /// temporary measure while we figure out how to handle
    /// trailing nulls
    // Beware this likely doubly-locks and support for that is
    // currently undefined
    /**!
     * temporary measure while we figure out how to handle
     * trailing nulls
     *
     */
    inline void ensure_trailing_null() FEATURE_STRING_CONST
    {
#ifdef FEATURE_STRING_SIZE_FIELD
        char* _str = lock();

        _str[length - 1] = 0;

        unlock();
#endif
    }


    /*
    auto_ptr_t get_auto_ptr_experimental()
    {
        auto_ptr_t temp(handle);

        ((char*)temp)[size] = 0;
    }*/

    bool operator ==(const char* src) FEATURE_STRING_CONST
    {
        ensure_trailing_null();

        auto_ptr_t str(handle);

        return strcmp(str, src) == 0;
    }

    bool operator ==(FEATURE_STRING_CONST string& compare_to) FEATURE_STRING_CONST
    {
        ensure_trailing_null();
        compare_to.ensure_trailing_null();

        auto_ptr_t  ours(handle),
                    theirs(compare_to.handle);

        return strcmp(ours, theirs) == 0;
    }


    /*
    // FIX: Wondering if this is needed for our map<> calls
    bool operator ==(const string& compare_to) const
    {
        return *((string*)this) == (string&)compare_to;
    } */


    bool operator !=(FEATURE_STRING_CONST string& compare_to) FEATURE_STRING_CONST
    {
        return !(*this == compare_to);
    }

    string& operator =(string& copy_from)
    {
        handle.copy_from(copy_from.handle, copy_from.length());

        return *this;
    }

    bool operator <(FEATURE_STRING_CONST string& compare_to) FEATURE_STRING_CONST
    {
        ensure_trailing_null();
        auto_ptr_t  ours(handle),
            theirs(compare_to.handle);

        const char* _ours = ours;
        const char* _theirs = theirs;

        return strcmp(_ours, _theirs) < 0;
    }
};

}}

#ifdef FEATURE_TOSTRING_STDIO
inline moducom::std::string& operator += (moducom::std::string& s, int value)
{
    char buffer[10];

    sprintf(buffer, "%d", value);

    s += buffer;

    return s;
}
#endif

#endif //SRC_STRING_H
