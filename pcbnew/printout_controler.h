/**************************/
/* printout_controler.h */
/**************************/

#ifndef PRINTOUT_CONTROLER_H
#define PRINTOUT_CONTROLER_H


#include <wx/dcps.h>

#define DEFAULT_ORIENTATION_PAPER wxLANDSCAPE   // other option is wxPORTRAIT


/**
 *  This class handle parameters used to draw (print) a board
 *  layers, scale and others options
 */

class PRINT_PARAMETERS
{
public:
    int    m_PenMinSize;                        // A minimal value pen size to plot/print items
    double m_PrintScale;                        // general scale when printing
    double m_XScaleAdjust, m_YScaleAdjust;      // fine scale adjust for X and Y axis
    bool   m_Print_Sheet_Ref;                   // Option: pring page references
    long   m_PrintMaskLayer;                    // Layers to print
    bool   m_PrintMirror;                       // Option: Print mirroed
    bool   m_Print_Black_and_White;             // Option: Print in B&W ou Color
    int    m_OptionPrintPage;                   // Option: 0 = a layer per page, all layers at once
    int    m_PageCount;                         // Nmuber of page to print
    bool   m_ForceCentered;                     // Forge plot origin to page centre (used in modedit)
    int    m_Flags;                             // auxiliary variable: can be used to pass some other info

public:
    PRINT_PARAMETERS();
};

/**
 *  This class derived from wxPrintout handle the necessary info
 *  to control a printer when printing a board
 */

class BOARD_PRINTOUT_CONTROLER : public wxPrintout
{
private:
    WinEDA_DrawFrame* m_Parent;
    PRINT_PARAMETERS m_PrintParams;

public:
    BOARD_PRINTOUT_CONTROLER( const PRINT_PARAMETERS& print_params,
                              WinEDA_DrawFrame* parent,
                              const wxString&   title );

    bool OnPrintPage( int page );

    bool HasPage( int page ) { return true; }       // do not test page num
    void GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo );

    void DrawPage();
};

#endif      // ifndef PRINTOUT_CONTROLER_H
