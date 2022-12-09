idf_component_register(SRCS "main.cpp" "udp-coap.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES estdlib embr esp_helper mc-coap
                    )
