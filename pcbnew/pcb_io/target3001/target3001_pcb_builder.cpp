/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "target3001_pcb_builder.h"

#include <algorithm>
#include <cmath>

#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <convert_basic_shapes_to_polygon.h>
#include <eda_units.h>
#include <footprint.h>
#include <geometry/shape_ellipse.h>
#include <geometry/shape_rect.h>
#include <ki_exception.h>
#include <macros.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <reporter.h>
#include <zone.h>

#include <wx/tokenzr.h>

using TARGET3001::ELEMENT;
using TARGET3001::ELEMENT_BASE;
using TARGET3001::ELEMENT_TYPE;
using TARGET3001::LINE;
using TARGET3001::ARC;
using TARGET3001::SPLINE;
using TARGET3001::TRIANGLE;
using TARGET3001::RECTANGLE;
using TARGET3001::TEXT;
using TARGET3001::FIDUCIAL;
using TARGET3001::TEXT_FUNCTION;
using TARGET3001::LAYER_FUNCTION;
using TARGET3001::PACKAGE;
using TARGET3001::BOARD_LAYER_COUNT;
using TARGET3001::NO_ASSOCIATION;
using TARGET3001::ALL_COPPER_LAYER;
using TARGET3001::ToWx;

namespace
{

// Corner chamfer of an octagonal pad as a fraction of its smaller side
constexpr double OCTAGON_CHAMFER_RATIO = 0.25;

EDA_ANGLE angle( float aDegrees )
{
    return EDA_ANGLE( double( aDegrees ), DEGREES_T );
}


LINE_STYLE lineStyle( uint8_t aDash )
{
    switch( aDash )
    {
    case 1:  return LINE_STYLE::DASH;
    case 2:  return LINE_STYLE::DASHDOT;
    case 3:  return LINE_STYLE::DASHDOTDOT;
    case 4:  return LINE_STYLE::DOT;
    default: return LINE_STYLE::SOLID;
    }
}


/// Vertices of a POLY_PAD value "x,y;x,y;..." in the pad's own frame, Y up
std::vector<VECTOR2I> parsePolyPad( const std::string& aValue )
{
    std::vector<VECTOR2I> pts;
    wxStringTokenizer     tokens( wxString( aValue ), wxS( ";" ), wxTOKEN_STRTOK );

    while( tokens.HasMoreTokens() )
    {
        wxString pair = tokens.GetNextToken();
        long     x = 0;
        long     y = 0;

        if( !pair.BeforeFirst( ',' ).ToLong( &x ) || !pair.AfterFirst( ',' ).ToLong( &y ) )
            return {};

        pts.emplace_back( int( x ), int( y ) );
    }

    return pts;
}


/// The value counted most often, the smallest one on a tie
int mostCommon( const std::map<int, int>& aCounts )
{
    auto it = std::max_element( aCounts.begin(), aCounts.end(),
                                []( const auto& a, const auto& b )
                                {
                                    return a.second < b.second;
                                } );

    return it->first;
}


/// Distance from a pad to the fill around it, 0 when the pad touches it or nothing lies within reach
int padFillGap( const PAD& aPad, PCB_LAYER_ID aLayer, const SHAPE_POLY_SET& aFill, int aReach )
{
    int         gap = 0;
    ::PAD_SHAPE shape = aPad.GetShape( aLayer );

    // The vendor may knock out an octagon around a rectangular pad, so only side midpoints agree; round
    // knockouts are polygons around the pad that touch it only at their tangent points
    if( shape != ::PAD_SHAPE::RECTANGLE && shape != ::PAD_SHAPE::ROUNDRECT && shape != ::PAD_SHAPE::CHAMFERED_RECT )
        return aFill.Collide( aPad.GetEffectiveShape( aLayer ).get(), aReach, &gap ) ? gap : 0;

    VECTOR2I half = aPad.GetSize( aLayer ) / 2;
    int      best = 0;

    for( VECTOR2I side :
         { VECTOR2I( half.x, 0 ), VECTOR2I( -half.x, 0 ), VECTOR2I( 0, half.y ), VECTOR2I( 0, -half.y ) } )
    {
        RotatePoint( side, aPad.GetOrientation() );

        SHAPE_RECT probe( aPad.ShapePos( aLayer ) + side, 1, 1 );

        if( !aFill.Collide( &probe, aReach, &gap ) || gap <= 0 )
            continue;

        best = best ? std::min( best, gap ) : gap;
    }

    return best;
}

} // namespace


TARGET3001_PCB_BUILDER::TARGET3001_PCB_BUILDER( BOARD* aBoard, const TARGET3001::PROJECT& aProject,
                                                REPORTER* aReporter ) :
        m_board( aBoard ),
        m_project( aProject ),
        m_reporter( aReporter )
{
}


void TARGET3001_PCB_BUILDER::Build()
{
    buildLayerMap();
    buildNets();
    buildComponents();
    buildSignalCopper();
    buildFreeItems();
    buildPourAreas();
    buildPolygonPours();
    buildAreaClearanceRules();
    buildAuraRules();
    buildPolygonRules();
    buildThermalGaps();

    // A pour runs up to a contour stroke without clearance; KiCad has one edge clearance per board
    if( m_contourCarvesNothing )
        m_board->GetDesignSettings().m_CopperEdgeClearance = 0;

    // BOARD::SetProject keeps importer settings only when they are flagged as loaded
    m_board->m_LegacyDesignSettingsLoaded = true;

    // TARGET 3001! places the board around its own origin with Y up; move it onto the sheet
    BOX2I bbox = m_board->GetBoardEdgesBoundingBox();

    if( bbox.GetWidth() == 0 || bbox.GetHeight() == 0 )
        bbox = m_board->ComputeBoundingBox( false );

    const int margin = pcbIUScale.mmToIU( 20 );
    m_board->Move( VECTOR2I( margin - bbox.GetX(), margin - bbox.GetY() ) );
}


LAYER_FUNCTION TARGET3001_PCB_BUILDER::LayerFunction( int aLayer ) const
{
    if( aLayer < 0 || aLayer >= BOARD_LAYER_COUNT )
        return LAYER_FUNCTION::OTHER;

    return LAYER_FUNCTION( m_project.layerFunctions[aLayer] );
}


bool TARGET3001_PCB_BUILDER::isCopper( int aLayer ) const
{
    LAYER_FUNCTION f = LayerFunction( aLayer );

    return f == LAYER_FUNCTION::COPPER_BOTTOM || f == LAYER_FUNCTION::COPPER_INSIDE
           || f == LAYER_FUNCTION::COPPER_TOP;
}


static wxString ruleMm( int aValue )
{
    return wxString::FromUTF8( EDA_UNIT_UTILS::FormatInternalUnits( pcbIUScale, aValue ) ) + wxS( "mm" );
}


/// An expression string literal, whose only escape is \', inside a quoted rules file string
static wxString ruleLiteral( const wxString& aValue )
{
    wxString text = aValue;
    text.Replace( wxS( "\\" ), wxS( "\\\\" ) );
    text.Replace( wxS( "'" ), wxS( "\\\\'" ) );
    text.Replace( wxS( "\"" ), wxS( "\\\"" ) );
    return wxS( "'" ) + text + wxS( "'" );
}


void TARGET3001_PCB_BUILDER::buildAreaClearanceRules()
{
    if( m_pourLayers.empty() && !m_hasPolygonPours )
        return;

    std::map<int, int>                   boardCounts;
    std::map<int, std::vector<wxString>> netsByClearance;

    for( const auto& [signal, counts] : m_signalClearances )
    {
        NETINFO_ITEM* item = net( signal );

        if( !item || item->GetNetCode() <= 0 || counts.empty() )
            continue;

        netsByClearance[mostCommon( counts )].push_back( item->GetNetname() );

        for( const auto& [clearance, count] : counts )
            boardCounts[clearance] += count;
    }

    if( boardCounts.empty() )
        return;

    auto addRule = [&]( const wxString& aName, int aClearance, const wxString& aCondition )
    {
        m_designRules += wxString::Format( wxS( "\n(rule \"%s\"\n    (constraint clearance (min %s))\n"
                                                "    (condition \"%s\"))\n" ),
                                           aName, ruleMm( aClearance ), aCondition );
    };

    // An area keeps each element's own clearance, which KiCad tracks and vias cannot hold
    int base = mostCommon( boardCounts );
    m_designRules = wxS( "(version 1)\n" );
    addRule( wxS( "TARGET 3001! copper areas" ), base, wxS( "A.Type == 'Zone'" ) );

    for( const auto& [clearance, nets] : netsByClearance )
    {
        if( clearance == base )
            continue;

        wxString condition;

        for( const wxString& name : nets )
        {
            if( !condition.IsEmpty() )
                condition += wxS( " || " );

            condition += wxS( "B.NetName == " ) + ruleLiteral( name );
        }

        addRule( wxS( "TARGET 3001! copper areas to " ) + ruleMm( clearance ) + wxS( " signals" ), clearance,
                 wxS( "A.Type == 'Zone' && (" ) + condition + wxS( ")" ) );
    }
}


