/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <zone_settings.h>

#include <pcb_base_frame.h>
#include <board.h>
#include <lset.h>
#include <settings/color_settings.h>
#include <zones.h>

#include <zone.h>
#include <wx/dataview.h>
#include <widgets/color_swatch.h>

bool ZONE_LAYER_PROPERTIES::operator==( const ZONE_LAYER_PROPERTIES& aOther ) const
{
    return hatching_offset == aOther.hatching_offset;
}

ZONE_SETTINGS::ZONE_SETTINGS()
{
    m_ZonePriority = 0;
    m_FillMode = ZONE_FILL_MODE::POLYGONS; // Mode for filling zone
    // Zone clearance value
    m_ZoneClearance = pcbIUScale.mmToIU( ZONE_CLEARANCE_MM );
    // Min thickness value in filled areas (this is the minimum width of copper to fill solid areas) :
    m_ZoneMinThickness = pcbIUScale.mmToIU( ZONE_THICKNESS_MM );
    // Arbitrary defaults for the hatch settings
    m_HatchThickness = std::max( m_ZoneMinThickness * 4, pcbIUScale.mmToIU( 1.0 ) );
    m_HatchGap       = std::max( m_ZoneMinThickness * 6, pcbIUScale.mmToIU( 1.5 ) );
    m_HatchOrientation = ANGLE_0;   // Grid style: orientation of grid lines
    m_HatchSmoothingLevel = 0;      // Grid pattern smoothing type. 0 = no smoothing
    m_HatchSmoothingValue = 0.1;    // Grid pattern chamfer value relative to the gap value
    m_HatchHoleMinArea = 0.15;      // Min size before holes are dropped (ratio of hole size)
    m_HatchBorderAlgorithm = 1;     // 0 = use zone min thickness; 1 = use hatch width
    m_Netcode = 0;                  // Net code for the current zone
    m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE; // Option to show the zone
                                                                         // outlines only, short
                                                                         // hatches or full hatches
    m_BorderHatchPitch = pcbIUScale.mmToIU( ZONE_BORDER_HATCH_DIST_MM );

    m_Layers.reset().set( F_Cu );
    m_Name = wxEmptyString;

    // thickness of the gap in thermal reliefs:
    m_ThermalReliefGap = pcbIUScale.mmToIU( ZONE_THERMAL_RELIEF_GAP_MM );
    // thickness of the copper bridge in thermal reliefs:
    m_ThermalReliefSpokeWidth = pcbIUScale.mmToIU( ZONE_THERMAL_RELIEF_COPPER_WIDTH_MM );

    m_padConnection = ZONE_CONNECTION::THERMAL; // How pads are covered by copper in zone

    m_Locked = false;

    m_cornerSmoothingType = SMOOTHING_NONE;
    m_cornerRadius = 0;

    m_removeIslands = ISLAND_REMOVAL_MODE::ALWAYS;
    m_minIslandArea = 10 * pcbIUScale.IU_PER_MM * pcbIUScale.IU_PER_MM;

    SetIsRuleArea( false );
    SetPlacementAreaSourceType( PLACEMENT_SOURCE_T::SHEETNAME );
    SetDoNotAllowZoneFills( false );
    SetDoNotAllowVias( true );
    SetDoNotAllowTracks( true );
    SetDoNotAllowPads( true );
    SetDoNotAllowFootprints( false );

    m_TeardropType = TEARDROP_TYPE::TD_NONE;
    m_placementAreaEnabled = false;
}


