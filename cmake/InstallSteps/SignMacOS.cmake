
function( sign_kicad_bundle target signing_id use_secure_timestamp use_hardened_runtime entitlements_file use_sentry
          native_model_preview_enabled preview_entitlements thumbnail_entitlements )

    # If the signing ID wasn't passed in, use - which means adhoc signing
    if ( NOT signing_id )
        set( signing_id "-")
    endif()

    MESSAGE( STATUS "Signing ${target} with ${signing_id}, hardened runtime: ${use_hardened_runtime}, secure timestamp: ${use_secure_timestamp}, entitlements file: ${entitlements_file}, sentry: ${use_sentry}" )

    # --deep doesn't really work and is officially deprecated as of macos 13
    # https://developer.apple.com/library/archive/technotes/tn2206/_index.html#//apple_ref/doc/uid/DTS40007919-CH1-TNTAG201

    # collect a list of things to sign, in order
    set( sign_list "${target}/Contents/Applications/eeschema.app/Contents/MacOS/eeschema"
            "${target}/Contents/Applications/eeschema.app"
            "${target}/Contents/Applications/gerbview.app/Contents/MacOS/gerbview"
            "${target}/Contents/Applications/gerbview.app"  "${target}/Contents/Applications/pcbnew.app/Contents/MacOS/pcbnew" "${target}/Contents/Applications/pcbnew.app" "${target}/Contents/Applications/bitmap2component.app/Contents/MacOS/bitmap2component" "${target}/Contents/Applications/bitmap2component.app" "${target}/Contents/Applications/pcb_calculator.app/Contents/MacOS/pcb_calculator" "${target}/Contents/Applications/pcb_calculator.app" "${target}/Contents/Applications/pl_editor.app/Contents/MacOS/pl_editor" "${target}/Contents/Applications/pl_editor.app")

    # add all the dylibs from contents/frameworks
    file( GLOB framework_dylibs "${target}/Contents/Frameworks/*.dylib" )

    # Add ordinary plug-ins. App extensions are signed separately as bundles.
    file( GLOB_RECURSE plugins "${target}/Contents/PlugIns/*" )
    list( FILTER plugins EXCLUDE REGEX "\\.appex(/|$)" )

    file( GLOB_RECURSE translations "${target}/Contents/SharedSupport/internat/*.mo" )

    # add all the files in Contents/MacOS/
    # But we've gotta sign kicad-cli before signing kicad, at least on x86_64
    set( kicad_bins "${target}/Contents/MacOS/dxf2idf"
            "${target}/Contents/MacOS/idf2vrml"
            "${target}/Contents/MacOS/idfcyl"
            "${target}/Contents/MacOS/idfrect"
            "${target}/Contents/MacOS/kicad-cli"
            "${target}/Contents/MacOS/kicad")

    if( use_sentry )
        set( sign_list ${sign_list} "${target}/Contents/MacOS/crashpad_handler" )
    endif()

    set( sign_list ${sign_list} ${plugins} ${translations} ${kicad_bins} )

    # add kicad.app!
    set( sign_list ${sign_list} "${target}" )

    # build the command used for signing
    set( command codesign --force --sign "${signing_id}" )

    if( use_secure_timestamp )
        set( command ${command} --timestamp )
    endif( )

    if( use_hardened_runtime )
        if ( signing_id STREQUAL "-" )
            message( FATAL_ERROR "Hardened runtime requires a (non-ad-hoc) signing identity." )
        endif( )

        set( command ${command} --options runtime )
    endif( )

    file( GLOB nested_frameworks "${target}/Contents/Frameworks/*.framework" )

    foreach( item ${framework_dylibs} )
        execute_process( COMMAND ${command} "${item}" RESULT_VARIABLE codesign_result )

        if( NOT codesign_result EQUAL 0 )
            message( FATAL_ERROR "macOS bundled library signing failed for ${item}" )
        endif()
    endforeach()

    foreach( framework ${nested_frameworks} )
        get_filename_component( framework_name "${framework}" NAME_WE )
        file( GLOB framework_executables "${framework}/Versions/*/${framework_name}" )

        set( canonical_framework_executables )

        foreach( item ${framework_executables} )
            file( REAL_PATH "${item}" canonical_item )
            list( APPEND canonical_framework_executables "${canonical_item}" )
        endforeach()

        list( REMOVE_DUPLICATES canonical_framework_executables )

        foreach( item ${canonical_framework_executables} )
            execute_process( COMMAND ${command} "${item}" RESULT_VARIABLE codesign_result )

            if( NOT codesign_result EQUAL 0 )
                message( FATAL_ERROR "macOS nested framework signing failed for ${item}" )
            endif()
        endforeach()
    endforeach()

    foreach( item ${nested_frameworks} )
        execute_process( COMMAND ${command} "${item}" RESULT_VARIABLE codesign_result )

        if( NOT codesign_result EQUAL 0 )
            message( FATAL_ERROR "macOS nested framework signing failed for ${item}" )
        endif()
    endforeach()

    set( preview_extension "${target}/Contents/PlugIns/KiCadModelPreview.appex" )
    set( thumbnail_extension "${target}/Contents/PlugIns/KiCadModelThumbnail.appex" )

    foreach( extension "${preview_extension}" "${thumbnail_extension}" )
        if( "${extension}" STREQUAL "${preview_extension}" )
            set( extension_entitlements "${preview_entitlements}" )
        else()
            set( extension_entitlements "${thumbnail_entitlements}" )
        endif()

        if( native_model_preview_enabled AND NOT EXISTS "${extension}" )
            message( FATAL_ERROR "Required macOS app extension is missing: ${extension}" )
        endif()

        if( native_model_preview_enabled )
            if( NOT extension_entitlements OR NOT EXISTS "${extension_entitlements}" )
                message( FATAL_ERROR "Required macOS app extension entitlements are missing: ${extension_entitlements}" )
            endif()

            set( extension_command ${command} )
            list( APPEND extension_command --entitlements "${extension_entitlements}" )

            execute_process( COMMAND ${extension_command} "${extension}" RESULT_VARIABLE codesign_result )

            if( NOT codesign_result EQUAL 0 )
                message( FATAL_ERROR "macOS app extension signing failed for ${extension}" )
            endif()
        endif()
    endforeach()

    if( entitlements_file )
        set( command ${command} --entitlements "${entitlements_file}" )
    endif( )

    foreach( item ${sign_list} )
        set( cmd ${command} "${item}" )

        # MESSAGE( STATUS "Running ${cmd}")
        execute_process( COMMAND ${cmd}
                RESULT_VARIABLE codesign_result)

        if( NOT codesign_result EQUAL 0 )
            message( FATAL_ERROR "macOS signing failed; ${cmd} returned ${codesign_result}" )
        endif( )
    endforeach( )
endfunction()


function( verify_signing target )
    set( cmd codesign --verify --deep --strict --verbose=3 "${target}" )

    execute_process( COMMAND ${cmd} RESULT_VARIABLE verify_result )
    if( NOT verify_result EQUAL 0 )
        message( FATAL_ERROR "macOS signing verification failed; ran ${cmd}" )
    endif( )
endfunction( )
