idf_component_register(SRCS "main.cpp" "app.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES estdlib embr esp_helper mc-coap
                    )

component_compile_options("-fstack-usage" "-Wstack-usage=128")

#component_compile_options("-Wstack-usage=128" "-DFEATURE_ESTD_CHARTRAITS=1")
#component_compile_options("-Wstack-usage=64" "-S")