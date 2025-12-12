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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "export_gencad_writer.h"

#include <build_version.h>
#include <board.h>
#include <board_design_settings.h>
#include <convert_basic_shapes_to_polygon.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <string_utils.h>
#include <macros.h>
#include <hash_eda.h>
#include <fmt.h>


/// Layer names for GenCAD export.
static std::string genCADLayerName( int aCuCount, PCB_LAYER_ID aId )
{
    if( IsCopperLayer( aId ) )
    {
        if( aId == F_Cu )
            return "TOP";
        else if( aId == B_Cu )
            return "BOTTOM";
        else if( aId <= 14 )
            return fmt::format( "INNER{}", aCuCount - aId - 1 );
        else
            return fmt::format( "LAYER{}", static_cast<int>( aId ) );
    }

    else
    {
        const char* txt;

        // using a switch to clearly show mapping & catch out of bounds index.
        switch( aId )
        {
        // Technicals
        case B_Adhes:   txt = "B.Adhes";                break;
        case F_Adhes:   txt = "F.Adhes";                break;
        case B_Paste:   txt = "SOLDERPASTE_BOTTOM";     break;
        case F_Paste:   txt = "SOLDERPASTE_TOP";        break;
        case B_SilkS:   txt = "SILKSCREEN_BOTTOM";      break;
        case F_SilkS:   txt = "SILKSCREEN_TOP";         break;
        case B_Mask:    txt = "SOLDERMASK_BOTTOM";      break;
        case F_Mask:    txt = "SOLDERMASK_TOP";         break;

        // Users
        case Dwgs_User: txt = "Dwgs.User";              break;
        case Cmts_User: txt = "Cmts.User";              break;
        case Eco1_User: txt = "Eco1.User";              break;
        case Eco2_User: txt = "Eco2.User";              break;
        case Edge_Cuts: txt = "Edge.Cuts";              break;
        case Margin:    txt = "Margin";                 break;

        // Footprint
        case F_CrtYd:   txt = "F_CrtYd";                break;
        case B_CrtYd:   txt = "B_CrtYd";                break;
        case F_Fab:     txt = "F_Fab";                  break;
        case B_Fab:     txt = "B_Fab";                  break;

        default:
            wxASSERT_MSG( 0, wxT( "aId UNEXPECTED" ) );
                        txt = "BAD-INDEX!";             break;
        }

        return txt;
    }
}


/// The flipped layer name for GenCAD export (to make CAM350 imports correct).
static std::string genCADLayerNameFlipped( int aCuCount, PCB_LAYER_ID aId )
{
    if( 1<= aId && aId <= 14 )
        return fmt::format(  "INNER{}", 14 - aId );

    return genCADLayerName( aCuCount, aId );
}


static wxString escapeString( const wxString& aString )
{
    wxString copy( aString );
    copy.Replace( wxT( "\"" ), wxT( "\\\"" ) );
    return copy;
}


static std::string fmt_mask( const LSET& aSet )
{
    std::string retv = ( aSet & LSET::AllCuMask() ).to_string();
    retv.erase( 0, retv.find_first_not_of( '0' ) );
    return retv;
}


/// Association between shape names (using shapeName index) and components.
static std::map<FOOTPRINT*, int> componentShapes;
static std::map<int, wxString> shapeNames;


const wxString GENCAD_EXPORTER::getShapeName( FOOTPRINT* aFootprint )
{
    static const wxString invalid( "invalid" );

    if( m_useIndividualShapes )
        return aFootprint->GetReference();

    auto itShape = componentShapes.find( aFootprint );
    wxCHECK( itShape != componentShapes.end(), invalid );

    auto itName = shapeNames.find( itShape->second );
    wxCHECK( itName != shapeNames.end(), invalid );

    return itName->second;
}


// GerbTool chokes on units different than INCH so this is the conversion factor
const static double SCALE_FACTOR = 1000.0 * pcbIUScale.IU_PER_MILS;


double GENCAD_EXPORTER::mapXTo( int aX )
{
    return ( aX - m_gencadOffset.x ) / SCALE_FACTOR;
}


double GENCAD_EXPORTER::mapYTo( int aY )
{
    return ( m_gencadOffset.y - aY ) / SCALE_FACTOR;
}


bool GENCAD_EXPORTER::WriteFile( const wxString& aFullFileName )
{
    componentShapes.clear();
    shapeNames.clear();

    m_file = wxFopen( aFullFileName, wxT( "wt" ) );

    if( !m_file )
        return false;

    BOARD*  pcb = m_board;

    // Update some board data, to ensure a reliable GenCAD export.
    pcb->ComputeBoundingBox( false );

    /* Temporary modification of footprints that are flipped (i.e. on bottom
     * layer) to convert them to non flipped footprints.
     *  This is necessary to easily export shapes to GenCAD,
     *  that are given as normal orientation (non flipped, rotation = 0))
     * these changes will be undone later
     */

    for( FOOTPRINT* footprint : pcb->Footprints() )
    {
        footprint->SetFlag( 0 );

        if( footprint->GetLayer() == B_Cu )
        {
            footprint->Flip( footprint->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );
            footprint->SetFlag( 1 );
        }
    }

    bool success = true;
    try
    {
        /* GenCAD has some mandatory and some optional sections: some importer
         * need the padstack section (which is optional) anyway. Also the
         * order of the section *is* important */

        createHeaderInfoData();         // GenCAD header
        createBoardSection();           // Board perimeter

        createPadsShapesSection();      // Pads and padstacks
        createArtworksSection();        // Empty but mandatory

        /* GenCAD splits a footprint information in shape, component and device.
         * We don't do any sharing (it would be difficult since each module is
         * customizable after placement) */
        createShapesSection();
        createComponentsSection();
        createDevicesSection();

        // In a similar way the netlist is split in net, track and route
        createSignalsSection();
        createTracksInfoData();
        createRoutesSection();
    }
    catch( ... )
    {
        success = false;
    }

    fclose( m_file );

    // Undo the footprints modifications (flipped footprints)
    for( FOOTPRINT* footprint : pcb->Footprints() )
    {
        if( footprint->GetFlag() )
        {
            footprint->Flip( footprint->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );
            footprint->SetFlag( 0 );
        }
    }

    componentShapes.clear();
    shapeNames.clear();

    return success;
}


