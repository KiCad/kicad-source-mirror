/**************/
/* gerbview.h */
/**************/

#ifndef GERBVIEW_H
#define GERBVIEW_H

#include <vector>
#include <set>

#include "dcode.h"
#include "class_gerber_draw_item.h"
#include "class_aperture_macro.h"

class WinEDA_GerberFrame;
//class BOARD;

class GERBER;

// Type of photoplotter action:
#define GERB_ACTIVE_DRAW 1      // Activate light (lower pen)
#define GERB_STOP_DRAW   2      // Extinguish light (lift pen)
#define GERB_FLASH       3      // Flash


typedef enum
{
    FORMAT_HPGL,
    FORMAT_GERBER,
    FORMAT_POST
} PlotFormat;

/**
 * Enum ITEM_VISIBLE
 * is a set of visible PCB elements.
 */
enum GERBER_VISIBLE
{
    DCODES_VISIBLE = 1,    // visible item id cannot be 0 because this id is used as wxWidget id
    GERBER_GRID_VISIBLE,
    END_GERBER_VISIBLE_LIST  // sentinel
};

/**
* size of single line of a text from a gerber file.
* warning: some files can have very long lines, so the buffer must be large.
*/
#define GERBER_BUFZ     4000

extern wxString g_PhotoFilenameExt;
extern wxString g_DrillFilenameExt;
extern wxString g_PenFilenameExt;

extern int     g_DisplayPolygonsModeSketch;

extern const wxString GerbviewProjectFileExt;
extern const wxString GerbviewProjectFileWildcard;

extern Ki_PageDescr* g_GerberPageSizeList[];

// Config keywords
extern const wxString GerbviewShowPageSizeOption;
extern const wxString GerbviewShowDCodes;

// Interpolation type
enum Gerb_Interpolation
{
    GERB_INTERPOL_LINEAR_1X = 0,
    GERB_INTERPOL_LINEAR_10X,
    GERB_INTERPOL_LINEAR_01X,
    GERB_INTERPOL_LINEAR_001X,
    GERB_INTERPOL_ARC_NEG,
    GERB_INTERPOL_ARC_POS
};


// Command Type (GCodes)
enum Gerb_GCommand
{
    GC_MOVE                     = 0,
    GC_LINEAR_INTERPOL_1X       = 1,
    GC_CIRCLE_NEG_INTERPOL      = 2,
    GC_CIRCLE_POS_INTERPOL      = 3,
    GC_COMMENT                  = 4,
    GC_LINEAR_INTERPOL_10X      = 10,
    GC_LINEAR_INTERPOL_0P1X     = 11,
    GC_LINEAR_INTERPOL_0P01X    = 12,
    GC_TURN_ON_POLY_FILL        = 36,
    GC_TURN_OFF_POLY_FILL       = 37,
    GC_SELECT_TOOL              = 54,
    GC_PHOTO_MODE               = 55,          // can start a D03 flash command: redundant with D03
    GC_SPECIFY_INCHES           = 70,
    GC_SPECIFY_MILLIMETERS      = 71,
    GC_TURN_OFF_360_INTERPOL    = 74,
    GC_TURN_ON_360_INTERPOL     = 75,
    GC_SPECIFY_ABSOLUES_COORD   = 90,
    GC_SPECIFY_RELATIVEES_COORD = 91
};


enum Gerb_Analyse_Cmd
{
    CMD_IDLE = 0,
    END_BLOCK,
    ENTER_RS274X_CMD
};


/**************/
/* rs274x.cpp */
/**************/
bool GetEndOfBlock( char buff[GERBER_BUFZ], char*& text, FILE* gerber_file );
extern GERBER* g_GERBER_List[32];

#include "pcbcommon.h"
#include "wxGerberFrame.h"

#endif  // ifndef GERBVIEW_H
