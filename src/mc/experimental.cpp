//
// Created by Malachi Burke on 1/6/18.
//

#include "experimental.h"

namespace moducom { namespace coap { namespace experimental {


// in regular conditions:
// * CON request always has some response
// * NON request may or may not have a response (only if server wants to communicate something back)
// * ACK request should never appear
// * RESET request
void process_header_request(Header input, Header* output, bool piggyback)
{
    if(input.type() == Header::Confirmable)
    {
        if(piggyback)
        {
            output->type(Header::Acknowledgement);
        } else
        {
            output->type(Header::Confirmable);
        }
    }
    else if(input.type() == Header::NonConfirmable)
    {
        output->type(Header::NonConfirmable);
    }
    else
    {
        // ACK and RESET should not appear for a request
        ASSERT_ERROR(true, false, "Unexpected header type " << input.type());
    }
}


bool process_header_response(Header input, Header* output)
{
    switch(input.type())
    {
        case Header::Confirmable:
            // response required
            output->type(Header::Acknowledgement);
            return true;

        case Header::Acknowledgement:
            // No output needed when processing an Ack response
            return false;

        case Header::Reset:
            // A retry with different mid/token may be necessary here
            // won't categorize as output because it's more of a reinitiation of request
            return false;

        case Header::NonConfirmable:
            // Expected when we issued a NON request, but MAY appear from a CON
            // request too.  In either case, no response required
            return false;
    }

}


}}}