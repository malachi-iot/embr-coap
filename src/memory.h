//
// Created by malachi on 10/20/17.
//

#ifndef SRC_MEMORY_H
#define SRC_MEMORY_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

namespace moducom { namespace dynamic {


#define MODUCOM_ASSERT(expected, actual)

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
    /// @param size_copy size in bytes to copy from data
    /// @return
    handle_t allocate(const void* data, size_t size, size_t size_copy = 0);

    /// frees an unlocked memory handle
    /// @param handle memory handle to free
    /// @return false is memory is locked and not freeable
    bool free(handle_t handle);

    /// may change handle_t in the process
    /// \param handle
    /// \param new_size
    /// \return
    handle_t realloc(handle_t handle, size_t new_size)
    {
        return ::realloc(handle, new_size);
    }

    // TODO: might manage size differently, but for now we have to be explicit
    ///
    /// \param handle
    /// \param size size of allocation
    /// \param size_copy size of copy must be less than size
    /// \return
    handle_t copy(handle_t handle, size_t size, size_t size_copy = 0);

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
        //const // can't be const due to current realloc behavior
        // with true memory pools, handle can return to being constant
        handle_t handle;
        Memory& memory;

    public:
        SmartHandle(handle_t handle, Memory& memory) :
                handle(handle),
                memory(memory)
        {}

        /// Copy Constructor
        /// \param smartHandle
        SmartHandle(const SmartHandle& smartHandle, size_t size) :
                handle(smartHandle.memory.copy(smartHandle.handle, size)),
                memory(smartHandle.memory)
        {

        }

        template <class T>
        T* lock() { return memory.lock<T>(handle); }

        void unlock() { memory.unlock(handle); }

        // FIX: beware, currently this might change handle
        // that behavior goes away when proper memory manager code is present
        void resize(size_t new_size)
        {
            handle = memory.realloc(handle, new_size);
        }

        SmartHandle append_into_new_experimental(const void* append_data,
                                                 size_t current_size,
                                                 size_t append_size) const
        {
            //handle_t new_data_handle = memory.allocate(current_size + append_size);
            handle_t new_data_handle =
                    memory.copy(handle,
                                current_size + append_size,
                                current_size);

            uint8_t* new_data = memory.lock<uint8_t>(new_data_handle);

            memcpy(new_data + current_size, append_data, append_size);

            memory.unlock(new_data_handle);

            return SmartHandle(new_data_handle, memory);
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


// Concept is: stack space is easier to push non-tiny objects around on
// so Memory& is OK here
class MemoryBuffer : public Memory::SmartHandle
{
    size_t size;

public:
    MemoryBuffer(Memory::handle_t handle, size_t size, Memory& memory = Memory::default_pool) :
            size(size),
            Memory::SmartHandle(handle, memory) {}

    MemoryBuffer(Memory::SmartHandle& copy_from, size_t size) :
            size(size),
            Memory::SmartHandle(copy_from) {}

    MemoryBuffer(MemoryBuffer& copy_from) :
            size(size),
            Memory::SmartHandle(copy_from) {}

    size_t length() const { return size; }

};


template <class T>
class MemoryBufferAuto : public MemoryBuffer
{
public:
    MemoryBufferAuto(size_t size, Memory& memory = Memory::default_pool) :
            MemoryBuffer(memory.allocate(size), size, memory) {}

    ~MemoryBufferAuto()
    {
        MODUCOM_ASSERT(true, memory.free(handle));
    }

    T* lock() { return MemoryBuffer::lock<T>(); }
};

}}

#endif //SRC_MEMORY_H