/// Sort vias for uniqueness.
static bool viaSort( const PCB_VIA* aPadref, const PCB_VIA* aPadcmp )
{
    if( aPadref->GetWidth( PADSTACK::ALL_LAYERS ) != aPadcmp->GetWidth( PADSTACK::ALL_LAYERS ) )
        return aPadref->GetWidth( PADSTACK::ALL_LAYERS ) < aPadcmp->GetWidth( PADSTACK::ALL_LAYERS );

    if( aPadref->GetDrillValue() != aPadcmp->GetDrillValue() )
        return aPadref->GetDrillValue() < aPadcmp->GetDrillValue();

    if( aPadref->GetLayerSet() != aPadcmp->GetLayerSet() )
        return aPadref->GetLayerSet().FmtBin().compare( aPadcmp->GetLayerSet().FmtBin() ) < 0;

    return false;
}


void GENCAD_EXPORTER::createArtworksSection()
{
    // The ARTWORKS section is empty but (officially) mandatory
    fmt::print( m_file, "$ARTWORKS\n" );
    fmt::print( m_file, "$ENDARTWORKS\n\n" );
}


void GENCAD_EXPORTER::createPadsShapesSection()
{
    // Emit PADS and PADSTACKS. They are sorted and emitted uniquely.
    // Via name is synthesized from their attributes, pads are numbered

    std::vector<PAD*>     padstacks;
    std::vector<PCB_VIA*> vias;
    std::vector<PCB_VIA*> viastacks;

    padstacks.resize( 1 ); // We count pads from 1

    LSEQ gc_seq = m_board->GetEnabledLayers().CuStack();
    std::reverse(gc_seq.begin(), gc_seq.end());

    // The master layermask (i.e. the enabled layers) for padstack generation
    LSET    master_layermask = m_board->GetDesignSettings().GetEnabledLayers();
    int     cu_count = m_board->GetCopperLayerCount();

    fmt::print( m_file, "$PADS\n" );

    // Enumerate and sort the pads
    std::vector<PAD*> pads = m_board->GetPads();
    std::sort( pads.begin(), pads.end(), []( const PAD* a, const PAD* b )
                                         {
                                             return PAD::Compare( a, b ) < 0;
                                         } );

    // The same for vias
    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( PCB_VIA* via = dyn_cast<PCB_VIA*>( track ) )
            vias.push_back( via );
    }

    std::sort( vias.begin(), vias.end(), viaSort );
    vias.erase( std::unique( vias.begin(), vias.end(), []( const PCB_VIA* a, const PCB_VIA* b )
                                                       {
                                                           return viaSort( a, b ) == false;
                                                       } ),
                             vias.end() );

    // Emit vias pads
    for( PCB_VIA* via : vias )
    {
        viastacks.push_back( via );
        fmt::print( m_file, "PAD V{}.{}.{} ROUND {}\nCIRCLE 0 0 {}\n",
                    via->GetWidth( PADSTACK::ALL_LAYERS ),
                    via->GetDrillValue(),
                    fmt_mask( via->GetLayerSet() & master_layermask ).c_str(),
                    via->GetDrillValue() / SCALE_FACTOR,
                    via->GetWidth( PADSTACK::ALL_LAYERS ) / (SCALE_FACTOR * 2) );
    }

    // Emit component pads
    PAD* old_pad = nullptr;
    int  pad_name_number = 0;

    for( unsigned i = 0; i<pads.size(); ++i )
    {
        PAD* pad = pads[i];
        const VECTOR2I& off = pad->GetOffset( PADSTACK::ALL_LAYERS );

        pad->SetSubRatsnest( pad_name_number );

        // @warning: This code is not 100% correct.  The #PAD::Compare function does not test
        //           custom pad primitives so there may be duplicate custom pads in the export.
        if( old_pad && 0 == PAD::Compare( old_pad, pad ) )
            continue;

        old_pad = pad;

        pad_name_number++;
        pad->SetSubRatsnest( pad_name_number );

        fmt::print( m_file, "PAD P{}", pad->GetSubRatsnest() );

        padstacks.push_back( pad ); // Will have its own padstack later
        int dx = pad->GetSize( PADSTACK::ALL_LAYERS ).x / 2;
        int dy = pad->GetSize( PADSTACK::ALL_LAYERS ).y / 2;

        switch( pad->GetShape( PADSTACK::ALL_LAYERS ) )
        {
        default:
            UNIMPLEMENTED_FOR( pad->ShowPadShape( PADSTACK::ALL_LAYERS ) );
            KI_FALLTHROUGH;

        case PAD_SHAPE::CIRCLE:
            fmt::print( m_file, " ROUND {}\n",
                        pad->GetDrillSize().x / SCALE_FACTOR );

            /* Circle is center, radius */
            fmt::print( m_file, "CIRCLE {} {} {}\n",
                        off.x / SCALE_FACTOR,
                        -off.y / SCALE_FACTOR,
                        pad->GetSize( PADSTACK::ALL_LAYERS ).x / (SCALE_FACTOR * 2) );
            break;

        case PAD_SHAPE::RECTANGLE:
            fmt::print( m_file, " RECTANGULAR {}\n",
                        pad->GetDrillSize().x / SCALE_FACTOR );

            // Rectangle is begin, size *not* begin, end!
            fmt::print( m_file, "RECTANGLE {} {} {} {}\n",
                        (-dx + off.x ) / SCALE_FACTOR,
                        (-dy - off.y ) / SCALE_FACTOR,
                        dx / (SCALE_FACTOR / 2), dy / (SCALE_FACTOR / 2) );
            break;

        case PAD_SHAPE::ROUNDRECT:
        case PAD_SHAPE::OVAL:
        {
            const VECTOR2I& size = pad->GetSize( PADSTACK::ALL_LAYERS );
            int radius = std::min( size.x, size.y ) / 2;

            if( pad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::ROUNDRECT )
            {
                radius = pad->GetRoundRectCornerRadius( PADSTACK::ALL_LAYERS );
            }

            int lineX = size.x / 2 - radius;
            int lineY = size.y / 2 - radius;

            fmt::print( m_file, " POLYGON {}\n", pad->GetDrillSize().x / SCALE_FACTOR );

            // bottom left arc
            fmt::print( m_file, "ARC {} {} {} {} {} {}\n",
                        ( off.x - lineX - radius ) / SCALE_FACTOR,
                        ( -off.y - lineY ) / SCALE_FACTOR,
                        ( off.x - lineX ) / SCALE_FACTOR,
                        ( -off.y - lineY - radius ) / SCALE_FACTOR,
                        ( off.x - lineX ) / SCALE_FACTOR,
                        ( -off.y - lineY ) / SCALE_FACTOR );

            // bottom line
            if( lineX > 0 )
            {
                fmt::print( m_file, "LINE {} {} {} {}\n",
                            ( off.x - lineX ) / SCALE_FACTOR,
                            ( -off.y - lineY - radius ) / SCALE_FACTOR,
                            ( off.x + lineX ) / SCALE_FACTOR,
                            ( -off.y - lineY - radius ) / SCALE_FACTOR );
            }

            // bottom right arc
            fmt::print( m_file, "ARC {} {} {} {} {} {}\n",
                        ( off.x + lineX ) / SCALE_FACTOR,
                        ( -off.y - lineY - radius ) / SCALE_FACTOR,
                        ( off.x + lineX + radius ) / SCALE_FACTOR,
                        ( -off.y - lineY ) / SCALE_FACTOR,
                        ( off.x + lineX ) / SCALE_FACTOR,
                        ( -off.y - lineY ) / SCALE_FACTOR );

            // right line
            if( lineY > 0 )
            {
                fmt::print( m_file, "LINE {} {} {} {}\n",
                            ( off.x + lineX + radius ) / SCALE_FACTOR,
                            ( -off.y + lineY ) / SCALE_FACTOR,
                            ( off.x + lineX + radius ) / SCALE_FACTOR,
                            ( -off.y - lineY ) / SCALE_FACTOR );
            }

            // top right arc
            fmt::print( m_file, "ARC {} {} {} {} {} {}\n",
                        ( off.x + lineX + radius ) / SCALE_FACTOR,
                        ( -off.y + lineY ) / SCALE_FACTOR,
                        ( off.x + lineX ) / SCALE_FACTOR,
                        ( -off.y + lineY + radius ) / SCALE_FACTOR,
                        ( off.x + lineX ) / SCALE_FACTOR,
                        ( -off.y + lineY ) / SCALE_FACTOR );

            // top line
            if( lineX > 0 )
            {
                fmt::print( m_file, "LINE {} {} {} {}\n",
                            ( off.x - lineX ) / SCALE_FACTOR,
                            ( -off.y + lineY + radius ) / SCALE_FACTOR,
                            ( off.x + lineX ) / SCALE_FACTOR,
                            ( -off.y + lineY + radius ) / SCALE_FACTOR );
            }

            // top left arc
            fmt::print( m_file, "ARC {} {} {} {} {} {}\n",
                        ( off.x - lineX ) / SCALE_FACTOR,
                        ( -off.y + lineY + radius ) / SCALE_FACTOR,
                        ( off.x - lineX - radius ) / SCALE_FACTOR,
                        ( -off.y + lineY ) / SCALE_FACTOR,
                        ( off.x - lineX ) / SCALE_FACTOR,
                        ( -off.y + lineY ) / SCALE_FACTOR );

            // left line
            if( lineY > 0 )
            {
                fmt::print( m_file, "LINE {} {} {} {}\n",
                            ( off.x - lineX - radius ) / SCALE_FACTOR,
                            ( -off.y - lineY ) / SCALE_FACTOR,
                            ( off.x - lineX - radius ) / SCALE_FACTOR,
                            ( -off.y + lineY ) / SCALE_FACTOR );
            }

            break;
        }

        case PAD_SHAPE::TRAPEZOID:
        {
            fmt::print( m_file, " POLYGON {}\n", pad->GetDrillSize().x / SCALE_FACTOR );

            int  ddx = pad->GetDelta( PADSTACK::ALL_LAYERS ).x / 2;
            int  ddy = pad->GetDelta( PADSTACK::ALL_LAYERS ).y / 2;

            VECTOR2I poly[4];
            poly[0] = VECTOR2I( -dx + ddy, dy + ddx );
            poly[1] = VECTOR2I( dx - ddy, dy - ddx );
            poly[2] = VECTOR2I( dx + ddy, -dy + ddx );
            poly[3] = VECTOR2I( -dx - ddy, -dy - ddx );

            for( int cur = 0; cur < 4; ++cur )
            {
                int next = ( cur + 1 ) % 4;
                fmt::print( m_file, "LINE {} {} {} {}\n",
                            ( off.x + poly[cur].x ) / SCALE_FACTOR,
                            ( -off.y - poly[cur].y ) / SCALE_FACTOR,
                            ( off.x + poly[next].x ) / SCALE_FACTOR,
                            ( -off.y - poly[next].y ) / SCALE_FACTOR );
            }

            break;
        }

        case PAD_SHAPE::CHAMFERED_RECT:
        {
            fmt::print( m_file, " POLYGON {}\n", pad->GetDrillSize().x / SCALE_FACTOR );

            SHAPE_POLY_SET outline;
            VECTOR2I       padOffset( 0, 0 );

            TransformRoundChamferedRectToPolygon( outline, padOffset,
                                                  pad->GetSize( PADSTACK::ALL_LAYERS ),
                                                  pad->GetOrientation(),
                                                  pad->GetRoundRectCornerRadius( PADSTACK::ALL_LAYERS ),
                                                  pad->GetChamferRectRatio( PADSTACK::ALL_LAYERS ),
                                                  pad->GetChamferPositions( PADSTACK::ALL_LAYERS ),
                                                  0, pad->GetMaxError(), ERROR_INSIDE );

            for( int jj = 0; jj < outline.OutlineCount(); ++jj )
            {
                const SHAPE_LINE_CHAIN& poly = outline.COutline( jj );
                int pointCount = poly.PointCount();

                for( int ii = 0; ii < pointCount; ii++ )
                {
                    int next = ( ii + 1 ) % pointCount;
                    fmt::print( m_file, "LINE {} {} {} {}\n",
                                poly.CPoint( ii ).x / SCALE_FACTOR,
                                -poly.CPoint( ii ).y / SCALE_FACTOR,
                                poly.CPoint( next ).x / SCALE_FACTOR,
                                -poly.CPoint( next ).y / SCALE_FACTOR );
                }
            }

            break;
        }

        case PAD_SHAPE::CUSTOM:
        {
            fmt::print( m_file, " POLYGON {}\n", pad->GetDrillSize().x / SCALE_FACTOR );

            SHAPE_POLY_SET outline;
            pad->MergePrimitivesAsPolygon( F_Cu, &outline );

            for( int jj = 0; jj < outline.OutlineCount(); ++jj )
            {
                const SHAPE_LINE_CHAIN& poly = outline.COutline( jj );
                int pointCount = poly.PointCount();

                for( int ii = 0; ii < pointCount; ii++ )
                {
                    int next = ( ii + 1 ) % pointCount;
                    fmt::print( m_file, "LINE {} {} {} {}\n",
                                ( off.x + poly.CPoint( ii ).x ) / SCALE_FACTOR,
                                ( -off.y - poly.CPoint( ii ).y ) / SCALE_FACTOR,
                                ( off.x + poly.CPoint( next ).x ) / SCALE_FACTOR,
                                ( -off.y - poly.CPoint( next ).y ) / SCALE_FACTOR );
                }
            }

            break;
        }
        }
    }

    fmt::print( m_file, "\n$ENDPADS\n\n" );

    // Now emit the padstacks definitions, using the combined layer masks
    fmt::print( m_file, "$PADSTACKS\n" );

    // Via padstacks
    for( unsigned i = 0; i < viastacks.size(); i++ )
    {
        PCB_VIA* via = viastacks[i];

        LSET mask = via->GetLayerSet() & master_layermask;

        fmt::print( m_file, "PADSTACK VIA{}.{}.{} {}\n",
                    via->GetWidth( PADSTACK::ALL_LAYERS ),
                    via->GetDrillValue(),
                    fmt_mask( mask ).c_str(),
                    via->GetDrillValue() / SCALE_FACTOR );

        for( PCB_LAYER_ID layer : mask.Seq( gc_seq ) )
        {
            fmt::print( m_file, "PAD V{}.{}.{} {} 0 0\n",
                        via->GetWidth( PADSTACK::ALL_LAYERS ),
                        via->GetDrillValue(),
                        fmt_mask( mask ).c_str(),
                        genCADLayerName( cu_count, layer ).c_str() );
        }
    }

    /* Component padstacks
     *  Older versions of CAM350 don't apply correctly the FLIP semantics for
     *  padstacks, i.e. doesn't swap the top and bottom layers... so I need to
     *  define the shape as MIRRORX and define a separate 'flipped' padstack...
     *  until it appears yet another non-compliant importer */
    for( unsigned i = 1; i < padstacks.size(); i++ )
    {
        PAD* pad = padstacks[i];

        // Straight padstack
        fmt::print( m_file, "PADSTACK PAD{} {}\n",
                    i,
                    pad->GetDrillSize().x / SCALE_FACTOR );

        LSET pad_set = pad->GetLayerSet() & master_layermask;

        // the special gc_seq
        for( PCB_LAYER_ID layer : pad_set.Seq( gc_seq ) )
        {
            fmt::print( m_file, "PAD P{} {} 0 0\n",
                        i,
                        genCADLayerName( cu_count, layer ).c_str() );
        }

        // Flipped padstack
        if( m_flipBottomPads )
        {
            fmt::print( m_file, "PADSTACK PAD{}F {}\n",
                        i,
                        pad->GetDrillSize().x / SCALE_FACTOR );

            // the normal PCB_LAYER_ID sequence is inverted from gc_seq[]
            for( PCB_LAYER_ID layer : pad_set.Seq() )
            {
                fmt::print( m_file, "PAD P{} {} 0 0\n",
                            i,
                            genCADLayerNameFlipped( cu_count, layer ).c_str() );
            }
        }
    }

    fputs( "$ENDPADSTACKS\n\n", m_file );
}


