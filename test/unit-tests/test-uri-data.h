#pragma once

#include <estd/type_traits.h>
#include <estd/cstdint.h>
#include <exp/uripathmap.h>

namespace uri {

CONSTEXPR int id_path_v1 = 0;
CONSTEXPR int id_path_v1_api = 1;
CONSTEXPR int id_path_v1_api_power = 2;
CONSTEXPR int id_path_v1_api_test = 6;
CONSTEXPR int id_path_dummy = 3;
CONSTEXPR int id_path_v2 = 4;
CONSTEXPR int id_path_v2_api = 5;
CONSTEXPR int id_path_well_known = 7;
CONSTEXPR int id_path_well_known_core = 8;

// sort by parent id first, then by node id
// this way we can easily optimize incoming request parsing by remembering
// where we left off on found node id position
// NOTE: might be better to sort by uri_path secondly instead of node_id,
// as it potentially could speed up searching for said path.  However,
// any of the secondary sorting the benefit is limited by our ability to know
// how large the 'parent' region is in which we are sorting, which so far
// is elusive
const moducom::coap::experimental::UriPathMap map[] =
{
    { "v1",     id_path_v1,             MCCOAP_URIPATH_NONE },
    { "api",    id_path_v1_api,         id_path_v1 },
    { "power",  id_path_v1_api_power,   id_path_v1_api },
    { "test",   id_path_v1_api_test,    id_path_v1_api },

    { "v2",     id_path_v2,             MCCOAP_URIPATH_NONE },
    { "api",    id_path_v2_api,         id_path_v2 },

    { ".well-known",    id_path_well_known,         0 },
    { "core",           id_path_well_known_core,    id_path_well_known }
};

}