cmake_minimum_required(VERSION 3.8)

### Project Info ###

# Project definition
project(glad VERSION 0.1.14 LANGUAGES C)
set(DEBUG_PROJECT_NAME "${PROJECT_NAME}-debug")

# Directories
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/release/include")
set(DEBUG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/debug/include")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/release/src")
set(DEBUG_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/debug/src")

### Source Content ###

# Headers
set(HEADERS
   "${INCLUDE_DIR}/glad/glad.h"
   "${INCLUDE_DIR}/KHR/khrplatform.h"
)
set(DEBUG_HEADERS
   "${DEBUG_INCLUDE_DIR}/glad/glad.h"
   "${DEBUG_INCLUDE_DIR}/KHR/khrplatform.h"
)

# Sources
set(SOURCES
   "${SRC_DIR}/glad.c"
)
set(DEBUG_SOURCES
   "${DEBUG_SRC_DIR}/glad.c"
)

### Target Info ###

# glad library
add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC "${INCLUDE_DIR}")
target_link_libraries(${PROJECT_NAME} PUBLIC ${CMAKE_DL_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "${PROJECT_NAME}")

# glad-debug library
add_library(${DEBUG_PROJECT_NAME} ${DEBUG_HEADERS} ${DEBUG_SOURCES})
target_include_directories(${DEBUG_PROJECT_NAME} PUBLIC "${DEBUG_INCLUDE_DIR}")
target_link_libraries(${DEBUG_PROJECT_NAME} PUBLIC ${CMAKE_DL_LIBS})
set_target_properties(${DEBUG_PROJECT_NAME} PROPERTIES FOLDER "${PROJECT_NAME}")
