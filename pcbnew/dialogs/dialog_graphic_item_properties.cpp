/**************************************/
/* dialog_graphic_item_properties.cpp */
/**************************************/

/* Edit parameters values of graphic items type DRAWSEGMENTS:
 * Lines
 * Circles
 * Arcs
 * used as graphic elements found on non copper layers in boards
 * items on edge layers are considered as graphic items
 * Pcb texts are not always graphic items and are not handled here
 */
#include <fctsys.h>
#include <macros.h>
#include <gr_basic.h>
#include <confirm.h>
#include <class_drawpanel.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <class_board_design_settings.h>

#include <class_board.h>
#include <class_drawsegment.h>

#include <dialog_graphic_item_properties_base.h>

class DIALOG_GRAPHIC_ITEM_PROPERTIES: public DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE
{
private:
    PCB_EDIT_FRAME* m_parent;
    wxDC* m_DC;
    DRAWSEGMENT* m_Item;
    BOARD_DESIGN_SETTINGS  m_brdSettings;

public:
    DIALOG_GRAPHIC_ITEM_PROPERTIES( PCB_EDIT_FRAME* aParent, DRAWSEGMENT * aItem, wxDC * aDC);
    ~DIALOG_GRAPHIC_ITEM_PROPERTIES() {};

private:
    void initDlg( );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnLayerChoice( wxCommandEvent& event );
};

DIALOG_GRAPHIC_ITEM_PROPERTIES::DIALOG_GRAPHIC_ITEM_PROPERTIES( PCB_EDIT_FRAME* aParent,
                                                          DRAWSEGMENT * aItem, wxDC * aDC):
    DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE( aParent )
{
    m_parent = aParent;
    m_DC = aDC;
    m_Item = aItem;
    m_brdSettings = m_parent->GetDesignSettings();
    initDlg();
    Layout();
    GetSizer()->SetSizeHints( this );
    Centre();
}


/*******************************************************************************************/
void PCB_EDIT_FRAME::InstallGraphicItemPropertiesDialog(DRAWSEGMENT * aItem, wxDC* aDC)
/*******************************************************************************************/
{
    if ( aItem == NULL )
    {
        DisplayError(this, wxT("InstallGraphicItemPropertiesDialog() error: NULL item"));
        return;
    }

    m_canvas->SetIgnoreMouseEvents( true );
    DIALOG_GRAPHIC_ITEM_PROPERTIES* dialog = new DIALOG_GRAPHIC_ITEM_PROPERTIES( this, aItem, aDC );
    dialog->ShowModal();
    dialog->Destroy();
    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );
}

/**************************************************************************/
void DIALOG_GRAPHIC_ITEM_PROPERTIES::initDlg( )
/**************************************************************************/
/* Initialize messages and values in text control,
 * according to the item parameters values
*/
{
    SetFocus();
    m_StandardButtonsSizerOK->SetDefault();

    // Set unit symbol
    wxStaticText * texts_unit[] =
    {
        m_StartPointXUnit,
        m_StartPointYUnit,
        m_EndPointXUnit,
        m_EndPointYUnit,
        m_ThicknessTextUnit,
        m_DefaulThicknessTextUnit,
        NULL
    };

    for( int ii = 0; ; ii++ )
    {
        if( texts_unit[ii] == NULL )
            break;
        texts_unit[ii]->SetLabel( GetAbbreviatedUnitsLabel() );
    }

    wxString msg;

    // Change texts according to the segment shape:
    switch ( m_Item->GetShape() )
    {
    case S_CIRCLE:
        m_StartPointXLabel->SetLabel(_("Center X"));
        m_StartPointYLabel->SetLabel(_("Center Y"));
        m_EndPointXLabel->SetLabel(_("Point X"));
        m_EndPointYLabel->SetLabel(_("Point Y"));
        m_Angle_Text->Show(false);
        m_Angle_Ctrl->Show(false);
        m_AngleUnit->Show(false);
        break;

    case S_ARC:
        m_StartPointXLabel->SetLabel(_("Center X"));
        m_StartPointYLabel->SetLabel(_("Center Y"));
        m_EndPointXLabel->SetLabel(_("Start Point X"));
        m_EndPointYLabel->SetLabel(_("Start Point Y"));
        msg << m_Item->GetAngle();
        m_Angle_Ctrl->SetValue(msg);
        break;

    default:
        m_Angle_Text->Show(false);
        m_Angle_Ctrl->Show(false);
        m_AngleUnit->Show(false);
        break;
    }

    PutValueInLocalUnits( *m_Center_StartXCtrl, m_Item->GetStart().x,
        m_parent->GetInternalUnits() );

    PutValueInLocalUnits( *m_Center_StartYCtrl, m_Item->GetStart().y,
        m_parent->GetInternalUnits() );

    PutValueInLocalUnits( *m_EndX_Radius_Ctrl, m_Item->GetEnd().x,
        m_parent->GetInternalUnits() );

    PutValueInLocalUnits( *m_EndY_Ctrl, m_Item->GetEnd().y,
        m_parent->GetInternalUnits() );

    PutValueInLocalUnits( *m_ThicknessCtrl, m_Item->GetWidth(),
        m_parent->GetInternalUnits() );

    int thickness;

    if( m_Item->GetLayer() == EDGE_N )
        thickness =  m_brdSettings.m_EdgeSegmentWidth;
    else
        thickness =  m_brdSettings.m_DrawSegmentWidth;

    PutValueInLocalUnits( *m_DefaultThicknessCtrl, thickness,
        m_parent->GetInternalUnits() );

    for( int layer=FIRST_NO_COPPER_LAYER; layer <= LAST_NO_COPPER_LAYER;  ++layer )
    {
        m_LayerSelectionCtrl->Append( m_parent->GetBoard()->GetLayerName( layer ) );
    }

    int layer =  m_Item->GetLayer();
    // Control:
    if ( layer < FIRST_NO_COPPER_LAYER )
        layer = FIRST_NO_COPPER_LAYER;
    if ( layer > LAST_NO_COPPER_LAYER )
        layer = LAST_NO_COPPER_LAYER;
    m_LayerSelectionCtrl->SetSelection( layer - FIRST_NO_COPPER_LAYER );
}


