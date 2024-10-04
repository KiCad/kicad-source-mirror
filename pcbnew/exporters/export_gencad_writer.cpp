/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <build_version.h>
#include <board.h>
#include <board_design_settings.h>
#include <convert_basic_shapes_to_polygon.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <richio.h>
#include <locale_io.h>
#include <macros.h>
#include <hash_eda.h>

#include <export_gencad_writer.h>


// layer names for Gencad export
static std::string GenCADLayerName( int aCuCount, PCB_LAYER_ID aId )
{
    if( IsCopperLayer( aId ) )
    {
        if( aId == F_Cu )
            return "TOP";
        else if( aId == B_Cu )
            return "BOTTOM";
        else if( aId <= 14 )
            return StrPrintf( "INNER%d", aCuCount - aId - 1 );
        else
            return StrPrintf( "LAYER%d", aId );
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


// flipped layer name for Gencad export (to make CAM350 imports correct)
static std::string GenCADLayerNameFlipped( int aCuCount, PCB_LAYER_ID aId )
{
    if( 1<= aId && aId <= 14 )
        return StrPrintf(  "INNER%d", 14 - aId );

    return GenCADLayerName( aCuCount, aId );
}


static wxString escapeString( const wxString& aString )
{
    wxString copy( aString );
    copy.Replace( wxT( "\"" ), wxT( "\\\"" ) );
    return copy;
}


static std::string fmt_mask( LSET aSet )
{
    return ( aSet & LSET::AllCuMask() ).to_string();
}


// Association between shape names (using shapeName index) and components
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


/* Two helper functions to calculate coordinates of footprints in gencad values
 * (GenCAD Y axis from bottom to top)
 */
double GENCAD_EXPORTER::MapXTo( int aX )
{
    return (aX - GencadOffset.x) / SCALE_FACTOR;
}


double GENCAD_EXPORTER::MapYTo( int aY )
{
    return (GencadOffset.y - aY) / SCALE_FACTOR;
}


bool GENCAD_EXPORTER::WriteFile( const wxString& aFullFileName )
{
    componentShapes.clear();
    shapeNames.clear();

    m_file = wxFopen( aFullFileName, wxT( "wt" ) );

    if( !m_file )
        return false;

    // Switch the locale to standard C (needed to print floating point numbers)
    LOCALE_IO toggle;

    BOARD*  pcb = m_board;
    // Update some board data, to ensure a reliable gencad export
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

    /* Gencad has some mandatory and some optional sections: some importer
     *  need the padstack section (which is optional) anyway. Also the
     *  order of the section *is* important */

    CreateHeaderInfoData();         // Gencad header
    CreateBoardSection();           // Board perimeter

    CreatePadsShapesSection();      // Pads and padstacks
    CreateArtworksSection();        // Empty but mandatory

    /* Gencad splits a component info in shape, component and device.
     *  We don't do any sharing (it would be difficult since each module is
     *  customizable after placement) */
    CreateShapesSection();
    CreateComponentsSection();
    CreateDevicesSection();

    // In a similar way the netlist is split in net, track and route
    CreateSignalsSection();
    CreateTracksInfoData();
    CreateRoutesSection();

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

    return true;
}


// Sort vias for uniqueness
static bool ViaSort( const PCB_VIA* aPadref, const PCB_VIA* aPadcmp )
{
    if( aPadref->GetWidth() != aPadcmp->GetWidth() )
        return aPadref->GetWidth() < aPadcmp->GetWidth();

    if( aPadref->GetDrillValue() != aPadcmp->GetDrillValue() )
        return aPadref->GetDrillValue() < aPadcmp->GetDrillValue();

    if( aPadref->GetLayerSet() != aPadcmp->GetLayerSet() )
        return aPadref->GetLayerSet().FmtBin().compare( aPadcmp->GetLayerSet().FmtBin() ) < 0;

    return false;
}


void GENCAD_EXPORTER::CreateArtworksSection( )
{
    // The ARTWORKS section is empty but (officially) mandatory
    fputs( "$ARTWORKS\n", m_file );
    fputs( "$ENDARTWORKS\n\n", m_file );
}


void GENCAD_EXPORTER::CreatePadsShapesSection()
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

    fputs( "$PADS\n", m_file );

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

    std::sort( vias.begin(), vias.end(), ViaSort );
    vias.erase( std::unique( vias.begin(), vias.end(), []( const PCB_VIA* a, const PCB_VIA* b )
                                                       {
                                                           return ViaSort( a, b ) == false;
                                                       } ),
            vias.end() );

    // Emit vias pads
    for( PCB_VIA* via : vias )
    {
        viastacks.push_back( via );
        fprintf( m_file, "PAD V%d.%d.%s ROUND %g\nCIRCLE 0 0 %g\n",
                 via->GetWidth(), via->GetDrillValue(),
                 fmt_mask( via->GetLayerSet() & master_layermask ).c_str(),
                 via->GetDrillValue() / SCALE_FACTOR,
                 via->GetWidth() / (SCALE_FACTOR * 2) );
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

        fprintf( m_file, "PAD P%d", pad->GetSubRatsnest() );

        padstacks.push_back( pad ); // Will have its own padstack later
        int dx = pad->GetSize( PADSTACK::ALL_LAYERS ).x / 2;
        int dy = pad->GetSize( PADSTACK::ALL_LAYERS ).y / 2;

        switch( pad->GetShape( PADSTACK::ALL_LAYERS ) )
        {
        default:
            UNIMPLEMENTED_FOR( pad->ShowPadShape( PADSTACK::ALL_LAYERS ) );
            KI_FALLTHROUGH;

        case PAD_SHAPE::CIRCLE:
            fprintf( m_file, " ROUND %g\n",
                     pad->GetDrillSize().x / SCALE_FACTOR );

            /* Circle is center, radius */
            fprintf( m_file, "CIRCLE %g %g %g\n",
                     off.x / SCALE_FACTOR,
                     -off.y / SCALE_FACTOR,
                     pad->GetSize( PADSTACK::ALL_LAYERS ).x / (SCALE_FACTOR * 2) );
            break;

        case PAD_SHAPE::RECTANGLE:
            fprintf( m_file, " RECTANGULAR %g\n",
                     pad->GetDrillSize().x / SCALE_FACTOR );

            // Rectangle is begin, size *not* begin, end!
            fprintf( m_file, "RECTANGLE %g %g %g %g\n",
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

            fprintf( m_file, " POLYGON %g\n", pad->GetDrillSize().x / SCALE_FACTOR );

            // bottom left arc
            fprintf( m_file, "ARC %g %g %g %g %g %g\n",
                     ( off.x - lineX - radius ) / SCALE_FACTOR,
                     ( -off.y - lineY ) / SCALE_FACTOR, ( off.x - lineX ) / SCALE_FACTOR,
                     ( -off.y - lineY - radius ) / SCALE_FACTOR,
                     ( off.x - lineX ) / SCALE_FACTOR, ( -off.y - lineY ) / SCALE_FACTOR );

            // bottom line
            if( lineX > 0 )
            {
                fprintf( m_file, "LINE %g %g %g %g\n",
                         ( off.x - lineX ) / SCALE_FACTOR,
                         ( -off.y - lineY - radius ) / SCALE_FACTOR,
                         ( off.x + lineX ) / SCALE_FACTOR,
                         ( -off.y - lineY - radius ) / SCALE_FACTOR );
            }

            // bottom right arc
            fprintf( m_file, "ARC %g %g %g %g %g %g\n",
                     ( off.x + lineX ) / SCALE_FACTOR,
                     ( -off.y - lineY - radius ) / SCALE_FACTOR,
                     ( off.x + lineX + radius ) / SCALE_FACTOR,
                     ( -off.y - lineY ) / SCALE_FACTOR, ( off.x + lineX ) / SCALE_FACTOR,
                     ( -off.y - lineY ) / SCALE_FACTOR );

            // right line
            if( lineY > 0 )
            {
                fprintf( m_file, "LINE %g %g %g %g\n",
                         ( off.x + lineX + radius ) / SCALE_FACTOR,
                         ( -off.y + lineY ) / SCALE_FACTOR,
                         ( off.x + lineX + radius ) / SCALE_FACTOR,
                         ( -off.y - lineY ) / SCALE_FACTOR );
            }

            // top right arc
            fprintf( m_file, "ARC %g %g %g %g %g %g\n",
                     ( off.x + lineX + radius ) / SCALE_FACTOR,
                     ( -off.y + lineY ) / SCALE_FACTOR, ( off.x + lineX ) / SCALE_FACTOR,
                     ( -off.y + lineY + radius ) / SCALE_FACTOR,
                     ( off.x + lineX ) / SCALE_FACTOR, ( -off.y + lineY ) / SCALE_FACTOR );

            // top line
            if( lineX > 0 )
            {
                fprintf( m_file, "LINE %g %g %g %g\n"
                         , ( off.x - lineX ) / SCALE_FACTOR,
                         ( -off.y + lineY + radius ) / SCALE_FACTOR,
                         ( off.x + lineX ) / SCALE_FACTOR,
                         ( -off.y + lineY + radius ) / SCALE_FACTOR );
            }

            // top left arc
            fprintf( m_file, "ARC %g %g %g %g %g %g\n",
                     ( off.x - lineX ) / SCALE_FACTOR,
                     ( -off.y + lineY + radius ) / SCALE_FACTOR,
                     ( off.x - lineX - radius ) / SCALE_FACTOR,
                     ( -off.y + lineY ) / SCALE_FACTOR, ( off.x - lineX ) / SCALE_FACTOR,
                     ( -off.y + lineY ) / SCALE_FACTOR );

            // left line
            if( lineY > 0 )
            {
                fprintf( m_file, "LINE %g %g %g %g\n",
                         ( off.x - lineX - radius ) / SCALE_FACTOR,
                         ( -off.y - lineY ) / SCALE_FACTOR,
                         ( off.x - lineX - radius ) / SCALE_FACTOR,
                         ( -off.y + lineY ) / SCALE_FACTOR );
            }

            break;
        }

        case PAD_SHAPE::TRAPEZOID:
        {
            fprintf( m_file, " POLYGON %g\n", pad->GetDrillSize().x / SCALE_FACTOR );

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
                fprintf( m_file, "LINE %g %g %g %g\n",
                         ( off.x + poly[cur].x ) / SCALE_FACTOR,
                         ( -off.y - poly[cur].y ) / SCALE_FACTOR,
                         ( off.x + poly[next].x ) / SCALE_FACTOR,
                         ( -off.y - poly[next].y ) / SCALE_FACTOR );
            }

            break;
        }

        case PAD_SHAPE::CHAMFERED_RECT:
        {
            fprintf( m_file, " POLYGON %g\n", pad->GetDrillSize().x / SCALE_FACTOR );

            SHAPE_POLY_SET outline;
            int            maxError = m_board->GetDesignSettings().m_MaxError;
            VECTOR2I       padOffset( 0, 0 );

            TransformRoundChamferedRectToPolygon( outline, padOffset,
                                                  pad->GetSize( PADSTACK::ALL_LAYERS ),
                                                  pad->GetOrientation(),
                                                  pad->GetRoundRectCornerRadius( PADSTACK::ALL_LAYERS ),
                                                  pad->GetChamferRectRatio( PADSTACK::ALL_LAYERS ),
                                                  pad->GetChamferPositions( PADSTACK::ALL_LAYERS ),
                                                  0, maxError, ERROR_INSIDE );

            for( int jj = 0; jj < outline.OutlineCount(); ++jj )
            {
                const SHAPE_LINE_CHAIN& poly = outline.COutline( jj );
                int pointCount = poly.PointCount();

                for( int ii = 0; ii < pointCount; ii++ )
                {
                    int next = ( ii + 1 ) % pointCount;
                    fprintf( m_file, "LINE %g %g %g %g\n",
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
            fprintf( m_file, " POLYGON %g\n", pad->GetDrillSize().x / SCALE_FACTOR );

            SHAPE_POLY_SET outline;
            pad->MergePrimitivesAsPolygon( F_Cu, &outline );

            for( int jj = 0; jj < outline.OutlineCount(); ++jj )
            {
                const SHAPE_LINE_CHAIN& poly = outline.COutline( jj );
                int pointCount = poly.PointCount();

                for( int ii = 0; ii < pointCount; ii++ )
                {
                    int next = ( ii + 1 ) % pointCount;
                    fprintf( m_file, "LINE %g %g %g %g\n",
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

    fputs( "\n$ENDPADS\n\n", m_file );

    // Now emit the padstacks definitions, using the combined layer masks
    fputs( "$PADSTACKS\n", m_file );

    // Via padstacks
    for( unsigned i = 0; i < viastacks.size(); i++ )
    {
        PCB_VIA* via = viastacks[i];

        LSET mask = via->GetLayerSet() & master_layermask;

        fprintf( m_file, "PADSTACK VIA%d.%d.%s %g\n",
                 via->GetWidth(), via->GetDrillValue(),
                 fmt_mask( mask ).c_str(),
                 via->GetDrillValue() / SCALE_FACTOR );

        for( PCB_LAYER_ID layer : mask.Seq( gc_seq ) )
        {
            fprintf( m_file, "PAD V%d.%d.%s %s 0 0\n",
                    via->GetWidth(), via->GetDrillValue(),
                    fmt_mask( mask ).c_str(),
                    GenCADLayerName( cu_count, layer ).c_str() );
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
        fprintf( m_file, "PADSTACK PAD%u %g\n", i, pad->GetDrillSize().x / SCALE_FACTOR );

        LSET pad_set = pad->GetLayerSet() & master_layermask;

        // the special gc_seq
        for( PCB_LAYER_ID layer : pad_set.Seq( gc_seq ) )
        {
            fprintf( m_file, "PAD P%u %s 0 0\n", i, GenCADLayerName( cu_count, layer ).c_str() );
        }

        // Flipped padstack
        if( m_flipBottomPads )
        {
            fprintf( m_file, "PADSTACK PAD%uF %g\n", i, pad->GetDrillSize().x / SCALE_FACTOR );

            // the normal PCB_LAYER_ID sequence is inverted from gc_seq[]
            for( PCB_LAYER_ID layer : pad_set.Seq() )
            {
                fprintf( m_file, "PAD P%u %s 0 0\n", i,
                         GenCADLayerNameFlipped( cu_count, layer ).c_str() );
            }
        }
    }

    fputs( "$ENDPADSTACKS\n\n", m_file );
}


/// Compute hashes for footprints without taking into account their position, rotation or layer
static size_t hashFootprint( const FOOTPRINT* aFootprint )
{
    size_t    ret = 0x11223344;
    constexpr int flags = HASH_FLAGS::HASH_POS | HASH_FLAGS::REL_COORD
                            | HASH_FLAGS::HASH_ROT | HASH_FLAGS::HASH_LAYER;

    for( PCB_FIELD* i : aFootprint->Fields() )
        ret += hash_fp_item( i, flags );

    for( BOARD_ITEM* i : aFootprint->GraphicalItems() )
        ret += hash_fp_item( i, flags );

    for( PAD* i : aFootprint->Pads() )
        ret += hash_fp_item( i, flags );

    return ret;
}


/* Creates the footprint shape list.
 * Since module shape is customizable after the placement we cannot share them;
 * instead we opt for the one-module-one-shape-one-component-one-device approach
 */
void GENCAD_EXPORTER::CreateShapesSection()
{
    const char* layer;
    wxString    pinname;
    const char* mirror = "0";
    std::map<wxString, size_t> shapes;

    fputs( "$SHAPES\n", m_file );

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
            FootprintWriteShape( footprint, shapeName );
        }
        else // individual shape for each component
        {
            FootprintWriteShape( footprint, footprint->GetReference() );
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

            // Bottom side footprints use the flipped padstack
            fprintf( m_file, ( m_flipBottomPads && footprint->GetFlag() ) ?
                     "PIN \"%s\" PAD%dF %g %g %s %g %s\n" :
                     "PIN \"%s\" PAD%d %g %g %s %g %s\n",
                     TO_UTF8( escapeString( pinname ) ), pad->GetSubRatsnest(),
                     padPos.x / SCALE_FACTOR,
                     -padPos.y / SCALE_FACTOR,
                     layer, orient.AsDegrees(), mirror );
        }
    }

    fputs( "$ENDSHAPES\n\n", m_file );
}


/* Creates the section $COMPONENTS (Footprints placement)
 * Bottom side components are difficult to handle: shapes must be mirrored or
 * flipped, silk layers need to be handled correctly and so on. Also it seems
 * that *no one* follows the specs...
 */
void GENCAD_EXPORTER::CreateComponentsSection()
{
    fputs( "$COMPONENTS\n", m_file );

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

        fprintf( m_file, "\nCOMPONENT \"%s\"\n",
                 TO_UTF8( escapeString( footprint->GetReference() ) ) );
        fprintf( m_file, "DEVICE \"DEV_%s\"\n",
                 TO_UTF8( escapeString( getShapeName( footprint ) ) ) );
        fprintf( m_file, "PLACE %g %g\n",
                 MapXTo( footprint->GetPosition().x ),
                 MapYTo( footprint->GetPosition().y ) );
        fprintf( m_file, "LAYER %s\n",
                 footprint->GetFlag() ? "BOTTOM" : "TOP" );
        fprintf( m_file, "ROTATION %g\n",
                 fp_orient.AsDegrees() );
        fprintf( m_file, "SHAPE \"%s\" %s %s\n",
                 TO_UTF8( escapeString( getShapeName( footprint ) ) ),
                 mirror, flip );

        // Text on silk layer: RefDes and value (are they actually useful?)
        for( PCB_TEXT* textItem : { &footprint->Reference(), &footprint->Value() } )
        {
            std::string layer = GenCADLayerName( cu_count,
                                                 footprint->GetFlag() ? B_SilkS : F_SilkS );

            fprintf( m_file, "TEXT %g %g %g %g %s %s \"%s\"",
                     textItem->GetFPRelativePosition().x / SCALE_FACTOR,
                    -textItem->GetFPRelativePosition().y / SCALE_FACTOR,
                     textItem->GetTextWidth() / SCALE_FACTOR,
                     textItem->GetTextAngle().AsDegrees(),
                     mirror,
                     layer.c_str(),
                     TO_UTF8( escapeString( textItem->GetText() ) ) );

            BOX2I textBox = textItem->GetTextBox();

            fprintf( m_file, " 0 0 %g %g\n",
                     textBox.GetWidth() / SCALE_FACTOR,
                     textBox.GetHeight() / SCALE_FACTOR );
        }

        // The SHEET is a 'generic description' for referencing the component
        fprintf( m_file, "SHEET \"RefDes: %s, Value: %s\"\n",
                 TO_UTF8( footprint->GetReference() ),
                 TO_UTF8( footprint->GetValue() ) );
    }

    fputs( "$ENDCOMPONENTS\n\n", m_file );
}


void GENCAD_EXPORTER::CreateSignalsSection()
{
    // Emit the netlist (which is actually the thing for which GenCAD is used these
    // days!); tracks are handled later

    wxString      msg;
    NETINFO_ITEM* net;
    int           NbNoConn = 1;

    fputs( "$SIGNALS\n", m_file );

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

            fputs( TO_UTF8( msg ), m_file );
            fputs( "\n", m_file );

            for( FOOTPRINT* footprint : m_board->Footprints() )
            {
                for( PAD* pad : footprint->Pads() )
                {
                    if( pad->GetNetCode() != net->GetNetCode() )
                        continue;

                    msg.Printf( wxT( "NODE \"%s\" \"%s\"" ),
                                escapeString( footprint->GetReference() ),
                                escapeString( pad->GetNumber() ) );

                    fputs( TO_UTF8( msg ), m_file );
                    fputs( "\n", m_file );
                }
            }
        }
    }

    fputs( "$ENDSIGNALS\n\n", m_file );
}


bool GENCAD_EXPORTER::CreateHeaderInfoData()
{
    wxString msg;

    fputs( "$HEADER\n", m_file );
    fputs( "GENCAD 1.4\n", m_file );

    // Please note: GenCAD syntax requires quoted strings if they can contain spaces
    msg.Printf( wxT( "USER \"KiCad %s\"\n" ), GetBuildVersion() );
    fputs( TO_UTF8( msg ), m_file );

    msg = wxT( "DRAWING \"" ) + m_board->GetFileName() + wxT( "\"\n" );
    fputs( TO_UTF8( msg ), m_file );

    wxString rev = ExpandTextVars( m_board->GetTitleBlock().GetRevision(), m_board->GetProject() );
    wxString date = ExpandTextVars( m_board->GetTitleBlock().GetDate(), m_board->GetProject() );
    msg = wxT( "REVISION \"" ) + rev + wxT( " " ) + date + wxT( "\"\n" );

    fputs( TO_UTF8( msg ), m_file );
    fputs( "UNITS INCH\n", m_file );

    // giving 0 as the argument to Map{X,Y}To returns the scaled origin point
    msg.Printf( wxT( "ORIGIN %g %g\n" ),
                m_storeOriginCoords ? MapXTo( 0 ) : 0,
                m_storeOriginCoords ? MapYTo( 0 ) : 0 );
    fputs( TO_UTF8( msg ), m_file );

    fputs( "INTERTRACK 0\n", m_file );
    fputs( "$ENDHEADER\n\n", m_file );

    return true;
}


void GENCAD_EXPORTER::CreateRoutesSection()
{
    /* Creates the section ROUTES
     * that handles tracks, vias
     * TODO: add zones
     *  section:
     *  $ROUTE
     *  ...
     *  $ENROUTE
     *  Track segments must be sorted by nets
     */

    int     vianum = 1;
    int     old_netcode, old_width, old_layer;
    LSET    master_layermask = m_board->GetDesignSettings().GetEnabledLayers();

    int     cu_count = m_board->GetCopperLayerCount();

    TRACKS tracks( m_board->Tracks() );
    std::sort( tracks.begin(), tracks.end(),
            []( const PCB_TRACK* a, const PCB_TRACK* b )
            {
                if( a->GetNetCode() == b->GetNetCode() )
                {
                    if( a->GetWidth() == b->GetWidth() )
                        return ( a->GetLayer() < b->GetLayer() );

                    return ( a->GetWidth() < b->GetWidth() );
                }

                return ( a->GetNetCode() < b->GetNetCode() );
            } );

    fputs( "$ROUTES\n", m_file );

    old_netcode = -1; old_width = -1; old_layer = -1;

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

            fprintf( m_file, "ROUTE \"%s\"\n", TO_UTF8( escapeString( netname ) ) );
        }

        if( old_width != track->GetWidth() )
        {
            old_width = track->GetWidth();
            fprintf( m_file, "TRACK TRACK%d\n", track->GetWidth() );
        }

        if( track->Type() == PCB_TRACE_T )
        {
            if( old_layer != track->GetLayer() )
            {
                old_layer = track->GetLayer();
                fprintf( m_file, "LAYER %s\n",
                        GenCADLayerName( cu_count, track->GetLayer() ).c_str() );
            }

            fprintf( m_file, "LINE %g %g %g %g\n",
                    MapXTo( track->GetStart().x ), MapYTo( track->GetStart().y ),
                    MapXTo( track->GetEnd().x ), MapYTo( track->GetEnd().y ) );
        }

        if( track->Type() == PCB_VIA_T )
        {
            const PCB_VIA* via = static_cast<const PCB_VIA*>(track);

            LSET vset = via->GetLayerSet() & master_layermask;

            fprintf( m_file, "VIA VIA%d.%d.%s %g %g ALL %g via%d\n",
                     via->GetWidth(), via->GetDrillValue(),
                     fmt_mask( vset ).c_str(),
                     MapXTo( via->GetStart().x ), MapYTo( via->GetStart().y ),
                     via->GetDrillValue() / SCALE_FACTOR, vianum++ );
        }
    }

    fputs( "$ENDROUTES\n\n", m_file );
}


void GENCAD_EXPORTER::CreateDevicesSection()
{
    /* Creates the section $DEVICES
     * This is a list of footprints properties
     *  ( Shapes are in section $SHAPE )
     */
    std::set<wxString> emitted;

    fputs( "$DEVICES\n", m_file );

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
        fprintf( m_file, "%s", TO_UTF8( item ) );

    fputs( "$ENDDEVICES\n\n", m_file );
}


void GENCAD_EXPORTER::CreateBoardSection( )
{
    // Creates the section $BOARD.
    //  We output here only the board perimeter

    fputs( "$BOARD\n", m_file );

    // Extract the board edges
    SHAPE_POLY_SET outline;
    m_board->GetBoardPolygonOutlines( outline );

    for( auto seg1 = outline.IterateSegmentsWithHoles(); seg1; seg1++ )
    {
        SEG seg = *seg1;
        fprintf( m_file, "LINE %g %g %g %g\n",
                 MapXTo( seg.A.x ), MapYTo( seg.A.y ),
                 MapXTo( seg.B.x ), MapYTo( seg.B.y ) );
    }

    fputs( "$ENDBOARD\n\n", m_file );
}


/* Creates the section "$TRACKS"
 *  This sections give the list of widths (tools) used in tracks and vias
 *  format:
 *  $TRACK
 *  TRACK <name> <width>
 *  $ENDTRACK
 *
 *  Each tool name is build like this: "TRACK" + track width.
 *  For instance for a width = 120 : name = "TRACK120".
 */
void GENCAD_EXPORTER::CreateTracksInfoData()
{
    // Find thickness used for traces
    std::set<int> trackinfo;

    for( PCB_TRACK* track : m_board->Tracks() )
        trackinfo.insert( track->GetWidth() );

    // Write data
    fputs( "$TRACKS\n", m_file );

    for( int size : trackinfo )
        fprintf( m_file, "TRACK TRACK%d %g\n", size, size / SCALE_FACTOR );

    fputs( "$ENDTRACKS\n\n", m_file );
}


/* Creates the shape of a footprint (section SHAPE)
 *  The shape is always given "normal" (Orient 0, not mirrored)
 * It's almost guaranteed that the silk layer will be imported wrong but
 * the shape also contains the pads!
 */
void GENCAD_EXPORTER::FootprintWriteShape( FOOTPRINT* aFootprint, const wxString& aShapeName )
{
    /* creates header: */
    fprintf( m_file, "\nSHAPE \"%s\"\n", TO_UTF8( escapeString( aShapeName ) ) );

    if( aFootprint->GetAttributes() & FP_THROUGH_HOLE )
        fprintf( m_file, "INSERT TH\n" );
    else
        fprintf( m_file, "INSERT SMD\n" );

    // Silk outline; wildly interpreted by various importers:
    // CAM350 read it right but only closed shapes
    // ProntoPlace double-flip it (at least the pads are correct)
    // GerberTool usually get it right...
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->Type() == PCB_SHAPE_T
                    && ( item->GetLayer() == F_SilkS || item->GetLayer() == B_SilkS ) )
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
                fprintf( m_file, "LINE %g %g %g %g\n",
                         start.x / SCALE_FACTOR,
                         -start.y / SCALE_FACTOR,
                         end.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR );
                break;

            case SHAPE_T::RECTANGLE:
                fprintf( m_file, "LINE %g %g %g %g\n",
                         start.x / SCALE_FACTOR,
                         -start.y / SCALE_FACTOR,
                         end.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR );
                fprintf( m_file, "LINE %g %g %g %g\n",
                         end.x / SCALE_FACTOR,
                         -start.y / SCALE_FACTOR,
                         end.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR );
                fprintf( m_file, "LINE %g %g %g %g\n",
                         end.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR,
                         start.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR );
                fprintf( m_file, "LINE %g %g %g %g\n",
                         start.x / SCALE_FACTOR,
                         -end.y / SCALE_FACTOR,
                         start.x / SCALE_FACTOR,
                         -start.y / SCALE_FACTOR );
                break;

            case SHAPE_T::CIRCLE:
            {
                int radius = KiROUND( end.Distance( start ) );

                fprintf( m_file, "CIRCLE %g %g %g\n",
                         start.x / SCALE_FACTOR,
                         -start.y / SCALE_FACTOR,
                         radius / SCALE_FACTOR );
                break;
            }

            case SHAPE_T::ARC:
                if( shape->GetArcAngle() > ANGLE_0 )
                    std::swap( start, end );

                fprintf( m_file, "ARC %g %g %g %g %g %g\n",
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