bool ZONE_SETTINGS::operator==( const ZONE_SETTINGS& aOther ) const
{
    if( m_ZonePriority                != aOther.m_ZonePriority ) return false;
    if( m_FillMode                    != aOther.m_FillMode ) return false;
    if( m_ZoneClearance               != aOther.m_ZoneClearance ) return false;
    if( m_ZoneMinThickness            != aOther.m_ZoneMinThickness ) return false;
    if( m_HatchThickness              != aOther.m_HatchThickness ) return false;
    if( m_HatchGap                    != aOther.m_HatchGap ) return false;
    if( m_HatchOrientation            != aOther.m_HatchOrientation ) return false;
    if( m_HatchSmoothingLevel         != aOther.m_HatchSmoothingLevel ) return false;
    if( m_HatchSmoothingValue         != aOther.m_HatchSmoothingValue ) return false;
    if( m_HatchBorderAlgorithm        != aOther.m_HatchBorderAlgorithm ) return false;
    if( m_HatchHoleMinArea            != aOther.m_HatchHoleMinArea ) return false;
    if( m_Netcode                     != aOther.m_Netcode ) return false;
    if( m_Name                        != aOther.m_Name ) return false;
    if( m_ZoneBorderDisplayStyle      != aOther.m_ZoneBorderDisplayStyle ) return false;
    if( m_BorderHatchPitch            != aOther.m_BorderHatchPitch ) return false;
    if( m_ThermalReliefGap            != aOther.m_ThermalReliefGap ) return false;
    if( m_ThermalReliefSpokeWidth     != aOther.m_ThermalReliefSpokeWidth ) return false;
    if( m_padConnection               != aOther.m_padConnection ) return false;
    if( m_cornerSmoothingType         != aOther.m_cornerSmoothingType ) return false;
    if( m_cornerRadius                != aOther.m_cornerRadius ) return false;
    if( m_isRuleArea                  != aOther.m_isRuleArea ) return false;
    if( m_placementAreaEnabled        != aOther.m_placementAreaEnabled ) return false;
    if( m_placementAreaSourceType     != aOther.m_placementAreaSourceType ) return false;
    if( m_placementAreaSource         != aOther.m_placementAreaSource ) return false;
    if( m_keepoutDoNotAllowZoneFills  != aOther.m_keepoutDoNotAllowZoneFills ) return false;
    if( m_keepoutDoNotAllowVias       != aOther.m_keepoutDoNotAllowVias ) return false;
    if( m_keepoutDoNotAllowTracks     != aOther.m_keepoutDoNotAllowTracks ) return false;
    if( m_keepoutDoNotAllowPads       != aOther.m_keepoutDoNotAllowPads ) return false;
    if( m_keepoutDoNotAllowFootprints != aOther.m_keepoutDoNotAllowFootprints ) return false;
    if( m_Locked                      != aOther.m_Locked ) return false;
    if( m_removeIslands               != aOther.m_removeIslands ) return false;
    if( m_minIslandArea               != aOther.m_minIslandArea ) return false;

    if( !std::equal( std::begin( m_LayerProperties ), std::end( m_LayerProperties ),
                     std::begin( aOther.m_LayerProperties ) ) )
        return false;

    // Currently, the teardrop area type is not really a ZONE_SETTINGS parameter,
    // but a ZONE parameter only.
    // However it can be used in dialogs
    if( m_TeardropType != aOther.m_TeardropType ) return false;

    if( m_Layers != aOther.m_Layers ) return false;

    return true;
}


