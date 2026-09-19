if (APPLE AND KICAD_OSX_CODESIGN )
    install( CODE "
        include( ${KICAD_CMAKE_MODULE_PATH}/InstallSteps/SignMacOS.cmake )

        message( STATUS \"Signing bundles...\" )
        sign_kicad_bundle( \"${OSX_BUNDLE_INSTALL_DIR}\" \"${KICAD_OSX_SIGNING_ID}\" \"${KICAD_OSX_SIGNING_USE_SECURE_TIMESTAMP}\" \"${KICAD_OSX_SIGNING_USE_HARDENED_RUNTIME}\" \"${KICAD_OSX_SIGNING_ENTITLEMENTS_FILE}\" \"${KICAD_USE_SENTRY}\" \"${KICAD_BUILD_QUICKLOOK}\" \"${PROJECT_SOURCE_DIR}/utils/quicklook/Preview/Preview.entitlements\" \"${PROJECT_SOURCE_DIR}/utils/quicklook/Thumbnail/Thumbnail.entitlements\" )
    " COMPONENT Runtime )
endif()
