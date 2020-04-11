### Project ###

cmake_minimum_required(VERSION 3.8)
project(Run-GBC VERSION 0.0.0.1 LANGUAGES CXX C)
set(PROJECT_DISPLAY_NAME "Run-G.B.C.")

# Options
set(GBC_LOG_MSVC_STYLE_DEFAULT OFF)
if(MSVC)
   set(GBC_LOG_MSVC_STYLE_DEFAULT ON)
endif()
option(GBC_LOG_MSVC_STYLE "Format logs for MSVC" ${GBC_LOG_MSVC_STYLE_DEFAULT})
option(GBC_RUN_TESTS "Run emulator tests" OFF)
option(GBC_ENABLE_BOOTSTRAP "Enable bootstrap" OFF)
option(GBC_ENABLE_UI "Enable UI" ON)
option(GBC_ENABLE_DEBUGGER "Enable debugger" ON)
if(${GBC_ENABLE_BOOTSTRAP})
   set(GBC_WITH_BOOTSTRAP ON)
else()
   set(GBC_WITH_BOOTSTRAP OFF)
endif()
if(${GBC_ENABLE_UI} AND NOT ${GBC_RUN_TESTS})
   set(GBC_WITH_UI ON)
else()
   set(GBC_WITH_UI OFF)
endif()
if(${GBC_WITH_UI} AND ${GBC_ENABLE_DEBUGGER})
   set(GBC_WITH_DEBUGGER ON)
else()
   set(GBC_WITH_DEBUGGER OFF)
endif()

### Target ###

add_executable(${PROJECT_NAME} "")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PUBLIC GBC_DEBUG=$<CONFIG:Debug>)
target_compile_definitions(${PROJECT_NAME} PRIVATE GBC_LOG_MSVC_STYLE=$<BOOL:${GBC_LOG_MSVC_STYLE}>)
target_compile_definitions(${PROJECT_NAME} PRIVATE GBC_RUN_TESTS=$<BOOL:${GBC_RUN_TESTS}>)
target_compile_definitions(${PROJECT_NAME} PRIVATE GBC_WITH_BOOTSTRAP=$<BOOL:${GBC_WITH_BOOTSTRAP}>)
target_compile_definitions(${PROJECT_NAME} PRIVATE GBC_WITH_UI=$<BOOL:${GBC_WITH_UI}>)
target_compile_definitions(${PROJECT_NAME} PRIVATE GBC_WITH_DEBUGGER=$<BOOL:${GBC_WITH_DEBUGGER}>)

include(TestBigEndian)
TEST_BIG_ENDIAN(GBC_IS_BIG_ENDIAN)
target_compile_definitions(${PROJECT_NAME} PRIVATE GBC_IS_BIG_ENDIAN=$<BOOL:${GBC_IS_BIG_ENDIAN}>)

include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX("filesystem" GBC_HAS_FILESYSTEM_SUPPORT)
target_compile_definitions(${PROJECT_NAME} PRIVATE GBC_HAS_FILESYSTEM_SUPPORT=$<BOOL:${GBC_HAS_FILESYSTEM_SUPPORT}>)

if(WIN32)
   target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
endif()

### Source ###

# Source files
include(Source.cmake)

# Resources
set(RES_DIR "${PROJECT_SOURCE_DIR}/Resources")
set(BIN_RES_DIR "${PROJECT_BINARY_DIR}/Resources")
include(Resources.cmake)

# Libraries
include(Libraries.cmake)

### Install ###

include(Install.cmake)
