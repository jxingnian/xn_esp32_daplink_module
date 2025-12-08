idf_component_register(SRCS "main.c" "usb_init.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_tinyusb tinyusb)
