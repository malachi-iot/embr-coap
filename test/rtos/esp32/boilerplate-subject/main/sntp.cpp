#include "context.h"

#include "esp_sntp.h"

#include <coap/header.h>
#include <coap/option.h>
// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here, and even
// if we were this is confusing and hard to remember to include
#include <coap/platform/lwip/encoder.h>

using namespace embr::coap;


void initialize_sntp()
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}


// DEBT: Do ISO 8601
// DEBT: use our own ostream formatter to avoid buf[64]
void send_time_response(AppContext& context, AppContext::encoder_type& encoder)
{
    static const char* TAG = "send_time_response";

    build_reply(context, encoder, Header::Code::Content);

    encoder.option(
        Option::ContentFormat,
        Option::TextPlain);

    encoder.payload();

    auto out = encoder.rdbuf();

    char buf[64];
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(buf, sizeof(buf), "%c", &timeinfo);

    ESP_LOGI(TAG, "The current date/time is: %s", buf);

    out->sputn(buf, strlen(buf));

    context.reply(encoder);
}