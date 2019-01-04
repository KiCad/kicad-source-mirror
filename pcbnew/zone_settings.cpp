/**
 * @brief class ZONE_SETTINGS used to handle zones parameters
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>

#include <convert_to_biu.h>
#include <pcbnew.h>
#include <pcb_base_frame.h>
#include <class_board.h>
#include <zones.h>

#include <class_zone.h>
#include <wx/dataview.h>
#include <widgets/color_swatch.h>

ZONE_SETTINGS::ZONE_SETTINGS()
{
    m_ZonePriority = 0;
    m_FillMode = ZFM_POLYGONS;                                             // Mode for filling zone : 1 use segments, 0 use polygons
    // Zone clearance value
    m_ZoneClearance      = Mils2iu( ZONE_CLEARANCE_MIL );
    // Min thickness value in filled areas (this is the minimum width of copper to fill solid areas) :
    m_ZoneMinThickness   = Mils2iu( ZONE_THICKNESS_MIL );
    m_NetcodeSelection   = 0;                                   // Net code selection for the current zone
    m_CurrentZone_Layer  = F_Cu;                                // Layer used to create the current zone
    m_Zone_HatchingStyle = ZONE_CONTAINER::DIAGONAL_EDGE;       // Option to show the zone area (outlines only, short hatches or full hatches

    m_ArcToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_HIGH_DEF; // Option to select number of segments to approximate a circle
                                                                // ARC_APPROX_SEGMENTS_COUNT_LOW_DEF
                                                                // or ARC_APPROX_SEGMENTS_COUNT_HIGH_DEF segments

    // thickness of the gap in thermal reliefs:
    m_ThermalReliefGap = Mils2iu( ZONE_THERMAL_RELIEF_GAP_MIL );
    // thickness of the copper bridge in thermal reliefs:
    m_ThermalReliefCopperBridge = Mils2iu( ZONE_THERMAL_RELIEF_COPPER_WIDTH_MIL );

    m_PadConnection = PAD_ZONE_CONN_THERMAL;                   // How pads are covered by copper in zone

    m_Zone_45_Only = false;

    m_cornerSmoothingType = SMOOTHING_NONE;
    m_cornerRadius = 0;

    SetIsKeepout( false );
    SetDoNotAllowCopperPour( false );
    SetDoNotAllowVias( true );
    SetDoNotAllowTracks( true );
}


ZONE_SETTINGS& ZONE_SETTINGS::operator << ( const ZONE_CONTAINER& aSource )
{
    m_ZonePriority = aSource.GetPriority();
    m_FillMode           = aSource.GetFillMode();
    m_ZoneClearance      = aSource.GetZoneClearance();
    m_ZoneMinThickness   = aSource.GetMinThickness();
    m_NetcodeSelection   = aSource.GetNetCode();
    m_Zone_HatchingStyle = aSource.GetHatchStyle();
    m_ArcToSegmentsCount = aSource.GetArcSegmentCount();
    m_ThermalReliefGap = aSource.GetThermalReliefGap();
    m_ThermalReliefCopperBridge = aSource.GetThermalReliefCopperBridge();
    m_PadConnection = aSource.GetPadConnection();
    m_cornerSmoothingType = aSource.GetCornerSmoothingType();
    m_cornerRadius = aSource.GetCornerRadius();
    m_isKeepout = aSource.GetIsKeepout();
    m_keepoutDoNotAllowCopperPour = aSource.GetDoNotAllowCopperPour();
    m_keepoutDoNotAllowVias = aSource.GetDoNotAllowVias();
    m_keepoutDoNotAllowTracks = aSource.GetDoNotAllowTracks();
    m_Zone_45_Only = aSource.GetHV45();

    m_CurrentZone_Layer  = aSource.GetLayer();
    m_Layers = aSource.GetLayerSet();

    return *this;
}


void ZONE_SETTINGS::ExportSetting( ZONE_CONTAINER& aTarget, bool aFullExport ) const
{
    aTarget.SetFillMode( m_FillMode );
    aTarget.SetZoneClearance( m_ZoneClearance );
    aTarget.SetMinThickness( m_ZoneMinThickness );
    aTarget.SetArcSegmentCount( m_ArcToSegmentsCount );
    aTarget.SetThermalReliefGap( m_ThermalReliefGap );
    aTarget.SetThermalReliefCopperBridge( m_ThermalReliefCopperBridge );
    aTarget.SetPadConnection( m_PadConnection );
    aTarget.SetCornerSmoothingType( m_cornerSmoothingType );
    aTarget.SetCornerRadius( m_cornerRadius );
    aTarget.SetIsKeepout( GetIsKeepout() );
    aTarget.SetDoNotAllowCopperPour( GetDoNotAllowCopperPour() );
    aTarget.SetDoNotAllowVias( GetDoNotAllowVias() );
    aTarget.SetDoNotAllowTracks( GetDoNotAllowTracks() );
    aTarget.SetHV45( m_Zone_45_Only );

    if( aFullExport )
    {
        aTarget.SetPriority( m_ZonePriority );
        aTarget.SetNetCode( m_NetcodeSelection );

        // Keepout zones can have multiple layers
        if( m_isKeepout )
        {
            aTarget.SetLayerSet( m_Layers );
        }
        else
        {
            aTarget.SetLayer( m_CurrentZone_Layer );
        }
    }

    // call SetHatch last, because hatch lines will be rebuilt,
    // using new parameters values
    aTarget.SetHatch( m_Zone_HatchingStyle, aTarget.GetDefaultHatchPitch(), true );
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

// A helper for setting up a dialog list for specifying zone layers.  Used by all three
// zone settings dialogs.
void ZONE_SETTINGS::SetupLayersList( wxDataViewListCtrl* aList, PCB_BASE_FRAME* aFrame,
                                     bool aShowCopper )
{
    BOARD* board = aFrame->GetBoard();
    COLOR4D backgroundColor = aFrame->Settings().Colors().GetLayerColor( LAYER_PCB_BACKGROUND );
    LSET layers = aShowCopper ? LSET::AllCuMask( board->GetCopperLayerCount() )
                              : LSET::AllNonCuMask();

    wxDataViewColumn* checkColumn = aList->AppendToggleColumn( wxEmptyString );
    wxDataViewColumn* layerColumn = aList->AppendIconTextColumn( wxEmptyString );
    wxDataViewColumn* layerIDColumn = aList->AppendTextColumn( wxEmptyString );
    layerIDColumn->SetHidden( true );

    int textWidth = 0;

    for( LSEQ layer = layers.UIOrder(); layer; ++layer )
    {
        PCB_LAYER_ID layerID = *layer;
        wxString     layerName = board->GetLayerName( layerID );

        // wxCOL_WIDTH_AUTOSIZE doesn't work on all platforms, so we calculate width here
        textWidth = std::max( textWidth, GetTextSize( layerName, aList ).x );

        COLOR4D layerColor = aFrame->Settings().Colors().GetLayerColor( layerID );
        auto bitmap = COLOR_SWATCH::MakeBitmap( layerColor, backgroundColor, LAYER_BITMAP_SIZE );
        wxIcon icon;
        icon.CopyFromBitmap( bitmap );

        wxVector<wxVariant> row;
        row.push_back( wxVariant( m_Layers.test( layerID ) ) );
        row.push_back( wxVariant( wxDataViewIconText( layerName, icon ) ) );
        row.push_back( wxVariant( wxString::Format( "%i", layerID ) ) );
        aList->AppendItem( row );

        if( m_CurrentZone_Layer == layerID )
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


