### Project Info ###

cmake_minimum_required(VERSION 3.1)
project(Run-GBC)
set(PROJECT_DISPLAY_NAME "Run-G.B.C.")

# Version info
set(VERSION_TYPE "Pre-Alpha")
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_MICRO 0)
set(VERSION_BUILD 1)

# Build type
if(NOT CMAKE_BUILD_TYPE)
   message(STATUS "No build type selected, defaulting to Debug")
   set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE)

# Compiler requirements
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   set(LINUX true)
else(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   set(LINUX false)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
   set(64_BIT true)
else(CMAKE_SIZEOF_VOID_P EQUAL 8)
   set(64_BIT false)
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

# rpath handling
if(LINUX)
   set(CMAKE_INSTALL_RPATH "$ORIGIN/")
endif(LINUX)

# Directories
set(SRC_DIR "${PROJECT_SOURCE_DIR}/source")
set(DEP_DIR "${PROJECT_SOURCE_DIR}/dependencies")
set(BIN_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include")
set(RES_DIR "${PROJECT_SOURCE_DIR}/resources")
set(BIN_RES_DIR "${PROJECT_BINARY_DIR}/resources")

# Options
option(LOG_TO_FILE "Enable logging to a file" OFF)
option(LOG_MSVC_STYLE "Format logs for MSVC" ${MSVC})

# Generated content
configure_file(
  "${SRC_DIR}/Constants.h.in"
  "${BIN_INCLUDE_DIR}/Constants.h"
)

# Source content
set(SOURCES
   ${SRC_DIR}/main.cpp
   ${SRC_DIR}/gbc/Cartridge.cpp
   ${SRC_DIR}/gbc/CPU.cpp
   ${SRC_DIR}/gbc/Device.cpp
   ${SRC_DIR}/gbc/Memory.cpp
   ${SRC_DIR}/test/CPUTester.cpp
   ${SRC_DIR}/wrapper/audio/AudioManager.cpp
   ${SRC_DIR}/wrapper/input/KeyboardInputDevice.cpp
   ${SRC_DIR}/wrapper/platform/IOUtils.cpp
   ${SRC_DIR}/wrapper/platform/OSUtils.cpp
   ${SRC_DIR}/wrapper/video/Mesh.cpp
   ${SRC_DIR}/wrapper/video/Model.cpp
   ${SRC_DIR}/wrapper/video/Renderer.cpp
   ${SRC_DIR}/wrapper/video/Shader.cpp
   ${SRC_DIR}/wrapper/video/ShaderProgram.cpp
   ${SRC_DIR}/wrapper/video/Texture.cpp
)

set(HEADERS
   ${BIN_INCLUDE_DIR}/Constants.h
   ${SRC_DIR}/FancyAssert.h
   ${SRC_DIR}/GLIncludes.h
   ${SRC_DIR}/Log.h
   ${SRC_DIR}/Pointers.h
   ${SRC_DIR}/gbc/Bootstrap.h
   ${SRC_DIR}/gbc/Cartridge.h
   ${SRC_DIR}/gbc/CPU.h
   ${SRC_DIR}/gbc/Device.h
   ${SRC_DIR}/gbc/Memory.h
   ${SRC_DIR}/gbc/Video.h
   ${SRC_DIR}/test/CPUTester.h
   ${SRC_DIR}/wrapper/audio/AudioManager.h
   ${SRC_DIR}/wrapper/input/InputDevice.h
   ${SRC_DIR}/wrapper/input/KeyboardInputDevice.h
   ${SRC_DIR}/wrapper/platform/IOUtils.h
   ${SRC_DIR}/wrapper/platform/OSUtils.h
   ${SRC_DIR}/wrapper/video/Mesh.h
   ${SRC_DIR}/wrapper/video/Model.h
   ${SRC_DIR}/wrapper/video/Renderer.h
   ${SRC_DIR}/wrapper/video/Shader.h
   ${SRC_DIR}/wrapper/video/ShaderProgram.h
   ${SRC_DIR}/wrapper/video/Texture.h
)

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
set(GLAD_DIR "${DEP_DIR}/glad")
attach_lib("${GLAD_DIR}/include" "${GLAD_DIR}/src/glad.c" "${CMAKE_DL_LIBS}")

# Pempek Assert
set(PEMPEK_DIR "${DEP_DIR}/PempekAssert")
attach_lib("${PEMPEK_DIR}/include" "${PEMPEK_DIR}/lib/pempek_assert.cpp" "")

# templog
set(TEMPLOG_DIR "${DEP_DIR}/templog")
attach_lib("${TEMPLOG_DIR}/include" "${TEMPLOG_DIR}/imp/logging.cpp" "")

## Static ##

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build shared libraries")

# Boxer
set(BOXER_DIR "${DEP_DIR}/Boxer")
add_subdirectory(${BOXER_DIR})
attach_lib("${BOXER_DIR}/include" "" "Boxer")

## Shared ##

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build shared libraries")

# GLFW
set(GLFW_DIR "${DEP_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
set(GLFW_USE_CHDIR OFF CACHE INTERNAL "Make glfwInit chdir to Contents/Resources")
add_subdirectory(${GLFW_DIR})
attach_lib("${GLFW_DIR}/include" "" "glfw")

# OpenAL
set(OPENAL_DIR "${DEP_DIR}/openal-soft")
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

# Includes
include_directories(${INCLUDES})

# Binary
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link
target_link_libraries(${PROJECT_NAME} ${LIBS})

include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
   target_compile_definitions(${PROJECT_NAME} PRIVATE "GBC_BIG_ENDIAN")
else (IS_BIG_ENDIAN)
   target_compile_definitions(${PROJECT_NAME} PRIVATE "GBC_LITTLE_ENDIAN")
endif (IS_BIG_ENDIAN)

if (WIN32)
   target_compile_definitions(${PROJECT_NAME} PRIVATE "NOMINMAX")
endif (WIN32)

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
     "${RES_DIR}/osx/Info.plist.in"
     "${BIN_RES_DIR}/Info.plist"
   )

   # Copy the generated Info.plist file
   install(FILES "${BIN_RES_DIR}/Info.plist" DESTINATION "${CONTENTS_DIR}")

   # Copy the icon
   install(FILES "${RES_DIR}/osx/${PROJECT_NAME}.icns" DESTINATION "${RESOURCES_DIR}")

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
