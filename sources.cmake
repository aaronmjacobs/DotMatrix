set(SOURCE_NAMES
   main.cpp

   gbc/Cartridge.cpp
   gbc/CPU.cpp
   gbc/Device.cpp
   gbc/LCDController.cpp
   gbc/Memory.cpp

   test/CPUTester.cpp

   wrapper/audio/AudioManager.cpp
   wrapper/input/KeyboardInputDevice.cpp

   wrapper/platform/IOUtils.cpp
   wrapper/platform/OSUtils.cpp

   wrapper/video/Mesh.cpp
   wrapper/video/Model.cpp
   wrapper/video/Renderer.cpp
   wrapper/video/Shader.cpp
   wrapper/video/ShaderProgram.cpp
   wrapper/video/Texture.cpp
)

set(HEADER_NAMES
   FancyAssert.h
   GLIncludes.h
   Log.h
   Pointers.h

   gbc/Bootstrap.h
   gbc/Cartridge.h
   gbc/CPU.h
   gbc/Debug.h
   gbc/Device.h
   gbc/LCDController.h
   gbc/Memory.h

   test/CPUTester.h

   wrapper/audio/AudioManager.h

   wrapper/input/InputDevice.h
   wrapper/input/KeyboardInputDevice.h

   wrapper/platform/IOUtils.h
   wrapper/platform/OSUtils.h

   wrapper/video/Mesh.h
   wrapper/video/Model.h
   wrapper/video/Renderer.h
   wrapper/video/Shader.h
   wrapper/video/ShaderProgram.h
   wrapper/video/Texture.h
)

source_group("Sources\\GBC" "${SRC_DIR}/gbc/*")
source_group("Sources\\Test" "${SRC_DIR}/test/*")
source_group("Sources\\Wrapper" "${SRC_DIR}/wrapper/*")
source_group("Sources\\Wrapper\\Audio" "${SRC_DIR}/wrapper/audio/*")
source_group("Sources\\Wrapper\\Input" "${SRC_DIR}/wrapper/input/*")
source_group("Sources\\Wrapper\\Platform" "${SRC_DIR}/wrapper/platform/*")
source_group("Sources\\Wrapper\\Video" "${SRC_DIR}/wrapper/video/*")