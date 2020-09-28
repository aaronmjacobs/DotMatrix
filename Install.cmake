if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/Install" CACHE PATH "..." FORCE)
endif()

# Platform detection
set(LINUX FALSE)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   set(LINUX TRUE)
endif()

## macOS ##

if(APPLE)
   set(APP_DIR "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app")
   set(CONTENTS_DIR "${APP_DIR}/Contents")
   set(MACOS_DIR "${CONTENTS_DIR}/MacOS")
   set(RESOURCES_DIR "${CONTENTS_DIR}/Resources")

   # Generate the .app file
   install(CODE "
      file(MAKE_DIRECTORY \"${MACOS_DIR}\")
      file(MAKE_DIRECTORY \"${RESOURCES_DIR}\")")

   # Copy the generated Info.plist file
   install(FILES "${BIN_RES_DIR}/Info.plist" DESTINATION "${CONTENTS_DIR}")

   # Copy the icon
   install(FILES "${RES_DIR}/macOS/${PROJECT_NAME}.icns" DESTINATION "${RESOURCES_DIR}")

   # Copy the executable (not install, so that the internal lib paths don't change)
   install(FILES "$<TARGET_FILE:${PROJECT_NAME}>" DESTINATION "${MACOS_DIR}"
           PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

   # Fix the bundle (copies and fixes shared libraries)
   install(CODE "
      include(BundleUtilities)
      fixup_bundle(\"${APP_DIR}\" \"\" \"\")")
endif()

## Windows ##

if(WIN32)
   # Install the executable
   install(TARGETS ${PROJECT_NAME} DESTINATION ".")

   # Install all shared libraries
   foreach(COPY_LIB ${COPY_LIBS})
      install(FILES "$<TARGET_FILE:${COPY_LIB}>" DESTINATION ".")
   endforeach()

   # Install system libraries
   set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".") # Copy into the same folder as the exe, not the 'bin' folder (which is the default)
   include(InstallRequiredSystemLibraries)
endif()

## Linux ##

if(LINUX)
   # rpath handling
   set(CMAKE_INSTALL_RPATH "$ORIGIN/")

   # Copy the executable (not install, so that the rpath doesn't change)
   install(FILES "$<TARGET_FILE:${PROJECT_NAME}>" DESTINATION "${CMAKE_INSTALL_PREFIX}"
           PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

   # Fix the bundle (in order to copy shared libraries)
   # Note: verify_app (called at the end of fixup_bundle) WILL fail, due to referencing external libraries
   # In order to ignore the error / continue, replace the verify_app function with one that does nothing
   install(CODE "
      include(BundleUtilities)
      function(verify_app app)
         message(STATUS \"(Not actually verifying...)\")
      endfunction()
      fixup_bundle(\"${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}\" \"\" \"\")")

   # Install the executable (this time, fixing the rpath)
   install(TARGETS ${PROJECT_NAME} DESTINATION ".")

   # Fix the bundle (just to do a proper verify)
   install(CODE "
      include(BundleUtilities)
      fixup_bundle(\"${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}\" \"\" \"\")")
endif()
