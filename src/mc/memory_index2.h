#pragma once

#include "../platform.h"
#include "../MemoryPool.h"

namespace moducom { namespace dynamic {


#ifdef USE_PRAGMA_PACK
#pragma pack(1)
#endif


struct PACKED MemoryPoolIndexed2HandlePage : MemoryPoolHandlePage
{
    /// Actual handle(s) residing within system page 0
    struct PACKED Index2Handle
    {
        /// which page this handle points to.  0 is reserved always for system handle
        /// descriptor area, and therefore is repurposed as an "inactive" indicator
        uint8_t page;

        bool is_active() const { return page != 0; }

        // header at the beginning of a non-system page (1-255)
        // only used for ACTIVE Index2Handle
        struct PACKED PageData
        {
            bool locked : 1;
            bool allocated : 1;

            /// # of utilized pages valid values from 1-255
            uint8_t size;

            /// Initialize an active but unlocked and unallocated
            /// page
            PageData(uint8_t size)
            {
                locked = false;
                allocated = false;
                this->size = size;
            }

            PageData(const PageData& copy_from)
            {
                *this = copy_from;
            }
        };
    };

    typedef Index2Handle handle_t;
    typedef IMemory::handle_opaque_t handle_opaque_t;

private:
    handle_t indexedHandle[];

public:
    const handle_t& get_descriptor(uint8_t handle) const
    {
        return indexedHandle[handle];
    }

    /// initialize with total byte count of page_size
    /// @param blank_page Page 1 from memory pool - note this should *already* have its size field initialized
    void initialize(size_t page_size, handle_t::PageData* blank_page)
    {
        header.tier = IMemory::Indexed2;
        header.size = 1;

        page_size -= sizeof(MemoryPoolHandlePage);

        // NOTE: Just a formality, don't need to do a sizeof since it's exactly one byte
        // doing it anyway cuz should optimize out and saves us if we do have to increase
        // size of handle_t
        for(size_t i = 1; i < page_size / sizeof(handle_t); i++)
        {
            indexedHandle[i].page = 0;
        }

        indexedHandle[0].page = 1;

        blank_page->allocated = false;
        blank_page->locked = false;
    }


    uint8_t get_page(handle_opaque_t handle) const
    {
        uint8_t page = get_descriptor(handle).page;
        return page;
    }

    size_t get_approximate_header_size() const
    {
        return header.size;
// TODO: re-enable this approximator, we need an algorithm to create the initial
// approximation.  Probably evaluate what the header.size currently is, then see if
// it's grown past its threshold value
// during the ALLOCATION operation, we do have an exact value in which we found the
// new handle(s) so that's a key thing to hang it off of
        //return 1 << (size_t) header.size;
    }

    typedef void (*page_data_iterator_fn)(void* context, handle_opaque_t handle, uint8_t page);

    void iterate_page_data(page_data_iterator_fn callback, void* context = NULLPTR) const
    {
        size_t count = get_approximate_header_size();

        for(size_t i = 0; i < count; i++)
        {
            const handle_t& h = indexedHandle[i];

            if(h.is_active()) callback(context, i, h.page);
        }
    }

#ifdef __CPP11__
#endif


    // FIX: temporarily passing in page_size until we settle down
    // get_approximate_header_size() behavior
    handle_opaque_t get_first_inactive_handle(size_t page_size) const
    {
        // always reflects more elements than are actually
        // present to optimize lookups in large page sizes
        // (remember each handle we can ascertain as active
        // by itself - by it being page == 0 or not)
        const size_t size_approximate = get_approximate_header_size();

        //for(size_t i = 0; i < size_approximate; i++)
        for(size_t i = 0; i < page_size / sizeof(handle_t); i++)
        {
            if(!get_descriptor(i).is_active())
                return i;
        }

        return IMemory::invalid_handle;
    }