void TARGET3001_PCB_BUILDER::buildAuraRules()
{
    if( m_pourLayers.empty() )
        return;

    if( m_designRules.IsEmpty() )
        m_designRules = wxS( "(version 1)\n" );

    // Later rules win, so the NO_AURA layers come last
    m_designRules += wxS( "\n(rule \"TARGET 3001! areas join own pads and vias\"\n"
                          "    (condition \"A.hasCustomProperty('TARGET3001_AURA') "
                          "&& B.hasCustomProperty('TARGET3001_AREA')\")\n"
                          "    (constraint zone_connection solid))\n" );

    for( int aura : m_auraValues )
    {
        m_designRules += wxString::Format( wxS( "\n(rule \"TARGET 3001! aura %s\"\n"
                                                "    (condition \"A.customProperty('TARGET3001_AURA') == '%d' "
                                                "&& B.hasCustomProperty('TARGET3001_AREA')\")\n"
                                                "    (constraint zone_connection none)\n"
                                                "    (constraint physical_clearance (min %s)))\n" ),
                                           ruleMm( aura ), aura, ruleMm( aura ) );
    }

    for( PCB_LAYER_ID layer : m_noAuraLayers )
    {
        wxString name = LSET::Name( layer );

        m_designRules += wxString::Format( wxS( "\n(rule \"TARGET 3001! no aura on %s\"\n"
                                                "    (layer \"%s\")\n"
                                                "    (condition \"A.customProperty('TARGET3001_NO_AURA') == '*,%s,*' "
                                                "&& B.hasCustomProperty('TARGET3001_AREA')\")\n"
                                                "    (constraint zone_connection solid)\n"
                                                "    (constraint physical_clearance (min 0mm)))\n" ),
                                           name, name, name );
    }
}


void TARGET3001_PCB_BUILDER::tagAura( BOARD_CONNECTED_ITEM& aItem, const TARGET3001::ELEMENT_BASE& aBase, int aAura )
{
    aItem.SetCustomProperty( wxS( "TARGET3001_AURA" ), wxString::Format( wxS( "%d" ), aAura ) );

    if( aAura <= 0 )
        return;

    m_auraValues.insert( aAura );

    // Names are comma delimited both sides so a rule can match one layer with a wildcard
    wxString layers;

    if( const std::string* list = aBase.FindProp( "NO_AURA" ) )
    {
        wxStringTokenizer tokens( wxString::FromUTF8( *list ), wxS( "," ) );

        while( tokens.HasMoreTokens() )
        {
            long layer = 0;

            if( !tokens.GetNextToken().Trim().Trim( false ).ToLong( &layer ) || !isCopper( int( layer ) ) )
                continue;

            if( std::optional<PCB_LAYER_ID> cu = mapLayer( int( layer ) ) )
            {
                layers += ( layers.IsEmpty() ? wxS( "," ) : wxS( "" ) ) + LSET::Name( *cu ) + wxS( "," );
                m_noAuraLayers.insert( *cu );
            }
        }
    }

    if( !layers.IsEmpty() )
        aItem.SetCustomProperty( wxS( "TARGET3001_NO_AURA" ), layers );
}


void TARGET3001_PCB_BUILDER::buildLayerMap()
{
    std::vector<int> top;
    std::vector<int> inner;
    std::vector<int> bottom;

    for( int layer = 0; layer < BOARD_LAYER_COUNT; layer++ )
    {
        switch( LayerFunction( layer ) )
        {
        case LAYER_FUNCTION::COPPER_TOP:    top.push_back( layer );    break;
        case LAYER_FUNCTION::COPPER_INSIDE: inner.push_back( layer );  break;
        case LAYER_FUNCTION::COPPER_BOTTOM: bottom.push_back( layer ); break;
        default:                                                       break;
        }
    }

    m_copperStack.clear();
    m_copperStack.insert( m_copperStack.end(), top.begin(), top.end() );
    // Inner copper layer numbers increase towards the top side
    m_copperStack.insert( m_copperStack.end(), inner.rbegin(), inner.rend() );
    m_copperStack.insert( m_copperStack.end(), bottom.begin(), bottom.end() );

    if( m_copperStack.size() > MAX_CU_LAYERS )
    {
        THROW_IO_ERROR( wxString::Format( _( "TARGET 3001! board has %zu copper layers; at most %d are supported." ),
                                          m_copperStack.size(), MAX_CU_LAYERS ) );
    }

    int copperCount = std::max<int>( 2, m_copperStack.size() );

    if( copperCount % 2 )
        copperCount++;

    m_board->SetCopperLayerCount( copperCount );

    for( size_t i = 0; i < m_copperStack.size(); i++ )
    {
        PCB_LAYER_ID id;

        // A board with only bottom copper keeps it on the bottom
        if( i + 1 == m_copperStack.size() && !bottom.empty() )
            id = B_Cu;
        else if( i == 0 )
            id = F_Cu;
        else
            id = BoardLayerFromLegacyId( int( i ) );

        m_layerMap[m_copperStack[i]] = id;
    }

    for( int layer = 0; layer < BOARD_LAYER_COUNT && !m_contourLayer; layer++ )
    {
        if( LayerFunction( layer ) == LAYER_FUNCTION::OUTLINE )
            m_contourLayer = layer;
    }

    int nextUser = 0;

    for( int layer = 0; layer < BOARD_LAYER_COUNT; layer++ )
    {
        if( m_layerMap.count( layer ) )
            continue;

        std::optional<PCB_LAYER_ID> id;

        switch( LayerFunction( layer ) )
        {
        case LAYER_FUNCTION::POSITION_BOTTOM: id = B_SilkS;   break;
        case LAYER_FUNCTION::POSITION_TOP:    id = F_SilkS;   break;
        case LAYER_FUNCTION::NAMES_BOTTOM:    id = B_SilkS;   break;
        case LAYER_FUNCTION::NAMES_TOP:       id = F_SilkS;   break;
        case LAYER_FUNCTION::VALUES_BOTTOM:   id = B_Fab;     break;
        case LAYER_FUNCTION::VALUES_TOP:      id = F_Fab;     break;
        case LAYER_FUNCTION::MASK_BOTTOM:     id = B_Mask;    break;
        case LAYER_FUNCTION::MASK_TOP:        id = F_Mask;    break;
        case LAYER_FUNCTION::PASTE_BOTTOM:    id = B_Paste;   break;
        case LAYER_FUNCTION::PASTE_TOP:       id = F_Paste;   break;
        case LAYER_FUNCTION::GLUE_BOTTOM:     id = B_Adhes;   break;
        case LAYER_FUNCTION::GLUE_TOP:        id = F_Adhes;   break;
        // Milling is a tool path around the outline, so it stays on its own user layer
        case LAYER_FUNCTION::OUTLINE:         id = Edge_Cuts; break;
        case LAYER_FUNCTION::DIMENSIONING:    id = Dwgs_User; break;
        case LAYER_FUNCTION::DRILL_PLAN:      id = Cmts_User; break;

        // Pour areas and keepouts become zones; air wires are derived data
        case LAYER_FUNCTION::AREA_BOTTOM:
        case LAYER_FUNCTION::AREA_INSIDE:
        case LAYER_FUNCTION::AREA_TOP:
        case LAYER_FUNCTION::DELETION_BOTTOM:
        case LAYER_FUNCTION::DELETION_INSIDE:
        case LAYER_FUNCTION::DELETION_TOP:
        case LAYER_FUNCTION::AIR_WIRES:
            break;

        default:
            if( nextUser < 9 )
                id = static_cast<PCB_LAYER_ID>( User_1 + 2 * nextUser++ );
            else
                id = Cmts_User;

            break;
        }

        if( id )
            m_layerMap[layer] = *id;
    }

    LSET enabled = m_board->GetEnabledLayers();

    for( const auto& [target, id] : m_layerMap )
        enabled.set( id );

    m_board->SetEnabledLayers( enabled );
    m_board->SetVisibleLayers( enabled );

    for( const auto& [target, id] : m_layerMap )
    {
        if( id >= User_1 && id <= User_9 )
            m_board->SetLayerName( id, wxString::Format( wxS( "Target layer %d" ), target ) );
    }
}


