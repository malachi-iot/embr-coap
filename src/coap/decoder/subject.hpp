/**
 * @file
 * @author  Malachi Burke
 */

#pragma once

#include "subject.h"

namespace moducom { namespace coap {

// inspects state of incoming decoder+context, and fires off associated decoder events via
// provided subject.
// shall bear very strong resemblace to predecessor's DecoderSubjectBase dispatch_iterate
// TODO: Phase this out in favor of streambuf flavor, since most any C++ spec which handles TSubject
// is going to handle streambufs
/// @return true when at end of context buffer, false otherwise
template <class TSubject, class TContext>
bool decode_and_notify(Decoder& decoder, TSubject& subject, Decoder::Context& context, TContext& app_context)
{
    typedef event::event_base::buffer_t buffer_t;

    // NOTE: We deviate from norm and do state machine processing before then evaluating
    // state.  This means we'll miss out on responding to 'Uninitialized' state (oh no)
    // and importantly, means that we can consistently respond to 'done' state
    bool at_end = decoder.process_iterate(context);

    typedef event::header header_event;
    typedef event::token token_event;
    typedef event::option option_event;
    typedef event::payload payload_event;
    typedef event::option_completed option_completed_event;
    typedef event::option_start option_start_event;
    typedef event::completed completed_event;

    switch(decoder.state())
    {
        case Decoder::HeaderDone:
            subject.notify(header_event(decoder.header_decoder()), app_context);
            break;

        case Decoder::TokenDone:
        {
            // FIX: Not 100% sure token_length is available at this point
            buffer_t chunk(decoder.token_decoder().data(),
                             decoder.header_decoder().token_length());

            // TODO: Do chunking
            subject.notify(token_event(chunk, true), app_context);
            break;
        }

        case Decoder::OptionsStart:
            subject.notify(option_start_event{}, app_context);
            break;

        case Decoder::Options:
            switch(decoder.option_state())
            {
                // a lot of option states pass by, but we always latch on to the ValueStart state
                // remember, this state is seen even with "0-length" values
                case OptionDecoder::ValueStart:
                {
                    uint16_t option_number = decoder.option_number();

                    if (decoder.option_length() > 0)
                    {
                        bool completed;

                        // will take more work than commented code, and not as fast,
                        // but this way it's code reuse & dogfooding
                        buffer_t b = decoder.option(context, &completed);
                        subject.notify(option_event(option_number, b, completed),
                                       app_context);
                    }
                    else
                        subject.notify(option_event(option_number), app_context);
                }

                default: break;
            }
            break;

        case Decoder::OptionsDone:
            subject.notify(option_completed_event{}, app_context);
            break;

        case Decoder::Payload:
            subject.notify(payload_event(context.remainder(), context.last_chunk),
                           app_context);
            break;

        case Decoder::Done:
            subject.notify(completed_event(), app_context);
            break;

        default: break;
    }

    return at_end;
}


// inspects state of incoming decoder+context, and fires off associated decoder events via
// provided subject.
// copy/paste of existing context-based decode_and_notify
// TODO: deduce actual streambuf by SFINAE looking for in_avail, etc
/// @return true when at end of context buffer, false otherwise
template <class TSubject, class TStreambuf, class TContext>
bool decode_and_notify(StreambufDecoder<TStreambuf>& decoder, TSubject& subject, TContext& app_context)
{
    typedef event::event_base::buffer_t buffer_t;
    typedef StreambufDecoder<TStreambuf> decoder_type;
    typedef typename decoder_type::span_type span_type;

    typedef event::header header_event;
    typedef event::token token_event;
    typedef event::option option_event;
    typedef event::option_completed option_completed_event;
    typedef event::option_start option_start_event;
    typedef event::completed completed_event;

    // NOTE: We deviate from norm and do state machine processing before then evaluating
    // state.  This means we'll miss out on responding to 'Uninitialized' state (oh no)
    // and importantly, means that we can consistently respond to 'done' state
    bool at_end = decoder.process_iterate_streambuf();

    switch(decoder.state())
    {
        case Decoder::HeaderDone:
            subject.notify(header_event(decoder.header_decoder()), app_context);
            break;

        case Decoder::TokenDone:
        {
            // FIX: Not 100% sure token_length is available at this point
            buffer_t chunk(decoder.token_decoder().data(),
                           decoder.header_decoder().token_length());

            // TODO: Do chunking
            subject.notify(token_event(chunk, true), app_context);
            break;
        }

        case Decoder::OptionsStart:
            subject.notify(option_start_event{}, app_context);
            break;

        case Decoder::Options:
            switch(decoder.option_state())
            {
                // a lot of option states pass by, but we always latch on to the ValueStart state
                // remember, this state is seen even with "0-length" values
                case OptionDecoder::ValueStart:
                {
                    uint16_t option_number = decoder.option_number();

                    // TODO: This is the legacy context way, but the streambuf way we'd prefer to
                    // send out a 'streambuf_option_event' similar to the payload_option_event and
                    // let the consumer fiddle with yanking out of the stream the way they see fit.
                    // This will be harder than the payload style though, because the payload very
                    // easily can tell that the payload was read or not since EOF is easy to detect,
                    // but knowing if we properly skipped the option value is not that trivial.
                    // Some options are:
                    // 1.  Never expect consumer to forward through the stream (only use gptr, etc)
                    //     But this would hamstring consumer's ability to chunk data, making the streambuf
                    //     not so helpful
                    // 2.  Always demand consumer forward through the stream, which is reasonable except
                    //     the penalty for failure is a screwed up state machine as it moves on to the 'next'
                    //     option while the data sits on the unconsumed option value
                    // 3.  Magically know how much data the consumer read, so we can keep the state machine
                    //     sync'd always.  Since chunking (moving between bufs) is a reality, we can't just
                    //     do a pbuf/in_avail on things.
                    //     a) Perhaps an event callback somehow?  That would seal
                    //     streambuf's fate as a C++11 + creature due to the variadic nature of our subject/notify
                    //     mechanism...... though, to be fair, this decode-notify itself is exactly that, so
                    //     perhaps make the notification compile-time optional.  For that to work, it might require
                    //     observer attach at runtime though, which currently is not really supported.
                    // 11/14/2019 UPDATE
                    // After pondering above, I realize that for options, re-evaluation of an option is a capability
                    // we may want.  i.e. two different observers look at the same option value.  A stream approach is
                    // not well suited to that, as we'd have to be able to rewind streams that are chunked.  So instead,
                    // we'll continue as we've been doing (implicitly a 1) but plan to add in a chunk-helper buffer
                    // to pull in one option which may span many chunks.  We'll make this externally provided so that
                    // it can survive non-blocking underflow scenarios also.
                    if (decoder.option_length() > 0)
                    {
                        bool completed;

                        // will take more work than commented code, and not as fast,
                        // but this way it's code reuse & dogfooding
                        span_type _b = decoder.option(&completed);
                        buffer_t b((uint8_t*)_b.data(), _b.size());
                        subject.notify(option_event(option_number, b, completed),
                                       app_context);
                    }
                    else
                        subject.notify(option_event(option_number), app_context);
                }

                default: break;
            }
            break;

        case Decoder::OptionsDone:
            subject.notify(option_completed_event{}, app_context);
            break;

        case Decoder::Payload:
        {
            typename decoder_type::streambuf_type& rdbuf = *decoder.rdbuf();
            // TODO: Going to do payload differently, since reading payload out of a
            // stream is more sensible than trying to send a big buffer (what if it's
            // chunked?)
            subject.notify(event::streambuf_payload<typename decoder_type::streambuf_type>(rdbuf),
                           app_context);

            // NOTE: fast forwarding *if necessary* after payload notification so that
            // if event handler ignored payload, external loops don't infinitely wait for
            // payload to be consumed
            int in_avail = rdbuf.in_avail();
            if (in_avail > 0)
                rdbuf.pubseekoff(in_avail, estd::ios_base::cur);
            break;
        }

        case Decoder::Done:
            subject.notify(completed_event(), app_context);
            break;

        default: break;
    }

    return at_end;
}

// TODO: Eventually clean up dispatch_option and then
// just run process_iterate always at the bottom
template <class TMessageObserver>
bool DecoderSubjectBase<TMessageObserver>::dispatch_iterate(Decoder::Context& context)
{
    switch(decoder.state())
    {
        case Decoder::HeaderDone:
            observer_on_header(decoder.header_decoder());
            break;

        case Decoder::TokenDone:
            dispatch_token();
            break;

        case Decoder::Options:
            dispatch_option(context);
            break;

        case Decoder::Payload:
        {
            observer_on_payload(context.remainder_legacy(), context.last_chunk);

            // FIX: A little hacky, falls through to brute force on_complete
            // we *might* run into double-calls of Decoder::Done in this case
            if(!context.last_chunk) break;
        }

        case Decoder::Done:
            // FIX: Frequently not called, needs work -
            // DecoderSubjectBase needs to know
            // incoming message length (is exhausted) to really ascertain this
            // That said, incoming message chunks denote 'last chunk'
            // indicating whether to expect more data, that should
            // be enough to deduce a done state
            observer_on_complete();
            break;

        default: break;
    }

    return decoder.process_iterate(context);
}


template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::observer_on_option(const ro_chunk_t& chunk)
{
    option_number_t option_number = (option_number_t) decoder.option_number();
    uint16_t option_length = decoder.option_length();

    if(chunk.length() < option_length)
    {
        observer_on_option(option_number, chunk, false);
        // We expect at least 1 more call to observer_on_option,
        // so decrement length accordingly
        decoder.optionHolder.length -= chunk.length();
    }
    else
    {
        observer_on_option(option_number, chunk.subset(option_length), true);
    }
}


// during times where we are processing options at the Decoder level, handle
// incoming "option header" and option-value data
template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::dispatch_option(Decoder::Context& context)
{
    switch (decoder.option_state())
    {
        case OptionDecoder::ValueStart:
        {
            option_number_t option_number = (option_number_t) decoder.option_number();
            uint16_t option_length = decoder.option_length();

            observer_on_option(option_number, option_length);

            if(option_length == 0) break;
        }

        case OptionDecoder::OptionValue:
            observer_on_option(context.remainder_legacy());
            break;

        default:
            break;
    }
}



template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::dispatch_token()
{
    // Though we like to observe TokenDone messages, we don't pass that on
    // as an empty token observation.  This is already indicated during header
    // of tkl==0
    if(decoder.header_decoder().token_length() == 0) return;

    const Decoder::token_decoder_t& token_decoder = decoder.token_decoder();
    // NOTE: 90% sure it's safe to intermingle header decoder + token decoder (pretty
    // sure we retain header all the way thru) but doublecheck and document
    ro_chunk_t chunk(token_decoder.data(),
                        decoder.header_decoder().token_length());

    // FIX: TODO: Chunking out token eventually will be necessary
    observer_on_token(chunk, true);
}

}}
