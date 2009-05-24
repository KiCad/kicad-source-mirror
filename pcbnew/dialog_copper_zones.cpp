/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_copper_zones.cpp
// Author:      jean-pierre Charras
// Created:     09/oct/2008
/// Licence:     GNU License
/////////////////////////////////////////////////////////////////////////////

#if defined (__GNUG__) && !defined (NO_GCC_PRAGMA)
#pragma implementation "zones.h"
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "PolyLine.h"
#include "pcbnew.h"
#include "trigo.h"
#include "autorout.h"
#include "zones.h"

#include "dialog_copper_zones.h"


/************************************************************************************************/
dialog_copper_zone::dialog_copper_zone( WinEDA_PcbFrame* parent, ZONE_SETTING* zone_setting ) :
    dialog_copper_zone_base( parent )
/************************************************************************************************/
{
    m_Parent = parent;
    m_Config = wxGetApp().m_EDA_Config;
    m_Zone_Setting = zone_setting;
    m_NetSorting   = 1;     // 0 = alphabetic sort, 1 = pad count sort, and filtering net names
    if( m_Config )
    {
        m_NetSorting = m_Config->Read( ZONE_NET_SORT_OPTION_KEY, 1l );
    }

    SetReturnCode( ZONE_ABORT ); // Will be changed on buttons click
}


/*****************************************************************/
void dialog_copper_zone::OnInitDialog( wxInitDialogEvent& event )
/*****************************************************************/