std::optional<PCB_LAYER_ID> TARGET3001_PCB_BUILDER::mapLayer( int aLayer ) const
{
    auto it = m_layerMap.find( aLayer );

    if( it == m_layerMap.end() )
        return std::nullopt;

    return it->second;
}


std::optional<PCB_LAYER_ID> TARGET3001_PCB_BUILDER::pourCopperLayer( int aLayer ) const
{
    int defaultCopper;

    switch( LayerFunction( aLayer ) )
    {
    case LAYER_FUNCTION::AREA_BOTTOM:
    case LAYER_FUNCTION::AREA_INSIDE:
    case LAYER_FUNCTION::AREA_TOP:
        defaultCopper = aLayer + 2;
        break;

    case LAYER_FUNCTION::DELETION_BOTTOM:
    case LAYER_FUNCTION::DELETION_INSIDE:
    case LAYER_FUNCTION::DELETION_TOP:
        defaultCopper = aLayer + 1;
        break;

    default:
        return std::nullopt;
    }

    int copper = m_project.layerAssociations[aLayer];

    // An association that names no copper layer is unset and the default pairing applies
    if( copper == NO_ASSOCIATION || !isCopper( copper ) )
        copper = defaultCopper;

    if( copper >= BOARD_LAYER_COUNT || !isCopper( copper ) )
        return std::nullopt;

    return mapLayer( copper );
}


std::optional<PCB_LAYER_ID> TARGET3001_PCB_BUILDER::polygonLayer( const TARGET3001::POLYGON_POUR& aPoly ) const
{
    std::optional<PCB_LAYER_ID> layer = mapLayer( aPoly.layer );

    if( !layer || !IsCopperLayer( *layer ) || aPoly.outline.size() < 3 )
        return std::nullopt;

    return layer;
}


void TARGET3001_PCB_BUILDER::buildNets()
{
    for( size_t i = 0; i < m_project.signals.size(); i++ )
    {
        const ELEMENT& e = m_project.signals[i];

        if( e.type != ELEMENT_TYPE::SIGNAL || !e.signal )
            continue;

        wxString name = ToWx( e.signal->name );

        if( name.IsEmpty() )
            name = wxString::Format( wxS( "Net-%zu" ), i + 1 );

        // Net names must be unique in KiCad
        if( m_board->FindNet( name ) )
            name = wxString::Format( wxS( "%s_%zu" ), name, i + 1 );

        NETINFO_ITEM* net = new NETINFO_ITEM( m_board, name, -1 );
        m_board->Add( net, ADD_MODE::APPEND );
        m_nets[e.base.signal] = net;
    }
}


NETINFO_ITEM* TARGET3001_PCB_BUILDER::net( uint16_t aSignal ) const
{
    auto it = m_nets.find( aSignal );
    return it == m_nets.end() ? nullptr : it->second;
}


void TARGET3001_PCB_BUILDER::buildComponents()
{
    for( const ELEMENT& e : m_project.components )
    {
        if( e.type == ELEMENT_TYPE::COMPONENT && e.component )
            addFootprint( e );
    }
}


void TARGET3001_PCB_BUILDER::addFootprint( const ELEMENT& aComponent )
{
    const TARGET3001::COMPONENT& comp = *aComponent.component;

    // A component is on the board only when its package heads the list and sits on a position layer;
    // anything else exists only in the schematic
    if( comp.packageItems.empty() )
        return;

    const ELEMENT* placement = &comp.packageItems.front();

    if( placement->type != ELEMENT_TYPE::PACKAGE || !placement->package )
        return;

    LAYER_FUNCTION placementLayer = LayerFunction( placement->base.layer );

    if( placementLayer != LAYER_FUNCTION::POSITION_TOP && placementLayer != LAYER_FUNCTION::POSITION_BOTTOM )
        return;

    const PACKAGE& pkg = *placement->package;

    FOOTPRINT* fp = new FOOTPRINT( m_board );
    m_board->Add( fp, ADD_MODE::APPEND );

    fp->SetFPIDAsString( wxS( "Target3001:" ) + ToWx( pkg.package ) );
    fp->SetPosition( toKi( placement->base.x, placement->base.y ) );
    fp->SetOrientation( angle( pkg.direction ) );

    // Items are stored already flipped, so only the side changes here
    if( pkg.bottomSide )
        fp->SetLayer( B_Cu );

    wxString reference = ToWx( comp.prefix ).Trim().Trim( false );
    uint16_t number = comp.referenceExtension.value_or( comp.referenceNumber );

    if( number )
        reference << number;

    fp->SetReference( reference );
    fp->SetValue( ToWx( comp.value ) );
    fp->Reference().SetVisible( false );
    fp->Value().SetVisible( false );

    for( const ELEMENT& item : comp.packageItems )
    {
        if( &item != placement )
            addItem( item, fp, fp, false );
    }
}


void TARGET3001_PCB_BUILDER::buildSignalCopper()
{
    for( const ELEMENT& e : m_project.signals )
    {
        if( e.type != ELEMENT_TYPE::SIGNAL || !e.signal )
            continue;

        std::set<int64_t> polygons;

        for( const TARGET3001::POLYGON_POUR& poly : e.signal->polygons )
        {
            if( polygonLayer( poly ) )
                polygons.insert( poly.id );
        }

        for( const ELEMENT& t : e.signal->tracks )
        {
            // Thermal spokes are generated by the vendor area fill; KiCad zones make their own
            if( t.base.FlagProp( "THERMAL_SNIPPET" ) )
                continue;

            // A polygon's stored fill becomes the fill of its zone
            if( std::optional<int64_t> id = t.base.PolygonId(); id && polygons.count( *id ) )
            {
                bool spoke = t.base.FlagProp( "THERMAL_LIGAMENT" );

                if( !( t.base.flags & 1 ) )
                    elementOutline( t, spoke ? m_polygonSpokes[*id] : m_polygonFills[*id] );

                continue;
            }

            addItem( t, m_board, nullptr, true );
        }
    }

    for( auto& [key, polys] : m_copperFills )
    {
        polys.Simplify();

        for( int i = 0; i < polys.OutlineCount(); i++ )
        {
            PCB_SHAPE* shape = new PCB_SHAPE( m_board, SHAPE_T::POLY );
            shape->SetPolyShape( SHAPE_POLY_SET( polys.Polygon( i ) ) );
            shape->SetFilled( true );
            shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
            shape->SetLayer( PCB_LAYER_ID( key.second ) );
            shape->SetNet( net( uint16_t( key.first ) ) );
            m_board->Add( shape, ADD_MODE::APPEND );
        }
    }
}


void TARGET3001_PCB_BUILDER::buildFreeItems()
{
    for( const ELEMENT& e : m_project.boardItems )
        addItem( e, m_board, nullptr, false );
}


