### Project Info ###

cmake_minimum_required(VERSION 2.8)
project(run-gbc)
set(PROJECT_DISPLAY_NAME "Run-G.B.C.")

# Version info
set(VERSION_TYPE "Pre-Alpha")
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_MICRO 0)
set(VERSION_BUILD 1)

# Directories
set(SRC_DIR "${PROJECT_SOURCE_DIR}/source")
set(DEP_DIR "${PROJECT_SOURCE_DIR}/dependencies")
set(BIN_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include")
set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")

# Options
option(LOG_TO_FILE "Enable logging to a file" OFF)

# Generated content
configure_file (
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

### Libraries ###

# Pempek Assert
set(PEMPEK_DIR "${DEP_DIR}/PempekAssert")
set(PEMPEK_INCLUDE_DIR "${PEMPEK_DIR}/include")
list(APPEND SOURCES
   ${PEMPEK_DIR}/lib/pempek_assert.cpp
)

# templog
set(TEMPLOG_DIR "${DEP_DIR}/templog")
set(TEMPLOG_INCLUDE_DIR "${TEMPLOG_DIR}/include")
list(APPEND SOURCES
   ${TEMPLOG_DIR}/imp/logging.cpp
)

# Boxer
set(BOXER_DIR "${DEP_DIR}/Boxer")
set(BOXER_INCLUDE_DIR "${BOXER_DIR}/include")
set(BOXER_LIBRARY Boxer)
add_subdirectory(${BOXER_DIR})

# GLFW
set(GLFW_DIR "${DEP_DIR}/glfw")
set(GLFW_INCLUDE_DIR "${GLFW_DIR}/include")
set(GLFW_LIBRARY glfw)
set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build shared libraries")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory(${GLFW_DIR})

# glad
set(GLAD_DIR "${DEP_DIR}/glad")
set(GLAD_INCLUDE_DIR "${GLAD_DIR}/include")
list(APPEND SOURCES
   ${GLAD_DIR}/src/glad.c
)

# OpenAL
set(OPENAL_DIR "${DEP_DIR}/openal-soft")
set(OPENAL_INCLUDE_DIR "${OPENAL_DIR}/include")
if(WIN32)
   set(OPENAL_LIBRARY OpenAL32)
else()
   set(OPENAL_LIBRARY openal)
endif()
set(ALSOFT_UTILS OFF CACHE INTERNAL "Build and install utility programs")
set(ALSOFT_NO_CONFIG_UTIL ON CACHE INTERNAL "Disable building the alsoft-config utility")
set(ALSOFT_EXAMPLES OFF CACHE INTERNAL "Build and install example programs")
set(ALSOFT_CONFIG OFF CACHE INTERNAL "Install alsoft.conf sample configuration file")
set(ALSOFT_HRTF_DEFS OFF CACHE INTERNAL "Install HRTF definition files")
add_subdirectory(${OPENAL_DIR})

### Building ###

# C++11
if (NOT WIN32)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif (NOT WIN32)

# OpenGL
find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
    message(ERROR " OpenGL not found!")
endif(NOT OPENGL_FOUND)

# Includes
include_directories(
   ${SRC_DIR}
   ${BIN_INCLUDE_DIR}
   ${PEMPEK_INCLUDE_DIR}
   ${TEMPLOG_INCLUDE_DIR}
   ${BOXER_INCLUDE_DIR}
   ${GLFW_INCLUDE_DIR}
   ${GLAD_INCLUDE_DIR}
   ${OPENAL_INCLUDE_DIR}
)

# Libraries
set(RELEASE_LIBRARIES
   ${OPENGL_LIBRARIES}
   ${BOXER_LIBRARY}
   ${GLFW_LIBRARY}
   ${CMAKE_DL_LIBS} # Needed for glad on Linux
   ${OPENAL_LIBRARY}
)

set(DEBUG_LIBRARIES
   ${OPENGL_LIBRARIES}
   ${BOXER_LIBRARY}
   ${GLFW_LIBRARY}
   ${CMAKE_DL_LIBS} # Needed for glad on Linux
   ${OPENAL_LIBRARY}
)

foreach(RELEASE_LIBRARY ${RELEASE_LIBRARIES})
   list(APPEND TAGGED_RELEASE_LIBRARIES
      optimized ${RELEASE_LIBRARY}
   )
endforeach(RELEASE_LIBRARY)

foreach(DEBUG_LIBRARY ${DEBUG_LIBRARIES})
   list(APPEND TAGGED_DEBUG_LIBRARIES
      debug ${DEBUG_LIBRARY}
   )
endforeach(DEBUG_LIBRARY)

# Binary
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link
target_link_libraries(${PROJECT_NAME} ${TAGGED_RELEASE_LIBRARIES} ${TAGGED_DEBUG_LIBRARIES})
