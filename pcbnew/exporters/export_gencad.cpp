/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file export_gencad.cpp
 * @brief Export GenCAD 1.4 format.
 */

#include <fctsys.h>
#include <confirm.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <pcb_edit_frame.h>
#include <trigo.h>
#include <build_version.h>
#include <macros.h>
#include <pcbnew.h>
#include <dialogs/dialog_gencad_export_options.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <hash_eda.h>

static bool CreateHeaderInfoData( FILE* aFile, PCB_EDIT_FRAME* frame );
static void CreateArtworksSection( FILE* aFile );
static void CreateTracksInfoData( FILE* aFile, BOARD* aPcb );
static void CreateBoardSection( FILE* aFile, BOARD* aPcb );
static void CreateComponentsSection( FILE* aFile, BOARD* aPcb );
static void CreateDevicesSection( FILE* aFile, BOARD* aPcb );
static void CreateRoutesSection( FILE* aFile, BOARD* aPcb );
static void CreateSignalsSection( FILE* aFile, BOARD* aPcb );
static void CreateShapesSection( FILE* aFile, BOARD* aPcb );
static void CreatePadsShapesSection( FILE* aFile, BOARD* aPcb );
static void FootprintWriteShape( FILE* File, MODULE* module, const wxString& aShapeName );

// layer names for Gencad export

#if 0 // was:
static const wxString GenCADLayerName[] =
{
    wxT( "BOTTOM" ),             wxT( "INNER1" ),          wxT( "INNER2" ),
    wxT( "INNER3" ),             wxT( "INNER4" ),          wxT( "INNER5" ),
    wxT( "INNER6" ),             wxT( "INNER7" ),          wxT( "INNER8" ),
    wxT( "INNER9" ),             wxT( "INNER10" ),         wxT( "INNER11" ),
    wxT( "INNER12" ),            wxT( "INNER13" ),         wxT( "INNER14" ),
    wxT( "TOP" ),                wxT( "LAYER17" ),         wxT( "LAYER18" ),
    wxT( "SOLDERPASTE_BOTTOM" ), wxT( "SOLDERPASTE_TOP" ),
    wxT( "SILKSCREEN_BOTTOM" ),  wxT( "SILKSCREEN_TOP" ),
    wxT( "SOLDERMASK_BOTTOM" ),  wxT( "SOLDERMASK_TOP" ),  wxT( "LAYER25" ),
    wxT( "LAYER26" ),            wxT( "LAYER27" ),         wxT( "LAYER28" ),
    wxT( "LAYER29" ),            wxT( "LAYER30" ),         wxT( "LAYER31" ),
    wxT( "LAYER32" )
};

// flipped layer name for Gencad export (to make CAM350 imports correct)
static const wxString GenCADLayerNameFlipped[32] =
{
    wxT( "TOP" ),             wxT( "INNER14" ),            wxT( "INNER13" ),
    wxT( "INNER12" ),         wxT( "INNER11" ),            wxT( "INNER10" ),
    wxT( "INNER9" ),          wxT( "INNER8" ),             wxT( "INNER7" ),
    wxT( "INNER6" ),          wxT( "INNER5" ),             wxT( "INNER4" ),
    wxT( "INNER3" ),          wxT( "INNER2" ),             wxT( "INNER1" ),
    wxT( "BOTTOM" ),          wxT( "LAYER17" ),            wxT( "LAYER18" ),
    wxT( "SOLDERPASTE_TOP" ), wxT( "SOLDERPASTE_BOTTOM" ),
    wxT( "SILKSCREEN_TOP" ),  wxT( "SILKSCREEN_BOTTOM" ),
    wxT( "SOLDERMASK_TOP" ),  wxT( "SOLDERMASK_BOTTOM" ),  wxT( "LAYER25" ),
    wxT( "LAYER26" ),         wxT( "LAYER27" ),            wxT( "LAYER28" ),
    wxT( "LAYER29" ),         wxT( "LAYER30" ),            wxT( "LAYER31" ),
    wxT( "LAYER32" )
};

#else