/// Compute hashes for footprints without taking into account their position, rotation or layer.
static size_t hashFootprint( const FOOTPRINT* aFootprint )
{
    size_t    ret = 0x11223344;
    constexpr int flags = HASH_FLAGS::HASH_POS | HASH_FLAGS::REL_COORD
                            | HASH_FLAGS::HASH_ROT | HASH_FLAGS::HASH_LAYER;

    for( BOARD_ITEM* i : aFootprint->GraphicalItems() )
        ret += hash_fp_item( i, flags );

    for( PAD* i : aFootprint->Pads() )
        ret += hash_fp_item( i, flags );

    return ret;
}


void GENCAD_EXPORTER::createShapesSection()
{
    const char* layer;
    wxString    pinname;
    const char* mirror = "0";
    std::map<wxString, size_t> shapes;

    fmt::print( m_file, "$SHAPES\n" );

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        if( !m_useIndividualShapes )
        {
            // Check if such shape has been already generated, and if so - reuse it
            // It is necessary to compute hash (i.e. check all children objects) as
            // certain components instances might have been modified on the board.
            // In such case the shape will be different despite the same LIB_ID.
            wxString shapeName = footprint->GetFPID().Format();

            auto shapeIt = shapes.find( shapeName );
            size_t modHash = hashFootprint( footprint );

            if( shapeIt != shapes.end() )
            {
                if( modHash != shapeIt->second )
                {
                    // there is an entry for this footprint, but it has a modified shape,
                    // so we need to create a new entry
                    wxString newShapeName;
                    int suffix = 0;

                    // find an unused name or matching entry
                    do
                    {
                        newShapeName = wxString::Format( wxT( "%s_%d" ), shapeName, suffix );
                        shapeIt = shapes.find( newShapeName );
                        ++suffix;
                    }
                    while( shapeIt != shapes.end() && shapeIt->second != modHash );

                    shapeName = newShapeName;
                }

                if( shapeIt != shapes.end() && modHash == shapeIt->second )
                {
                    // shape found, so reuse it
                    componentShapes[footprint] = modHash;
                    continue;
                }
            }

            // new shape
            componentShapes[footprint] = modHash;
            shapeNames[modHash] = shapeName;
            shapes[shapeName] = modHash;
            footprintWriteShape( footprint, shapeName );
        }
        else // individual shape for each component
        {
            footprintWriteShape( footprint, footprint->GetReference() );
        }

        // set of already emitted pins to check for duplicates
        std::set<wxString> pins;

        for( PAD* pad : footprint->Pads() )
        {
            /* Padstacks are defined using the correct layers for the pads, therefore to
             * all pads need to be marked as TOP to use the padstack information correctly.
             */
            layer = "TOP";
            pinname = pad->GetNumber();

            if( pinname.IsEmpty() )
                pinname = wxT( "none" );

            if( m_useUniquePins )
            {
                int suffix = 0;
                wxString origPinname( pinname );

                auto it = pins.find( pinname );

                while( it != pins.end() )
                {
                    pinname = wxString::Format( wxT( "%s_%d" ), origPinname, suffix );
                    ++suffix;
                    it = pins.find( pinname );
                }

                pins.insert( pinname );
            }

            EDA_ANGLE orient = pad->GetOrientation() - footprint->GetOrientation();
            orient.Normalize();

            VECTOR2I padPos = pad->GetFPRelativePosition();

            std::string flipStr = ( m_flipBottomPads && footprint->GetFlag() ) ? "F" : "";

            // Bottom side footprints use the flipped padstack
            fmt::print( m_file,
                        "PIN \"{}\" PAD{}{} {} {} {} {} {}\n",
                        TO_UTF8( escapeString( pinname ) ),
                        pad->GetSubRatsnest(),
                        flipStr,
                        padPos.x / SCALE_FACTOR,
                        -padPos.y / SCALE_FACTOR,
                        layer,
                        orient.AsDegrees(),
                        mirror );
        }
    }

    fmt::print( m_file, "$ENDSHAPES\n\n" );
}


