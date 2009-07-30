/***************************************************************************/
/* Dialog editor for text on copper and technical layers (TEXTE_PCB class) */
/***************************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "drawtxt.h"
#include "confirm.h"

enum id_TextPCB_properties {
    ID_TEXTPCB_SELECT_LAYER = 1900
};

/************************************/
/* class WinEDA_TextPCBPropertiesFrame */
/************************************/

class WinEDA_TextPCBPropertiesFrame : public wxDialog
{
private:

    WinEDA_PcbFrame*     m_Parent;
    wxDC*                m_DC;
    TEXTE_PCB*           CurrentTextPCB;
    WinEDA_EnterText*    m_Name;
    WinEDA_PositionCtrl* m_TxtPosCtrl;
    WinEDA_SizeCtrl*     m_TxtSizeCtrl;
    WinEDA_ValueCtrl*    m_TxtWidthCtlr;
    wxRadioBox*          m_Orient;
    wxRadioBox*          m_Mirror;
    wxRadioBox*          m_Style;
    WinEDAChoiceBox*     m_SelLayerBox;

public:

    // Constructor and destructor
    WinEDA_TextPCBPropertiesFrame( WinEDA_PcbFrame* parent,
                                   TEXTE_PCB* TextPCB, wxDC* DC );
    ~WinEDA_TextPCBPropertiesFrame()
    {
    }


private:
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_TextPCBPropertiesFrame, wxDialog )
    EVT_BUTTON( wxID_OK, WinEDA_TextPCBPropertiesFrame::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, WinEDA_TextPCBPropertiesFrame::OnCancelClick )
END_EVENT_TABLE()


void WinEDA_PcbFrame::InstallTextPCBOptionsFrame( TEXTE_PCB* TextPCB, wxDC* DC )
{
    DrawPanel->m_IgnoreMouseEvents = TRUE;
    WinEDA_TextPCBPropertiesFrame* frame =
        new WinEDA_TextPCBPropertiesFrame( this, TextPCB, DC );
    frame->ShowModal();
    frame->Destroy();
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;
}


