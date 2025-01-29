# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
set(INCLUDE_DIRS
    
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/*.*
    ${CMAKE_SOURCE_DIR}/src/*.*/*.*
    ${CMAKE_SOURCE_DIR}/include
    # ${CMAKE_SOURCE_DIR}/include/*.*
    # ${CMAKE_SOURCE_DIR}/include/*.*/*.*
    # ${CMAKE_SOURCE_DIR}/include/*.*/*.*/*.*
    # ${CMAKE_SOURCE_DIR}/include/*.*/*.*/*.*/*.*
    # ${CMAKE_SOURCE_DIR}/components/*.*
    # ${CMAKE_SOURCE_DIR}/components/*.*/*.*
    # ${CMAKE_SOURCE_DIR}/components/*.*/*.*/*.*
    # ${CMAKE_SOURCE_DIR}/components/*.*/*.*/*.*/*.*
    # ${CMAKE_SOURCE_DIR}/components/*.*/*.*/*.*/*.*/*.*
    # ${CMAKE_SOURCE_DIR}/components/*.*/*.*/*.*/*.*/*.*/*.*
    # ${CMAKE_SOURCE_DIR}/components/*.*/*.*/*.*/*.*/*.*/*.*/*.*
)
idf_component_register(SRCS ${app_sources})