static std::string GenCADLayerName( int aCuCount, PCB_LAYER_ID aId )
{
    if( IsCopperLayer( aId ) )
    {
        if( aId == F_Cu )
            return "TOP";
        else if( aId == B_Cu )
            return "BOTTOM";

        else if( aId <= 14 )
        {
            return StrPrintf(  "INNER%d", aCuCount - aId - 1 );
        }
        else
        {
            return StrPrintf( "LAYER%d", aId );
        }
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


static const PCB_LAYER_ID gc_seq[] = {
    B_Cu,
    In30_Cu,
    In29_Cu,
    In28_Cu,
    In27_Cu,
    In26_Cu,
    In25_Cu,
    In24_Cu,
    In23_Cu,
    In22_Cu,
    In21_Cu,
    In20_Cu,
    In19_Cu,
    In18_Cu,
    In17_Cu,
    In16_Cu,
    In15_Cu,
    In14_Cu,
    In13_Cu,
    In12_Cu,
    In11_Cu,
    In10_Cu,
    In9_Cu,
    In8_Cu,
    In7_Cu,
    In6_Cu,
    In5_Cu,
    In4_Cu,
    In3_Cu,
    In2_Cu,
    In1_Cu,
    F_Cu,
};


// flipped layer name for Gencad export (to make CAM350 imports correct)
static std::string GenCADLayerNameFlipped( int aCuCount, PCB_LAYER_ID aId )
{
    if( 1<= aId && aId <= 14 )
    {
        return StrPrintf(  "INNER%d", 14 - aId );
    }

    return GenCADLayerName( aCuCount, aId );
}


static wxString escapeString( const wxString& aString )
{
    wxString copy( aString );
    copy.Replace( "\"", "\\\"" );
    return copy;
}

#endif

static std::string fmt_mask( LSET aSet )
{
#if 0
    return aSet.FmtHex();
#else
    return StrPrintf( "%08x", (unsigned) ( aSet & LSET::AllCuMask() ).to_ulong() );
#endif
}

// Export options
static bool flipBottomPads;
static bool uniquePins;
static bool individualShapes;
static bool storeOriginCoords;

// These are the export origin (the auxiliary axis)
static int GencadOffsetX, GencadOffsetY;

// Association between shape names (using shapeName index) and components
static std::map<MODULE*, int> componentShapes;
static std::map<int, wxString> shapeNames;

static const wxString getShapeName( MODULE* aModule )
{
    static const wxString invalid( "invalid" );

    if( individualShapes )
        return aModule->GetReference();

    auto itShape = componentShapes.find( aModule );
    wxCHECK( itShape != componentShapes.end(), invalid );

    auto itName = shapeNames.find( itShape->second );
    wxCHECK( itName != shapeNames.end(), invalid );

    return itName->second;
}

// GerbTool chokes on units different than INCH so this is the conversion factor
const static double SCALE_FACTOR = 1000.0 * IU_PER_MILS;

/* Two helper functions to calculate coordinates of modules in gencad values
 * (GenCAD Y axis from bottom to top)
 */
static double MapXTo( int aX )
{
    return (aX - GencadOffsetX) / SCALE_FACTOR;
}


static double MapYTo( int aY )
{
    return (GencadOffsetY - aY) / SCALE_FACTOR;
}


/* Driver function: processing starts here */
void PCB_EDIT_FRAME::ExportToGenCAD( wxCommandEvent& aEvent )
{
    // Build default output file name
    wxString path = GetLastPath( LAST_PATH_GENCAD );

    if( path.IsEmpty() )
    {
        wxFileName brdFile = GetBoard()->GetFileName();
        brdFile.SetExt( "cad" );
        path = brdFile.GetFullPath();
    }

    DIALOG_GENCAD_EXPORT_OPTIONS optionsDialog( this, path );

    if( optionsDialog.ShowModal() == wxID_CANCEL )
        return;

    path = optionsDialog.GetFileName();
    SetLastPath( LAST_PATH_GENCAD, path );
    FILE* file = wxFopen( path, "wt" );

    if( !file )
    {
        DisplayError( this, wxString::Format( _( "Unable to create \"%s\"" ),
                                              optionsDialog.GetFileName() ) );
        return;
    }

    // Get options
    flipBottomPads = optionsDialog.GetOption( FLIP_BOTTOM_PADS );
    uniquePins = optionsDialog.GetOption( UNIQUE_PIN_NAMES );
    individualShapes = optionsDialog.GetOption( INDIVIDUAL_SHAPES );
    storeOriginCoords = optionsDialog.GetOption( STORE_ORIGIN_COORDS );

    // Switch the locale to standard C (needed to print floating point numbers)
    LOCALE_IO toggle;

    // Update some board data, to ensure a reliable gencad export
    GetBoard()->ComputeBoundingBox();

    // Save the auxiliary origin for the rest of the module
    GencadOffsetX = optionsDialog.GetOption( USE_AUX_ORIGIN ) ? GetAuxOrigin().x : 0;
    GencadOffsetY = optionsDialog.GetOption( USE_AUX_ORIGIN ) ? GetAuxOrigin().y : 0;

    // No idea on *why* this should be needed... maybe to fix net names?
    Compile_Ratsnest( true );

    /* Temporary modification of footprints that are flipped (i.e. on bottom
     * layer) to convert them to non flipped footprints.
     *  This is necessary to easily export shapes to GenCAD,
     *  that are given as normal orientation (non flipped, rotation = 0))
     * these changes will be undone later
     */
    BOARD*  pcb = GetBoard();

    for( auto module : pcb->Modules() )
    {
        module->SetFlag( 0 );

        if( module->GetLayer() == B_Cu )
        {
            module->Flip( module->GetPosition(), Settings().m_FlipLeftRight );
            module->SetFlag( 1 );
        }
    }

    /* Gencad has some mandatory and some optional sections: some importer
     *  need the padstack section (which is optional) anyway. Also the
     *  order of the section *is* important */

    CreateHeaderInfoData( file, this );     // Gencad header
    CreateBoardSection( file, pcb );        // Board perimeter

    CreatePadsShapesSection( file, pcb );   // Pads and padstacks
    CreateArtworksSection( file );          // Empty but mandatory

    /* Gencad splits a component info in shape, component and device.
     *  We don't do any sharing (it would be difficult since each module is
     *  customizable after placement) */
    CreateShapesSection( file, pcb );
    CreateComponentsSection( file, pcb );
    CreateDevicesSection( file, pcb );

    // In a similar way the netlist is split in net, track and route
    CreateSignalsSection( file, pcb );
    CreateTracksInfoData( file, pcb );
    CreateRoutesSection( file, pcb );

    fclose( file );

    // Undo the footprints modifications (flipped footprints)
    for( auto module : pcb->Modules() )
    {
        if( module->GetFlag() )
        {
            module->Flip( module->GetPosition(), Settings().m_FlipLeftRight );
            module->SetFlag( 0 );
        }
    }

    componentShapes.clear();
    shapeNames.clear();
}


// Sort vias for uniqueness
static bool ViaSort( const VIA* aPadref, const VIA* aPadcmp )
{
    if( aPadref->GetWidth() != aPadcmp->GetWidth() )
        return aPadref->GetWidth() < aPadcmp->GetWidth();

    if( aPadref->GetDrillValue() != aPadcmp->GetDrillValue() )
        return aPadref->GetDrillValue() < aPadcmp->GetDrillValue();

    if( aPadref->GetLayerSet() != aPadcmp->GetLayerSet() )
        return aPadref->GetLayerSet().FmtBin().compare( aPadcmp->GetLayerSet().FmtBin() ) < 0;

    return false;
}


// The ARTWORKS section is empty but (officially) mandatory
static void CreateArtworksSection( FILE* aFile )
{
    /* The artworks section is empty */
    fputs( "$ARTWORKS\n", aFile );
    fputs( "$ENDARTWORKS\n\n", aFile );
}


// Emit PADS and PADSTACKS. They are sorted and emitted uniquely.
// Via name is synthesized from their attributes, pads are numbered
static void CreatePadsShapesSection( FILE* aFile, BOARD* aPcb )
{
    std::vector<D_PAD*> padstacks;
    std::vector<VIA*>   vias;
    std::vector<VIA*>   viastacks;

    padstacks.resize( 1 ); // We count pads from 1

    // The master layermask (i.e. the enabled layers) for padstack generation
    LSET    master_layermask = aPcb->GetDesignSettings().GetEnabledLayers();
    int     cu_count = aPcb->GetCopperLayerCount();

    fputs( "$PADS\n", aFile );

    // Enumerate and sort the pads

    auto pads( aPcb->GetPads() );
    std::sort( pads.begin(), pads.end(),
            []( const D_PAD* a, const D_PAD* b ) { return D_PAD::Compare( a, b ) < 0; } );


    // The same for vias
    for( auto track : aPcb->Tracks() )
    {
        if( auto via = dyn_cast<VIA*>( track ) )
            vias.push_back( via );
    }

    std::sort( vias.begin(), vias.end(), ViaSort );
    vias.erase( std::unique( vias.begin(), vias.end(),
                        []( const VIA* a, const VIA* b ) { return ViaSort( a, b ) == false; } ),
            vias.end() );

    // Emit vias pads

    for( auto item : vias )
    {
        VIA* via = static_cast<VIA*>( item );

        viastacks.push_back( via );
        fprintf( aFile, "PAD V%d.%d.%s ROUND %g\nCIRCLE 0 0 %g\n",
                via->GetWidth(), via->GetDrillValue(),
                fmt_mask( via->GetLayerSet() & master_layermask ).c_str(),
                via->GetDrillValue() / SCALE_FACTOR,
                via->GetWidth() / (SCALE_FACTOR * 2) );
    }

    // Emit component pads
    D_PAD* old_pad = 0;
    int    pad_name_number = 0;

    for( unsigned i = 0; i<pads.size(); ++i )
    {
        D_PAD* pad = pads[i];
        const wxPoint& off = pad->GetOffset();

        pad->SetSubRatsnest( pad_name_number );

        if( old_pad && 0==D_PAD::Compare( old_pad, pad ) )
            continue;  // already created

        old_pad = pad;

        pad_name_number++;
        pad->SetSubRatsnest( pad_name_number );

        fprintf( aFile, "PAD P%d", pad->GetSubRatsnest() );

        padstacks.push_back( pad ); // Will have its own padstack later
        int dx = pad->GetSize().x / 2;
        int dy = pad->GetSize().y / 2;

        switch( pad->GetShape() )
        {
        default:
            wxASSERT_MSG( false, "Pad type not implemented" );
            // fall-through

        case PAD_SHAPE_CIRCLE:
            fprintf( aFile, " ROUND %g\n",
                     pad->GetDrillSize().x / SCALE_FACTOR );
            /* Circle is center, radius */
            fprintf( aFile, "CIRCLE %g %g %g\n",
                    off.x / SCALE_FACTOR,
                    -off.y / SCALE_FACTOR,
                    pad->GetSize().x / (SCALE_FACTOR * 2) );
            break;

        case PAD_SHAPE_RECT:
            fprintf( aFile, " RECTANGULAR %g\n",
                     pad->GetDrillSize().x / SCALE_FACTOR );

            // Rectangle is begin, size *not* begin, end!
            fprintf( aFile, "RECTANGLE %g %g %g %g\n",
                    (-dx + off.x ) / SCALE_FACTOR,
                    (-dy - off.y ) / SCALE_FACTOR,
                    dx / (SCALE_FACTOR / 2), dy / (SCALE_FACTOR / 2) );
            break;

        case PAD_SHAPE_ROUNDRECT:
        case PAD_SHAPE_OVAL:
            {
                const wxSize& size = pad->GetSize();
                int radius;

                if( pad->GetShape() == PAD_SHAPE_ROUNDRECT )
                    radius = pad->GetRoundRectCornerRadius();
                else
                    radius = std::min( size.x, size.y ) / 2;

                int lineX = size.x / 2 - radius;
                int lineY = size.y / 2 - radius;

                fprintf( aFile, " POLYGON %g\n", pad->GetDrillSize().x / SCALE_FACTOR );

                // bottom left arc
                fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                        ( off.x - lineX - radius ) / SCALE_FACTOR,
                        ( -off.y - lineY ) / SCALE_FACTOR, ( off.x - lineX ) / SCALE_FACTOR,
                        ( -off.y - lineY - radius ) / SCALE_FACTOR,
                        ( off.x - lineX ) / SCALE_FACTOR, ( -off.y - lineY ) / SCALE_FACTOR );

                // bottom line
                if( lineX > 0 )
                {
                    fprintf( aFile, "LINE %g %g %g %g\n",
                            ( off.x - lineX ) / SCALE_FACTOR,
                            ( -off.y - lineY - radius ) / SCALE_FACTOR,
                            ( off.x + lineX ) / SCALE_FACTOR,
                            ( -off.y - lineY - radius ) / SCALE_FACTOR );
                }

                // bottom right arc
                fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                        ( off.x + lineX ) / SCALE_FACTOR,
                        ( -off.y - lineY - radius ) / SCALE_FACTOR,
                        ( off.x + lineX + radius ) / SCALE_FACTOR,
                        ( -off.y - lineY ) / SCALE_FACTOR, ( off.x + lineX ) / SCALE_FACTOR,
                        ( -off.y - lineY ) / SCALE_FACTOR );

                // right line
                if( lineY > 0 )
                {
                    fprintf( aFile, "LINE %g %g %g %g\n",
                            ( off.x + lineX + radius ) / SCALE_FACTOR,
                            ( -off.y + lineY ) / SCALE_FACTOR,
                            ( off.x + lineX + radius ) / SCALE_FACTOR,
                            ( -off.y - lineY ) / SCALE_FACTOR );
                }

                // top right arc
                fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                        ( off.x + lineX + radius ) / SCALE_FACTOR,
                        ( -off.y + lineY ) / SCALE_FACTOR, ( off.x + lineX ) / SCALE_FACTOR,
                        ( -off.y + lineY + radius ) / SCALE_FACTOR,
                        ( off.x + lineX ) / SCALE_FACTOR, ( -off.y + lineY ) / SCALE_FACTOR );

                // top line
                if( lineX > 0 )
                {
                    fprintf( aFile, "LINE %g %g %g %g\n"
                            , ( off.x - lineX ) / SCALE_FACTOR,
                            ( -off.y + lineY + radius ) / SCALE_FACTOR,
                            ( off.x + lineX ) / SCALE_FACTOR,
                            ( -off.y + lineY + radius ) / SCALE_FACTOR );
                }

                // top left arc
                fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                        ( off.x - lineX ) / SCALE_FACTOR,
                        ( -off.y + lineY + radius ) / SCALE_FACTOR,
                        ( off.x - lineX - radius ) / SCALE_FACTOR,
                        ( -off.y + lineY ) / SCALE_FACTOR, ( off.x - lineX ) / SCALE_FACTOR,
                        ( -off.y + lineY ) / SCALE_FACTOR );

                // left line
                if( lineY > 0 )
                {
                    fprintf( aFile, "LINE %g %g %g %g\n",
                            ( off.x - lineX - radius ) / SCALE_FACTOR,
                            ( -off.y - lineY ) / SCALE_FACTOR,
                            ( off.x - lineX - radius ) / SCALE_FACTOR,
                            ( -off.y + lineY ) / SCALE_FACTOR );
                }
            }
            break;

        case PAD_SHAPE_TRAPEZOID:
            {
                fprintf( aFile, " POLYGON %g\n", pad->GetDrillSize().x / SCALE_FACTOR );

                wxPoint poly[4];
                pad->BuildPadPolygon( poly, wxSize( 0, 0 ), 0 );

                for( int cur = 0; cur < 4; ++cur )
                {
                    int next = ( cur + 1 ) % 4;
                    fprintf( aFile, "LINE %g %g %g %g\n",
                            ( off.x + poly[cur].x ) / SCALE_FACTOR,
                            ( -off.y - poly[cur].y ) / SCALE_FACTOR,
                            ( off.x + poly[next].x ) / SCALE_FACTOR,
                            ( -off.y - poly[next].y ) / SCALE_FACTOR );
                }
            }
            break;

        case PAD_SHAPE_CUSTOM:
            {
                fprintf( aFile, " POLYGON %g\n", pad->GetDrillSize().x / SCALE_FACTOR );

                const SHAPE_POLY_SET& outline = pad->GetCustomShapeAsPolygon();

                for( int jj = 0; jj < outline.OutlineCount(); ++jj )
                {
                    const SHAPE_LINE_CHAIN& poly = outline.COutline( jj );
                    int pointCount = poly.PointCount();

                    for( int ii = 0; ii < pointCount; ii++ )
                    {
                        int next = ( ii + 1 ) % pointCount;
                        fprintf( aFile, "LINE %g %g %g %g\n",
                                ( off.x + poly.CPoint( ii ).x ) / SCALE_FACTOR,
                                ( -off.y - poly.CPoint( ii ).y ) / SCALE_FACTOR,
                                ( off.x + poly.CPoint( next ).x ) / SCALE_FACTOR,
                                ( -off.y - poly.CPoint( next ).y ) / SCALE_FACTOR );
                    }
                }
            }
            break;
        }
    }

    fputs( "\n$ENDPADS\n\n", aFile );

    // Now emit the padstacks definitions, using the combined layer masks
    fputs( "$PADSTACKS\n", aFile );

    // Via padstacks
    for( unsigned i = 0; i < viastacks.size(); i++ )
    {
        VIA* via = viastacks[i];

        LSET mask = via->GetLayerSet() & master_layermask;

        fprintf( aFile, "PADSTACK VIA%d.%d.%s %g\n",
                 via->GetWidth(), via->GetDrillValue(),
                 fmt_mask( mask ).c_str(),
                 via->GetDrillValue() / SCALE_FACTOR );

        for( LSEQ seq = mask.Seq( gc_seq, arrayDim( gc_seq ) );  seq;  ++seq )
        {
            PCB_LAYER_ID layer = *seq;

            fprintf( aFile, "PAD V%d.%d.%s %s 0 0\n",
                    via->GetWidth(), via->GetDrillValue(),
                    fmt_mask( mask ).c_str(),
                    GenCADLayerName( cu_count, layer ).c_str()
                    );
        }
    }

    /* Component padstacks
     *  Older versions of CAM350 don't apply correctly the FLIP semantics for
     *  padstacks, i.e. doesn't swap the top and bottom layers... so I need to
     *  define the shape as MIRRORX and define a separate 'flipped' padstack...
     *  until it appears yet another noncompliant importer */
    for( unsigned i = 1; i < padstacks.size(); i++ )
    {
        D_PAD* pad = padstacks[i];

        // Straight padstack
        fprintf( aFile, "PADSTACK PAD%u %g\n", i, pad->GetDrillSize().x / SCALE_FACTOR );

        LSET pad_set = pad->GetLayerSet() & master_layermask;

        // the special gc_seq
        for( LSEQ seq = pad_set.Seq( gc_seq, arrayDim( gc_seq ) );  seq;  ++seq )
        {
            PCB_LAYER_ID layer = *seq;

            fprintf( aFile, "PAD P%u %s 0 0\n", i, GenCADLayerName( cu_count, layer ).c_str() );
        }

        // Flipped padstack
        if( flipBottomPads )
        {
            fprintf( aFile, "PADSTACK PAD%uF %g\n", i, pad->GetDrillSize().x / SCALE_FACTOR );

            // the normal PCB_LAYER_ID sequence is inverted from gc_seq[]
            for( LSEQ seq = pad_set.Seq();  seq;  ++seq )
            {
                PCB_LAYER_ID layer = *seq;

                fprintf( aFile, "PAD P%u %s 0 0\n", i, GenCADLayerNameFlipped( cu_count, layer ).c_str() );
            }
        }
    }

    fputs( "$ENDPADSTACKS\n\n", aFile );
}