void GENCAD_EXPORTER::createComponentsSection()
{
    fmt::print( m_file, "$COMPONENTS\n" );

    int cu_count = m_board->GetCopperLayerCount();

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        const char*   mirror;
        const char*   flip;
        EDA_ANGLE     fp_orient = footprint->GetOrientation();

        if( footprint->GetFlag() )
        {
            mirror = "MIRRORX";
            flip   = "FLIP";
            fp_orient = fp_orient.Invert().Normalize();
        }
        else
        {
            mirror = "0";
            flip   = "0";
        }

        fmt::print( m_file, "\nCOMPONENT \"{}\"\n",
                    TO_UTF8( escapeString( footprint->GetReference() ) ) );
        fmt::print( m_file, "DEVICE \"DEV_{}\"\n",
                    TO_UTF8( escapeString( getShapeName( footprint ) ) ) );
        fmt::print( m_file, "PLACE {} {}\n",
                    mapXTo( footprint->GetPosition().x ),
                    mapYTo( footprint->GetPosition().y ) );
        fmt::print( m_file, "LAYER {}\n",
                    footprint->GetFlag() ? "BOTTOM" : "TOP" );
        fmt::print( m_file, "ROTATION {}\n",
                    fp_orient.AsDegrees() );
        fmt::print( m_file, "SHAPE \"{}\" {} {}\n",
                    TO_UTF8( escapeString( getShapeName( footprint ) ) ),
                    mirror, flip );

        // Text on silk layer: RefDes and value (are they actually useful?)
        for( PCB_TEXT* textItem : { &footprint->Reference(), &footprint->Value() } )
        {
            std::string layer = genCADLayerName( cu_count, footprint->GetFlag() ? B_SilkS : F_SilkS );

            fmt::print( m_file, "TEXT {} {} {} {} {} {} \"{}\"",
                        textItem->GetFPRelativePosition().x / SCALE_FACTOR,
                        -textItem->GetFPRelativePosition().y / SCALE_FACTOR,
                        textItem->GetTextWidth() / SCALE_FACTOR,
                        textItem->GetTextAngle().AsDegrees(),
                        mirror,
                        layer.c_str(),
                        TO_UTF8( escapeString( textItem->GetText() ) ) );

            BOX2I textBox = textItem->GetTextBox( nullptr );

            fmt::print( m_file, " 0 0 {} {}\n",
                        textBox.GetWidth() / SCALE_FACTOR,
                        textBox.GetHeight() / SCALE_FACTOR );
        }

        // The SHEET is a 'generic description' for referencing the component
        fmt::print( m_file, "SHEET \"RefDes: {}, Value: {}\"\n",
                    TO_UTF8( footprint->GetReference() ),
                    TO_UTF8( footprint->GetValue() ) );
    }

    fmt::print( m_file, "$ENDCOMPONENTS\n\n" );
}


