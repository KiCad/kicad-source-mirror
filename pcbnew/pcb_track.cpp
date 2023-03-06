/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_base_frame.h>
#include <core/mirror.h>
#include <connectivity/connectivity_data.h>
#include <board.h>
#include <board_design_settings.h>
#include <convert_basic_shapes_to_polygon.h>
#include <pcb_track.h>
#include <base_units.h>
#include <bitmaps.h>
#include <string_utils.h>
#include <view/view.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <i18n_utility.h>
#include <geometry/seg.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_arc.h>
#include <drc/drc_engine.h>
#include <pcb_painter.h>
#include <trigo.h>

using KIGFX::PCB_PAINTER;
using KIGFX::PCB_RENDER_SETTINGS;

PCB_TRACK::PCB_TRACK( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_CONNECTED_ITEM( aParent, idtype )
{
    m_Width = pcbIUScale.mmToIU( 0.2 );     // Gives a reasonable default width
    m_CachedScale = -1.0;                   // Set invalid to force update
    m_CachedLOD = 0.0;                      // Set to always display
}


EDA_ITEM* PCB_TRACK::Clone() const
{
    return new PCB_TRACK( *this );
}


PCB_ARC::PCB_ARC( BOARD_ITEM* aParent, const SHAPE_ARC* aArc ) :
    PCB_TRACK( aParent, PCB_ARC_T )
{
    m_Start = aArc->GetP0();
    m_End = aArc->GetP1();
    m_Mid = aArc->GetArcMid();
}


EDA_ITEM* PCB_ARC::Clone() const
{
    return new PCB_ARC( *this );
}


PCB_VIA::PCB_VIA( BOARD_ITEM* aParent ) :
        PCB_TRACK( aParent, PCB_VIA_T )
{
    SetViaType( VIATYPE::THROUGH );
    m_bottomLayer = B_Cu;
    SetDrillDefault();

    m_removeUnconnectedLayer = false;
    m_keepStartEndLayer = true;

    m_zoneLayerOverrides.fill( ZLO_NONE );

    m_isFree = false;
}


PCB_VIA::PCB_VIA( const PCB_VIA& aOther ) :
        PCB_TRACK( aOther.GetParent(), PCB_VIA_T )
{
    PCB_VIA::operator=( aOther );

    const_cast<KIID&>( m_Uuid ) = aOther.m_Uuid;
    m_zoneLayerOverrides = aOther.m_zoneLayerOverrides;
}


PCB_VIA& PCB_VIA::operator=( const PCB_VIA &aOther )
{
    BOARD_CONNECTED_ITEM::operator=( aOther );

    m_Width = aOther.m_Width;
    m_Start = aOther.m_Start;
    m_End = aOther.m_End;
    m_CachedLOD = aOther.m_CachedLOD;
    m_CachedScale = aOther.m_CachedScale;

    m_bottomLayer = aOther.m_bottomLayer;
    m_viaType = aOther.m_viaType;
    m_drill = aOther.m_drill;
    m_removeUnconnectedLayer = aOther.m_removeUnconnectedLayer;
    m_keepStartEndLayer = aOther.m_keepStartEndLayer;
    m_isFree = aOther.m_isFree;

    return *this;
}


EDA_ITEM* PCB_VIA::Clone() const
{
    return new PCB_VIA( *this );
}


wxString PCB_VIA::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    wxString formatStr;

    switch( GetViaType() )
    {
    case VIATYPE::BLIND_BURIED: formatStr = _( "Blind/Buried Via %s on %s" ); break;
    case VIATYPE::MICROVIA:     formatStr = _( "Micro Via %s on %s" );        break;
    default:                    formatStr = _( "Via %s on %s" );              break;
    }

    return wxString::Format( formatStr, GetNetnameMsg(), layerMaskDescribe() );
}


BITMAPS PCB_VIA::GetMenuImage() const
{
    return BITMAPS::via;
}


bool PCB_TRACK::ApproxCollinear( const PCB_TRACK& aTrack )
{
    SEG a( m_Start, m_End );
    SEG b( aTrack.GetStart(), aTrack.GetEnd() );
    return a.ApproxCollinear( b );
}


int PCB_TRACK::GetLocalClearance( wxString* aSource ) const
{
    // Not currently implemented
    return 0;
}


MINOPTMAX<int> PCB_TRACK::GetWidthConstraint( wxString* aSource ) const
{
    DRC_CONSTRAINT constraint;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

        constraint = bds.m_DRCEngine->EvalRules( TRACK_WIDTH_CONSTRAINT, this, nullptr, m_layer );
    }

    if( aSource )
        *aSource = constraint.GetName();

    return constraint.Value();
}


