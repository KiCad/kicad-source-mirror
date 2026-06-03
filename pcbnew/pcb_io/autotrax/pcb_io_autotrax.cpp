/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Format interpretation derived from pcb-rnd src_plugins/io_autotrax:
 *   Copyright (C) 2016, 2017, 2018, 2020 Tibor 'Igor2' Palinkas
 *   Copyright (C) 2016, 2017 Erich S. Heinzle
 * Used under GPL v2-or-later.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_io_autotrax.h"
#include "autotrax_parser.h"

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <zone.h>
#include <netinfo.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/eda_angle.h>
#include <geometry/shape_poly_set.h>
#include <reporter.h>
#include <math/util.h>
#include <math/vector2d.h>

#include <geometry/shape_line_chain.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

using AUTOTRAX::ARC;
using AUTOTRAX::BOARD_DATA;
using AUTOTRAX::FILL;
using AUTOTRAX::NET_NODE;
using AUTOTRAX::TEXT;
using AUTOTRAX::TRACK;
using AUTOTRAX::VIA;

// AUTOTRAX::PAD and AUTOTRAX::COMPONENT collide with KiCad's global PAD class and
// board.h's COMPONENT class, so both are always spelled out with the namespace
// prefix here and the KiCad PAD class is reached as ::PAD.


PCB_IO_AUTOTRAX::PCB_IO_AUTOTRAX() :
        PCB_IO( wxS( "Protel Autotrax" ) )
{
}


PCB_IO_AUTOTRAX::~PCB_IO_AUTOTRAX()
{
}


/// Read a file into @p aOut. A non-zero @p aLimit reads at most that many bytes,
/// which CanReadBoard() uses to sniff the header without slurping a whole board.
///
/// The raw bytes are decoded as UTF-8, falling back to Latin-1 for the DOS-era
/// 8-bit encodings these legacy files often use; Latin-1 never fails, so the
/// whole board is never lost to a single stray high byte.
static bool readFile( const wxString& aFileName, wxString& aOut, size_t aLimit = 0 )
{
    std::ifstream file( aFileName.fn_str(), std::ios::binary );

    if( !file.is_open() )
        return false;

    std::string raw;

    if( aLimit > 0 )
    {
        raw.resize( aLimit );
        file.read( raw.data(), aLimit );
        raw.resize( static_cast<size_t>( file.gcount() ) );
    }
    else
    {
        std::ostringstream ss;
        ss << file.rdbuf();
        raw = ss.str();
    }

    aOut = wxString::FromUTF8( raw.data(), raw.size() );

    if( aOut.IsEmpty() && !raw.empty() )
        aOut = wxString::From8BitData( raw.data(), raw.size() );

    return true;
}


bool PCB_IO_AUTOTRAX::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    // The magic header is in the first non-blank line, so a small prefix read is
    // enough to disambiguate from the gEDA .pcb files sharing this extension.
    wxString contents;

    if( !readFile( aFileName, contents, 4096 ) )
        return false;

    return AUTOTRAX_PARSER::Sniff( contents );
}


BOARD* PCB_IO_AUTOTRAX::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                   const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    m_props = aProperties;
    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Own a freshly-allocated board until parsing succeeds so a thrown IO_ERROR
    // does not leak it. An appended board belongs to the caller.
    std::unique_ptr<BOARD> boardDeleter( aAppendToMe ? nullptr : m_board );

    m_nets.clear();
    m_maxY = 0;

    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    wxString contents;

    if( !readFile( aFileName, contents ) )
        THROW_IO_ERROR( wxString::Format( _( "Could not read file '%s'." ), aFileName ) );

    REPORTER&  reporter = m_reporter ? *m_reporter : NULL_REPORTER::GetInstance();
    BOARD_DATA data;

    AUTOTRAX_PARSER parser( &reporter );

    if( !parser.Parse( contents, data ) )
        THROW_IO_ERROR( wxString::Format( _( "'%s' is not a valid Protel Autotrax file." ), aFileName ) );

    buildBoard( data );

    boardDeleter.release();
    return m_board;
}


