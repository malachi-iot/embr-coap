#pragma once

#include "observable.h"


namespace moducom { namespace coap {

template <class TCollection,
          class TIncomingContext,
          class TRequestContextTraits>
template <class TDataPump>
void ObservableRegistrar<TCollection, TIncomingContext, TRequestContextTraits>::
    for_each(
        TDataPump& datapump,
        void (*emit_observe_fn)(
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
            moducom::coap::NetBufEncoder<typename TDataPump::netbuf_t>& encoder,
#else
            moducom::coap::NetBufEncoder<typename TDataPump::netbuf_t&>& encoder,
#endif
            const observable_session_t& sess),
        bool autosend_observe_option)
{
    typedef typename TDataPump::netbuf_t netbuf_t;
    typedef typename TDataPump::addr_t addr_t;
    //typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    iterator it = begin();

    while(it != end())
    {
        //std::clog << "Sending to " << addr << std::endl;
#ifdef FEATURE_MCCOAP_IOSTREAM_NATIVE
        std::clog << "Event fired" << std::endl;
#endif

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        NetBufEncoder<netbuf_t> encoder;
#else
        NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

        static int mid = 0;

        // we want a NON or CON since observer messages are not a normal response
        // but instead classified as 'notifications'
        Header header(Header::NonConfirmable, Header::Code::Content);

        addr_t addr = (*it).addr;
        const layer2::Token& token = (*it).token;
        // NOTE: it appears coap-cli doesn't like a sequence # of 0
        int sequence = ++(*it).sequence;

        header.message_id(mid++);
        header.token_length(token.length());

        encoder.header(header);
        encoder.token(token);

        encoder.option(Option::Observe, sequence); // using mid also for observe counter since we aren't doing CON it won't matter

        emit_observe_fn(encoder, *it);

        ++it;

        encoder.complete();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        datapump.enqueue_out(std::forward<netbuf_t>(encoder.netbuf()), addr);
#else
        datapump.enqueue_out(encoder.netbuf(), addr);
#endif
    }}


template <class TRequestContext>
void ObservableOptionObserverBase<TRequestContext>::on_option(option_number_t number,
                                             const ro_chunk_t& chunk,
                                             bool last_chunk)
{
    switch(number)
    {
        case Option::Observe:
            ASSERT_ERROR(1, chunk.length(), "Chunk must be only 1 long here");
            ASSERT_ERROR(Header::Code::Get, base_t::context().header().code(), "Must be a get request");
            ASSERT_ERROR(true, last_chunk, "Should always be last chunk");

            switch(chunk[0])
            {
                case 0: // register
                    // denote as always-interested so we can do add URI to list
                    // while remembering we want to register
                    interested(base_t::Always);
                    break;

                case 1: // deregister
                    // denote as always-interested so we can remove URI from list
                    // while remembering we want to deregister
                    interested(base_t::Always);
                    break;

                default:
                    // error
                    interested(base_t::Never);
                    break;
            }
            break;

        case Option::UriPath:
            if(base_t::is_always_interested())
            {
                typename registrar_t::Context context(this->context(), is_registering);

                // pass on uri path for registrar to evaluate.  It may or may not end
                // up being a valid registrable path
                registrar.on_uri_path(context, chunk, last_chunk);

                // if always interested, then we will have permanantly allocated some space so we know if
                // we're registering or not
                if(is_registering)
                {
                    // build registration.  Remember UriPath shows up in pieces
                }
                else
                {
                    // build deregistration.  Remember UriPath shows up in pieces
                }
            }
            else
                // if we haven't determined we are always interested by now, we are never interested
                interested(base_t::Never);

            break;

        default:break;
    }
}


template <class TRequestContext>
void ObservableOptionObserverBase<TRequestContext>::on_complete()
{
    if(base_t::is_always_interested())
    {
        typename registrar_t::Context context(this->context(), is_registering);

        // uri path gathering is complete.  Now registrar will
        // either act on the valid uri path or ignore the invalid uri path
        registrar.on_complete(context);

        if(is_registering)
        {
            // issue registration
        }
        else
        {
            // issue deregistration
        }
    }
}

}}
