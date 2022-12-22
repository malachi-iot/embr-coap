target_link_libraries(${PROJECT_NAME} pico_stdlib)

# enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# wait indefinitely for a CDC to appear so we can see test results
# Beware -- reports indicate this hangs UART indefinitely
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=-1)
