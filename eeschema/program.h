/********************************************/
/* Definitions for the EESchema program:	*/
/********************************************/

#ifndef PROGRAM_H
#define PROGRAM_H

#define HIGHLIGHT_COLOR WHITE
#define TEXT_NO_VISIBLE 1

#include "wxEeschemaStruct.h"
#include "macros.h"
#include "base_struct.h"
#include "sch_item_struct.h"

#include "class_sch_component.h"
#include "class_sch_screen.h"
#include "class_drawsheet.h"
#include "class_drawsheetpath.h"
#include "class_text-label.h"
#include "class_schematic_items.h"


/* Rotation, mirror of graphic items in components bodies are handled by a
 * transform matrix.  The default matix is useful to draw lib entries with
 * a defualt matix ( no rotation, no mirror but Y axis is bottom to top, and
 * Y draw axis is to to bottom so we must have a default matix that reverses
 * the Y coordinate and keeps the X coordiate
 *      DefaultTransformMatrix[0][0] = 1; DefaultTransformMatrix[1][1] = -1;
 *      DefaultTransformMatrix[1][0] = DefaultTransformMatrix[0][1] = 0;
 */
extern int DefaultTransformMatrix[2][2];

#define MIN_BUSLINES_THICKNESS 12   // min bus lines and entries thickness

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
