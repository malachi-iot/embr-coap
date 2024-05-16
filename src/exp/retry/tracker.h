// See README.md
#pragma once

#include <estd/algorithm.h>
#include <estd/cstdint.h>
#include <estd/forward_list.h>
#include <estd/functional.h>

#include <embr/scheduler.h>

#include "metadata.h"

namespace embr { namespace coap { namespace experimental { namespace retry {

// A connection tracker of sorts.  Does not participate in any scheduling
// TItem is more or less an impl pattern to help avoid virtual functions
// DEBT: Tracker not threadsafe, but will need to be for FreeRTOS operation
// DEBT: Can probably unwind TTransport and pass endpoint/buffer in directly again
// NOTE: Shaping up to not be CoAP specific, which is long term goal.  Goal may
// be suspended with minimal debt - completion more important than non-CoAP reusability
template <class TTimePoint, class TTransport,
    // DEBT: Might be able to do an Item with TTransport specialization.
    // Remember also this default class is for reference
    class TItem = Item<
        typename TTransport::endpoint_type,
        TTimePoint,
        typename TTransport::const_buffer_type> >
struct Tracker
{
    typedef TTransport transport_type;
    typedef typename transport_type::endpoint_type endpoint_type;
    typedef TTimePoint time_point;
    typedef typename transport_type::buffer_type buffer_type;
    typedef typename transport_type::const_buffer_type const_buffer_type;

    typedef TItem item_type;

    // DEBT: Replace this with a proper memory pool
    // DEBT: Doing this as item_type* because memory location needs to be fixed
    // for estd::detail::function to find its model
    typedef estd::layer1::vector<item_type*, 10> vector_type;

private:
    vector_type tracked;

public:

    typedef typename vector_type::iterator iterator;
    typedef typename vector_type::const_iterator const_iterator;

    const_iterator begin() const { return tracked.begin(); }
    const_iterator end() const { return tracked.end(); }

    bool empty() const { return tracked.empty(); }

    ~Tracker()
    {
        for(item_type* i : tracked)
            delete i;
    }

    // DEBT: Would prefer TArgs prepend, but that breaks typical C++ paradigm
    // for variadic on a method
    template <class ...TArgs>
    const item_type* track(
        const endpoint_type& endpoint,
        time_point time_sent,
        const_buffer_type&& buffer,
        TArgs&&...args)
    {
        /*
        item_type _i{endpoint};
        tracked.push_back(_i);
        const item_type& i = tracked.back();
        return &i; */

        //const item_type& i = tracked.emplace_back(endpoint, time_sent, buffer);
        //return &i;

        auto i = new item_type(endpoint, time_sent, std::move(buffer), 
            std::forward<TArgs>(args)...);
        tracked.push_back(i);
        return i;
    }


    // copy of buffer holder version, not used at the moment
    template <class ...TArgs>
    const item_type* track(
        const endpoint_type& endpoint,
        time_point time_sent,
        const const_buffer_type& buffer,
        TArgs...args)
    {
        // DEBT: Do std::forward
        auto i = new item_type(endpoint, time_sent, buffer, args...);
        tracked.push_back(i);
        return i;
    }

    iterator match(const endpoint_type& endpoint, uint16_t mid)
    {
        auto it = estd::find_if(tracked.begin(), tracked.end(), [&](const item_type* v)
        {
            //return endpoint == v.endpoint() && mid == v.mid();
            return endpoint == v->endpoint() && mid == v->mid();
        });
        return it;
    }

    void untrack(iterator it)
    {
        tracked.erase(it);

        // NOTE: Cannot delete this here because scheduler still wants to pick up model contained
        // in 'it'
        // DEBT: This now means consumer is responsible for GC, which is not thought through
        // at this time
        //delete it.lock();
    }

