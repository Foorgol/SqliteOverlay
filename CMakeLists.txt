project(SqliteOverlay)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
add_library(${PROJECT_NAME} SHARED ${SRC_LIST})

set(LIB_VERSION_MAJOR 0)
set(LIB_VERSION_MINOR 1)
set(LIB_VERSION_PATCH 1)
set(LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${LIB_VERSION_STRING}
                                          SOVERSION ${LIB_VERSION_MAJOR})