void TARGET3001_PCB_BUILDER::buildPourAreas()
{
    std::map<std::pair<uint16_t, PCB_LAYER_ID>, SHAPE_POLY_SET> pours;

    for( POUR_AREA& area : m_pourAreas )
    {
        // TARGET 3001! leaves an area unfilled when its anchor lies outside the contour box
        if( m_contourBox.IsValid() && !m_contourBox.Contains( area.anchor ) )
            continue;

        pours[{ area.signal, area.layer }].Append( area.outline );
    }

    for( auto& [key, outline] : pours )
    {
        outline.Simplify();

        for( int i = 0; i < outline.OutlineCount(); i++ )
        {
            ZONE* zone = new ZONE( m_board );
            zone->SetLayer( key.second );
            zone->SetNet( net( key.first ) );

            // Unconnected islands of an area stay filled
            zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );

            // Each foreign element carves the fill by its own clearance, which only rules can hold; a zero
            // local clearance defers to them
            zone->SetLocalClearance( 0 );
            zone->SetCustomProperty( wxS( "TARGET3001_AREA" ), wxS( "1" ) );
            m_pourLayers.insert( key );

            // Keep holes; a ring of edge strokes must not become a solid area
            *zone->Outline() = SHAPE_POLY_SET( outline.Polygon( i ) );
            m_board->Add( zone, ADD_MODE::APPEND );
        }
    }

    for( auto& [layer, keepout] : m_pourKeepouts )
    {
        keepout.Simplify();

        for( int i = 0; i < keepout.OutlineCount(); i++ )
        {
            ZONE* zone = new ZONE( m_board );
            zone->SetIsRuleArea( true );
            zone->SetDoNotAllowZoneFills( true );
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowPads( false );
            zone->SetDoNotAllowFootprints( false );
            zone->SetLayer( PCB_LAYER_ID( layer ) );
            *zone->Outline() = SHAPE_POLY_SET( keepout.Polygon( i ) );
            m_board->Add( zone, ADD_MODE::APPEND );
        }
    }
}


void TARGET3001_PCB_BUILDER::buildPolygonPours()
{
    for( const ELEMENT& e : m_project.signals )
    {
        if( e.type != ELEMENT_TYPE::SIGNAL || !e.signal )
            continue;

        for( const TARGET3001::POLYGON_POUR& poly : e.signal->polygons )
        {
            std::optional<PCB_LAYER_ID> layer = polygonLayer( poly );

            if( !layer )
                continue;

            ZONE* zone = new ZONE( m_board );
            zone->SetLayer( *layer );
            zone->SetNet( net( e.base.signal ) );
            zone->Outline()->NewOutline();

            // The outline repeats its first vertex at the end; KiCad closes it implicitly
            size_t count = poly.outline.size();

            if( poly.outline.front() == poly.outline.back() )
                count--;

            for( size_t i = 0; i < count; i++ )
                zone->Outline()->Append( toKi( poly.outline[i].first, poly.outline[i].second ) );

            // An unset stroke width falls back to the vendor default
            int width = poly.width > 0 ? poly.width : pcbIUScale.mmToIU( 0.15 );

            zone->SetMinThickness( width );
            zone->SetThermalReliefSpokeWidth( 2 * width );
            zone->SetPadConnection( poly.thermals ? ZONE_CONNECTION::THERMAL : ZONE_CONNECTION::FULL );
            zone->SetIslandRemovalMode( poly.removeIslands ? ISLAND_REMOVAL_MODE::ALWAYS : ISLAND_REMOVAL_MODE::NEVER );
            zone->SetAssignedPriority( unsigned( std::max( 0, poly.rank ) ) );
            zone->SetLocalClearance( 0 );

            if( poly.edgeClearance > 0 )
            {
                zone->SetCustomProperty( wxS( "TARGET3001_EDGE_CLEARANCE" ),
                                         wxString::Format( wxS( "%d" ), poly.edgeClearance ) );
                m_polygonEdgeClearances.insert( poly.edgeClearance );
            }

            if( poly.aura > 0 )
            {
                zone->SetCustomProperty( wxS( "TARGET3001_POLYGON_AURA" ), wxString::Format( wxS( "%d" ), poly.aura ) );
                m_polygonAuras.insert( poly.aura );
            }

            // The stored fill shows the board as the vendor left it until the zones are refilled
            SHAPE_POLY_SET& body = m_polygonFills[poly.id];
            SHAPE_POLY_SET& spokes = m_polygonSpokes[poly.id];

            if( !body.IsEmpty() )
            {
                body.Simplify();
                spokes.Simplify();

                SHAPE_POLY_SET fill = body;
                fill.BooleanAdd( spokes );
                fill.Fracture();
                zone->SetFilledPolysList( *layer, fill );
                zone->SetIsFilled( true );
                zone->SetNeedRefill( false );
            }

            if( poly.thermals )
                m_polygonZones.emplace_back( zone, poly.id );

            m_hasPolygonPours = true;
            m_board->Add( zone, ADD_MODE::APPEND );
        }
    }
}


void TARGET3001_PCB_BUILDER::buildThermalGaps()
{
    // Vendor knockouts stand off a pad by a few um more than its gap, well below any gap a designer enters
    const int step = pcbIUScale.mmToIU( 0.01 );
    const int reach = pcbIUScale.mmToIU( 5 );

    std::map<PAD*, std::map<ZONE*, int>> padGaps;
    std::map<ZONE*, std::map<int, int>>  zoneCounts;
    std::map<int, int>                   boardCounts;

    for( const auto& [zone, id] : m_polygonZones )
    {
        const SHAPE_POLY_SET& body = m_polygonFills[id];
        const SHAPE_POLY_SET& spokes = m_polygonSpokes[id];
        PCB_LAYER_ID          layer = zone->GetLayer();

        for( FOOTPRINT* fp : m_board->Footprints() )
        {
            for( PAD* pad : fp->Pads() )
            {
                if( pad->GetNetCode() != zone->GetNetCode() || !pad->FlashLayer( layer )
                    || !zone->Outline()->Contains( pad->GetPosition() ) )
                {
                    continue;
                }

                // Only a pad the fill reaches through spokes shows its thermal gap
                if( !spokes.Collide( pad->GetEffectiveShape( layer ).get() ) )
                    continue;

                int gap = KiROUND( double( padFillGap( *pad, layer, body, reach ) ) / step ) * step;

                if( gap <= 0 )
                    continue;

                padGaps[pad][zone] = gap;
                zoneCounts[zone][gap]++;
                boardCounts[gap]++;
            }
        }
    }

    if( boardCounts.empty() )
        return;

    // A zone without spoked pads takes the board's gap, so a refill treats its pads alike
    for( const auto& [zone, id] : m_polygonZones )
    {
        auto it = zoneCounts.find( zone );
        zone->SetThermalReliefGap( mostCommon( it != zoneCounts.end() ? it->second : boardCounts ) );
    }

    std::map<wxString, int> references;

    for( FOOTPRINT* fp : m_board->Footprints() )
        references[fp->GetReference()]++;

    std::map<int, std::vector<wxString>> footprintsByGap;

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        std::map<PAD*, int> overrides;
        std::set<int>       gaps;

        for( PAD* pad : fp->Pads() )
        {
            auto it = padGaps.find( pad );

            if( it == padGaps.end() )
                continue;

            std::map<int, int> counts;
            bool               differs = false;

            for( const auto& [zone, gap] : it->second )
            {
                counts[gap]++;
                gaps.insert( gap );
                differs |= gap != zone->GetThermalReliefGap();
            }

            if( differs )
                overrides[pad] = mostCommon( counts );
        }

        if( overrides.empty() )
            continue;

        // A footprint rule needs one gap for all its measured pads and a reference no other footprint shares
        const wxString& ref = fp->GetReference();

        if( gaps.size() == 1 && !ref.IsEmpty() && references[ref] == 1 )
        {
            footprintsByGap[*gaps.begin()].push_back( ref );
            continue;
        }

        for( const auto& [pad, gap] : overrides )
            pad->SetLocalThermalGapOverride( gap );
    }

    if( footprintsByGap.empty() )
        return;

    if( m_designRules.IsEmpty() )
        m_designRules = wxS( "(version 1)\n" );

    // Pads inherit a thermal gap rule that matches their footprint
    for( const auto& [gap, refs] : footprintsByGap )
    {
        wxString condition;

        for( const wxString& ref : refs )
        {
            if( !condition.IsEmpty() )
                condition += wxS( " || " );

            condition += wxS( "A.Reference == " ) + ruleLiteral( ref );
        }

        m_designRules += wxString::Format( wxS( "\n(rule \"TARGET 3001! thermal gap %s\"\n"
                                                "    (condition \"A.Type == 'Footprint' && (%s)\")\n"
                                                "    (constraint thermal_relief_gap (min %s)))\n" ),
                                           ruleMm( gap ), condition, ruleMm( gap ) );
    }
}


