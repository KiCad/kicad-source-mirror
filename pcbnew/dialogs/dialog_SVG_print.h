
#ifndef _DIALOG_SVG_PRINT_H_
#define _DIALOG_SVG_PRINT_H_


#include <dialog_SVG_print_base.h>


class BASE_SCREEN;
class PCB_BASE_FRAME;
class wxConfig;


class DIALOG_SVG_PRINT : public DIALOG_SVG_PRINT_base
{
private:
    PCB_BASE_FRAME*  m_Parent;
    wxConfig*        m_Config;
    long             m_PrintMaskLayer;
    wxCheckBox*      m_BoxSelectLayer[32];

public:
    DIALOG_SVG_PRINT( EDA_DRAW_FRAME* parent );
    ~DIALOG_SVG_PRINT() {}

private:
    void OnCloseWindow( wxCloseEvent& event );
    void initDialog( );
    void OnButtonPrintSelectedClick( wxCommandEvent& event );
    void OnButtonPrintBoardClick( wxCommandEvent& event );
    void OnButtonCancelClick( wxCommandEvent& event );
    void OnSetColorModeSelected( wxCommandEvent& event );
    void SetPenWidth();
    void PrintSVGDoc( bool aPrintAll, bool aPrint_Frame_Ref );
    bool DrawPage( const wxString& FullFileName, BASE_SCREEN* screen, bool aPrint_Frame_Ref );
};


#endif    // _DIALOG_SVG_PRINT_H_
