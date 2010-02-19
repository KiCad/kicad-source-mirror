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
#include "fctsys.h"
#include "macros.h"
#include "common.h"
#include "confirm.h"
#include "class_drawpanel.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"

#include "dialog_graphic_item_properties_base.h"

///////////////////////////////////////////////////////////////////////////

class DialogGraphicItemProperties: public DialogGraphicItemProperties_base
{
private:
    WinEDA_PcbFrame* m_Parent;
    wxDC* m_DC;
    DRAWSEGMENT* m_Item;
    BOARD_DESIGN_SETTINGS*  m_BrdSettings;

public:
    DialogGraphicItemProperties( WinEDA_PcbFrame* aParent, DRAWSEGMENT * aItem, wxDC * aDC);
    ~DialogGraphicItemProperties() {};

private:
    void Init( );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnLayerChoice( wxCommandEvent& event );
};

DialogGraphicItemProperties::DialogGraphicItemProperties( WinEDA_PcbFrame* aParent, DRAWSEGMENT * aItem, wxDC * aDC):
    DialogGraphicItemProperties_base( aParent )
{
    m_Parent = aParent;
    m_DC = aDC;
    m_Item = aItem;
    m_BrdSettings = m_Parent->GetBoard()->GetBoardDesignSettings();
    Init();
    Layout();
    GetSizer()->SetSizeHints( this );
}


/*******************************************************************************************/
void WinEDA_PcbFrame::InstallGraphicItemPropertiesDialog(DRAWSEGMENT * aItem, wxDC* aDC)
/*******************************************************************************************/
{
    if ( aItem == NULL )
    {
        DisplayError(this, wxT("InstallGraphicItemPropertiesDialog() error: NULL item"));
        return;
    }
    DrawPanel->m_IgnoreMouseEvents = TRUE;
    DialogGraphicItemProperties* dialog = new DialogGraphicItemProperties( this,
        aItem, aDC );
    dialog->ShowModal(); dialog->Destroy();
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;
}

/**************************************************************************/
void DialogGraphicItemProperties::Init( )
/**************************************************************************/
/* Initialize messages and values in text control,
 * according to the item parameters values
*/
{
    SetFocus();

    wxString msg;
    // Change texts according to the segment shape:
    switch ( m_Item->m_Shape )
    {
        case S_CIRCLE:
            m_Start_Center_XText->SetLabel(_("Center X"));
            m_Start_Center_YText->SetLabel(_("Center Y"));
            m_EndX_Radius_Text->SetLabel(_("Point X"));
            m_EndY_Text->SetLabel(_("Point Y"));
            m_Angle_Text->Show(false);
            m_Angle_Ctrl->Show(false);
            break;

        case S_ARC:
            m_Start_Center_XText->SetLabel(_("Center X"));
            m_Start_Center_YText->SetLabel(_("Center Y"));
            m_EndX_Radius_Text->SetLabel(_("Start Point X"));
            m_EndY_Text->SetLabel(_("Start Point Y"));
            msg << m_Item->m_Angle;
            m_Angle_Ctrl->SetValue(msg);
            break;

        default:
            m_Angle_Text->Show(false);
            m_Angle_Ctrl->Show(false);
            break;
    }
    AddUnitSymbol( *m_Start_Center_XText );

    PutValueInLocalUnits( *m_Center_StartXCtrl, m_Item->m_Start.x,
        m_Parent->m_InternalUnits );

    AddUnitSymbol( *m_Start_Center_YText );
    PutValueInLocalUnits( *m_Center_StartYCtrl, m_Item->m_Start.y,
        m_Parent->m_InternalUnits );

    AddUnitSymbol( *m_EndX_Radius_Text );
    PutValueInLocalUnits( *m_EndX_Radius_Ctrl, m_Item->m_End.x,
        m_Parent->m_InternalUnits );

    AddUnitSymbol( *m_EndY_Text );
    PutValueInLocalUnits( *m_EndY_Ctrl, m_Item->m_End.y,
        m_Parent->m_InternalUnits );

    AddUnitSymbol( *m_ItemThicknessText );
    PutValueInLocalUnits( *m_ThicknessCtrl, m_Item->m_Width,
        m_Parent->m_InternalUnits );

    AddUnitSymbol( *m_DefaultThicknessText );
    int thickness;
    if( m_Item->GetLayer() == EDGE_N )
        thickness =  m_BrdSettings->m_EdgeSegmentWidth;
    else
        thickness =  m_BrdSettings->m_DrawSegmentWidth;
    PutValueInLocalUnits( *m_DefaultThicknessCtrl, thickness,
        m_Parent->m_InternalUnits );

    for( int layer=FIRST_NO_COPPER_LAYER; layer <= LAST_NO_COPPER_LAYER;  ++layer )
    {
        m_LayerSelection->Append( m_Parent->GetBoard()->GetLayerName( layer ) );
    }

    int layer =  m_Item->GetLayer();
    // Control:
    if ( layer < FIRST_NO_COPPER_LAYER )
        layer = FIRST_NO_COPPER_LAYER;
    if ( layer > LAST_NO_COPPER_LAYER )
        layer = LAST_NO_COPPER_LAYER;
    m_LayerSelection->SetSelection( layer - FIRST_NO_COPPER_LAYER );


}


