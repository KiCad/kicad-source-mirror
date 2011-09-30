/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_copper_zones.cpp
// Author:      jean-pierre Charras
// Created:     09/oct/2008
// Licence:     GNU License
/////////////////////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include <wx/imaglist.h>
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "confirm.h"
#include "PolyLine.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "trigo.h"
#include "zones.h"

#include "dialog_copper_zones.h"
#include "class_zone_setting.h"
#include "class_board.h"


#define LAYER_BITMAP_SIZE_X 20
#define LAYER_BITMAP_SIZE_Y 10

// Initialize static member variables
wxString DIALOG_COPPER_ZONE::m_netNameShowFilter( wxT( "*" ) );
wxPoint DIALOG_COPPER_ZONE::prevPosition( -1, -1 );
wxSize DIALOG_COPPER_ZONE::prevSize;


DIALOG_COPPER_ZONE::DIALOG_COPPER_ZONE( PCB_EDIT_FRAME* parent, ZONE_SETTING* zone_setting ) :
    DIALOG_COPPER_ZONE_BASE( parent )
{
    m_Parent = parent;
    m_Config = wxGetApp().m_EDA_Config;
    m_Zone_Setting = zone_setting;
    m_NetSortingByPadCount = true;     // false = alphabetic sort, true = pad count sort
    m_OnExitCode = ZONE_ABORT;

    SetReturnCode( ZONE_ABORT ); // Will be changed on buttons click

    m_LayerSelectionCtrl = new wxListView( this, wxID_ANY,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxLC_NO_HEADER | wxLC_REPORT
                                           | wxLC_SINGLE_SEL | wxRAISED_BORDER );
    wxListItem col0;
    col0.SetId( 0 );
    m_LayerSelectionCtrl->InsertColumn( 0, col0 );
    m_layerSizer->Add( m_LayerSelectionCtrl, 1,
                       wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    // Fix static text widget minimum width to a suitable value so that
    // resizing the dialog is not necessary when changing the corner smoothing type.
    // Depends on the default text in the widget.
    m_cornerSmoothingTitle->SetMinSize( m_cornerSmoothingTitle->GetSize() );

    initDialog();

    GetSizer()->SetSizeHints( this );

    if( prevPosition.x != -1 )
        SetSize( prevPosition.x, prevPosition.y,
                 prevSize.x, prevSize.y );
    else
        Center();
}


void DIALOG_COPPER_ZONE::initDialog()
{
    BOARD* board = m_Parent->GetBoard();

    SetFocus();     // Required under wxGTK if we want to demiss the dialog with the ESC key

    wxString msg;

    if( g_Zone_45_Only )
        m_OrientEdgesOpt->SetSelection( 1 );

    m_FillModeCtrl->SetSelection( m_Zone_Setting->m_FillMode ? 1 : 0 );

    AddUnitSymbol( *m_ClearanceValueTitle, g_UserUnit );
    msg = ReturnStringFromValue( g_UserUnit,
                                 m_Zone_Setting->m_ZoneClearance,
                                 m_Parent->m_InternalUnits );
    m_ZoneClearanceCtrl->SetValue( msg );

    AddUnitSymbol( *m_MinThicknessValueTitle, g_UserUnit );
    msg = ReturnStringFromValue( g_UserUnit,
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

    AddUnitSymbol( *m_AntipadSizeText, g_UserUnit );
    AddUnitSymbol( *m_CopperBridgeWidthText, g_UserUnit );
    PutValueInLocalUnits( *m_AntipadSizeValue,
                          m_Zone_Setting->m_ThermalReliefGapValue,
                          PCB_INTERNAL_UNIT );
    PutValueInLocalUnits( *m_CopperWidthValue,
                          m_Zone_Setting->m_ThermalReliefCopperBridgeValue,
                          PCB_INTERNAL_UNIT );

    m_cornerSmoothingChoice->SetSelection( m_Zone_Setting->GetCornerSmoothingType() );

    PutValueInLocalUnits( *m_cornerSmoothingCtrl,
                          m_Zone_Setting->GetCornerRadius(),
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

    m_ArcApproximationOpt->SetSelection(
        m_Zone_Setting->m_ArcToSegmentsCount == ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF ? 1 : 0 );

    // Build copper layer list and append to layer widget
    int layerCount = board->GetCopperLayerCount();
    int layerNumber, itemIndex, layerColor;
    wxImageList* imageList = new wxImageList( LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );
    m_LayerSelectionCtrl->AssignImageList( imageList, wxIMAGE_LIST_SMALL );
    for( int ii = 0; ii < layerCount; ii++ )
    {
        layerNumber = LAYER_N_BACK;

        if( layerCount <= 1 || ii < layerCount - 1 )
            layerNumber = ii;
        else if( ii == layerCount - 1 )
            layerNumber = LAYER_N_FRONT;

        m_LayerId.insert( m_LayerId.begin(), layerNumber );

        msg = board->GetLayerName( layerNumber ).Trim();
        layerColor = board->GetLayerColor( layerNumber );
        imageList->Add( makeLayerBitmap( layerColor ) );
        itemIndex = m_LayerSelectionCtrl->InsertItem( 0, msg, ii );

        if( m_Zone_Setting->m_CurrentZone_Layer == layerNumber )
            m_LayerSelectionCtrl->Select( itemIndex );
    }

    wxString netNameDoNotShowFilter = wxT( "N-*" );
    if( m_Config )
    {
        int opt = m_Config->Read( ZONE_NET_SORT_OPTION_KEY, 1l );
        m_NetDisplayOption->SetSelection( opt );
        m_Config->Read( ZONE_NET_FILTER_STRING_KEY, netNameDoNotShowFilter );
    }
    else
        m_NetDisplayOption->SetSelection( 1 );

    m_ShowNetNameFilter->SetValue( m_netNameShowFilter );
    initListNetsParams();

    // Build list of nets:
    m_DoNotShowNetNameFilter->SetValue( netNameDoNotShowFilter );
    buildAvailableListOfNets();

    wxCommandEvent event;
    OnCornerSmoothingModeChoice( event );
}


void DIALOG_COPPER_ZONE::OnButtonCancelClick( wxCommandEvent& event )
{
    Close( true );
}


void DIALOG_COPPER_ZONE::OnClose( wxCloseEvent& event )
{
    prevPosition = GetPosition();
    prevSize = GetSize();
    EndModal( m_OnExitCode );
}


void DIALOG_COPPER_ZONE::OnSize( wxSizeEvent& event )
{
    Layout();

    // Set layer list column width to widget width minus a few pixels
    m_LayerSelectionCtrl->SetColumnWidth( 0, m_LayerSelectionCtrl->GetSize().x - 5 );
    event.Skip();
}


void DIALOG_COPPER_ZONE::OnCornerSmoothingModeChoice( wxCommandEvent& event )
{
    int selection = m_cornerSmoothingChoice->GetSelection();

    switch( selection )
    {
    case ZONE_SETTING::SMOOTHING_NONE:
        m_cornerSmoothingTitle->Enable( false );
        m_cornerSmoothingCtrl->Enable( false );
        break;
    case ZONE_SETTING::SMOOTHING_CHAMFER:
        m_cornerSmoothingTitle->Enable( true );
        m_cornerSmoothingCtrl->Enable( true );
        m_cornerSmoothingTitle->SetLabel( _( "Chamfer distance" ) );
        AddUnitSymbol( *m_cornerSmoothingTitle, g_UserUnit );
        break;
    case ZONE_SETTING::SMOOTHING_FILLET:
        m_cornerSmoothingTitle->Enable( true );
        m_cornerSmoothingCtrl->Enable( true );
        m_cornerSmoothingTitle->SetLabel( _( "Fillet radius" ) );
        AddUnitSymbol( *m_cornerSmoothingTitle, g_UserUnit );
        break;
    }
}


bool DIALOG_COPPER_ZONE::AcceptOptions( bool aPromptForErrors, bool aUseExportableSetupOnly )
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

    m_Zone_Setting->m_ArcToSegmentsCount = m_ArcApproximationOpt->GetSelection() == 1 ?
                                           ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF :
                                           ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;

    if( m_Config )
    {
        m_Config->Write( ZONE_NET_OUTLINES_HATCH_OPTION_KEY,
                         (long) m_Zone_Setting->m_Zone_HatchingStyle );
        wxString Filter = m_DoNotShowNetNameFilter->GetValue();
        m_Config->Write( ZONE_NET_FILTER_STRING_KEY, Filter );
    }

    m_netNameShowFilter = m_ShowNetNameFilter->GetValue();
    m_Zone_Setting->m_FillMode = (m_FillModeCtrl->GetSelection() == 0) ? 0 : 1;

    wxString txtvalue = m_ZoneClearanceCtrl->GetValue();
    m_Zone_Setting->m_ZoneClearance =
        ReturnValueFromString( g_UserUnit, txtvalue, m_Parent->m_InternalUnits );

    // Test if this is a reasonnable value for this parameter
    // A too large value can hang Pcbnew
    #define CLEARANCE_MAX_VALUE 5000    // in 1/10000 inch
    if( m_Zone_Setting->m_ZoneClearance > CLEARANCE_MAX_VALUE )
    {
        DisplayError( this, _( "Clearance must be smaller than 0.5\" / 12.7 mm." ) );
        return false;
    }

    txtvalue = m_ZoneMinThicknessCtrl->GetValue();
    m_Zone_Setting->m_ZoneMinThickness =
        ReturnValueFromString( g_UserUnit, txtvalue, m_Parent->m_InternalUnits );
    if( m_Zone_Setting->m_ZoneMinThickness < 10 )
    {
        DisplayError( this,
                     _( "Minimum width must be larger than 0.001\" / 0.0254 mm." ) );
        return false;
    }

    m_Zone_Setting->SetCornerSmoothingType( m_cornerSmoothingChoice->GetSelection() );
    txtvalue = m_cornerSmoothingCtrl->GetValue();
    m_Zone_Setting->SetCornerRadius( ReturnValueFromString( g_UserUnit, txtvalue, m_Parent->m_InternalUnits ) );

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
                     _( "Thermal relief spoke width is larger than the minimum width." ) );
        return false;
    }

    // If we use only exportable to others zones parameters, exit here:
    if( aUseExportableSetupOnly )
        return true;

    // Get the layer selection for this zone
    int ii = m_LayerSelectionCtrl->GetFirstSelected();

    if( ii < 0 && aPromptForErrors )
    {
        DisplayError( this, _( "No layer selected." ) );
        return false;
    }

    m_Zone_Setting->m_CurrentZone_Layer = m_LayerId[ii];

    // Get the net name selection for this zone
    ii = m_ListNetNameSelection->GetSelection();

    if( ii < 0 && aPromptForErrors )
    {
        DisplayError( this, _( "No net selected." ) );
        return false;
    }

    if( ii == 0 )   // the not connected option was selected: this is not a good practice: warn:
    {
        if( !IsOK( this, _(
                      "You have chosen the \"not connected\" option. This will create insulated copper islands. Are you sure ?" ) )
            )
            return false;
    }

    wxString net_name = m_ListNetNameSelection->GetString( ii );

    g_Zone_Default_Setting.m_NetcodeSelection = 0;

    // Search net_code for this net, if a net was selected
    if( m_ListNetNameSelection->GetSelection() > 0 )
    {
        NETINFO_ITEM* net = m_Parent->GetBoard()->FindNet( net_name );
        if( net )
            g_Zone_Default_Setting.m_NetcodeSelection = net->GetNet();
    }

    return true;
}


