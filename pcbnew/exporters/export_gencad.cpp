/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <wxPcbStruct.h>
#include <trigo.h>
#include <build_version.h>
#include <macros.h>

#include <pcbnew.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>


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
static void FootprintWriteShape( FILE* File, MODULE* module );

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

static std::string GenCADLayerName( int aCuCount, LAYER_ID aId )
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
};


static const LAYER_ID gc_seq[] = {
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
static std::string GenCADLayerNameFlipped( int aCuCount, LAYER_ID aId )
{
    if( 1<= aId && aId <= 14 )
    {
        return StrPrintf(  "INNER%d", 14 - aId );
    }

    return GenCADLayerName( aCuCount, aId );
};


#endif

static std::string fmt_mask( LSET aSet )
{
#if 0
    return aSet.FmtHex();
#else
    return StrPrintf( "%08x", (unsigned) ( aSet & LSET::AllCuMask() ).to_ulong() );
#endif
}


// These are the export origin (the auxiliary axis)
static int GencadOffsetX, GencadOffsetY;

/* GerbTool chokes on units different than INCH so this is the conversion
 *  factor */
const static double SCALE_FACTOR = 10000.0 * IU_PER_DECIMILS;


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
    wxFileName  fn = GetBoard()->GetFileName();
    FILE*       file;

    wxString    ext = wxT( "cad" );
    wxString    wildcard = _( "GenCAD 1.4 board files (.cad)|*.cad" );

    fn.SetExt( ext );

    wxString pro_dir = wxPathOnly( Prj().GetProjectFullName() );

    wxFileDialog dlg( this, _( "Save GenCAD Board File" ), pro_dir,
                      fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( ( file = wxFopen( dlg.GetPath(), wxT( "wt" ) ) ) == NULL )
    {
        wxString    msg;

        msg.Printf( _( "Unable to create <%s>" ), GetChars( dlg.GetPath() ) );
        DisplayError( this, msg ); return;
    }

    SetLocaleTo_C_standard(); // No pesky decimal separators in gencad

    // Update some board data, to ensure a reliable gencad export
    GetBoard()->ComputeBoundingBox();

    // Save the auxiliary origin for the rest of the module
    GencadOffsetX = GetAuxOrigin().x;
    GencadOffsetY = GetAuxOrigin().y;

    // No idea on *why* this should be needed... maybe to fix net names?
    Compile_Ratsnest( NULL, true );

    /* Temporary modification of footprints that are flipped (i.e. on bottom
     * layer) to convert them to non flipped footprints.
     *  This is necessary to easily export shapes to GenCAD,
     *  that are given as normal orientation (non flipped, rotation = 0))
     * these changes will be undone later
     */
    BOARD*  pcb = GetBoard();
    MODULE* module;

    for( module = pcb->m_Modules; module; module = module->Next() )
    {
        module->SetFlag( 0 );

        if( module->GetLayer() == B_Cu )
        {
            module->Flip( module->GetPosition() );
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
    SetLocaleTo_Default();  // revert to the current locale

    // Undo the footprints modifications (flipped footprints)
    for( module = pcb->m_Modules; module; module = module->Next() )
    {
        if( module->GetFlag() )
        {
            module->Flip( module->GetPosition() );
            module->SetFlag( 0 );
        }
    }
}


// Comparator for sorting pads with qsort
static int PadListSortByShape( const void* aRefptr, const void* aObjptr )
{
    const D_PAD* padref = *(D_PAD**) aRefptr;
    const D_PAD* padcmp = *(D_PAD**) aObjptr;

    return D_PAD::Compare( padref, padcmp );
}


// Sort vias for uniqueness
static int ViaSort( const void* aRefptr, const void* aObjptr )
{
    VIA* padref = *(VIA**) aRefptr;
    VIA* padcmp = *(VIA**) aObjptr;

    if( padref->GetWidth() != padcmp->GetWidth() )
        return padref->GetWidth() - padcmp->GetWidth();

    if( padref->GetDrillValue() != padcmp->GetDrillValue() )
        return padref->GetDrillValue() - padcmp->GetDrillValue();

    if( padref->GetLayerSet() != padcmp->GetLayerSet() )
        return padref->GetLayerSet().FmtBin().compare( padcmp->GetLayerSet().FmtBin() );

    return 0;
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
    std::vector<D_PAD*> pads;
    std::vector<D_PAD*> padstacks;
    std::vector<VIA*>   vias;
    std::vector<VIA*>   viastacks;

    padstacks.resize( 1 ); // We count pads from 1

    // The master layermask (i.e. the enabled layers) for padstack generation
    LSET    master_layermask = aPcb->GetDesignSettings().GetEnabledLayers();
    int     cu_count = aPcb->GetCopperLayerCount();

    fputs( "$PADS\n", aFile );

    // Enumerate and sort the pads
    if( aPcb->GetPadCount() > 0 )
    {
        pads = aPcb->GetPads();
        qsort( &pads[0], aPcb->GetPadCount(), sizeof( D_PAD* ),
               PadListSortByShape );
    }

    // The same for vias
    for( VIA* via = GetFirstVia( aPcb->m_Track ); via;
            via = GetFirstVia( via->Next() ) )
    {
        vias.push_back( via );
    }

    qsort( &vias[0], vias.size(), sizeof(VIA*), ViaSort );

    // Emit vias pads
    TRACK* old_via = 0;

    for( unsigned i = 0; i < vias.size(); i++ )
    {
        VIA* via = vias[i];

        if( old_via && 0 == ViaSort( &old_via, &via ) )
            continue;

        old_via = via;
        viastacks.push_back( via );
        fprintf( aFile, "PAD V%d.%d.%s ROUND %g\nCIRCLE 0 0 %g\n",
                via->GetWidth(), via->GetDrillValue(),
                fmt_mask( via->GetLayerSet() ).c_str(),
                via->GetDrillValue() / SCALE_FACTOR,
                via->GetWidth() / (SCALE_FACTOR * 2) );
    }

    // Emit component pads
    D_PAD* old_pad = 0;
    int    pad_name_number = 0;

    for( unsigned i = 0; i<pads.size(); ++i )
    {
        D_PAD* pad = pads[i];

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
        case PAD_SHAPE_CIRCLE:
            fprintf( aFile, " ROUND %g\n",
                     pad->GetDrillSize().x / SCALE_FACTOR );
            /* Circle is center, radius */
            fprintf( aFile, "CIRCLE %g %g %g\n",
                    pad->GetOffset().x / SCALE_FACTOR,
                    -pad->GetOffset().y / SCALE_FACTOR,
                    pad->GetSize().x / (SCALE_FACTOR * 2) );
            break;

        case PAD_SHAPE_RECT:
            fprintf( aFile, " RECTANGULAR %g\n",
                     pad->GetDrillSize().x / SCALE_FACTOR );

            // Rectangle is begin, size *not* begin, end!
            fprintf( aFile, "RECTANGLE %g %g %g %g\n",
                    (-dx + pad->GetOffset().x ) / SCALE_FACTOR,
                    (-dy - pad->GetOffset().y ) / SCALE_FACTOR,
                    dx / (SCALE_FACTOR / 2), dy / (SCALE_FACTOR / 2) );
            break;

        case PAD_SHAPE_OVAL:     // Create outline by 2 lines and 2 arcs
            {
                // OrCAD Layout call them OVAL or OBLONG - GenCAD call them FINGERs
                fprintf( aFile, " FINGER %g\n",
                         pad->GetDrillSize().x / SCALE_FACTOR );
                int dr = dx - dy;

                if( dr >= 0 )       // Horizontal oval
                {
                    int radius = dy;
                    fprintf( aFile, "LINE %g %g %g %g\n",
                             (-dr + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y - radius) / SCALE_FACTOR,
                             (dr + pad->GetOffset().x ) / SCALE_FACTOR,
                             (-pad->GetOffset().y - radius) / SCALE_FACTOR );

                    // GenCAD arcs are (start, end, center)
                    fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                             (dr + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y - radius) / SCALE_FACTOR,
                             (dr + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y + radius) / SCALE_FACTOR,
                             (dr + pad->GetOffset().x) / SCALE_FACTOR,
                             -pad->GetOffset().y / SCALE_FACTOR );

                    fprintf( aFile, "LINE %g %g %g %g\n",
                             (dr + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y + radius) / SCALE_FACTOR,
                             (-dr + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y + radius) / SCALE_FACTOR );
                    fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                             (-dr + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y + radius) / SCALE_FACTOR,
                             (-dr + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y - radius) / SCALE_FACTOR,
                             (-dr + pad->GetOffset().x) / SCALE_FACTOR,
                             -pad->GetOffset().y / SCALE_FACTOR );
                }
                else        // Vertical oval
                {
                    dr = -dr;
                    int radius = dx;
                    fprintf( aFile, "LINE %g %g %g %g\n",
                             (-radius + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y - dr) / SCALE_FACTOR,
                             (-radius + pad->GetOffset().x ) / SCALE_FACTOR,
                             (-pad->GetOffset().y + dr) / SCALE_FACTOR );
                    fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                             (-radius + pad->GetOffset().x ) / SCALE_FACTOR,
                             (-pad->GetOffset().y + dr) / SCALE_FACTOR,
                             (radius + pad->GetOffset().x ) / SCALE_FACTOR,
                             (-pad->GetOffset().y + dr) / SCALE_FACTOR,
                             pad->GetOffset().x / SCALE_FACTOR,
                             (-pad->GetOffset().y + dr) / SCALE_FACTOR );

                    fprintf( aFile, "LINE %g %g %g %g\n",
                             (radius + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y + dr) / SCALE_FACTOR,
                             (radius + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y - dr) / SCALE_FACTOR );
                    fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                             (radius + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y - dr) / SCALE_FACTOR,
                             (-radius + pad->GetOffset().x) / SCALE_FACTOR,
                             (-pad->GetOffset().y - dr) / SCALE_FACTOR,
                             pad->GetOffset().x / SCALE_FACTOR,
                             (-pad->GetOffset().y - dr) / SCALE_FACTOR );
                }
            }
            break;

        case PAD_SHAPE_TRAPEZOID:
            fprintf( aFile, " POLYGON %g\n",
                     pad->GetDrillSize().x / SCALE_FACTOR );

            // XXX TO BE IMPLEMENTED! and I don't know if it could be actually imported by something
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

        for( LSEQ seq = mask.Seq( gc_seq, DIM( gc_seq ) );  seq;  ++seq )
        {
            LAYER_ID layer = *seq;

            fprintf( aFile, "PAD V%d.%d.%s %s 0 0\n",
                    via->GetWidth(), via->GetDrillValue(),
                    fmt_mask( mask ).c_str(),
                    GenCADLayerName( cu_count, layer ).c_str()
                    );
        }
    }

    /* Component padstacks
     *  CAM350 don't apply correctly the FLIP semantics for padstacks, i.e. doesn't
     *  swap the top and bottom layers... so I need to define the shape as MIRRORX
     *  and define a separate 'flipped' padstack... until it appears yet another
     *  noncompliant importer */
    for( unsigned i = 1; i < padstacks.size(); i++ )
    {
        D_PAD* pad = padstacks[i];

        // Straight padstack
        fprintf( aFile, "PADSTACK PAD%u %g\n", i, pad->GetDrillSize().x / SCALE_FACTOR );

        LSET pad_set = pad->GetLayerSet() & master_layermask;

        // the special gc_seq
        for( LSEQ seq = pad_set.Seq( gc_seq, DIM( gc_seq ) );  seq;  ++seq )
        {
            LAYER_ID layer = *seq;

            fprintf( aFile, "PAD P%u %s 0 0\n", i, GenCADLayerName( cu_count, layer ).c_str() );
        }

        // Flipped padstack
        fprintf( aFile, "PADSTACK PAD%uF %g\n", i, pad->GetDrillSize().x / SCALE_FACTOR );

        // the normal LAYER_ID sequence is inverted from gc_seq[]
        for( LSEQ seq = pad_set.Seq();  seq;  ++seq )
        {
            LAYER_ID layer = *seq;

            fprintf( aFile, "PAD P%u %s 0 0\n", i, GenCADLayerNameFlipped( cu_count, layer ).c_str() );
        }
    }

    fputs( "$ENDPADSTACKS\n\n", aFile );
}


