# For convenience we define the sources as a variable. You can add 
# header files and cpp / c files and CMake will sort them out
SET(SRCS    kicad
            treeprj_frame
            treeprj_datas
            buildmnu
            ../share/infospgm
            mainframe
            prjconfig
            preferences
            commandframe
            files-io
   )
 
# If we build for windows systems, we also include the resource file
# containing the manifest, icon and other resources
IF(WIN32)
  SET(SRCS ${SRCS} kicad.rc)
ENDIF(WIN32)
 
# Here we define the executable kicad.exe or kicad on other systems
# the above paths and defines will be used in this build
ADD_EXECUTABLE(kicad WIN32 ${SRCS})
    
# We add to our target 'kicad' the wxWidgets libraries. These are
# set for us by the find script. If you need other libraries, you
# can add them here as well. 
TARGET_LINK_LIBRARIES(kicad ${wxWidgets_LIBRARIES})

LINK_DIRECTORIES(${KICAD_EDA_BINARY_DIR}/common
)
