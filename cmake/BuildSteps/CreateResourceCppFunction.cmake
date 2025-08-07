#
# Used to mark and generate resource files to conversion to binary arrays
#
# Usage example:
# add_compiled_resource( pcbnew ${CMAKE_SOURCE_DIR}/resources/html/about.html about_html)
#
# The source file about.html above will be converted to about_html.cpp/.h and placed into
# the /resources/cpp cmake binary directory folder. The include directories for the target are also
# expanded to cover including about_html.h
#
# The output is marked as depends so changes to about.html will trigger a update of the about_html.cpp/.h
#
# The CreateResourceCpp.cmake helper script is also marked a depends so any changes to our output writing
# will cause updates.
#


function( add_compiled_resource outTarget inFile resourceName )
    set(outCppName "${resourceName}.cpp")
    set(outHeaderName "${resourceName}.h")

    add_custom_command(
        OUTPUT  ${CMAKE_BINARY_DIR}/resources/cpp/${outCppName}
                ${CMAKE_BINARY_DIR}/resources/cpp/${outHeaderName}
        COMMAND ${CMAKE_COMMAND}
            -DSOURCE_FILE="${inFile}"
            -DOUT_CPP_DIR="${CMAKE_BINARY_DIR}/resources/cpp"
            -DOUT_HEADER_DIR="${CMAKE_BINARY_DIR}/resources/cpp"
            -DOUT_CPP_FILENAME="${outCppName}"
            -DOUT_HEADER_FILENAME="${outHeaderName}"
            -DOUT_VAR_NAME="${resourceName}"
            -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/CreateResourceCpp.cmake
        DEPENDS ${inFile}
                ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/CreateResourceCpp.cmake
        )

    target_sources( ${outTarget} PRIVATE ${CMAKE_BINARY_DIR}/resources/cpp/${outCppName} )
    target_include_directories( ${outTarget} PUBLIC ${CMAKE_BINARY_DIR}/resources/cpp )
endfunction()