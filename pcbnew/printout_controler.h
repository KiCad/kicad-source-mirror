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
    int    m_PenDefaultSize;                    // The defAUlt value pen size to plot/print items
                                                // that have no defined pen size
    double m_PrintScale;                        // general scale when printing
    double m_XScaleAdjust, m_YScaleAdjust;      // fine scale adjust for X and Y axis
    bool   m_Print_Sheet_Ref;                   // Option: pring page references
    long   m_PrintMaskLayer;                    // Layers to print
    bool   m_PrintMirror;                       // Option: Print mirroed
    bool   m_Print_Black_and_White;             // Option: Print in B&W ou Color
    int    m_OptionPrintPage;                   // Option: 0 = a layer per page, 1 = all layers at once
    int    m_PageCount;                         // Number of page to print
    bool   m_ForceCentered;                     // Forge plot origin to page centre (used in modedit)
    int    m_Flags;                             // auxiliary variable: can be used to pass some other info
    wxPageSetupDialogData* m_PageSetupData;     // A wxPageSetupDialogData to know page options (margins)

    enum DrillShapeOptT {
        NO_DRILL_SHAPE    = 0,
        SMALL_DRILL_SHAPE = 1,
        FULL_DRILL_SHAPE  = 2
    };
    DrillShapeOptT m_DrillShapeOpt;               // Options to print pads and vias holes

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
    EDA_DRAW_FRAME* m_Parent;
    PRINT_PARAMETERS m_PrintParams;

public:
    BOARD_PRINTOUT_CONTROLER( const PRINT_PARAMETERS& print_params,
                              EDA_DRAW_FRAME* parent,
                              const wxString&   title );

    bool OnPrintPage( int page );

    bool HasPage( int page )       // do not test page num
    {
        if (page <= m_PrintParams.m_PageCount)
            return true;
        else
            return false;
    }

    void GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo );

    void DrawPage();
};

#endif      // ifndef PRINTOUT_CONTROLER_H