int PCB_VIA::GetMinAnnulus( PCB_LAYER_ID aLayer, wxString* aSource ) const
{
    if( !FlashLayer( aLayer ) )
    {
        if( aSource )
            *aSource = _( "removed annular ring" );

        return 0;
    }

    DRC_CONSTRAINT constraint;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

        constraint = bds.m_DRCEngine->EvalRules( ANNULAR_WIDTH_CONSTRAINT, this, nullptr, aLayer );
    }

    if( constraint.Value().HasMin() )
    {
        if( aSource )
            *aSource = constraint.GetName();

        return constraint.Value().Min();
    }

    return 0;
}


int PCB_VIA::GetDrillValue() const
{
    if( m_drill > 0 ) // Use the specific value.
        return m_drill;

    // Use the default value from the Netclass
    NETCLASS* netclass = GetEffectiveNetClass();

    if( GetViaType() == VIATYPE::MICROVIA )
        return netclass->GetuViaDrill();

    return netclass->GetViaDrill();
}


EDA_ITEM_FLAGS PCB_TRACK::IsPointOnEnds( const VECTOR2I& point, int min_dist ) const
{
    EDA_ITEM_FLAGS result = 0;

    if( min_dist < 0 )
        min_dist = m_Width / 2;

    if( min_dist == 0 )
    {
        if( m_Start == point  )
            result |= STARTPOINT;

        if( m_End == point )
            result |= ENDPOINT;
    }
    else
    {
        double dist = GetLineLength( m_Start, point );

        if( min_dist >= KiROUND( dist ) )
            result |= STARTPOINT;

        dist = GetLineLength( m_End, point );

        if( min_dist >= KiROUND( dist ) )
            result |= ENDPOINT;
    }

    return result;
}


const BOX2I PCB_TRACK::GetBoundingBox() const
{
    // end of track is round, this is its radius, rounded up
    int radius = ( m_Width + 1 ) / 2;
    int ymax, xmax, ymin, xmin;

    if( Type() == PCB_VIA_T )
    {
        ymax = m_Start.y;
        xmax = m_Start.x;

        ymin = m_Start.y;
        xmin = m_Start.x;
    }
    else if( Type() == PCB_ARC_T )
    {
        std::shared_ptr<SHAPE> arc = GetEffectiveShape();
        BOX2I bbox = arc->BBox();

        xmin = bbox.GetLeft();
        xmax = bbox.GetRight();
        ymin = bbox.GetTop();
        ymax = bbox.GetBottom();
    }
    else
    {
        ymax = std::max( m_Start.y, m_End.y );
        xmax = std::max( m_Start.x, m_End.x );

        ymin = std::min( m_Start.y, m_End.y );
        xmin = std::min( m_Start.x, m_End.x );
    }

    ymax += radius;
    xmax += radius;

    ymin -= radius;
    xmin -= radius;

    // return a rectangle which is [pos,dim) in nature.  therefore the +1
    BOX2I ret( VECTOR2I( xmin, ymin ), VECTOR2I( xmax - xmin + 1, ymax - ymin + 1 ) );

    return ret;
}


double PCB_TRACK::GetLength() const
{
    return GetLineLength( m_Start, m_End );
}


void PCB_TRACK::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_Start, aRotCentre, aAngle );
    RotatePoint( m_End, aRotCentre, aAngle );
}


void PCB_ARC::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_Start, aRotCentre, aAngle );
    RotatePoint( m_End, aRotCentre, aAngle );
    RotatePoint( m_Mid, aRotCentre, aAngle );
}


void PCB_TRACK::Mirror( const VECTOR2I& aCentre, bool aMirrorAroundXAxis )
{
    if( aMirrorAroundXAxis )
    {
        MIRROR( m_Start.y, aCentre.y );
        MIRROR( m_End.y, aCentre.y );
    }
    else
    {
        MIRROR( m_Start.x, aCentre.x );
        MIRROR( m_End.x, aCentre.x );
    }
}


void PCB_ARC::Mirror( const VECTOR2I& aCentre, bool aMirrorAroundXAxis )
{
    if( aMirrorAroundXAxis )
    {
        MIRROR( m_Start.y, aCentre.y );
        MIRROR( m_End.y, aCentre.y );
        MIRROR( m_Mid.y, aCentre.y );
    }
    else
    {
        MIRROR( m_Start.x, aCentre.x );
        MIRROR( m_End.x, aCentre.x );
        MIRROR( m_Mid.x, aCentre.x );
    }
}


void PCB_TRACK::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        m_Start.x = aCentre.x - ( m_Start.x - aCentre.x );
        m_End.x   = aCentre.x - ( m_End.x - aCentre.x );
    }
    else
    {
        m_Start.y = aCentre.y - ( m_Start.y - aCentre.y );
        m_End.y   = aCentre.y - ( m_End.y - aCentre.y );
    }

    int copperLayerCount = GetBoard()->GetCopperLayerCount();
    SetLayer( FlipLayer( GetLayer(), copperLayerCount ) );
}


