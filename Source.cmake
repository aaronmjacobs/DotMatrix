set(SRC_ROOT_DIR "${PROJECT_SOURCE_DIR}/Source")
set(SRC_DIR "${SRC_ROOT_DIR}/${PROJECT_NAME}")

target_include_directories(${PROJECT_NAME} PUBLIC "${SRC_ROOT_DIR}")

target_sources(${PROJECT_NAME} PRIVATE
   "${SRC_DIR}/Core/Archive.h"
   "${SRC_DIR}/Core/Assert.h"
   "${SRC_DIR}/Core/Enum.h"
   "${SRC_DIR}/Core/Log.h"
   "${SRC_DIR}/Core/Log.cpp"
   "${SRC_DIR}/Core/Math.h"

   "${SRC_DIR}/GameBoy/Cartridge.h"
   "${SRC_DIR}/GameBoy/Cartridge.cpp"
   "${SRC_DIR}/GameBoy/CPU.h"
   "${SRC_DIR}/GameBoy/CPU.cpp"
   "${SRC_DIR}/GameBoy/GameBoy.h"
   "${SRC_DIR}/GameBoy/GameBoy.cpp"
   "${SRC_DIR}/GameBoy/LCDController.h"
   "${SRC_DIR}/GameBoy/LCDController.cpp"
   "${SRC_DIR}/GameBoy/MemoryBankController.h"
   "${SRC_DIR}/GameBoy/MemoryBankController.cpp"
   "${SRC_DIR}/GameBoy/Operations.h"
   "${SRC_DIR}/GameBoy/Operations.cpp"
   "${SRC_DIR}/GameBoy/SoundController.h"
   "${SRC_DIR}/GameBoy/SoundController.cpp"
)

get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
source_group(TREE ${SRC_DIR} PREFIX Source FILES ${SOURCE_FILES})
