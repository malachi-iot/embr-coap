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
                    // also denote as always-interested so we can do add URI to list
                    // while remembering we want to register
                    break;

                case 1: // deregister
                    // also denote as always-interested so we can remove URI from list
                    // while remembering we want to deregister
                    break;

                default:
                    // error
                    break;
            }
            break;

        case Option::UriPath:
            break;

        default:break;
    }
}

}}
