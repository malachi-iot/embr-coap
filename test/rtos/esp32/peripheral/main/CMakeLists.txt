idf_component_register(SRCS
                    "adc.cpp" "main.cpp" "app.cpp" "context.cpp" "ip.cpp"
                    "ledc.cpp" "mdns.cpp" "sntp.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES estdlib embr esp_adc esp_helper mc-coap
                    )

component_compile_options("-fstack-usage" "-Wstack-usage=256")

#component_compile_options("-Wstack-usage=128" "-DFEATURE_ESTD_CHARTRAITS=1")
#component_compile_options("-Wstack-usage=64" "-S")