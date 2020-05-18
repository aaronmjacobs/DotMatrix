set(LIB_DIR "${PROJECT_SOURCE_DIR}/Libraries")

# libretro
if(${DOT_MATRIX_WITH_LIBRETRO})
	set(LIBRETRO_DIR "${LIB_DIR}/libretro")
	target_sources(${PROJECT_NAME} PRIVATE "${LIBRETRO_DIR}/libretro.h")
	target_include_directories(${PROJECT_NAME} PUBLIC "${LIBRETRO_DIR}")
	source_group("Libraries\\libretro" "${LIBRETRO_DIR}")
endif()

# PPK_ASSERT
set(PPK_DIR "${LIB_DIR}/PPK_ASSERT")
target_sources(${PROJECT_NAME} PRIVATE "${PPK_DIR}/src/ppk_assert.h;${PPK_DIR}/src/ppk_assert.cpp")
target_include_directories(${PROJECT_NAME} PUBLIC "${PPK_DIR}/src")
source_group("Libraries\\PPK_ASSERT" "${PPK_DIR}/src")

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