void PCB_ARC::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        m_Start.x = aCentre.x - ( m_Start.x - aCentre.x );
        m_End.x   = aCentre.x - ( m_End.x - aCentre.x );
        m_Mid.x = aCentre.x - ( m_Mid.x - aCentre.x );
    }
    else
    {
        m_Start.y = aCentre.y - ( m_Start.y - aCentre.y );
        m_End.y   = aCentre.y - ( m_End.y - aCentre.y );
        m_Mid.y = aCentre.y - ( m_Mid.y - aCentre.y );
    }

    int copperLayerCount = GetBoard()->GetCopperLayerCount();
    SetLayer( FlipLayer( GetLayer(), copperLayerCount ) );
}


bool PCB_ARC::IsCCW() const
{
    VECTOR2I start_end = m_End - m_Start;
    VECTOR2I start_mid = m_Mid - m_Start;

    return start_end.Cross( start_mid ) < 0;
}


void PCB_VIA::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        m_Start.x = aCentre.x - ( m_Start.x - aCentre.x );
        m_End.x   = aCentre.x - ( m_End.x - aCentre.x );
    }
    else
    {
        m_Start.y = aCentre.y - ( m_Start.y - aCentre.y );
        m_End.y   = aCentre.y - ( m_End.y - aCentre.y );
    }

    if( GetViaType() != VIATYPE::THROUGH )
    {
        int          copperLayerCount = GetBoard()->GetCopperLayerCount();
        PCB_LAYER_ID top_layer;
        PCB_LAYER_ID bottom_layer;
        LayerPair( &top_layer, &bottom_layer );
        top_layer    = FlipLayer( top_layer, copperLayerCount );
        bottom_layer = FlipLayer( bottom_layer, copperLayerCount );
        SetLayerPair( top_layer, bottom_layer );
    }
}


INSPECT_RESULT PCB_TRACK::Visit( INSPECTOR inspector, void* testData,
                                 const std::vector<KICAD_T>& aScanTypes )
{
    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == Type() )
        {
            if( INSPECT_RESULT::QUIT == inspector( this, testData ) )
                return INSPECT_RESULT::QUIT;
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


std::shared_ptr<SHAPE_SEGMENT> PCB_VIA::GetEffectiveHoleShape() const
{
    return std::make_shared<SHAPE_SEGMENT>( SEG( m_Start, m_Start ), m_drill );
}


bool PCB_VIA::IsTented() const
{
    const BOARD* board = GetBoard();

    if( board )
        return board->GetTentVias();
    else
        return true;
}


int PCB_VIA::GetSolderMaskExpansion() const
{
    const BOARD* board = GetBoard();

    if( board )
        return board->GetDesignSettings().m_SolderMaskExpansion;
    else
        return 0;
}


bool PCB_VIA::IsOnLayer( PCB_LAYER_ID aLayer, bool aIncludeCourtyards ) const
{
#if 0
    // Nice and simple, but raises its ugly head in performance profiles....
    return GetLayerSet().test( aLayer );
#endif

    if( aLayer >= m_layer && aLayer <= m_bottomLayer )
        return true;

    if( !IsTented() )
    {
        if( aLayer == F_Mask )
            return IsOnLayer( F_Cu );
        else if( aLayer == B_Mask )
            return IsOnLayer( B_Cu );
    }

    return false;
}


LSET PCB_VIA::GetLayerSet() const
{
    LSET layermask;

    if( GetViaType() == VIATYPE::THROUGH )
        layermask = LSET::AllCuMask();
    else
        wxASSERT( m_layer <= m_bottomLayer );

    // PCB_LAYER_IDs are numbered from front to back, this is top to bottom.
    for( int id = m_layer; id <= m_bottomLayer; ++id )
        layermask.set( id );

    if( !IsTented() )
    {
        if( layermask.test( F_Cu ) )
            layermask.set( F_Mask );

        if( layermask.test( B_Cu ) )
            layermask.set( B_Mask );
    }

    return layermask;
}


void PCB_VIA::SetLayerSet( LSET aLayerSet )
{
    bool first = true;

    for( PCB_LAYER_ID layer : aLayerSet.Seq() )
    {
        if( first )
        {
            m_layer = layer;
            first = false;
        }

        m_bottomLayer = layer;
    }
}


void PCB_VIA::SetLayerPair( PCB_LAYER_ID aTopLayer, PCB_LAYER_ID aBottomLayer )
{

    m_layer = aTopLayer;
    m_bottomLayer = aBottomLayer;
    SanitizeLayers();
}


void PCB_VIA::SetTopLayer( PCB_LAYER_ID aLayer )
{
    m_layer = aLayer;
}


void PCB_VIA::SetBottomLayer( PCB_LAYER_ID aLayer )
{
    m_bottomLayer = aLayer;
}


void PCB_VIA::LayerPair( PCB_LAYER_ID* top_layer, PCB_LAYER_ID* bottom_layer ) const
{
    PCB_LAYER_ID t_layer = F_Cu;
    PCB_LAYER_ID b_layer = B_Cu;

    if( GetViaType() != VIATYPE::THROUGH )
    {
        b_layer = m_bottomLayer;
        t_layer = m_layer;

        if( b_layer < t_layer )
            std::swap( b_layer, t_layer );
    }

    if( top_layer )
        *top_layer = t_layer;

    if( bottom_layer )
        *bottom_layer = b_layer;
}


PCB_LAYER_ID PCB_VIA::TopLayer() const
{
    return m_layer;
}


PCB_LAYER_ID PCB_VIA::BottomLayer() const
{
    return m_bottomLayer;
}


void PCB_VIA::SanitizeLayers()
{
    if( GetViaType() == VIATYPE::THROUGH )
    {
        m_layer       = F_Cu;
        m_bottomLayer = B_Cu;
    }

    if( m_bottomLayer < m_layer )
        std::swap( m_bottomLayer, m_layer );
}


bool PCB_VIA::FlashLayer( LSET aLayers ) const
{
    for( auto layer : aLayers.Seq() )
    {
        if( FlashLayer( layer ) )
            return true;
    }

    return false;
}


bool PCB_VIA::FlashLayer( int aLayer ) const
{
    // Return the "normal" shape if the caller doesn't specify a particular layer
    if( aLayer == UNDEFINED_LAYER )
        return true;

    const BOARD* board = GetBoard();

    if( !board )
        return true;

    if( !IsOnLayer( static_cast<PCB_LAYER_ID>( aLayer ) ) )
        return false;

    if( !m_removeUnconnectedLayer )
    {
        return true;
    }

    if( m_keepStartEndLayer && ( aLayer == m_layer || aLayer == m_bottomLayer ) )
        return true;

    // Must be static to keep from raising its ugly head in performance profiles
    static std::initializer_list<KICAD_T> connectedTypes = { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T,
                                                             PCB_PAD_T };

    if( m_zoneLayerOverrides[ aLayer ] == ZLO_FORCE_FLASHED )
        return true;
    else
        return board->GetConnectivity()->IsConnectedOnLayer( this, aLayer, connectedTypes );
}


void PCB_TRACK::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Show the track and its netname on different layers
    aLayers[0] = GetLayer();
    aLayers[1] = GetNetnameLayer( aLayers[0] );
    aCount = 2;

    if( IsLocked() )
        aLayers[ aCount++ ] = LAYER_LOCKED_ITEM_SHADOW;
}


