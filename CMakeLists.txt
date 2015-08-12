### Project Info ###

cmake_minimum_required(VERSION 2.8)
project(run-gbc)
set(PROJECT_DISPLAY_NAME "Run-G.B.C.")
set(PROJECT_DISPLAY_NAME_FILE_SAFE "Run-GBC")

# Version info
set(VERSION_TYPE "Pre-Alpha")
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_MICRO 0)
set(VERSION_BUILD 1)

# Compiler requirements
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Directories
set(SRC_DIR "${PROJECT_SOURCE_DIR}/source")
set(DEP_DIR "${PROJECT_SOURCE_DIR}/dependencies")
set(BIN_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include")
set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")

# Options
option(LOG_TO_FILE "Enable logging to a file" OFF)

# Generated content
configure_file(
  "${SRC_DIR}/Constants.h.in"
  "${BIN_INCLUDE_DIR}/Constants.h"
)

# Source content
set(SOURCES
   ${SRC_DIR}/main.cpp
   ${SRC_DIR}/gbc/CPU.cpp
   ${SRC_DIR}/wrapper/audio/AudioManager.cpp
   ${SRC_DIR}/wrapper/input/KeyboardInputDevice.cpp
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
   ${SRC_DIR}/LogHelper.h
   ${SRC_DIR}/Types.h
   ${SRC_DIR}/gbc/CPU.h
   ${SRC_DIR}/gbc/Memory.h
   ${SRC_DIR}/gbc/Video.h
   ${SRC_DIR}/wrapper/audio/AudioManager.h
   ${SRC_DIR}/wrapper/input/InputDevice.h
   ${SRC_DIR}/wrapper/input/KeyboardInputDevice.h
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

function(attachlib include source lib)
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
endfunction(attachlib)

## Integrated ##

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build shared libraries")

# Pempek Assert
set(PEMPEK_DIR "${DEP_DIR}/PempekAssert")
attachlib("${PEMPEK_DIR}/include" "${PEMPEK_DIR}/lib/pempek_assert.cpp" "")

# templog
set(TEMPLOG_DIR "${DEP_DIR}/templog")
attachlib("${TEMPLOG_DIR}/include" "${TEMPLOG_DIR}/imp/logging.cpp" "")

# glad
set(GLAD_DIR "${DEP_DIR}/glad")
attachlib("${GLAD_DIR}/include" "${GLAD_DIR}/src/glad.c" "${CMAKE_DL_LIBS}")

## Static ##

# Boxer
set(BOXER_DIR "${DEP_DIR}/Boxer")
add_subdirectory(${BOXER_DIR})
attachlib("${BOXER_DIR}/include" "" "Boxer")

## Shared ##

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build shared libraries")

# GLFW
set(GLFW_DIR "${DEP_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory(${GLFW_DIR})
attachlib("${GLFW_DIR}/include" "" "glfw")

# OpenAL
set(OPENAL_DIR "${DEP_DIR}/openal-soft")
set(ALSOFT_UTILS OFF CACHE INTERNAL "Build and install utility programs")
set(ALSOFT_NO_CONFIG_UTIL ON CACHE INTERNAL "Disable building the alsoft-config utility")
set(ALSOFT_EXAMPLES OFF CACHE INTERNAL "Build and install example programs")
set(ALSOFT_CONFIG OFF CACHE INTERNAL "Install alsoft.conf sample configuration file")
set(ALSOFT_HRTF_DEFS OFF CACHE INTERNAL "Install HRTF definition files")
add_subdirectory(${OPENAL_DIR})
if(WIN32)
   set(OPENAL_LIBRARY OpenAL32)
else(WIN32)
   set(OPENAL_LIBRARY openal)
endif(WIN32)
attachlib("${OPENAL_DIR}/include" "" "${OPENAL_LIBRARY}")

### Building ###

# Includes
include_directories(${INCLUDES})

# Binary
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link
target_link_libraries(${PROJECT_NAME} ${LIBS})

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

set(INSTALL_DIR ${PROJECT_DISPLAY_NAME_FILE_SAFE})

# Install the executable
install(TARGETS ${PROJECT_NAME} DESTINATION "${INSTALL_DIR}" COMPONENT Runtime)

# Install all shared libraries
foreach(COPY_LIB ${COPY_LIBS})
   install(FILES "$<TARGET_FILE:${COPY_LIB}>" DESTINATION ${INSTALL_DIR} COMPONENT Runtime)
endforeach(COPY_LIB)

# Install system libraries
if(WIN32)
   include(InstallRequiredSystemLibraries)
   if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
      install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION "${INSTALL_DIR}" COMPONENT Runtime)
   endif(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
endif(WIN32)