void GENCAD_EXPORTER::createSignalsSection()
{
    // Emit the netlist (which is actually the thing for which GenCAD is used these
    // days!); tracks are handled later

    wxString      msg;
    NETINFO_ITEM* net;
    int           NbNoConn = 1;

    fmt::print( m_file, "$SIGNALS\n" );

    for( unsigned ii = 0; ii < m_board->GetNetCount(); ii++ )
    {
        net = m_board->FindNet( ii );

        if( net )
        {
            if( net->GetNetname() == wxEmptyString ) // dummy netlist (no connection)
            {
                msg.Printf( wxT( "NoConnection%d" ), NbNoConn++ );
            }

            if( net->GetNetCode() <= 0 )  // dummy netlist (no connection)
                continue;

            msg = wxT( "SIGNAL \"" ) + escapeString( net->GetNetname() ) + wxT( "\"" );

            fmt::print( m_file, "{}", TO_UTF8( msg ) );
            fmt::print( m_file, "\n" );

            for( FOOTPRINT* footprint : m_board->Footprints() )
            {
                for( PAD* pad : footprint->Pads() )
                {
                    if( pad->GetNetCode() != net->GetNetCode() )
                        continue;

                    msg.Printf( wxT( "NODE \"%s\" \"%s\"" ),
                                escapeString( footprint->GetReference() ),
                                escapeString( pad->GetNumber() ) );

                    fmt::print( m_file, "{}", TO_UTF8( msg ) );
                    fmt::print( m_file, "\n" );
                }
            }
        }
    }

    fmt::print( m_file, "$ENDSIGNALS\n\n" );
}