double PCB_TRACK::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    PCB_PAINTER*         painter = static_cast<PCB_PAINTER*>( aView->GetPainter() );
    PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();

    if( !aView->IsLayerVisible( LAYER_TRACKS ) )
        return HIDE;

    if( IsNetnameLayer( aLayer ) )
    {
        if( GetNetCode() <= NETINFO_LIST::UNCONNECTED )
            return HIDE;

        // Hide netnames on dimmed tracks
        if( renderSettings->GetHighContrast() )
        {
            if( m_layer != renderSettings->GetPrimaryHighContrastLayer() )
                return HIDE;
        }

        // Pick the approximate size of the netname (square chars)
        wxString netName = GetUnescapedShortNetname();
        size_t  num_chars = netName.size();

        if( GetLength() < num_chars * GetWidth() )
            return HIDE;

        // When drawing netnames, clip the track to the viewport
        VECTOR2I start( GetStart() );
        VECTOR2I end( GetEnd() );
        BOX2D    viewport = aView->GetViewport();
        BOX2I    clipBox( viewport.GetOrigin(), viewport.GetSize() );

        ClipLine( &clipBox, start.x, start.y, end.x, end.y );

        VECTOR2I line = ( end - start );

        if( line.EuclideanNorm() == 0 )
            return HIDE;

        // Netnames will be shown only if zoom is appropriate
        return ( double ) pcbIUScale.mmToIU( 4 ) / ( m_Width + 1 );
    }

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Hide shadow if the main layer is not shown
        if( !aView->IsLayerVisible( m_layer ) )
            return HIDE;

        // Hide shadow on dimmed tracks
        if( renderSettings->GetHighContrast() )
        {
            if( m_layer != renderSettings->GetPrimaryHighContrastLayer() )
                return HIDE;
        }
    }

    // Other layers are shown without any conditions
    return 0.0;
}


const BOX2I PCB_TRACK::ViewBBox() const
{
    BOX2I        bbox = GetBoundingBox();
    const BOARD* board = GetBoard();

    if( board )
        bbox.Inflate( 2 * board->GetDesignSettings().GetBiggestClearanceValue() );
    else
        bbox.Inflate( GetWidth() );     // Add a bit extra for safety

    return bbox;
}


