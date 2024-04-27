set(app_sources "main.c" "pc_monitor.c")

idf_component_register(SRCS ${app_sources} INCLUDE_DIRS "../include" "../../../src" "../../../src/output_i2s" "../../../src/output_common" REQUIRES epdiy nvs_flash json)