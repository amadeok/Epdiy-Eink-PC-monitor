set(app_sources "main.c" "pc_monitor.c")

idf_component_register(SRCS ${app_sources} INCLUDE_DIRS "../include" "../../../src/epd_driver" "../../../src/epd_driver" REQUIRES epd_driver nvs_flash)
