set(LIB_DIR "${PROJECT_SOURCE_DIR}/Libraries")
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
set(ALSOFT_TESTS OFF CACHE INTERNAL "Build and install test programs")
set(ALSOFT_CONFIG OFF CACHE INTERNAL "Install alsoft.conf sample configuration file")
set(ALSOFT_HRTF_DEFS OFF CACHE INTERNAL "Install HRTF definition files")
set(ALSOFT_AMBDEC_PRESETS OFF CACHE INTERNAL "Install AmbDec preset files")
set(ALSOFT_INSTALL OFF CACHE INTERNAL "Install headers and libraries")
add_subdirectory("${LIB_DIR}/openal-soft")
target_link_libraries(${PROJECT_NAME} PUBLIC OpenAL)
# openal-soft doesn't populate INTERFACE_INCLUDE_DIRECTORIES, so we need to manually add the includes here
target_include_directories(${PROJECT_NAME} PUBLIC $<TARGET_PROPERTY:OpenAL,INCLUDE_DIRECTORIES>)
list(APPEND COPY_LIBS OpenAL)

## Post-Build ##

# Copy DLLs
if(WIN32)
   foreach(COPY_LIB ${COPY_LIBS})
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${COPY_LIB}>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
      )
   endforeach()
endif()