/// Compute hashes for modules without taking into account their position, rotation or layer
static size_t hashModule( const MODULE* aModule )
{
    size_t ret = 0x11223344;
    constexpr int flags = HASH_FLAGS::POSITION | HASH_FLAGS::REL_COORD
                | HASH_FLAGS::ROTATION | HASH_FLAGS::LAYER;


    for( auto i : aModule->GraphicalItems() )
        ret ^= hash_eda( i, flags );

    for( auto i : aModule->Pads() )
        ret ^= hash_eda( i, flags );

    return ret;
}


/* Creates the footprint shape list.
 * Since module shape is customizable after the placement we cannot share them;
 * instead we opt for the one-module-one-shape-one-component-one-device approach
 */
static void CreateShapesSection( FILE* aFile, BOARD* aPcb )
{
    const char* layer;
    wxString    pinname;
    const char* mirror = "0";
    std::map<wxString, size_t> shapes;

    fputs( "$SHAPES\n", aFile );

    for( auto module : aPcb->Modules() )
    {
        if( !individualShapes )
        {
            // Check if such shape has been already generated, and if so - reuse it
            // It is necessary to compute hash (i.e. check all children objects) as
            // certain components instances might have been modified on the board.
            // In such case the shape will be different despite the same LIB_ID.
            wxString shapeName = module->GetFPID().Format();

            auto shapeIt = shapes.find( shapeName );
            size_t modHash = hashModule( module );

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
                        newShapeName = wxString::Format( "%s_%d", shapeName, suffix );
                        shapeIt = shapes.find( newShapeName );
                        ++suffix;
                    }
                    while( shapeIt != shapes.end() && shapeIt->second != modHash );

                    shapeName = newShapeName;
                }

                if( shapeIt != shapes.end() && modHash == shapeIt->second )
                {
                    // shape found, so reuse it
                    componentShapes[module] = modHash;
                    continue;
                }
            }

            // new shape
            componentShapes[module] = modHash;
            shapeNames[modHash] = shapeName;
            shapes[shapeName] = modHash;
            FootprintWriteShape( aFile, module, shapeName );
        }
        else // individual shape for each component
        {
            FootprintWriteShape( aFile, module, module->GetReference() );
        }

        // set of already emitted pins to check for duplicates
        std::set<wxString> pins;

        for( auto pad : module->Pads() )
        {
            /* Padstacks are defined using the correct layers for the pads, therefore to
             * all pads need to be marked as TOP to use the padstack information correctly.
             */
            layer = "TOP";
            pinname = pad->GetName();

            if( pinname.IsEmpty() )
                pinname = wxT( "none" );

            if( uniquePins )
            {
                int suffix = 0;
                wxString origPinname( pinname );

                auto it = pins.find( pinname );

                while( it != pins.end() )
                {
                    pinname = wxString::Format( "%s_%d", origPinname, suffix );
                    ++suffix;
                    it = pins.find( pinname );
                }

                pins.insert( pinname );
            }

            double orient = pad->GetOrientation() - module->GetOrientation();
            NORMALIZE_ANGLE_POS( orient );

            // Bottom side modules use the flipped padstack
            fprintf( aFile, ( flipBottomPads && module->GetFlag() ) ?
                     "PIN \"%s\" PAD%dF %g %g %s %g %s\n" :
                     "PIN \"%s\" PAD%d %g %g %s %g %s\n",
                     TO_UTF8( escapeString( pinname ) ), pad->GetSubRatsnest(),
                     pad->GetPos0().x / SCALE_FACTOR,
                     -pad->GetPos0().y / SCALE_FACTOR,
                     layer, orient / 10.0, mirror );
        }
    }

    fputs( "$ENDSHAPES\n\n", aFile );
}


