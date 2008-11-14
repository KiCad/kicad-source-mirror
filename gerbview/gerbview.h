/*******************************************************/
/* Menu General de Trace (PLOT): Fichier inclus PLOT.H */
/*******************************************************/

#ifndef GERBVIEW_H
#define GERBVIEW_H

#include <vector>
#include <set>


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

/**
 * Enum APERTURE_T
 * is the set of all gerber aperture types allowed, according to page 16 of
 * http://gerbv.sourceforge.net/docs/rs274xrevd_e.pdf
 */
enum APERTURE_T
{
    APT_CIRCLE = 'C',
    APT_LINE = 'L',
    APT_RECT = 'R',
    APT_OVAL = '0',
    APT_POLYGON = 'P',
    APT_MACRO = 'M'
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


/**
 * Class DCODE_PARAM
 * holds a parameter for a DCODE or an "aperture macro" as defined within standard RS274X.
 * The \a value field can be either a constant or a place holder for a DCODE
 * parameter.
 */
class DCODE_PARAM
{
public:
    DCODE_PARAM() :
        index(-1),
        value(0.0)
    {}

    double  GetValue( const D_CODE* aDcode ) const;
    void SetValue( double aValue )
    {
        value = aValue;
        index = -1;
    }

    /**
     * Function IsImmediate
     * tests if this DCODE_PARAM holds an immediate parameter or is a pointer into
     * a parameter held by an owning D_CODE.
     */
    bool IsImmediate() const { return index == -1; }

    int GetIndex() const
    {
        return index;
    }

    void SetIndex( int aIndex )
    {
        index = aIndex;
    }

private:
    int     index;      ///< if -1, then \a value field is an immediate value, else this is an index into parent's D_CODE.m_am_params.
    double  value;      ///< if IsImmediate()==true then the value, else not used.
};


/**
 * Enum AM_PRIMITIVE_ID
 * is the set of all "aperture macro primitives" (primitive numbers).  See
 * Table 3 in http://gerbv.sourceforge.net/docs/rs274xrevd_e.pdf
 */
enum AM_PRIMITIVE_ID
{
    AMP_CIRCLE  = 1,
    AMP_LINE2 = 2,
    AMP_LINE20 = 20,
    AMP_LINE_CENTER = 21,
    AMP_LINE_LOWER_LEFT = 22,
    AMP_EOF = 3,
    AMP_OUTLINE = 4,
    AMP_POLYGON = 5,
    AMP_MOIRE = 6,
    AMP_THERMAL = 7,
};


typedef std::vector<DCODE_PARAM>   DCODE_PARAMS;

/**
 * Struct AM_PRIMITIVE
 * holds an aperture macro primitive as given in Table 3 of
 * http://gerbv.sourceforge.net/docs/rs274xrevd_e.pdf
 */
struct AM_PRIMITIVE
{
    AM_PRIMITIVE_ID     primitive_id;   ///< The primitive type
    DCODE_PARAMS        params;         ///< A sequence of parameters used by the primitive

    /**
     * Function GetExposure
     * returns the first parameter in integer form.  Some but not all primitives
     * use the first parameter as an exposure control.
     */
    int GetExposure() const
    {
        wxASSERT( params.size() && params[0].IsImmediate() );    // we have no D_CODE* for GetValue()
        return (int) params[0].GetValue( NULL );
    }
};


typedef std::vector<AM_PRIMITIVE>   AM_PRIMITIVES;

/**
 * Struct APERTURE_MACRO
 * helps support the "aperture macro" defined within standard RS274X.
 */
struct APERTURE_MACRO
{
    wxString        name;           ///< The name of the aperture macro
    AM_PRIMITIVES   primitives;     ///< A sequence of AM_PRIMITIVEs
};


/**
 * Struct APERTURE_MACRO_less_than
 * is used by std:set<APERTURE_MACRO> instantiation which uses APERTURE_MACRO.name as its key.
 */
struct APERTURE_MACRO_less_than
{
    // a "less than" test on two APERTURE_MACROs (.name wxStrings)
    bool operator()( const APERTURE_MACRO& am1, const APERTURE_MACRO& am2) const
    {
        return am1.name.Cmp( am2.name ) < 0;  // case specific wxString compare
    }
};


/**
 * Type APERTURE_MACRO_SET
 * is a sorted collection of APERTURE_MACROS whose key is the name field in
 * the APERTURE_MACRO.
 */
typedef std::set<APERTURE_MACRO, APERTURE_MACRO_less_than>  APERTURE_MACRO_SET;
typedef std::pair<APERTURE_MACRO_SET::iterator, bool> APERTURE_MACRO_SET_PAIR;


/**
 * Class D_CODE
 * holds a gerber DCODE definition.
 */
class D_CODE
{
    APERTURE_MACRO* m_Macro;    ///< no ownership, points to GERBER.m_aperture_macros element

    /**
     * parameters used only when this D_CODE holds a reference to an aperture
     * macro, and these parameters would customize the macro.
     */
    DCODE_PARAMS   m_am_params;

public:
    wxSize      m_Size;        /* Dimensions horiz et Vert */
    APERTURE_T  m_Shape;       /* shape ( Line, rect , circulaire , ovale .. ) */
    int         m_Num_Dcode;   /* numero de code ( >= 10 ) */
    wxSize      m_Drill;       /* dimension du trou central (s'il existe) */
    int         m_DrillShape;  /* forme du trou central ( rond = 1, rect = 2 ) */
    bool        m_InUse;       /* FALSE si non utilisé */
    bool        m_Defined;     /* FALSE si non defini */
    wxString    m_SpecialDescr;

public:
    D_CODE( int num_dcode );
    ~D_CODE();
    void Clear_D_CODE_Data();

