### Project Info ###

cmake_minimum_required(VERSION 3.8)
project(Run-GBC VERSION 0.0.0.1 LANGUAGES C CXX)
set(PROJECT_DISPLAY_NAME "Run-G.B.C.")

# Build type
if(NOT CMAKE_BUILD_TYPE)
   message(STATUS "No build type selected, defaulting to Debug")
   set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE)

# Compiler requirements
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
set(LINUX FALSE)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   set(LINUX TRUE)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# rpath handling
if(LINUX)
   set(CMAKE_INSTALL_RPATH "$ORIGIN/")
endif(LINUX)

# Directories
set(SRC_DIR "${PROJECT_SOURCE_DIR}/Source")
set(LIB_DIR "${PROJECT_SOURCE_DIR}/Libraries")
set(BIN_INCLUDE_DIR "${PROJECT_BINARY_DIR}/Include")
set(RES_DIR "${PROJECT_SOURCE_DIR}/Resources")
set(BIN_RES_DIR "${PROJECT_BINARY_DIR}/Resources")

# Options
set(GBC_LOG_MSVC_STYLE_DEFAULT OFF)
if(MSVC)
   set(GBC_LOG_MSVC_STYLE_DEFAULT ON)
endif(MSVC)
option(GBC_LOG_MSVC_STYLE "Format logs for MSVC" ${GBC_LOG_MSVC_STYLE_DEFAULT})
option(GBC_RUN_TESTS "Run emulator tests" OFF)

# Generated content
configure_file(
  "${SRC_DIR}/Constants.h.in"
  "${BIN_INCLUDE_DIR}/Constants.h"
)

# Source content
include(Sources.cmake)

set(SOURCES)
foreach(SOURCE_NAME ${SOURCE_NAMES})
   list(APPEND SOURCES "${SRC_DIR}/${SOURCE_NAME}")
endforeach(SOURCE_NAME)

set(HEADERS)
foreach(HEADER_NAME ${HEADER_NAMES})
   list(APPEND HEADERS "${SRC_DIR}/${HEADER_NAME}")
endforeach(HEADER_NAME)

set(RESOURCES)

source_group(TREE ${SRC_DIR} PREFIX Sources FILES ${SOURCES} ${HEADERS})
source_group("Sources\\Generated" "${BIN_INCLUDE_DIR}/*")
source_group("Libraries" "${LIB_DIR}/*")
source_group("Resources" "${RES_DIR}/*")

if(APPLE)
   list(APPEND RESOURCES
      "${RES_DIR}/macOS/Info.plist.in"
      "${RES_DIR}/macOS/Run-GBC.icns"
   )
elseif(WIN32)
   list(APPEND RESOURCES
      "${RES_DIR}/Windows/Run-GBC.rc"
      "${RES_DIR}/Windows/Run-GBC.ico"
   )
endif(APPLE)

list(APPEND HEADERS "${BIN_INCLUDE_DIR}/Constants.h")

set(INCLUDES)
list(APPEND INCLUDES
   ${SRC_DIR}
   ${BIN_INCLUDE_DIR}
)

### Libraries ###

function(attach_lib include source lib)
   if(include)
      list(APPEND INCLUDES ${include})
      set(INCLUDES ${INCLUDES} PARENT_SCOPE)
   endif(include)

   if(source)
      list(APPEND SOURCES ${source})
      set(SOURCES ${SOURCES} PARENT_SCOPE)
   endif(source)

   if(lib)
      list(APPEND LIBS ${lib})
      set(LIBS ${LIBS} PARENT_SCOPE)

      if(BUILD_SHARED_LIBS)
         list(APPEND COPY_LIBS ${lib})
         set(COPY_LIBS ${COPY_LIBS} PARENT_SCOPE)
      endif(BUILD_SHARED_LIBS)
   endif(lib)
endfunction(attach_lib)

## Integrated ##

# glad
set(GLAD_DIR "${LIB_DIR}/glad")
attach_lib("${GLAD_DIR}/include" "${GLAD_DIR}/src/glad.c" "${CMAKE_DL_LIBS}")
source_group("Libraries\\glad" "${GLAD_DIR}/*")

# Pempek Assert
set(PEMPEK_DIR "${LIB_DIR}/PempekAssert")
attach_lib("${PEMPEK_DIR}/include" "${PEMPEK_DIR}/lib/pempek_assert.cpp" "")
source_group("Libraries\\PempekAssert" "${PEMPEK_DIR}/*")

# readerwriterqueue
set(QUEUE_DIR "${LIB_DIR}/readerwriterqueue")
attach_lib("${QUEUE_DIR}" "${QUEUE_DIR}/readerwriterqueue.h;${QUEUE_DIR}/atomicops.h" "")
source_group("Libraries\\readerwriterqueue" "${QUEUE_DIR}/*")

# templog
set(TEMPLOG_DIR "${LIB_DIR}/templog")
attach_lib("${TEMPLOG_DIR}/include" "${TEMPLOG_DIR}/imp/logging.cpp" "")
source_group("Libraries\\Templog" "${TEMPLOG_DIR}/*")

## Static ##

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build shared libraries")

# Boxer
set(BOXER_DIR "${LIB_DIR}/Boxer")
add_subdirectory(${BOXER_DIR})
attach_lib("${BOXER_DIR}/include" "" "Boxer")