WinEDA_TextPCBPropertiesFrame::WinEDA_TextPCBPropertiesFrame( WinEDA_PcbFrame* parent,
                                                              TEXTE_PCB* TextPCB,
                                                              wxDC* DC ) :
    wxDialog( parent, -1, _( "TextPCB properties" ), wxDefaultPosition,
              wxSize( 390, 340 ) )
{
    wxButton* Button;
    BOARD*    board = parent->GetBoard();

    m_Parent = parent;

    m_DC = DC;
    Centre();

    CurrentTextPCB = TextPCB;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* LeftBoxSizer   = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* MiddleBoxSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* RightBoxSizer  = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( MiddleBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );


    /* Creation des boutons de commande */
    Button = new wxButton( this, wxID_OK, _( "OK" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );
    Button->SetDefault();

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    m_Name = new WinEDA_EnterText( this, _( "Text:" ),
                                   TextPCB->m_Text,
                                   LeftBoxSizer, wxSize( 200, 60 ), true );
    m_Name->SetFocus();
    m_Name->SetSelection( -1, -1 );

    m_TxtSizeCtrl = new WinEDA_SizeCtrl( this, _( "Size" ),
                                         TextPCB->m_Size,
                                         g_UnitMetric, LeftBoxSizer,
                                         m_Parent->m_InternalUnits );

    m_TxtWidthCtlr = new WinEDA_ValueCtrl( this, _( "Width" ),
                                           TextPCB->m_Width,
                                           g_UnitMetric, LeftBoxSizer,
                                           m_Parent->m_InternalUnits );

    m_TxtPosCtrl = new WinEDA_PositionCtrl( this, _( "Position" ),
                                            TextPCB->m_Pos,
                                            g_UnitMetric, LeftBoxSizer,
                                            m_Parent->m_InternalUnits );

    m_SelLayerBox = new WinEDAChoiceBox( this, ID_TEXTPCB_SELECT_LAYER,
                                         wxDefaultPosition, wxDefaultSize );
    MiddleBoxSizer->Add( m_SelLayerBox, 0, wxGROW | wxALL, 5 );

    for( int layer = 0; layer<NB_LAYERS;  ++layer )
    {
        m_SelLayerBox->Append( board->GetLayerName( layer ) );
    }

    m_SelLayerBox->SetSelection( TextPCB->GetLayer() );


    static const wxString orient_msg[4] =
    {
        wxT( "0" ), wxT( "90" ), wxT( "180" ), wxT( "-90" )
    };

    m_Orient = new wxRadioBox( this, -1, _( "Orientation" ), wxDefaultPosition,
                               wxSize( -1, -1 ), 4, orient_msg,
                               1, wxRA_SPECIFY_COLS );
    MiddleBoxSizer->Add( m_Orient, 0, wxGROW | wxALL, 5 );

    switch( TextPCB->m_Orient )
    {
    default:
        m_Orient->SetSelection( 0 );
        break;

    case 900:
        m_Orient->SetSelection( 1 );
        break;

    case 1800:
        m_Orient->SetSelection( 2 );
        break;

    case 2700:
        m_Orient->SetSelection( 3 );
        break;
    }

    wxString display_msg[2] = { _( "Normal" ), _( "Mirror" ) };
    m_Mirror = new wxRadioBox( this, -1, _( "Display" ), wxDefaultPosition,
                               wxSize( -1, -1 ), 2, display_msg,
                               1, wxRA_SPECIFY_COLS );
    if( TextPCB->m_Mirror )
        m_Mirror->SetSelection( 1 );
    MiddleBoxSizer->Add( m_Mirror, 0, wxGROW | wxALL, 5 );

    int      style = 0;
    if( CurrentTextPCB->m_Italic )
        style = 1;
    wxString style_msg[] = { _( "Normal" ), _( "Italic" ) };
    m_Style = new wxRadioBox( this, -1, _( "Style" ), wxDefaultPosition,
                              wxSize( -1, -1 ), 2, style_msg,
                              1, wxRA_SPECIFY_COLS );
    m_Style->SetSelection( style );
    MiddleBoxSizer->Add( m_Style, 0, wxGROW | wxALL, 5 );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void WinEDA_TextPCBPropertiesFrame::OnCancelClick( wxCommandEvent& WXUNUSED( event ) )
{
    EndModal( wxID_CANCEL );
}


void WinEDA_TextPCBPropertiesFrame::OnOkClick( wxCommandEvent& event )
{
// test for acceptable values for parameters:
    wxSize newsize = m_TxtSizeCtrl->GetValue();

    if( newsize.x < TEXTS_MIN_SIZE )
        newsize.x = TEXTS_MIN_SIZE;
    if( newsize.y < TEXTS_MIN_SIZE )
        newsize.y = TEXTS_MIN_SIZE;
    if( newsize.x > TEXTS_MAX_WIDTH )
        newsize.x = TEXTS_MAX_WIDTH;
    if( newsize.y > TEXTS_MAX_WIDTH )
        newsize.y = TEXTS_MAX_WIDTH;

    if( m_DC )     // Erase old text on screen
    {
        CurrentTextPCB->Draw( m_Parent->DrawPanel, m_DC, GR_XOR );
    }

    if( !m_Name->GetValue().IsEmpty() )
    {
        CurrentTextPCB->m_Text = m_Name->GetValue();
    }
    CurrentTextPCB->m_Pos  = m_TxtPosCtrl->GetValue();
    CurrentTextPCB->m_Size = newsize;

    CurrentTextPCB->m_Width = m_TxtWidthCtlr->GetValue();

    // test for acceptable values for parameters:
    int maxthickness = Clamp_Text_PenSize( CurrentTextPCB->m_Width, CurrentTextPCB->m_Size  );
    if( CurrentTextPCB->m_Width > maxthickness )
    {
        DisplayError(this, _("The text thickness is too large for the text size. It will be clamped"));
        CurrentTextPCB->m_Width = maxthickness;
    }

    CurrentTextPCB->m_Mirror = (m_Mirror->GetSelection() == 1) ? true : false;
    CurrentTextPCB->m_Orient = m_Orient->GetSelection() * 900;
    CurrentTextPCB->SetLayer( m_SelLayerBox->GetChoice() );
    CurrentTextPCB->m_Italic = m_Style->GetSelection() ? 1 : 0;

    if( m_DC )     // Display new text
    {
        CurrentTextPCB->Draw( m_Parent->DrawPanel, m_DC, GR_OR );
    }
    m_Parent->GetScreen()->SetModify();
    EndModal( 1 );
}
