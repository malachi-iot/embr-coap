/**
 *  @file
 */
#pragma once

#include "coap/platform.h"
#include <estd/forward_list.h>
#include <estd/vector.h>
#include <estd/queue.h>
#include <estd/chrono.h>
#include <estd/functional.h>
//#include <mc/memory-pool.h>
//#include "coap/decoder/netbuf.h"
#include <stdint.h> // for uint8_t

//#include <embr/datapump.h>
//#include <embr/events.h>
#include <embr/exp/pbuf.h>

#include "retry/metadata.h"
#include "retry/scheduler.h"
#include "retry/manager.h"

#if defined(__unix__) || defined(__POSIX__) || defined(__MACH__)
#include <sys/time.h>
#endif

namespace embr { namespace coap { namespace experimental {

struct TimePolicy
{
    typedef estd::chrono::steady_clock clock;

    typedef clock::time_point time_t;

    // FIX: convenience function since there isn't quite a unified cross platform time acqusition fn
    // specifically this retrieves number of milliseconds since a fixed point in time
    static time_t now() { return clock::now(); }
};

// eventually use http://en.cppreference.com/w/cpp/numeric/random
struct RandomPolicy
{
    // if !is_stateful, that means random seed is global
    static CONSTEXPR bool is_stateful() { return false; }
    static CONSTEXPR int max() { return RAND_MAX; }
    static int rand() { return std::rand(); }
    static void seed(unsigned s) { std::srand(s); }
    // as per http://en.cppreference.com/w/cpp/experimental/randint
    // even if it's not available, we can make it available
    template <class IntType>
    static IntType rand(IntType lower_bound, IntType upper_bound)
    {
        // for now just brute force through it, but eventually lean on
        // either/or experimental/random randint or uniform_int_distribution
        IntType delta = upper_bound - lower_bound;
        CONSTEXPR int m = max();
        // macOS doesn't seem to have an even distribution of random
        // (all very low in the RAND_MAX range), so using technique
        // suggested here https://forums.macrumors.com/threads/random-numbers-for-c.176521/
        //long result = std::rand();
        //result *= delta;
        //result /= m;
        int result = std::rand() % delta;
        return result + lower_bound;
    }
};


template <class TTimePolicy = TimePolicy, class TRandomPolicy = RandomPolicy>
struct RetryPolicy
{
    typedef TTimePolicy time;
    typedef TRandomPolicy random;
};

// uses move semantic so that someone always owns the netbuf, somewhat
// sidestepping shared_ptr/ref counters (even though implementations
// like PBUF have inbuilt ref counters)
//#define FEATURE_MCCOAP_RETRY_INLINE
// ^^ disabled because the priority_queue stuff needs copy/swap capabilities
//    for the Item holding netbuf

#if defined(FEATURE_MCCOAP_RETRY_INLINE)
#if !defined(FEATURE_CPP_MOVESEMANTIC)
#error "Move semantic required for inline retry mode"
#endif

#ifndef FEATURE_EMBR_DATAPUMP_INLINE
#error "embr inline datapump should also be on when using inline retry"
#endif

#endif

/**
 *
 *  @details
 *
 * only CON messages live here, expected to be shuffled here right out of datapump
 * they are removed from our retry_list when an ACK is received, or when our backof
 * logic finally expires
 * in support of https://tools.ietf.org/html/rfc7252#section-4.2
 **/
template <
        class TTransportDescriptor,
        class TPolicy = RetryPolicy<> >
class Retry
{
public:
    typedef TTransportDescriptor transport_descriptor_t;
    // TODO: Use TTransportDescriptor instead of discrete TNetBuf and TAddr
    typedef typename transport_descriptor_t::addr_t addr_t;

