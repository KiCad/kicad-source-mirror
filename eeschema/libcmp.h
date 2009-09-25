/*****************************************************************/
/*  Headers for library definition and lib component definitions */
/*****************************************************************/

#ifndef LIBCMP_H
#define LIBCMP_H


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


#endif  //  LIBCMP_H