void TARGET3001_PCB_BUILDER::buildPolygonRules()
{
    if( m_polygonEdgeClearances.empty() && m_polygonAuras.empty() )
        return;

    if( m_designRules.IsEmpty() )
        m_designRules = wxS( "(version 1)\n" );

    for( int clearance : m_polygonEdgeClearances )
    {
        m_designRules += wxString::Format( wxS( "\n(rule \"TARGET 3001! polygon contour distance %s\"\n"
                                                "    (condition \"A.customProperty('TARGET3001_EDGE_CLEARANCE') "
                                                "== '%d'\")\n"
                                                "    (constraint edge_clearance (min %s)))\n" ),
                                           ruleMm( clearance ), clearance, ruleMm( clearance ) );
    }

    // The larger aura of two polygons wins, so larger values come later
    for( int aura : m_polygonAuras )
    {
        m_designRules += wxString::Format( wxS( "\n(rule \"TARGET 3001! polygon aura %s\"\n"
                                                "    (condition \"A.customProperty('TARGET3001_POLYGON_AURA') "
                                                "== '%d' && B.Type == 'Zone'\")\n"
                                                "    (constraint clearance (min %s)))\n" ),
                                           ruleMm( aura ), aura, ruleMm( aura ) );
    }
}


bool TARGET3001_PCB_BUILDER::elementOutline( const ELEMENT& aElem, SHAPE_POLY_SET& aOut ) const
{
    const ELEMENT_BASE& b = aElem.base;
    VECTOR2I            p0 = toKi( b.x, b.y );

    switch( aElem.type )
    {
    case ELEMENT_TYPE::TRIANGLE:
    {
        const TRIANGLE& t = aElem.triangle;
        aOut.NewOutline();
        aOut.Append( p0 );
        aOut.Append( toKi( t.x2, t.y2 ) );
        aOut.Append( toKi( t.x3, t.y3 ) );
        return true;
    }

    case ELEMENT_TYPE::RECTANGLE:
    {
        const RECTANGLE& r = aElem.rectangle;
        EDA_ANGLE        rot = angle( r.direction );
        VECTOR2D         corners[4] = { { 0.0, 0.0 }, { double( r.width ), 0.0 },
                                        { double( r.width ), double( r.height ) }, { 0.0, double( r.height ) } };

        aOut.NewOutline();

        for( const VECTOR2D& c : corners )
        {
            VECTOR2D rotated = c;
            RotatePoint( rotated, -rot );
            aOut.Append( toKi( b.x + KiROUND( rotated.x ), b.y + KiROUND( rotated.y ) ) );
        }

        return true;
    }

    case ELEMENT_TYPE::DISC:
        TransformCircleToPolygon( aOut, p0, aElem.disc.radius, ARC_HIGH_DEF, ERROR_INSIDE );
        return true;

    case ELEMENT_TYPE::LINE:
        TransformOvalToPolygon( aOut, p0, toKi( aElem.line.x2, aElem.line.y2 ), std::max( 1, aElem.line.width ),
                                ARC_HIGH_DEF, ERROR_INSIDE );
        return true;

    case ELEMENT_TYPE::FIDUCIAL:
        for( const std::unique_ptr<PCB_SHAPE>& shape : fiducialShapes( aElem, m_board ) )
            shape->TransformShapeToPolygon( aOut, F_Cu, 0, ARC_HIGH_DEF, ERROR_INSIDE );

        return true;

    case ELEMENT_TYPE::ARC:
    {
        const ARC& a = aElem.arc;
        VECTOR2I   center = toKi( a.centerX, a.centerY );
        int        width = std::max( 1, a.width );
        double     sweep = a.Sweep();

        if( sweep >= 360.0 - 1e-6 )
        {
            TransformRingToPolygon( aOut, center, a.radius, width, ARC_HIGH_DEF, ERROR_INSIDE );
            return true;
        }

        auto onArc = [&]( double aDeg )
        {
            return center + VECTOR2I( KiROUND( a.radius * cos( DEG2RAD( aDeg ) ) ),
                                      -KiROUND( a.radius * sin( DEG2RAD( aDeg ) ) ) );
        };

        TransformArcToPolygon( aOut, onArc( a.startAngle ), onArc( a.startAngle + sweep / 2.0 ),
                               onArc( a.startAngle + sweep ), width, ARC_HIGH_DEF, ERROR_INSIDE );
        return true;
    }

    default:
        return false;
    }
}


void TARGET3001_PCB_BUILDER::mergeContour( const ELEMENT& aElem )
{
    const ELEMENT_BASE& b = aElem.base;

    m_contourBox.Merge( toKi( b.x, b.y ) );

    if( ( aElem.type == ELEMENT_TYPE::LINE && aElem.line.clearance == 0 )
        || ( aElem.type == ELEMENT_TYPE::ARC && aElem.arc.clearance == 0 )
        || ( aElem.type == ELEMENT_TYPE::SPLINE && aElem.spline.clearance == 0 ) )
    {
        m_contourCarvesNothing = true;
    }

    switch( aElem.type )
    {
    case ELEMENT_TYPE::LINE:
        m_contourBox.Merge( toKi( aElem.line.x2, aElem.line.y2 ) );
        break;

    case ELEMENT_TYPE::ARC:
    {
        const ARC& a = aElem.arc;
        double     start = std::fmod( double( a.startAngle ), 360.0 );
        double     end = start + a.Sweep();

        auto onArc = [&]( double aDeg )
        {
            return VECTOR2I( a.centerX + KiROUND( a.radius * cos( DEG2RAD( aDeg ) ) ),
                             a.centerY + KiROUND( a.radius * sin( DEG2RAD( aDeg ) ) ) );
        };

        // Every axis crossing inside the sweep extends the box
        for( double axis = std::ceil( start / 90.0 ) * 90.0; axis <= end; axis += 90.0 )
        {
            VECTOR2I p = onArc( axis );
            m_contourBox.Merge( toKi( p.x, p.y ) );
        }

        VECTOR2I p0 = onArc( start );
        VECTOR2I p1 = onArc( end );
        m_contourBox.Merge( toKi( p0.x, p0.y ) );
        m_contourBox.Merge( toKi( p1.x, p1.y ) );
        m_contourBox.Merge( toKi( a.endX, a.endY ) );
        break;
    }

    case ELEMENT_TYPE::TRIANGLE:
    case ELEMENT_TYPE::RECTANGLE:
    {
        SHAPE_POLY_SET outline;

        if( elementOutline( aElem, outline ) )
            m_contourBox.Merge( outline.BBox() );

        break;
    }

    default:
        break;
    }
}