void PCB_VIA::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aLayers[0] = LAYER_VIA_HOLES;
    aLayers[1] = LAYER_VIA_HOLEWALLS;
    aLayers[2] = LAYER_VIA_NETNAMES;

    // Just show it on common via & via holes layers
    switch( GetViaType() )
    {
    case VIATYPE::THROUGH:      aLayers[3] = LAYER_VIA_THROUGH;  break;
    case VIATYPE::BLIND_BURIED: aLayers[3] = LAYER_VIA_BBLIND;   break;
    case VIATYPE::MICROVIA:     aLayers[3] = LAYER_VIA_MICROVIA; break;
    default:                    aLayers[3] = LAYER_GP_OVERLAY;   break;
    }

    aCount = 4;

    if( IsLocked() )
        aLayers[ aCount++ ] = LAYER_LOCKED_ITEM_SHADOW;
}


double PCB_VIA::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = (double)std::numeric_limits<double>::max();

    PCB_PAINTER*         painter = static_cast<PCB_PAINTER*>( aView->GetPainter() );
    PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();
    const BOARD*         board = GetBoard();
    LSET                 visible = LSET::AllLayersMask();

    // Meta control for hiding all vias
    if( !aView->IsLayerVisible( LAYER_VIAS ) )
        return HIDE;

    // Handle board visibility
    if( board )
        visible = board->GetVisibleLayers() & board->GetEnabledLayers();

    // In high contrast mode don't show vias that don't cross the high-contrast layer
    if( renderSettings->GetHighContrast() )
    {
        PCB_LAYER_ID highContrastLayer = renderSettings->GetPrimaryHighContrastLayer();

        if( LSET::FrontTechMask().Contains( highContrastLayer ) )
            highContrastLayer = F_Cu;
        else if( LSET::BackTechMask().Contains( highContrastLayer ) )
            highContrastLayer = B_Cu;

        if( !GetLayerSet().Contains( highContrastLayer ) )
            return HIDE;
    }

    if( IsHoleLayer( aLayer ) )
    {
        if( m_viaType == VIATYPE::BLIND_BURIED || m_viaType == VIATYPE::MICROVIA )
        {
            // Show a blind or micro via's hole if it crosses a visible layer
            if( !( visible & GetLayerSet() ).any() )
                return HIDE;
        }
        else
        {
            // Show a through via's hole if any physical layer is shown
            if( !( visible & LSET::PhysicalLayersMask() ).any() )
                return HIDE;
        }
    }
    else if( IsNetnameLayer( aLayer ) )
    {
        if( renderSettings->GetHighContrast() )
        {
            // Hide netnames unless via is flashed to a high-contrast layer
            if( !FlashLayer( renderSettings->GetPrimaryHighContrastLayer() ) )
                return HIDE;
        }
        else
        {
            // Hide netnames unless pad is flashed to a visible layer
            if( !FlashLayer( visible ) )
                return HIDE;
        }

        // Netnames will be shown only if zoom is appropriate
        return m_Width == 0 ? HIDE : ( (double)pcbIUScale.mmToIU( 10 ) / m_Width );
    }

    // Passed all tests; show.
    return 0.0;
}


wxString PCB_TRACK::GetFriendlyName() const
{
    switch( Type() )
    {
    case PCB_ARC_T:     return _( "Track (arc)" );
    case PCB_VIA_T:     return _( "Via" );
    case PCB_TRACE_T:
    default:            return _( "Track" );
    }
}


void PCB_TRACK::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString  msg;
    BOARD*    board = GetBoard();

    aList.emplace_back( _( "Type" ), GetFriendlyName() );

    GetMsgPanelInfoBase_Common( aFrame, aList );

    aList.emplace_back( _( "Layer" ), layerMaskDescribe() );

    aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( m_Width ) );

    if( Type() == PCB_ARC_T )
    {
        double radius = static_cast<PCB_ARC*>( this )->GetRadius();
        aList.emplace_back( _( "Radius" ), aFrame->MessageTextFromValue( radius ) );
    }

    aList.emplace_back( _( "Segment Length" ), aFrame->MessageTextFromValue( GetLength() ) );

    // Display full track length (in Pcbnew)
    if( board && GetNetCode() > 0 )
    {
        int    count;
        double trackLen;
        double lenPadToDie;

        std::tie( count, trackLen, lenPadToDie ) = board->GetTrackLength( *this );

        aList.emplace_back( _( "Routed Length" ), aFrame->MessageTextFromValue( trackLen ) );

        if( lenPadToDie != 0 )
        {
            msg = aFrame->MessageTextFromValue( lenPadToDie );
            aList.emplace_back( _( "Pad To Die Length" ), msg );

            msg = aFrame->MessageTextFromValue( trackLen + lenPadToDie );
            aList.emplace_back( _( "Full Length" ), msg );
        }
    }

    wxString source;
    int clearance = GetOwnClearance( GetLayer(), &source );

    aList.emplace_back( wxString::Format( _( "Min Clearance: %s" ),
                                          aFrame->MessageTextFromValue( clearance ) ),
                        wxString::Format( _( "(from %s)" ), source ) );

    MINOPTMAX<int> c = GetWidthConstraint( &source );

    if( c.HasMax() )
    {
        aList.emplace_back( wxString::Format( _( "Width Constraints: min %s, max %s" ),
                                              aFrame->MessageTextFromValue( c.Min() ),
                                              aFrame->MessageTextFromValue( c.Max() ) ),
                            wxString::Format( _( "(from %s)" ), source ) );
    }
    else
    {
        aList.emplace_back( wxString::Format( _( "Width Constraints: min %s" ),
                                              aFrame->MessageTextFromValue( c.Min() ) ),
                            wxString::Format( _( "(from %s)" ), source ) );
    }
}


