// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

using namespace moducom::coap;

#include "context.h"
#include "observer-esp-idf.h"


constexpr int id_path_v1 = 0;
constexpr int id_path_v1_api = 1;
constexpr int id_path_v1_api_test = 3;
constexpr int id_path_v1_api_version = 4;
constexpr int id_path_v1_api_stats = 5;
constexpr int id_path_well_known = 20;
constexpr int id_path_well_known_core = 21;

// NOTE: Alphabetization is important.  id# ordering is not
const UriPathMap uri_map[7] =
{
    { "v1",         id_path_v1,                 MCCOAP_URIPATH_NONE },
    { "api",        id_path_v1_api,             id_path_v1 },
    { "stats",      id_path_v1_api_stats,       id_path_v1_api },
    { "test",       id_path_v1_api_test,        id_path_v1_api },
    { "version",    id_path_v1_api_version,     id_path_v1_api },

    { ".well-known",    id_path_well_known,         MCCOAP_URIPATH_NONE },
    { "core",           id_path_well_known_core,    id_path_well_known }
};


struct Observer : ExperimentalDecoderEventTypedefs
{
    static constexpr const char* TAG = "Observer";

    void on_notify(completed_event, AppContext& context)
    {
        ESP_LOGI(TAG, "on_notify completed");
    }
};


embr::layer0::subject<
    HeaderContextObserver,
    TokenContextObserver,
    UriParserObserver,
    VersionObserver<id_path_v1_api_version, AppContext>,
    Auto404Observer<AppContext>
    > app_subject;


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