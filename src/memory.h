//
// Created by malachi on 10/20/17.
//

#ifndef SRC_MEMORY_H
#define SRC_MEMORY_H

#include <stdlib.h>

namespace moducom { namespace coap {


class Memory {
public:
    // FIX: eventually this will be an integer handle
    typedef void* handle_t;

public:
    /// Allocate a block of memory, but don't lock it yet
    /// @param size size in bytes of memory chunk to allocate
    /// @return
    handle_t allocate(size_t size);
    /// Allocate a block of memory, but don't lock it yet
    /// @param data data to copy into newly allocated block
    /// @param size size in bytes of memory chunk to allocate
    /// @return
    handle_t allocate(const void* data, size_t size);

    /// frees an unlocked memory handle
    /// @param handle memory handle to free
    /// @return false is memory is locked and not freeable
    bool free(handle_t handle);
    void resize(handle_t handle, size_t new_size);

    void* lock(handle_t handle) { return handle; }

    template <class T>
    T* lock(handle_t handle)
    { return static_cast<T*>(lock(handle)); }

    void unlock(handle_t handle) {}
    //void unlock(void* pointer);

    /// Compact memory around this one handle.  Deterministic runtime
    /// \param handle
    void compact(handle_t handle);
    /// Compact memory for all unlocked handles.  Nondeterministic runtime
    void compact();

    size_t available();

    static Memory default_pool;

    class SmartHandle
    {
    protected:
        const handle_t handle;
        Memory& memory;

    public:
        SmartHandle(handle_t handle, Memory& memory) :
                handle(handle),
                memory(memory)
        {}

        template <class T>
        T* lock() { return memory.lock<T>(handle); }

        void unlock() { memory.unlock(handle); }

        void resize(size_t new_size)
        {
            memory.resize(handle, new_size);
        }
    };

    class SmartHandleAutoAllocate : public SmartHandle
    {
    public:
        SmartHandleAutoAllocate(const void* data, size_t size, Memory& memory)
                : SmartHandle(memory.allocate(data, size), memory)
        {

        }

        SmartHandleAutoAllocate(size_t size, Memory& memory)
                : SmartHandle(memory.allocate(size), memory)
        {

        }

        ~SmartHandleAutoAllocate()
        {
            memory.free(handle);
        }
    };
};

}}

#endif //SRC_MEMORY_H
