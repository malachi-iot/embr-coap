#pragma once

#include "coap/platform.h"
#include <estd/forward_list.h>
#include <estd/vector.h>
#include <mc/memory-pool.h>
#include "coap/decoder/netbuf.h"
#include <stdint.h> // for uint8_t
#include "datapump-observer.h" // for IDataPumpObserver

#if defined(__POSIX__) || defined(__MACH__)
#include <sys/time.h>
#endif

namespace moducom { namespace coap { namespace experimental {


// only CON messages live here, expected to be shuffled here right out of datapump
// they are removed from our retry_list when an ACK is received, or when our backoff
// logic finally expires
// in support of https://tools.ietf.org/html/rfc7252#section-4.2
template <class TNetBuf, class TAddr>
class Retry
{
public:
    typedef TAddr addr_t;
    // FIX: We're gonna need a proper mapping for platform specific timing, though
    // merely tracking as milliseconds might be enough
    typedef uint32_t time_t;

    // FIX: convenience function since there isn't quite a unified cross platform time acqusition fn
    // specifically this retrieves number of milliseconds since a fixed point in time
    static time_t now()
    {
#if defined(__POSIX__) || defined(__MACH__)
        timeval curTime;
        gettimeofday(&curTime, NULL);
        int milli = curTime.tv_usec / 1000;
        return milli;
#endif
    }

    struct Metadata
    {
        // from 0-4 (COAP_MAX_RETRANSMIT)
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

    struct Metadata_Old
    {
        // number of retries attempted so far
        // "retransmission counter" as referenced by section 4.2
        uint8_t retry_count;

        // when to send it by
        time_t due;

        Metadata_Old() :
            retry_count(0) {}
    };

    // proper MID and Token are buried in netbuf, so don't need to be carried
    // seperately
    // TODO: Utilize ObservableSession as a base once we resolve netbuf-inline behaviors here
    struct Item :
#ifdef FEATURE_MCCOAP_DATAPUMP_OBSERVABLE
            IDataPumpObserver<TNetBuf, TAddr>,
#endif
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
                parent(parent)
        {
            // TODO: assign addr
        }

#ifdef FEATURE_MCCOAP_DATAPUMP_OBSERVABLE
        // IDataPumpObserver interface
    private:
        virtual void on_message_transmitting(TNetBuf* netbuf, const TAddr& addr) OVERRIDE
        {

        }


        virtual bool on_message_transmitted(TNetBuf* netbuf, const TAddr& addr) OVERRIDE
        {
            // This function perhaps only should indicate whether we should be taking ownership
            // of buffer after send (return true), and let process/rescheduler handle everything
            // else
            if(base_t::retransmission_counter < 4)
            {
                if(base_t::is_definitely_con()) return true;

                if(header().type() == Header::Confirmable)
                {
                    base_t::con_helper_flag_bit = 1;
                    return true;
                }

                // TODO: Have an indicator that it is_definitely_not_con
            }

            return false;

            // at this point we should only be looking at first-try CON message (or candidate CON)
            // and after we verify it IS CON, then queue up for initial resend.  We do not need
            // to evaluate retries > 0 here, that should be handled by the process/reschedule area


            // at this point we'll want to evaluate if:
            // CON == true and our retry count is < 4, and if so...
            // NOTE: retransmission counter eval not necessary now
            if(base_t::retransmission_counter < 4 &&
                    (base_t::is_definitely_con() || header().type() == Header::Confirmable))
            {
                // do proper timeout delta calculations to schedule a resend.  Also take
                // ownership of netbuf away from incoming datapump, we'll only give it
                // back once it's time to schedule a retry CON operation
                // NOTE: that incoming netbuf and addr SHOULD match already tracked
                time_t due = now() + base_t::delta();

                this->due = due;

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
                // we'll need to do a std::forward move operation to hold on to incoming netbuf
#else
                // we'll need to assign netbuf and give a clue as to a reference counter/delete indicator
                // for datapump netbuf cleanup code
#endif
                // true = we have taken ownership of netbuf, signal to caller not to deallocate
                // false = we are not interested, caller maintains ownership of netbuf
                return true;
            }

            return false;
        }
#endif
    };

private:
    //Using raw linked list pool for now until we get a better sorted
    // solution in place
    typedef moducom::mem::LinkedListPool<int, 10> llpool_t;
    //typedef moducom::mem::LinkedListPoolNodeTraits<int, 10> node_traits_t;

    /*
    typedef estd::forward_list<int,
            estd::inlinevalue_node_traits<estd::experimental::forward_node_base,
                    mem::LinkedListPool */

    typedef estd::layer1::vector<Item, 10> list_t;

    // TODO: this should eventually be a priority_queue or similar
    list_t retry_list;

public:
    Retry() {}

    bool enqueue(const addr_t& addr, TNetBuf& netbuf)
    {
        // TODO: ensure it's sorted by 'due'
        // for now just brute force things
        Item item(this, addr, &netbuf);

        // TODO: revamp the push_back code to return success or fail
        retry_list.push_back(item);

        return true;
    }

    // call this to get next item for transport to send, or NULLPTR if nothing
    // keep in mind Item shall have 'due' in there to indicate when item should
    // *actually* be sent, it is up to consumer to heed this
    //const Item* front() const
    Item* front()
    {
        // TODO: make this const eventually ... though maybe
        // we can't since transport ultimately will want to diddle with netbuf
        Item* best = NULLPTR;

        //for(Item* v : retry_list)
        typename list_t::iterator i = retry_list.begin();

        while(i != retry_list.end())
        {
            Item& v = *i;
            if(best == NULLPTR)
                best = &v;
            else if(v.due < best->due)
                best = &v;

            i++;
        }

        return best;
    }

    // called when ACK is received to determine if we should remove anything from the
    // retry_queue
    void ack_received(const addr_t& from_addr, uint16_t mid, const coap::layer2::Token& token)
    {

    }

    // call this after front() is called and its contained 'due' has passed
    // will:
    //   a) evaluate first (front()) item to see if current_time > due and
    //      1. if so, requeue for later retry attempt again
    //      2. if not, do nothing
    //   b)
    template <class TDataPump>
    void service(time_t current_time, TDataPump& datapump)
    {
        Item* f = front();

        if(f != NULLPTR)
        {
            if(f->due >= current_time)
            {
                // Will need to maintain some kind of signal / capability
                // to keep taking ownership of netbuf so that it doesn't
                // get deleted until we're done with our resends (got ACK
                // or ==4 retransmission)
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
                datapump.enqueue_out(std::forward(f->netbuf()), f->addr, f);
#else
                datapump.enqueue_out(f->netbuf(), f->addr, f);
#endif
            }
        }
    }
};


}}}