bool GENCAD_EXPORTER::createHeaderInfoData()
{
    fmt::print( m_file, "$HEADER\n" );
    fmt::print( m_file, "GENCAD 1.4\n" );

    // Please note: GenCAD syntax requires quoted strings if they can contain spaces
    fmt::print( m_file, "USER \"KiCad {}\"\n", GetBuildVersion() );

    fmt::print( m_file, "DRAWING \"{}\"\n", m_board->GetFileName() );

    wxString rev = ExpandTextVars( m_board->GetTitleBlock().GetRevision(), m_board->GetProject() );
    wxString date = ExpandTextVars( m_board->GetTitleBlock().GetDate(), m_board->GetProject() );

    fmt::print( m_file, "REVISION \"{} {}\"\n", rev, date );
    fmt::print( m_file, "UNITS INCH\n" );

    // giving 0 as the argument to Map{X,Y}To returns the scaled origin point
    fmt::print( m_file, "ORIGIN {} {}\n", m_storeOriginCoords ? mapXTo( 0 ) : 0,
                                          m_storeOriginCoords ? mapYTo( 0 ) : 0 );

    fmt::print( m_file, "INTERTRACK 0\n" );
    fmt::print( m_file, "$ENDHEADER\n\n" );

    return true;
}


void GENCAD_EXPORTER::createRoutesSection()
{
    int     vianum = 1;
    int     old_netcode, old_width, old_layer;
    LSET    master_layermask = m_board->GetDesignSettings().GetEnabledLayers();
    int     cu_count = m_board->GetCopperLayerCount();
    TRACKS  tracks( m_board->Tracks() );

    std::sort( tracks.begin(), tracks.end(),
               []( const PCB_TRACK* a, const PCB_TRACK* b )
               {
                   int widthA = 0;
                   int widthB = 0;

                   if( a->Type() == PCB_VIA_T )
                       widthA = static_cast<const PCB_VIA*>( a )->GetWidth( PADSTACK::ALL_LAYERS );
                   else
                       widthA = a->GetWidth();

                   if( b->Type() == PCB_VIA_T )
                       widthB = static_cast<const PCB_VIA*>( b )->GetWidth( PADSTACK::ALL_LAYERS );
                   else
                       widthB = b->GetWidth();

                   if( a->GetNetCode() == b->GetNetCode() )
                   {
                       if( widthA == widthB )
                           return ( a->GetLayer() < b->GetLayer() );

                       return ( widthA < widthB );
                   }

                   return ( a->GetNetCode() < b->GetNetCode() );
               } );

    fmt::print( m_file, "$ROUTES\n" );

    old_netcode = -1;
    old_width = -1;
    old_layer = -1;

    for( PCB_TRACK* track : tracks )
    {
        if( old_netcode != track->GetNetCode() )
        {
            old_netcode = track->GetNetCode();
            NETINFO_ITEM* net = track->GetNet();
            wxString      netname;

            if( net && (net->GetNetname() != wxEmptyString) )
                netname = net->GetNetname();
            else
                netname = wxT( "_noname_" );

            fmt::print( m_file, "ROUTE \"{}\"\n", TO_UTF8( escapeString( netname ) ) );
        }

        int currentWidth = 0;

        if( track->Type() == PCB_VIA_T )
            currentWidth = static_cast<const PCB_VIA*>( track )->GetWidth( PADSTACK::ALL_LAYERS );
        else
            currentWidth = track->GetWidth();

        if( old_width != currentWidth )
        {
            old_width = currentWidth;
            fmt::print( m_file, "TRACK TRACK{}\n", currentWidth );
        }

        if( track->Type() == PCB_TRACE_T )
        {
            if( old_layer != track->GetLayer() )
            {
                old_layer = track->GetLayer();
                fmt::print( m_file, "LAYER {}\n",
                            genCADLayerName( cu_count, track->GetLayer() ).c_str() );
            }

            fmt::print( m_file, "LINE {} {} {} {}\n",
                        mapXTo( track->GetStart().x ), mapYTo( track->GetStart().y ),
                        mapXTo( track->GetEnd().x ), mapYTo( track->GetEnd().y ) );
        }
        else if( track->Type() == PCB_ARC_T )
        {
            if( old_layer != track->GetLayer() )
            {
                old_layer = track->GetLayer();
                fmt::print( m_file, "LAYER {}\n",
                            genCADLayerName( cu_count, track->GetLayer() ).c_str() );
            }

            VECTOR2I start = track->GetStart();
            VECTOR2I end = track->GetEnd();

            const PCB_ARC* arc = static_cast<const PCB_ARC*>( track );

            // GenCAD arcs are always drawn counter-clockwise (IsCCW works backwards because Y-axis is up in GenCAD).
            if( arc->IsCCW() )
                std::swap( start, end );

            VECTOR2I center = arc->GetCenter();

            fmt::print( m_file, "ARC {} {} {} {} {} {}\n",
                        mapXTo( start.x ), mapYTo( start.y ),
                        mapXTo( end.x ), mapYTo( end.y ),
                        mapXTo( center.x ), mapYTo( center.y ) );
        }
        else if( track->Type() == PCB_VIA_T )
        {
            const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

            LSET vset = via->GetLayerSet() & master_layermask;

            fmt::print( m_file, "VIA VIA{}.{}.{} {} {} ALL {} via{}\n",
                        via->GetWidth( PADSTACK::ALL_LAYERS ),
                        via->GetDrillValue(),
                        fmt_mask( vset ).c_str(),
                        mapXTo( via->GetStart().x ), mapYTo( via->GetStart().y ),
                        via->GetDrillValue() / SCALE_FACTOR,
                        vianum++ );
        }
    }

    fmt::print( m_file, "$ENDROUTES\n\n" );
}