void PCB_VIA::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString  msg;

    switch( GetViaType() )
    {
    case VIATYPE::MICROVIA:     msg = _( "Micro Via" );        break;
    case VIATYPE::BLIND_BURIED: msg = _( "Blind/Buried Via" ); break;
    case VIATYPE::THROUGH:      msg = _( "Through Via" );      break;
    default:                    msg = _( "Via" );              break;
    }

    aList.emplace_back( _( "Type" ), msg );

    GetMsgPanelInfoBase_Common( aFrame, aList );

    aList.emplace_back( _( "Layer" ), layerMaskDescribe() );
    aList.emplace_back( _( "Diameter" ), aFrame->MessageTextFromValue( m_Width ) );
    aList.emplace_back( _( "Hole" ), aFrame->MessageTextFromValue( GetDrillValue() ) );

    wxString  source;
    int clearance = GetOwnClearance( GetLayer(), &source );

    aList.emplace_back( wxString::Format( _( "Min Clearance: %s" ),
                                          aFrame->MessageTextFromValue( clearance ) ),
                        wxString::Format( _( "(from %s)" ), source ) );

    int minAnnulus = GetMinAnnulus( GetLayer(), &source );

    aList.emplace_back( wxString::Format( _( "Min Annular Width: %s" ),
                                          aFrame->MessageTextFromValue( minAnnulus ) ),
                        wxString::Format( _( "(from %s)" ), source ) );
}


void PCB_TRACK::GetMsgPanelInfoBase_Common( EDA_DRAW_FRAME* aFrame,
                                            std::vector<MSG_PANEL_ITEM>& aList ) const
{
    aList.emplace_back( _( "Net" ), UnescapeString( GetNetname() ) );

    aList.emplace_back( _( "Resolved Netclass" ),
                        UnescapeString( GetEffectiveNetClass()->GetName() ) );

#if 0   // Enable for debugging
    if( GetBoard() )
        aList.emplace_back( _( "NetCode" ), wxString::Format( wxT( "%d" ), GetNetCode() ) );

    aList.emplace_back( wxT( "Flags" ), wxString::Format( wxT( "0x%08X" ), m_flags ) );

    aList.emplace_back( wxT( "Start pos" ), wxString::Format( wxT( "%d %d" ),
                                                              m_Start.x,
                                                              m_Start.y ) );
    aList.emplace_back( wxT( "End pos" ), wxString::Format( wxT( "%d %d" ),
                                                            m_End.x,
                                                            m_End.y ) );
#endif

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );
}


wxString PCB_VIA::layerMaskDescribe() const
{
    const BOARD* board = GetBoard();
    PCB_LAYER_ID top_layer;
    PCB_LAYER_ID bottom_layer;

    LayerPair( &top_layer, &bottom_layer );

    return board->GetLayerName( top_layer ) + wxT( " - " ) + board->GetLayerName( bottom_layer );
}


bool PCB_TRACK::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    return TestSegmentHit( aPosition, m_Start, m_End, aAccuracy + ( m_Width / 2 ) );
}


bool PCB_ARC::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    int max_dist = aAccuracy + ( m_Width / 2 );

    // Short-circuit common cases where the arc is connected to a track or via at an endpoint
    if( EuclideanNorm( GetStart() - aPosition ) <= max_dist ||
            EuclideanNorm( GetEnd() - aPosition ) <= max_dist )
    {
        return true;
    }

    VECTOR2I center = GetPosition();
    VECTOR2I relpos = aPosition - center;
    double dist = EuclideanNorm( relpos );
    double radius = GetRadius();

    if( std::abs( dist - radius ) > max_dist )
        return false;

    EDA_ANGLE arc_angle = GetAngle();
    EDA_ANGLE arc_angle_start = GetArcAngleStart();    // Always 0.0 ... 360 deg
    EDA_ANGLE arc_hittest( relpos );

    // Calculate relative angle between the starting point of the arc, and the test point
    arc_hittest -= arc_angle_start;

    // Normalise arc_hittest between 0 ... 360 deg
    arc_hittest.Normalize();

    if( arc_angle < ANGLE_0 )
        return arc_hittest >= ANGLE_360 + arc_angle;

    return arc_hittest <= arc_angle;
}