bool PCB_IO_AUTOTRAX::mapLayer( int aLayer, PCB_LAYER_ID& aResult ) const
{
    using namespace AUTOTRAX;

    switch( aLayer )
    {
    case LAYER_TOP_COPPER: aResult = F_Cu; return true;
    case LAYER_MID1: aResult = In1_Cu; return true;
    case LAYER_MID2: aResult = In2_Cu; return true;
    case LAYER_MID3: aResult = In3_Cu; return true;
    case LAYER_MID4: aResult = In4_Cu; return true;
    case LAYER_BOTTOM_COPPER: aResult = B_Cu; return true;
    case LAYER_TOP_SILK: aResult = F_SilkS; return true;
    case LAYER_BOTTOM_SILK: aResult = B_SilkS; return true;
    case LAYER_GND_PLANE: aResult = In5_Cu; return true;
    case LAYER_POWER_PLANE: aResult = In6_Cu; return true;
    case LAYER_BOARD: aResult = Edge_Cuts; return true;
    case LAYER_MULTI: aResult = F_Cu; return true; // through-all features
    default:
        // 0 (unset) and 12 (keepout) have no KiCad target and are dropped.
        return false;
    }
}


NETINFO_ITEM* PCB_IO_AUTOTRAX::getNet( const wxString& aNetName )
{
    if( aNetName.IsEmpty() )
        return nullptr;

    auto it = m_nets.find( aNetName );

    if( it != m_nets.end() )
        return it->second;

    // Reuse a net already on the board (e.g. when appending) so codes and names
    // are never duplicated.
    if( NETINFO_ITEM* existing = m_board->FindNet( aNetName ) )
    {
        m_nets[aNetName] = existing;
        return existing;
    }

    NETINFO_ITEM* net = new NETINFO_ITEM( m_board, aNetName );
    m_board->Add( net );
    m_nets[aNetName] = net;
    return net;
}


BOARD_ITEM* PCB_IO_AUTOTRAX::parentOf( FOOTPRINT* aFootprint ) const
{
    return aFootprint ? static_cast<BOARD_ITEM*>( aFootprint ) : static_cast<BOARD_ITEM*>( m_board );
}


void PCB_IO_AUTOTRAX::addItem( BOARD_ITEM* aItem, FOOTPRINT* aFootprint )
{
    if( aFootprint )
        aFootprint->Add( aItem );
    else
        m_board->Add( aItem, ADD_MODE::APPEND );
}


/// One arc as a (start angle, signed sweep) pair in the file's native Y-down
/// frame.
struct ARC_SPAN
{
    double startDeg;
    double deltaDeg;
};


/// Translate an Autotrax arc quadrant bitmask into the arc spans it represents.
/// Most masks are a single contiguous run of quadrants, but masks 5 and 10
/// encode two disjoint quadrants and therefore yield two arcs.
static std::vector<ARC_SPAN> arcSpansFromSegments( int aSegments )
{
    switch( aSegments )
    {
    case 1: return { { 90.0, 90.0 } };                  // RU quadrant
    case 2: return { { 0.0, 90.0 } };                   // LU quadrant
    case 4: return { { 270.0, 90.0 } };                 // LL quadrant
    case 8: return { { 180.0, 90.0 } };                 // RL quadrant
    case 3: return { { 0.0, 180.0 } };                  // upper half
    case 6: return { { 270.0, 180.0 } };                // left half
    case 12: return { { 180.0, 180.0 } };               // lower half
    case 9: return { { 90.0, 180.0 } };                 // right half
    case 14: return { { 180.0, 270.0 } };               // not RU
    case 13: return { { 90.0, 270.0 } };                // not LU
    case 11: return { { 0.0, 270.0 } };                 // not LL
    case 7: return { { 270.0, 270.0 } };                // not RL
    case 5: return { { 270.0, 90.0 }, { 90.0, 90.0 } }; // RU + LL quadrants
    case 10: return { { 180.0, 90.0 }, { 0.0, 90.0 } }; // LU + RL quadrants
    default: return { { 0.0, 360.0 } };                 // full circle
    }
}


void PCB_IO_AUTOTRAX::emitTrack( const TRACK& aTrack, FOOTPRINT* aFootprint )
{
    PCB_LAYER_ID layer;

    if( !mapLayer( aTrack.layer, layer ) )
        return;

    // A free copper segment becomes a routed PCB_TRACK. Everything else, and any
    // segment owned by a footprint, becomes a graphic PCB_SHAPE so it stays with
    // the footprint (KiCad footprints cannot own PCB_TRACK objects).
    if( IsCopperLayer( layer ) && !aFootprint )
    {
        PCB_TRACK* track = new PCB_TRACK( m_board );
        track->SetStart( toBoard( aTrack.x1, aTrack.y1 ) );
        track->SetEnd( toBoard( aTrack.x2, aTrack.y2 ) );
        track->SetWidth( std::max( 1, toIU( aTrack.width ) ) );
        track->SetLayer( layer );
        m_board->Add( track, ADD_MODE::APPEND );
        return;
    }

    PCB_SHAPE* shape = new PCB_SHAPE( parentOf( aFootprint ), SHAPE_T::SEGMENT );
    shape->SetLayer( layer );
    shape->SetStart( toBoard( aTrack.x1, aTrack.y1 ) );
    shape->SetEnd( toBoard( aTrack.x2, aTrack.y2 ) );
    shape->SetWidth( std::max( 1, toIU( aTrack.width ) ) );
    addItem( shape, aFootprint );
}


