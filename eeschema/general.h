/**
 * @file general.h
 */

#ifndef _GENERAL_H_
#define _GENERAL_H_

#include <wx/string.h>
#include <wx/gdicmn.h>

#include <block_commande.h>
#include <class_netlist_object.h>


class SCH_ITEM;
class SCH_SHEET;
class TRANSFORM;


#define EESCHEMA_VERSION 2

#define SCHEMATIC_HEAD_STRING "Schematic File Version"
#define EESCHEMA_FILE_STAMP   "EESchema"
#define NULL_STRING           "_NONAME_"

// Define the char buffer size used to read library files
#define LINE_BUFFER_LEN_LARGE   8000
#define LINE_BUFFER_LEN   1024

#define MAX_PIN_INFO 10

#define TXTMARGE 10                 // Offset in mils for placement of labels and pin numbers.

#define HIGHLIGHT_COLOR WHITE

//#define GR_DEFAULT_DRAWMODE GR_COPY
#define GR_DEFAULT_DRAWMODE GR_COPY

#define DANGLING_SYMBOL_SIZE 12

extern wxString g_DefaultSchematicFileName;

typedef enum
{
    LAYER_WIRE,
    LAYER_BUS,
    LAYER_JUNCTION,
    LAYER_LOCLABEL,
    LAYER_GLOBLABEL,
    LAYER_HIERLABEL,
    LAYER_PINFUN,
    LAYER_PINNUM,
    LAYER_PINNAM,
    LAYER_REFERENCEPART,
    LAYER_VALUEPART,
    LAYER_FIELDS,
    LAYER_DEVICE,
    LAYER_NOTES,
    LAYER_NETNAM,
    LAYER_PIN,
    LAYER_SHEET,
    LAYER_SHEETNAME,
    LAYER_SHEETFILENAME,
    LAYER_SHEETLABEL,
    LAYER_NOCONNECT,
    LAYER_ERC_WARN,
    LAYER_ERC_ERR,
    LAYER_DEVICE_BACKGROUND,
    LAYER_GRID,

    MAX_LAYER                   /* Maximum layers */
} LayerNumber;


typedef enum
{
    FILE_SAVE_AS,
    FILE_SAVE_NEW
} FileSaveType;


/* Rotation, mirror of graphic items in components bodies are handled by a
 * transform matrix.  The default matix is useful to draw lib entries with
 * a defualt matix ( no rotation, no mirror but Y axis is bottom to top, and
 * Y draw axis is to to bottom so we must have a default matix that reverses
 * the Y coordinate and keeps the X coordiate
 */
extern TRANSFORM DefaultTransform;

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

extern wxSize g_RepeatStep;
extern int g_RepeatDeltaLabel;

// Management options.
extern bool g_HVLines;

extern struct EESchemaVariables g_EESchemaVar;

extern int      g_DefaultTextLabelSize;

struct HPGL_Pen_Descr_Struct
{
    int m_Pen_Num;      /* Pen number */
    int m_Pen_Speed;    /* Pen speed in cm/s */
    int m_Pen_Diam;     /* Pen diameter in mils */
};
extern HPGL_Pen_Descr_Struct g_HPGL_Pen_Descr;

/* First and main (root) screen */
extern SCH_SHEET*     g_RootSheet;

extern wxString       g_NetCmpExtBuffer;

extern const wxString SymbolFileExtension;
extern const wxString SymbolFileWildcard;

extern const wxString CompLibFileExtension;
extern const wxString CompLibFileWildcard;

extern const wxString g_SchematicBackupFileExtension;

extern LayerStruct    g_LayerDescr;

/// True to prevent displacing pins, when they are at the same position.
extern bool           g_EditPinByPinIsOn;

/**
 * Default line (in Eeschema units) thickness used to draw/plot items having a
 * default thickness line value (i.e. = 0 ).
 * 0 = single pixel line width.
 */
extern int            g_DrawDefaultLineThickness;


/// Color to draw selected items
extern int g_ItemSelectetColor;

/// Color to draw items flagged invisible, in libedit (they are invisible in Eeschema
extern int g_InvisibleItemColor;

/* Global Variables */

extern NETLIST_OBJECT_LIST g_NetObjectslist;

extern bool g_OptNetListUseNames;   /* true to use names rather than
                                     * net numbers. SPICE netlist only
                                     */

#endif   // _GENERAL_H_
