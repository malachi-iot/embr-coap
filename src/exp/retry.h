#pragma once

#include "coap/platform.h"
#include <estd/forward_list.h>
#include <estd/vector.h>
#include <estd/queue.h>
#include <mc/memory-pool.h>
#include "coap/decoder/netbuf.h"
#include <stdint.h> // for uint8_t
#include "datapump-observer.h" // for IDataPumpObserver

#if defined(__unix__) || defined(__POSIX__) || defined(__MACH__)
#include <sys/time.h>
#endif

namespace moducom { namespace coap { namespace experimental {

// FIX: Much more likely FRAB is a place for this, stuffing this in for now
// so that we can be resilient with our time handling + facilitate unit tests
struct TimePolicy
{
    // FIX: We're gonna need a proper mapping for platform specific timing, though
    // merely tracking as milliseconds might be enough
    typedef uint32_t time_t;

    // FIX: convenience function since there isn't quite a unified cross platform time acqusition fn
    // specifically this retrieves number of milliseconds since a fixed point in time
    static time_t now()
    {
#if defined(__unix__) || defined(__POSIX__) || defined(__MACH__)
        // NOTE: this doesn't seem to be working right.  milli is returning values like 134, 14, etc.
        timeval curTime;
        gettimeofday(&curTime, NULL);
        int milli = curTime.tv_usec / 1000;
        return milli;
#else
#error Needs implementation for this platform
#endif
    }
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

// only CON messages live here, expected to be shuffled here right out of datapump
// they are removed from our retry_list when an ACK is received, or when our backoff
// logic finally expires
// in support of https://tools.ietf.org/html/rfc7252#section-4.2
template <class TNetBuf, class TAddr, class TPolicy = RetryPolicy<> >
class Retry
{
public:
    typedef TAddr addr_t;

    typedef typename TPolicy::time time_traits;
    typedef typename TPolicy::random random_policy;
    typedef typename time_traits::time_t time_t;
    typedef coap::address_traits<addr_t> address_traits;
    typedef TNetBuf netbuf_t;

    struct AlwaysConsumeNetbuf : IDataPumpObserver<netbuf_t, addr_t>
    {


        // IDataPumpObserver interface
    public:
        virtual void on_message_transmitting(netbuf_t *netbuf, const addr_t &addr) OVERRIDE
        {}

        virtual bool on_message_transmitted(netbuf_t *netbuf, const addr_t &addr) OVERRIDE
        {
            return true;
        }
    };

    AlwaysConsumeNetbuf always_consume_netbuf;

    struct Metadata
    {
        // from 0-4 (COAP_MAX_RETRANSMIT)
        // NOTE: Technically at this time this actually represents *queued for transmit* and may
        // or may not reflect whether it has actually been transmitted yet
        uint16_t retransmission_counter : 3;

        // represents in milliseconds initial timeout as described in section 4.2.
        // This is  between COAP_ACK_TIMEOUT and COAP_ACK_TIMEOUT*COAP_ACK_RANDOM_FACTOR
        // which for this variable works out to be 2000-3000
        uint16_t initial_timeout_ms : 12;

        // helper bit to help us avoid decoding outgoing message to see if it's really
        // a CON message.  Or, in otherwords, a cached value indicating outgoing message
        // REALLY is CON.
        // value: 1 = definitely a CON message
        // value: 0 = maybe a CON message, decode required
        uint16_t con_helper_flag_bit : 1;

        //bool is_definitely_con() { return con_helper_flag_bit; }
        // TODO: an optimization, and we aren't there quite yet
        bool is_definitely_con() { return false; }

        // TODO: optimize.  Consider making initial_timeout_ms actually at
        // 10-ms resolution, which should be fully adequate for coap timeouts
        uint32_t delta()
        {
            // double initial_timeout_ms with each retransmission
            uint16_t multiplier = 1 << retransmission_counter;

            return initial_timeout_ms * multiplier;
        }
    };

