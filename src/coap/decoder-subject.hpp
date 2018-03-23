#pragma once

#include "decoder-subject.h"

namespace moducom { namespace coap {


// TODO: Eventually clean up dispatch_option and then
// just run process_iterate always at the bottom
template <class TMessageObserver>
bool DecoderSubjectBase<TMessageObserver>::dispatch_iterate(Decoder::Context& context)
{
    const pipeline::MemoryChunk::readonly_t& chunk = context.chunk;
    size_t& pos = context.pos; // how far into chunk our locus of processing should be

    switch(decoder.state())
    {
        case Decoder::HeaderDone:
            dispatch_header();
            decoder.process_iterate(context);
            break;

        case Decoder::TokenDone:
            dispatch_token();
            decoder.process_iterate(context);
            break;

        case Decoder::Options:
        {
            OptionDecoder& option_decoder = decoder.option_decoder();

            // Repeating ourselves (and not calling) from decoder because of the special way in which
            // dispatcher reads out options
#ifdef DEBUG2
            std::clog << __func__ << ": 1 optionDecoder state = " << (optionDecoder.state()) << std::endl;
#endif

            // Explicitly check for payload marker here as it's permissible to receive a message
            // with no options but a payload present
            pipeline::MemoryChunk::readonly_t remainder = chunk.remainder(pos);
            // FIX: this code is broken until decoder itself properly processes no-option
            // but-payload-marker-found event
            if(remainder[0] == 0xFF)
            {
                // OptionsDone will check 0xFF again
                // FIX: Seems a bit inefficient so revisit
                // decoder.state(Decoder::OptionsDone);
                break;
            }
            pos += dispatch_option(remainder);

#ifdef DEBUG2
            std::clog << __func__ << ": 2 optionDecoder state = " << (optionDecoder.state()) << std::endl;
#endif

            // handle option a.1), a.2) or b.1) described below
            if ((pos == chunk.length() && context.last_chunk) || chunk[pos] == 0xFF)
            {
                ASSERT_ERROR(true,
                             (option_decoder.state() == OptionDecoder::OptionValueDone) ||
                             (option_decoder.state() == OptionDecoder::OptionDeltaAndLengthDone),
                             "Must be either optionValueDone or optionDeltaAndlengthDone.  Got: " << option_decoder.state());
                // will check again for 0xFF
                // FIX: Very naughty, repair this
                decoder.state_for_decoder_subject(Decoder::OptionsDone);
            }
            else
            {
                // UNSUPPORTED
                // b.2 means we are not sure if another option or an incoming chunk[0] == 0xFF is coming
                // MAY need an OptionsBoundary state, but hopefully not
                // better would be to beef up OptionDecoder awareness of Payload marker
            }

            break;
        }

        case Decoder::Payload:
        {
            bool last_payload_chunk = context.last_chunk;

            if(pos == 0)
                // arrives here in the unlikely case payload starts on new chunk
                // boundary
                dispatch_payload(chunk, last_payload_chunk);
            else
                // typically represents an a.1 scenario, but doesn't have to
                dispatch_payload(chunk.remainder(pos), last_payload_chunk);

            decoder.process_iterate(context);
            break;
        }

        default:
            decoder.process_iterate(context);
            break;
    }

    // TODO: Do an assert to make sure pos never is >
    ASSERT_ERROR(true, pos <= chunk.length(), "pos should never exceed chunk length");

    return pos == chunk.length();
}

template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::dispatch_header()
{
    observer_on_header(decoder.header_decoder());
}

// also handles pre-dispatch processing
template <class TMessageObserver>
size_t DecoderSubjectBase<TMessageObserver>::dispatch_option(const pipeline::MemoryChunk::readonly_t& optionChunk)
{
    OptionDecoder& option_decoder = decoder.option_decoder();
    // FIX: Accessing optionHolder directly here is a nono
    OptionDecoder::OptionExperimental& optionHolder = decoder.optionHolder;

    // FIX: we generally expect to be at FirstByte state here, however in the future this
    // requirement should go away (once we clean up needs_value_processed behavior)
    size_t processed_length = option_decoder.process_iterate(optionChunk, &optionHolder);
    size_t value_pos = processed_length;

    // FIX: Kludgey, can really be cleaned up and optimized
    // ensures that our linked list doesn't execute process_iterate multiple times to acquire
    // option-value.  Instead, our linked list looping might need to happen per state() switched on
    // Will also cause problems if there are no registered handlers, since count will be off as
    // value is never processed
    bool needs_value_processed = true;
    size_t total_length = processed_length;
#ifdef FEATURE_DISCRETE_OBSERVERS
    typedef IOptionObserver::number_t option_number_t;
#else
    typedef IMessageObserver::number_t option_number_t;
#endif

    switch (option_decoder.state())
    {
        //case OptionDecoder::OptionLengthDone:
        //case OptionDecoder::OptionDeltaAndLengthDone:
        case OptionDecoder::ValueStart:
        {
            option_number_t option_number = (option_number_t) optionHolder.number_delta;
            uint16_t option_length = optionHolder.length;
#ifdef DEBUG2
            std::clog << "Dispatching option: " << option_number << " with length " << optionHolder.length;
            std::clog << std::endl;
#endif
            observer_on_option(option_number, option_length);

            if(needs_value_processed)
            {
                // Getting here
                processed_length = option_decoder.process_iterate(optionChunk.remainder(processed_length), &optionHolder);
                total_length += processed_length;
                needs_value_processed = false;
            }

            // this will happen normally during multi-chunk operations
            // we shouldn't be seeing that for a while, however
            ASSERT_WARN(option_length, processed_length, "option length mismatch with processed bytes");

            if(option_length > 0)
            {
                // TODO: Need to demarkate boundary here too so that on_option knows where boundary starts
                pipeline::MemoryChunk partialChunk((uint8_t*)(optionChunk.data()) + value_pos, processed_length);

#ifdef DEBUG2
                std::clog << "Dispatching option: data = ";

                // once we resolve the two different OptionExperimentals, use this
                //CoAP::OptionExperimentalDeprecated::get_format()
                //optionHolder.get_format();
                // for now, just assume everything is a string
                for(int i = 0; i < option_length; i++)
                {
                    std::clog << (char)partialChunk[i];
                }

                std::clog << std::endl;
#endif
                // FIX: We are arriving here with values of FirstByte(Done?) and OptionValue
                // suggesting strongly we aren't iterating completely or fast-forwarding past
                // value when we need to
                bool full_option_value = option_decoder.state() == OptionDecoder::OptionValueDone;
                observer_on_option(option_number, partialChunk, full_option_value);
            }
            break;
        }

        case OptionDecoder::FirstByteDone:

            break;

        case OptionDecoder::OptionValue:
        {
            // FIX: Not yet tested
            //processed_length = optionDecoder.process_iterate(optionChunk, NULLPTR);
            //pipeline::MemoryChunk partialChunk(optionChunk.data, processed_length);
            //bool full_option_value = optionDecoder.state() == OptionDecoder::OptionValueDone;
            //handler->on_option(partialChunk, full_option_value);
            option_number_t option_number = (option_number_t) optionHolder.number_delta;
            observer_on_option(option_number, optionChunk, false);
            break;
        }

        case OptionDecoder::OptionValueDone:
        {
            // FIX: Not yet supported
            // FIX: Not quite correct code.  We need to suss out how large
            // the chunk we're passing through really is and of course
            // at what boundary it starts
            //processed_length = optionDecoder.process_iterate(optionChunk, NULLPTR);
            option_number_t option_number = (option_number_t) optionHolder.number_delta;
            observer_on_option(option_number, optionChunk, true);
            break;
        }

        case OptionDecoder::Payload:
            // NOTE: Not yet used, we don't iterate enough to get here
            break;

        default:break;
    }

    return total_length;
}


template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::dispatch_payload(const pipeline::MemoryChunk::readonly_t& payloadChunk, bool last_chunk)
{
    observer_on_payload(payloadChunk, last_chunk);
}


template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::dispatch_token()
{
    // TODO: fix-can't do const due to data() call
    TokenDecoder& token_decoder = decoder.token_decoder();
    // NOTE: 90% sure it's safe to intermingle header decoder + token decoder (pretty
    // sure we retain header all the way thru) but doublecheck and document
    pipeline::MemoryChunk chunk(token_decoder.data(),
                                            decoder.header_decoder().token_length());

    // FIX: TODO: Chunking out token eventually will be necessary
    observer_on_token(chunk, true);
}

}}