/*******************************************************************/
void DialogGraphicItemProperties::OnLayerChoice( wxCommandEvent& event )
/*******************************************************************/
{
    int thickness;
    if( (m_LayerSelection->GetCurrentSelection() + FIRST_NO_COPPER_LAYER) == EDGE_N )
        thickness =  m_BrdSettings->m_EdgeSegmentWidth;
    else
        thickness =  m_BrdSettings->m_DrawSegmentWidth;
    PutValueInLocalUnits( *m_DefaultThicknessCtrl, thickness,
        m_Parent->m_InternalUnits );
}

/*******************************************************************/
void DialogGraphicItemProperties::OnOkClick( wxCommandEvent& event )
/*******************************************************************/
/* Copy values in text control to the item parameters
*/
{
    m_Parent->SaveCopyInUndoList( m_Item, UR_CHANGED );

    wxString msg;
    if ( m_DC )
        m_Item->Draw( m_Parent->DrawPanel, m_DC, GR_XOR );

    msg = m_Center_StartXCtrl->GetValue();
    m_Item->m_Start.x = ReturnValueFromString( g_UnitMetric, msg,
        m_Parent->m_InternalUnits );

    msg = m_Center_StartYCtrl->GetValue();
    m_Item->m_Start.y = ReturnValueFromString( g_UnitMetric, msg,
        m_Parent->m_InternalUnits );

    msg = m_EndX_Radius_Ctrl->GetValue();
    m_Item->m_End.x = ReturnValueFromString( g_UnitMetric, msg,
        m_Parent->m_InternalUnits );

    msg = m_EndY_Ctrl->GetValue();
    m_Item->m_End.y = ReturnValueFromString( g_UnitMetric, msg,
        m_Parent->m_InternalUnits );

    msg = m_ThicknessCtrl->GetValue();
    m_Item->m_Width = ReturnValueFromString( g_UnitMetric, msg,
        m_Parent->m_InternalUnits );

    msg = m_DefaultThicknessCtrl->GetValue();
    int thickness = ReturnValueFromString( g_UnitMetric, msg,
        m_Parent->m_InternalUnits );

    m_Item->SetLayer( m_LayerSelection->GetCurrentSelection() + FIRST_NO_COPPER_LAYER);

    if( m_Item->GetLayer() == EDGE_N )
         m_BrdSettings->m_EdgeSegmentWidth = thickness;
    else
         m_BrdSettings->m_DrawSegmentWidth = thickness;

    if ( m_Item->m_Shape == S_ARC )
    {
        long angle;
        m_Angle_Ctrl->GetValue().ToLong(&angle);
        NORMALIZE_ANGLE(angle);
        m_Item->m_Angle = angle;
    }

    m_Parent->OnModify();
    if ( m_DC )
        m_Item->Draw( m_Parent->DrawPanel, m_DC, GR_OR );
    m_Item->DisplayInfo( m_Parent );

    Close( TRUE );
}

void DialogGraphicItemProperties::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}

