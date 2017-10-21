//
// Created by malachi on 10/20/17.
//

#ifndef SRC_STRING_H
#define SRC_STRING_H

#include "memory.h"
#include <string.h>
#include <stdint.h>



namespace moducom { namespace std {

// replacement for std::string class which instead uses handle-based memory
class string {
    typedef coap::Memory memory_t;

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

    // experimental.  Safer than leaving string locked open and
    // I imagine 95% of the time this is the operation the caller
    // ultimately wants also
    void populate(char* output)
    {
        char* input = handle.lock<char>();

#ifdef FEATURE_STRING_SIZE_FIELD
        memcpy(output, input, size);
        output[size] = 0;
#else
        strcpy(output, input);
#endif

        handle.unlock();
    }

    string& operator += (const char* src)
    {
        size_t src_len = strlen(src);
#ifndef FEATURE_STRING_SIZE_FIELD
        // TODO: optimize this somehow
        char* dest = lock();
        size_t size = strlen(dest);
        unlock();
        handle.resize(size + src_len + 1);
        dest = lock();
        strcat(dest, src);
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
        size_t size = strlen(lock());
#endif
        memory_t::SmartHandle new_handle =
                handle.append_into_new_experimental(src, size, src_len);
        return string (new_handle
#ifdef FEATURE_STRING_SIZE_FIELD
                , size + src_len
#endif
        );
    }


    // experimental
    template <class T>
    class auto_locking_ptr
    {
        memory_t::SmartHandle& handle;
        T* ptr;

    public:
        auto_locking_ptr(memory_t::SmartHandle handle) : handle(handle)
        {
            ptr = handle.lock<T>();
        }

        ~auto_locking_ptr()
        {
            handle.unlock();
        }

        inline operator T* () { return ptr; }
    };

    typedef auto_locking_ptr<char> auto_ptr_t;

    /*
    auto_ptr_t get_auto_ptr_experimental()
    {
        auto_ptr_t temp(handle);

        ((char*)temp)[size] = 0;
    }*/

    bool operator ==(const char* src)
    {
        auto_ptr_t str(handle);
        char* _str = str;

#ifdef FEATURE_STRING_SIZE_FIELD
        _str[size - 1] = 0;
#endif

        return strcmp(_str, src) == 0;
    }
};

}}
#endif //SRC_STRING_H