/* Creates the section $COMPONENTS (Footprints placement)
 * Bottom side components are difficult to handle: shapes must be mirrored or
 * flipped, silk layers need to be handled correctly and so on. Also it seems
 * that *noone* follows the specs...
 */
static void CreateComponentsSection( FILE* aFile, BOARD* aPcb )
{
    fputs( "$COMPONENTS\n", aFile );

    int cu_count = aPcb->GetCopperLayerCount();

    for( auto module : aPcb->Modules() )
    {
        const char*   mirror;
        const char*   flip;
        double        fp_orient = module->GetOrientation();

        if( module->GetFlag() )
        {
            mirror = "MIRRORX";
            flip   = "FLIP";
            NEGATE_AND_NORMALIZE_ANGLE_POS( fp_orient );
        }
        else
        {
            mirror = "0";
            flip   = "0";
        }

        fprintf( aFile, "\nCOMPONENT \"%s\"\n",
                 TO_UTF8( escapeString( module->GetReference() ) ) );
        fprintf( aFile, "DEVICE \"DEV_%s\"\n",
                 TO_UTF8( escapeString( getShapeName( module ) ) ) );
        fprintf( aFile, "PLACE %g %g\n",
                 MapXTo( module->GetPosition().x ),
                 MapYTo( module->GetPosition().y ) );
        fprintf( aFile, "LAYER %s\n",
                 module->GetFlag() ? "BOTTOM" : "TOP" );
        fprintf( aFile, "ROTATION %g\n",
                 fp_orient / 10.0 );
        fprintf( aFile, "SHAPE \"%s\" %s %s\n",
                 TO_UTF8( escapeString( getShapeName( module ) ) ),
                 mirror, flip );

        // Text on silk layer: RefDes and value (are they actually useful?)
        TEXTE_MODULE *textmod = &module->Reference();

        for( int ii = 0; ii < 2; ii++ )
        {
            double txt_orient = textmod->GetTextAngle();
            std::string layer = GenCADLayerName( cu_count, module->GetFlag() ? B_SilkS : F_SilkS );

            fprintf( aFile, "TEXT %g %g %g %g %s %s \"%s\"",
                     textmod->GetPos0().x / SCALE_FACTOR,
                    -textmod->GetPos0().y / SCALE_FACTOR,
                     textmod->GetTextWidth() / SCALE_FACTOR,
                     txt_orient / 10.0,
                     mirror,
                     layer.c_str(),
                     TO_UTF8( escapeString( textmod->GetText() ) ) );

            // Please note, the width is approx
            fprintf( aFile, " 0 0 %g %g\n",
                     ( textmod->GetTextWidth() * textmod->GetLength() ) / SCALE_FACTOR,
                     textmod->GetTextHeight() / SCALE_FACTOR );

            textmod = &module->Value(); // Dirty trick for the second iteration
        }

        // The SHEET is a 'generic description' for referencing the component
        fprintf( aFile, "SHEET \"RefDes: %s, Value: %s\"\n",
                 TO_UTF8( module->GetReference() ),
                 TO_UTF8( module->GetValue() ) );
    }

    fputs( "$ENDCOMPONENTS\n\n", aFile );
}


