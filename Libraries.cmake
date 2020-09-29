set(LIB_DIR "${PROJECT_SOURCE_DIR}/Libraries")
set(COPY_LIBS)

## Integrated ##

# imgui
if(DOT_MATRIX_WITH_UI)
   set(IMGUI_DIR "${LIB_DIR}/imgui")
   set(IMGUI_SOURCES
      "${IMGUI_DIR}/imconfig.h"
      "${IMGUI_DIR}/imgui.cpp"
      "${IMGUI_DIR}/imgui.h"
      "${IMGUI_DIR}/imgui_demo.cpp"
      "${IMGUI_DIR}/imgui_draw.cpp"
      "${IMGUI_DIR}/imgui_internal.h"
      "${IMGUI_DIR}/imgui_widgets.cpp"
      "${IMGUI_DIR}/imstb_rectpack.h"
      "${IMGUI_DIR}/imstb_textedit.h"
      "${IMGUI_DIR}/imstb_truetype.h"
      "${IMGUI_DIR}/examples/imgui_impl_glfw.h"
      "${IMGUI_DIR}/examples/imgui_impl_glfw.cpp"
      "${IMGUI_DIR}/examples/imgui_impl_opengl3.h"
      "${IMGUI_DIR}/examples/imgui_impl_opengl3.cpp"
   )
   target_sources(${PROJECT_NAME} PRIVATE "${IMGUI_SOURCES}")
   target_include_directories(${PROJECT_NAME} PRIVATE "${IMGUI_DIR}")
   target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD IMGUI_USER_CONFIG="UI/UIConfig.h")

   set(IMGUI_CLUB_DIR "${LIB_DIR}/imgui_club")
   target_sources(${PROJECT_NAME} PRIVATE "${IMGUI_CLUB_DIR}/imgui_memory_editor/imgui_memory_editor.h")
   target_include_directories(${PROJECT_NAME} PRIVATE "${IMGUI_CLUB_DIR}")

   source_group("Libraries\\imgui_club\\imgui_memory_editor" "${IMGUI_CLUB_DIR}/imgui_memory_editor/*") # This needs to come first
   source_group("Libraries\\imgui" "${IMGUI_DIR}/*")
endif()

# libretro
set(LIBRETRO_DIR "${LIB_DIR}/libretro")
target_sources(${RETRO_PROJECT_NAME} PRIVATE "${LIBRETRO_DIR}/libretro.h")
target_include_directories(${RETRO_PROJECT_NAME} PRIVATE "${LIBRETRO_DIR}")
source_group("Libraries\\libretro" "${LIBRETRO_DIR}")

# PPK_ASSERT
set(PPK_DIR "${LIB_DIR}/PPK_ASSERT")
target_sources(${COMMON_PROJECT_NAME} INTERFACE "${PPK_DIR}/src/ppk_assert.h;${PPK_DIR}/src/ppk_assert.cpp")
target_include_directories(${COMMON_PROJECT_NAME} INTERFACE "${PPK_DIR}/src")
source_group("Libraries\\PPK_ASSERT" "${PPK_DIR}/src")

# readerwriterqueue
set(QUEUE_DIR "${LIB_DIR}/readerwriterqueue")
target_sources(${EXECUTABLE_COMMON_PROJECT_NAME} INTERFACE "${QUEUE_DIR}/atomicops.h;${QUEUE_DIR}/readerwriterqueue.h")
target_include_directories(${EXECUTABLE_COMMON_PROJECT_NAME} INTERFACE "${QUEUE_DIR}")
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
target_sources(${COMMON_PROJECT_NAME} INTERFACE "${TEMPLOG_SOURCES}")
target_include_directories(${COMMON_PROJECT_NAME} INTERFACE "${TEMPLOG_DIR}")
source_group("Libraries\\templog" "${TEMPLOG_DIR}/*")

## Static ##

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build shared libraries")

# Boxer
add_subdirectory("${LIB_DIR}/Boxer")
target_link_libraries(${PROJECT_NAME} PRIVATE Boxer)

# glad
add_subdirectory("${LIB_DIR}/glad")
target_link_libraries(${PROJECT_NAME} PRIVATE optimized glad)
target_link_libraries(${PROJECT_NAME} PRIVATE debug glad-debug)
list(APPEND COPY_LIBS "${CMAKE_DL_LIBS}")

# PlatformUtils
add_subdirectory("${LIB_DIR}/PlatformUtils")
target_link_libraries(${EXECUTABLE_COMMON_PROJECT_NAME} INTERFACE PlatformUtils)

## Shared ##

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build shared libraries")

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory("${LIB_DIR}/glfw")
target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
list(APPEND COPY_LIBS glfw)

# OpenAL
if(DOT_MATRIX_WITH_AUDIO)
   set(OPENAL_DIR "${LIB_DIR}/openal-soft")
   set(ALSOFT_UTILS OFF CACHE INTERNAL "Build and install utility programs")
   set(ALSOFT_NO_CONFIG_UTIL ON CACHE INTERNAL "Disable building the alsoft-config utility")
   set(ALSOFT_EXAMPLES OFF CACHE INTERNAL "Build and install example programs")
   set(ALSOFT_INSTALL OFF CACHE INTERNAL "Install headers and libraries")
   set(ALSOFT_INSTALL_CONFIG OFF CACHE INTERNAL "Install alsoft.conf sample configuration file")
   set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE INTERNAL "Install HRTF data files")
   set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE INTERNAL "Install AmbDec preset files")
   set(ALSOFT_INSTALL_EXAMPLES OFF CACHE INTERNAL "Install example programs (alplay, alstream, ...)")
   set(ALSOFT_INSTALL_UTILS OFF CACHE INTERNAL "Install utility programs (openal-info, alsoft-config, ...)")
   set(ALSOFT_UPDATE_BUILD_VERSION OFF CACHE INTERNAL "Update git build version info")
   add_subdirectory("${LIB_DIR}/openal-soft")
   target_link_libraries(${PROJECT_NAME} PRIVATE OpenAL)
   list(APPEND COPY_LIBS OpenAL)
endif()

## Post-Build ##

# Copy DLLs
if(WIN32)
   foreach(COPY_LIB ${COPY_LIBS})
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${COPY_LIB}>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
      )
      add_custom_command(TARGET ${TEST_PROJECT_NAME} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${COPY_LIB}>" "$<TARGET_FILE_DIR:${TEST_PROJECT_NAME}>"
      )
   endforeach()
endif()
