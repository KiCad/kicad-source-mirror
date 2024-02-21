include(GNUInstallDirs)

set(CLIPPER2_INC
  Clipper2Lib/include/clipper2/clipper.h
  Clipper2Lib/include/clipper2/clipper.core.h
  Clipper2Lib/include/clipper2/clipper.engine.h
  Clipper2Lib/include/clipper2/clipper.minkowski.h
  Clipper2Lib/include/clipper2/clipper.offset.h
  Clipper2Lib/include/clipper2/clipper.rectclip.h
)

set(CLIPPER2_SRC
  Clipper2Lib/src/clipper.engine.cpp
  Clipper2Lib/src/clipper.offset.cpp
  Clipper2Lib/src/clipper.rectclip.cpp
)


add_library(clipper2 STATIC ${CLIPPER2_INC} ${CLIPPER2_SRC})

target_include_directories(clipper2
  PUBLIC Clipper2Lib/include
)

target_compile_definitions(clipper2 PUBLIC USINGZ)

if (MSVC)
  target_compile_options(clipper2 PRIVATE /W4 /WX)
else()
  target_compile_options(clipper2 PRIVATE -Wall -Wextra -Wpedantic)
  target_link_libraries(clipper2 PUBLIC -lm)
endif()