    //!
    //! \param handle
    //! \param pages 0-based, inclusive of system page
    //! \param page_size
    //! \return
    handle_t::PageData& get_page_data(handle_opaque_t handle, uint8_t* pages, size_t page_size)
    {
        ASSERT(true, pages != NULLPTR);

        const handle_t& descriptor = get_descriptor(handle);

        ASSERT(true, descriptor.is_active());

        handle_t::PageData* p = reinterpret_cast<handle_t::PageData*>(
                pages + (page_size * descriptor.page));

        return *p;
    }

    // TBD
    // doesn't assign page size just yet
    // see if we can consolidate with above one
    handle_t::PageData& new_page_data(handle_opaque_t new_handle, uint8_t page, uint8_t* pages, size_t page_size)
    {
        handle_t& descriptor = indexedHandle[new_handle];

        descriptor.page = page;

        handle_t::PageData* p = reinterpret_cast<handle_t::PageData*>(
                pages + (page_size * descriptor.page));

        return *p;
    }

    //!
    //! \param handle
    //! \param pages
    //! \param page_size
    //! \return
    void* lock(handle_opaque_t handle, uint8_t* pages, size_t page_size)
    {
        handle_t::PageData& page_data = get_page_data(handle, pages, page_size);

        ASSERT(true, page_data.allocated);
        ASSERT(false, page_data.locked);

        page_data.locked = true;

        return (uint8_t *)(&page_data) + sizeof(handle_t::PageData);
    }

    void unlock(handle_opaque_t handle, uint8_t* pages, size_t page_size)
    {
        handle_t::PageData& page_data = get_page_data(handle, pages, page_size);

        ASSERT(true, page_data.allocated);
        ASSERT(true, page_data.locked);

        page_data.locked = false;
    }

    void free(handle_opaque_t handle, uint8_t* pages, size_t page_size)
    {
        handle_t::PageData& page_data = get_page_data(handle, pages, page_size);

        ASSERT(true, page_data.allocated);
        ASSERT(false, page_data.locked);

        page_data.allocated = false;
    }

    /**!
     * Scouring through active and unallocated handles, find one whose minimum size meets our requirement
     * Does a best-fit match
     * @param minimum
     * @param pages
     * @param page_size
     * @param page_data
     * @return
     */
    handle_opaque_t  get_unallocated_handle(size_t minimum, uint8_t* pages, size_t page_size, handle_t::PageData** page_data)
    {
        const size_t size_approximate = get_approximate_header_size();
        handle_t::PageData* candidate = NULLPTR;
        int candidate_index;

        for(size_t i = 0; i < size_approximate; i++)
        {
            const handle_t& descriptor = get_descriptor(i);

            if(descriptor.is_active())
            {
                handle_t::PageData* p = reinterpret_cast<handle_t::PageData*>(
                        pages + (page_size * descriptor.page));

                if(!p->allocated && p->size >= minimum)
                {
                    if(candidate != NULLPTR)
                    {
                        if(p->size < candidate->size)
                        {
                            candidate = p;
                            candidate_index = i;
                        }
                    }
                    else
                    {
                        candidate = p;
                        candidate_index = i;
                    }
                }
            }
        }

        if(candidate == NULLPTR) return IMemory::invalid_handle;

        *page_data = candidate;

        return candidate_index;
    }

    size_t get_total_unallocated_bytes(const uint8_t* pages, size_t page_size) const
    {
        typedef const handle_t::PageData page_data_t;
        const size_t size_approximate = get_approximate_header_size();
        size_t total = 0;

        for(size_t i = 0; i < size_approximate; i++)
        {
            const handle_t& descriptor = get_descriptor(i);

            if(descriptor.is_active())
            {
                page_data_t* p = reinterpret_cast<page_data_t*>(
                        pages + (page_size * descriptor.page));

                if(!p->allocated) total += (p->size * page_size) - sizeof(page_data_t);
            }
        }

        return total;
    }

};


#ifdef USE_PRAGMA_PACK
#pragma pack()
#endif

}}
