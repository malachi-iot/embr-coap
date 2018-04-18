#include "encoder.h"

namespace moducom { namespace coap {

template <class TNetBuf>
bool NetBufEncoder<TNetBuf>::option(option_number_t number, const pipeline::MemoryChunk& option_value)
{
    typedef OptionEncoder oe_t;
    typedef experimental::layer2::OptionBase option_base_t;

    option_base_t ob(number);

    oe_t& oe = option_encoder;
    size_t length = option_value.length();

    ob.value_opaque = option_value.data();
    ob.length = length;

    // FIX: This is not resilient to partial netbuf writes, option encoder itself might need
    // some adjustment to be part-way initialized after a former write - mainly because
    // OptionBase itself might vary due to:
    // 1) OptionBase being stack reallocated
    // 2) option_value potentially being drastically different
    // this might be partially addressed by making OptionBase itself an instance of OptionEncoder,
    // rather than a pointer.  Not much seems to be gained by making it a pointer, although the
    // question remains what to do when option_value is split up.
    // FIX: the output_data code below DOES NOT handle option_value
    // output, so I anticipated that scenario somewhat
    new (&oe) oe_t(ob);

    // OptionValueDone is for non-value version benefit (always reached, whether value is present or not)
    // OptionValue is for value version benefit, as we manually handle value output
    Option::State isDone = length > 0 ? Option::OptionValue : Option::OptionValueDone;

    uint8_t* output_data = data();
    int pos = 0;

    // while option encoder is not done, keep spitting out to netbuf
    while (oe.state() != isDone)
    {
        oe_t::output_t output = oe.generate_iterate();

        // FIX: Ensure pos doesn't exceed netbuf current size
        if (output != oe_t::signal_continue)
            output_data[pos++] = output;
    }

    advance(pos);

    return true;
}

}}
