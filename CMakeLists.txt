### Project Info ###

cmake_minimum_required(VERSION 3.8)
project(Run-GBC VERSION 0.0.0.1 LANGUAGES C CXX)
set(PROJECT_DISPLAY_NAME "Run-G.B.C.")

# Platform detection
set(LINUX FALSE)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   set(LINUX TRUE)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

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

### Target Info ###

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})
target_compile_definitions(${PROJECT_NAME} PRIVATE GBC_DEBUG=$<CONFIG:Debug>)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)

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

### Libraries ###

set(COPY_LIBS)

## Integrated ##

# PPK_ASSERT
set(PPK_DIR "${LIB_DIR}/PPK_ASSERT")
target_sources(${PROJECT_NAME} PRIVATE "${PPK_DIR}/src/ppk_assert.h;${PPK_DIR}/src/ppk_assert.cpp")
target_include_directories(${PROJECT_NAME} PUBLIC "${PPK_DIR}/src")
source_group("Libraries\\PPK_ASSERT" "${PPK_DIR}/src")

# readerwriterqueue
set(QUEUE_DIR "${LIB_DIR}/readerwriterqueue")
target_sources(${PROJECT_NAME} PRIVATE "${QUEUE_DIR}/atomicops.h;${QUEUE_DIR}/readerwriterqueue.h")
target_include_directories(${PROJECT_NAME} PUBLIC "${QUEUE_DIR}")
source_group("Libraries\\readerwriterqueue" "${QUEUE_DIR}/*")

# templog
set(TEMPLOG_DIR "${LIB_DIR}/templog")
set(TEMPLOG_SOURCES
   "${TEMPLOG_DIR}/config.h"
   "${TEMPLOG_DIR}/logging.h"
   "${TEMPLOG_DIR}/templ_meta.h"
   "${TEMPLOG_DIR}/tuples.h"
   "${TEMPLOG_DIR}/type_lists.h"
   "${TEMPLOG_DIR}/imp/logging.cpp"
)
target_sources(${PROJECT_NAME} PRIVATE "${TEMPLOG_SOURCES}")
target_include_directories(${PROJECT_NAME} PUBLIC "${TEMPLOG_DIR}")
source_group("Libraries\\templog" "${TEMPLOG_DIR}/*")

## Static ##

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build shared libraries")

# Boxer
add_subdirectory("${LIB_DIR}/Boxer")
target_link_libraries(${PROJECT_NAME} PUBLIC Boxer)

# glad
add_subdirectory("${LIB_DIR}/glad")
target_link_libraries(${PROJECT_NAME} PUBLIC optimized glad)
target_link_libraries(${PROJECT_NAME} PUBLIC debug glad-debug)
list(APPEND COPY_LIBS "${CMAKE_DL_LIBS}")

## Shared ##

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build shared libraries")

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory("${LIB_DIR}/glfw")
target_compile_definitions(${PROJECT_NAME} PUBLIC GLFW_INCLUDE_NONE)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)
list(APPEND COPY_LIBS glfw)

# OpenAL
set(OPENAL_DIR "${LIB_DIR}/openal-soft")
set(ALSOFT_UTILS OFF CACHE INTERNAL "Build and install utility programs")
set(ALSOFT_NO_CONFIG_UTIL ON CACHE INTERNAL "Disable building the alsoft-config utility")
set(ALSOFT_EXAMPLES OFF CACHE INTERNAL "Build and install example programs")
set(ALSOFT_CONFIG OFF CACHE INTERNAL "Install alsoft.conf sample configuration file")
set(ALSOFT_HRTF_DEFS OFF CACHE INTERNAL "Install HRTF definition files")
set(ALSOFT_INSTALL OFF CACHE INTERNAL "Install headers and libraries")
add_subdirectory("${LIB_DIR}/openal-soft")
if(WIN32)
   set(OPENAL_LIBRARY OpenAL32)
else(WIN32)
   set(OPENAL_LIBRARY openal)
endif(WIN32)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENAL_LIBRARY})
# openal-soft doesn't populate INTERFACE_INCLUDE_DIRECTORIES, so we need to manually add the includes here
target_include_directories(${PROJECT_NAME} PUBLIC $<TARGET_PROPERTY:${OPENAL_LIBRARY},INCLUDE_DIRECTORIES>)
list(APPEND COPY_LIBS ${OPENAL_LIBRARY})

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

## macOS ##

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
   set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".") # Copy into the same folder as the exe, not the 'bin' folder (which is the default)
   include(InstallRequiredSystemLibraries)
endif(WIN32)

## Linux ##

if(LINUX)
   # rpath handling
   set(CMAKE_INSTALL_RPATH "$ORIGIN/")

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
