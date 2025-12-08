idf_component_register(SRCS "main.c" "usb_init.c" "usb_descriptors.c" "dap_handler.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_tinyusb tinyusb DAP)
