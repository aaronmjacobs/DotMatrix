set(SOURCE_NAMES
   Emulator.cpp
   Main.cpp

   GBC/Cartridge.cpp
   GBC/CPU.cpp
   GBC/Device.cpp
   GBC/LCDController.cpp
   GBC/Memory.cpp
   GBC/SoundController.cpp

   Test/CPUTester.cpp
   Test/TestMain.cpp

   Wrapper/Audio/AudioManager.cpp

   Wrapper/Input/ControllerInputDevice.cpp
   Wrapper/Input/KeyboardInputDevice.cpp

   Wrapper/Platform/IOUtils.cpp
   Wrapper/Platform/OSUtils.cpp

   Wrapper/Video/Mesh.cpp
   Wrapper/Video/Model.cpp
   Wrapper/Video/Renderer.cpp
   Wrapper/Video/Shader.cpp
   Wrapper/Video/ShaderProgram.cpp
   Wrapper/Video/Texture.cpp
)

set(HEADER_NAMES
   Emulator.h
   FancyAssert.h
   GLIncludes.h
   Log.h
   Pointers.h

   GBC/Bootstrap.h
   GBC/Cartridge.h
   GBC/CPU.h
   GBC/Debug.h
   GBC/Device.h
   GBC/LCDController.h
   GBC/Memory.h
   GBC/Operations.h
   GBC/SoundController.h

   Test/CPUTester.h

   Wrapper/Audio/AudioManager.h

   Wrapper/Input/InputDevice.h
   Wrapper/Input/ControllerInputDevice.h
   Wrapper/Input/KeyboardInputDevice.h

   Wrapper/Platform/IOUtils.h
   Wrapper/Platform/OSUtils.h

   Wrapper/Video/Mesh.h
   Wrapper/Video/Model.h
   Wrapper/Video/Renderer.h
   Wrapper/Video/Shader.h
   Wrapper/Video/ShaderProgram.h
   Wrapper/Video/Texture.h
)
