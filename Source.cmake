set(SRC_DIR "${PROJECT_SOURCE_DIR}/Source")
set(GENERATED_DIR "${PROJECT_BINARY_DIR}/Generated")

target_include_directories(${PROJECT_NAME} PUBLIC "${SRC_DIR}" "${GENERATED_DIR}")

target_sources(${PROJECT_NAME} PRIVATE
   "${SRC_DIR}/Core/Archive.h"
   "${SRC_DIR}/Core/Assert.h"
   "${SRC_DIR}/Core/Constants.h.in"
   "${SRC_DIR}/Core/Enum.h"
   "${SRC_DIR}/Core/Log.h"
   "${SRC_DIR}/Core/Log.cpp"
   "${SRC_DIR}/Core/Math.h"
   "${SRC_DIR}/Core/Pointers.h"

   "${SRC_DIR}/GBC/Cartridge.h"
   "${SRC_DIR}/GBC/Cartridge.cpp"
   "${SRC_DIR}/GBC/CPU.h"
   "${SRC_DIR}/GBC/CPU.cpp"
   "${SRC_DIR}/GBC/GameBoy.h"
   "${SRC_DIR}/GBC/GameBoy.cpp"
   "${SRC_DIR}/GBC/LCDController.h"
   "${SRC_DIR}/GBC/LCDController.cpp"
   "${SRC_DIR}/GBC/MemoryBankController.h"
   "${SRC_DIR}/GBC/MemoryBankController.cpp"
   "${SRC_DIR}/GBC/Operations.h"
   "${SRC_DIR}/GBC/Operations.cpp"
   "${SRC_DIR}/GBC/SoundController.h"
   "${SRC_DIR}/GBC/SoundController.cpp"

   "${SRC_DIR}/Platform/Input/InputDevice.h"
   "${SRC_DIR}/Platform/Input/ControllerInputDevice.h"
   "${SRC_DIR}/Platform/Input/ControllerInputDevice.cpp"
   "${SRC_DIR}/Platform/Input/KeyboardInputDevice.h"
   "${SRC_DIR}/Platform/Input/KeyboardInputDevice.cpp"

   "${SRC_DIR}/Platform/Utils/IOUtils.h"
   "${SRC_DIR}/Platform/Utils/IOUtils.cpp"
   "${SRC_DIR}/Platform/Utils/OSUtils.h"
   "${SRC_DIR}/Platform/Utils/OSUtils.cpp"

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

   "${SRC_DIR}/UI/UIFriend.h"
)

if(GBC_WITH_AUDIO)
   target_sources(${PROJECT_NAME} PRIVATE
      "${SRC_DIR}/Platform/Audio/AudioManager.h"
      "${SRC_DIR}/Platform/Audio/AudioManager.cpp"
   )
endif()

if(GBC_WITH_UI)
   target_sources(${PROJECT_NAME} PRIVATE
      "${SRC_DIR}/UI/CartridgeWindow.cpp"
      "${SRC_DIR}/UI/CPUWindow.cpp"
      "${SRC_DIR}/UI/EmulatorWindow.cpp"
      "${SRC_DIR}/UI/JoypadWindow.cpp"
      "${SRC_DIR}/UI/MemoryWindow.cpp"
      "${SRC_DIR}/UI/ScreenWindow.cpp"
      "${SRC_DIR}/UI/SoundControllerWindow.cpp"
      "${SRC_DIR}/UI/TimerWindow.cpp"
      "${SRC_DIR}/UI/UI.h"
      "${SRC_DIR}/UI/UI.cpp"
      "${SRC_DIR}/UI/UIConfig.h"
   )

   if (GBC_WITH_DEBUGGER)
      target_sources(${PROJECT_NAME} PRIVATE
         "${SRC_DIR}/UI/DebuggerWindow.cpp"
      )
   endif()
endif()

if(GBC_RUN_TESTS)
   target_sources(${PROJECT_NAME} PRIVATE
      "${SRC_DIR}/Test/TestMain.cpp"
   )
else()
   target_sources(${PROJECT_NAME} PRIVATE
      "${SRC_DIR}/Emulator.h"
      "${SRC_DIR}/Emulator.cpp"
      "${SRC_DIR}/Logo.inl"
      "${SRC_DIR}/Main.cpp"
   )
endif()

if(APPLE)
   target_sources(${PROJECT_NAME} PRIVATE "${SRC_DIR}/Platform/Utils/MacOSUtils.mm")
endif()

get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
source_group(TREE ${SRC_DIR} PREFIX Source FILES ${SOURCE_FILES})

set(GENERATED_FILE_NAMES "Core/Constants.h")

set(GENERATED_FILES)
foreach(GENERATED_FILE_NAME ${GENERATED_FILE_NAMES})
   set(GENERATED_FILE "${GENERATED_DIR}/${GENERATED_FILE_NAME}")
   configure_file("${SRC_DIR}/${GENERATED_FILE_NAME}.in" "${GENERATED_FILE}")
   list(APPEND GENERATED_FILES "${GENERATED_FILE}")
endforeach()

target_sources(${PROJECT_NAME} PRIVATE ${GENERATED_FILES})
source_group(TREE ${GENERATED_DIR} PREFIX Generated FILES ${GENERATED_FILES})