ZONE_SETTINGS& ZONE_SETTINGS::operator << ( const ZONE& aSource )
{
    m_ZonePriority                = aSource.GetAssignedPriority();
    m_FillMode                    = aSource.GetFillMode();
    m_ZoneClearance               = aSource.GetLocalClearance().value();
    m_ZoneMinThickness            = aSource.GetMinThickness();
    m_HatchThickness              = aSource.GetHatchThickness();
    m_HatchGap                    = aSource.GetHatchGap();
    m_HatchOrientation            = aSource.GetHatchOrientation();
    m_HatchSmoothingLevel         = aSource.GetHatchSmoothingLevel();
    m_HatchSmoothingValue         = aSource.GetHatchSmoothingValue();
    m_HatchBorderAlgorithm        = aSource.GetHatchBorderAlgorithm();
    m_HatchHoleMinArea            = aSource.GetHatchHoleMinArea();
    m_Netcode                     = aSource.GetNetCode();
    m_Name                        = aSource.GetZoneName();
    m_ZoneBorderDisplayStyle      = aSource.GetHatchStyle();
    m_BorderHatchPitch            = aSource.GetBorderHatchPitch();
    m_ThermalReliefGap            = aSource.GetThermalReliefGap();
    m_ThermalReliefSpokeWidth     = aSource.GetThermalReliefSpokeWidth();
    m_padConnection               = aSource.GetPadConnection();
    m_cornerSmoothingType         = aSource.GetCornerSmoothingType();
    m_cornerRadius                = aSource.GetCornerRadius();
    m_isRuleArea                  = aSource.GetIsRuleArea();
    m_placementAreaEnabled        = aSource.GetPlacementAreaEnabled();
    m_placementAreaSourceType     = aSource.GetPlacementAreaSourceType();
    m_placementAreaSource         = aSource.GetPlacementAreaSource();
    m_keepoutDoNotAllowZoneFills  = aSource.GetDoNotAllowZoneFills();
    m_keepoutDoNotAllowVias       = aSource.GetDoNotAllowVias();
    m_keepoutDoNotAllowTracks     = aSource.GetDoNotAllowTracks();
    m_keepoutDoNotAllowPads       = aSource.GetDoNotAllowPads();
    m_keepoutDoNotAllowFootprints = aSource.GetDoNotAllowFootprints();
    m_Locked                      = aSource.IsLocked();
    m_removeIslands               = aSource.GetIslandRemovalMode();
    m_minIslandArea               = aSource.GetMinIslandArea();
    m_LayerProperties             = aSource.LayerProperties();

    // Currently, the teardrop area type is not really a ZONE_SETTINGS parameter,
    // but a ZONE parameter only.
    // However it can be used in dialogs
    m_TeardropType                = aSource.GetTeardropAreaType();

    m_Layers = aSource.GetLayerSet();

    return *this;
}


void ZONE_SETTINGS::ExportSetting( ZONE& aTarget, bool aFullExport ) const
{
    aTarget.SetFillMode( m_FillMode );
    aTarget.SetLocalClearance( m_ZoneClearance );
    aTarget.SetMinThickness( m_ZoneMinThickness );
    aTarget.SetHatchThickness( m_HatchThickness );
    aTarget.SetHatchGap( m_HatchGap );
    aTarget.SetHatchOrientation( m_HatchOrientation );
    aTarget.SetHatchSmoothingLevel( m_HatchSmoothingLevel );
    aTarget.SetHatchSmoothingValue( m_HatchSmoothingValue );
    aTarget.SetHatchBorderAlgorithm( m_HatchBorderAlgorithm );
    aTarget.SetHatchHoleMinArea( m_HatchHoleMinArea );
    aTarget.SetThermalReliefGap( m_ThermalReliefGap );
    aTarget.SetThermalReliefSpokeWidth( m_ThermalReliefSpokeWidth );
    aTarget.SetPadConnection( m_padConnection );
    aTarget.SetCornerSmoothingType( m_cornerSmoothingType );
    aTarget.SetCornerRadius( m_cornerRadius );
    aTarget.SetIsRuleArea( GetIsRuleArea() );
    aTarget.SetPlacementAreaEnabled( GetPlacementAreaEnabled() );
    aTarget.SetPlacementAreaSourceType( GetPlacementAreaSourceType() );
    aTarget.SetPlacementAreaSource( GetPlacementAreaSource() );
    aTarget.SetDoNotAllowZoneFills( GetDoNotAllowZoneFills() );
    aTarget.SetDoNotAllowVias( GetDoNotAllowVias() );
    aTarget.SetDoNotAllowTracks( GetDoNotAllowTracks() );
    aTarget.SetDoNotAllowPads( GetDoNotAllowPads() );
    aTarget.SetDoNotAllowFootprints( GetDoNotAllowFootprints() );
    aTarget.SetLocked( m_Locked );
    aTarget.SetIslandRemovalMode( GetIslandRemovalMode() );
    aTarget.SetMinIslandArea( GetMinIslandArea() );
    // Currently, the teardrop area type is not imported from a ZONE_SETTINGS, because
    // it is not really a ZONE_SETTINGS parameter, but a ZONE parameter only
#if 0
    aTarget.SetTeardropAreaType( m_TeardropType );
#endif


    if( aFullExport )
    {
        aTarget.SetAssignedPriority( m_ZonePriority );

        aTarget.SetLayerProperties( m_LayerProperties );
        aTarget.SetLayerSet( m_Layers );

        aTarget.SetZoneName( m_Name );

        if( !m_isRuleArea )
            aTarget.SetNetCode( m_Netcode );
    }

    // call SetBorderDisplayStyle last, because hatch lines will be rebuilt,
    // using new parameters values
    aTarget.SetBorderDisplayStyle( m_ZoneBorderDisplayStyle,
                                   m_BorderHatchPitch, true );
}

