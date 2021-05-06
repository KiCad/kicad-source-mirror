/**
 * @brief class ZONE_SETTINGS used to handle zones parameters
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <convert_to_biu.h>
#include <pcbnew.h>
#include <pcb_base_frame.h>
#include <board.h>
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
    m_ZoneClearance = Mils2iu( ZONE_CLEARANCE_MIL );
    // Min thickness value in filled areas (this is the minimum width of copper to fill solid areas) :
    m_ZoneMinThickness = Mils2iu( ZONE_THICKNESS_MIL );
    m_HatchThickness = 0;        // good value of grid line thickness if m_FillMode = ZFM_GRID_PATTERN
    m_HatchGap = 0;              // good value  of grid line gap if m_FillMode = ZFM_GRID_PATTERN
    m_HatchOrientation = 0.0;    // Grid style: orientation of grid lines in degrees
    m_HatchSmoothingLevel = 0;   // Grid pattern smoothing type. 0 = no smoothing
    m_HatchSmoothingValue = 0.1; // Grid pattern chamfer value relative to the gap value
    m_HatchHoleMinArea = 0.3;    // Min size before holes are dropped (ratio of hole size)
    m_HatchBorderAlgorithm = 1;  // 0 = use zone min thickness; 1 = use hatch width
    m_NetcodeSelection = 0;      // Net code selection for the current zone
    m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE; // Option to show the zone
                                                                         // outlines only, short
                                                                         // hatches or full hatches

    m_Layers.reset().set( F_Cu );
    m_Name = wxEmptyString;

    // thickness of the gap in thermal reliefs:
    m_ThermalReliefGap = Mils2iu( ZONE_THERMAL_RELIEF_GAP_MIL );
    // thickness of the copper bridge in thermal reliefs:
    m_ThermalReliefSpokeWidth = Mils2iu( ZONE_THERMAL_RELIEF_COPPER_WIDTH_MIL );

    m_padConnection = ZONE_CONNECTION::THERMAL; // How pads are covered by copper in zone

    m_Zone_45_Only = false;

    m_cornerSmoothingType = SMOOTHING_NONE;
    m_cornerRadius = 0;

    m_removeIslands = ISLAND_REMOVAL_MODE::ALWAYS;
    m_minIslandArea = 0;

    SetIsRuleArea( false );
    SetDoNotAllowCopperPour( false );
    SetDoNotAllowVias( true );
    SetDoNotAllowTracks( true );
    SetDoNotAllowPads( true );
    SetDoNotAllowFootprints( false );
}


ZONE_SETTINGS& ZONE_SETTINGS::operator << ( const ZONE& aSource )
{
    m_ZonePriority                = aSource.GetPriority();
    m_FillMode                    = aSource.GetFillMode();
    m_ZoneClearance               = aSource.GetLocalClearance();
    m_ZoneMinThickness            = aSource.GetMinThickness();
    m_HatchThickness              = aSource.GetHatchThickness();
    m_HatchGap                    = aSource.GetHatchGap();
    m_HatchOrientation            = aSource.GetHatchOrientation();
    m_HatchSmoothingLevel         = aSource.GetHatchSmoothingLevel();
    m_HatchSmoothingValue         = aSource.GetHatchSmoothingValue();
    m_HatchBorderAlgorithm        = aSource.GetHatchBorderAlgorithm();
    m_HatchHoleMinArea            = aSource.GetHatchHoleMinArea();
    m_NetcodeSelection            = aSource.GetNetCode();
    m_Name                        = aSource.GetZoneName();
    m_ZoneBorderDisplayStyle      = aSource.GetHatchStyle();
    m_ThermalReliefGap            = aSource.GetThermalReliefGap();
    m_ThermalReliefSpokeWidth   = aSource.GetThermalReliefSpokeWidth();
    m_padConnection               = aSource.GetPadConnection();
    m_cornerSmoothingType         = aSource.GetCornerSmoothingType();
    m_cornerRadius                = aSource.GetCornerRadius();
    m_isRuleArea                  = aSource.GetIsRuleArea();
    m_keepoutDoNotAllowCopperPour = aSource.GetDoNotAllowCopperPour();
    m_keepoutDoNotAllowVias       = aSource.GetDoNotAllowVias();
    m_keepoutDoNotAllowTracks     = aSource.GetDoNotAllowTracks();
    m_keepoutDoNotAllowPads       = aSource.GetDoNotAllowPads();
    m_keepoutDoNotAllowFootprints = aSource.GetDoNotAllowFootprints();
    m_Zone_45_Only                = aSource.GetHV45();
    m_Locked                      = aSource.IsLocked();
    m_removeIslands               = aSource.GetIslandRemovalMode();
    m_minIslandArea               = aSource.GetMinIslandArea();

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
    aTarget.SetDoNotAllowCopperPour( GetDoNotAllowCopperPour() );
    aTarget.SetDoNotAllowVias( GetDoNotAllowVias() );
    aTarget.SetDoNotAllowTracks( GetDoNotAllowTracks() );
    aTarget.SetDoNotAllowPads( GetDoNotAllowPads() );
    aTarget.SetDoNotAllowFootprints( GetDoNotAllowFootprints() );
    aTarget.SetHV45( m_Zone_45_Only );
    aTarget.SetLocked( m_Locked );
    aTarget.SetIslandRemovalMode( GetIslandRemovalMode() );
    aTarget.SetMinIslandArea( GetMinIslandArea() );

    if( aFullExport )
    {
        aTarget.SetPriority( m_ZonePriority );
        aTarget.SetLayerSet( m_Layers );
        aTarget.SetZoneName( m_Name );

        if( !m_isRuleArea )
            aTarget.SetNetCode( m_NetcodeSelection );
    }

    // call SetBorderDisplayStyle last, because hatch lines will be rebuilt,
    // using new parameters values
    aTarget.SetBorderDisplayStyle( m_ZoneBorderDisplayStyle, aTarget.GetDefaultHatchPitch(),
                                   true );
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


// A helper for setting up a dialog list for specifying zone layers.  Used by all three
// zone settings dialogs.
void ZONE_SETTINGS::SetupLayersList( wxDataViewListCtrl* aList, PCB_BASE_FRAME* aFrame,
                                     bool aShowCopper, bool aFpEditorMode )
{
    BOARD* board = aFrame->GetBoard();
    COLOR4D backgroundColor = aFrame->GetColorSettings()->GetColor( LAYER_PCB_BACKGROUND );
    LSET layers = aShowCopper ? LSET::AllCuMask( board->GetCopperLayerCount() )
                              : LSET::AllNonCuMask();

    // In the Footprint Editor In1_Cu is used as a proxy for "all inner layers"
    if( aFpEditorMode )
        layers.set( In1_Cu );

    wxDataViewColumn* checkColumn = aList->AppendToggleColumn( wxEmptyString );
    wxDataViewColumn* layerColumn = aList->AppendIconTextColumn( wxEmptyString );
    wxDataViewColumn* layerIDColumn = aList->AppendTextColumn( wxEmptyString );
    layerIDColumn->SetHidden( true );

    int textWidth = 0;

    for( LSEQ layer = layers.UIOrder(); layer; ++layer )
    {
        PCB_LAYER_ID layerID = *layer;
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
        row.push_back( wxVariant( wxString::Format( "%i", layerID ) ) );
        aList->AppendItem( row );

        if( m_Layers.test( layerID ) )
            aList->SetToggleValue( true, (unsigned) aList->GetItemCount() - 1, 0 );
    }

    int checkColSize = 22;
    int layerColSize = textWidth + LAYER_BITMAP_SIZE.x + 15;

    // You'd think the fact that m_layers is a list would encourage wxWidgets not to save room
    // for the tree expanders... but you'd be wrong.  Force indent to 0.
    aList->SetIndent( 0 );
    aList->SetMinClientSize( wxSize( checkColSize + layerColSize, aList->GetMinClientSize().y ) );

    checkColumn->SetWidth( checkColSize );
    layerColumn->SetWidth( layerColSize );
}


