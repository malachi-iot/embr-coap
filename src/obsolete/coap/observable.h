#pragma once

#include <coap/observable.h>

namespace embr { namespace  coap {

namespace experimental {

// specialized code to handle re-queuing outgoing messages by modifying their headers
// so that we can blast through 'j' number of notification targets but only allocating
// one or two netbufs
template<class TDataPump,
        class TIterator,
        class TNetBuf = typename TDataPump::netbuf_t,
        class TAddr = typename TDataPump::addr_t>
class ObservableMetaSession :
        public moducom::coap::IDataPumpObserver<TNetBuf, TAddr>
{
    typedef TNetBuf netbuf_t;

    TDataPump& datapump;
    TIterator it;
    TIterator it_end;       // TODO: Would really like to optimize this out

    virtual void on_message_transmitting(TNetBuf* netbuf, const TAddr& addr) OVERRIDE
    {

    }

    virtual void on_message_transmitted(TNetBuf* netbuf, const TAddr& addr) OVERRIDE
    {
        // When a observable message is fired off, we want to then:
        // 1. notice if there are any more in the observable list which want to be notified
        // 2. if so, let 'n' = new target to notify
        // 3. take this incoming netbuf and requeue it for send, modifying mid/token in coap
        //    to match 'n' original mid/token which it registered with, and of course sending
        //    to 'n' address
        // 4. eventually, we also need to re-encode Option sequence number, though I expect the
        //    importance of that is a small edge case
        if(it != it.end())
        {
            typedef typename TIterator::value_type value_type;
            value_type& v = *it;
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
            NetBufEncoder<netbuf_t> rewritten_header_and_token;
            netbuf_t& new_netbuf = rewritten_header_and_token.netbuf();
#else
            netbuf_t new_netbuf = new netbuf_t;

            NetBufEncoder<netbuf_t&> rewritten_header_and_token(*new_netbuf);
#endif
            NetBufDecoder<netbuf_t&> decoder(*netbuf);

            Header header = decoder.header();

            header.token_length(it.token.length());

            rewritten_header_and_token.header(header);
            rewritten_header_and_token.token(it.token());

#ifdef FEATURE_MCCOAP_DISCRETE_OBSERVER_SEQUENCE
#error Discrete copied observe sequence feature not yet implemented
#endif

            // copy just-sent netbuf to new newbut, skipping the header and token of
            // source (explicitly) and destingation (implicitly)
            coap_netbuf_copy(*netbuf, rewritten_header_and_token.netbuf(), 4 + header.token_length());

            rewritten_header_and_token.netbuf().complete();

#ifdef FEATURE_EMBR_DATAPUMP_INLINE
            datapump.enqueue_out(std::forward(new_netbuf), it.addr);
#else
            datapump.enqueue_out(*netbuf, it.addr);
#endif

            ++it;
        }

        // if it == end here, then nothing was enqueued, therefore this observer should
        // fall away and not be called again
    }

public:
    ObservableMetaSession(TDataPump& datapump,
                          TIterator it,
                          TIterator it_end) :
            datapump(datapump),
            it(it),
            it_end(it_end)
    {}
};

}

}}