void DIALOG_COPPER_ZONE::OnNetSortingOptionSelected( wxCommandEvent& event )
{
    initListNetsParams();
    buildAvailableListOfNets();

    m_netNameShowFilter = m_ShowNetNameFilter->GetValue();
    if( m_Config )
    {
        m_Config->Write( ZONE_NET_SORT_OPTION_KEY, (long) m_NetDisplayOption->GetSelection() );
        wxString Filter = m_DoNotShowNetNameFilter->GetValue();
        m_Config->Write( ZONE_NET_FILTER_STRING_KEY, Filter );
    }
}


void DIALOG_COPPER_ZONE::OnButtonOkClick( wxCommandEvent& event )
{
    m_netNameShowFilter = m_ShowNetNameFilter->GetValue();
    prevPosition = GetPosition();
    prevSize = GetSize();

    if( AcceptOptions( true ) )
        EndModal( ZONE_OK );
}


void DIALOG_COPPER_ZONE::ExportSetupToOtherCopperZones( wxCommandEvent& event )
{
    prevPosition = GetPosition();
    prevSize = GetSize();

    if( !AcceptOptions( true, true ) )
        return;

    // Export settings ( but layer and netcode ) to others zones:
    BOARD* pcb = m_Parent->GetBoard();
    for( int ii = 0; ii < pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = pcb->GetArea( ii );
        m_Zone_Setting->ExportSetting( *zone, false );  // false = partiel export
        m_Parent->OnModify();
    }

    m_OnExitCode = ZONE_EXPORT_VALUES;     // values are exported to others zones
}


