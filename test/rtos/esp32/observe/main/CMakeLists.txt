idf_component_register(SRCS "../../template/main.cpp" app.cpp resource.cpp
                    INCLUDE_DIRS "."
                    REQUIRES estdlib embr esp_helper mc-coap
                    )

component_compile_options("-fstack-usage" "-Wstack-usage=192")

