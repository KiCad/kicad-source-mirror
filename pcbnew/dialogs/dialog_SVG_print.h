
#ifndef _DIALOG_SVG_PRINT_H_
#define _DIALOG_SVG_PRINT_H_


#include <dialog_SVG_print_base.h>


class BASE_SCREEN;
class PCB_BASE_FRAME;
class wxConfig;


class DIALOG_SVG_PRINT : public DIALOG_SVG_PRINT_base
{
private:
    PCB_BASE_FRAME* m_parent;
    BOARD*          m_board;
    wxConfig*       m_config;
    long            m_printMaskLayer;
    wxCheckBox*     m_boxSelectLayer[32];
    bool            m_printBW;
    wxString        m_outputDirectory;

    // Static member to store options
    static bool     m_printMirror;
    static bool     m_oneFileOnly;

public:
    DIALOG_SVG_PRINT( EDA_DRAW_FRAME* parent );
    ~DIALOG_SVG_PRINT() {}

private:
    void OnCloseWindow( wxCloseEvent& event );
    void initDialog( );
    void OnButtonPlot( wxCommandEvent& event );
    void OnButtonCancelClick( wxCommandEvent& event );
    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event );
    void SetPenWidth();
    void ExportSVGFile( bool aOnlyOneFile );
    bool PageIsBoardBoundarySize()
    {
        return m_rbSvgPageSizeOpt->GetSelection() == 2;
    }
    bool PrintPageRef()
    {
        return m_rbSvgPageSizeOpt->GetSelection() == 0;
    }
    bool CreateSVGFile( const wxString& FullFileName );
};


#endif    // _DIALOG_SVG_PRINT_H_