void DIALOG_COPPER_ZONE::OnPadsInZoneClick( wxCommandEvent& event )
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


void DIALOG_COPPER_ZONE::initListNetsParams()
{
    switch( m_NetDisplayOption->GetSelection() )
    {
    case 0:
        m_NetSortingByPadCount = false;
        m_NetFiltering = false;
        break;

    case 1:
        m_NetSortingByPadCount = true;
        m_NetFiltering = false;
        break;

    case 2:
        m_NetSortingByPadCount = false;
        m_NetFiltering = true;
        break;

    case 3:
        m_NetSortingByPadCount = true;
        m_NetFiltering = true;
        break;
    }
}


void DIALOG_COPPER_ZONE::OnRunFiltersButtonClick( wxCommandEvent& event )
{
    m_netNameShowFilter = m_ShowNetNameFilter->GetValue();

    // Ensure filtered option for nets
    if( m_NetDisplayOption->GetSelection() == 0 )
        m_NetDisplayOption->SetSelection( 2 );
    else if( m_NetDisplayOption->GetSelection() == 1 )
        m_NetDisplayOption->SetSelection( 3 );
    initListNetsParams();
    buildAvailableListOfNets();
}


void DIALOG_COPPER_ZONE::buildAvailableListOfNets()
{
    wxArrayString   listNetName;

    m_Parent->GetBoard()->ReturnSortedNetnamesList( listNetName, m_NetSortingByPadCount );

    if( m_NetFiltering )
    {
        wxString doNotShowFilter = m_DoNotShowNetNameFilter->GetValue();
        wxString ShowFilter = m_ShowNetNameFilter->GetValue();

        for( unsigned ii = 0; ii < listNetName.GetCount(); ii++ )
        {
            if( listNetName[ii].Matches( doNotShowFilter ) )
            {
                listNetName.RemoveAt( ii );
                ii--;
            }
            else if( !listNetName[ii].Matches( ShowFilter ) )
            {
                listNetName.RemoveAt( ii );
                ii--;
            }
        }
    }

    listNetName.Insert( wxT( "<no net>" ), 0 );

    // Ensure currently selected net for the zone is visible, regardless of filters
    int selectedNetListNdx = -1;
    int net_select = m_Zone_Setting->m_NetcodeSelection;

    if( net_select > 0 )
    {
        NETINFO_ITEM* equipot = m_Parent->GetBoard()->FindNet( net_select );
        if( equipot )
        {
            selectedNetListNdx = listNetName.Index( equipot->GetNetname() );

            if( wxNOT_FOUND == selectedNetListNdx )
            {
                // the currently selected net must *always* be visible.
                listNetName.Insert( equipot->GetNetname(), 0 );
                selectedNetListNdx = 0;
            }
        }
    }
    else if( net_select == 0 )
        selectedNetListNdx = 0;     // SetSelection() on "<no net>"
    else
    {
        // selectedNetListNdx remains -1, no net selected.
    }

    m_ListNetNameSelection->Clear();
    m_ListNetNameSelection->InsertItems( listNetName, 0 );
    m_ListNetNameSelection->SetSelection( 0 );

    if( selectedNetListNdx >= 0 )
    {
        m_ListNetNameSelection->SetSelection( selectedNetListNdx );
        m_ListNetNameSelection->EnsureVisible( selectedNetListNdx );
    }
}


wxBitmap DIALOG_COPPER_ZONE::makeLayerBitmap( int aColor )
{
    wxBitmap    bitmap( LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );
    wxBrush     brush;
    wxMemoryDC  iconDC;

    iconDC.SelectObject( bitmap );
    brush.SetColour( MakeColour( aColor ) );
    brush.SetStyle( wxSOLID );
    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );

    return bitmap;
}
