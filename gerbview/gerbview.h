/*******************************************************/
/* Menu General de Trace (PLOT): Fichier inclus PLOT.H */
/*******************************************************/

#ifndef GERBVIEW_H
#define GERBVIEW_H

#ifndef eda_global
#define eda_global extern
#endif

// Type d'action du phototraceur:
#define GERB_ACTIVE_DRAW 1      // activation de lumiere ( baisser de plume)
#define GERB_STOP_DRAW   2      // extinction de lumiere ( lever de plume)
#define GERB_FLASH       3      // Flash


typedef enum {
    FORMAT_HPGL,
    FORMAT_GERBER,
    FORMAT_POST
} PlotFormat;

//eda_global wxString g_Plot_FileName;
eda_global wxString g_PhotoFilenameExt;
eda_global wxString g_DrillFilenameExt;
eda_global wxString g_PenFilenameExt;

eda_global int      g_DCodesColor;
eda_global int      g_Default_GERBER_Format;


/* Gestion des ouvertures GERBER */
eda_global int g_Plot_Spot_Mini;            /* Diametre mini de l'ouverture pour trace GERBER */


/*************************************/
/* Constantes utiles en trace GERBER */
/*************************************/

/* codes de type de forme d'outils */
enum Gerb_StandardShape {
    GERB_CIRCLE = 1,
    GERB_RECT,
    GERB_LINE,
    GERB_OVALE,
    GERB_SPECIAL_SHAPE
};

// Interpolation type
enum Gerb_Interpolation {
    GERB_INTERPOL_LINEAR_1X = 0,
    GERB_INTERPOL_LINEAR_10X,
    GERB_INTERPOL_LINEAR_01X,
    GERB_INTERPOL_LINEAR_001X,
    GERB_INTERPOL_ARC_NEG,
    GERB_INTERPOL_ARC_POS,
};


// Command Type (GCodes)
enum Gerb_GCommand {
    GC_MOVE = 0,
    GC_LINEAR_INTERPOL_1X    = 1,
    GC_CIRCLE_NEG_INTERPOL   = 2,
    GC_CIRCLE_POS_INTERPOL   = 3,
    GC_COMMENT = 4,
    GC_LINEAR_INTERPOL_10X   = 10,
    GC_LINEAR_INTERPOL_0P1X  = 11,
    GC_LINEAR_INTERPOL_0P01X = 12,
    GC_TURN_ON_POLY_FILL        = 36,
    GC_TURN_OFF_POLY_FILL       = 37,
    GC_SELECT_TOOL = 54,
    GC_SPECIFY_INCHES = 70,
    GC_SPECIFY_MILLIMETERS      = 71,
    GC_TURN_OFF_360_INTERPOL    = 74,
    GC_TURN_ON_360_INTERPOL     = 75,
    GC_SPECIFY_ABSOLUES_COORD   = 90,
    GC_SPECIFY_RELATIVEES_COORD = 91
};


#define MAX_TOOLS   2048
#define FIRST_DCODE 10

enum Gerb_Analyse_Cmd {
    CMD_IDLE = 0,
    END_BLOCK,
    ENTER_RS274X_CMD
};


class D_CODE;

