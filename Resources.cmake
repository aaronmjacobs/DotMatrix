if(DOT_MATRIX_ARMGCC)
   configure_file(
      "${RES_DIR}/Playdate/pdxinfo"
      "${PROJECT_BINARY_DIR}/Source/pdxinfo"
   )
   configure_file(
      "${RES_DIR}/Playdate/card.png"
      "${PROJECT_BINARY_DIR}/Source/card.png"
      COPYONLY
   )
   configure_file(
      "${RES_DIR}/Playdate/card-pressed.png"
      "${PROJECT_BINARY_DIR}/Source/card-pressed.png"
      COPYONLY
   )
elseif(APPLE)
   configure_file(
     "${RES_DIR}/macOS/Info.plist.in"
     "${BIN_RES_DIR}/Info.plist"
   )

   target_sources(${PROJECT_NAME} PRIVATE
      "${BIN_RES_DIR}/Info.plist"
      "${RES_DIR}/macOS/Info.plist.in"
      "${RES_DIR}/macOS/${PROJECT_NAME}.icns"
   )
elseif(WIN32)
   target_sources(${PROJECT_NAME} PRIVATE
      "${RES_DIR}/Windows/${PROJECT_NAME}.rc"
      "${RES_DIR}/Windows/${PROJECT_NAME}.ico"
   )
endif()

source_group("Resources" "${RES_DIR}/*")
source_group("Resources\\Generated" "${BIN_RES_DIR}/*")
