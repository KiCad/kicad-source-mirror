
#ifndef _DIALOG_SVG_PRINT_H_
#define _DIALOG_SVG_PRINT_H_


class WinEDA_DrawFrame;
class BASE_SCREEN;


#include "dialog_SVG_print_base.h"


class DIALOG_SVG_PRINT : public DIALOG_SVG_PRINT_base
{
private:
    WinEDA_DrawFrame*   m_Parent;
    wxConfig*           m_Config;

public:
    DIALOG_SVG_PRINT( WinEDA_DrawFrame* parent );
    ~DIALOG_SVG_PRINT() {}

private:
    void        OnCloseWindow( wxCloseEvent& event );
    void        OnInitDialog( wxInitDialogEvent& event );
    void        OnButtonPlotCurrentClick( wxCommandEvent& event );
    void        OnButtonPlotAllClick( wxCommandEvent& event );
    void        OnButtonCancelClick( wxCommandEvent& event );
    void        OnSetColorModeSelected( wxCommandEvent& event );
    void        SetPenWidth();
    void        PrintSVGDoc( bool aPrintAll, bool aPrint_Sheet_Ref );

public:
    static bool DrawSVGPage( WinEDA_DrawFrame* frame,
                             const wxString& FullFileName, BASE_SCREEN* screen,
                             bool aPrintBlackAndWhite = false,
                             bool aPrint_Sheet_Ref = false );
};


#endif    // _DIALOG_SVG_PRINT_H_