void TARGET3001_PCB_BUILDER::addItem( const ELEMENT& aElem, BOARD_ITEM_CONTAINER* aParent, FOOTPRINT* aFootprint,
                                      bool aIsTrack )
{
    const ELEMENT_BASE& b = aElem.base;

    if( b.layer == m_contourLayer && b.flags == 0 && aElem.type != ELEMENT_TYPE::TEXT )
        mergeContour( aElem );

    // Hidden board elements are not drawn; hidden text is kept as invisible text
    if( ( b.flags & 1 ) && aElem.type != ELEMENT_TYPE::TEXT )
        return;

    if( aElem.type == ELEMENT_TYPE::PAD )
    {
        if( aFootprint )
            addPad( aElem, aFootprint );
        else if( aIsTrack )
            addVia( aElem );
        else
            addPad( aElem, nullptr );

        return;
    }

    // Pour areas and keepouts are collected and turned into zones at the end
    if( std::optional<PCB_LAYER_ID> pourLayer = pourCopperLayer( b.layer ) )
    {
        SHAPE_POLY_SET outline;

        if( aElem.type == ELEMENT_TYPE::TEXT )
        {
            if( ( b.flags & 1 ) || aElem.text.weight == 0 )
                return;

            // Text on these layers cuts or fills by its glyph strokes
            PCB_TEXT glyphs( m_board );
            glyphs.SetText( m_project.FunctionText( aElem.text, false ).value_or( ToWx( aElem.text.text ) ) );
            configureText( &glyphs, aElem, *pourLayer );
            glyphs.TransformTextToPolySet( outline, 0, ARC_HIGH_DEF, ERROR_INSIDE );
        }
        else if( !elementOutline( aElem, outline ) )
        {
            return;
        }

        LAYER_FUNCTION f = LayerFunction( b.layer );
        bool isArea = f == LAYER_FUNCTION::AREA_BOTTOM || f == LAYER_FUNCTION::AREA_INSIDE
                      || f == LAYER_FUNCTION::AREA_TOP;

        // Flagged areas take no part in the fill
        if( isArea && b.signal && b.flags == 0 )
            m_pourAreas.push_back( { b.signal, *pourLayer, toKi( b.x, b.y ), std::move( outline ) } );
        else if( !isArea )
            m_pourKeepouts[*pourLayer].Append( outline );

        return;
    }

    std::vector<PCB_LAYER_ID> layers;

    // The all-copper layer draws the same element, unmirrored, on every copper layer
    if( b.layer == ALL_COPPER_LAYER )
    {
        for( PCB_LAYER_ID cu : m_board->GetEnabledLayers().CuStack() )
            layers.push_back( cu );
    }
    else if( std::optional<PCB_LAYER_ID> layer = mapLayer( b.layer ) )
    {
        layers.push_back( *layer );
    }

    for( PCB_LAYER_ID layer : layers )
    {
        if( aElem.type == ELEMENT_TYPE::TEXT )
            addText( aElem, aParent, aFootprint, layer );
        else
            addShape( aElem, aParent, aFootprint, aIsTrack, layer );
    }
}


void TARGET3001_PCB_BUILDER::addShape( const ELEMENT& aElem, BOARD_ITEM_CONTAINER* aParent, FOOTPRINT* aFootprint,
                                       bool aIsTrack, PCB_LAYER_ID aLayer )
{
    const ELEMENT_BASE& b = aElem.base;
    bool                copper = IsCopperLayer( aLayer );

    if( copper )
    {
        std::optional<int> clearance;

        if( aElem.type == ELEMENT_TYPE::LINE )
            clearance = aElem.line.clearance;
        else if( aElem.type == ELEMENT_TYPE::ARC )
            clearance = aElem.arc.clearance;
        else if( aElem.type == ELEMENT_TYPE::SPLINE )
            clearance = aElem.spline.clearance;

        // A zero clearance keeps the netclass clearance rather than letting another signal's area touch
        if( clearance && *clearance > 0 && b.signal )
            m_signalClearances[b.signal][*clearance]++;
    }

    switch( aElem.type )
    {
    case ELEMENT_TYPE::LINE:
    {
        const LINE& l = aElem.line;

        if( copper && aIsTrack && !aFootprint )
        {
            PCB_TRACK* track = new PCB_TRACK( m_board );
            track->SetStart( toKi( b.x, b.y ) );
            track->SetEnd( toKi( l.x2, l.y2 ) );
            track->SetWidth( l.width );
            track->SetLayer( aLayer );
            track->SetNet( net( b.signal ) );
            m_board->Add( track, ADD_MODE::APPEND );
            return;
        }

        PCB_SHAPE* shape = new PCB_SHAPE( aParent, SHAPE_T::SEGMENT );
        shape->SetStart( toKi( b.x, b.y ) );
        shape->SetEnd( toKi( l.x2, l.y2 ) );
        shape->SetStroke( STROKE_PARAMS( l.width, lineStyle( l.dashStyle ) ) );
        shape->SetLayer( aLayer );

        if( copper )
            shape->SetNet( net( b.signal ) );

        aParent->Add( shape, ADD_MODE::APPEND );
        return;
    }

    case ELEMENT_TYPE::ARC:
    {
        const ARC& a = aElem.arc;
        VECTOR2I   center = toKi( a.centerX, a.centerY );
        double     sweep = a.Sweep();

        bool full = sweep >= 360.0 - 1e-6;

        if( full )
        {
            PCB_SHAPE* shape = new PCB_SHAPE( aParent, SHAPE_T::CIRCLE );
            shape->SetCenter( center );
            shape->SetRadius( a.radius );
            shape->SetStroke( STROKE_PARAMS( a.width, LINE_STYLE::SOLID ) );
            shape->SetLayer( aLayer );

            if( copper )
                shape->SetNet( net( b.signal ) );

            aParent->Add( shape, ADD_MODE::APPEND );
            return;
        }

        double   midDeg = double( a.startAngle ) + sweep / 2.0;
        VECTOR2I start = center + VECTOR2I( KiROUND( a.radius * cos( DEG2RAD( a.startAngle ) ) ),
                                            -KiROUND( a.radius * sin( DEG2RAD( a.startAngle ) ) ) );
        VECTOR2I mid = center + VECTOR2I( KiROUND( a.radius * cos( DEG2RAD( midDeg ) ) ),
                                          -KiROUND( a.radius * sin( DEG2RAD( midDeg ) ) ) );
        VECTOR2I end = center + VECTOR2I( KiROUND( a.radius * cos( DEG2RAD( a.startAngle + sweep ) ) ),
                                          -KiROUND( a.radius * sin( DEG2RAD( a.startAngle + sweep ) ) ) );

        if( copper && aIsTrack && !aFootprint )
        {
            PCB_ARC* arc = new PCB_ARC( m_board );
            arc->SetStart( start );
            arc->SetMid( mid );
            arc->SetEnd( end );
            arc->SetWidth( a.width );
            arc->SetLayer( aLayer );
            arc->SetNet( net( b.signal ) );
            m_board->Add( arc, ADD_MODE::APPEND );
            return;
        }

        PCB_SHAPE* shape = new PCB_SHAPE( aParent, SHAPE_T::ARC );
        shape->SetArcGeometry( start, mid, end );
        shape->SetStroke( STROKE_PARAMS( a.width, LINE_STYLE::SOLID ) );
        shape->SetLayer( aLayer );

        if( copper )
            shape->SetNet( net( b.signal ) );

        aParent->Add( shape, ADD_MODE::APPEND );
        return;
    }

    case ELEMENT_TYPE::SPLINE:
    {
        const SPLINE& s = aElem.spline;
        VECTOR2I      p0 = toKi( b.x, b.y );
        VECTOR2I      c = toKi( s.controlX, s.controlY );
        VECTOR2I      p2 = toKi( s.endX, s.endY );

        // Elevate the quadratic curve to the cubic form KiCad stores
        PCB_SHAPE* shape = new PCB_SHAPE( aParent, SHAPE_T::BEZIER );
        shape->SetStart( p0 );
        shape->SetBezierC1( p0 + ( c - p0 ) * 2 / 3 );
        shape->SetBezierC2( p2 + ( c - p2 ) * 2 / 3 );
        shape->SetEnd( p2 );
        shape->SetStroke( STROKE_PARAMS( s.width, LINE_STYLE::SOLID ) );
        shape->SetLayer( aLayer );
        shape->RebuildBezierToSegmentsPointsList( ARC_HIGH_DEF );

        if( copper )
            shape->SetNet( net( b.signal ) );

        aParent->Add( shape, ADD_MODE::APPEND );
        return;
    }

    case ELEMENT_TYPE::TRIANGLE:
    case ELEMENT_TYPE::RECTANGLE:
    case ELEMENT_TYPE::DISC:
    {
        SHAPE_POLY_SET outline;
        elementOutline( aElem, outline );

        // Stored copper fills are triangulated; merge them back into polygons per net and layer
        if( copper && aIsTrack && !aFootprint && aElem.type == ELEMENT_TYPE::TRIANGLE )
        {
            m_copperFills[{ b.signal, aLayer }].Append( outline );
            return;
        }

        PCB_SHAPE* shape;

        if( aElem.type == ELEMENT_TYPE::DISC )
        {
            shape = new PCB_SHAPE( aParent, SHAPE_T::CIRCLE );
            shape->SetCenter( toKi( b.x, b.y ) );
            shape->SetRadius( aElem.disc.radius );
        }
        else
        {
            shape = new PCB_SHAPE( aParent, SHAPE_T::POLY );
            shape->SetPolyShape( outline );
        }

        shape->SetFilled( true );
        shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        shape->SetLayer( aLayer );

        if( copper )
            shape->SetNet( net( b.signal ) );

        aParent->Add( shape, ADD_MODE::APPEND );
        return;
    }

    case ELEMENT_TYPE::FIDUCIAL:
        for( std::unique_ptr<PCB_SHAPE>& shape : fiducialShapes( aElem, aParent ) )
        {
            shape->SetLayer( aLayer );

            if( copper )
                shape->SetNet( net( b.signal ) );

            aParent->Add( shape.release(), ADD_MODE::APPEND );
        }

        return;

    default:
        return;
    }
}