void ZONE_SETTINGS::CopyFrom( const ZONE_SETTINGS& aOther, bool aCopyFull )
{
    m_ZonePriority                = aOther.m_ZonePriority;
    m_FillMode                    = aOther.m_FillMode;
    m_ZoneClearance               = aOther.m_ZoneClearance;
    m_ZoneMinThickness            = aOther.m_ZoneMinThickness;
    m_HatchThickness              = aOther.m_HatchThickness;
    m_HatchGap                    = aOther.m_HatchGap;
    m_HatchOrientation            = aOther.m_HatchOrientation;
    m_HatchSmoothingLevel         = aOther.m_HatchSmoothingLevel;
    m_HatchSmoothingValue         = aOther.m_HatchSmoothingValue;
    m_HatchBorderAlgorithm        = aOther.m_HatchBorderAlgorithm;
    m_HatchHoleMinArea            = aOther.m_HatchHoleMinArea;
    m_Netcode                     = aOther.m_Netcode;
    m_Name                        = aOther.m_Name;
    m_ZoneBorderDisplayStyle      = aOther.m_ZoneBorderDisplayStyle;
    m_BorderHatchPitch            = aOther.m_BorderHatchPitch;
    m_ThermalReliefGap            = aOther.m_ThermalReliefGap;
    m_ThermalReliefSpokeWidth     = aOther.m_ThermalReliefSpokeWidth;
    m_padConnection               = aOther.m_padConnection;
    m_cornerSmoothingType         = aOther.m_cornerSmoothingType;
    m_cornerRadius                = aOther.m_cornerRadius;
    m_isRuleArea                  = aOther.m_isRuleArea;
    m_placementAreaEnabled        = aOther.m_placementAreaEnabled;
    m_placementAreaSourceType     = aOther.m_placementAreaSourceType;
    m_placementAreaSource         = aOther.m_placementAreaSource;
    m_keepoutDoNotAllowZoneFills  = aOther.m_keepoutDoNotAllowZoneFills;
    m_keepoutDoNotAllowVias       = aOther.m_keepoutDoNotAllowVias;
    m_keepoutDoNotAllowTracks     = aOther.m_keepoutDoNotAllowTracks;
    m_keepoutDoNotAllowPads       = aOther.m_keepoutDoNotAllowPads;
    m_keepoutDoNotAllowFootprints = aOther.m_keepoutDoNotAllowFootprints;
    m_Locked                      = aOther.m_Locked;
    m_removeIslands               = aOther.m_removeIslands;
    m_minIslandArea               = aOther.m_minIslandArea;
    m_LayerProperties             = aOther.m_LayerProperties;

    if( aCopyFull )
    {
        m_TeardropType    = aOther.m_TeardropType;
        m_Layers          = aOther.m_Layers;
    }
}


void ZONE_SETTINGS::SetCornerRadius( int aRadius )
{
    if( aRadius < 0 )
        m_cornerRadius = 0;
    else
        m_cornerRadius = aRadius;
}