/* Emit the netlist (which is actually the thing for which GenCAD is used these
 * days!); tracks are handled later */
static void CreateSignalsSection( FILE* aFile, BOARD* aPcb )
{
    wxString      msg;
    NETINFO_ITEM* net;
    int           NbNoConn = 1;

    fputs( "$SIGNALS\n", aFile );

    for( unsigned ii = 0; ii < aPcb->GetNetCount(); ii++ )
    {
        net = aPcb->FindNet( ii );

        if( net->GetNetname() == wxEmptyString ) // dummy netlist (no connection)
        {
            msg.Printf( "NoConnection%d", NbNoConn++ );
        }

        if( net->GetNet() <= 0 )  // dummy netlist (no connection)
            continue;

        msg = wxT( "SIGNAL \"" ) + escapeString( net->GetNetname() ) + "\"";

        fputs( TO_UTF8( msg ), aFile );
        fputs( "\n", aFile );

        for( auto module : aPcb->Modules() )
        {
            for( auto pad : module->Pads() )
            {
                if( pad->GetNetCode() != net->GetNet() )
                    continue;

                msg.Printf( wxT( "NODE \"%s\" \"%s\"" ),
                            GetChars( escapeString( module->GetReference() ) ),
                            GetChars( escapeString( pad->GetName() ) ) );

                fputs( TO_UTF8( msg ), aFile );
                fputs( "\n", aFile );
            }
        }
    }

    fputs( "$ENDSIGNALS\n\n", aFile );
}


