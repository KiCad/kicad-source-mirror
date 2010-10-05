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
class BOARD;

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

extern wxString g_PhotoFilenameExt;
extern wxString g_DrillFilenameExt;
extern wxString g_PenFilenameExt;

extern int      g_Default_GERBER_Format;

extern int      g_Plot_Spot_Mini;    /* Diameter of the opening mini-track for
                                      * GERBER */

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

class D_CODE;


/**
 * Class GERBER
 * holds the data for one gerber file or layer
 */
class GERBER
{
    WinEDA_GerberFrame * m_Parent;              // the parent WinEDA_GerberFrame (used to display messages...)
    D_CODE*       m_Aperture_List[TOOLS_MAX_COUNT];   ///< Dcode (Aperture) List for this layer
    bool          m_Exposure;                   ///< whether an aperture macro tool is flashed on or off

    BOARD*        m_Pcb;

public:
    wxString      m_FileName;                   // Full File Name for this layer
    wxString      m_Name;                       // Layer name
    int           m_Layer;                      // Layer Number
    bool          m_LayerNegative;              // TRUE = Negative Layer
    bool          m_GerbMetric;                 // FALSE = Inches, TRUE = metric
    bool          m_Relative;                   // FALSE = absolute Coord, RUE = relative Coord
    bool          m_NoTrailingZeros;            // True: remove tailing zeros.
    bool          m_MirorA;                     // True: miror / axe A (X)
    bool          m_MirorB;                     // True: miror / axe B (Y)
    bool          m_Has_DCode;                  // TRUE = DCodes in file (FALSE = no DCode->
    // separate DCode file
    wxPoint       m_Offset;                     // Coord Offset
    wxSize        m_FmtScale;                   // Fmt 2.3: m_FmtScale = 3, fmt 3.4: m_FmtScale = 4
    wxSize        m_FmtLen;                     // Nb chars per coord. ex fmt 2.3, m_FmtLen = 5
    wxRealPoint   m_LayerScale;                 // scale (X and Y) of layer.
    int           m_Rotation;
    int           m_Iterpolation;               // Linear, 90 arc, Circ.
    bool          m_ImageNegative;              // TRUE = Negative image
    int           m_Current_Tool;               // Current Tool (Dcode) number selected
    int           m_Last_Pen_Command;           // Current or last pen state (0..9, set by Dn option with n <10
    int           m_CommandState;               // state of gerber analysis command.
    wxPoint       m_CurrentPos;                 // current specified coord for plot
    wxPoint       m_PreviousPos;                // old current specified coord for plot
    wxPoint       m_IJPos;                      // IJ coord (for arcs & circles )

    FILE*         m_Current_File;               // Current file to read
    FILE*         m_FilesList[12];              // Files list
    int           m_FilesPtr;                   // Stack pointer for files list

    int           m_Selected_Tool;              // Pour editions: Tool (Dcode) selectionn�

    bool          m_360Arc_enbl;                // Enbl 360 deg circular interpolation
    bool          m_PolygonFillMode;            // Enbl polygon mode (read coord as a polygon descr)
    int           m_PolygonFillModeState;       // In polygon mode: 0 = first segm, 1 = next segm

    APERTURE_MACRO_SET   m_aperture_macros;     ///< a collection of APERTURE_MACROS, sorted by name

public:
    GERBER( WinEDA_GerberFrame * aParent, int layer );
    ~GERBER();
    void    Clear_GERBER();
    int     ReturnUsedDcodeNumber();
    void    ResetDefaultValues();

    /** function ReportMessage
     * Add a message (a string) in message list
     * for instance when reading a Gerber file
     * @param aMessage = the straing to add in list
     */
    void ReportMessage(const wxString aMessage );

    /** function ClearMessageList
     * Clear the message list
     * Call it before reading a Gerber file
     */
    void ClearMessageList( );

    /**
     * Function InitToolTable
     */
    void    InitToolTable();

    wxPoint ReadXYCoord( char*& Text );
    wxPoint ReadIJCoord( char*& Text );
    int     ReturnGCodeNumber( char*& Text );
    int     ReturnDCodeNumber( char*& Text );
    bool    Execute_G_Command( char*& text, int G_commande );
    bool    Execute_DCODE_Command( WinEDA_GerberFrame* frame,
                                   char*& text, int D_commande );

   /**
    * size of single line of a text from a gerber file.
    * warning: some files can have very long lines, so the buffer must be large.
    */
#define GERBER_BUFZ     4000

    /**
     * Function ReadRS274XCommand
     * reads a single RS274X command terminated with a %
     */
    bool    ReadRS274XCommand( WinEDA_GerberFrame* frame,
                               char aBuff[GERBER_BUFZ], char*& text );

    /**
     * Function ExecuteRS274XCommand
     * executes 1 command
     */
    bool    ExecuteRS274XCommand( int command, char aBuff[GERBER_BUFZ],
                                  char*& text );


    /**
     * Function ReadApertureMacro
     * reads in an aperture macro and saves it in m_aperture_macros.
     * @param aBuff a character buffer at least GERBER_BUFZ long that can be
     *          used to read successive lines from the gerber file.
     * @param text A reference to a character pointer which gives the initial
     *              text to read from.
     * @param gerber_file Which file to read from for continuation.
     * @return bool - true if a macro was read in successfully, else false.
     */
    bool    ReadApertureMacro( char aBuff[GERBER_BUFZ], char*& text,
                               FILE* gerber_file );


    /**
     * Function GetDCODE
     * returns a pointer to the D_CODE within this GERBER for the given
     * \a aDCODE.
     * @param aDCODE The numeric value of the D_CODE to look up.
     * @param createIfNoExist If true, then create the D_CODE if it does not
     *                        exist.
     * @return D_CODE* - the one implied by the given \a aDCODE, or NULL
     *            if the requested \a aDCODE is out of range.
     */
    D_CODE* GetDCODE( int aDCODE, bool createIfNoExist=true );

    /**
     * Function FindApertureMacro
     * looks up a previously read in aperture macro.
     * @param aLookup A dummy APERTURE_MACRO with [only] the name field set.
     * @return APERTURE_MACRO* - the one with a matching name, or NULL if
     *                           not found.
     */
    APERTURE_MACRO* FindApertureMacro( const APERTURE_MACRO& aLookup );
};


/**************/
/* rs274x.cpp */
/**************/
bool GetEndOfBlock( char buff[GERBER_BUFZ], char*& text, FILE* gerber_file );


extern GERBER* g_GERBER_List[32];

extern int     g_DisplayPolygonsModeSketch;

#include "pcbcommon.h"
#include "wxGerberFrame.h"

#endif  // ifndef GERBVIEW_H
