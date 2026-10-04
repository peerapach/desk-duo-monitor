cmake_minimum_required(VERSION 3.16)
set(PROJECT_VER "rollback-probe")
# PlatformIO requires its generated language-probe component alongside main.
set(COMPONENTS main __pio_env)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(desk_duo)
