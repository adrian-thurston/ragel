cmake_minimum_required(VERSION 3.0)

project(ragel
    VERSION 7.0.0.10
    LANGUAGES C CXX)
set(PROJECT_PUBDATE "May 2017")
set(PROJECT_EXPECTED_COLM 0.13.0.5)

find_package(colm ${PROJECT_EXPECTED_COLM} REQUIRED)

set(${PROJECT_NAME}_MAKE_INSTALL ON CACHE BOOL
    "Set to OFF to disable install rules (default is ON)")

add_subdirectory(src)