void GENCAD_EXPORTER::createDevicesSection()
{
    std::set<wxString> emitted;

    fmt::print( m_file, "$DEVICES\n" );

    // componentShapes (as a std::map<>) does not give the same order for items between 2 runs.
    // This is annoying when one want to compare 2 similar files.
    // Therefore we store the strings in a wxArrayString, and after created, strings will be sorted.
    // This is not perfect, because the selected footprint used to create the DEVICE section is
    // not always the same between runs, but this is much better than no sort
    wxArrayString data;

    for( const auto& componentShape : componentShapes )
    {
        const wxString& shapeName = shapeNames[componentShape.second];
        bool newDevice;
        std::tie( std::ignore, newDevice ) = emitted.insert( shapeName );

        if( !newDevice )        // do not repeat device definitions
            continue;

        const FOOTPRINT* footprint = componentShape.first;

        wxString txt;
        txt.Printf( "\nDEVICE \"DEV_%s\"\n", escapeString( shapeName ) );
        txt += wxString::Format( "PART \"%s\"\n", escapeString( footprint->GetValue() ) );
        txt += wxString::Format( "PACKAGE \"%s\"\n", escapeString( footprint->GetFPID().Format() ) );

        data.Add( txt );
    }

    data.Sort();

    for( wxString& item : data )
        fmt::print( m_file, "{}", TO_UTF8( item ) );

    fmt::print( m_file, "$ENDDEVICES\n\n" );
}


