# Generate a static library target named "bitmaps"
# (with filename libbitmaps.a on Linux)

# Copy the *.xpm files to ${XPM_CPP_PATH}/*.cpp, on change only.
# Compile those *.cpp files and put them into the library, then done.

# builds a set of all the .xpm files
FILE(GLOB BITMAP_SRCS *.xpm)

# Get the path of the *.xpm files into "PATH"
set(PATH ${CMAKE_CURRENT_SOURCE_DIR})
#message("PATH = ${PATH}")

# The name of the directory to put the copied and renamed *.xpm files into.
# As files are copied they are renamed to *.cpp.
set(XPM_CPP_PATH "${PATH}/auto_renamed_to_cpp" CACHE PATH "path to store renamed .xpm files for compilation" )

foreach(LOOP_VAR ${BITMAP_SRCS})
    get_filename_component(BASENAME ${LOOP_VAR} NAME_WE)
    set(CPP_BITMAP "${XPM_CPP_PATH}/${BASENAME}.cpp")
    add_custom_command(
        OUTPUT ${CPP_BITMAP}
        COMMAND cmake -E copy "${PATH}/${BASENAME}.xpm" "${CPP_BITMAP}"
        DEPENDS ${BASENAME}.xpm)
    list(APPEND CPP_BITMAPS ${CPP_BITMAP})
    set_source_files_properties(${CPP_BITMAP} PROPERTIES COMPILE_FLAGS -DXPMMAIN)
endforeach(LOOP_VAR)

add_library(bitmaps ${CPP_BITMAPS})
