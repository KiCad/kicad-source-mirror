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
    m_HatchHoleMinArea = 0.3;       // Min size before holes are dropped (ratio of hole size)
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
    SetRuleAreaPlacementSourceType( RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME );
    SetDoNotAllowCopperPour( false );
    SetDoNotAllowVias( true );
    SetDoNotAllowTracks( true );
    SetDoNotAllowPads( true );
    SetDoNotAllowFootprints( false );

    m_TeardropType = TEARDROP_TYPE::TD_NONE;
    m_ruleAreaPlacementEnabled = false;
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
    if( m_ruleAreaPlacementEnabled != aOther.m_ruleAreaPlacementEnabled )
        return false;
    if( m_ruleAreaPlacementSourceType != aOther.m_ruleAreaPlacementSourceType ) return false;
    if( m_ruleAreaPlacementSource     != aOther.m_ruleAreaPlacementSource ) return false;
    if( m_keepoutDoNotAllowCopperPour != aOther.m_keepoutDoNotAllowCopperPour ) return false;
    if( m_keepoutDoNotAllowVias       != aOther.m_keepoutDoNotAllowVias ) return false;
    if( m_keepoutDoNotAllowTracks     != aOther.m_keepoutDoNotAllowTracks ) return false;
    if( m_keepoutDoNotAllowPads       != aOther.m_keepoutDoNotAllowPads ) return false;
    if( m_keepoutDoNotAllowFootprints != aOther.m_keepoutDoNotAllowFootprints ) return false;
    if( m_Locked                      != aOther.m_Locked ) return false;
    if( m_removeIslands               != aOther.m_removeIslands ) return false;
    if( m_minIslandArea               != aOther.m_minIslandArea ) return false;

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
    m_ruleAreaPlacementEnabled = aSource.GetRuleAreaPlacementEnabled();
    m_ruleAreaPlacementSourceType = aSource.GetRuleAreaPlacementSourceType();
    m_ruleAreaPlacementSource     = aSource.GetRuleAreaPlacementSource();
    m_keepoutDoNotAllowCopperPour = aSource.GetDoNotAllowCopperPour();
    m_keepoutDoNotAllowVias       = aSource.GetDoNotAllowVias();
    m_keepoutDoNotAllowTracks     = aSource.GetDoNotAllowTracks();
    m_keepoutDoNotAllowPads       = aSource.GetDoNotAllowPads();
    m_keepoutDoNotAllowFootprints = aSource.GetDoNotAllowFootprints();
    m_Locked                      = aSource.IsLocked();
    m_removeIslands               = aSource.GetIslandRemovalMode();
    m_minIslandArea               = aSource.GetMinIslandArea();

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
    aTarget.SetRuleAreaPlacementEnabled( GetRuleAreaPlacementEnabled() );
    aTarget.SetRuleAreaPlacementSourceType( GetRuleAreaPlacementSourceType() );
    aTarget.SetRuleAreaPlacementSource( GetRuleAreaPlacementSource() );
    aTarget.SetDoNotAllowCopperPour( GetDoNotAllowCopperPour() );
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
void ZONE_SETTINGS::SetupLayersList( wxDataViewListCtrl* aList, PCB_BASE_FRAME* aFrame,
                                     LSET aLayers, bool aFpEditorMode )
{
    BOARD* board = aFrame->GetBoard();
    COLOR4D backgroundColor = aFrame->GetColorSettings()->GetColor( LAYER_PCB_BACKGROUND );

    // In the Footprint Editor In1_Cu is used as a proxy for "all inner layers"
    if( aFpEditorMode )
        aLayers.set( In1_Cu );

    wxDataViewColumn* checkColumn = aList->AppendToggleColumn(
            wxEmptyString, wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_DEFAULT, wxALIGN_CENTER );

    wxDataViewColumn* layerColumn = aList->AppendIconTextColumn( wxEmptyString );
    wxDataViewColumn* layerIDColumn = aList->AppendTextColumn( wxEmptyString );
    layerIDColumn->SetHidden( true );

    int textWidth = 0;

    for( PCB_LAYER_ID layerID : aLayers.UIOrder() )
    {
        wxString layerName = board->GetLayerName( layerID );

        if( aFpEditorMode && layerID == In1_Cu )
            layerName = _( "Inner layers" );

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
    aList->SetMinClientSize( wxSize( checkColSize + layerColSize,
                                     aList->GetMinClientSize().y ) );

    checkColumn->SetWidth( checkColSize );
    layerColumn->SetWidth( layerColSize );
}


