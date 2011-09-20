/**
 * @file dimension.cpp
 * @brief Dialog and code for editing a dimension object.
 */

#include "fctsys.h"
#include "confirm.h"
#include "class_drawpanel.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"
#include "drawtxt.h"
#include "dialog_helpers.h"

/* Local functions */
static void MoveDimension( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                           const wxPoint& aPosition, bool aErase );

/* Local variables : */
static int status_dimension; /* Used in dimension creation:
                              * = 0 : initial value: no dimension in progress
                              * = 1 : First point created
                              * = 2 : Second point created, the text must be placed */

/*
 *  A dimension has this shape:
 *  It has 2 reference points, and a text
 * |            |
 * |    dist    |
 * |<---------->|
 * |            |
 *
 */


/*********************************/
/* class DIMENSION_EDITOR_DIALOG */
/*********************************/

class DIMENSION_EDITOR_DIALOG : public wxDialog
{
private:

    PCB_EDIT_FRAME* m_Parent;
    wxDC*           m_DC;
    DIMENSION*      CurrentDimension;
    wxTextCtrl*     m_Name;
    EDA_SIZE_CTRL*  m_TxtSizeCtrl;
    EDA_VALUE_CTRL* m_TxtWidthCtrl;
    wxRadioBox*     m_Mirror;
    wxComboBox*     m_SelLayerBox;

public:

