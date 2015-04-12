### Project Info ###

cmake_minimum_required(VERSION 2.8)
project(run-gbc)

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
)

set(HEADERS
   ${BIN_INCLUDE_DIR}/Constants.h
   ${SRC_DIR}/FancyAssert.h
   ${SRC_DIR}/LogHelper.h
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

### Building ###

# C++11
if (NOT WIN32)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif (NOT WIN32)

# Includes
include_directories(
   ${SRC_DIR}
   ${BIN_INCLUDE_DIR}
   ${PEMPEK_INCLUDE_DIR}
   ${TEMPLOG_INCLUDE_DIR}
   ${BOXER_INCLUDE_DIR}
)

# Libraries
set(RELEASE_LIBRARIES
   ${BOXER_LIBRARY}
)

set(DEBUG_LIBRARIES
   ${BOXER_LIBRARY}
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