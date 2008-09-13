/*****************************************************************/
/*	Headers for library definition and lib component definitions */
/*****************************************************************/

#ifndef LIBCMP_H
#define LIBCMP_H

#ifndef eda_global
#define eda_global extern
#endif

#include "priorque.h"

#define LIB_VERSION_MAJOR 2
#define LIB_VERSION_MINOR 3
#define LIBFILE_IDENT     "EESchema-LIBRARY Version"        /* Must be the first line of lib files. */
#define DOCFILE_IDENT     "EESchema-DOCLIB  Version 2.0"    /* Must be the first line of doc files. */
#define DOC_EXT           wxT( ".dcm" )                     /* Ext. of documentation files */


//Offsets used in editing library component, for handle aliad dats
#define ALIAS_NAME         0
#define ALIAS_DOC          1
#define ALIAS_KEYWORD      2
#define ALIAS_DOC_FILENAME 3
#define ALIAS_NEXT         4


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

/* flags utilises dans FindLibPart() : */
#define FIND_ROOT  0    /* Used to search for a root component by its name
                         *  if the name is an alias name, FindLibPart() returns the root component */
#define FIND_ALIAS 1    /* Used to search for a component by its name
                         *  FindLibPart() returns the component (root or alias ) */

#include "class_library.h"


/* Variables */
extern LibraryStruct*              LibraryList;         /* All part libs are saved here. */

/* Variables used by LibEdit */
eda_global LibEDA_BaseStruct*      LibItemToRepeat;     /* pointer on a graphic item than can be duplicated by the Ins key
                                                          * (usually the last created item */
eda_global LibraryStruct*          CurrentLib;          /* Current opened library */
eda_global EDA_LibComponentStruct* CurrentLibEntry;     /* Current component */
eda_global LibEDA_BaseStruct*      CurrentDrawItem;     /* current edited item */

eda_global wxString CurrentAliasName;                   // Current selected alias (for components which have aliases)
eda_global bool     g_AsDeMorgan;                       // True if the current component has a "De Morgan" representation
eda_global int      CurrentUnit                         // Current selected part
#ifdef MAIN
= 1
#endif
;
eda_global int CurrentConvert                   /* Convert = 1 .. 255 */
#ifdef MAIN
= 1
#endif
;

eda_global wxString FindLibName;        /* Library (name) containing the last component find by FindLibPart() */

#endif  //  LIBCMP_H