// Initialise all dialog options and values in wxTextCtrl
{
    BOARD* board = m_Parent->GetBoard();

    SetFocus();     // Required under wxGTK if we want to demiss the dialog with the ESC key

    wxString msg;

    if( g_Zone_45_Only )
        m_OrientEdgesOpt->SetSelection( 1 );

    m_FillModeCtrl->SetSelection( m_Zone_Setting->m_FillMode ? 1 : 0 );

    AddUnitSymbol( *m_ClearanceValueTitle, g_UnitMetric );
    msg = ReturnStringFromValue( g_UnitMetric,
                                 m_Zone_Setting->m_ZoneClearance,
                                 m_Parent->m_InternalUnits );
    m_ZoneClearanceCtrl->SetValue( msg );

    AddUnitSymbol( *m_MinThicknessValueTitle, g_UnitMetric );
    msg = ReturnStringFromValue( g_UnitMetric,
                                 m_Zone_Setting->m_ZoneMinThickness,
                                 m_Parent->m_InternalUnits );
    m_ZoneMinThicknessCtrl->SetValue( msg );

    switch( m_Zone_Setting->m_Zone_Pad_Options )
    {
    case PAD_NOT_IN_ZONE:           // Pads are not covered
        m_PadInZoneOpt->SetSelection( 2 );
        break;

    case THERMAL_PAD:               // Use thermal relief for pads
        m_PadInZoneOpt->SetSelection( 1 );
        break;

    case PAD_IN_ZONE:               // pads are covered by copper
        m_PadInZoneOpt->SetSelection( 0 );
        break;
    }


    if( m_Zone_Setting->m_Zone_Pad_Options != THERMAL_PAD )
    {
        m_AntipadSizeValue->Enable( false );
        m_CopperWidthValue->Enable( false );
    }
    else
    {
        m_AntipadSizeValue->Enable( true );
        m_CopperWidthValue->Enable( true );
    }

    AddUnitSymbol( *m_AntipadSizeText, g_UnitMetric );
    AddUnitSymbol( *m_CopperBridgeWidthText, g_UnitMetric );
    PutValueInLocalUnits( *m_AntipadSizeValue,
                          m_Zone_Setting->m_ThermalReliefGapValue,
                          PCB_INTERNAL_UNIT );
    PutValueInLocalUnits( *m_CopperWidthValue,
                          m_Zone_Setting->m_ThermalReliefCopperBridgeValue,
                          PCB_INTERNAL_UNIT );

    switch( m_Zone_Setting->m_Zone_HatchingStyle )
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

    m_ArcApproximationOpt->SetSelection( m_Zone_Setting->m_ArcToSegmentsCount == 32 ? 1 : 0 );

    /* build copper layers list */
    int layer_cnt = board->GetCopperLayerCount();
    for( int ii = 0; ii < board->GetCopperLayerCount(); ii++ )
    {
        int layer_number = COPPER_LAYER_N;

        if( layer_cnt <= 1 || ii < layer_cnt - 1 )
            layer_number = ii;
        else if( ii == layer_cnt - 1 )
            layer_number = LAYER_CMP_N;

        m_LayerId[ii] = layer_number;

        msg = board->GetLayerName( layer_number ).Trim();
        m_LayerSelectionCtrl->InsertItems( 1, &msg, ii );

        if( m_Zone_Setting->m_CurrentZone_Layer == layer_number )
            m_LayerSelectionCtrl->SetSelection( ii );
    }

    m_NetSortingOption->SetSelection( m_NetSorting );

    wxString NetNameFilter = wxT( "N_0*" );
    if( m_Config )
    {
        NetNameFilter =
            m_Config->Read( ZONE_NET_FILTER_STRING_KEY );
    }

    // Build list of nets:
    m_NetNameFilter->SetValue( NetNameFilter );
    wxArrayString ListNetName;
    m_Parent->GetBoard()->ReturnSortedNetnamesList(
        ListNetName,
        m_NetSorting == 0 ? false : true );

    if( m_NetSorting != 0 )
    {
        wxString Filter = m_NetNameFilter->GetValue();
        for( unsigned ii = 0; ii < ListNetName.GetCount(); ii++ )
        {
            if( ListNetName[ii].Matches( Filter.GetData() ) )
            {
                ListNetName.RemoveAt( ii );
                ii--;
            }
        }
    }

    ListNetName.Insert( wxT( "<no net>" ), 0 );
    m_ListNetNameSelection->InsertItems( ListNetName, 0 );

    // Select net:
    int net_select = m_Zone_Setting->m_NetcodeSelection;

    if( net_select > 0 )
    {
        NETINFO_ITEM* equipot = m_Parent->GetBoard()->FindNet( net_select );
        if( equipot )  // Search net in list and select it
        {
            for( unsigned ii = 0; ii < ListNetName.GetCount(); ii++ )
            {
                if( ListNetName[ii] == equipot->GetNetname() )
                {
                    m_ListNetNameSelection->SetSelection( ii );
                    m_ListNetNameSelection->EnsureVisible( ii );
                    break;
                }
            }
        }
    }
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
    Center();
}


/********************************************************************/
void dialog_copper_zone::OnButtonCancelClick( wxCommandEvent& event )
/********************************************************************/
{
    EndModal( ZONE_ABORT );
}


/********************************************************************************************/
bool dialog_copper_zone::AcceptOptions( bool aPromptForErrors, bool aUseExportableSetupOnly )
/********************************************************************************************/

