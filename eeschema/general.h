/***************/
/*  GENERAL.H  */
/***************/

#ifndef _GENERAL_H_
#define _GENERAL_H_


#define EESCHEMA_VERSION 2

#define SCHEMATIC_HEAD_STRING "Schematic File Version"
#define EESCHEMA_FILE_STAMP   "EESchema"
#define NULL_STRING           "_NONAME_"

#define MAX_PIN_INFO 10

#define TXTMARGE 10                 /* Offset in mils for placement of labels
                                     * and pin numbers. */

#define HIGHLIGHT_COLOR WHITE

/* Used for EDA_BaseStruct, .m_Select member */
#define IS_SELECTED 1

#define TEXT_NO_VISIBLE 1


//#define GR_DEFAULT_DRAWMODE GR_COPY
#define GR_DEFAULT_DRAWMODE GR_COPY

#define DANGLING_SYMBOL_SIZE 12

extern wxString g_DefaultSchematicFileName;

/* Search mask for locating objects in editor. */
#define LIBITEM                    1
#define WIREITEM                   2
#define BUSITEM                    4
#define RACCORDITEM                4
#define JUNCTIONITEM               0x10
#define DRAWITEM                   0x20
#define TEXTITEM                   0x40
#define LABELITEM                  0x80
#define SHEETITEM                  0x100
#define MARKERITEM                 0x200
#define NOCONNECTITEM              0x400
#define SEARCH_PINITEM             0x800
#define SHEETLABELITEM             0x1000
#define FIELDCMPITEM               0x2000
#define EXCLUDE_WIRE_BUS_ENDPOINTS 0x4000
#define WIRE_BUS_ENDPOINTS_ONLY    0x8000

#define SEARCHALL ( LIBITEM | WIREITEM | BUSITEM | RACCORDITEM |        \
                    JUNCTIONITEM | DRAWITEM | TEXTITEM | LABELITEM |    \
                    SHEETITEM | MARKERITEM | NOCONNECTITEM |            \
                    SEARCH_PINITEM | SHEETLABELITEM )

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

    MAX_LAYER                   /* Maximum layers */
} LayerNumber;


typedef enum
{
    FILE_SAVE_AS,
    FILE_SAVE_NEW
} FileSaveType;


extern int            g_OptNetListUseNames;  /* TRUE to use names rather than
                                              * net numbers (PSPICE netlist
                                              * only) */
extern SCH_ITEM*      g_ItemToRepeat; /* Pointer to the last structure used
                                       * by the repeat command.   NULL if no
                                       * item to repeat */
extern wxSize         g_RepeatStep;
extern int            g_RepeatDeltaLabel;

extern SCH_ITEM*      g_ItemToUndoCopy; /* copy of last modified schematic item
                                         * before it is modified (used for undo
                                         * managing to restore old values ) */

extern bool           g_LastSearchIsMarker; /* True if last search is a marker
                                             * search.  False for a schematic
                                             * item search.  Used for hotkey
                                             * next search. */

/* Block operation (copy, paste) */
extern BLOCK_SELECTOR g_BlockSaveDataList; /* List of items to paste (Created
                                            * by Block Save) */

// Management options.
extern bool      g_HVLines;

// Management variables, option ... to be stored.  Reset to 0 during a
// project reload.
struct EESchemaVariables
{
    int NbErrorErc;
    int NbWarningErc;
};

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

extern wxString       g_SimulatorCommandLine;
extern wxString       g_NetListerCommandLine;

extern LayerStruct    g_LayerDescr;

extern bool           g_EditPinByPinIsOn;   /* True to prevent displacing
                                             * pins, when they are at the
                                             * same position. */

extern int            g_DrawDefaultLineThickness; /* Default line (in EESCHEMA
                                                   * units) thickness used to
                                                   * draw/plot items having a
                                                   * default thickness line
                                                   * value (i.e. = 0 ).
                                                   * 0 = single pixel line width
                                                   */

// Color to draw selected items
extern int g_ItemSelectetColor;

// Color to draw items flagged invisible, in libedit (they are invisible in
// eeschema
extern int g_InvisibleItemColor;

#endif   // _GENERAL_H_