void PCB_IO_AUTOTRAX::emitArc( const ARC& aArc, FOOTPRINT* aFootprint )
{
    PCB_LAYER_ID layer;

    if( !mapLayer( aArc.layer, layer ) )
        return;

    int      r = toIU( aArc.radius );
    int      width = std::max( 1, toIU( aArc.width ) );
    VECTOR2I center = toBoard( aArc.centerX, aArc.centerY );

    // A point on the circle for an angle in the Y-down file frame. toBoard()
    // applies the Y flip so the resulting geometry lands in KiCad space.
    auto pointAt = [&]( double aDeg )
    {
        double rad = aDeg * M_PI / 180.0;
        return toBoard( aArc.centerX + aArc.radius * std::cos( rad ), aArc.centerY + aArc.radius * std::sin( rad ) );
    };

    for( const ARC_SPAN& span : arcSpansFromSegments( aArc.segments ) )
    {
        PCB_SHAPE* shape = new PCB_SHAPE( parentOf( aFootprint ) );
        shape->SetLayer( layer );
        shape->SetWidth( width );

        if( span.deltaDeg >= 360.0 )
        {
            shape->SetShape( SHAPE_T::CIRCLE );
            shape->SetCenter( center );
            shape->SetEnd( VECTOR2I( center.x + r, center.y ) );
        }
        else
        {
            shape->SetShape( SHAPE_T::ARC );
            shape->SetArcGeometry( pointAt( span.startDeg ), pointAt( span.startDeg + span.deltaDeg / 2.0 ),
                                   pointAt( span.startDeg + span.deltaDeg ) );
        }

        addItem( shape, aFootprint );
    }
}


void PCB_IO_AUTOTRAX::emitVia( const VIA& aVia, FOOTPRINT* aFootprint )
{
    int drill = std::max( 1, toIU( aVia.drill ) );
    int diameter = std::max( drill + 2, toIU( aVia.diameter ) );

    if( aFootprint )
    {
        // A via inside a component becomes a through-hole pad so it travels with
        // the footprint.
        AUTOTRAX::PAD pad;
        pad.x = aVia.x;
        pad.y = aVia.y;
        pad.xSize = aVia.diameter;
        pad.ySize = aVia.diameter;
        pad.shape = 1;
        pad.drill = aVia.drill;
        pad.layer = AUTOTRAX::LAYER_MULTI;
        emitPad( pad, aFootprint );
        return;
    }

    PCB_VIA* via = new PCB_VIA( m_board );
    via->SetPosition( toBoard( aVia.x, aVia.y ) );
    via->SetDrill( drill );
    via->SetWidth( PADSTACK::ALL_LAYERS, diameter );
    via->SetViaType( VIATYPE::THROUGH );
    via->SetLayerPair( F_Cu, B_Cu );
    m_board->Add( via, ADD_MODE::APPEND );
}