    // Constructor and destructor
    DIMENSION_EDITOR_DIALOG( PCB_EDIT_FRAME* aParent, DIMENSION* aDimension, wxDC* aDC );
    ~DIMENSION_EDITOR_DIALOG()
    {
    }


private:
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( DIMENSION_EDITOR_DIALOG, wxDialog )
    EVT_BUTTON( wxID_OK, DIMENSION_EDITOR_DIALOG::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, DIMENSION_EDITOR_DIALOG::OnCancelClick )
END_EVENT_TABLE()


DIMENSION_EDITOR_DIALOG::DIMENSION_EDITOR_DIALOG( PCB_EDIT_FRAME* aParent,
                                                  DIMENSION* aDimension, wxDC* aDC ) :
    wxDialog( aParent, -1,  _( "Dimension Properties" ) )
{
    wxButton* Button;

    m_Parent = aParent;
    m_DC = aDC;
    Centre();

    CurrentDimension = aDimension;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    /* Create command buttons. */
    Button = new wxButton( this, wxID_OK, _( "OK" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    wxString display_msg[2] = { _( "Normal" ), _( "Mirror" ) };
    m_Mirror = new wxRadioBox( this, -1, _( "Display" ),
                               wxDefaultPosition, wxSize( -1, -1 ), 2, display_msg,
                               1, wxRA_SPECIFY_COLS );

    if( aDimension->m_Text->m_Mirror )
        m_Mirror->SetSelection( 1 );

    RightBoxSizer->Add( m_Mirror, 0, wxGROW | wxALL, 5 );

    LeftBoxSizer->Add( new wxStaticText( this, -1, _( "Text:" ) ),
                       0, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 5 );

    m_Name = new wxTextCtrl( this, -1, aDimension->m_Text->m_Text,
                             wxDefaultPosition, wxSize( 200, -1 ) );

    m_Name->SetInsertionPoint( 1 );

    LeftBoxSizer->Add( m_Name,
                       0,
                       wxGROW | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM,
                       5 );

    m_TxtSizeCtrl = new EDA_SIZE_CTRL( this, _( "Size" ), aDimension->m_Text->m_Size,
                                       g_UserUnit, LeftBoxSizer, m_Parent->m_InternalUnits );

    m_TxtWidthCtrl = new EDA_VALUE_CTRL( this, _( "Width" ), aDimension->m_Width,
                                         g_UserUnit, LeftBoxSizer, m_Parent->m_InternalUnits );

    wxStaticText* text = new wxStaticText( this, -1, _( "Layer:" ) );
    LeftBoxSizer->Add( text, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );
    m_SelLayerBox = new wxComboBox( this, wxID_ANY, wxEmptyString,
                                    wxDefaultPosition, wxDefaultSize,
                                    0, NULL, wxCB_READONLY );
    LeftBoxSizer->Add( m_SelLayerBox, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    for( int layer = FIRST_NO_COPPER_LAYER;  layer<NB_LAYERS;  layer++ )
    {
        m_SelLayerBox->Append( aParent->GetBoard()->GetLayerName( layer ) );
    }

    m_SelLayerBox->SetSelection( aDimension->GetLayer() - FIRST_NO_COPPER_LAYER );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void DIMENSION_EDITOR_DIALOG::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


void DIMENSION_EDITOR_DIALOG::OnOkClick( wxCommandEvent& event )
{
    if( m_DC )     // Delete old text.
    {
        CurrentDimension->Draw( m_Parent->DrawPanel, m_DC, GR_XOR );
    }

    m_Parent->SaveCopyInUndoList(CurrentDimension, UR_CHANGED);

    if( m_Name->GetValue() != wxEmptyString )
    {
        CurrentDimension->SetText( m_Name->GetValue() );
    }

    CurrentDimension->m_Text->m_Size  = m_TxtSizeCtrl->GetValue();

    int width = m_TxtWidthCtrl->GetValue();
    int maxthickness = Clamp_Text_PenSize( width, CurrentDimension->m_Text->m_Size );

    if( width > maxthickness )
    {
        DisplayError( NULL,
                      _( "The text thickness is too large for the text size. It will be clamped") );
        width = maxthickness;
    }

    CurrentDimension->m_Text->m_Thickness = CurrentDimension->m_Width = width ;

    CurrentDimension->m_Text->m_Mirror = ( m_Mirror->GetSelection() == 1 ) ? true : false;

    CurrentDimension->SetLayer( m_SelLayerBox->GetCurrentSelection() + FIRST_NO_COPPER_LAYER );

    CurrentDimension->AdjustDimensionDetails( true );

    if( m_DC )     // Display new text
    {
        CurrentDimension->Draw( m_Parent->DrawPanel, m_DC, GR_OR );
    }

    m_Parent->OnModify();
    EndModal( 1 );
}


static void AbortMoveDimension( EDA_DRAW_PANEL* Panel, wxDC* aDC )
{
    DIMENSION* Dimension = (DIMENSION*) Panel->GetScreen()->GetCurItem();

    if( Dimension )
    {
        if( Dimension->IsNew() )
        {
            Dimension->Draw( Panel, aDC, GR_XOR );
            Dimension->DeleteStructure();
        }
        else
        {
            Dimension->Draw( Panel, aDC, GR_OR );
        }
    }

    status_dimension = 0;
    ((PCB_EDIT_FRAME*)Panel->GetParent())->SetCurItem( NULL );
}


DIMENSION* PCB_EDIT_FRAME::EditDimension( DIMENSION* aDimension, wxDC* aDC )
{
    wxPoint pos;

    if( aDimension == NULL )
    {
        status_dimension = 1;
        pos = GetScreen()->GetCrossHairPosition();

        aDimension = new DIMENSION( GetBoard() );
        aDimension->m_Flags = IS_NEW;

        aDimension->SetLayer( getActiveLayer() );

        aDimension->m_crossBarOx = aDimension->m_crossBarFx = pos.x;
        aDimension->m_crossBarOy = aDimension->m_crossBarFy = pos.y;

        aDimension->m_featureLineDOx = aDimension->m_featureLineDFx = pos.x;
        aDimension->m_featureLineDOy = aDimension->m_featureLineDFy = pos.y;

        aDimension->m_featureLineGOx = aDimension->m_featureLineGFx = pos.x;
        aDimension->m_featureLineGOy = aDimension->m_featureLineGFy = pos.y;

        aDimension->m_arrowG1Ox = aDimension->m_arrowG1Fx = pos.x;
        aDimension->m_arrowG1Oy = aDimension->m_arrowG1Fy = pos.y;

        aDimension->m_arrowG2Ox = aDimension->m_arrowG2Fx = pos.x;
        aDimension->m_arrowG2Oy = aDimension->m_arrowG2Fy = pos.y;

        aDimension->m_arrowD1Ox = aDimension->m_arrowD1Fx = pos.x;
        aDimension->m_arrowD1Oy = aDimension->m_arrowD1Fy = pos.y;

        aDimension->m_arrowD2Ox = aDimension->m_arrowD2Fx = pos.x;
        aDimension->m_arrowD2Oy = aDimension->m_arrowD2Fy = pos.y;

        aDimension->m_Text->m_Size   = GetBoard()->GetBoardDesignSettings()->m_PcbTextSize;
        int width = GetBoard()->GetBoardDesignSettings()->m_PcbTextWidth;
        int maxthickness = Clamp_Text_PenSize(width, aDimension->m_Text->m_Size );

        if( width > maxthickness )
        {
            width = maxthickness;
        }

        aDimension->m_Text->m_Thickness = aDimension->m_Width = width ;

        aDimension->AdjustDimensionDetails( );

        aDimension->Draw( DrawPanel, aDC, GR_XOR );

        DrawPanel->SetMouseCapture( MoveDimension, AbortMoveDimension );
        return aDimension;
    }

    // Dimension != NULL
    if( status_dimension == 1 )
    {
        status_dimension = 2;
        return aDimension;
    }

    aDimension->Draw( DrawPanel, aDC, GR_OR );
    aDimension->m_Flags = 0;

    /* ADD this new item in list */
    GetBoard()->Add( aDimension );

    // Add store it in undo/redo list
    SaveCopyInUndoList( aDimension, UR_NEW );

    OnModify();
    DrawPanel->SetMouseCapture( NULL, NULL );

    return NULL;
}


static void MoveDimension( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                           const wxPoint& aPosition, bool aErase )
{
    PCB_SCREEN* screen   = (PCB_SCREEN*) aPanel->GetScreen();
    DIMENSION*  Dimension = (DIMENSION*) screen->GetCurItem();
    wxPoint     pos = screen->GetCrossHairPosition();

    if( Dimension == NULL )
        return;

    // Erase previous dimension.
    if( aErase )
    {
        Dimension->Draw( aPanel, aDC, GR_XOR );
    }

    Dimension->SetLayer( screen->m_Active_Layer );

    if( status_dimension == 1 )
    {
        Dimension->m_featureLineDOx = pos.x;
        Dimension->m_featureLineDOy = pos.y;
        Dimension->m_crossBarFx  = Dimension->m_featureLineDOx;
        Dimension->m_crossBarFy  = Dimension->m_featureLineDOy;
        Dimension->AdjustDimensionDetails( );
    }
    else
    {
        int   deltax, deltay, dx, dy;
        float angle, depl;
        deltax = Dimension->m_featureLineDOx - Dimension->m_featureLineGOx;
        deltay = Dimension->m_featureLineDOy - Dimension->m_featureLineGOy;

        /* Calculating the direction of travel perpendicular to the selected axis. */
        angle = atan2( (double)deltay, (double)deltax ) + (M_PI / 2);

        deltax = pos.x - Dimension->m_featureLineDOx;
        deltay = pos.y - Dimension->m_featureLineDOy;
        depl   = ( deltax * cos( angle ) ) + ( deltay * sin( angle ) );
        dx = (int) ( depl * cos( angle ) );
        dy = (int) ( depl * sin( angle ) );
        Dimension->m_crossBarOx = Dimension->m_featureLineGOx + dx;
        Dimension->m_crossBarOy = Dimension->m_featureLineGOy + dy;
        Dimension->m_crossBarFx = Dimension->m_featureLineDOx + dx;
        Dimension->m_crossBarFy = Dimension->m_featureLineDOy + dy;

        Dimension->AdjustDimensionDetails( );
    }

    Dimension->Draw( aPanel, aDC, GR_XOR );
}


void PCB_EDIT_FRAME::ShowDimensionPropertyDialog( DIMENSION* aDimension, wxDC* aDC )
{
    if( aDimension == NULL )
        return;

    DIMENSION_EDITOR_DIALOG* frame = new DIMENSION_EDITOR_DIALOG( this, aDimension, aDC );
    frame->ShowModal();
    frame->Destroy();
}


void PCB_EDIT_FRAME::DeleteDimension( DIMENSION* aDimension, wxDC* aDC )
{
    if( aDimension == NULL )
        return;

    if( aDC )
        aDimension->Draw( DrawPanel, aDC, GR_XOR );

    SaveCopyInUndoList( aDimension, UR_DELETED );
    aDimension->UnLink();
    OnModify();
}