/* Creates the footprint shape list.
 * Since module shape is customizable after the placement we cannot share them;
 * instead we opt for the one-module-one-shape-one-component-one-device approach
 */
static void CreateShapesSection( FILE* aFile, BOARD* aPcb )
{
    MODULE*     module;
    D_PAD*      pad;
    const char* layer;
    wxString    pinname;
    const char* mirror = "0";

    fputs( "$SHAPES\n", aFile );

    const LSET all_cu = LSET::AllCuMask();

    for( module = aPcb->m_Modules; module; module = module->Next() )
    {
        FootprintWriteShape( aFile, module );

        for( pad = module->Pads(); pad; pad = pad->Next() )
        {
            /* Funny thing: GenCAD requires the pad side even if you use
             *  padstacks (which are theorically optional but gerbtools
             *requires* them). Now the trouble thing is that 'BOTTOM'
             *  is interpreted by someone as a padstack flip even
             *  if the spec explicitly says it's not... */
            layer = "ALL";

            if( ( pad->GetLayerSet() & all_cu ) == LSET( B_Cu ) )
            {
                layer = module->GetFlag() ? "TOP" : "BOTTOM";
            }
            else if( ( pad->GetLayerSet() & all_cu ) == LSET( F_Cu ) )
            {
                layer = module->GetFlag() ? "BOTTOM" : "TOP";
            }

            pad->StringPadName( pinname );

            if( pinname.IsEmpty() )
                pinname = wxT( "none" );

            double orient = pad->GetOrientation() - module->GetOrientation();
            NORMALIZE_ANGLE_POS( orient );

            // Bottom side modules use the flipped padstack
            fprintf( aFile, (module->GetFlag()) ?
                     "PIN %s PAD%dF %g %g %s %g %s\n" :
                     "PIN %s PAD%d %g %g %s %g %s\n",
                     TO_UTF8( pinname ), pad->GetSubRatsnest(),
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

    for( MODULE* module = aPcb->m_Modules; module; module = module->Next() )
    {
        const char*   mirror;
        const char*   flip;
        double        orient = module->GetOrientation();

        if( module->GetFlag() )
        {
            mirror = "0";
            flip   = "FLIP";
            NEGATE_AND_NORMALIZE_ANGLE_POS( orient );
        }
        else
        {
            mirror = "0";
            flip   = "0";
        }

        fprintf( aFile, "\nCOMPONENT %s\n",
                 TO_UTF8( module->GetReference() ) );
        fprintf( aFile, "DEVICE %s_%s\n",
                 TO_UTF8( module->GetReference() ),
                 TO_UTF8( module->GetValue() ) );
        fprintf( aFile, "PLACE %g %g\n",
                 MapXTo( module->GetPosition().x ),
                 MapYTo( module->GetPosition().y ) );
        fprintf( aFile, "LAYER %s\n",
                 (module->GetFlag()) ? "BOTTOM" : "TOP" );
        fprintf( aFile, "ROTATION %g\n",
                 orient / 10.0 );
        fprintf( aFile, "SHAPE %s %s %s\n",
                 TO_UTF8( module->GetReference() ),
                 mirror, flip );

        // Text on silk layer: RefDes and value (are they actually useful?)
        TEXTE_MODULE *textmod = &module->Reference();

        for( int ii = 0; ii < 2; ii++ )
        {
            double      orient = textmod->GetOrientation();
            std::string layer  = GenCADLayerName( cu_count, module->GetFlag() ? B_SilkS : F_SilkS );

            fprintf( aFile, "TEXT %g %g %g %g %s %s \"%s\"",
                     textmod->GetPos0().x / SCALE_FACTOR,
                    -textmod->GetPos0().y / SCALE_FACTOR,
                     textmod->GetSize().x / SCALE_FACTOR,
                     orient / 10.0,
                     mirror,
                     layer.c_str(),
                     TO_UTF8( textmod->GetText() ) );

            // Please note, the width is approx
            fprintf( aFile, " 0 0 %g %g\n",
                     ( textmod->GetSize().x * textmod->GetLength() ) / SCALE_FACTOR,
                     textmod->GetSize().y / SCALE_FACTOR );

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
    D_PAD*        pad;
    MODULE*       module;
    int           NbNoConn = 1;

    fputs( "$SIGNALS\n", aFile );

    for( unsigned ii = 0; ii < aPcb->GetNetCount(); ii++ )
    {
        net = aPcb->FindNet( ii );

        if( net->GetNetname() == wxEmptyString ) // dummy netlist (no connection)
        {
            wxString msg; msg << wxT( "NoConnection" ) << NbNoConn++;
        }

        if( net->GetNet() <= 0 )  // dummy netlist (no connection)
            continue;

        msg = wxT( "SIGNAL " ) + net->GetNetname();

        fputs( TO_UTF8( msg ), aFile );
        fputs( "\n", aFile );

        for( module = aPcb->m_Modules; module; module = module->Next() )
        {
            for( pad = module->Pads(); pad; pad = pad->Next() )
            {
                wxString padname;

                if( pad->GetNetCode() != net->GetNet() )
                    continue;

                pad->StringPadName( padname );
                msg.Printf( wxT( "NODE %s %s" ),
                            GetChars( module->GetReference() ),
                            GetChars( padname ) );

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

    msg.Printf( wxT( "ORIGIN %g %g\n" ),
                MapXTo( aFrame->GetAuxOrigin().x ),
                MapYTo( aFrame->GetAuxOrigin().y ) );
    fputs( TO_UTF8( msg ), aFile );

    fputs( "INTERTRACK 0\n", aFile );
    fputs( "$ENDHEADER\n\n", aFile );

    return true;
}


/*
 *  Sort function used to sort tracks segments:
 *   items are sorted by netcode, then by width then by layer
 */
static int TrackListSortByNetcode( const void* refptr, const void* objptr )
{
    const TRACK* ref, * cmp;
    int          diff;

    ref = *( (TRACK**) refptr );
    cmp = *( (TRACK**) objptr );

    if( ( diff = ref->GetNetCode() - cmp->GetNetCode() ) )
        return diff;

    if( ( diff = ref->GetWidth() - cmp->GetWidth() ) )
        return diff;

    if( ( diff = ref->GetLayer() - cmp->GetLayer() ) )
        return diff;

    return 0;
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
    TRACK*  track, ** tracklist;
    int     vianum = 1;
    int     old_netcode, old_width, old_layer;
    int     nbitems, ii;
    LSET    master_layermask = aPcb->GetDesignSettings().GetEnabledLayers();

    int     cu_count = aPcb->GetCopperLayerCount();

    // Count items
    nbitems = 0;

    for( track = aPcb->m_Track; track; track = track->Next() )
        nbitems++;

    for( track = aPcb->m_Zone; track; track = track->Next() )
    {
        if( track->Type() == PCB_ZONE_T )
            nbitems++;
    }

    tracklist = (TRACK**) operator new( (nbitems + 1)* sizeof( TRACK* ) );

    nbitems = 0;

    for( track = aPcb->m_Track; track; track = track->Next() )
        tracklist[nbitems++] = track;

    for( track = aPcb->m_Zone; track; track = track->Next() )
    {
        if( track->Type() == PCB_ZONE_T )
            tracklist[nbitems++] = track;
    }

    tracklist[nbitems] = NULL;

    qsort( tracklist, nbitems, sizeof(TRACK*), TrackListSortByNetcode );

    fputs( "$ROUTES\n", aFile );

    old_netcode = -1; old_width = -1; old_layer = -1;

    for( ii = 0; ii < nbitems; ii++ )
    {
        track = tracklist[ii];

        if( old_netcode != track->GetNetCode() )
        {
            old_netcode = track->GetNetCode();
            NETINFO_ITEM* net = track->GetNet();
            wxString      netname;

            if( net && (net->GetNetname() != wxEmptyString) )
                netname = net->GetNetname();
            else
                netname = wxT( "_noname_" );

            fprintf( aFile, "ROUTE %s\n", TO_UTF8( netname ) );
        }

        if( old_width != track->GetWidth() )
        {
            old_width = track->GetWidth();
            fprintf( aFile, "TRACK TRACK%d\n", track->GetWidth() );
        }

        if( (track->Type() == PCB_TRACE_T) || (track->Type() == PCB_ZONE_T) )
        {
            if( old_layer != track->GetLayer() )
            {
                old_layer = track->GetLayer();
                fprintf( aFile, "LAYER %s\n",
                        GenCADLayerName( cu_count, track->GetLayer() ).c_str()
                        );
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

    delete tracklist;
}


/* Creates the section $DEVICES
 * This is a list of footprints properties
 *  ( Shapes are in section $SHAPE )
 */
static void CreateDevicesSection( FILE* aFile, BOARD* aPcb )
{
    MODULE* module;

    fputs( "$DEVICES\n", aFile );

    for( module = aPcb->m_Modules; module; module = module->Next() )
    {
        fprintf( aFile, "DEVICE \"%s\"\n", TO_UTF8( module->GetReference() ) );
        fprintf( aFile, "PART \"%s\"\n", TO_UTF8( module->GetValue() ) );
        fprintf( aFile, "PACKAGE \"%s\"\n", module->GetFPID().Format().c_str() );

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
    for( EDA_ITEM* drawing = aPcb->m_Drawings; drawing != 0;
        drawing = drawing->Next() )
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
    TRACK* track;
    int    last_width = -1;

    // Find thickness used for traces
    // XXX could use the same sorting approach used for pads

    std::vector <int> trackinfo;

    unsigned          ii;

    for( track = aPcb->m_Track; track; track = track->Next() )
    {
        if( last_width != track->GetWidth() ) // Find a thickness already used.
        {
            for( ii = 0; ii < trackinfo.size(); ii++ )
            {
                if( trackinfo[ii] == track->GetWidth() )
                    break;
            }

            if( ii == trackinfo.size() )    // not found
                trackinfo.push_back( track->GetWidth() );

            last_width = track->GetWidth();
        }
    }

    for( track = aPcb->m_Zone; track; track = track->Next() )
    {
        if( last_width != track->GetWidth() ) // Find a thickness already used.
        {
            for( ii = 0; ii < trackinfo.size(); ii++ )
            {
                if( trackinfo[ii] == track->GetWidth() )
                    break;
            }

            if( ii == trackinfo.size() )    // not found
                trackinfo.push_back( track->GetWidth() );

            last_width = track->GetWidth();
        }
    }

    // Write data
    fputs( "$TRACKS\n", aFile );

    for( ii = 0; ii < trackinfo.size(); ii++ )
    {
        fprintf( aFile, "TRACK TRACK%d %g\n", trackinfo[ii],
                 trackinfo[ii] / SCALE_FACTOR );
    }

    fputs( "$ENDTRACKS\n\n", aFile );
}


/* Creates the shape of a footprint (section SHAPE)
 *  The shape is always given "normal" (Orient 0, not mirrored)
 * It's almost guaranteed that the silk layer will be imported wrong but
 * the shape also contains the pads!
 */
static void FootprintWriteShape( FILE* aFile, MODULE* module )
{
    EDGE_MODULE* PtEdge;
    EDA_ITEM*    PtStruct;

    // Control Y axis change sign for flipped modules
    int          Yaxis_sign = -1;

    // Flip for bottom side components
    if( module->GetFlag() )
        Yaxis_sign = 1;

    /* creates header: */
    fprintf( aFile, "\nSHAPE %s\n", TO_UTF8( module->GetReference() ) );

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
    for( PtStruct = module->GraphicalItems(); PtStruct; PtStruct = PtStruct->Next() )
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
                             (PtEdge->m_Start0.x) / SCALE_FACTOR,
                             (Yaxis_sign * PtEdge->m_Start0.y) / SCALE_FACTOR,
                             (PtEdge->m_End0.x) / SCALE_FACTOR,
                             (Yaxis_sign * PtEdge->m_End0.y ) / SCALE_FACTOR );
                    break;

                case S_CIRCLE:
                {
                    int radius = KiROUND( GetLineLength( PtEdge->m_End0,
                                                         PtEdge->m_Start0 ) );
                    fprintf( aFile, "CIRCLE %g %g %g\n",
                             PtEdge->m_Start0.x / SCALE_FACTOR,
                             Yaxis_sign * PtEdge->m_Start0.y / SCALE_FACTOR,
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
                    if( Yaxis_sign == -1 )
                    {
                        // Flipping Y flips the arc direction too
                        fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                                 (arcendx) / SCALE_FACTOR,
                                 (Yaxis_sign * arcendy) / SCALE_FACTOR,
                                 (PtEdge->m_End0.x) / SCALE_FACTOR,
                                 (Yaxis_sign * PtEdge->GetEnd0().y) / SCALE_FACTOR,
                                 (PtEdge->GetStart0().x) / SCALE_FACTOR,
                                 (Yaxis_sign * PtEdge->GetStart0().y) / SCALE_FACTOR );
                    }
                    else
                    {
                        fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                                 (PtEdge->GetEnd0().x) / SCALE_FACTOR,
                                 (Yaxis_sign * PtEdge->GetEnd0().y) / SCALE_FACTOR,
                                 (arcendx) / SCALE_FACTOR,
                                 (Yaxis_sign * arcendy) / SCALE_FACTOR,
                                 (PtEdge->GetStart0().x) / SCALE_FACTOR,
                                 (Yaxis_sign * PtEdge->GetStart0().y) / SCALE_FACTOR );
                    }
                    break;
                }

                default:
                    DisplayError( NULL, wxT( "Type Edge Module invalid." ) );
                    break;
                }
            }
            break;

        default:
            break;
        }
    }
}