## Shared ##

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build shared libraries")

# GLFW
set(GLFW_DIR "${LIB_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
set(GLFW_USE_CHDIR OFF CACHE INTERNAL "Make glfwInit chdir to Contents/Resources")
add_subdirectory(${GLFW_DIR})
attach_lib("${GLFW_DIR}/include" "" "glfw")

# OpenAL
set(OPENAL_DIR "${LIB_DIR}/openal-soft")
set(ALSOFT_UTILS OFF CACHE INTERNAL "Build and install utility programs")
set(ALSOFT_NO_CONFIG_UTIL ON CACHE INTERNAL "Disable building the alsoft-config utility")
set(ALSOFT_EXAMPLES OFF CACHE INTERNAL "Build and install example programs")
set(ALSOFT_CONFIG OFF CACHE INTERNAL "Install alsoft.conf sample configuration file")
set(ALSOFT_HRTF_DEFS OFF CACHE INTERNAL "Install HRTF definition files")
set(ALSOFT_INSTALL OFF CACHE INTERNAL "Install headers and libraries")
add_subdirectory(${OPENAL_DIR})
if(WIN32)
   set(OPENAL_LIBRARY OpenAL32)
else(WIN32)
   set(OPENAL_LIBRARY openal)
endif(WIN32)
attach_lib("${OPENAL_DIR}/include" "" "${OPENAL_LIBRARY}")

### Building ###

# Binary
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

# Includes
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})

# Link
target_link_libraries(${PROJECT_NAME} ${LIBS})

# Platform compile definitions
include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
   target_compile_definitions(${PROJECT_NAME} PRIVATE "GBC_BIG_ENDIAN")
else(IS_BIG_ENDIAN)
   target_compile_definitions(${PROJECT_NAME} PRIVATE "GBC_LITTLE_ENDIAN")
endif(IS_BIG_ENDIAN)

if(WIN32)
   target_compile_definitions(${PROJECT_NAME} PRIVATE "NOMINMAX")
endif(WIN32)

### Post-Build ###

# Copy DLLs
if(WIN32)
   foreach(COPY_LIB ${COPY_LIBS})
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${COPY_LIB}>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
      )
   endforeach(COPY_LIB)
endif(WIN32)

### Install ###

## OS X ##

if(APPLE)
   set(APP_DIR "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app")
   set(CONTENTS_DIR "${APP_DIR}/Contents")
   set(MACOS_DIR "${CONTENTS_DIR}/MacOS")
   set(RESOURCES_DIR "${CONTENTS_DIR}/Resources")

   # Generate the .app file
   install(CODE "
      file(MAKE_DIRECTORY \"${MACOS_DIR}\")
      file(MAKE_DIRECTORY \"${RESOURCES_DIR}\")")

   # Generate the Info.plist file (at build time, not install time)
   configure_file(
     "${RES_DIR}/macOS/Info.plist.in"
     "${BIN_RES_DIR}/Info.plist"
   )

   # Copy the generated Info.plist file
   install(FILES "${BIN_RES_DIR}/Info.plist" DESTINATION "${CONTENTS_DIR}")

   # Copy the icon
   install(FILES "${RES_DIR}/macOS/${PROJECT_NAME}.icns" DESTINATION "${RESOURCES_DIR}")

   # Copy the executable (not install, so that the internal lib paths don't change)
   install(FILES "$<TARGET_FILE:${PROJECT_NAME}>" DESTINATION "${MACOS_DIR}"
           PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

   # Fix the bundle (copies and fixes shared libraries)
   install(CODE "
      include(BundleUtilities)
      fixup_bundle(\"${APP_DIR}\" \"\" \"\")")
endif(APPLE)

## Windows ##

if(WIN32)
   # Install the executable
   install(TARGETS ${PROJECT_NAME} DESTINATION ".")

   # Install all shared libraries
   foreach(COPY_LIB ${COPY_LIBS})
      install(FILES "$<TARGET_FILE:${COPY_LIB}>" DESTINATION ".")
   endforeach(COPY_LIB)

   # Install system libraries
   set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP true) # Let us choose the install location
   include(InstallRequiredSystemLibraries)
   if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
      install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ".")
   endif(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
endif(WIN32)

## Linux ##

if(LINUX)
   # Copy the executable (not install, so that the rpath doesn't change)
   install(FILES "$<TARGET_FILE:${PROJECT_NAME}>" DESTINATION "${CMAKE_INSTALL_PREFIX}"
           PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

   # Fix the bundle (in order to copy shared libraries)
   # Note: verify_app (called at the end of fixup_bundle) WILL fail, due to referencing external libraries
   # In order to ignore the error / continue, replace the verify_app function with one that does nothing
   install(CODE "
      include(BundleUtilities)
      function(verify_app app)
         message(STATUS \"(Not actually verifying...)\")
      endfunction(verify_app)
      fixup_bundle(\"${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}\" \"\" \"\")")

   # Install the executable (this time, fixing the rpath)
   install(TARGETS ${PROJECT_NAME} DESTINATION ".")

   # Fix the bundle (just to do a proper verify)
   install(CODE "
      include(BundleUtilities)
      fixup_bundle(\"${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}\" \"\" \"\")")
endif(LINUX)
