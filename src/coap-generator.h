//
// Created by malachi on 11/13/17.
//

#ifndef MC_COAP_TEST_COAP_GENERATOR_H
#define MC_COAP_TEST_COAP_GENERATOR_H

#include "mc/pipeline.h"
#include "coap_transmission.h"
#include "coap-encoder.h"

namespace moducom { namespace coap {

namespace experimental {
class OptionEncoderHelper;
}

// Non-blocking coap message generator state machine wrapper
class CoAPGenerator
{
    // FIX: Don't like this friend here
    friend class GeneratorResponder;
    friend class experimental::OptionEncoderHelper;

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
    typedef coap::CoAP::OptionExperimentalDeprecated::number_t option_number_t;

#ifdef __CPP11__
    typedef CoAP::ParserDeprecated::State state_t;
    typedef CoAP::ParserDeprecated::SubState substate_t;
#else
    typedef CoAP::ParserDeprecated state_t;
    typedef CoAP::ParserDeprecated substate_t;
#endif

private:
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

    // need a separate next/begin because one initializes option generator state,
    // while the other advances forward while leaving current option number alnne
    bool output_option_next(const option_t& option);
    void output_option_next()
    {
        _option_state_t& option_state = get_option_state();
        option_state.next();
    }
    bool output_option_begin(const option_t& option);

    void output_header_begin(const CoAP::Header& header)
    {
        header_state.pos = 0;
        memcpy(header_state.bytes, header.bytes, 4);
    }

    void output_option()
    {
        while(!output_option_iterate())
        {
            // TODO: place a yield statement in here since this is a spinwait
        }
    }

    bool output_header_iterate();

    void output_payload_begin() { payload_state.pos = 0; }
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

namespace experimental {

// Keep in mind a developing vocabulary:
// Encoder spits out bytes
// Generator pushes out through a pipeline
class OptionEncoderHelper
{
    //typedef CoAP::OptionExperimentalDeprecated option_t;
    typedef layer2::OptionBase option_t;

    int state;
    uint16_t next_number;
    //option_t option;
    uint8_t _option[sizeof(option_t)];

    option_t& option() const { return *(option_t*) _option; }

protected:
    // sets up option instance for next operation
    // returns what next option shall be. 0 means we're done with options
    // be sure to return & process options in correct order
    virtual uint16_t option_start_callback() = 0;

#ifdef UNUSED
    // sample
    virtual uint16_t option_start_callback() OVERRIDE
    {
        switch(option.number)
        {
            case 0:
            case UriPath:
                // set up URI info
                return ContentFormat;

            case ContentFormat:
                // set up Content format for subsequent outgoing handling

            default:
                return 0;
        }
    }
#endif

public:
    void initialize(CoAPGenerator& encoder, uint16_t option_number = 0)
    {
        state = 2;
        this->next_number = option_number;
        option_t& o = option();
        new (&o) option_t(next_number);
        encoder.output_option_begin(o);
        next_number = option_start_callback();
    }

    void initialize(OptionEncoder& encoder, uint16_t option_number = 0)
    {
        this->next_number = option_number;
        option_t& o = option();
        new (&o) option_t(next_number);
        // FIX: Really should be an initialization of encoder, but will do for now
        encoder.next(o);
    }

    OptionEncoder::output_t process_iterate(OptionEncoder& encoder)
    {
        switch(state)
        {
            case 1:
            {
                option_t &o = option();
                new(&o) option_t(next_number);
                encoder.next();
                next_number = option_start_callback();
                state = 2;
                break;
            }
            case 2:
                // If option has been completely emitted
                if(encoder.generate_iterate() != OptionEncoder::signal_continue)
                    // rotate to state 1: next option header
                    state = 3;
                break;

            case 3:
                if(next_number == 0) return true;
                break;
        }

        return OptionEncoder::signal_continue;
    }

    bool process_iterate(CoAPGenerator& encoder)
    {
        switch(state)
        {
            case 1:
            {
                option_t &o = option();
                new(&o) option_t(next_number);
                encoder.output_option_next();
                next_number = option_start_callback();
                state = 2;
                break;
            }
            case 2:
                // If option has been completely emitted
                if(encoder.output_option_iterate())
                    // rotate to state 1: next option header
                    state = 3;
                break;

            case 3:
                if(next_number == 0) return true;
                break;
        }

        return false;
    }

};
}
}}

#endif //MC_COAP_TEST_COAP_GENERATOR_H
