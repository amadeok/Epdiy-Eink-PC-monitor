set(app_sources "main.c" "pc_monitor.c")

idf_component_register(SRCS ${app_sources} INCLUDE_DIRS "../include" "../../../components/epd_driver" "../../../components/epd_driver" REQUIRES epd_driver nvs_flash)
