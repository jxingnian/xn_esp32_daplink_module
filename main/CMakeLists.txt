idf_component_register(SRCS "main.c" "usb_init.c" "usb_descriptors.c" "dap_handler.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_tinyusb tinyusb DAP xn_web_wifi_manger xn_esp_frpc
                    PRIV_REQUIRES nvs_flash)
