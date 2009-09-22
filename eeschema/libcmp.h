/*****************************************************************/
/*  Headers for library definition and lib component definitions */
/*****************************************************************/

#ifndef LIBCMP_H
#define LIBCMP_H


#define LIB_VERSION_MAJOR 2
#define LIB_VERSION_MINOR 3

/* Must be the first line of component library (.lib) files. */
#define LIBFILE_IDENT     "EESchema-LIBRARY Version"

#define LIB_VERSION( major, minor ) ( major * 100 + minor )

#define IS_LIB_CURRENT_VERSION( major, minor )              \
    (                                                       \
        LIB_VERSION( major1, minor1 ) ==                    \
        LIB_VERSION( LIB_VERSION_MAJOR, LIB_VERSION_MINOR)  \
    )

/*
 * Library versions 2.3 and lower use the old separate library (.lib) and
 * document (.dcm) files.  Component libraries after 2.3 merged the library
 * and document files into a single library file.  This macro checks if the
 * library version supports the old format
 */
#define USE_OLD_DOC_FILE_FORMAT( major, minor )                 \
    ( LIB_VERSION( major, minor ) < LIB_VERSION( 2, 3 )

/* Must be the first line of component library document (.dcm) files. */
#define DOCFILE_IDENT     "EESchema-DOCLIB  Version 2.0"

#define DOC_EXT           wxT( "dcm" )


enum LocateDrawStructType
{
    LOCATE_COMPONENT_ARC_DRAW_TYPE          = 1,
    LOCATE_COMPONENT_CIRCLE_DRAW_TYPE       = 2,
    LOCATE_COMPONENT_GRAPHIC_TEXT_DRAW_TYPE = 4,
    LOCATE_COMPONENT_RECT_DRAW_TYPE         = 8,
    LOCATE_LINE_DRAW_TYPE                   = 0x10,
    LOCATE_COMPONENT_POLYLINE_DRAW_TYPE     = 0x20,
    LOCATE_COMPONENT_LINE_DRAW_TYPE         = 0x40
};

#define LOCATE_ALL_DRAW_ITEM 0xFFFFFFFF


#include "class_library.h"


/* Variables used by LibEdit */
extern LibEDA_BaseStruct* LibItemToRepeat; /* pointer on a graphic item than
                                            * can be duplicated by the Ins key
                                            * (usually the last created item */
extern CMP_LIBRARY*       CurrentLib;      /* Current opened library */
extern LibEDA_BaseStruct* CurrentDrawItem; /* current edited item */

extern wxString CurrentAliasName;
extern bool     g_AsDeMorgan;
extern int      CurrentUnit;
extern int      CurrentConvert;

#endif  //  LIBCMP_H
