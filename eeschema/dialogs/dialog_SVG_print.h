
#ifndef _DIALOG_SVG_PRINT_H_
#define _DIALOG_SVG_PRINT_H_


class EDA_DRAW_FRAME;


#include "dialog_SVG_print_base.h"


class DIALOG_SVG_PRINT : public DIALOG_SVG_PRINT_base
{
private:
    EDA_DRAW_FRAME* m_Parent;
    wxConfig*       m_Config;

public:
    DIALOG_SVG_PRINT( EDA_DRAW_FRAME* parent );
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
    static bool DrawSVGPage( EDA_DRAW_FRAME* frame,
                             const wxString& FullFileName, SCH_SCREEN* screen,
                             bool aPrintBlackAndWhite = false,
                             bool aPrint_Sheet_Ref = false );
};


#endif    // _DIALOG_SVG_PRINT_H_
