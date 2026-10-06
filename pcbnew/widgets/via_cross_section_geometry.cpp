/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <widgets/via_cross_section_geometry.h>

#include <algorithm>
#include <board_stackup_manager/board_stackup.h>
#include <via_protection_rules.h>


// The drawing is laid out on a fixed width grid whose height follows the layer count
static constexpr double OUTER_TOP = 20.0;
static constexpr double LAYER_PITCH = 10.0;

static constexpr double COPPER_THICKNESS = 8.0;
static constexpr double MASK_THICKNESS = 8.0;
static constexpr double COVER_THICKNESS = 6.0;
static constexpr double INNER_THICKNESS = 4.0;

static constexpr double GRID_W = VIA_CROSS_SECTION_GEOMETRY::GRID_W;
static constexpr double BOARD_LEFT = 10.0;
static constexpr double BOARD_RIGHT = GRID_W - 10.0;
static constexpr double HOLE_LEFT = GRID_W / 2 - 17.0;
static constexpr double HOLE_RIGHT = GRID_W / 2 + 17.0;
static constexpr double BARREL_LEFT = HOLE_LEFT - 5.0;
static constexpr double BARREL_RIGHT = HOLE_RIGHT + 5.0;

static constexpr double MAX_HALF_WIDTH = 60.0;
static constexpr double MASK_CLEARANCE = 12.0;
static constexpr double COVER_OVERHANG = 4.0;
static constexpr double RING_GAP = 10.0;
static constexpr double PLUG_DEPTH = 34.0;
static constexpr double PLUG_ROUND = 12.0;


using STATE = VIA_CROSS_SECTION_GEOMETRY::STATE;
using PIECE = VIA_CROSS_SECTION_GEOMETRY::PIECE;
using BODY = VIA_CROSS_SECTION_GEOMETRY::BODY;
using MATERIAL = VIA_CROSS_SECTION_GEOMETRY::MATERIAL;
using CLIP = VIA_CROSS_SECTION_GEOMETRY::CLIP;


static int innerLayersOf( const STATE& aState )
{
    if( aState.m_LayerDepths.size() >= 2 )
        return (int) aState.m_LayerDepths.size() - 2;

    return std::max( 0, aState.m_InnerLayers );
}


static double coreHeightFor( int aInnerLayers )
{
    return std::max( 76.0, LAYER_PITCH * ( aInnerLayers + 1 ) );
}


static PIECE rect( double aLeft, double aTop, double aRight, double aBottom, bool aMixed = false )
{
    PIECE shape;
    shape.m_Rect = true;
    shape.m_Points = { { aLeft, aTop }, { aRight, aBottom } };
    shape.m_Mixed = aMixed;
    return shape;
}


static PIECE polygon( std::vector<VIA_CROSS_SECTION_GEOMETRY::POINT> aPoints, bool aMixed = false )
{
    PIECE shape;
    shape.m_Rect = false;
    shape.m_Points = std::move( aPoints );
    shape.m_Mixed = aMixed;
    return shape;
}


double VIA_CROSS_SECTION_GEOMETRY::GridHeightFor( int aInnerLayers )
{
    return coreHeightFor( aInnerLayers ) + 2 * ( OUTER_TOP + COPPER_THICKNESS );
}


int VIA_CROSS_SECTION_GEOMETRY::SetLayersFromStackup( STATE& aState, const BOARD_STACKUP& aStackup,
                                                      int aCopperLayerCount )
{
    std::vector<int> centres;
    std::vector<int> thicknesses;
    int              total = 0;

    for( const BOARD_STACKUP_ITEM* item : aStackup.GetList() )
    {
        if( !item->IsEnabled() )
            continue;

        if( item->GetType() == BS_ITEM_TYPE_COPPER )
        {
            centres.push_back( total + item->GetThickness() / 2 );
            thicknesses.push_back( item->GetThickness() );
            total += item->GetThickness();
        }
        else if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        {
            for( int i = 0; i < item->GetSublayersCount(); i++ )
                total += item->GetThickness( i );
        }
    }

    aState.m_LayerDepths.clear();
    aState.m_LayerThicknesses.clear();

    if( total > 0 && (int) centres.size() == aCopperLayerCount )
    {
        for( size_t i = 0; i < centres.size(); i++ )
        {
            aState.m_LayerDepths.push_back( (double) centres[i] / total );
            aState.m_LayerThicknesses.push_back( (double) thicknesses[i] / total );
        }
    }

    return total;
}


