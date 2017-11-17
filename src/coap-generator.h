//
// Created by malachi on 11/13/17.
//

#ifndef MC_COAP_TEST_COAP_GENERATOR_H
#define MC_COAP_TEST_COAP_GENERATOR_H

#include "mc/pipeline.h"
#include "coap_transmission.h"

namespace moducom { namespace coap {

// Non-blocking coap message generator state machine wrapper
class CoAPGenerator
{
    //typedef pipeline::experimental::IBufferProviderPipeline pipeline_t;
    typedef pipeline::IPipeline pipeline_t;

    pipeline_t& output;

    struct payload_state_t
    {
        size_t pos;
    };

    typedef experimental::layer2::OptionGenerator::StateMachine _option_state_t;

public:
    typedef experimental::layer2::OptionBase option_t;
    typedef coap::CoAP::OptionExperimental::number_t option_number_t;

#ifdef __CPP11__
    typedef CoAP::Parser::State state_t;
    typedef CoAP::Parser::SubState substate_t;
#else
    typedef CoAP::Parser state_t;
    typedef CoAP::Parser substate_t;
#endif

private:
    // FIX: lift this out of here, because we have explicit 'begins'
    // for the different state sections, we're able to initialize which
    // substate we're in
    CoAP::Parser::State _state;

    CoAP::Parser::State state() const { return _state; }
    void state(CoAP::Parser::State state) { _state = state; }

    struct header_state_t : public payload_state_t
    {
        uint8_t bytes[4];
    };

    struct option_state_t
    {
        uint8_t fakebuffer[sizeof(_option_state_t)];
    };

    union
    {
        header_state_t header_state;
        payload_state_t payload_state;
        option_state_t option_state;
    };

    // workarounds for C++98/C++03 union limitations
    inline _option_state_t* get_option_state_ptr()
    {
        return (_option_state_t*) option_state.fakebuffer;
    }

    _option_state_t& get_option_state()
    {
        return *get_option_state_ptr();
    }

public:
    CoAP::Header* get_header() const
    {
        // NOTE: Be very careful with this cast! Make sure Header class itself
        // at least starts with those 4 bytes...
        return (CoAP::Header*) header_state.bytes;
    }

    CoAPGenerator(pipeline_t& output) :
            // FIX: temporarily marking as HeaderDone, since the header handling
            // code isn't ready yet
            _state(state_t::HeaderDone),
            output(output) {}

    // for raw access , useful for optimized zero copy scenarios
    pipeline_t& get_output() { return output; }

    // doesn't utilize value, that must happen manually.  Instead just sets up
    // number and length of option
    bool output_option_raw_iterate(const option_t& option);

    //!
    //! \param option
    //! \return
    // TODO: Make a version of this which can re-use same option over and over again
    // and figure out a new option is present by some other means
    bool output_option_iterate();
    bool output_option_next(const option_t& option);

    bool output_header_begin(const CoAP::Header& header)
    {
        header_state.pos = 0;
        memcpy(header_state.bytes, header.bytes, 4);
    }

    bool output_header_iterate();

    void output_payload_begin() {}
    bool output_payload_iterate(const pipeline::MemoryChunk& chunk);


    void output_token_begin()
    {
    }


    bool output_token_iterate(const pipeline::MemoryChunk& chunk)
    {
        // FIX: For now, this code presumes that chunk represents exactly
        // the right number of token bytes (not a segmented/batched chunk)
        return output.write(chunk);
    }


    void _output(const option_t& option)
    {
        output_option_next(option);

        while(!output_option_iterate())
        {
            // TODO: place a yield statement in here since this is a spinwait
        }
    }


    // this is for payload, we may need a flag (or a distinct call) to designate
    void _output(const pipeline::MemoryChunk& chunk)
    {
        output_payload_begin();

        while(!output_payload_iterate(chunk))
        {
            // TODO: place a yield statement in here since this is a spinwait
        }
    }


    void _output(const CoAP::Header& header)
    {
        output_header_begin(header);

        while(!output_header_iterate())
        {
            // TODO: place a yield statement in here
        }
    }
};


}}

#endif //MC_COAP_TEST_COAP_GENERATOR_H