// Creates the header section
static bool CreateHeaderInfoData( FILE* aFile, PCB_EDIT_FRAME* aFrame )
{
    wxString    msg;
    BOARD *board = aFrame->GetBoard();

    fputs( "$HEADER\n", aFile );
    fputs( "GENCAD 1.4\n", aFile );

    // Please note: GenCAD syntax requires quoted strings if they can contain spaces
    msg.Printf( wxT( "USER \"%s %s\"\n" ),
               GetChars( Pgm().App().GetAppName() ),
               GetChars( GetBuildVersion() ) );
    fputs( TO_UTF8( msg ), aFile );

    msg = wxT( "DRAWING \"" ) + board->GetFileName() + wxT( "\"\n" );
    fputs( TO_UTF8( msg ), aFile );

    const TITLE_BLOCK&  tb = aFrame->GetTitleBlock();

    msg = wxT( "REVISION \"" ) + tb.GetRevision() + wxT( " " ) + tb.GetDate() + wxT( "\"\n" );

    fputs( TO_UTF8( msg ), aFile );
    fputs( "UNITS INCH\n", aFile );

    // giving 0 as the argument to Map{X,Y}To returns the scaled origin point
    msg.Printf( wxT( "ORIGIN %g %g\n" ),
            storeOriginCoords ? MapXTo( 0 ) : 0,
            storeOriginCoords ? MapYTo( 0 ) : 0 );
    fputs( TO_UTF8( msg ), aFile );

    fputs( "INTERTRACK 0\n", aFile );
    fputs( "$ENDHEADER\n\n", aFile );

    return true;
}