    void AppendParam( double aValue )
    {
        DCODE_PARAM param;

        param.SetValue( aValue );

        m_am_params.push_back( param );
    }

    void SetMacro( APERTURE_MACRO* aMacro )
    {
        m_Macro = aMacro;
    }
    APERTURE_MACRO* GetMacro() { return m_Macro; }

    /**
     * Function ShowApertureType
     * returns a character string telling what type of aperture type \a aType is.
     * @param aType The aperture type to show.
     */
    static const wxChar* ShowApertureType( APERTURE_T aType );
};


inline double DCODE_PARAM::GetValue( const D_CODE* aDcode ) const
{
    if( IsImmediate() )
        return value;
    else
    {
        // get the parameter from aDcode
        return 0.0;
    }
}


/**
 * Class GERBER
 * holds the data for one gerber file or layer
 */
class GERBER
{
    D_CODE*       m_Aperture_List[MAX_TOOLS];                   ///< Dcode (Aperture) List for this layer
    bool          m_Exposure;                                   ///< whether an aperture macro tool is flashed on or off


public:
    wxString      m_FileName;                                   // Full File Name for this layer
    wxString      m_Name;                                       // Layer name
    int           m_Layer;                                      // Layer Number
    bool          m_LayerNegative;                              // TRUE = Negative Layer
    bool          m_GerbMetric;                                 // FALSE = Inches, TRUE = metric
    bool          m_Relative;                                   // FALSE = absolute Coord, RUE = relative Coord
    bool          m_NoTrailingZeros;                            // True: zeros a droite supprimés
    bool          m_MirorA;                                     // True: miror / axe A (X)
    bool          m_MirorB;                                     // True: miror / axe B (Y)
    bool          m_Has_DCode;                                  // TRUE = DCodes in file (FALSE = no DCode->
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

    FILE*         m_Current_File;                               // Current file to read
    FILE*         m_FilesList[12];                              // Files list
    int           m_FilesPtr;                                   // Stack pointer for files list

    int           m_Selected_Tool;                              // Pour editions: Tool (Dcode) selectionné

    int           m_Transform[2][2];                            // The rotation/mirror transformation matrix.
    bool          m_360Arc_enbl;                                // Enbl 360 deg circular interpolation
    bool          m_PolygonFillMode;                            // Enbl polygon mode (read coord as a polygone descr)
    int           m_PolygonFillModeState;                       // In polygon mode: 0 = first segm, 1 = next segm

    APERTURE_MACRO_SET   m_aperture_macros;                     ///< a collection of APERTURE_MACROS, sorted by name

public:
    GERBER( int layer );
    ~GERBER();
    void    Clear_GERBER();
    int     ReturnUsedDcodeNumber();
    void    ResetDefaultValues();


    /**
     * Function InitToolTable
     */
    void    InitToolTable();

    // Routines utilisées en lecture de ficher gerber
    wxPoint ReadXYCoord( char*& Text );
    wxPoint ReadIJCoord( char*& Text );
    int     ReturnGCodeNumber( char*& Text );
    int     ReturnDCodeNumber( char*& Text );
    bool    Execute_G_Command( char*& text, int G_commande );
    bool    Execute_DCODE_Command( WinEDA_GerberFrame* frame, wxDC* DC,
                                   char*& text, int D_commande );

   /**
    * size of single line of a text line from a gerber file.
    * warning: some files can have very long lines, so the buffer must be large
    */
#define GERBER_BUFZ     4000

    /**
     * Function ReadRS274XCommand
     * reads a single RS274X command terminated with a %
     */
    bool    ReadRS274XCommand( WinEDA_GerberFrame* frame, wxDC* DC,
                               char aBuff[GERBER_BUFZ], char*& text );

    /**
     * Function ExecuteRS274XCommand
     * executes 1 commande
     */
    bool    ExecuteRS274XCommand( int command, char aBuff[GERBER_BUFZ], char*& text );


    /**
     * Function ReadApertureMacro
     * reads in an aperture macro and saves it in m_aperture_macros.
     * @param aBuff a character buffer at least GERBER_BUFZ long that can be
     *          used to read successive lines from the gerber file.
     * @param text A reference to a character pointer which gives the initial text to read from.
     * @param gerber_file Which file to read from for continuation.
     * @return bool - true if a macro was read in successfully, else false.
     */
    bool    ReadApertureMacro( char aBuff[GERBER_BUFZ], char*& text, FILE* gerber_file );


    /**
     * Function GetDCODE
     * returns a pointer to the D_CODE within this GERBER for the given
     * \a aDCODE.
     * @param aDCODE The numeric value of the D_CODE to look up.
     * @param createIfNoExist If true, then create the D_CODE if it does not exist.
     * @return D_CODE* - the one implied by the given \a aDCODE, or NULL
     *            if the requested \a aDCODE is out of range.
     */
    D_CODE* GetDCODE( int aDCODE, bool createIfNoExist=true );

    /**
     * Function FindApertureMacro
     * looks up a previously read in aperture macro.
     * @param aLookup A dummy APERTURE_MACRO with [only] the name field set.
     * @return APERTURE_MACRO* - the one with a matching name, or NULL if not found.
     */
    APERTURE_MACRO* FindApertureMacro( const APERTURE_MACRO& aLookup );
};



/**************/
/* rs274x.cpp */
/**************/
bool GetEndOfBlock( char buff[GERBER_BUFZ], char*& text, FILE* gerber_file );


eda_global GERBER* g_GERBER_List[32];


/**
 * How to show filled polygons :
 * 0 = filled
 * 1 = Sketch mode
 */
eda_global int           g_DisplayPolygonsModeSketch;


#include "pcbnew.h"

#endif  // ifndef GERBVIEW_H