    typedef typename TPolicy::time time_traits;
    typedef typename TPolicy::random random_policy;
    typedef typename time_traits::time_t time_t;
    typedef embr::experimental::address_traits<addr_t> address_traits;
    typedef typename transport_descriptor_t::netbuf_type netbuf_t;

private:
    ///
    /// \brief does a one-shot decode of netbuf just to extract header
    ///
    /// One can reasonably expect the decoder itself to be very efficient.  netbuf efficiency
    /// is not under control of decoder
    ///
    /// \param netbuf
    /// \return acquired header
    ///
    static Header header(netbuf_t& netbuf)
    {
        // TODO: optimize and use header decoder only and directly
        NetBufDecoder<netbuf_t&> decoder(netbuf);

        return decoder.header();
    }

public:
    ///
    /// \brief Underlying data associated with any potential-retry item
    ///
    struct Metadata : retry::Metadata
    {
        //bool is_definitely_con() { return con_helper_flag_bit; }
        // TODO: an optimization, and we aren't there quite yet
        bool is_definitely_con() { return false; }
    };

    // proper MID and Token are buried in netbuf, so don't need to be carried
    // seperately
    // TODO: Utilize ObservableSession as a base once we resolve netbuf-inline behaviors here
    ///
    /// \brief Struct representing possibly-retrying outgoing message
    ///
    /// if 'due' deadline is exceeded
    ///
    struct Item :
            Metadata
    {
        typedef Metadata base_t;

        // where to send retry
        addr_t addr;

#ifdef FEATURE_MCCOAP_RETRY_INLINE
        // Doing this to sidestep lack of copy constructors (so that sorting can work)
        // Could be dangerous!
        // I don't think so though
        estd::experimental::raw_instance_provider<netbuf_t> m_raw_netbuf;
        //netbuf_t m_netbuf;

        // FIX: brute forcing constness, for now.  Obviously that's bad
        netbuf_t& netbuf() const { return (netbuf_t&)m_raw_netbuf.value(); }
        //const netbuf_t& netbuf() const { return m_raw_netbuf.value(); }
#else
        // what to send
        // right now hard-wired to non-inline netbuf style.  May have implications
        // for memory management of netbuf - who destructs/deallocates it?
        netbuf_t* m_netbuf;

        netbuf_t& netbuf() const { return *m_netbuf; }
#endif

        // when to send it by.  absolute time
        time_t due;

        bool ack_received;

    protected:
        // NOTE: Not currently used, and would be nice to get rid of if we can
        Retry* parent;

    public:
        // get MID from sent netbuf, for incoming ACK comparison
        uint16_t mid() const
        {
            return header(netbuf()).message_id();
        }

        // get Token from sent netbuf, for incoming ACK comparison
        // TODO: Make a layer3::Token which can lean on netbuf contents,
        // as right now netbuf has a requirement which it must at least support
        // the first 12 bytes (head + token) without fragementation
        coap::layer2::Token token() const
        {
            // TODO: optimize and use header & token decoder only and directly
            NetBufDecoder<netbuf_t&> decoder(netbuf());
            coap::layer2::Token token;

            decoder.header();
            decoder.token(&token);

            return token;
        }

        // needed for unallocated portions of vector
        Item() {}

        Item(Retry* parent, const addr_t& a,
#ifdef FEATURE_MCCOAP_RETRY_INLINE
                netbuf_t&& netbuf) :
                // FIX: that particular provider still needs a constructor
                //m_raw_netbuf(std::move(netbuf)),
#else
                netbuf_t* netbuf) :
                m_netbuf(netbuf),
#endif
                addr(a),
                parent(parent)
        {
#ifdef FEATURE_MCCOAP_RETRY_INLINE
            new (m_raw_netbuf.buf) netbuf_t(std::move(netbuf));
#endif
            this->retransmission_counter = 0;
            this->initial_timeout_ms =
                    random_policy::rand(
                            1000 * COAP_ACK_TIMEOUT,
                            1000 * COAP_ACK_RANDOM_FACTOR * COAP_ACK_TIMEOUT);
        }

    public:
        // used primarily for priority_queue to sort these items by
        // eventually use an explicit static sort method as it is
        // more explicit requiring less guesswork when glancing at code
        bool operator > (const Item& compare_to) const
        {
            return due > compare_to.due;
        }
    };

private:
    //Using raw linked list pool for now until we get a better sorted
    // solution in place