void PCB_IO_AUTOTRAX::emitPad( const AUTOTRAX::PAD& aPad, FOOTPRINT* aFootprint )
{
    // Layer 11 ("board") pads and the unsupported target shapes are dropped.
    if( aPad.layer == AUTOTRAX::LAYER_BOARD || aPad.shape == 5 || aPad.shape == 6 )
        return;

    // Autotrax only places pads on the top (1), bottom (6) or multi/through (13)
    // layers; other pad layers are unsupported and dropped.
    if( aPad.layer != AUTOTRAX::LAYER_TOP_COPPER && aPad.layer != AUTOTRAX::LAYER_BOTTOM_COPPER
        && aPad.layer != AUTOTRAX::LAYER_MULTI )
    {
        return;
    }

    PCB_LAYER_ID layer;

    if( !mapLayer( aPad.layer, layer ) )
        return;

    // A free pad with no owning footprint still needs a footprint container so
    // KiCad can hold the pad; create a throwaway one anchored at the pad.
    FOOTPRINT* owner = aFootprint;
    bool       standalone = false;

    if( !owner )
    {
        owner = new FOOTPRINT( m_board );
        owner->SetPosition( toBoard( aPad.x, aPad.y ) );
        standalone = true;
    }

    ::PAD* pad = new ::PAD( owner );
    pad->SetNumber( aPad.name );
    pad->SetPosition( toBoard( aPad.x, aPad.y ) );

    VECTOR2I size( std::max( 1, toIU( aPad.xSize ) ), std::max( 1, toIU( aPad.ySize ) ) );
    pad->SetSize( PADSTACK::ALL_LAYERS, size );

    switch( aPad.shape )
    {
    case 2: pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE ); break;

    case 3: // octagon, approximated by chamfering all four corners
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CHAMFERED_RECT );
        pad->SetChamferRectRatio( PADSTACK::ALL_LAYERS, 0.25 );
        pad->SetChamferPositions( PADSTACK::ALL_LAYERS, RECT_CHAMFER_ALL );
        break;

    case 4: pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::ROUNDRECT ); break;

    default:
        pad->SetShape( PADSTACK::ALL_LAYERS, ( aPad.xSize == aPad.ySize ) ? PAD_SHAPE::CIRCLE : PAD_SHAPE::OVAL );
        break;
    }

    int drill = toIU( aPad.drill );

    if( drill > 0 )
    {
        pad->SetAttribute( PAD_ATTRIB::PTH );
        pad->SetDrillSize( VECTOR2I( drill, drill ) );
        pad->SetLayerSet( ::PAD::PTHMask() );
    }
    else
    {
        pad->SetAttribute( PAD_ATTRIB::SMD );

        LSET smdMask = ::PAD::SMDMask();

        if( layer == B_Cu )
            smdMask.FlipStandardLayers();

        pad->SetLayerSet( smdMask );
    }

    owner->Add( pad );

    if( standalone )
        m_board->Add( owner, ADD_MODE::APPEND );
}


void PCB_IO_AUTOTRAX::emitFill( const FILL& aFill, FOOTPRINT* aFootprint )
{
    PCB_LAYER_ID layer;

    if( !mapLayer( aFill.layer, layer ) )
        return;

    VECTOR2I p1 = toBoard( aFill.x1, aFill.y1 );
    VECTOR2I p2 = toBoard( aFill.x2, aFill.y2 );

    int minX = std::min( p1.x, p2.x );
    int maxX = std::max( p1.x, p2.x );
    int minY = std::min( p1.y, p2.y );
    int maxY = std::max( p1.y, p2.y );

    // Autotrax fills are rectangular pours. On copper layers they become a zone;
    // elsewhere a filled rectangle graphic.
    if( IsCopperLayer( layer ) && !aFootprint )
    {
        SHAPE_POLY_SET   poly;
        SHAPE_LINE_CHAIN outline;
        outline.Append( VECTOR2I( minX, minY ) );
        outline.Append( VECTOR2I( maxX, minY ) );
        outline.Append( VECTOR2I( maxX, maxY ) );
        outline.Append( VECTOR2I( minX, maxY ) );
        outline.SetClosed( true );
        poly.AddOutline( outline );

        ZONE* zone = new ZONE( m_board );
        zone->SetLayer( layer );
        zone->SetOutline( new SHAPE_POLY_SET( poly ) );
        zone->SetAssignedPriority( 0 );
        m_board->Add( zone, ADD_MODE::APPEND );
        return;
    }

    PCB_SHAPE* shape = new PCB_SHAPE( parentOf( aFootprint ), SHAPE_T::RECTANGLE );
    shape->SetLayer( layer );
    shape->SetStart( VECTOR2I( minX, minY ) );
    shape->SetEnd( VECTOR2I( maxX, maxY ) );
    shape->SetFilled( true );
    shape->SetWidth( 0 );
    addItem( shape, aFootprint );
}


void PCB_IO_AUTOTRAX::emitText( const TEXT& aText, FOOTPRINT* aFootprint )
{
    PCB_LAYER_ID layer;

    if( !mapLayer( aText.layer, layer ) )
        return;

    PCB_TEXT* text = new PCB_TEXT( parentOf( aFootprint ) );
    text->SetText( aText.text );
    text->SetLayer( layer );
    text->SetPosition( toBoard( aText.x, aText.y ) );

    int height = std::max( 1, toIU( aText.height ) );
    text->SetTextSize( VECTOR2I( height, height ) );
    text->SetTextThickness( std::max( 1, toIU( aText.width ) ) );

    // Direction is 0..3 in 90 degree steps. The Y flip mirrors the rotation, so
    // negate it to keep the text reading the same way it did in Autotrax.
    text->SetTextAngle( EDA_ANGLE( -90.0 * aText.direction, DEGREES_T ) );

    if( layer == B_Cu || layer == B_SilkS )
        text->SetMirrored( true );

    addItem( text, aFootprint );
}


