/********************************************/
/* Definitions for the EESchema program:	*/
/********************************************/

#ifndef PROGRAM_H
#define PROGRAM_H

#ifndef eda_global
#define eda_global extern
#endif

#include "wxEeschemaStruct.h"
#include "macros.h"
#include "base_struct.h"
#include "sch_item_struct.h"

#include "component_class.h"
#include "class_screen.h"
#include "class_drawsheet.h"
#include "class_drawsheetpath.h"
#include "class_text-label.h"
#include "class_schematic_items.h"

#define HIGHLIGHT_COLOR WHITE

#define TEXT_NO_VISIBLE 1

/* Rotation, mirror of graphic items in components bodies are handled by a transform matrix
 *  The default matix is useful to draw lib entries with a defualt matix ( no rotation, no mirrot
 *  but Y axis is bottom to top, and Y draw axis is to to bottom
 *  so we must have a default matix that reverses the Y coordinate and keeps the X coordiante
 *      DefaultTransformMatrix[0][0] = 1; DefaultTransformMatrix[1][1] = -1;
 *      DefaultTransformMatrix[1][0] = DefaultTransformMatrix[0][1] = 0;
 */
eda_global int DefaultTransformMatrix[2][2]
#ifdef MAIN
    = { {1, 0}, {0, -1} }
#endif
;


#define MAX_LAYERS 44
class LayerStruct
{
public:
    char LayerNames[MAX_LAYERS + 1][8];
    int  LayerColor[MAX_LAYERS + 1];
    char LayerStatus[MAX_LAYERS + 1];
    int  NumberOfLayers;
    int  CurrentLayer;
    int  CurrentWidth;
    int  CommonColor;
    int  Flags;
};


#endif /* PROGRAM_H */