std::vector<std::unique_ptr<PCB_SHAPE>> TARGET3001_PCB_BUILDER::fiducialShapes( const ELEMENT& aElem,
                                                                                BOARD_ITEM_CONTAINER* aParent ) const
{
    const FIDUCIAL&                         f = aElem.fiducial;
    const ELEMENT_BASE&                     b = aElem.base;
    std::vector<std::unique_ptr<PCB_SHAPE>> shapes;

    auto circle = [&]( int aRadius, int aWidth, bool aFilled )
    {
        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( aParent, SHAPE_T::CIRCLE );
        shape->SetCenter( toKi( b.x, b.y ) );
        shape->SetRadius( aRadius );
        shape->SetStroke( STROKE_PARAMS( aWidth, LINE_STYLE::SOLID ) );
        shape->SetFilled( aFilled );
        shapes.push_back( std::move( shape ) );
    };

    if( f.kind == uint8_t( TARGET3001::FIDUCIAL_KIND::FILLED_CIRCLE ) )
    {
        circle( f.radius, 0, true );
        return shapes;
    }

    int ring = std::max( 0, f.radius - f.width / 2 );
    circle( ring, f.width, false );

    if( f.kind != uint8_t( TARGET3001::FIDUCIAL_KIND::CROSSHAIR_RINGS ) )
        return shapes;

    circle( KiROUND( ring * 2.0 / 3.0 ), f.width, false );
    circle( KiROUND( ring / 3.0 ), f.width, false );

    // Cross arms reach the outer ring's centreline along the mark's own axes
    for( double axis : { 0.0, 90.0 } )
    {
        double  rad = DEG2RAD( f.direction + axis );
        int32_t dx = KiROUND( ring * cos( rad ) );
        int32_t dy = KiROUND( ring * sin( rad ) );

        std::unique_ptr<PCB_SHAPE> arm = std::make_unique<PCB_SHAPE>( aParent, SHAPE_T::SEGMENT );
        arm->SetStart( toKi( b.x + dx, b.y + dy ) );
        arm->SetEnd( toKi( b.x - dx, b.y - dy ) );
        arm->SetStroke( STROKE_PARAMS( f.width, LINE_STYLE::SOLID ) );
        shapes.push_back( std::move( arm ) );
    }

    return shapes;
}


void TARGET3001_PCB_BUILDER::addPad( const ELEMENT& aElem, FOOTPRINT* aFootprint )
{
    const TARGET3001::PAD& src = aElem.pad;
    const ELEMENT_BASE& b = aElem.base;

    FOOTPRINT* fp = aFootprint;

    // Free pads (mounting holes, test points) get a footprint of their own
    if( !fp )
    {
        fp = new FOOTPRINT( m_board );
        fp->SetPosition( toKi( b.x, b.y ) );
        fp->SetReference( wxString::Format( wxS( "H%d" ), ++m_freePadCount ) );
        fp->Reference().SetVisible( false );
        fp->Value().SetVisible( false );
        fp->SetAttributes( FP_BOARD_ONLY | FP_EXCLUDE_FROM_BOM | FP_EXCLUDE_FROM_POS_FILES );
        m_board->Add( fp, ADD_MODE::APPEND );
    }

    ::PAD* pad = new ::PAD( fp );
    pad->SetPosition( toKi( b.x, b.y ) );
    pad->SetOrientation( angle( src.direction ) );

    VECTOR2I            size( std::max( 1, src.width ), std::max( 1, src.height ) );
    std::optional<LSET> padstack = padstackLayers( b.layer );

    pad->SetSize( PADSTACK::ALL_LAYERS, size );

    switch( TARGET3001::PAD_SHAPE( src.shape ) )
    {
    case TARGET3001::PAD_SHAPE::ROUND:
        if( size.x == size.y )
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, ::PAD_SHAPE::CIRCLE );
        }
        else
        {
            // Unequal round pads are true ellipses, which KiCad only has as a custom shape
            bool          wide = size.x > size.y;
            SHAPE_ELLIPSE ellipse( VECTOR2I( 0, 0 ), std::max( size.x, size.y ) / 2,
                                   std::min( size.x, size.y ) / 2, wide ? ANGLE_0 : ANGLE_90 );
            SHAPE_POLY_SET outline( ellipse.ConvertToPolyline( m_board->GetDesignSettings().m_MaxError ) );
            int            anchor = std::min( size.x, size.y );

            pad->SetShape( PADSTACK::ALL_LAYERS, ::PAD_SHAPE::CUSTOM );
            pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, ::PAD_SHAPE::CIRCLE );
            pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( anchor, anchor ) );
            pad->AddPrimitivePoly( PADSTACK::ALL_LAYERS, outline, 0, true );
        }

        break;

    case TARGET3001::PAD_SHAPE::POLYGON:
    {
        const std::string*    prop = b.FindProp( "POLY_PAD" );
        std::vector<VECTOR2I> pts = prop ? parsePolyPad( *prop ) : std::vector<VECTOR2I>();

        if( pts.size() >= 3 )
        {
            std::vector<VECTOR2I> local;

            for( const VECTOR2I& pt : pts )
                local.emplace_back( pt.x, -pt.y );

            // The custom shape carries all the copper; the anchor only needs to sit inside it
            int anchor = pcbIUScale.mmToIU( 0.001 );

            pad->SetShape( PADSTACK::ALL_LAYERS, ::PAD_SHAPE::CUSTOM );
            pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, ::PAD_SHAPE::CIRCLE );
            pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( anchor, anchor ) );
            pad->AddPrimitivePoly( PADSTACK::ALL_LAYERS, local, 0, true );
            break;
        }

        // Fewer than three vertices falls back to an octagon
        KI_FALLTHROUGH;
    }

    case TARGET3001::PAD_SHAPE::OCTAGON:
        pad->SetShape( PADSTACK::ALL_LAYERS, ::PAD_SHAPE::CHAMFERED_RECT );
        pad->SetChamferRectRatio( PADSTACK::ALL_LAYERS, OCTAGON_CHAMFER_RATIO );
        pad->SetChamferPositions( PADSTACK::ALL_LAYERS, RECT_CHAMFER_ALL );
        break;

    case TARGET3001::PAD_SHAPE::OVAL:
        pad->SetShape( PADSTACK::ALL_LAYERS, ::PAD_SHAPE::OVAL );
        break;

    case TARGET3001::PAD_SHAPE::RECTANGLE:
    default:
    {
        long rounded = 0;

        // RECT_ROUNDED is the corner diameter in percent of the smaller side
        if( const std::string* prop = b.FindProp( "RECT_ROUNDED" ) )
            wxString( *prop ).ToLong( &rounded );

        if( rounded > 0 )
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, ::PAD_SHAPE::ROUNDRECT );
            pad->SetRoundRectRadiusRatio( PADSTACK::ALL_LAYERS, std::min( 0.5, rounded / 200.0 ) );
        }
        else
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, ::PAD_SHAPE::RECTANGLE );
        }

        break;
    }
    }

    pad->SetNumber( TARGET3001::PadNumber( aElem ) );

    bool hasHole = src.drill > 0;

    if( hasHole )
    {
        pad->SetDrillSize( VECTOR2I( src.drill, src.drill ) );

        if( const std::string* slot = b.FindProp( "SLOT_HOLE" ) )
        {
            long length = 0;

            if( wxString( *slot ).ToLong( &length ) && length > src.drill )
            {
                pad->SetDrillShape( PAD_DRILL_SHAPE::OBLONG );
                pad->SetDrillSize( VECTOR2I( int( length ), src.drill ) );
            }
        }

        // A hole is unplated only when it is larger than its copper
        bool plated = std::max( src.width, src.height ) >= src.drill;

        pad->SetAttribute( plated ? PAD_ATTRIB::PTH : PAD_ATTRIB::NPTH );

        if( !plated )
            pad->SetLayerSet( ::PAD::UnplatedHoleMask() );
        else if( padstack )
            pad->SetLayerSet( *padstack );
        else
            pad->SetLayerSet( ::PAD::PTHMask() );

        if( !plated )
        {
            VECTOR2I hole = pad->GetDrillSize();

            pad->DeletePrimitivesList();
            pad->SetShape( PADSTACK::ALL_LAYERS, hole.x == hole.y ? ::PAD_SHAPE::CIRCLE : ::PAD_SHAPE::OVAL );
            pad->SetSize( PADSTACK::ALL_LAYERS, hole );
        }
    }
    else if( padstack )
    {
        pad->SetAttribute( PAD_ATTRIB::SMD );
        pad->SetLayerSet( *padstack );
    }
    else
    {
        std::optional<PCB_LAYER_ID> copper = mapLayer( b.layer );
        PCB_LAYER_ID                side = copper && IsCopperLayer( *copper ) ? *copper : F_Cu;
        LSET                        layers( { side } );

        if( side == F_Cu )
            layers |= LSET( { F_Mask, F_Paste } );
        else if( side == B_Cu )
            layers |= LSET( { B_Mask, B_Paste } );

        pad->SetAttribute( PAD_ATTRIB::SMD );
        pad->SetLayerSet( layers );
    }

    if( src.clearance > 0 )
        pad->SetLocalClearance( src.clearance );

    pad->SetNet( net( b.signal ) );

    if( b.signal )
        tagAura( *pad, b, src.clearance );

    fp->Add( pad, ADD_MODE::APPEND );

    if( padstack )
        addPadstackCuts( *pad, b.layer - TARGET3001::FIRST_PADSTACK_LAYER );
}