/* Structure de Description d'option d'une layer GERBER : */
class GERBER_Descr
{
public:
    GERBER_Descr* m_Parent;                                     // Pointeur sur la racine pour layers imbriquées
    GERBER_Descr* m_Pback;                                      // Pointeur de chainage arriere pour layers imbriquées
    GERBER_Descr* m_Pnext;                                      // Pointeur de chainage avant pour layers imbriquées
    wxString      m_FileName;                                   // Full File Name for this layer
    wxString      m_Name;                                       // Layer name
    int           m_Layer;                                      // Layer Number
    bool          m_LayerNegative;                              // TRUE = Negative Layer
    bool          m_GerbMetric;                                 // FALSE = Inches, TRUE = metric
    bool          m_Relative;                                   // FALSE = absolute Coord, RUE = relative Coord
    bool          m_NoTrailingZeros;                            // True: zeros a droite supprimés
    bool          m_MirorA;                                     // True: miror / axe A (X)
    bool          m_MirorB;                                     // True: miror / axe B (Y)
    bool          m_As_DCode;                                   // TRUE = DCodes in file (FALSE = no DCode->
    // separate DCode file
    wxPoint       m_Offset;                                     // Coord Offset
    wxSize        m_FmtScale;                                   // Fmt 2.3: m_FmtScale = 3, fmt 3.4: m_FmtScale = 4
    wxSize        m_FmtLen;                                     // Nb chars per coord. ex fmt 2.3, m_FmtLen = 5
    wxRealPoint   m_LayerScale;                                 // scale (X et Y) pour cette layer
    int           m_Rotation;
    int           m_Iterpolation;                               // Linear, 90 arc, Circ.
    bool          m_ImageNegative;                              // TRUE = Negative image
    int           m_Current_Tool;                               // Current Tool (Dcode) number selected
    int           m_Last_Pen_Command;                           // Current or last pen state (0..9, set by Dn option with n <10
    int           m_CommandState;                               // donne l'etat de l'analyse des commandes gerber
    wxPoint       m_CurrentPos;                                 // current specified coord for plot
    wxPoint       m_PreviousPos;                                // old current specified coord for plot
    wxPoint       m_IJPos;                                      // IJ coord (for arcs & circles )
    D_CODE*       m_Aperture_List[MAX_TOOLS + FIRST_DCODE + 1]; // Dcode (Aperture) List for this layer

    FILE*         m_Current_File;                               // Current file to read
    FILE*         m_FilesList[12];                              // Files list
    int           m_FilesPtr;                                   // Stack pointer for files list

    int           m_Selected_Tool;                              // Pour editions: Tool (Dcode) selectionné

    int           m_Transform[2][2];                            // The rotation/mirror transformation matrix.
    bool          m_360Arc_enbl;                                // Enbl 360 deg circular interpolation
    bool          m_PolygonFillMode;                            // Enbl polygon mode (read coord as a polygone descr)
    int           m_PolygonFillModeState;                       // In polygon mode: 0 = first segm, 1 = next segm

public:
    GERBER_Descr( int layer );
    ~GERBER_Descr();
    void    Clear_GERBER_Descr();
    int     ReturnUsedDcodeNumber();
    void    ResetDefaultValues();
    void    InitToolTable();

    // Routines utilisées en lecture de ficher gerber
    wxPoint ReadXYCoord( char*& Text );
    wxPoint ReadIJCoord( char*& Text );
    int     ReturnGCodeNumber( char*& Text );
    int     ReturnDCodeNumber( char*& Text );
    bool    Execute_G_Command( char*& text, int G_commande );
    bool    Execute_DCODE_Command( WinEDA_GerberFrame* frame, wxDC* DC,
                                   char*& text, int D_commande );
    bool    ReadRS274XCommand( WinEDA_GerberFrame* frame, wxDC* DC,
                               char* buff, char*& text );
    bool    ExecuteRS274XCommand( int command, char* buff, char*& text );
};


/* Structure de Description d'un D_CODE GERBER : */

class D_CODE
{
public:
    wxSize   m_Size;        /* Dimensions horiz et Vert */
    int      m_Shape;       /* shape ( Line, rect , circulaire , ovale .. ) */
    int      m_Num_Dcode;   /* numero de code ( >= 10 ) */
    wxSize   m_Drill;       /* dimension du trou central (s'il existe) */
    int      m_DrillShape;  /* forme du trou central ( rond = 1, rect = 2 ) */
    bool     m_InUse;       /* FALSE si non utilisé */
    bool     m_Defined;     /* FALSE si non defini */
    wxString m_SpecialDescr;

public:
    D_CODE( int num_dcode );
    ~D_CODE();
    void Clear_D_CODE_Data();
};

eda_global const wxChar* g_GERBER_Tool_Type[6]
#ifdef MAIN
= {
    wxT( "????" ), wxT( "Rond" ), wxT( "Rect" ), wxT( "Line" ), wxT( "Oval" ), wxT( "Macro" )
}


#endif
;

eda_global GERBER_Descr* g_GERBER_Descr_List[32];
eda_global int           g_DisplayPolygonsModeSketch; /* How to show filled polygons :
                                        * 0 = filled
                                        * 1 = Sketch mode
                                        */


#include "pcbnew.h"

#endif  // ifndef GERBVIEW_H
