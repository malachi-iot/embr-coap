#ifndef CLEANUP_COAP_CPP
class TestResponder : public CoAP::IResponderDeprecated
{
public:
    virtual void OnHeader(const CoAP::Header header);
    virtual void OnToken(const uint8_t message[], size_t length) OVERRIDE;
    virtual void OnOption(const CoAP::OptionExperimentalDeprecated& option);
    virtual void OnPayload(const uint8_t message[], size_t length);
};


void TestResponder::OnHeader(const CoAP::Header header) 
{
}


void TestResponder::OnToken(const uint8_t *message, size_t length)
{

}

void TestResponder::OnOption(const CoAP::OptionExperimentalDeprecated& option)
{
    switch (option.number)
    {
        case 270:
            REQUIRE(option.length == 1);
            break;

        case 271:
            REQUIRE(option.length == 2);
            break;

        case Option::UriPath:
            // FIX: First failure is option.length coming in
            // at 3, which suggests the TEST portion is not
            // coming through properly (have a feeling it's
            // getting eaten by our uri matcher)
            REQUIRE(option.length == 4);
            break;

        default:
            FAIL("Unexpected Option Number: " << option.number);
    }
}

void TestResponder::OnPayload(const uint8_t message[], size_t length)
{
    REQUIRE(length == 7);
    REQUIRE(memcmp(&buffer_16bit_delta[12], message, length) == 0);
}
#endif


#ifndef CLEANUP_COAP_CPP
    SECTION("Parsing to IResponderDeprecated")
    {
        typedef CoAP::ParserDeprecated parser_t;

        TestResponder responder;

        CoAP::ParseToIResponderDeprecated p(&responder);

        p.process(buffer_16bit_delta, sizeof(buffer_16bit_delta));

    }
#endif


#ifndef CLEANUP_TRANSMISSION_CPP
    SECTION("Parsing thru experimental DispatchingResponder")
    {
        embr::coap::experimental::DispatchingResponder responder;
        TestResponder user_responder;

        responder.add_handle("test", &user_responder);
        responder.add_handle("TEST/POS/", &user_responder);

        typedef CoAP::ParserDeprecated parser_t;

        CoAP::ParseToIResponderDeprecated p(&responder);

        p.process(buffer_plausible, sizeof(buffer_plausible));
    }
    SECTION("Experimental CoAP::Header::Code tests")
    {
        CoAP::Header header;

        header.raw = 0;
        header.response_code(CoAP::Header::Code::ProxyingNotSupported);

        CoAP::Header::Code& code = header.code_experimental();

        REQUIRE(code.get_class() == 5);
        REQUIRE(code.detail() == 05);
    }
#endif
