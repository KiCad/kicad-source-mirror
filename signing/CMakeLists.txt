if (APPLE AND KICAD_OSX_CODESIGN )
    install( CODE "
        include( ${KICAD_CMAKE_MODULE_PATH}/SignMacOS.cmake )

        message( STATUS \"Signing bundles...\" )
        sign_kicad_bundle( \"${OSX_BUNDLE_INSTALL_DIR}\" \"\${KICAD_OSX_SIGNING_ID}\" \"\${KICAD_OSX_SIGNING_USE_SECURE_TIMESTAMP}\" \"\${KICAD_OSX_SIGNING_USE_HARDENED_RUNTIME}\" \"\${KICAD_OSX_SIGNING_ENTITLEMENTS_FILE}\" )
    " COMPONENT Runtime )
endif()
