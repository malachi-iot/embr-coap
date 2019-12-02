// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

using namespace moducom::coap;
using namespace moducom::coap::experimental;

#include "context.h"
#include "observer-util.h"



struct Observer
{
    static constexpr const char* TAG = "Observer";

    void on_notify(completed_event, AppContext& context)
    {
        ESP_LOGI(TAG, "on_notify completed");

        AppContext::encoder_type encoder = context.make_encoder();

        Header header = context.header();
        moducom::coap::layer3::Token token = context.token();

        header.type(Header::TypeEnum::Acknowledgement);
        header.code(Header::Code::Valid);

        encoder.header(header);
        encoder.token(token);
        encoder.finalize();

        context.reply(encoder);
    }
};


embr::layer0::subject<
    HeaderContextObserver,
    TokenContextObserver> app_subject;


void do_notify(AppContext& context, struct pbuf* p)
{
    const char* TAG = "::do_notify";

    ESP_LOGD(TAG, "p->len=%d", p->len);

    auto subject = embr::layer1::make_subject(
        embr::layer1::make_observer_proxy(app_subject),
        Observer()
        );

    context.do_notify(p, subject);
}