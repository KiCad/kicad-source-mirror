
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "confirm.h"
#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "zones.h"

#include "dialog_non_copper_zones_properties_base.h"

/* Local functions */

/* Local variables */

/* Class DialogNonCopperZonesEditor
 * Dialog editor for non copper zones properties
 * Derived from DialogNonCopperZonesPropertiesBase, created by wxFormBuilder
 */
class DialogNonCopperZonesEditor : public DialogNonCopperZonesPropertiesBase
{
private:
    WinEDA_PcbFrame* m_Parent;
    ZONE_CONTAINER*  m_Zone_Container;
    ZONE_SETTING* m_Zone_Setting;

private:
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void Init();

public:
    DialogNonCopperZonesEditor( WinEDA_PcbFrame* parent,
                                ZONE_CONTAINER*  zone_container,
                                ZONE_SETTING*    zone_setting );
    ~DialogNonCopperZonesEditor();
};


/*******************************************************************************************/
DialogNonCopperZonesEditor::DialogNonCopperZonesEditor( WinEDA_PcbFrame* parent,
                                                        ZONE_CONTAINER*  zone_container,
                                                        ZONE_SETTING*    zone_setting ) :
    DialogNonCopperZonesPropertiesBase( parent )
/*******************************************************************************************/
{
    m_Parent = parent;
    m_Zone_Container = zone_container;
    m_Zone_Setting   = zone_setting;
    Init();
    /* the size of some items has changed, so we must call SetSizeHints() */
    GetSizer()->SetSizeHints( this );
}


/********************************************************/
DialogNonCopperZonesEditor::~DialogNonCopperZonesEditor()
/********************************************************/
{
}


bool WinEDA_PcbFrame::InstallDialogNonCopperZonesEditor( ZONE_CONTAINER* aZone )
{
    DialogNonCopperZonesEditor frame( this, aZone, &g_Zone_Default_Setting );
    bool diag = frame.ShowModal();

    return diag;
}


/********************************************************************/
void DialogNonCopperZonesEditor::Init()
/********************************************************************/
{
    SetFocus();
    SetReturnCode( ZONE_ABORT );  // Will be changed on buttons click

    m_FillModeCtrl->SetSelection( m_Zone_Setting->m_FillMode ? 1 : 0 );

    AddUnitSymbol( *m_MinThicknessValueTitle, g_UserUnit );
    wxString msg = ReturnStringFromValue( g_UserUnit,
                                 m_Zone_Setting->m_ZoneMinThickness,
                                 m_Parent->m_InternalUnits );
    m_ZoneMinThicknessCtrl->SetValue( msg );

    if( g_Zone_45_Only )
        m_OrientEdgesOpt->SetSelection( 1 );

    switch( g_Zone_Default_Setting.m_Zone_HatchingStyle )
    {
    case CPolyLine::NO_HATCH:
        m_OutlineAppearanceCtrl->SetSelection( 0 );
        break;

    case CPolyLine::DIAGONAL_EDGE:
        m_OutlineAppearanceCtrl->SetSelection( 1 );
        break;

    case CPolyLine::DIAGONAL_FULL:
        m_OutlineAppearanceCtrl->SetSelection( 2 );
        break;
    }

    for( int layer_number = FIRST_NO_COPPER_LAYER, ii = 0;
         layer_number <= LAST_NO_COPPER_LAYER;
         layer_number++, ii++ )
    {
        wxString msg;

        msg = m_Parent->GetBoard()->GetLayerName( layer_number ).Trim();
        m_LayerSelectionCtrl->InsertItems( 1, &msg, ii );

        if( m_Zone_Container )
        {
            if( m_Zone_Container->GetLayer() == layer_number )
                m_LayerSelectionCtrl->SetSelection( ii );
        }
        else
        {
            if( ( (PCB_SCREEN*)( m_Parent->GetScreen() ) )->m_Active_Layer == layer_number )
                m_LayerSelectionCtrl->SetSelection( ii );
        }
    }
}


/******************************************************************/
void DialogNonCopperZonesEditor::OnOkClick( wxCommandEvent& event )
/******************************************************************/
{
    wxString txtvalue = m_ZoneMinThicknessCtrl->GetValue();
    m_Zone_Setting->m_ZoneMinThickness =
        ReturnValueFromString( g_UserUnit, txtvalue, m_Parent->m_InternalUnits );
    if( m_Zone_Setting->m_ZoneMinThickness < 10 )
    {
        DisplayError( this,
                     _(
                         "Error :\nyou must choose a copper min thickness value bigger than 0.001 inch (or 0.0254 mm)" ) );
        return;
    }

    m_Zone_Setting->m_FillMode = (m_FillModeCtrl->GetSelection() == 0) ? 0 : 1;

    switch( m_OutlineAppearanceCtrl->GetSelection() )
    {
    case 0:
        g_Zone_Default_Setting.m_Zone_HatchingStyle = CPolyLine::NO_HATCH;
        break;

    case 1:
        g_Zone_Default_Setting.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_EDGE;
        break;

    case 2:
        g_Zone_Default_Setting.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_FULL;
        break;
    }

    if( wxGetApp().m_EDA_Config )
    {
        wxGetApp().m_EDA_Config->Write( ZONE_NET_OUTLINES_HATCH_OPTION_KEY,
                                        (long) g_Zone_Default_Setting.m_Zone_HatchingStyle );
    }

    if( m_OrientEdgesOpt->GetSelection() == 0 )
        g_Zone_45_Only = FALSE;
    else
        g_Zone_45_Only = TRUE;

    /* Get the layer selection for this zone */
    int ii = m_LayerSelectionCtrl->GetSelection();
    if( ii < 0 )
    {
        DisplayError( this, _( "Error : you must choose a layer" ) );
        return;
    }
    g_Zone_Default_Setting.m_CurrentZone_Layer = ii + FIRST_NO_COPPER_LAYER;
    EndModal( ZONE_OK );
}


/**********************************************************************/
void DialogNonCopperZonesEditor::OnCancelClick( wxCommandEvent& event )
/**********************************************************************/
{
    EndModal( ZONE_ABORT );
}
