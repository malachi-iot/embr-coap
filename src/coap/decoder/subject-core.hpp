#pragma once

#include "streambuf.h"
#include "events.h"

// DEBT: Move this and subject.hpp contents into decode-and-notify.hpp
namespace embr { namespace coap { namespace iterated {

// inspects state of incoming decoder+context, and fires off associated decoder events via
// provided subject.
// copy/paste of existing context-based decode_and_notify
// TODO: deduce actual streambuf by SFINAE looking for in_avail, etc
/// @return true when at end of context buffer, false otherwise
template <class TSubject, class TStreambuf, class TContext>
decode_result decode_and_notify(StreambufDecoder<TStreambuf>& decoder, TSubject& subject, TContext& app_context)
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
    iterated::decode_result at_end = decoder.process_iterate_streambuf();

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
            subject.notify(option_start_event(), app_context);
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
            subject.notify(option_completed_event(), app_context);
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
            at_end.done = true;
            subject.notify(completed_event(), app_context);
            break;

        default: break;
    }

    return at_end;
}

}}}
