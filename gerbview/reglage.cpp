/***************/
/* reglage.cpp */
/***************/

/*
 * Options for file extensions
 */


#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"


enum
{
    ID_SAVE_CFG = 1000
};


class WinEDA_ConfigFrame : public wxDialog
{
private:

    WinEDA_GerberFrame* m_Parent;
    wxListBox*          ListLibr;
    int LibModified;

    WinEDA_EnterText*   TextDrillExt;
    WinEDA_EnterText*   TextPhotoExt;
    WinEDA_EnterText*   TextPenExt;

public:
    WinEDA_ConfigFrame( WinEDA_GerberFrame* parent, const wxPoint& pos );
    ~WinEDA_ConfigFrame() { };

private:
    void    SaveCfg( wxCommandEvent& event );
    void    OnOkClick( wxCommandEvent& event );
    void    OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE( WinEDA_ConfigFrame, wxDialog )
    EVT_BUTTON( ID_SAVE_CFG, WinEDA_ConfigFrame::SaveCfg )
    EVT_BUTTON( wxID_OK, WinEDA_ConfigFrame::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, WinEDA_ConfigFrame::OnCancelClick )
END_EVENT_TABLE()


/** Function InstallConfigFrame
 * install the dialog box to configure some gerbview options
 * mainly the default file extensions
 */
void WinEDA_GerberFrame::InstallConfigFrame( const wxPoint& pos )
{
    WinEDA_ConfigFrame* CfgFrame = new WinEDA_ConfigFrame( this, pos );

    CfgFrame->ShowModal();
    CfgFrame->Destroy();
}


WinEDA_ConfigFrame::WinEDA_ConfigFrame( WinEDA_GerberFrame* parent,
                                        const wxPoint&      framepos ) :
    wxDialog( parent, -1, wxEmptyString, framepos, wxSize( 300, 180 ),
              wxDEFAULT_DIALOG_STYLE | wxFRAME_FLOAT_ON_PARENT )
{
    const int LEN_EXT = 100;
    wxString  title;

    m_Parent = parent;

    /* Shows the config filename currently used : */
    title = _( "from " ) + wxGetApp().m_CurrentOptionFile;
    SetTitle( title );

    LibModified = FALSE;
    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    /* Created the buttons */
    wxButton* Button = new wxButton( this, ID_SAVE_CFG, _( "Save Cfg..." ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    // Provide a spacer to improve appearance of dialog box
    RightBoxSizer->AddSpacer( 20 );

    Button = new wxButton( this, wxID_OK, _( "OK" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    wxSize size;
    size.x = LEN_EXT;
    size.y = -1;
    TextDrillExt = new WinEDA_EnterText( this,
                                         _( "Drill File Ext:" ),
                                         g_DrillFilenameExt,
                                         LeftBoxSizer, size );

    TextPhotoExt = new WinEDA_EnterText( this,
                                         _( "Gerber File Ext:" ),
                                         g_PhotoFilenameExt,
                                         LeftBoxSizer, size );

    TextPenExt = new WinEDA_EnterText( this,
                                       _( "D code File Ext:" ),
                                       g_PenFilenameExt,
                                       LeftBoxSizer, size );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void WinEDA_ConfigFrame::OnOkClick( wxCommandEvent& WXUNUSED (event) )
{
    g_DrillFilenameExt = TextDrillExt->GetValue();
    g_PhotoFilenameExt = TextPhotoExt->GetValue();
    g_PenFilenameExt   = TextPenExt->GetValue();

    EndModal( 1 );
}


void WinEDA_ConfigFrame::OnCancelClick( wxCommandEvent& WXUNUSED (event) )
{
    EndModal( -1 );
}


void WinEDA_ConfigFrame::SaveCfg( wxCommandEvent& event )
{
    m_Parent->Update_config();
}
