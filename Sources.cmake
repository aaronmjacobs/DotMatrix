set(SOURCE_NAMES
   Emulator.cpp
   Main.cpp

   Core/Log.cpp

   GBC/Cartridge.cpp
   GBC/CPU.cpp
   GBC/GameBoy.cpp
   GBC/LCDController.cpp
   GBC/Memory.cpp
   GBC/SoundController.cpp

   Platform/Audio/AudioManager.cpp

   Platform/Input/ControllerInputDevice.cpp
   Platform/Input/KeyboardInputDevice.cpp

   Platform/Utils/IOUtils.cpp
   Platform/Utils/OSUtils.cpp

   Platform/Video/Mesh.cpp
   Platform/Video/Model.cpp
   Platform/Video/Renderer.cpp
   Platform/Video/Shader.cpp
   Platform/Video/ShaderProgram.cpp
   Platform/Video/Texture.cpp

   Test/CPUTester.cpp
   Test/TestMain.cpp
)

set(HEADER_NAMES
   Emulator.h

   Core/Archive.h
   Core/Assert.h
   Core/Log.h
   Core/Pointers.h

   GBC/Bootstrap.h
   GBC/Cartridge.h
   GBC/CPU.h
   GBC/GameBoy.h
   GBC/LCDController.h
   GBC/Memory.h
   GBC/Operations.h
   GBC/SoundController.h

   Platform/Audio/AudioManager.h

   Platform/Input/InputDevice.h
   Platform/Input/ControllerInputDevice.h
   Platform/Input/KeyboardInputDevice.h

   Platform/Utils/IOUtils.h
   Platform/Utils/OSUtils.h

   Platform/Video/Mesh.h
   Platform/Video/Model.h
   Platform/Video/Renderer.h
   Platform/Video/Shader.h
   Platform/Video/ShaderProgram.h
   Platform/Video/Texture.h

   Test/CPUTester.h
)