    // TODO: make this policy-based also so that we can vary size and implementation of
    // actual data container for retry queue
    typedef estd::layer1::vector<Item, 10> list_t;
    typedef estd::priority_queue<Item, list_t, estd::greater<Item> > priority_queue_t;

    ///
    /// \brief Enhanced priority queue
    ///
    /// is-a queue so that we have visibility into underlying container
    struct RetryQueue : priority_queue_t
    {
        typedef priority_queue_t base_t;
        typedef typename base_t::container_type container_type;

        container_type& get_container() { return base_t::c; }

        void erase(typename container_type::const_iterator it)
        {
            base_t::c.erase(it);
            std::make_heap(base_t::c.begin(), base_t::c.end(), base_t::get_compare());
        }
    };

    /// \brief main retry queue sorted based on 'due'
    RetryQueue retry_queue;

public:
    Retry() {}

    // NOTE: Not used and likely something like
    // 'header(netbuf).is_confirmable' would be better
    static bool is_con(netbuf_t& netbuf)
    {
        return header(netbuf).type() == Header::Confirmable;
    }

    // allocate a not-yet-sent retry slot, inclusive of tracking
    // netbuf and addr and due time
#ifdef FEATURE_MCCOAP_RETRY_INLINE
    void enqueue(netbuf_t&& netbuf, const addr_t& addr)
    {
        // TODO: ensure it's sorted by 'due'
        // for now just brute force things
        Item item(this, addr, std::move(netbuf));
#else
    void enqueue(netbuf_t& netbuf, const addr_t& addr)
    {
        // TODO: ensure it's sorted by 'due'
        // for now just brute force things
        Item item(this, addr, &netbuf);
#endif

        // FIX: Really need to set due either during service call or on
        // on_transmitted call.  Setting here is just a stop gap and will
        // have innacurate side effects for first resend since at this point
        // outgoing packet isn't even queued in datapump yet
        item.due = time_traits::now() + item.delta();

        // TODO: revamp the push_back code to return success or fail
        retry_queue.push(item);
    }

    /// \brief returns whether or not there are any items in the retry queue
    /// \return
    bool empty() const { return retry_queue.empty(); }


    /// \brief returns first item from retry queue
    ///
    /// This shall be the soonest due item
    Item& front() const
    {
        // FIX: want accessor to be a little more transparent
        return retry_queue.top().lock();
    }


    /// \brief given address, mid and token see if we're tracking this message
    /// \param from_addr
    /// \param mid
    /// \param token
    /// \return returns iterator to found tracked item
    /// \remarks we may end up using this to aid in shuffling ownership back and forth
    /// between Retry and Datapump of netbuf in an attempt to sidestep using shared_ptr
    /// or similar (we'd semi-reenque a just-sent item if already queued here)
    estd::optional<typename list_t::iterator> match(
            const addr_t& from_addr,
            uint16_t mid,
            const coap::layer2::Token& token)
    {
        typename RetryQueue::container_type&
                c = retry_queue.get_container();
        typename list_t::iterator i = c.begin();

        while(i != c.end())
        {
            Item& v = *i; // FIX: doing *i++ here blows up, do we have a bug in the postfix iterator?

            uint16_t v_mid = v.mid();
            const coap::layer2::Token& v_token = v.token();

            // address, token and mid all have to match
            // FIX: it's quite probable address won't exactly match because our address
            // for UDP carries the source port, which can actually vary.  Until we iron that
            // out, only compare against token and mid - this will work in the short term but
            // will ultimately fail when multiple IPs are involved
            if(
                    address_traits::equals_fromto(from_addr, v.addr) &&
                    v_token == token && v_mid == mid)
            {
                estd::optional<typename list_t::iterator> matched;

                matched = i;
                // FIX: lingering issue, can't return i directly yet
                return matched;
            }

            ++i;
        }

        return estd::nullopt;
    }

    // NOTE: should be enabled by movesemantic, not by this feature flag
#ifdef FEATURE_MCCOAP_RETRY_INLINE
    /// \brief Called after we've sent the netbuf
    ///
    /// semi-requeues the netbuf into our list.  We already are tracking it actually,
    /// but this specifically invokes the move semantics so that we have just one
    /// truly 'active' netbuf in memory - kind of an implicit unique_ptr scenario
    ///
    /// \param netbuf
    /// \param to_addr
    /// \return
    bool con_sent_experimental(netbuf_t&& netbuf, const addr_t& to_addr)
    {
        NetBufDecoder<netbuf_t&> decoder(netbuf);

        Header h = decoder.header();
        coap::layer2::Token token;
        decoder.token(&token);

        estd::optional<typename list_t::iterator> matched =
                match(to_addr, h.message_id(), token);

        if(matched)
        {
            typename list_t::iterator i = *matched;
            Item& item = *i;
            netbuf_t& nb = item.netbuf();

            // move netbuf ownership back into existing slot
            new (&nb) netbuf_t(std::move(netbuf));
            return true;
        }

        return false;
    }
#endif

    ///
    /// \brief called when ACK is received to determine if we should remove anything from the retry_queue
    ///
    /// Remember, ACK means we don't want to retry any more
    ///
    /// \param from_addr
    /// \param mid
    /// \param token
    /// \return true if we found and removed item from our queue, false otherwise
    ///
    bool ack_received(const addr_t& from_addr, uint16_t mid, const coap::layer2::Token& token)
    {
        estd::optional<typename list_t::iterator> matched = match(from_addr, mid, token);

        if(matched)
        {
            // NOTE: probably more efficient to let it just sit
            // in priority queue with a 'kill' flag on it
            retry_queue.erase(*matched);
            return true;
        }

        return false;
    }


    ///
    /// \brief evaluates an incoming netbuf to see if it has an ack
    ///
    /// If so, call ack_received to see if it matches an addr+mid+token that
    /// we are tracking
    ///
    /// \param netbuf
    /// \param addr
    /// @return if incoming ACK matched one we are tracking (and we no longer will attempt to retry it)
    ///
    bool service_ack(netbuf_t& netbuf, const addr_t& addr)
    {
        // TODO: optimize and use header decoder only and directly
        NetBufDecoder<netbuf_t&> decoder(netbuf);

        Header h = decoder.header();
        coap::layer2::Token token;
        decoder.token(&token);

        if(h.type() == Header::Acknowledgement)
        {
            return ack_received(addr,
                         h.message_id(),
                         token);
        }

        return false;
    }

    // a bit problematic because it's conceivable someone will eat the ack before we get it,
    // so for now be sure to call service_ack before calling others.  Problematic also because
    // of redundant header/token decoding
    template <class TDataPump>
    /// @return if incoming ACK matched one we are tracking (and we no longer will attempt to retry it)
    bool service_ack(TDataPump& datapump)
    {
        if(datapump.dequeue_empty()) return false;

        typedef typename TDataPump::Item item_t;

        // we specifically are *not* popping this, as we are not 100%
        // sure if ACK actually is serviced by us.  It's likely
        // though this is the only ACK processor in the system, in which
        // case we DO want to pop it if we get a match
        item_t& item = datapump.dequeue_front();
        netbuf_t& netbuf = *item.netbuf();

        // for now, caller is responsible for popping the item -
        return service_ack(netbuf, item.addr());
    }

    // call this after front() is called and its contained 'due' has passed
    // will:
    //   a) evaluate first (front()) item to see if current_time > due and
    //      1. if so, requeue for later retry attempt again.  If on MAX_RETRANSMIT, then
    //         deactivate this particular retry item as it has run its course
    //      2. if not, do nothing
    //   b)
    template <class TDataPump>
    void service_retry(time_t current_time, TDataPump& datapump)
    {
        // anything in the retry list has already been vetted to be CON
        if(!empty())
        {
            // if it's time for a retransmit
            if(current_time >= front().due)
            {
                // doing a copy here because we're going to need to pop and push
                // it back again to instigate a resort of this item
#ifdef FEATURE_MCCOAP_RETRY_INLINE
                // NOTE: We can actually clean this up, since we're only
                // using f specifically to get at netbuf until we then
                // want to really make a new-ish one, in which case maybe even an
                // emplace can work for us
                Item f = std::move(front());
#else
                Item f = front();
#endif

                // and if we're still interested in retransmissions
                // (retry #1 and retry #2) - going to be counter values 0 and 1
                // since we're *about* to issue the retransmit
                if(f.retransmission_counter < COAP_MAX_RETRANSMIT - 2)
                {
                    // Will need to maintain some kind of signal / capability
                    // to keep taking ownership of netbuf so that it doesn't
                    // get deleted until we're done with our resends (got ACK
                    // or == 3 retransmission)
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
                    // FIX: The problem is that moving out this netbuf means that
                    // message_id and Header aren't available now - unless we
                    // copy them out separately.  Would rather do ref counters than that
                    datapump.enqueue_out(std::move(f.netbuf()), f.addr);
#else
                    datapump.enqueue_out(f.netbuf(), f.addr);
#endif
                    // NOTE: Just putting this here to keep things consistent - so that
                    // retransmissio_counter *really does* represent that netbuf is queued
                    f.retransmission_counter++;

                    // effectively reschedule this item
                    f.due = current_time + f.delta();

                    retry_queue.pop();
                    retry_queue.push(f);
                }
                // or if this is our last retransmission, queue without observer and remove
                // our Item for retry list
                else if(f.retransmission_counter == COAP_MAX_RETRANSMIT - 2)
                {
                    // last retry attempt has no observer, which means datapump fully owns
                    // netbuf, which means it will be erased normally
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
                    datapump.enqueue_out(std::move(f.netbuf()), f.addr);
#else
                    datapump.enqueue_out(f.netbuf(), f.addr);
#endif
                    retry_queue.pop();
                }
                else
                {
                    //ERROR: Should never get here

                    retry_queue.pop();
                }
            }
        }
    }


    template <class TDataPump>
    void service(TDataPump& datapump)
    {
        service_retry(time_traits::now(), datapump);
    }
};

// FIX: 09DEC22 MB Adding in this FEATURE_EMBR_TRANSPORT_EVENTS guard, but no such feature
// exists.  During an embr cleanup phase I removed events since they were attached to
// the obsolete datapump and dataport mechanism.  I plan to bring a similar creature back,
// likely in the context of transport absractions.
#if FEATURE_EMBR_TRANSPORT_EVENTS
// TRetry = retry manager.  can be inline or a ref whatever is convenient
template <class TRetry>//, class TDataport>
class RetryObserver
{
    typedef typename estd::remove_reference<TRetry>::type retry_type;
    typedef typename retry_type::transport_descriptor_t transport_descriptor_t;
    //typedef TTransportDescriptor transport_descriptor_t;
    typedef typename embr::event::Transport<transport_descriptor_t>::transport_sent transport_sent;
    //typedef embr::event::ReceiveDequeued<

    //typedef typename TDataport::event event;

    TRetry retry_manager;

public:
    // transport events are wrong, we want dataport events so that we can precisely time
    // the netbuf move.  transport_sent probably is OK too... feel better about dataport
    // event
    // WAIT: never mind, transport_sent is only ever sent by dataport, and at the exact
    // time.
    void on_notify(transport_sent e)
    {
        if(retry_type::is_con(e.netbuf))
        {
#ifdef FEATURE_MCCOAP_RETRY_INLINE
            retry_manager.enqueue(std::move(e.netbuf), e.addr);
#else
            // WARN: Not tested, design not thought out for this context
            retry_manager.enqueue(e.netbuf, e.addr);
#endif
        }
    }
};
#endif

}}}