#ifdef __WXOSX_MAC__
const static wxSize LAYER_BITMAP_SIZE( 28, 28 );  // wxCocoa impl unhappy if this isn't square...
#else
const static wxSize LAYER_BITMAP_SIZE( 24, 16 );
#endif

const static wxSize CHECKERBOARD_SIZE( 8, 8 );


const ZONE_SETTINGS& ZONE_SETTINGS::GetDefaultSettings()
{
    static ZONE_SETTINGS defaultSettings;

    return defaultSettings;
}


// A helper for setting up a dialog list for specifying zone layers.  Used by all three
// zone settings dialogs.
void ZONE_SETTINGS::SetupLayersList( wxDataViewListCtrl* aList, PCB_BASE_FRAME* aFrame, LSET aLayers )
{
    BOARD* board = aFrame->GetBoard();
    COLOR4D backgroundColor = aFrame->GetColorSettings()->GetColor( LAYER_PCB_BACKGROUND );

    wxDataViewColumn* checkColumn = aList->AppendToggleColumn( wxEmptyString, wxDATAVIEW_CELL_ACTIVATABLE,
                                                               wxCOL_WIDTH_DEFAULT, wxALIGN_CENTER );

    wxDataViewColumn* layerColumn = aList->AppendIconTextColumn( wxEmptyString );
    wxDataViewColumn* layerIDColumn = aList->AppendTextColumn( wxEmptyString );
    layerIDColumn->SetHidden( true );

    int textWidth = 0;

    for( PCB_LAYER_ID layerID : aLayers.UIOrder() )
    {
        wxString layerName = board->GetLayerName( layerID );

        // wxCOL_WIDTH_AUTOSIZE doesn't work on all platforms, so we calculate width here
        textWidth = std::max( textWidth, KIUI::GetTextSize( layerName, aList ).x );

        COLOR4D layerColor = aFrame->GetColorSettings()->GetColor( layerID );
        auto bitmap = COLOR_SWATCH::MakeBitmap( layerColor, backgroundColor, LAYER_BITMAP_SIZE,
                                                CHECKERBOARD_SIZE, aList->GetBackgroundColour() );
        wxIcon icon;
        icon.CopyFromBitmap( bitmap );

        wxVector<wxVariant> row;
        row.push_back( wxVariant( m_Layers.test( layerID ) ) );
        row.push_back( wxVariant( wxDataViewIconText( layerName, icon ) ) );
        row.push_back( wxVariant( wxString::Format( wxT( "%i" ), layerID ) ) );
        aList->AppendItem( row );

        if( m_Layers.test( layerID ) )
            aList->SetToggleValue( true, (unsigned) aList->GetItemCount() - 1, 0 );
    }

    int checkColSize = aList->FromDIP( 22 );
    int layerColSize = textWidth + LAYER_BITMAP_SIZE.x + aList->FromDIP( 15 );

    // You'd think the fact that m_layers is a list would encourage wxWidgets not to save room
    // for the tree expanders... but you'd be wrong.  Force indent to 0.
    aList->SetIndent( 0 );

    checkColumn->SetWidth( checkColSize );
    layerColumn->SetWidth( layerColSize );

#ifdef __WXMAC__
    // TODO: something in wxWidgets 3.1.x pads checkbox columns with extra space.  (It used to
    // also be that the width of the column would get set too wide (to 30), but that's patched in
    // our local wxWidgets fork.)
    checkColSize += 50;
#endif

    aList->SetMinClientSize( wxSize( checkColSize + layerColSize, aList->GetMinClientSize().y ) );
}


LAYER_PROPERTIES_GRID_TABLE::LAYER_PROPERTIES_GRID_TABLE( PCB_BASE_FRAME* aFrame, std::function<LSET()> getLayers ) :
        m_frame( aFrame ),
        m_getLayersFunc( getLayers )
{
    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &LAYER_PROPERTIES_GRID_TABLE::onUnitsChanged, this );
}


