set(SRC_DIR "${PROJECT_SOURCE_DIR}/Source")

# Files

set(CORE_SOURCE_FILES
   "${SRC_DIR}/Core/Archive.h"
   "${SRC_DIR}/Core/Assert.h"
   "${SRC_DIR}/Core/Enum.h"
   "${SRC_DIR}/Core/Log.h"
   "${SRC_DIR}/Core/Log.cpp"
   "${SRC_DIR}/Core/Math.h"
   "${SRC_DIR}/Core/RingBuffer.h"
)

set(EMULATOR_SOURCE_FILES
   "${SRC_DIR}/Emulator/Emulator.h"
   "${SRC_DIR}/Emulator/Emulator.cpp"
   "${SRC_DIR}/Emulator/Logo.inl"
   "${SRC_DIR}/Emulator/Main.cpp"
)

set(GAMEBOY_SOURCE_FILES
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

set(PLATFORM_AUDIO_SOURCE_FILES
   "${SRC_DIR}/Platform/Audio/AudioManager.h"
   "${SRC_DIR}/Platform/Audio/AudioManager.cpp"
)

set(PLATFORM_INPUT_SOURCE_FILES
   "${SRC_DIR}/Platform/Input/InputDevice.h"
   "${SRC_DIR}/Platform/Input/ControllerInputDevice.h"
   "${SRC_DIR}/Platform/Input/ControllerInputDevice.cpp"
   "${SRC_DIR}/Platform/Input/KeyboardInputDevice.h"
   "${SRC_DIR}/Platform/Input/KeyboardInputDevice.cpp"
)

set(PLATFORM_VIDEO_SOURCE_FILES
   "${SRC_DIR}/Platform/Video/Mesh.h"
   "${SRC_DIR}/Platform/Video/Mesh.cpp"
   "${SRC_DIR}/Platform/Video/Model.h"
   "${SRC_DIR}/Platform/Video/Model.cpp"
   "${SRC_DIR}/Platform/Video/Renderer.h"
   "${SRC_DIR}/Platform/Video/Renderer.cpp"
   "${SRC_DIR}/Platform/Video/Shader.h"
   "${SRC_DIR}/Platform/Video/Shader.cpp"
   "${SRC_DIR}/Platform/Video/ShaderProgram.h"
   "${SRC_DIR}/Platform/Video/ShaderProgram.cpp"
   "${SRC_DIR}/Platform/Video/Texture.h"
   "${SRC_DIR}/Platform/Video/Texture.cpp"
)

set(RETRO_SOURCE_FILES
   "${SRC_DIR}/Retro/Retro.cpp"
)

set(PLAYDATE_SOURCE_FILES
   "${SRC_DIR}/Playdate/Playdate.cpp"
)

set(TEST_SOURCE_FILES
   "${SRC_DIR}/Test/TestMain.cpp"
)

set(UI_SOURCE_FILES
   "${SRC_DIR}/UI/AfterCoreIncludes.inl"
   "${SRC_DIR}/UI/BeforeCoreIncludes.inl"
   "${SRC_DIR}/UI/CartridgeWindow.cpp"
   "${SRC_DIR}/UI/CPUWindow.cpp"
   "${SRC_DIR}/UI/EmulatorWindow.cpp"
   "${SRC_DIR}/UI/JoypadWindow.cpp"
   "${SRC_DIR}/UI/LCDControllerWindow.cpp"
   "${SRC_DIR}/UI/MemoryWindow.cpp"
   "${SRC_DIR}/UI/ScreenWindow.cpp"
   "${SRC_DIR}/UI/SoundControllerWindow.cpp"
   "${SRC_DIR}/UI/TimerWindow.cpp"
   "${SRC_DIR}/UI/UI.h"
   "${SRC_DIR}/UI/UI.cpp"
   "${SRC_DIR}/UI/UIConfig.h"
)

set(UI_DEBUGGER_SOURCE_FILES
   "${SRC_DIR}/UI/DebuggerWindow.cpp"
)

# File groups

set(ALL_SOURCE_FILES
   ${CORE_SOURCE_FILES}
   ${EMULATOR_SOURCE_FILES}
   ${GAMEBOY_SOURCE_FILES}
   ${PLATFORM_AUDIO_SOURCE_FILES}
   ${PLATFORM_INPUT_SOURCE_FILES}
   ${PLATFORM_VIDEO_SOURCE_FILES}
   ${RETRO_SOURCE_FILES}
   ${PLAYDATE_SOURCE_FILES}
   ${TEST_SOURCE_FILES}
   ${UI_SOURCE_FILES}
   ${UI_DEBUGGER_SOURCE_FILES}
)

set(EMULATOR_PROJECT_SOURCE_FILES
   ${CORE_SOURCE_FILES}
   ${EMULATOR_SOURCE_FILES}
   ${GAMEBOY_SOURCE_FILES}
   ${PLATFORM_INPUT_SOURCE_FILES}
   ${PLATFORM_VIDEO_SOURCE_FILES}
)

if(DOT_MATRIX_WITH_AUDIO)
   list(APPEND EMULATOR_PROJECT_SOURCE_FILES
      ${PLATFORM_AUDIO_SOURCE_FILES}
   )
endif()

if(DOT_MATRIX_WITH_UI)
   list(APPEND EMULATOR_PROJECT_SOURCE_FILES
      ${UI_SOURCE_FILES}
   )

   if (DOT_MATRIX_WITH_DEBUGGER)
      list(APPEND EMULATOR_PROJECT_SOURCE_FILES
         ${UI_DEBUGGER_SOURCE_FILES}
      )
   endif()
endif()

set(RETRO_PROJECT_SOURCE_FILES
   ${CORE_SOURCE_FILES}
   ${GAMEBOY_SOURCE_FILES}
   ${RETRO_SOURCE_FILES}
)

set(PLAYDATE_PROJECT_SOURCE_FILES
   ${CORE_SOURCE_FILES}
   ${GAMEBOY_SOURCE_FILES}
   ${PLAYDATE_SOURCE_FILES}
)

set(TEST_PROJECT_SOURCE_FILES
   ${CORE_SOURCE_FILES}
   ${GAMEBOY_SOURCE_FILES}
   ${TEST_SOURCE_FILES}
)

# Targets

target_include_directories(${COMMON_PROJECT_NAME} INTERFACE "${SRC_DIR}")

target_sources(${PROJECT_NAME} PRIVATE ${EMULATOR_PROJECT_SOURCE_FILES})
target_sources(${RETRO_PROJECT_NAME} PRIVATE ${RETRO_PROJECT_SOURCE_FILES})
target_sources(${PLAYDATE_PROJECT_NAME} PRIVATE ${PLAYDATE_PROJECT_SOURCE_FILES})
target_sources(${TEST_PROJECT_NAME} PRIVATE ${TEST_PROJECT_SOURCE_FILES})

source_group(TREE ${SRC_DIR} PREFIX Source FILES ${ALL_SOURCE_FILES})
