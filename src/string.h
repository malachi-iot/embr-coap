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
    inline char* lock()
    {
        return handle.lock<char>();
    }
    inline void unlock() { handle.unlock(); }

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
        auto_ptr_t(memory_t::SmartHandle& handle) : auto_locking_ptr<char>(handle)
        {
        }

        auto_ptr_t(string& s) : auto_locking_ptr<char>(s.handle)
        {
        }
    };


#ifndef FEATURE_STRING_SIZE_FIELD
    size_t length()
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
    void populate(char* output)
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

    inline string operator +(const char *src)
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
    inline void ensure_trailing_null()
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

    bool operator ==(const char* src)
    {
        ensure_trailing_null();

        auto_ptr_t str(handle);

        return strcmp(str, src) == 0;
    }

    bool operator ==(string& compare_to)
    {
        ensure_trailing_null();
        compare_to.ensure_trailing_null();

        auto_ptr_t  ours(handle),
                    theirs(compare_to.handle);

        return strcmp(ours, theirs) == 0;
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
