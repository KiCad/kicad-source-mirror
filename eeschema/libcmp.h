/*****************************************************************/
/*  Headers for library definition and lib component definitions */
/*****************************************************************/

#ifndef LIBCMP_H
#define LIBCMP_H


#define LIB_VERSION_MAJOR 2
#define LIB_VERSION_MINOR 3
#define LIBFILE_IDENT     "EESchema-LIBRARY Version"        /* Must be the first line of lib files. */
#define DOCFILE_IDENT     "EESchema-DOCLIB  Version 2.0"    /* Must be the first line of doc files. */
#define DOC_EXT           wxT( "dcm" )                     /* Ext. of documentation files */


enum LocateDrawStructType {
    LOCATE_COMPONENT_ARC_DRAW_TYPE = 1,
    LOCATE_COMPONENT_CIRCLE_DRAW_TYPE   = 2,
    LOCATE_COMPONENT_GRAPHIC_TEXT_DRAW_TYPE = 4,
    LOCATE_COMPONENT_RECT_DRAW_TYPE     = 8,
    LOCATE_LINE_DRAW_TYPE = 0x10,
    LOCATE_COMPONENT_POLYLINE_DRAW_TYPE = 0x20,
    LOCATE_COMPONENT_LINE_DRAW_TYPE     = 0x40
};

#define LOCATE_ALL_DRAW_ITEM 0xFFFFFFFF


#include "class_library.h"


/* Variables */
extern LibraryStruct*      LibraryList;     /* All part libs are saved here. */

/* Variables used by LibEdit */
extern LibEDA_BaseStruct*  LibItemToRepeat;     /* pointer on a graphic item than can be duplicated by the Ins key
                                                          * (usually the last created item */
extern LibraryStruct*          CurrentLib;          /* Current opened library */
extern EDA_LibComponentStruct* CurrentLibEntry;     /* Current component */
extern LibEDA_BaseStruct*      CurrentDrawItem;     /* current edited item */

extern wxString CurrentAliasName;
extern bool     g_AsDeMorgan;
extern int      CurrentUnit;
extern int      CurrentConvert;

extern wxString FindLibName;

#endif  //  LIBCMP_H
