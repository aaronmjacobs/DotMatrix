set(SRC_DIR "${PROJECT_SOURCE_DIR}/Source")
set(CORE_SRC_DIR "${SRC_DIR}/${PROJECT_NAME}")

target_include_directories(${PROJECT_NAME} PUBLIC "${SRC_DIR}")

target_sources(${PROJECT_NAME} PRIVATE
   "${CORE_SRC_DIR}/Core/Archive.h"
   "${CORE_SRC_DIR}/Core/Assert.h"
   "${CORE_SRC_DIR}/Core/Enum.h"
   "${CORE_SRC_DIR}/Core/Log.h"
   "${CORE_SRC_DIR}/Core/Log.cpp"
   "${CORE_SRC_DIR}/Core/Math.h"

   "${CORE_SRC_DIR}/GameBoy/Cartridge.h"
   "${CORE_SRC_DIR}/GameBoy/Cartridge.cpp"
   "${CORE_SRC_DIR}/GameBoy/CPU.h"
   "${CORE_SRC_DIR}/GameBoy/CPU.cpp"
   "${CORE_SRC_DIR}/GameBoy/GameBoy.h"
   "${CORE_SRC_DIR}/GameBoy/GameBoy.cpp"
   "${CORE_SRC_DIR}/GameBoy/LCDController.h"
   "${CORE_SRC_DIR}/GameBoy/LCDController.cpp"
   "${CORE_SRC_DIR}/GameBoy/MemoryBankController.h"
   "${CORE_SRC_DIR}/GameBoy/MemoryBankController.cpp"
   "${CORE_SRC_DIR}/GameBoy/Operations.h"
   "${CORE_SRC_DIR}/GameBoy/Operations.cpp"
   "${CORE_SRC_DIR}/GameBoy/SoundController.h"
   "${CORE_SRC_DIR}/GameBoy/SoundController.cpp"
)

if(${DOT_MATRIX_WITH_LIBRETRO})
   target_sources(${PROJECT_NAME} PRIVATE
      "${SRC_DIR}/Retro/Retro.cpp"
   )
endif()

get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
source_group(TREE ${SRC_DIR} PREFIX Source FILES ${SOURCE_FILES})