    void untrack(const item_type* item)
    {
        // DEBT: Add https://en.cppreference.com/w/cpp/algorithm/remove and consider using
        // that here
        auto it = match(item->endpoint(), item->mid());

        if(it == tracked.end())
            return;

        untrack(it);
    }
};

// 13MAY24 MB
// Wholly different approach, a pseudo-state-machine with a baked in scheduler
template <class Endpoint, class Buffer, unsigned N = 10,
    class TimePoint = estd::chrono::duration<uint32_t, estd::milli> >
struct Tracker2
{
    using time_point = TimePoint;

    struct match_param
    {
        const unsigned mid;
        const Endpoint endpoint;
    };

    struct SchedulerItem :
        embr::internal::scheduler::impl::ReferenceBase<time_point>
    {
        struct value_type : Item<Endpoint, TimePoint, Buffer>
        {
            using base_type = Item<Endpoint, TimePoint, Buffer>;

            explicit value_type(time_point t, time_point random,
                const Endpoint& endpoint, Buffer buffer) :
                base_type(endpoint, t, buffer)
            {
            }

            bool match(match_param v) const
            {
                return
                    v.endpoint == base_type::endpoint() &&
                    v.mid == base_type::mid();
            }
        };

        constexpr static time_point get_time_point(const value_type& v)
        {
            return v.first_transmit() + v.delta();
        }

        // true = reschedule requested
        // false = finished
        static bool process(value_type& value, time_point)
        {
            // ACK received immediately queues us for deletion
            if(value.ack_received())    return false;

            ++value.retransmission_counter;
            return value.finished() == false;
        }
    };

    embr::internal::layer1::Scheduler<N, SchedulerItem> scheduler_;

    using value_type = typename SchedulerItem::value_type;

    bool track(time_point t, time_point random, const Endpoint& endpoint, const Buffer& b)
    {
        // DEBT: Return false if we can't schedule (full)
        scheduler_.schedule(t, random, endpoint, b);
        return true;
    }

    bool track(time_point t, const Endpoint& endpoint, const Buffer& b)
    {
        return track(t, typename value_type::milliseconds(500), endpoint, b);
    }


    /// At the exact moment an ACK is encountered, brute force look it up
    /// and mark the tracked item as such.  Later, when process time comes,
    /// the item will be removed
    /// \param endpoint
    /// \param mid
    /// \return
    bool ack_encountered(const Endpoint& endpoint, uint16_t mid)
    {
        // DEBT: Brute force search
        value_type* matched = scheduler_.match(
            match_param{mid, endpoint});

        if(matched)
        {
            matched->ack_received(true);
        }

        return matched;
    }

    bool empty() const { return scheduler_.empty(); }

    // DEBT: (light debt) cascade this downward into estd::layer1::vector, embr::layer1::Scheduler, etc
    bool full() const
    {
        return scheduler_.size() == N;
    }

    value_type& top() { return scheduler_.top(); }

    time_point top_time() const { return scheduler_.top_time(); }

    value_type* ready(time_point current)
    {
        if(!scheduler_.empty() && current >= scheduler_.top_time())
            return &top();
        else
            return nullptr;
    }

    /// For a retry item at the ready position,
    /// mark it sent, bump retry counter and reprocess
    /// @return false if nothing present to process, true if we process 'ready'
    bool mark_con_sent(time_point current)
    {
        value_type* top = ready(current);

        if(top == nullptr) return false;

        top->con_helper_flag_bit = 1;

        // Doing this inside "process" itself because that's the only appropriate place
        // to specify a new time stamp for scheduler to pick it up
        //++top->retransmission_counter;

        // FIX: Still squirrely, coming along though
        return scheduler_.process_one(current);
    }

    bool mark_con_sent()
    {
        return mark_con_sent(scheduler_.top_time());
    }


    bool mark_ack_processed(time_point current)
    {
        value_type* top = ready(current);

        if(top == nullptr)  return false;

        if(top->ack_received() == false)   return false;

        return scheduler_.process_one(current);
    }

    bool mark_ack_processed()
    {
        return mark_ack_processed(scheduler_.top_time());
    }

    // EXPERIMETNAL, don't think I like this one -- too implicit
    bool mark()
    {
        return scheduler_.top().ack_received() ?
            mark_ack_processed() :
            mark_con_sent();

    }
};


}}}}