    // proper MID and Token are buried in netbuf, so don't need to be carried
    // seperately
    // TODO: Utilize ObservableSession as a base once we resolve netbuf-inline behaviors here
    struct Item :
            Metadata
    {
        typedef Metadata base_t;

        // where to send retry
        addr_t addr;

        // what to send
        // right now hard-wired to non-inline netbuf style
        TNetBuf* m_netbuf;

        // when to send it by
        time_t due;

        bool ack_received;

        TNetBuf& netbuf() const { return *m_netbuf; }

    protected:
        Header header() const
        {
            // TODO: optimize and use header decoder only and directly
            NetBufDecoder<TNetBuf&> decoder(netbuf());

            return decoder.header();
        }

        Retry* parent;

    public:
        // get MID from sent netbuf, for incoming ACK comparison
        uint16_t mid() const
        {
            return header().message_id();
        }

        // get Token from sent netbuf, for incoming ACK comparison
        // TODO: Make a layer3::Token which can lean on netbuf contents,
        // as right now netbuf has a requirement which it must at least support
        // the first 12 bytes (head + token) without fragementation
        coap::layer2::Token token() const
        {
            // TODO: optimize and use header & token decoder only and directly
            NetBufDecoder<TNetBuf&> decoder(netbuf());
            coap::layer2::Token token;

            decoder.header();
            decoder.process_token_experimental(&token);

            return token;
        }

        // needed for unallocated portions of vector
        Item() {}

        Item(Retry* parent, const addr_t& a, TNetBuf* netbuf) :
                m_netbuf(netbuf),
                addr(a),
                parent(parent)
        {
            this->retransmission_counter = 0;
            this->initial_timeout_ms =
                    random_policy::rand(
                            1000 * COAP_ACK_TIMEOUT,
                            1000 * COAP_ACK_RANDOM_FACTOR * COAP_ACK_TIMEOUT);
        }

    public:
        bool operator > (const Item& compare_to) const
        {
            return due > compare_to.due;
        }
    };

private:
    //Using raw linked list pool for now until we get a better sorted
    // solution in place
    typedef moducom::mem::LinkedListPool<int, 10> llpool_t;
    //typedef moducom::mem::LinkedListPoolNodeTraits<int, 10> node_traits_t;

   typedef estd::layer1::vector<Item, 10> list_t;
    typedef estd::priority_queue<Item, list_t, std::greater<Item> > priority_queue_t;

    // sneak in and peer at container
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

    RetryQueue retry_queue;

    static Header header(TNetBuf& netbuf)
    {
        // TODO: optimize and use header decoder only and directly
        NetBufDecoder<TNetBuf&> decoder(netbuf);

        return decoder.header();
    }

public:
    Retry() {}

    static bool is_con(TNetBuf& netbuf)
    {
        return header(netbuf).type() == Header::Confirmable;
    }

    // allocate a not-yet-sent retry slot
    void enqueue(TNetBuf& netbuf, const addr_t& addr)
    {
        // TODO: ensure it's sorted by 'due'
        // for now just brute force things
        Item item(this, addr, &netbuf);

        // FIX: Really need to set due either during service call or on
        // on_transmitted call.  Setting here is just a stop gap and will
        // have innacurate side effects for first resend since at this point
        // outgoing packet isn't even queued in datapump yet
        item.due = time_traits::now() + item.delta();

        // TODO: revamp the push_back code to return success or fail
        retry_queue.push(item);
    }

    bool empty() const { return retry_queue.empty(); }

    Item& front() const
    {
        // FIX: want accessor to be a little more transparent
        return retry_queue.top().lock();
    }


    // called when ACK is received to determine if we should remove anything from the
    // retry_queue
    void ack_received(const addr_t& from_addr, uint16_t mid, const coap::layer2::Token& token)
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
                // NOTE: probably more efficient to let it just sit
                // in priority queue with a 'kill' flag on it
                retry_queue.erase(i);
                return;
            }

            ++i;
        }
    }

    void service_ack(netbuf_t& netbuf, const addr_t& addr)
    {
        // TODO: optimize and use header decoder only and directly
        NetBufDecoder<TNetBuf&> decoder(netbuf);

        Header h = decoder.header();
        coap::layer2::Token token;
        decoder.process_token_experimental(&token);

        if(h.type() == Header::Acknowledgement)
        {
            ack_received(addr,
                         h.message_id(),
                         token);
        }
    }

    // a bit problematic because it's conceivable someone will eat the ack before we get it,
    // so for now be sure to call service_ack before calling others.  Problematic also because
    // of redundant header/token decoding
    template <class TDataPump>
    void service_ack(TDataPump& datapump)
    {
        if(datapump.dequeue_empty()) return;

        typedef typename TDataPump::Item item_t;

        // we specifically are *not* poping this
        item_t& item = datapump.dequeue_front();
        netbuf_t& netbuf = *item.netbuf();

        service_ack(netbuf, item.addr());
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
                // it back again
                Item f = front();

                // and if we're still interested in retransmissions
                // (retry #1 and retry #2) - going to be counter values 0 and 1
                // since we're *about* to issue the retransmit
                if(f.retransmission_counter < COAP_MAX_RETRANSMIT - 2)
                {
                    // Will need to maintain some kind of signal / capability
                    // to keep taking ownership of netbuf so that it doesn't
                    // get deleted until we're done with our resends (got ACK
                    // or == 3 retransmission)
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
                    datapump.enqueue_out(std::forward<netbuf_t>(f.netbuf()), f.addr, &always_consume_netbuf);
#else
                    datapump.enqueue_out(f.netbuf(), f.addr, &always_consume_netbuf);
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
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
                    datapump.enqueue_out(std::forward<netbuf_t>(f.netbuf()), f.addr);
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


}}}