/** Function dialog_copper_zone::AcceptOptions(
 * @return false if incorrect options, true if Ok.
 * @param aPromptForErrors = true to prompt user on incorrectparams
 * @param aUseExportableSetupOnly = true to use exportable parametres only (used to export this setup to other zones)
 */
{
    switch( m_PadInZoneOpt->GetSelection() )
    {
    case 2:
        m_Zone_Setting->m_Zone_Pad_Options = PAD_NOT_IN_ZONE;       // Pads are not covered
        break;

    case 1:
        m_Zone_Setting->m_Zone_Pad_Options = THERMAL_PAD;           // Use thermal relief for pads
        break;

    case 0:
        m_Zone_Setting->m_Zone_Pad_Options = PAD_IN_ZONE;           // pads are covered by copper
        break;
    }

    switch( m_OutlineAppearanceCtrl->GetSelection() )
    {
    case 0:
        m_Zone_Setting->m_Zone_HatchingStyle = CPolyLine::NO_HATCH;
        break;

    case 1:
        m_Zone_Setting->m_Zone_HatchingStyle = CPolyLine::DIAGONAL_EDGE;
        break;

    case 2:
        m_Zone_Setting->m_Zone_HatchingStyle = CPolyLine::DIAGONAL_FULL;
        break;
    }

    m_Zone_Setting->m_ArcToSegmentsCount = m_ArcApproximationOpt->GetSelection() == 1 ? 32 : 16;

    if( m_Config )
    {
        m_Config->Write( ZONE_NET_OUTLINES_HATCH_OPTION_KEY,
                         (long) m_Zone_Setting->m_Zone_HatchingStyle );
        wxString Filter = m_NetNameFilter->GetValue();
        m_Config->Write( ZONE_NET_FILTER_STRING_KEY, Filter );
    }

    m_Zone_Setting->m_FillMode = (m_FillModeCtrl->GetSelection() == 0) ? 0 : 1;

    wxString txtvalue = m_ZoneClearanceCtrl->GetValue();
    m_Zone_Setting->m_ZoneClearance =
        ReturnValueFromString( g_UnitMetric, txtvalue, m_Parent->m_InternalUnits );

    // Test if this is a reasonnable value for this parameter
    // A too large value can hang pcbnew
    #define CLEARANCE_MAX_VALUE 5000    // in 1/10000 inch
    if ( m_Zone_Setting->m_ZoneClearance > CLEARANCE_MAX_VALUE )
    {
        DisplayError( this, _( "Error : Zone clearance is set to an unreasonnable value" ) );
        return false;
    }

    txtvalue = m_ZoneMinThicknessCtrl->GetValue();
    m_Zone_Setting->m_ZoneMinThickness =
        ReturnValueFromString( g_UnitMetric, txtvalue, m_Parent->m_InternalUnits );
    if( m_Zone_Setting->m_ZoneMinThickness < 10 )
    {
        DisplayError( this,
                     _(
                         "Error :\nyou must choose a copper min thickness value bigger than 0.001 inch (or 0.0254 mm)" ) );
        return false;
    }

    if( m_OrientEdgesOpt->GetSelection() == 0 )
        g_Zone_45_Only = FALSE;
    else
        g_Zone_45_Only = TRUE;

    m_Zone_Setting->m_ThermalReliefGapValue = ReturnValueFromTextCtrl( *m_AntipadSizeValue,
                                                                       PCB_INTERNAL_UNIT );
    m_Zone_Setting->m_ThermalReliefCopperBridgeValue = ReturnValueFromTextCtrl(
        *m_CopperWidthValue,
        PCB_INTERNAL_UNIT );

    m_Config->Write( ZONE_THERMAL_RELIEF_GAP_STRING_KEY,
                     (long) m_Zone_Setting->m_ThermalReliefGapValue );
    m_Config->Write(
        ZONE_THERMAL_RELIEF_COPPER_WIDTH_STRING_KEY,
        (long) m_Zone_Setting->m_ThermalReliefCopperBridgeValue );

    if( m_Zone_Setting->m_ThermalReliefCopperBridgeValue <= m_Zone_Setting->m_ZoneMinThickness )
    {
        DisplayError( this,
                     _(
                         "Error :\nyou must choose a copper bridge value for thermal reliefs bigger than the min zone thickness" ) );
        return false;
    }

    // If we use only exportable to others zones parameters, exit here:
    if( aUseExportableSetupOnly )
        return true;

    /* Get the layer selection for this zone */
    int ii = m_LayerSelectionCtrl->GetSelection();
    if( ii < 0 && aPromptForErrors )
    {
        DisplayError( this, _( "Error : you must choose a layer" ) );
        return false;
    }


    m_Zone_Setting->m_CurrentZone_Layer = m_LayerId[ii];


    /* Get the net name selection for this zone */
    ii = m_ListNetNameSelection->GetSelection();
    if( ii < 0 && aPromptForErrors )
    {
        DisplayError( this, _( "Error : you must choose a net name" ) );
        return false;
    }

    if ( ii == 0 )  // the not connected option was selected: this is not a good practice: warn:
    {
       if( ! IsOK( this, _(
           "You have chosen the \"not connected\" option. This will create insulated copper islands. Are you sure ?") )
            )
        return false;
     }

    wxString net_name = m_ListNetNameSelection->GetString( ii );

    g_Zone_Default_Setting.m_NetcodeSelection = 0;

    /* Search net_code for this net, if a net was selected */
    if( m_ListNetNameSelection->GetSelection() > 0 )
    {
        NETINFO_ITEM* net = m_Parent->GetBoard()->FindNet(net_name);
        if( net )
            g_Zone_Default_Setting.m_NetcodeSelection = net->GetNet();
    }

    return true;
}


