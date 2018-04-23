#include "observable.h"


namespace moducom { namespace coap {

void ObservableOptionObserverBase::on_option(option_number_t number,
                                             const ro_chunk_t& chunk,
                                             bool last_chunk)
{
    switch(number)
    {
        case Option::Observe:
            ASSERT_ERROR(1, chunk.length(), "Chunk must be only 1 long here");
            ASSERT_ERROR(Header::Code::Get, context.header().code(), "Must be a get request");
            ASSERT_ERROR(true, last_chunk, "Should always be last chunk");

            switch(chunk[0])
            {
                case 0: // register
                    // denote as always-interested so we can do add URI to list
                    // while remembering we want to register
                    interested(Always);
                    break;

                case 1: // deregister
                    // denote as always-interested so we can remove URI from list
                    // while remembering we want to deregister
                    interested(Always);
                    break;

                default:
                    // error
                    interested(Never);
                    break;
            }
            break;

        case Option::UriPath:
            if(is_always_interested())
            {
                registrar_t::Context context(this->context, is_registering, this->context.address());

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
                interested(Never);

            break;

        default:break;
    }
}


void ObservableOptionObserverBase::on_complete()
{
    if(is_always_interested())
    {
        registrar_t::Context context(this->context, is_registering, this->context.address());

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
