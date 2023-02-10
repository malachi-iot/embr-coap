idf_component_register(SRCS "esp-now.cpp" "main.cpp" "udp-coap.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES estdlib embr esp_helper mc-coap
                    )