/*******************************************************************/
void DIALOG_GRAPHIC_ITEM_PROPERTIES::OnLayerChoice( wxCommandEvent& event )
/*******************************************************************/
{
    int thickness;

    if( (m_LayerSelectionCtrl->GetCurrentSelection() + FIRST_NO_COPPER_LAYER) == EDGE_N )
        thickness =  m_brdSettings.m_EdgeSegmentWidth;
    else
        thickness =  m_brdSettings.m_DrawSegmentWidth;

    PutValueInLocalUnits( *m_DefaultThicknessCtrl, thickness,
        m_parent->GetInternalUnits() );
}

/*******************************************************************/
void DIALOG_GRAPHIC_ITEM_PROPERTIES::OnOkClick( wxCommandEvent& event )
/*******************************************************************/
/* Copy values in text control to the item parameters
*/
{
    m_parent->SaveCopyInUndoList( m_Item, UR_CHANGED );

    wxString msg;

    if( m_DC )
        m_Item->Draw( m_parent->GetCanvas(), m_DC, GR_XOR );

    msg = m_Center_StartXCtrl->GetValue();
    m_Item->SetStartX( ReturnValueFromString( g_UserUnit, msg, m_parent->GetInternalUnits() ));

    msg = m_Center_StartYCtrl->GetValue();
    m_Item->SetStartY( ReturnValueFromString( g_UserUnit, msg, m_parent->GetInternalUnits() ));

    msg = m_EndX_Radius_Ctrl->GetValue();
    m_Item->SetEndX( ReturnValueFromString( g_UserUnit, msg, m_parent->GetInternalUnits() ));

    msg = m_EndY_Ctrl->GetValue();
    m_Item->SetEndY( ReturnValueFromString( g_UserUnit, msg, m_parent->GetInternalUnits() ));

    msg = m_ThicknessCtrl->GetValue();
    m_Item->SetWidth( ReturnValueFromString( g_UserUnit, msg, m_parent->GetInternalUnits() ));

    msg = m_DefaultThicknessCtrl->GetValue();
    int thickness = ReturnValueFromString( g_UserUnit, msg, m_parent->GetInternalUnits() );

    m_Item->SetLayer( m_LayerSelectionCtrl->GetCurrentSelection() + FIRST_NO_COPPER_LAYER);

    if( m_Item->GetLayer() == EDGE_N )
         m_brdSettings.m_EdgeSegmentWidth = thickness;
    else
         m_brdSettings.m_DrawSegmentWidth = thickness;

    if( m_Item->GetShape() == S_ARC )
    {
        double angle;
        m_Angle_Ctrl->GetValue().ToDouble( &angle );
        NORMALIZE_ANGLE_360(angle);
        m_Item->SetAngle( angle );
    }

    m_parent->OnModify();

    if( m_DC )
        m_Item->Draw( m_parent->GetCanvas(), m_DC, GR_OR );

    m_Item->DisplayInfo( m_parent );

    m_parent->SetDesignSettings( m_brdSettings );

    Close( true );
}

void DIALOG_GRAPHIC_ITEM_PROPERTIES::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}