/***************************************************************************/
void dialog_copper_zone::OnNetSortingOptionSelected( wxCommandEvent& event )
/***************************************************************************/
{
    wxArrayString ListNetName;

    m_NetSorting = m_NetSortingOption->GetSelection();
    m_Parent->GetBoard()->ReturnSortedNetnamesList(
        ListNetName, m_NetSorting == 0 ? false : true );
    if( m_NetSorting != 0 )
    {
        wxString Filter = m_NetNameFilter->GetValue();
        for( unsigned ii = 0; ii < ListNetName.GetCount(); ii++ )
        {
            if( ListNetName[ii].Matches( Filter.GetData() ) )
            {
                ListNetName.RemoveAt( ii );
                ii--;
            }
        }
    }
    m_ListNetNameSelection->Clear();
    m_ListNetNameSelection->InsertItems( ListNetName, 0 );
    if( m_Config )
    {
        m_Config->Write( ZONE_NET_SORT_OPTION_KEY, (long) m_NetSorting );
        wxString Filter = m_NetNameFilter->GetValue();
        m_Config->Write( ZONE_NET_FILTER_STRING_KEY, Filter );
    }

    // Select and display current zone net name in listbox:
    int net_select = m_Zone_Setting->m_NetcodeSelection;
    if( net_select > 0 )
    {
        NETINFO_ITEM* equipot = m_Parent->GetBoard()->FindNet( net_select );
        if( equipot )  // Search net in list and select it
        {
            for( unsigned ii = 0; ii < ListNetName.GetCount(); ii++ )
            {
                if( ListNetName[ii] == equipot->GetNetname() )
                {
                    m_ListNetNameSelection->SetSelection( ii );
                    m_ListNetNameSelection->EnsureVisible( ii );
                    break;
                }
            }
        }
    }
}


/*****************************************************************/
void dialog_copper_zone::OnButtonOkClick( wxCommandEvent& event )
/*****************************************************************/
{
    if( AcceptOptions( true ) )
        EndModal( ZONE_OK );
}


/******************************************************************************/
void dialog_copper_zone::ExportSetupToOtherCopperZones( wxCommandEvent& event )
/******************************************************************************/
{
    if( !AcceptOptions( true, true ) )
        return;

    // Export settings ( but layer and netcode ) to others zones:
    BOARD* pcb = m_Parent->GetBoard();
    for( int ii = 0; ii < pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = pcb->GetArea( ii );
        m_Zone_Setting->ExportSetting( *zone, false );  // false = partiel export
        m_Parent->GetScreen()->SetModify();
    }
}


/******************************************************************/
void dialog_copper_zone::OnPadsInZoneClick( wxCommandEvent& event )
/******************************************************************/
{
    switch( m_PadInZoneOpt->GetSelection() )
    {
    default:
        m_AntipadSizeValue->Enable( false );
        m_CopperWidthValue->Enable( false );
        break;

    case 1:
        m_AntipadSizeValue->Enable( true );
        m_CopperWidthValue->Enable( true );
        break;
    }
}
