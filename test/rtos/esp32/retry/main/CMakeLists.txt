idf_component_register(SRCS 
                    "ack_receiver.cpp"
                    "app.cpp"
                    # DEBT: Weak linkage seems pretty broken, so adapting 'main'
                    "main2.cpp"
                    INCLUDE_DIRS "."
                    # NOTE: Experimenting with mocks which *may* need tcp_transport
                    REQUIRES driver esp_wifi estdlib embr esp_helper embr-coap tcp_transport)