bool PCB_VIA::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    int max_dist = aAccuracy + ( m_Width / 2 );

    // rel_pos is aPosition relative to m_Start (or the center of the via)
    VECTOR2I rel_pos = aPosition - m_Start;
    double dist = (double) rel_pos.x * rel_pos.x + (double) rel_pos.y * rel_pos.y;
    return  dist <= (double) max_dist * max_dist;
}


bool PCB_TRACK::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( GetStart() ) && arect.Contains( GetEnd() );
    else
        return arect.Intersects( GetStart(), GetEnd() );
}


bool PCB_ARC::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Inflate( aAccuracy );

    BOX2I box( GetStart() );
    box.Merge( GetMid() );
    box.Merge( GetEnd() );

    box.Inflate( GetWidth() / 2 );

    if( aContained )
        return arect.Contains( box );
    else
        return arect.Intersects( box );
}


bool PCB_VIA::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Inflate( aAccuracy );

    BOX2I box( GetStart() );
    box.Inflate( GetWidth() / 2 );

    if( aContained )
        return arect.Contains( box );
    else
        return arect.IntersectsCircle( GetStart(), GetWidth() / 2 );
}


wxString PCB_TRACK::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( Type() == PCB_ARC_T ? _("Track (arc) %s on %s, length %s" )
                                                 : _("Track %s on %s, length %s" ),
                             GetNetnameMsg(),
                             GetLayerName(),
                             aUnitsProvider->MessageTextFromValue( GetLength() ) );
}


BITMAPS PCB_TRACK::GetMenuImage() const
{
    return BITMAPS::add_tracks;
}

void PCB_TRACK::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_TRACE_T );

    std::swap( *((PCB_TRACK*) this), *((PCB_TRACK*) aImage) );
}

void PCB_ARC::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_ARC_T );

    std::swap( *this, *static_cast<PCB_ARC*>( aImage ) );
}

void PCB_VIA::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_VIA_T );

    std::swap( *((PCB_VIA*) this), *((PCB_VIA*) aImage) );
}


VECTOR2I PCB_ARC::GetPosition() const
{
    VECTOR2I center = CalcArcCenter( m_Start, m_Mid, m_End );
    return center;
}


double PCB_ARC::GetRadius() const
{
    auto center = CalcArcCenter( m_Start, m_Mid , m_End );
    return GetLineLength( center, m_Start );
}


EDA_ANGLE PCB_ARC::GetAngle() const
{
    VECTOR2I  center = GetPosition();
    EDA_ANGLE angle1 = EDA_ANGLE( m_Mid - center ) - EDA_ANGLE( m_Start - center );
    EDA_ANGLE angle2 = EDA_ANGLE( m_End - center ) - EDA_ANGLE( m_Mid - center );

    return angle1.Normalize180() + angle2.Normalize180();
}


EDA_ANGLE PCB_ARC::GetArcAngleStart() const
{
    EDA_ANGLE angleStart( m_Start - GetPosition() );
    return angleStart.Normalize();
}


EDA_ANGLE PCB_ARC::GetArcAngleEnd() const
{
    EDA_ANGLE angleEnd( m_End - GetPosition() );
    return angleEnd.Normalize();
}


bool PCB_TRACK::cmp_tracks::operator() ( const PCB_TRACK* a, const PCB_TRACK* b ) const
{
    if( a->GetNetCode() != b->GetNetCode() )
        return a->GetNetCode() < b->GetNetCode();

    if( a->GetLayer() != b->GetLayer() )
        return a->GetLayer() < b->GetLayer();

    if( a->Type() != b->Type() )
        return a->Type() < b->Type();

    if( a->m_Uuid != b->m_Uuid )
        return a->m_Uuid < b->m_Uuid;

    return a < b;
}


std::shared_ptr<SHAPE> PCB_TRACK::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    return std::make_shared<SHAPE_SEGMENT>( m_Start, m_End, m_Width );
}


std::shared_ptr<SHAPE> PCB_VIA::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    if( aFlash == FLASHING::ALWAYS_FLASHED
            || ( aFlash == FLASHING::DEFAULT && FlashLayer( aLayer ) ) )
    {
        return std::make_shared<SHAPE_CIRCLE>( m_Start, m_Width / 2 );
    }
    else
    {
        return std::make_shared<SHAPE_CIRCLE>( m_Start, GetDrillValue() / 2 );
    }
}


std::shared_ptr<SHAPE> PCB_ARC::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    return std::make_shared<SHAPE_ARC>( GetStart(), GetMid(), GetEnd(), GetWidth() );
}


