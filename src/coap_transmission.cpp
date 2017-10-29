//
// Created by malachi on 10/25/17.
//

#include "coap_transmission.h"

namespace moducom {
namespace coap {

void DummyDispatcher::receive()
{

}

namespace experimental {
namespace layer2 {

/*
void option_delta_length_helper(uint16_t value, uint8_t* output)
{
    uint8_t mini_header = 0;

    if(value < CoAP::Parser::Extended8Bit)
    {
        mini_header = value << 4;
    }

} */

// TODO: make this a state machine generator also
size_t OptionGenerator::generate(uint16_t last_number, uint8_t *output)
{
    ASSERT_ERROR(true, number > last_number, "Option number not greater than preceding number");

    uint16_t delta = number - last_number;
    return 0;
}

OptionGenerator::StateMachine::StateMachine(const OptionBase& option_base) :
    option_base(option_base)
{
    _sub_state = CoAP::Parser::OptionSize;
    current_option_number = 0;
    pos = 0;
}


uint8_t generator_helper(uint16_t value, int pos = 0)
{
    if (value < CoAP::Parser::Extended8Bit)
    {
        return value;
    }
    else if (value < COAP_EXTENDED8_BIT_MAX)
    {
        if (pos == 0) return CoAP::Parser::Extended8Bit;

        value -= 13;

        if (pos == 1) return value;
    }
    else if (value < COAP_EXTENDED16_BIT_MAX)
    {
        if (pos == 0) return CoAP::Parser::Extended16Bit;

        value -= 269;

        if (pos == 1) return (value >> 8);
        if (pos == 2) return (value & 0xFF);
    }

    ASSERT_ERROR(false, false, "Should not get here");
    return -1;
}

// TODO: make all these ins and outs a different type
// so that we can spit out -1 or similar indicating "more processing required"
// so that we don't have to pull stunts to pop around extra steps
OptionGenerator::StateMachine::output_t OptionGenerator::StateMachine::generate_iterate()
{
    uint8_t option_delta_root = generator_helper(option_base.number - current_option_number);
    uint8_t option_length_root = generator_helper(option_base.length);

    switch (sub_state())
    {
        case CoAP::Parser::OptionSize:
            pos = 0;
            sub_state(CoAP::Parser::OptionSizeDone);

            option_length_root |= option_delta_root << 4;

            return option_length_root;

        case CoAP::Parser::OptionSizeDone:
            if (option_delta_root >= CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionDelta);
            else if (option_length_root >= CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionDeltaDone);
            else
                sub_state(CoAP::Parser::OptionDeltaAndLengthDone);

            return signal_continue;

        case CoAP::Parser::OptionDelta:
        {
            uint8_t option_delta_next = generator_helper(option_base.number - current_option_number, ++pos);
            if (option_delta_root == CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionDeltaDone);
            else if (option_delta_root == CoAP::Parser::Extended16Bit && pos == 2)
                sub_state(CoAP::Parser::OptionDeltaDone);
            else if (pos > 2)
            {
                ASSERT_ERROR(false, false, "Should not be here");
            }
            return option_delta_next;
        }

        case CoAP::Parser::OptionDeltaDone:
            current_option_number = option_base.number;
            if (option_length_root >= CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionLength);
            else
                sub_state(CoAP::Parser::OptionLengthDone);

            return signal_continue;

        case CoAP::Parser::OptionLength:
        {
            uint8_t option_length_next = generator_helper(option_base.length, ++pos);
            if (option_length_next == CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionLengthDone);
            else if (option_length_next == CoAP::Parser::Extended16Bit && pos == 2)
                sub_state(CoAP::Parser::OptionLengthDone);
            else if (pos > 2)
            {
                ASSERT_ERROR(false, false, "Should not be here");
            }
            return option_length_next;
        }

        case CoAP::Parser::OptionDeltaAndLengthDone:

        case CoAP::Parser::OptionLengthDone:
            pos = 0;
            sub_state(CoAP::Parser::OptionValue);

        case CoAP::Parser::OptionValue:
            if (pos == option_base.length - 1)
                sub_state(CoAP::Parser::OptionValueDone);

            return option_base.value_opaque[pos++];

        case CoAP::Parser::OptionValueDone:
            break;
    }

    ASSERT_ERROR(false, false, "Should not get here");
    return -1;
}


}

void TestResponder::OnHeader(const coap::CoAP::Header header)
{
}

void TestResponder::OnOption(const coap::CoAP::OptionExperimental& option)
{
#ifdef __CPP11__
    typedef CoAP::Header::TypeEnum type_t;
#else
    typedef CoAP::Header type_t;
#endif

    // figure out if we're handling a request or a response
    // FIX: https://tools.ietf.org/html/rfc7252#section-2.2
    // denotes that a response can also be confirmable/noncomfirmable
    // (cases are non-piggyback response or probably also registered observer/subscriber responses)
    // so we need to instead pay closer attention to the token to figure out whether
    // we're looking at a response or not
    switch(header.type())
    {
        // request
        case type_t::Confirmable:
        case type_t::NonConfirmable:
            if(context->is_server)
                OnOptionRequest(option);
            else
                OnOptionResponse(option);

            break;

            // response
        case type_t::Acknowledgement:
            OnOptionResponse(option);
            break;
    }
}

void TestResponder::OnOptionRequest(const coap::CoAP::OptionExperimental& option)
{
#ifdef __CPP11__
    typedef coap::CoAP::OptionExperimental::Numbers enum_t;
#else
    typedef coap::CoAP::OptionExperimental enum_t;
#endif    

    switch (option.get_number())
    {
        case enum_t::UriPath:
        {
            // TODO: make it switchable whether we allocate more memory or keep pointing
            // to the value buffer given to us
            std::string uri((const char*)option.value, option.length);

            // RFC7252 Section 6.5 #6
            this->uri += uri + "/";

            // TODO: case sensitivity flag

#ifdef DEBUG
            char temp[128];
            this->uri.populate(temp);
#endif
            break;
        }

#ifdef COAP_FEATURE_SUBSCRIPTIONS
        case enum_t::Observe:
            switch(option.get_uint8_t())
            {
                // register/subscribe
                case 0:
                    register_subscriber();
                    break;

                // deregister/ubsubscribe
                case 1:
                    deregister_subscriber();
                    break;
            }
            break;
#endif

        case enum_t::ContentFormat:
            break;

        case enum_t::UriPort:
        {
            port = option.value_uint;
            break;
        }
    }

    // we have enough to assign user_responder, so do so
    // FIX: be sure to pay attention to port as well
    if (user_responder == NULLPTR && option.number >= enum_t::UriPath)
    {
        user_responder = uri_list[this->uri];
    }

    if (user_responder)
    {
        user_responder->OnOption(option);
    }
}


void TestResponder::OnToken(const uint8_t *token, size_t length)
{
#ifdef __CPP11__
    typedef CoAP::Header::TypeEnum type_t;
#else
    typedef CoAP::Header type_t;
#endif

    switch(header.type())
    {
        case type_t::Confirmable:
        case type_t::NonConfirmable:
        {
            // add a token to our known list on an incoming request to use later for
            // the outgoing response
            // NOTE: in a non-piggybacked server outgoing response, I *THINK* we need to
            // get an existing token since the tokens are *always* client-generated according
            // to spec.  This means also that the ACK in that context is the client ACK'ing
            // this server request, in which we may be able to leave the Acknowledgement switch
            // alone
            const Token *t = token_manager.get(token, length);
            bool is_new_token = t == NULLPTR;

            if(is_new_token)
            {
                // go here when incoming CoAP is not matched to a token,
                // indicating a new token+request is coming in (signals that
                // we are the server)
                t = token_manager.add(token, length, NULLPTR);
            }
            else
            {
                // go here when incoming CoAP IS matched to a token
                // indicating a response (non-piggybacked/separate server response)
                // FIX: Shouldn't this actually be an OnToken call?
                t->context.responder->OnHeader(header);
            }

            this->token = t;

            // FIX: kludgy brute force here, clean this up when code settles down
            context = (RequestContext*) &t->context;

            // incoming fresh new token means we are receiving a client request,
            // making us the server
            context->is_server = is_new_token;

            break;
        }

        case type_t::Acknowledgement:
        {
            // look up a token on an incoming response to match against a previously outgoing request
            const Token *t = token_manager.get(token, length);

            this->token = t;

            // if receiving a piggybacked message, expect a payload with this ACK
            break;
        }
    }

    // pass this on to the registered responder, who will probably just ignore it
    if(context->responder)
        context->responder->OnToken(token, length);
}


void TestResponder::OnOptionResponse(const CoAP::OptionExperimental &option)
{
#ifdef __CPP11__
    typedef coap::CoAP::OptionExperimental::Numbers enum_t;
#else
    typedef coap::CoAP::OptionExperimental enum_t;
#endif

#ifdef COAP_FEATURE_SUBSCRIPTIONS
    switch(option.get_number())
    {
        case enum_t::Observe:
            // this is a rising sequence to ensure out-of-order updates are
            // discarded.  Check incoming option value against our tracked one
            uint32_t incoming = option.get_uint();
            bool is_fresh_observed = incoming > token->context.observer_option_value;
            experimental::Token* t = ((experimental::Token*)token);

            if(is_fresh_observed)
                t->context.observer_option_value = incoming;

            // FIX: need a better indicator, because this is going to stay "true"
            // when all kinds of non-observe situations may show up which should
            // make it false
            t->context.is_fresh_observed = is_fresh_observed;

            break;
    }
#endif

    if(context->responder) context->responder->OnOption(option);

    header.token_length();
}


void TestResponder::OnPayload(const uint8_t message[], size_t length)
{
    // FIX: consolidate/clean this code
    if (user_responder == NULLPTR)
    {
        user_responder = uri_list[uri];

        if(user_responder == NULLPTR) return;
    }

    user_responder->OnPayload(message, length);
}


void TestResponder::OnCompleted()
{
    if(user_responder) user_responder->OnCompleted();
}

void TestOutgoingMessageHandler::write_request_header(CoAP::Header::TypeEnum type,
                                                      CoAP::Header::RequestMethodEnum method)
{

}

void TestOutgoingMessageHandler::write_response_header(CoAP::Header::TypeEnum type,
                                                       CoAP::Header::ResponseCode::Codes response_code)
{
    CoAP::Header header(type);

    header.response_code(response_code);

    if(token != NULLPTR)
    {
        header.token_length(token->length);

        // FIX: slightly clumsy, in some senses context should contain token not the other way around
        header.message_id(token->context.message_id);
    }

    write_header(header);
}

void TestOutgoingMessageHandler::write_token()
{
    if(token != NULLPTR)
    {
        write_bytes(token->data, token->length);
    }
}

void TestOutgoingMessageHandler::write_options()
{
    //option_generator.
}

void TestOutgoingMessageHandler::write_payload(const uint8_t *message, size_t length)
{
    write_byte(0xFF); // payload marker
    write_bytes(message, length);
}

void TestOutgoingMessageHandler::send_ack()
{
#ifdef __CPP11__
    typedef CoAP::Header::TypeEnum type_t;
    typedef CoAP::Header::ResponseCode::Codes codes_t;
#else
    typedef CoAP::Header type_t;
    typedef CoAP::Header::ResponseCode codes_t;
#endif

    write_start();
    write_response_header(type_t::Acknowledgement, codes_t::Empty);
    write_token();
    write_end();
}

void OutgoingRequestHandler::send(CoAP::Header::RequestMethodEnum request_method,
                                              const uint8_t *payload,
                                              size_t length)
{
#ifdef __CPP11__
    typedef CoAP::Header::TypeEnum type_t;
    typedef CoAP::Header::ResponseCode::Codes codes_t;
#else
    typedef CoAP::Header type_t;
    typedef CoAP::Header::ResponseCode codes_t;
#endif

    write_start();
    write_request_header(type_t::NonConfirmable, request_method);
    write_token();
    write_options();
    write_payload(payload, length);
    write_end();
}

// piggybacking is done specifically
// to tend to "long running" acqusition of response data, and therefore
// an all-in-one sequential send_response is not suitable to send
// two messages at once
void OutgoingResponseHandler::send(CoAP::Header::ResponseCode::Codes response_code,
                                               const uint8_t *payload, size_t length, bool piggyback)
{
#ifdef __CPP11__
    typedef CoAP::Header::TypeEnum type_t;
    typedef CoAP::Header::ResponseCode::Codes codes_t;
#else
    typedef CoAP::Header type_t;
    typedef CoAP::Header::ResponseCode codes_t;
#endif

    write_start();

    // We have to decide whether we are going to do ACK for a piggybacked response,
    // or Confirmable/NonConfirmable for a separate response
    // We may also have to decide here whether we are going to do the additional
    // separate ACK for the separate response
    if(piggyback)
        write_response_header(type_t::Acknowledgement, response_code);
    else
        // non-piggybacked means we have to send two distinct CoAP messages
        // so be sure to call send_ack BEFORE calling send_response
        write_response_header(type_t::NonConfirmable, response_code);

    write_token();
    write_options();

    if(payload != NULLPTR && length != 0)
        write_payload(payload, length);

    write_end();
}

}
}
}