void GENCAD_EXPORTER::createBoardSection()
{
    // Creates the section $BOARD.
    //  We output here only the board perimeter
    fmt::print( m_file, "$BOARD\n" );

    // Extract the board edges
    SHAPE_POLY_SET outline;

    if( !m_board->GetBoardPolygonOutlines( outline, true ) )
        wxLogError( _( "Board outline is malformed. Run DRC for a full analysis." ) );

    for( auto seg1 = outline.IterateSegmentsWithHoles(); seg1; seg1++ )
    {
        SEG seg = *seg1;
        fmt::print( m_file, "LINE {} {} {} {}\n",
                    mapXTo( seg.A.x ), mapYTo( seg.A.y ),
                    mapXTo( seg.B.x ), mapYTo( seg.B.y ) );
    }

    fmt::print( m_file, "$ENDBOARD\n\n" );
}


void GENCAD_EXPORTER::createTracksInfoData()
{
    // Find thickness used for traces
    std::set<int> trackinfo;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            continue;

        trackinfo.insert( track->GetWidth() );
    }

    // Write data
    fmt::print( m_file, "$TRACKS\n" );

    for( int size : trackinfo )
        fmt::print( m_file, "TRACK TRACK{} {}\n", size, size / SCALE_FACTOR );

    fmt::print( m_file, "$ENDTRACKS\n\n" );
}


void GENCAD_EXPORTER::footprintWriteShape( FOOTPRINT* aFootprint, const wxString& aShapeName )
{
    /* creates header: */
    fmt::print( m_file, "\nSHAPE \"{}\"\n", TO_UTF8( escapeString( aShapeName ) ) );

    if( aFootprint->GetAttributes() & FP_THROUGH_HOLE )
        fmt::print( m_file, "INSERT TH\n" );
    else
        fmt::print( m_file, "INSERT SMD\n" );

    // Silk outline; wildly interpreted by various importers:
    // CAM350 read it right but only closed shapes
    // ProntoPlace double-flip it (at least the pads are correct)
    // GerberTool usually get it right...
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->Type() == PCB_SHAPE_T && ( item->GetLayer() == F_SilkS || item->GetLayer() == B_SilkS ) )
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );
            VECTOR2I   start = shape->GetStart() - aFootprint->GetPosition();
            VECTOR2I   end = shape->GetEnd() - aFootprint->GetPosition();
            VECTOR2I   center = shape->GetCenter() - aFootprint->GetPosition();

            RotatePoint( start, -aFootprint->GetOrientation() );
            RotatePoint( end, -aFootprint->GetOrientation() );
            RotatePoint( center, -aFootprint->GetOrientation() );

            switch( shape->GetShape() )
            {
            case SHAPE_T::SEGMENT:
                fmt::print( m_file, "LINE {} {} {} {}\n",
                         start.x / SCALE_FACTOR,
                         -start.y / SCALE_FACTOR,
                         end.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR );
                break;

            case SHAPE_T::RECTANGLE:
                fmt::print( m_file, "LINE {} {} {} {}\n",
                         start.x / SCALE_FACTOR,
                         -start.y / SCALE_FACTOR,
                         end.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR );
                fmt::print( m_file, "LINE {} {} {} {}\n",
                         end.x / SCALE_FACTOR,
                         -start.y / SCALE_FACTOR,
                         end.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR );
                fmt::print( m_file, "LINE {} {} {} {}\n",
                         end.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR,
                         start.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR );
                fmt::print( m_file, "LINE {} {} {} {}\n",
                         start.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR,
                         start.x / SCALE_FACTOR,
                         -start.y / SCALE_FACTOR );
                break;

            case SHAPE_T::CIRCLE:
            {
                int radius = KiROUND( end.Distance( start ) );

                fmt::print( m_file, "CIRCLE {} {} {}\n",
                            start.x / SCALE_FACTOR,
                            -start.y / SCALE_FACTOR,
                            radius / SCALE_FACTOR );
                break;
            }

            case SHAPE_T::ARC:
                if( shape->GetArcAngle() > ANGLE_0 )
                    std::swap( start, end );

                fmt::print( m_file, "ARC {} {} {} {} {} {}\n",
                            start.x / SCALE_FACTOR,
                            -start.y / SCALE_FACTOR,
                            end.x / SCALE_FACTOR,
                            -end.y / SCALE_FACTOR,
                            center.x / SCALE_FACTOR,
                            -center.y / SCALE_FACTOR );
                break;

            case SHAPE_T::POLY:
                // Not exported (TODO)
                break;

            default:
                wxFAIL_MSG( wxString::Format( wxT( "Shape type %d invalid." ), item->Type() ) );
                break;
            }
        }
    }
}
