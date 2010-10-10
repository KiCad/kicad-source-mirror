/******************/
/* class_GERBER.h */
/******************/

#ifndef _CLASS_GERBER_H_
#define _CLASS_GERBER_H_

#include <vector>
#include <set>

#include "dcode.h"
#include "class_gerber_draw_item.h"
#include "class_aperture_macro.h"

class WinEDA_GerberFrame;
class BOARD;
class D_CODE;


/**
 * Class GERBER
 * holds the data for one gerber file or layer
 */
class GERBER
{
    WinEDA_GerberFrame* m_Parent;                           // the parent WinEDA_GerberFrame (used to display messages...)
    D_CODE*             m_Aperture_List[TOOLS_MAX_COUNT];   ///< Dcode (Aperture) List for this layer
    bool               m_Exposure;                          ///< whether an aperture macro tool is flashed on or off

    BOARD*             m_Pcb;

public:
    wxString           m_FileName;                                      // Full File Name for this layer
    wxString           m_ImageName;                                     // Image name, from IN <name>* command
    wxString           m_LayerName;                                     // Layer name, from LN <name>* command
    int                m_Layer;                                         // Layer Number
    bool               m_LayerNegative;                                 // true = Negative Layer
    bool               m_GerbMetric;                                    // false = Inches, true = metric
    bool               m_Relative;                                      // false = absolute Coord, true = relative Coord
    bool               m_NoTrailingZeros;                               // true: remove tailing zeros.
    bool               m_SwapAxis;                                      // false (default) if A = X and B = Y
                                                                        // true if A = Y, B = X
    bool               m_MirrorA;                                       // true: miror / axe A (X)
    bool               m_MirrorB;                                       // true: miror / axe B (Y)
    wxPoint            m_ImageOffset;                                   // Coord Offset, from IO command
    wxPoint            m_Offset;                                        // Coord Offset, from OF command
    wxSize             m_FmtScale;                                      // Fmt 2.3: m_FmtScale = 3, fmt 3.4: m_FmtScale = 4
    wxSize             m_FmtLen;                                        // Nb chars per coord. ex fmt 2.3, m_FmtLen = 5
    wxRealPoint        m_LayerScale;                                    // scale (X and Y) of layer.
    int                m_Rotation;                                      // Image rotation (0, 90, 180, 270
                                                                        // Note these values are stored in 0.1 degrees
    int                m_Iterpolation;                                  // Linear, 90 arc, Circ.
    bool               m_ImageNegative;                                 // true = Negative image
    int                m_Current_Tool;                                  // Current Tool (Dcode) number selected
    int                m_Last_Pen_Command;                              // Current or last pen state (0..9, set by Dn option with n <10
    int                m_CommandState;                                  // state of gerber analysis command.
    wxPoint            m_CurrentPos;                                    // current specified coord for plot
    wxPoint            m_PreviousPos;                                   // old current specified coord for plot
    wxPoint            m_IJPos;                                         // IJ coord (for arcs & circles )

    FILE*              m_Current_File;                                  // Current file to read
    #define            INCLUDE_FILES_CNT_MAX 10
    FILE*              m_FilesList[INCLUDE_FILES_CNT_MAX + 2];          // Included files list
    int                m_FilesPtr;                                      // Stack pointer for files list

    int                m_Selected_Tool;                                 // For hightlight: current selected Dcode
    bool               m_Has_DCode;                                     // true = DCodes in file
                                                                        // (false = no DCode -> separate DCode file
    bool               m_360Arc_enbl;                                   // Enbl 360 deg circular interpolation
    bool               m_PolygonFillMode;                               // Enable polygon mode (read coord as a polygon descr)
    int                m_PolygonFillModeState;                          // In polygon mode: 0 = first segm, 1 = next segm

    APERTURE_MACRO_SET m_aperture_macros;                               ///< a collection of APERTURE_MACROS, sorted by name

public:
    GERBER( WinEDA_GerberFrame* aParent, int layer );
    ~GERBER();
    void    Clear_GERBER();
    int     ReturnUsedDcodeNumber();
    void    ResetDefaultValues();

    /** function ReportMessage
     * Add a message (a string) in message list
     * for instance when reading a Gerber file
     * @param aMessage = the straing to add in list
     */
    void    ReportMessage( const wxString aMessage );

    /** function ClearMessageList
     * Clear the message list
     * Call it before reading a Gerber file
     */
    void    ClearMessageList();

    /**
     * Function InitToolTable
     */
    void    InitToolTable();

    /** function ReadXYCoord
     * Returns the current coordinate type pointed to by XnnYnn Text (XnnnnYmmmm)
     */
    wxPoint ReadXYCoord( char*& Text );

    /** function ReadIJCoord
     * Returns the current coordinate type pointed to by InnJnn Text (InnnnJmmmm)
     * These coordinates are relative, so if coordinate is absent, it's value
     * defaults to 0
     */
    wxPoint ReadIJCoord( char*& Text );

    // functions to read G commands or D commands:
    int     ReturnGCodeNumber( char*& Text );
    int     ReturnDCodeNumber( char*& Text );

    // functions to execute G commands or D commands:
    bool    Execute_G_Command( char*& text, int G_commande );
    bool    Execute_DCODE_Command( WinEDA_GerberFrame* frame,
                                   char*& text, int D_commande );

    /**
     * Function ReadRS274XCommand
     * reads a single RS274X command terminated with a %
     */
    bool ReadRS274XCommand( WinEDA_GerberFrame * frame,
                            char aBuff[GERBER_BUFZ], char* & text );

    /**
     * Function ExecuteRS274XCommand
     * executes 1 command
     */
    bool ExecuteRS274XCommand( int command, char aBuff[GERBER_BUFZ],
                               char* & text );


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
    bool ReadApertureMacro( char aBuff[GERBER_BUFZ], char* & text,
                            FILE * gerber_file );


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
    D_CODE*         GetDCODE( int aDCODE, bool createIfNoExist = true );

    /**
     * Function FindApertureMacro
     * looks up a previously read in aperture macro.
     * @param aLookup A dummy APERTURE_MACRO with [only] the name field set.
     * @return APERTURE_MACRO* - the one with a matching name, or NULL if
     *                           not found.
     */
    APERTURE_MACRO* FindApertureMacro( const APERTURE_MACRO& aLookup );
};


#endif  // ifndef _CLASS_GERBER_H_