LAYER_PROPERTIES_GRID_TABLE::~LAYER_PROPERTIES_GRID_TABLE()
{
    m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &LAYER_PROPERTIES_GRID_TABLE::onUnitsChanged, this );
}


wxString LAYER_PROPERTIES_GRID_TABLE::GetValue( int aRow, int aCol )
{
    VECTOR2I offset = m_items[aRow].second.hatching_offset.value_or( VECTOR2I() );

    switch( aCol )
    {
    case 1:  return m_frame->StringFromValue( offset.x, true );
    case 2:  return m_frame->StringFromValue( offset.y, true );

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        return wxT( "bad wxWidgets!" );
    }
}


void LAYER_PROPERTIES_GRID_TABLE::SetValue( int aRow, int aCol, const wxString& aValue )
{
    VECTOR2I offset = m_items[aRow].second.hatching_offset.value_or( VECTOR2I() );

    switch( aCol )
    {
    case 1:  offset.x = m_frame->ValueFromString( aValue );                           break;
    case 2:  offset.y = m_frame->ValueFromString( aValue );                           break;
    default: wxFAIL_MSG( wxString::Format( wxT( "column %d isn't a long" ), aCol ) ); break;
    }

    m_items[aRow].second.hatching_offset = offset;
}


long LAYER_PROPERTIES_GRID_TABLE::GetValueAsLong( int aRow, int aCol )
{
    switch( aCol )
    {
    case 0:  return m_items[aRow].first;
    default: wxFAIL_MSG( wxString::Format( wxT( "column %d isn't a long" ), aCol ) ); return -1;
    }
}


void LAYER_PROPERTIES_GRID_TABLE::SetValueAsLong( int aRow, int aCol, long aValue )
{
    switch( aCol )
    {
    case 0:  m_items[aRow].first = ToLAYER_ID( (int) aValue );                        break;
    default: wxFAIL_MSG( wxString::Format( wxT( "column %d isn't a long" ), aCol ) ); break;
    }
}


void LAYER_PROPERTIES_GRID_TABLE::AddItem( PCB_LAYER_ID aLayer, const ZONE_LAYER_PROPERTIES& aProps )
{
    m_items.emplace_back( std::make_pair( aLayer, aProps ) );

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
        GetView()->ProcessTableMessage( msg );
    }
}


bool LAYER_PROPERTIES_GRID_TABLE::AppendRows( size_t aNumRows )
{
    PCB_LAYER_ID next = UNDEFINED_LAYER;

    auto contains =
            [&]( PCB_LAYER_ID candidate )
            {
                for( const auto& [l, props] : m_items )
                {
                    if( candidate == l )
                        return true;
                }

                return false;
            };

    for( PCB_LAYER_ID layer : m_getLayersFunc().UIOrder() )
    {
        if( !contains( layer ) )
        {
            next = layer;
            break;
        }
    }

    if( next == UNDEFINED_LAYER )
        return false;

    AddItem( next, ZONE_LAYER_PROPERTIES( { VECTOR2I( 0, 0 ) } ) );

    return true;
}


bool LAYER_PROPERTIES_GRID_TABLE::DeleteRows( size_t aPos, size_t aNumRows )
{
    // aPos may be a large positive, e.g. size_t(-1), and the sum of
    // aPos+aNumRows may wrap here, so both ends of the range are tested.
    if( aPos < m_items.size() && aPos + aNumRows <= m_items.size() )
    {
        m_items.erase( m_items.begin() + (int) aPos, m_items.begin() + (int) aPos + (int) aNumRows );

        if( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, (int) aPos, (int) aNumRows );
            GetView()->ProcessTableMessage( msg );
        }

        return true;
    }

    return false;
}


void LAYER_PROPERTIES_GRID_TABLE::onUnitsChanged( wxCommandEvent& aEvent )
{
    if( GetView() )
        GetView()->ForceRefresh();

    aEvent.Skip();
}


