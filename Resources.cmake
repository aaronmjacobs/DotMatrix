if(APPLE)
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
