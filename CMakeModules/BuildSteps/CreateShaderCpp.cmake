file( READ ${SOURCE} SOURCE_TEXT )

set( outCppText
"
#include <${OUTHEADERFILE}>

namespace KIGFX {
namespace BUILTIN_SHADERS {
const char ${OUTVARNAME}[] = R\"SHADER_SOURCE(
${SOURCE_TEXT}
)SHADER_SOURCE\";
}
}
" )

file(
    WRITE ${DESTINATION_SOURCE_DIR}/${OUTCPPFILE}
    "${outCppText}"
)


set( outHeaderText
"namespace KIGFX {
    namespace BUILTIN_SHADERS {
        extern const char ${OUTVARNAME}[];
    }
}"
)

file(
    WRITE ${DESTINATION_HEADER_DIR}/${OUTHEADERFILE}
    "${outHeaderText}"
)

message(STATUS "Shader ${SOURCE} converted to ${DESTINATION_SOURCE_DIR}/${OUTCPPFILE}")