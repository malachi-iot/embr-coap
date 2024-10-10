/**
 * @file
 * @author  Malachi Burke
 */
#pragma once

#include <catch2/catch.hpp>

#include <exp/events.h>
#include "test-data.h"

using namespace embr::coap;
using namespace embr::coap::experimental;

struct test_observer
{
    int counter;

    test_observer() : counter(0) {}

    void on_notify(event::completed)
    {
        REQUIRE(counter >= 2);
        counter++;
    }

    void on_notify(const event::option& e)
    {
        REQUIRE(e.option_number >= 270);
        counter++;
    }
};


