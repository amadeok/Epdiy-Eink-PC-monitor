cmake_minimum_required(VERSION 3.16.0)
set(EXTRA_COMPONENT_DIRS "../../src/" "$ENV{IDF_PATH}/components/nvs_flash")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(firmware)
