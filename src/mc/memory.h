#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../platform.h"

namespace moducom { namespace dynamic {

class IMemory
{
protected:
public:
    // need to be big enough to also just hold a regular non-lockable pointer
    // (for the IMemory providers which don't actually do any locking/tracking)
    // TODO: Consider a non-locking IMemory implementation which still does minimal tracking
    // so that we can use 16-bit handles all the way around
#ifdef ENVIRONMENT64
    typedef uint64_t handle_opaque_t;
#elif defined(ENVIRONMENT32)
    typedef uint32_t handle_opaque_t;
#elif defined(ENVIRONMENT16)
    typedef uint16_t handle_opaque_t;
#endif

	// NOTE: We allow this brute force of negative here because we are 100%
	// OK with it also being the "top" of whatever bitness our words are
    static CONSTEXPR handle_opaque_t invalid_handle = (handle_opaque_t)-1;

    // TODO: Move this out of IMemory abstract class
    enum TierEnum
    {
        Direct = 0,
        Indexed = 1,
        Compact = 2,
        Indexed2 = 3
        // Compact2 would be like the linked-list version I was working on before, but might have to carry handle info along too
    };

    virtual handle_opaque_t allocate(size_t size) = 0;
    virtual handle_opaque_t allocate(const void* data, size_t size, size_t size_copy = 0) = 0;
    virtual bool free(handle_opaque_t handle) = 0;
    // TODO: consider making this an append instead (add a append byte* who is 'size' big)
    virtual bool expand(handle_opaque_t handle, size_t size) = 0;
    virtual void shrink(handle_opaque_t handle, size_t size) = 0;

    virtual handle_opaque_t copy(handle_opaque_t copy_from,
                                 size_t size,
                                 size_t size_copy) { return invalid_handle; };

    virtual void* lock(handle_opaque_t handle) = 0;
    virtual void unlock(handle_opaque_t handle) = 0;
};

#define MODUCOM_ASSERT(expected, actual)

class Memory : public IMemory
{
public:
    typedef IMemory::handle_opaque_t handle_t;

public:
    /// Allocate a block of memory, but don't lock it yet
    /// @param size size in bytes of memory chunk to allocate
    /// @return
    handle_t allocate(size_t size) OVERRIDE;
    /// Allocate a block of memory, but don't lock it yet
    /// @param data data to copy into newly allocated block
    /// @param size size in bytes of memory chunk to allocate
    /// @param size_copy size in bytes to copy from data, or 0 if it matches 'size'
    /// @return
    handle_t allocate(const void* data, size_t size, size_t size_copy = 0) OVERRIDE;

    /// frees an unlocked memory handle
    /// @param handle memory handle to free
    /// @return false is memory is locked and not freeable
    bool free(handle_t handle) OVERRIDE;

    /// may change handle_t in the process
    /// \param handle
    /// \param new_size
    /// \return
    handle_t realloc(handle_t handle, size_t new_size)
    {
        return (handle_t) ::realloc((void*)handle, new_size);
    }

    // TODO: might manage size differently, but for now we have to be explicit
    /// Copies from one handle to a new handle
    /// \param handle
    /// \param size size of allocation
    /// \param size_copy size of copy must be less than size
    /// \return
    handle_t copy(handle_t handle, size_t size, size_t size_copy = 0);

    void* lock(handle_t handle) { return (void*)handle; }

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

    // cannot safely expand with this approach (it may want to change the "handle")
    virtual bool expand(handle_t handle, size_t size) OVERRIDE
    {
        return false;
    }

    // not 100% gaurunteed, but close enough for diagnostics - realloc I imagine 99.9%
    // of the time keeps the same pointer
    virtual void shrink(handle_t handle, size_t size) OVERRIDE
    {
        realloc(handle, size);
    }

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

        void copy_from(handle_t handle, size_t size)
        {
            memory.free(handle);
            handle = memory.copy(handle, size);
        }


        void copy_from(SmartHandle& sh, size_t size)
        {
            copy_from(sh.handle, size);
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
            size(copy_from.size),
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