void PCB_IO_AUTOTRAX::buildComponent( const AUTOTRAX::COMPONENT& aComp )
{
    FOOTPRINT* fp = new FOOTPRINT( m_board );
    fp->SetPosition( toBoard( aComp.x, aComp.y ) );

    if( !aComp.refdes.IsEmpty() )
        fp->SetReference( aComp.refdes );

    if( !aComp.value.IsEmpty() )
        fp->SetValue( aComp.value );

    if( !aComp.name.IsEmpty() )
        fp->SetFPID( LIB_ID( wxEmptyString, aComp.name ) );

    for( const TRACK& t : aComp.tracks )
        emitTrack( t, fp );

    for( const ARC& a : aComp.arcs )
        emitArc( a, fp );

    for( const VIA& v : aComp.vias )
        emitVia( v, fp );

    for( const AUTOTRAX::PAD& p : aComp.pads )
        emitPad( p, fp );

    for( const FILL& f : aComp.fills )
        emitFill( f, fp );

    for( const TEXT& s : aComp.texts )
        emitText( s, fp );

    m_board->Add( fp, ADD_MODE::APPEND );
}


void PCB_IO_AUTOTRAX::buildBoard( const BOARD_DATA& aData )
{
    // A single walk over every primitive (free and component-owned) collects two
    // things needed before any item is emitted: the deepest inner copper layer
    // referenced, and the Y extent of the data.
    //
    // mapLayer() places the four inner copper layers on In1..In4 and the GND and
    // Power planes on In5/In6, so the board must enable enough copper layers for
    // the deepest inner layer actually referenced (otherwise items land on a
    // disabled layer). A two-sided board keeps just F_Cu/B_Cu.
    //
    // The Y extent drives the Y-down -> Y-up flip about the data bounding box
    // so all coordinates stay positive.
    int    innerNeeded = 0;
    double maxYmils = 0.0;

    auto noteLayer = [&]( int aLayer )
    {
        switch( aLayer )
        {
        case AUTOTRAX::LAYER_MID1: innerNeeded = std::max( innerNeeded, 1 ); break;
        case AUTOTRAX::LAYER_MID2: innerNeeded = std::max( innerNeeded, 2 ); break;
        case AUTOTRAX::LAYER_MID3: innerNeeded = std::max( innerNeeded, 3 ); break;
        case AUTOTRAX::LAYER_MID4: innerNeeded = std::max( innerNeeded, 4 ); break;
        case AUTOTRAX::LAYER_GND_PLANE: innerNeeded = std::max( innerNeeded, 5 ); break;
        case AUTOTRAX::LAYER_POWER_PLANE: innerNeeded = std::max( innerNeeded, 6 ); break;
        default: break;
        }
    };

    // BOARD_DATA and COMPONENT share the same six primitive members, so one
    // generic walk covers both the free primitives and each component's.
    auto scan = [&]( const auto& aContainer )
    {
        for( const TRACK& t : aContainer.tracks )
        {
            noteLayer( t.layer );
            maxYmils = std::max( { maxYmils, t.y1, t.y2 } );
        }

        for( const ARC& a : aContainer.arcs )
        {
            noteLayer( a.layer );
            maxYmils = std::max( maxYmils, a.centerY + a.radius );
        }

        for( const VIA& v : aContainer.vias )
            maxYmils = std::max( maxYmils, v.y );

        for( const AUTOTRAX::PAD& p : aContainer.pads )
        {
            noteLayer( p.layer );
            maxYmils = std::max( maxYmils, p.y );
        }

        for( const FILL& f : aContainer.fills )
        {
            noteLayer( f.layer );
            maxYmils = std::max( { maxYmils, f.y1, f.y2 } );
        }

        for( const TEXT& s : aContainer.texts )
            maxYmils = std::max( maxYmils, s.y );
    };

    scan( aData );

    for( const AUTOTRAX::COMPONENT& c : aData.components )
        scan( c );

    m_board->SetCopperLayerCount( 2 + innerNeeded );
    m_maxY = toIU( maxYmils );

    // Pre-create nets so pads/tracks can reference them by name.
    for( const NET_NODE& node : aData.netNodes )
        getNet( node.netName );

    for( const TRACK& t : aData.tracks )
        emitTrack( t, nullptr );

    for( const ARC& a : aData.arcs )
        emitArc( a, nullptr );

    for( const VIA& v : aData.vias )
        emitVia( v, nullptr );

    for( const AUTOTRAX::PAD& p : aData.pads )
        emitPad( p, nullptr );

    for( const FILL& f : aData.fills )
        emitFill( f, nullptr );

    for( const TEXT& s : aData.texts )
        emitText( s, nullptr );

    for( const AUTOTRAX::COMPONENT& c : aData.components )
        buildComponent( c );
}