/* Creates the section ROUTES
 * that handles tracks, vias
 * TODO: add zones
 *  section:
 *  $ROUTE
 *  ...
 *  $ENROUTE
 *  Track segments must be sorted by nets
 */
static void CreateRoutesSection( FILE* aFile, BOARD* aPcb )
{
    int     vianum = 1;
    int     old_netcode, old_width, old_layer;
    LSET    master_layermask = aPcb->GetDesignSettings().GetEnabledLayers();

    int     cu_count = aPcb->GetCopperLayerCount();

    TRACKS tracks( aPcb->Tracks() );
    std::sort( tracks.begin(), tracks.end(), []( const TRACK* a, const TRACK* b ) {
        if( a->GetNetCode() == b->GetNetCode() )
        {
            if( a->GetWidth() == b->GetWidth() )
                return ( a->GetLayer() < b->GetLayer() );

            return ( a->GetWidth() < b->GetWidth() );
        }

        return ( a->GetNetCode() < b->GetNetCode() );
    } );

    fputs( "$ROUTES\n", aFile );

    old_netcode = -1; old_width = -1; old_layer = -1;

    for( auto track : tracks )
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

            fprintf( aFile, "ROUTE \"%s\"\n", TO_UTF8( escapeString( netname ) ) );
        }

        if( old_width != track->GetWidth() )
        {
            old_width = track->GetWidth();
            fprintf( aFile, "TRACK TRACK%d\n", track->GetWidth() );
        }

        if( track->Type() == PCB_TRACE_T )
        {
            if( old_layer != track->GetLayer() )
            {
                old_layer = track->GetLayer();
                fprintf( aFile, "LAYER %s\n",
                        GenCADLayerName( cu_count, track->GetLayer() ).c_str() );
            }

            fprintf( aFile, "LINE %g %g %g %g\n",
                    MapXTo( track->GetStart().x ), MapYTo( track->GetStart().y ),
                    MapXTo( track->GetEnd().x ), MapYTo( track->GetEnd().y ) );
        }

        if( track->Type() == PCB_VIA_T )
        {
            const VIA* via = static_cast<const VIA*>(track);

            LSET vset = via->GetLayerSet() & master_layermask;

            fprintf( aFile, "VIA VIA%d.%d.%s %g %g ALL %g via%d\n",
                     via->GetWidth(), via->GetDrillValue(),
                     fmt_mask( vset ).c_str(),
                     MapXTo( via->GetStart().x ), MapYTo( via->GetStart().y ),
                     via->GetDrillValue() / SCALE_FACTOR, vianum++ );
        }
    }

    fputs( "$ENDROUTES\n\n", aFile );
}


/* Creates the section $DEVICES
 * This is a list of footprints properties
 *  ( Shapes are in section $SHAPE )
 */
static void CreateDevicesSection( FILE* aFile, BOARD* aPcb )
{
    std::set<wxString> emitted;
    fputs( "$DEVICES\n", aFile );

    for( const auto& componentShape : componentShapes )
    {
        const wxString& shapeName = shapeNames[componentShape.second];
        bool newDevice;
        std::tie( std::ignore, newDevice ) = emitted.insert( shapeName );

        if( !newDevice )        // do not repeat device definitions
            continue;

        const MODULE* module = componentShape.first;
        fprintf( aFile, "\nDEVICE \"DEV_%s\"\n", TO_UTF8( escapeString( shapeName ) ) );
        fprintf( aFile, "PART \"%s\"\n", TO_UTF8( escapeString( module->GetValue() ) ) );
        fprintf( aFile, "PACKAGE \"%s\"\n", TO_UTF8( escapeString( module->GetFPID().Format() ) ) );

        // The TYPE attribute is almost freeform
        const char* ty = "TH";

        if( module->GetAttributes() & MOD_CMS )
            ty = "SMD";

        if( module->GetAttributes() & MOD_VIRTUAL )
            ty = "VIRTUAL";

        fprintf( aFile, "TYPE %s\n", ty );
    }

    fputs( "$ENDDEVICES\n\n", aFile );
}


/* Creates the section $BOARD.
 *  We output here only the board perimeter
 */
