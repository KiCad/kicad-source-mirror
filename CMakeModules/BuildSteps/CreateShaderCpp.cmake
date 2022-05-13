file( READ ${SOURCE} SOURCE_TEXT )
file( SIZE ${SOURCE} SOURCE_FILESIZE )

set( MAX_BYTES_PER_LITERAL 16380 )

math(EXPR NUMBER_LITERALS "${SOURCE_FILESIZE}/${MAX_BYTES_PER_LITERAL}")


set( outCppTextStdString "std::string ${OUTVARNAME} = ")

set( outCppText
"
#include <string>
#include <${OUTHEADERFILE}>

namespace KIGFX {
namespace BUILTIN_SHADERS {
")


MATH(EXPR LAST_LITERAL_ITER "${LITERAL_ITER}-1")

foreach(LITERAL_ITER RANGE ${NUMBER_LITERALS})
    set( outCppText
    "
${outCppText}

const char ${OUTVARNAME}_p${LITERAL_ITER}[] = R\"SHADER_SOURCE(
${SOURCE_TEXT}
)SHADER_SOURCE\";

    " )

    set( outCppTextStdString "${outCppTextStdString} std::string(${OUTVARNAME}_p${LITERAL_ITER})")
    if( ${LITERAL_ITER} LESS  ${LAST_LITERAL_ITER})
        set( outCppTextStdString " ${outCppTextStdString} +")
    endif()
endforeach()

set( outCppTextStdString "${outCppTextStdString};")

set( outCppText
"
${outCppText}

${outCppTextStdString}
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
        extern std::string ${OUTVARNAME};
    }
}"
)

file(
    WRITE ${DESTINATION_HEADER_DIR}/${OUTHEADERFILE}
    "${outHeaderText}"
)

message(STATUS "Shader ${SOURCE} converted to ${DESTINATION_SOURCE_DIR}/${OUTCPPFILE}")