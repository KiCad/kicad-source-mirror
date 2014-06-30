/**
 * @file zones_non_copper_type_functions.cpp
 */

#include <fctsys.h>
//#include <pgm_base.h>
#include <kiface_i.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <base_units.h>

#include <class_board.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <zones.h>

#include <dialog_non_copper_zones_properties_base.h>


/**
 * Class DIALOG_NON_COPPER_ZONES_EDITOR
 * is a dialog editor for non copper zones properties,
 * derived from DialogNonCopperZonesPropertiesBase, which is maintained and
 * created by wxFormBuilder
 */
class DIALOG_NON_COPPER_ZONES_EDITOR : public DialogNonCopperZonesPropertiesBase
{
private:
    PCB_BASE_FRAME* m_Parent;
    ZONE_CONTAINER* m_zone;
    ZONE_SETTINGS*  m_ptr;
    ZONE_SETTINGS   m_settings;     // working copy of zone settings

    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void Init();

public:
    DIALOG_NON_COPPER_ZONES_EDITOR( PCB_BASE_FRAME* aParent,
                                    ZONE_CONTAINER* aZone, ZONE_SETTINGS* aSettings );
};


ZONE_EDIT_T InvokeNonCopperZonesEditor( PCB_BASE_FRAME* aParent,
                                        ZONE_CONTAINER* aZone, ZONE_SETTINGS* aSettings )
{
    DIALOG_NON_COPPER_ZONES_EDITOR  dlg( aParent, aZone, aSettings );

    ZONE_EDIT_T result = ZONE_EDIT_T( dlg.ShowModal() );

    // D(printf( "%s: result:%d\n", __FUNCTION__, result );)

    return result;
}


DIALOG_NON_COPPER_ZONES_EDITOR::DIALOG_NON_COPPER_ZONES_EDITOR( PCB_BASE_FRAME* aParent,
                                                                ZONE_CONTAINER* aZone,
                                                                ZONE_SETTINGS* aSettings ) :
    DialogNonCopperZonesPropertiesBase( aParent )
{
    m_Parent = aParent;

    m_zone = aZone;
    m_ptr  = aSettings;
    m_settings = *aSettings;

    Init();

    // the size of some items has changed, so we must call SetSizeHints()
    GetSizer()->SetSizeHints( this );
}


void DIALOG_NON_COPPER_ZONES_EDITOR::Init()
{
    SetReturnCode( ZONE_ABORT );  // Will be changed on button click

    AddUnitSymbol( *m_MinThicknessValueTitle, g_UserUnit );
    wxString msg = StringFromValue( g_UserUnit, m_settings.m_ZoneMinThickness );
    m_ZoneMinThicknessCtrl->SetValue( msg );

    if( m_settings.m_Zone_45_Only )
        m_OrientEdgesOpt->SetSelection( 1 );

    switch( m_settings.m_Zone_HatchingStyle )
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

    int ii = 0;

    for( LSEQ seq = LSET::AllNonCuMask().Seq();  seq;  ++seq, ++ii )
    {
        LAYER_ID layer = *seq;

        wxString msg = m_Parent->GetBoard()->GetLayerName( layer );

        msg.Trim();

        m_LayerSelectionCtrl->InsertItems( 1, &msg, ii );

        if( m_zone )
        {
            if( m_zone->GetLayer() == layer )
                m_LayerSelectionCtrl->SetSelection( ii );
        }
        else
        {
            if( ( (PCB_SCREEN*) m_Parent->GetScreen() )->m_Active_Layer == layer )
                m_LayerSelectionCtrl->SetSelection( ii );
        }
    }
}


void DIALOG_NON_COPPER_ZONES_EDITOR::OnOkClick( wxCommandEvent& event )
{
    wxString txtvalue = m_ZoneMinThicknessCtrl->GetValue();

    m_settings.m_ZoneMinThickness = ValueFromString( g_UserUnit, txtvalue );

    if( m_settings.m_ZoneMinThickness < 10 )
    {
        DisplayError( this,
                      _( "Error :\nyou must choose a min thickness value bigger than 0.001 inch (or 0.0254 mm)" ) );
        return;
    }

    m_settings.m_FillMode = 0;  // Use always polygon fill mode

    switch( m_OutlineAppearanceCtrl->GetSelection() )
    {
    case 0:
        m_settings.m_Zone_HatchingStyle = CPolyLine::NO_HATCH;
        break;

    case 1:
        m_settings.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_EDGE;
        break;

    case 2:
        m_settings.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_FULL;
        break;
    }

    wxConfigBase* cfg = Kiface().KifaceSettings();
    wxASSERT( cfg );

    cfg->Write( ZONE_NET_OUTLINES_HATCH_OPTION_KEY, (long) m_settings.m_Zone_HatchingStyle );

    if( m_OrientEdgesOpt->GetSelection() == 0 )
        m_settings.m_Zone_45_Only = false;
    else
        m_settings.m_Zone_45_Only = true;

    // Get the layer selection for this zone
    int ii = m_LayerSelectionCtrl->GetSelection();

    if( ii < 0 )
    {
        DisplayError( this, _( "Error : you must choose a layer" ) );
        return;
    }

    LSEQ seq = LSET::AllNonCuMask().Seq();

    m_settings.m_CurrentZone_Layer = seq[ii];

    *m_ptr = m_settings;

    EndModal( ZONE_OK );
}


void DIALOG_NON_COPPER_ZONES_EDITOR::OnCancelClick( wxCommandEvent& event )
{
    // do not save the edits.

    EndModal( ZONE_ABORT );
}

