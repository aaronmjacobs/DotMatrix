### Project ###

cmake_minimum_required(VERSION 3.17)
project(DotMatrixCore VERSION 0.0.0.1 LANGUAGES CXX C)

# Options
option(DOT_MATRIX_WITH_BOOTSTRAP "Enable bootstrap" OFF)
option(DOT_MATRIX_WITH_DEBUGGER "Enable debugger" OFF)
option(DOT_MATRIX_WITH_PER_CHANNEL_AUDIO_BUFFERS "Enable per-channel audio buffers" OFF)

### Target ###

add_library(${PROJECT_NAME} "")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PUBLIC DM_DEBUG=$<CONFIG:Debug>)
target_compile_definitions(${PROJECT_NAME} PUBLIC DM_WITH_BOOTSTRAP=$<BOOL:${DOT_MATRIX_WITH_BOOTSTRAP}>)
target_compile_definitions(${PROJECT_NAME} PUBLIC DM_WITH_DEBUGGER=$<BOOL:${DOT_MATRIX_WITH_DEBUGGER}>)
target_compile_definitions(${PROJECT_NAME} PUBLIC DM_WITH_PER_CHANNEL_AUDIO_BUFFERS=$<BOOL:${DOT_MATRIX_WITH_PER_CHANNEL_AUDIO_BUFFERS}>)

include(TestBigEndian)
TEST_BIG_ENDIAN(DOT_MATRIX_IS_BIG_ENDIAN)
target_compile_definitions(${PROJECT_NAME} PUBLIC DM_IS_BIG_ENDIAN=$<BOOL:${DOT_MATRIX_IS_BIG_ENDIAN}>)

if(WIN32)
   target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
endif()

set_property(DIRECTORY ${CMAKE_PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

### Source ###

# Source files
include(Source.cmake)

# Libraries
include(Libraries.cmake)
