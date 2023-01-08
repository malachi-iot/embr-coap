#pragma once

namespace embr { namespace coap {

namespace internal {

// DEBT: Need a better name.  Root class of helpers for message-level
// Encoder/Decoder operations
class Root
{
public:
    // 30DEC22 MB
    // this boundary code was used with old pipeline/netbuf approach.  Very obsolete.  Keeping
    // commented here because RFC 7641 support might ponder this ideology if we do pbuf chaining
    // in context of breaking out options/payload from header/token
    /*
    // for IPipelineWriter, mainly
    typedef uint8_t boundary_t;

    // The end of header, token, options are marked with segment
    // NOTE: Not fully functional yet
    static CONSTEXPR boundary_t boundary_segment = 3;
    // the end of the entire message marked with this one
    static CONSTEXPR boundary_t boundary_message = 4;
    */

    enum State
    {
        Uninitialized,
        Header,
        HeaderDone,
        TokenStart, // currently unused
        Token,
        /// Emit always, even if tkl = 0
        TokenDone,
        OptionsStart,
        Options,
        OptionsDone, // all options are done being processed
        Payload,
        PayloadDone,
        // Denotes completion of entire CoAP message
        Done,
    };
};

typedef Root::State root_state_t;
typedef Root _root_state_t;

}
}}