VIA_CROSS_SECTION_GEOMETRY::VIA_CROSS_SECTION_GEOMETRY( const STATE& aState ) :
        m_innerLayers( innerLayersOf( aState ) ),
        m_gridHeight( GridHeightFor( m_innerLayers ) )
{
    build( aState );
}


double VIA_CROSS_SECTION_GEOMETRY::LayerCentre( int aOrdinal ) const
{
    return m_layerCentres[std::clamp( aOrdinal, 0, m_innerLayers + 1 )];
}


const BODY& VIA_CROSS_SECTION_GEOMETRY::Body( MATERIAL aMaterial ) const
{
    return m_bodies[static_cast<size_t>( aMaterial )];
}


void VIA_CROSS_SECTION_GEOMETRY::build( const STATE& aState )
{
    const std::vector<double>& depths = aState.m_LayerDepths;
    const std::vector<double>& thicknesses = aState.m_LayerThicknesses;
    const bool                 realDepths = depths.size() >= 2;
    const bool                 realThickness = realDepths && thicknesses.size() == depths.size();
    const int                  innerLayers = m_innerLayers;
    const int                  lastOrdinal = innerLayers + 1;
    const double               boardHeight = coreHeightFor( innerLayers ) + 2 * COPPER_THICKNESS;

    auto shown = []( const std::optional<bool>& aFlag )
    {
        return !aFlag.has_value() || *aFlag;
    };
    auto mixed = []( const std::optional<bool>& aFlag )
    {
        return !aFlag.has_value();
    };

    // Copper is drawn at its stackup thickness when known, with a floor so it stays visible
    // under the outlines at dialog size
    auto copperThickness = [&]( int aOrdinal, double aFallback )
    {
        if( !realThickness )
            return aFallback;

        return std::max( std::min( 4.0, aFallback ), boardHeight * thicknesses[aOrdinal] );
    };

    const double outerTop = OUTER_TOP;
    const double outerBottom = OUTER_TOP + boardHeight;
    const double coreTop = outerTop + copperThickness( 0, COPPER_THICKNESS );
    const double coreBottom = outerBottom - copperThickness( lastOrdinal, COPPER_THICKNESS );
    const double maskTop = outerTop - MASK_THICKNESS;
    const double maskBottom = outerBottom + MASK_THICKNESS;
    const double coverTop = maskTop - COVER_THICKNESS;
    const double coverBottom = maskBottom + COVER_THICKNESS;

    // Widths are multiples of the hole diameter, clamped so the picture always fits. The mask is
    // applied after backdrilling, so on a backdrilled side the opening surrounds the bore.
    const double centre = ( HOLE_LEFT + HOLE_RIGHT ) / 2;
    const double holeHalf = ( HOLE_RIGHT - HOLE_LEFT ) / 2;
    const double padHalf = std::clamp( holeHalf * aState.m_PadRatio, BARREL_RIGHT - centre + 4, MAX_HALF_WIDTH );

    auto boreHalf = [&]( double aRatio )
    {
        return std::clamp( holeHalf * aRatio, holeHalf + 2, MAX_HALF_WIDTH );
    };

    // Inner layers sit at their real depth when the stackup is known, evenly spaced otherwise
    m_layerCentres.clear();

    for( int ordinal = 0; ordinal <= lastOrdinal; ordinal++ )
    {
        if( ordinal == 0 )
        {
            m_layerCentres.push_back( ( outerTop + coreTop ) / 2 );
        }
        else if( ordinal == lastOrdinal )
        {
            m_layerCentres.push_back( ( coreBottom + outerBottom ) / 2 );
        }
        else if( realDepths )
        {
            double y = outerTop + boardHeight * depths[ordinal];
            m_layerCentres.push_back( std::clamp( y, coreTop + INNER_THICKNESS, coreBottom - INNER_THICKNESS ) );
        }
        else
        {
            m_layerCentres.push_back( coreTop + ( coreBottom - coreTop ) * ordinal / ( innerLayers + 1 ) );
        }
    }

    auto layerCentre = [&]( int aOrdinal )
    {
        return LayerCentre( aOrdinal );
    };

    double spacing = coreBottom - coreTop;

    for( int i = 0; i <= innerLayers; i++ )
        spacing = std::min( spacing, layerCentre( i + 1 ) - layerCentre( i ) );

    auto innerThickness = [&]( int aOrdinal )
    {
        return copperThickness( aOrdinal, std::clamp( spacing / 2, 1.0, INNER_THICKNESS ) );
    };

    // A surface the via does not reach has no pad and nothing done to the hole there
    const int viaTopOrdinal = std::clamp( aState.m_TopLayer, 0, lastOrdinal );
    const int viaBottomOrdinal = std::clamp( aState.m_BottomLayer.value_or( lastOrdinal ), viaTopOrdinal, lastOrdinal );
    const bool   reachesFront = viaTopOrdinal == 0;
    const bool   reachesBack = viaBottomOrdinal == lastOrdinal;
    const double viaTop = reachesFront ? outerTop : layerCentre( viaTopOrdinal ) - innerThickness( viaTopOrdinal ) / 2;
    const double viaBottom =
            reachesBack ? outerBottom : layerCentre( viaBottomOrdinal ) + innerThickness( viaBottomOrdinal ) / 2;

    auto hasRing = [&]( int aOrdinal )
    {
        const std::vector<bool>& rings = aState.m_RingLayers;
        return rings.size() != (size_t) lastOrdinal + 1 || rings[aOrdinal];
    };

    const bool throughVia = reachesFront && reachesBack;
    const int frontRemoved = throughVia ? std::clamp( aState.m_BackdrillFrontLayers.value_or( 0 ), 0, innerLayers ) : 0;
    const int backRemoved = throughVia ? std::clamp( aState.m_BackdrillBackLayers.value_or( 0 ), 0, innerLayers ) : 0;

    const bool   frontBored = frontRemoved > 0;
    const bool   backBored = backRemoved > 0;
    const double frontBoreHalf = boreHalf( aState.m_BackdrillFrontWidthRatio );
    const double backBoreHalf = boreHalf( aState.m_BackdrillBackWidthRatio );
    const double frontOpenHalf = std::max( padHalf, frontBored ? frontBoreHalf : 0.0 ) + MASK_CLEARANCE;
    const double backOpenHalf = std::max( padHalf, backBored ? backBoreHalf : 0.0 ) + MASK_CLEARANCE;

    // A backdrill takes the pad and the barrel with it and a fab wants mask clearance around
    // the bore, so nothing is drawn over it
    const bool frontOpen = reachesFront && !frontBored;
    const bool backOpen = reachesBack && !backBored;

    // The features a fab can build together, with a mixed value taken as present
    const bool filled = shown( aState.m_Filled );
    const bool capped = shown( aState.m_Capped ) && VIA_PROTECTION_RULES::CapAllowed( filled );
    const bool plugFront = shown( aState.m_PlugFront ) && VIA_PROTECTION_RULES::PlugAllowed( filled );
    const bool plugBack = shown( aState.m_PlugBack ) && VIA_PROTECTION_RULES::PlugAllowed( filled );
    const bool coverFront = shown( aState.m_CoverFront )
                            && VIA_PROTECTION_RULES::CoverAllowed( filled, shown( aState.m_TentFront ), plugFront );
    const bool coverBack = shown( aState.m_CoverBack )
                           && VIA_PROTECTION_RULES::CoverAllowed( filled, shown( aState.m_TentBack ), plugBack );

    // Backdrills reach from the cover surface to half way between the last removed layer and
    // the first one kept
    m_bores.clear();

    if( frontBored )
    {
        double depth = ( layerCentre( frontRemoved - 1 ) + layerCentre( frontRemoved ) ) / 2;
        m_bores.push_back( rect( centre - frontBoreHalf, coverTop, centre + frontBoreHalf, depth ) );
    }

    if( backBored )
    {
        int    last = innerLayers + 1 - backRemoved;
        double depth = ( layerCentre( last ) + layerCentre( last + 1 ) ) / 2;
        m_bores.push_back( rect( centre - backBoreHalf, depth, centre + backBoreHalf, coverBottom ) );
    }

    // Widths are multiples of the hole diameter, depth is a fraction of the board, drawn against
    // the real layer positions
    m_cuts.clear();

    auto postMachining = [&]( const POST_MACHINING& aCut, bool aFront )
    {
        if( !aCut.m_Mode.has_value() )
            return;

        const double halfWidth = std::clamp( holeHalf * aCut.m_WidthRatio, holeHalf, MAX_HALF_WIDTH );
        const double depth = std::clamp( boardHeight * aCut.m_DepthFraction, 2.0, boardHeight - 2 );
        const double surface = aFront ? outerTop : outerBottom;
        const double edge = aFront ? coverTop - 2 : coverBottom + 2;
        const double floor = aFront ? surface + depth : surface - depth;

        switch( *aCut.m_Mode )
        {
        case PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE:
            m_cuts.push_back(
                    rect( centre - halfWidth, std::min( edge, floor ), centre + halfWidth, std::max( edge, floor ) ) );
            break;

        case PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK:
            m_cuts.push_back( polygon( { { centre - halfWidth, edge },
                                         { centre + halfWidth, edge },
                                         { centre + halfWidth, surface },
                                         { centre + holeHalf, floor },
                                         { centre - holeHalf, floor },
                                         { centre - halfWidth, surface } } ) );
            break;

        default: break;
        }
    };

    if( reachesFront )
        postMachining( aState.m_PostMachiningFront, true );

    if( reachesBack )
        postMachining( aState.m_PostMachiningBack, false );

    m_bodies.clear();

    auto body = [&]( MATERIAL aMaterial, CLIP aClip ) -> std::vector<PIECE>&
    {
        m_bodies.push_back( { aMaterial, aClip, {} } );
        return m_bodies.back().m_Pieces;
    };

    auto tentShape = [&]( bool aFront, bool aMixed )
    {
        if( aFront )
            return rect( centre - frontOpenHalf, maskTop, centre + frontOpenHalf, outerTop, aMixed );
        else
            return rect( centre - backOpenHalf, outerBottom, centre + backOpenHalf, maskBottom, aMixed );
    };

    // Solder mask sits on the outer copper with an opening around the pad. A tent bridges it.
    // The mask is painted first, so the copper's outline lands on the mask and not the reverse.
    {
        std::vector<PIECE>& mask = body( MATERIAL::SOLDER_MASK, CLIP::OVER_MASK );

        if( reachesFront )
        {
            mask.push_back( rect( BOARD_LEFT, maskTop, centre - frontOpenHalf, outerTop ) );
            mask.push_back( rect( centre + frontOpenHalf, maskTop, BOARD_RIGHT, outerTop ) );

            if( frontOpen && shown( aState.m_TentFront ) )
                mask.push_back( tentShape( true, mixed( aState.m_TentFront ) ) );
        }
        else
        {
            mask.push_back( rect( BOARD_LEFT, maskTop, BOARD_RIGHT, outerTop ) );
        }

        if( reachesBack )
        {
            mask.push_back( rect( BOARD_LEFT, outerBottom, centre - backOpenHalf, maskBottom ) );
            mask.push_back( rect( centre + backOpenHalf, outerBottom, BOARD_RIGHT, maskBottom ) );

            if( backOpen && shown( aState.m_TentBack ) )
                mask.push_back( tentShape( false, mixed( aState.m_TentBack ) ) );
        }
        else
        {
            mask.push_back( rect( BOARD_LEFT, outerBottom, BOARD_RIGHT, maskBottom ) );
        }
    }

    {
        std::vector<PIECE>& core = body( MATERIAL::CORE, CLIP::UNDER_MASK );

        core.push_back( rect( BOARD_LEFT, coreTop, BARREL_LEFT, coreBottom ) );
        core.push_back( rect( BARREL_RIGHT, coreTop, BOARD_RIGHT, coreBottom ) );

        if( !reachesFront )
            core.push_back( rect( BARREL_LEFT, coreTop, BARREL_RIGHT, viaTop ) );

        if( !reachesBack )
            core.push_back( rect( BARREL_LEFT, viaBottom, BARREL_RIGHT, coreBottom ) );

        // A buried hole fills with laminate resin at lamination, it is never left empty
        if( !reachesFront && !reachesBack && !filled )
            core.push_back( rect( HOLE_LEFT, viaTop, HOLE_RIGHT, viaBottom ) );
    }

    // Plugs are rounded blobs from each side. A fill supersedes them and stops at a cap. They
    // are painted before the copper so the cap's outline lands on the fill, not the reverse.
    auto plugShape = [&]( bool aFront, bool aMixed )
    {
        const double mouth = aFront ? viaTop : viaBottom;
        const double flat = aFront ? std::min( viaTop + PLUG_DEPTH, viaBottom - PLUG_ROUND - 2 )
                                   : std::max( viaBottom - PLUG_DEPTH, viaTop + PLUG_ROUND + 2 );
        const double tip = aFront ? flat + PLUG_ROUND : flat - PLUG_ROUND;

        std::vector<POINT> pts = { { HOLE_LEFT, mouth }, { HOLE_RIGHT, mouth }, { HOLE_RIGHT, flat } };

        for( int i = 1; i < 8; i++ )
        {
            double t = i / 8.0;
            double x = ( 1 - t ) * ( 1 - t ) * HOLE_RIGHT + 2 * ( 1 - t ) * t * centre + t * t * HOLE_LEFT;
            double y = ( 1 - t ) * ( 1 - t ) * flat + 2 * ( 1 - t ) * t * tip + t * t * flat;
            pts.push_back( { x, y } );
        }

        pts.push_back( { HOLE_LEFT, flat } );
        return polygon( std::move( pts ), aMixed );
    };

    {
        std::vector<PIECE>& plug = body( MATERIAL::PLUG, CLIP::UNDER_MASK );

        if( filled )
        {
            plug.push_back( rect( HOLE_LEFT, capped && frontOpen ? coreTop : viaTop, HOLE_RIGHT,
                                  capped && backOpen ? coreBottom : viaBottom, mixed( aState.m_Filled ) ) );
        }
        else
        {
            if( frontOpen && plugFront )
                plug.push_back( plugShape( true, mixed( aState.m_PlugFront ) ) );

            if( backOpen && plugBack )
                plug.push_back( plugShape( false, mixed( aState.m_PlugBack ) ) );
        }
    }

    // The outer layers, the plated barrel, the inner rings and a cap are one copper body. The
    // outer layers are full copper, with the hole through them only where the via reaches them.
    {
        std::vector<PIECE>& copper = body( MATERIAL::COPPER, CLIP::UNDER_MASK );

        if( reachesFront )
        {
            copper.push_back( rect( BOARD_LEFT, outerTop, BARREL_LEFT, coreTop ) );
            copper.push_back( rect( BARREL_RIGHT, outerTop, BOARD_RIGHT, coreTop ) );
        }
        else
        {
            copper.push_back( rect( BOARD_LEFT, outerTop, BOARD_RIGHT, coreTop ) );
        }

        if( reachesBack )
        {
            copper.push_back( rect( BOARD_LEFT, coreBottom, BARREL_LEFT, outerBottom ) );
            copper.push_back( rect( BARREL_RIGHT, coreBottom, BOARD_RIGHT, outerBottom ) );
        }
        else
        {
            copper.push_back( rect( BOARD_LEFT, coreBottom, BOARD_RIGHT, outerBottom ) );
        }

        copper.push_back( rect( BARREL_LEFT, viaTop, HOLE_LEFT, viaBottom ) );
        copper.push_back( rect( HOLE_RIGHT, viaTop, BARREL_RIGHT, viaBottom ) );

        // An inner layer outside the span runs across, one without a ring keeps clear
        for( int i = 1; i <= innerLayers; i++ )
        {
            double t = innerThickness( i );
            double y = layerCentre( i ) - t / 2;

            if( i < viaTopOrdinal || i > viaBottomOrdinal )
            {
                copper.push_back( rect( BOARD_LEFT, y, BOARD_RIGHT, y + t ) );
            }
            else
            {
                double gap = hasRing( i ) ? 0.0 : RING_GAP;
                copper.push_back( rect( BOARD_LEFT, y, BARREL_LEFT - gap, y + t ) );
                copper.push_back( rect( BARREL_RIGHT + gap, y, BOARD_RIGHT, y + t ) );
            }
        }

        if( capped )
        {
            if( frontOpen )
                copper.push_back( rect( BARREL_LEFT, outerTop, BARREL_RIGHT, coreTop, mixed( aState.m_Capped ) ) );

            if( backOpen )
                copper.push_back(
                        rect( BARREL_LEFT, coreBottom, BARREL_RIGHT, outerBottom, mixed( aState.m_Capped ) ) );
        }
    }

    // A cover is a second coat over the tent. Over a plug or a fill on an untented side it
    // bridges the opening itself.
    auto coverShape = [&]( bool aFront, bool aMixed )
    {
        if( aFront )
        {
            return rect( centre - frontOpenHalf - COVER_OVERHANG, coverTop, centre + frontOpenHalf + COVER_OVERHANG,
                         maskTop, aMixed );
        }
        else
        {
            return rect( centre - backOpenHalf - COVER_OVERHANG, maskBottom, centre + backOpenHalf + COVER_OVERHANG,
                         coverBottom, aMixed );
        }
    };

    {
        std::vector<PIECE>& cover = body( MATERIAL::COVER, CLIP::OVER_MASK );

        if( frontOpen && coverFront )
        {
            cover.push_back( coverShape( true, mixed( aState.m_CoverFront ) ) );

            if( !aState.m_TentFront.value_or( false ) )
                cover.push_back( tentShape( true, mixed( aState.m_CoverFront ) ) );
        }

        if( backOpen && coverBack )
        {
            cover.push_back( coverShape( false, mixed( aState.m_CoverBack ) ) );

            if( !aState.m_TentBack.value_or( false ) )
                cover.push_back( tentShape( false, mixed( aState.m_CoverBack ) ) );
        }
    }
}
