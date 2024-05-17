idf_component_register(SRCS 
                    "ack_receiver.cpp"
                    "app.cpp"
                    # DEBT: Weak linkage seems pretty broken, so adapting 'main'
                    "main2.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES estdlib embr esp_helper embr-coap)