void TARGET3001_PCB_BUILDER::addPadstackCuts( const BOARD_ITEM& aItem, int aPadstack )
{
    const TARGET3001::PADSTACK_DEF& def = m_project.settings.padstacks[aPadstack];

    // A padstack flagged on a deletion layer cuts the computed copper there, grown by its oversize
    for( int layer = 0; layer < TARGET3001::BOARD_LAYER_COUNT; layer++ )
    {
        LAYER_FUNCTION f = LayerFunction( layer );

        if( !def.onLayer[layer]
            || ( f != LAYER_FUNCTION::DELETION_BOTTOM && f != LAYER_FUNCTION::DELETION_INSIDE
                 && f != LAYER_FUNCTION::DELETION_TOP ) )
        {
            continue;
        }

        if( std::optional<PCB_LAYER_ID> copper = pourCopperLayer( layer ) )
        {
            aItem.TransformShapeToPolygon( m_pourKeepouts[*copper], *copper, std::max( 0, def.oversize[layer] ),
                                           ARC_HIGH_DEF, ERROR_OUTSIDE );
        }
    }
}


std::optional<LSET> TARGET3001_PCB_BUILDER::padstackLayers( int aLayer ) const
{
    const std::vector<TARGET3001::PADSTACK_DEF>& padstacks = m_project.settings.padstacks;
    size_t                                       index = size_t( aLayer - TARGET3001::FIRST_PADSTACK_LAYER );

    if( aLayer < TARGET3001::FIRST_PADSTACK_LAYER || index >= padstacks.size() )
        return std::nullopt;

    LSET layers;

    for( int layer = 0; layer < TARGET3001::BOARD_LAYER_COUNT; layer++ )
    {
        if( !padstacks[index].onLayer[layer] )
            continue;

        std::optional<PCB_LAYER_ID> mapped = mapLayer( layer );

        if( mapped && ( IsCopperLayer( *mapped ) || IsSolderMaskLayer( *mapped ) || *mapped == F_Paste
                        || *mapped == B_Paste ) )
        {
            layers.set( *mapped );
        }
    }

    return layers;
}


void TARGET3001_PCB_BUILDER::addVia( const ELEMENT& aElem )
{
    const TARGET3001::PAD& src = aElem.pad;
    const ELEMENT_BASE& b = aElem.base;

    TARGET3001::PAD_SHAPE shape = TARGET3001::PAD_SHAPE( src.shape );
    bool                  slotted = b.FindProp( "SLOT_HOLE" ) != nullptr;
    std::optional<LSET>   padstack = padstackLayers( b.layer );
    LSET                  allCopper = LSET::AllCuMask( m_board->GetCopperLayerCount() );

    // Copper without a hole is a pad-shaped piece of copper; a hole larger than its copper is unplated.
    // Slots, oval or rectangular copper and padstacks missing a copper layer have no via equivalent.
    if( src.drill <= 0 || std::max( src.width, src.height ) < src.drill || slotted
        || shape == TARGET3001::PAD_SHAPE::OVAL || shape == TARGET3001::PAD_SHAPE::RECTANGLE
        || ( padstack && ( *padstack & allCopper ) != allCopper ) )
    {
        addPad( aElem, nullptr );
        return;
    }

    PCB_VIA* via = new PCB_VIA( m_board );
    via->SetPosition( toKi( b.x, b.y ) );
    via->SetWidth( PADSTACK::ALL_LAYERS, std::max( src.width, src.height ) );
    via->SetDrill( src.drill );
    via->SetViaType( VIATYPE::THROUGH );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetNet( net( b.signal ) );

    if( b.signal )
        tagAura( *via, b, src.clearance );

    m_board->Add( via, ADD_MODE::APPEND );

    if( b.signal && src.clearance > 0 )
        m_signalClearances[b.signal][src.clearance]++;

    if( padstack )
        addPadstackCuts( *via, b.layer - TARGET3001::FIRST_PADSTACK_LAYER );
}


void TARGET3001_PCB_BUILDER::configureText( PCB_TEXT* aText, const ELEMENT& aElem, PCB_LAYER_ID aLayer )
{
    const TEXT&         t = aElem.text;
    const ELEMENT_BASE& b = aElem.base;

    aText->SetLayer( aLayer );
    m_fonts.Apply( aText, aElem, 1 );
    aText->SetTextAngle( angle( t.direction ) );
    aText->SetMirrored( t.mirrored != 0 );
    aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    aText->SetTextPos( toKi( b.x, b.y ) );

    // Right-aligned text also stores its right end, which keeps the alignment when font metrics differ
    if( std::optional<std::pair<int32_t, int32_t>> end = b.TextRightEnd() )
    {
        // Mirroring flips both ends, so the stored end stays the right-justified one
        aText->SetTextPos( toKi( end->first, end->second ) );
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
    }

    aText->SetTextPos( aText->GetTextPos() + TARGET3001::FONTS::BaselineShift( *aText ) );
    aText->SetVisible( !( b.flags & 1 ) && t.weight != 0 );
}


void TARGET3001_PCB_BUILDER::addText( const ELEMENT& aElem, BOARD_ITEM_CONTAINER* aParent,
                                      FOOTPRINT* aFootprint, PCB_LAYER_ID aLayer )
{
    const TEXT& t = aElem.text;

    if( aFootprint && t.function == uint8_t( TEXT_FUNCTION::REFERENCE ) )
    {
        aFootprint->Reference().SetText( ToWx( t.text ) );
        configureText( &aFootprint->Reference(), aElem, aLayer );
        return;
    }

    if( aFootprint && t.function == uint8_t( TEXT_FUNCTION::VALUE ) )
    {
        aFootprint->Value().SetText( ToWx( t.text ) );
        configureText( &aFootprint->Value(), aElem, aLayer );
        return;
    }

    PCB_TEXT* text = new PCB_TEXT( aParent );
    text->SetText( m_project.FunctionText( t, false ).value_or( ToWx( t.text ) ) );
    configureText( text, aElem, aLayer );
    aParent->Add( text, ADD_MODE::APPEND );
}