void PCB_TRACK::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                         int aClearance, int aError, ERROR_LOC aErrorLoc,
                                         bool ignoreLineWidth ) const
{
    wxASSERT_MSG( !ignoreLineWidth, wxT( "IgnoreLineWidth has no meaning for tracks." ) );


    switch( Type() )
    {
    case PCB_VIA_T:
    {
        int radius = ( m_Width / 2 ) + aClearance;
        TransformCircleToPolygon( aBuffer, m_Start, radius, aError, aErrorLoc );
        break;
    }

    case PCB_ARC_T:
    {
        const PCB_ARC* arc = static_cast<const PCB_ARC*>( this );
        int            width = m_Width + ( 2 * aClearance );

        TransformArcToPolygon( aBuffer, arc->GetStart(), arc->GetMid(), arc->GetEnd(), width,
                               aError, aErrorLoc );
        break;
    }

    default:
    {
        int width = m_Width + ( 2 * aClearance );

        TransformOvalToPolygon( aBuffer, m_Start, m_End, width, aError, aErrorLoc );
        break;
    }
    }
}


static struct TRACK_VIA_DESC
{
    TRACK_VIA_DESC()
    {
        ENUM_MAP<VIATYPE>::Instance()
            .Undefined( VIATYPE::NOT_DEFINED )
            .Map( VIATYPE::THROUGH,      _HKI( "Through" ) )
            .Map( VIATYPE::BLIND_BURIED, _HKI( "Blind/buried" ) )
            .Map( VIATYPE::MICROVIA,     _HKI( "Micro" ) );

        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( LSEQ seq = LSET::AllLayersMask().Seq(); seq; ++seq )
                layerEnum.Map( *seq, LSET::Name( *seq ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();

        // Track
        REGISTER_TYPE( PCB_TRACK );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TRACK ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );

        propMgr.AddProperty( new PROPERTY<PCB_TRACK, int>( _HKI( "Width" ),
            &PCB_TRACK::SetWidth, &PCB_TRACK::GetWidth, PROPERTY_DISPLAY::PT_SIZE ) );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Position X" ),
            new PROPERTY<PCB_TRACK, int, BOARD_ITEM>( _HKI( "Start X" ),
            &PCB_TRACK::SetX, &PCB_TRACK::GetX, PROPERTY_DISPLAY::PT_COORD,
            ORIGIN_TRANSFORMS::ABS_X_COORD) );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Position Y" ),
            new PROPERTY<PCB_TRACK, int, BOARD_ITEM>( _HKI( "Start Y" ),
            &PCB_TRACK::SetY, &PCB_TRACK::GetY, PROPERTY_DISPLAY::PT_COORD,
            ORIGIN_TRANSFORMS::ABS_Y_COORD ) );
        propMgr.AddProperty( new PROPERTY<PCB_TRACK, int>( _HKI( "End X" ),
            &PCB_TRACK::SetEndX, &PCB_TRACK::GetEndX, PROPERTY_DISPLAY::PT_COORD,
            ORIGIN_TRANSFORMS::ABS_X_COORD) );
        propMgr.AddProperty( new PROPERTY<PCB_TRACK, int>( _HKI( "End Y" ),
            &PCB_TRACK::SetEndY, &PCB_TRACK::GetEndY, PROPERTY_DISPLAY::PT_COORD,
            ORIGIN_TRANSFORMS::ABS_Y_COORD) );

        // Arc
        REGISTER_TYPE( PCB_ARC );
        propMgr.InheritsAfter( TYPE_HASH( PCB_ARC ), TYPE_HASH( PCB_TRACK ) );

        // Via
        REGISTER_TYPE( PCB_VIA );
        propMgr.InheritsAfter( TYPE_HASH( PCB_VIA ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );

        // TODO test drill, use getdrillvalue?
        const wxString groupVia = _HKI( "Via Properties" );

        propMgr.Mask( TYPE_HASH( PCB_VIA ), TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Layer" ) );

        propMgr.ReplaceProperty( TYPE_HASH( PCB_TRACK ), _HKI( "Width" ),
            new PROPERTY<PCB_VIA, int, PCB_TRACK>( _HKI( "Diameter" ),
            &PCB_VIA::SetWidth, &PCB_VIA::GetWidth, PROPERTY_DISPLAY::PT_SIZE ) );
        propMgr.AddProperty( new PROPERTY<PCB_VIA, int>( _HKI( "Hole" ),
            &PCB_VIA::SetDrill, &PCB_VIA::GetDrillValue, PROPERTY_DISPLAY::PT_SIZE ), groupVia );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ),
            new PROPERTY_ENUM<PCB_VIA, PCB_LAYER_ID, BOARD_ITEM>( _HKI( "Layer Top" ),
            &PCB_VIA::SetLayer, &PCB_VIA::GetLayer ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, PCB_LAYER_ID>( _HKI( "Layer Bottom" ),
            &PCB_VIA::SetBottomLayer, &PCB_VIA::BottomLayer ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, VIATYPE>( _HKI( "Via Type" ),
            &PCB_VIA::SetViaType, &PCB_VIA::GetViaType ), groupVia );
    }
} _TRACK_VIA_DESC;

ENUM_TO_WXANY( VIATYPE );