static void CreateBoardSection( FILE* aFile, BOARD* aPcb )
{
    fputs( "$BOARD\n", aFile );

    // Extract the board edges
    for( auto drawing : aPcb->Drawings() )
    {
        if( drawing->Type() == PCB_LINE_T )
        {
            DRAWSEGMENT* drawseg = static_cast<DRAWSEGMENT*>( drawing );
            if( drawseg->GetLayer() == Edge_Cuts )
            {
                // XXX GenCAD supports arc boundaries but I've seen nothing that reads them
                fprintf( aFile, "LINE %g %g %g %g\n",
                         MapXTo( drawseg->GetStart().x ), MapYTo( drawseg->GetStart().y ),
                         MapXTo( drawseg->GetEnd().x ), MapYTo( drawseg->GetEnd().y ) );
            }
        }
    }

    fputs( "$ENDBOARD\n\n", aFile );
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
static void CreateTracksInfoData( FILE* aFile, BOARD* aPcb )
{
    // Find thickness used for traces

    std::set<int> trackinfo;

    for( auto track : aPcb->Tracks() )
        trackinfo.insert( track->GetWidth() );

    // Write data
    fputs( "$TRACKS\n", aFile );

    for( auto size : trackinfo )
        fprintf( aFile, "TRACK TRACK%d %g\n", size, size / SCALE_FACTOR );

    fputs( "$ENDTRACKS\n\n", aFile );
}


/* Creates the shape of a footprint (section SHAPE)
 *  The shape is always given "normal" (Orient 0, not mirrored)
 * It's almost guaranteed that the silk layer will be imported wrong but
 * the shape also contains the pads!
 */
static void FootprintWriteShape( FILE* aFile, MODULE* module, const wxString& aShapeName )
{
    EDGE_MODULE* PtEdge;

    /* creates header: */
    fprintf( aFile, "\nSHAPE \"%s\"\n", TO_UTF8( escapeString( aShapeName ) ) );

    if( module->GetAttributes() & MOD_VIRTUAL )
    {
        fprintf( aFile, "INSERT SMD\n" );
    }
    else
    {
        if( module->GetAttributes() & MOD_CMS )
        {
            fprintf( aFile, "INSERT SMD\n" );
        }
        else
        {
            fprintf( aFile, "INSERT TH\n" );
        }
    }

#if 0 /* ATTRIBUTE name and value is unspecified and the original exporter
       * got the syntax wrong, so CAM350 rejected the whole shape! */

    if( module->m_Attributs != MOD_DEFAULT )
    {
        fprintf( aFile, "ATTRIBUTE" );

        if( module->m_Attributs & MOD_CMS )
            fprintf( aFile, " PAD_SMD" );

        if( module->m_Attributs & MOD_VIRTUAL )
            fprintf( aFile, " VIRTUAL" );

        fprintf( aFile, "\n" );
    }
#endif

    // Silk outline; wildly interpreted by various importers:
    // CAM350 read it right but only closed shapes
    // ProntoPlace double-flip it (at least the pads are correct)
    // GerberTool usually get it right...
    for( auto PtStruct : module->GraphicalItems() )
    {
        switch( PtStruct->Type() )
        {
        case PCB_MODULE_TEXT_T:

            // If we wanted to export text, this is not the correct section
            break;

        case PCB_MODULE_EDGE_T:
            PtEdge = (EDGE_MODULE*) PtStruct;
            if( PtEdge->GetLayer() == F_SilkS
                || PtEdge->GetLayer() == B_SilkS )
            {
                switch( PtEdge->GetShape() )
                {
                case S_SEGMENT:
                    fprintf( aFile, "LINE %g %g %g %g\n",
                             PtEdge->m_Start0.x / SCALE_FACTOR,
                             -PtEdge->m_Start0.y / SCALE_FACTOR,
                             PtEdge->m_End0.x / SCALE_FACTOR,
                             -PtEdge->m_End0.y / SCALE_FACTOR );
                    break;

                case S_CIRCLE:
                {
                    int radius = KiROUND( GetLineLength( PtEdge->m_End0,
                                                         PtEdge->m_Start0 ) );
                    fprintf( aFile, "CIRCLE %g %g %g\n",
                             PtEdge->m_Start0.x / SCALE_FACTOR,
                             -PtEdge->m_Start0.y / SCALE_FACTOR,
                             radius / SCALE_FACTOR );
                    break;
                }

                case S_ARC:
                {
                    int arcendx, arcendy;
                    arcendx = PtEdge->m_End0.x - PtEdge->m_Start0.x;
                    arcendy = PtEdge->m_End0.y - PtEdge->m_Start0.y;
                    RotatePoint( &arcendx, &arcendy, -PtEdge->GetAngle() );
                    arcendx += PtEdge->GetStart0().x;
                    arcendy += PtEdge->GetStart0().y;

                    fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                                PtEdge->m_End0.x / SCALE_FACTOR,
                                -PtEdge->GetEnd0().y / SCALE_FACTOR,
                                arcendx / SCALE_FACTOR,
                                -arcendy / SCALE_FACTOR,
                                PtEdge->GetStart0().x / SCALE_FACTOR,
                                -PtEdge->GetStart0().y / SCALE_FACTOR );
                    break;
                }

                case S_POLYGON:
                    // Not exported (TODO)
                    break;

                default:
                    DisplayError( NULL, wxString::Format( "Type Edge Module %d invalid.", PtStruct->Type() ) );
                    break;
                }
            }
            break;

        default:
            break;
        }
    }
}
