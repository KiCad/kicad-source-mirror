/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "appl_wxstruct.h"
#include "wxPcbStruct.h"
#include "trigo.h"
#include "build_version.h"
#include "macros.h"

#include "pcbnew.h"

#include "class_board.h"
#include "class_module.h"
#include "class_track.h"
#include "class_edge_mod.h"


static bool CreateHeaderInfoData( FILE* aFile, PCB_EDIT_FRAME* frame );
static void CreateArtworksSection( FILE* aFile);
static void CreateTracksInfoData( FILE* aFile, BOARD* aPcb );
static void CreateBoardSection( FILE* aFile, BOARD* aPcb );
static void CreateComponentsSection( FILE* aFile, BOARD* aPcb );
static void CreateDevicesSection( FILE* aFile, BOARD* aPcb );
static void CreateRoutesSection( FILE* aFile, BOARD* aPcb );
static void CreateSignalsSection( FILE* aFile, BOARD* aPcb );
static void CreateShapesSection( FILE* aFile, BOARD* aPcb );
static void CreatePadsShapesSection( FILE* aFile, BOARD* aPcb );
static void FootprintWriteShape( FILE* File, MODULE* module );

// layer name for Gencad export
static const wxString GenCADLayerName[32] =
{
    wxT( "BOTTOM" ),             wxT( "INNER1" ),            wxT( "INNER2" ),
    wxT( "INNER3" ),             wxT( "INNER4" ),            wxT( "INNER5" ),
    wxT( "INNER6" ),             wxT( "INNER7" ),            wxT( "INNER8" ),
    wxT( "INNER9" ),             wxT( "INNER10" ),           wxT( "INNER11" ),
    wxT( "INNER12" ),            wxT( "INNER13" ),           wxT( "INNER14" ),
    wxT( "TOP" ),                wxT( "LAYER17" ),           wxT( "LAYER18" ),
    wxT( "SOLDERPASTE_BOTTOM" ), wxT( "SOLDERPASTE_TOP" ),
    wxT( "SILKSCREEN_BOTTOM" ),  wxT( "SILKSCREEN_TOP" ),
    wxT( "SOLDERMASK_BOTTOM" ),  wxT( "SOLDERMASK_TOP" ),    wxT( "LAYER25" ),
    wxT( "LAYER26" ),            wxT( "LAYER27" ),           wxT( "LAYER28" ),
    wxT( "LAYER29" ),            wxT( "LAYER30" ),           wxT( "LAYER31" ),
    wxT( "LAYER32" )
};

// flipped layer name for Gencad export (to make CAM350 imports correct)
static const wxString GenCADLayerNameFlipped[32] =
{
    wxT( "TOP" ),                wxT( "INNER14" ),           wxT( "INNER13" ),
    wxT( "INNER12" ),            wxT( "INNER11" ),           wxT( "INNER10" ),
    wxT( "INNER9" ),             wxT( "INNER8" ),            wxT( "INNER7" ),
    wxT( "INNER6" ),             wxT( "INNER5" ),            wxT( "INNER4" ),
    wxT( "INNER3" ),             wxT( "INNER2" ),            wxT( "INNER1" ),
    wxT( "BOTTOM" ),             wxT( "LAYER17" ),           wxT( "LAYER18" ),
    wxT( "SOLDERPASTE_TOP" ),    wxT( "SOLDERPASTE_BOTTOM" ),
    wxT( "SILKSCREEN_TOP" ),     wxT( "SILKSCREEN_BOTTOM" ),
    wxT( "SOLDERMASK_TOP" ),  	 wxT( "SOLDERMASK_BOTTOM" ), wxT( "LAYER25" ),
    wxT( "LAYER26" ),            wxT( "LAYER27" ),           wxT( "LAYER28" ),
    wxT( "LAYER29" ),            wxT( "LAYER30" ),           wxT( "LAYER31" ),
    wxT( "LAYER32" )
};

// These are the export origin (the auxiliary axis)
static int GencadOffsetX, GencadOffsetY;

/* GerbTool chokes on units different than INCH so this is the conversion
   factor */
const static double SCALE_FACTOR = 10000.0;


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
    wxFileName fn = GetScreen()->GetFileName();
    wxString   msg, ext, wildcard;
    FILE*      file;

    ext = wxT( "cad" );
    wildcard = _( "GenCAD 1.4 board files (.cad)|*.cad" );
    fn.SetExt( ext );

    wxFileDialog dlg( this, _( "Save GenCAD Board File" ), wxGetCwd(),
                      fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( ( file = wxFopen( dlg.GetPath(), wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Unable to create " ) + dlg.GetPath();
        DisplayError( this, msg ); return;
    }

    SetLocaleTo_C_standard(); // No pesky decimal separators in gencad

    // Update some board data, to ensure a reliable gencad export
    GetBoard()->ComputeBoundingBox();

    // Save the auxiliary origin for the rest of the module
    GencadOffsetX = m_Auxiliary_Axis_Position.x;
    GencadOffsetY = m_Auxiliary_Axis_Position.y;

    // No idea on *why* this should be needed... maybe to fix net names?
    Compile_Ratsnest( NULL, true );

    /* Temporary modification of footprints that are flipped (i.e. on bottom
     * layer) to convert them to non flipped footprints.
     *  This is necessary to easily export shapes to GenCAD,
     *  that are given as normal orientation (non flipped, rotation = 0))
     * these changes will be undone later
     */
    BOARD* pcb = GetBoard();
    MODULE* module;

    for( module = pcb->m_Modules; module != NULL; module = module->Next() )
    {
        module->flag = 0;

        if( module->GetLayer() == LAYER_N_BACK )
        {
            module->Flip( module->m_Pos );
            module->flag = 1;
        }
    }

    /* Gencad has some mandatory and some optional sections: some importer
       need the padstack section (which is optional) anyway. Also the
       order of the section *is* important */

    CreateHeaderInfoData( file, this ); // Gencad header
    CreateBoardSection( file, pcb ); // Board perimeter

    CreatePadsShapesSection( file, pcb ); // Pads and padstacks
    CreateArtworksSection( file ); // Empty but mandatory

    /* Gencad splits a component info in shape, component and device.
       We don't do any sharing (it would be difficult since each module is
       customizable after placement) */
    CreateShapesSection( file, pcb );
    CreateComponentsSection( file, pcb );
    CreateDevicesSection( file, pcb );

    // In a similar way the netlist is split in net, track and route
    CreateSignalsSection( file, pcb );
    CreateTracksInfoData( file, pcb );
    CreateRoutesSection( file, pcb );

    fclose( file );
    SetLocaleTo_Default();	// revert to the current locale

    // Undo the footprints modifications (flipped footprints)
    for( module = pcb->m_Modules; module != NULL; module = module->Next() )
    {
        if( module->flag )
        {
            module->Flip( module->m_Pos );
            module->flag = 0;
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
static int ViaSort(const void* aRefptr, const void* aObjptr )
{
    TRACK* padref = *(TRACK**)aRefptr;
    TRACK* padcmp = *(TRACK**)aObjptr;

    if( padref->m_Width != padcmp->m_Width )
    return padref->m_Width-padcmp->m_Width;

    if( padref->GetDrillValue() != padcmp->GetDrillValue() )
    return padref->GetDrillValue()-padcmp->GetDrillValue();

    if( padref->ReturnMaskLayer() != padcmp->ReturnMaskLayer() )
    return padref->ReturnMaskLayer()-padcmp->ReturnMaskLayer();

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
    std::vector<TRACK*> vias;
    std::vector<TRACK*> viastacks;
    padstacks.resize(1); // We count pads from 1

    // The master layermask (i.e. the enabled layers) for padstack generation
    unsigned master_layermask = aPcb->GetDesignSettings().GetEnabledLayers();

    fputs( "$PADS\n", aFile );

    // Enumerate and sort the pads
    if( aPcb->GetPadsCount() > 0 )
    {
        pads.insert( pads.end(),
                     aPcb->m_NetInfo->m_PadsFullList.begin(),
                     aPcb->m_NetInfo->m_PadsFullList.end() );
        qsort( &pads[0], aPcb->GetPadsCount(), sizeof( D_PAD* ),
        PadListSortByShape );
    }

    // The same for vias
    for( TRACK* track = aPcb->m_Track; track != NULL; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
    {
        vias.push_back( track );
    }
    }
    qsort( &vias[0], vias.size(), sizeof(TRACK*), ViaSort );

    // Emit vias pads
    TRACK* old_via = 0;
    for( unsigned i = 0; i < vias.size(); i++ )
    {
    TRACK* via = vias[i];
    if (old_via && 0 == ViaSort(&old_via, &via))
        continue;

    old_via = via;
    viastacks.push_back(via);
    fprintf( aFile, "PAD V%d.%d.%X ROUND %g\nCIRCLE 0 0 %g\n",
        via->m_Width,via->GetDrillValue(),
        via->ReturnMaskLayer(),
        via->GetDrillValue()/SCALE_FACTOR,
        via->m_Width/(SCALE_FACTOR*2) );
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

    padstacks.push_back(pad); // Will have its own padstack later
        int dx = pad->m_Size.x / 2;
        int dy = pad->m_Size.y / 2;

        switch( pad->m_PadShape )
        {
        default:
        case PAD_CIRCLE:
            fprintf( aFile, " ROUND %g\n",
            pad->m_Drill.x / SCALE_FACTOR );
        /* Circle is center, radius */
            fprintf( aFile, "CIRCLE %g %g %g\n",
            pad->m_Offset.x / SCALE_FACTOR,
            -pad->m_Offset.y / SCALE_FACTOR,
            pad->m_Size.x / (SCALE_FACTOR*2) );
            break;

        case PAD_RECT:
            fprintf( aFile, " RECTANGULAR %g\n",
                    pad->m_Drill.x / SCALE_FACTOR );
        // Rectangle is begin, size *not* begin, end!
            fprintf( aFile, "RECTANGLE %g %g %g %g\n",
                    (-dx + pad->m_Offset.x ) / SCALE_FACTOR,
                    (-dy - pad->m_Offset.y ) / SCALE_FACTOR,
                    dx / (SCALE_FACTOR/2) , dy / (SCALE_FACTOR/2) );
            break;

        case PAD_OVAL:     // Create outline by 2 lines and 2 arcs
        {
        // OrCAD Layout call them OVAL or OBLONG - GenCAD call them FINGERs
            fprintf( aFile, " FINGER %g\n",
                    pad->m_Drill.x / SCALE_FACTOR );
            int dr = dx - dy;

            if( dr >= 0 )       // Horizontal oval
            {
                int radius = dy;
                fprintf( aFile, "LINE %g %g %g %g\n",
            (-dr + pad->m_Offset.x) / SCALE_FACTOR,
            (-pad->m_Offset.y - radius) / SCALE_FACTOR,
            (dr + pad->m_Offset.x ) / SCALE_FACTOR,
            (-pad->m_Offset.y - radius) / SCALE_FACTOR );
        // GenCAD arcs are (start, end, center)
                fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                        (dr + pad->m_Offset.x) / SCALE_FACTOR,
                        (-pad->m_Offset.y - radius) / SCALE_FACTOR,
                        (dr + pad->m_Offset.x) / SCALE_FACTOR,
                        (-pad->m_Offset.y + radius) / SCALE_FACTOR,
                        (dr + pad->m_Offset.x) / SCALE_FACTOR,
                        -pad->m_Offset.y / SCALE_FACTOR );

                fprintf( aFile, "LINE %g %g %g %g\n",
                        (dr + pad->m_Offset.x) / SCALE_FACTOR,
                        (-pad->m_Offset.y + radius) / SCALE_FACTOR,
                        (-dr + pad->m_Offset.x) / SCALE_FACTOR,
                        (-pad->m_Offset.y + radius) / SCALE_FACTOR );
                fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                        (-dr + pad->m_Offset.x) / SCALE_FACTOR,
                        (-pad->m_Offset.y + radius) / SCALE_FACTOR,
                        (-dr + pad->m_Offset.x) / SCALE_FACTOR,
                        (-pad->m_Offset.y - radius) / SCALE_FACTOR,
                        (-dr + pad->m_Offset.x) / SCALE_FACTOR,
                        -pad->m_Offset.y / SCALE_FACTOR );
            }
            else        // Vertical oval
            {
                dr = -dr;
                int radius = dx;
                fprintf( aFile, "LINE %g %g %g %g\n",
                        (-radius + pad->m_Offset.x) / SCALE_FACTOR,
                        (-pad->m_Offset.y - dr) / SCALE_FACTOR,
                        (-radius + pad->m_Offset.x ) / SCALE_FACTOR,
                        (-pad->m_Offset.y + dr) / SCALE_FACTOR );
                fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                        (-radius + pad->m_Offset.x ) / SCALE_FACTOR,
                        (-pad->m_Offset.y + dr) / SCALE_FACTOR,
                        (radius + pad->m_Offset.x ) / SCALE_FACTOR,
                        (-pad->m_Offset.y + dr) / SCALE_FACTOR,
                        pad->m_Offset.x / SCALE_FACTOR,
                        (-pad->m_Offset.y + dr) / SCALE_FACTOR );

                fprintf( aFile, "LINE %g %g %g %g\n",
                        (radius + pad->m_Offset.x) / SCALE_FACTOR,
                        (-pad->m_Offset.y + dr) / SCALE_FACTOR,
                        (radius + pad->m_Offset.x) / SCALE_FACTOR,
                        (-pad->m_Offset.y - dr) / SCALE_FACTOR );
                fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                         (radius + pad->m_Offset.x) / SCALE_FACTOR,
             (-pad->m_Offset.y - dr) / SCALE_FACTOR,
                         (-radius + pad->m_Offset.x) / SCALE_FACTOR,
             (-pad->m_Offset.y - dr) / SCALE_FACTOR,
                         pad->m_Offset.x / SCALE_FACTOR,
             (-pad->m_Offset.y - dr) / SCALE_FACTOR );
            }
            break;
        }

        case PAD_TRAPEZOID:
            fprintf( aFile, " POLYGON %g\n",
                    pad->m_Drill.x / SCALE_FACTOR );
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
    TRACK *via = viastacks[i];
    unsigned mask = via->ReturnMaskLayer() & master_layermask;
    fprintf( aFile, "PADSTACK VIA%d.%d.%X %g\n",
        via->m_Width, via->GetDrillValue(), mask,
        via->GetDrillValue() / SCALE_FACTOR );

    for( int layer = 0; layer < 32; layer++)
    {
        if( mask & (1<<layer) )
        {
        fprintf( aFile, "PAD V%d.%d.%X %s 0 0\n",
            via->m_Width, via->GetDrillValue(),
            mask,
            TO_UTF8( GenCADLayerName[layer]) );
        }
}
    }
    /* Component padstacks
       CAM350 don't apply correctly the FLIP semantics for padstacks, i.e. doesn't
       swap the top and bottom layers... so I need to define the shape as MIRRORX
       and define a separate 'flipped' padstack... until it appears yet another
       noncompliant importer */
    for( unsigned i = 1; i < padstacks.size(); i++ )
    {
    D_PAD *pad = padstacks[i];

    // Straight padstack
    fprintf( aFile, "PADSTACK PAD%d %g\n", i,
                pad->m_Drill.x / SCALE_FACTOR);
    for( int layer = 0; layer < 32; layer++ )
    {
        if( pad->m_layerMask & (1<<layer) & master_layermask )
        {
        fprintf( aFile, "PAD P%d %s 0 0\n", i,
            TO_UTF8( GenCADLayerName[layer] ) );
        }
    }

    // Flipped padstack
    fprintf( aFile, "PADSTACK PAD%dF %g\n", i,
                pad->m_Drill.x / SCALE_FACTOR);
    for( int layer = 0; layer < 32; layer++ )
    {
        if( pad->m_layerMask & (1<<layer) & master_layermask )
{
        fprintf( aFile, "PAD P%d %s 0 0\n", i,
            TO_UTF8( GenCADLayerNameFlipped[layer] ) );
        }
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
    int         orient;
    wxString    pinname;
    const char* mirror = "0";

    fputs( "$SHAPES\n", aFile );

    for( module = aPcb->m_Modules; module != NULL; module = module->Next() )
    {
        FootprintWriteShape( aFile, module );

        for( pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
        /* Funny thing: GenCAD requires the pad side even if you use
           padstacks (which are theorically optional but gerbtools
           *requires* them). Now the trouble thing is that 'BOTTOM'
           is interpreted by someone as a padstack flip even
           if the spec explicitly says it's not... */
            layer = "ALL";

            if( ( pad->m_layerMask & ALL_CU_LAYERS ) == LAYER_BACK )
            {
        layer = ( module->flag ) ? "TOP" : "BOTTOM";
            }
            else if( ( pad->m_layerMask & ALL_CU_LAYERS ) == LAYER_FRONT )
            {
        layer = ( module->flag ) ? "BOTTOM" : "TOP";
            }

            pad->ReturnStringPadName( pinname );

            if( pinname.IsEmpty() )
                pinname = wxT( "none" );

            orient = pad->m_Orient - module->m_Orient;
            NORMALIZE_ANGLE_POS( orient );

        // Bottom side modules use the flipped padstack
            fprintf( aFile, (module->flag) ?
            "PIN %s PAD%dF %g %g %s %g %s\n" :
            "PIN %s PAD%d %g %g %s %g %s\n",
                    TO_UTF8( pinname ), pad->GetSubRatsnest(),
                    pad->m_Pos0.x / SCALE_FACTOR,
                    -pad->m_Pos0.y / SCALE_FACTOR,
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

    for(MODULE* module = aPcb->m_Modules ; module != NULL; module = module->Next() )
    {
    TEXTE_MODULE* textmod;
    const char*   mirror;
    const char*   flip;
        int orient = module->m_Orient;

        if( module->flag )
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
        TO_UTF8( module->m_Reference->m_Text ) );
        fprintf( aFile, "DEVICE %s_%s\n",
        TO_UTF8( module->m_Reference->m_Text ),
        TO_UTF8( module->m_Value->m_Text ) );
        fprintf( aFile, "PLACE %g %g\n",
        MapXTo( module->m_Pos.x ),
        MapYTo( module->m_Pos.y ) );
        fprintf( aFile, "LAYER %s\n",
        (module->flag) ? "BOTTOM" : "TOP" );
        fprintf( aFile, "ROTATION %g\n",
        orient / 10.0 );
        fprintf( aFile, "SHAPE %s %s %s\n",
        TO_UTF8( module->m_Reference->m_Text ),
        mirror, flip );

        // Text on silk layer: RefDes and value (are they actually useful?)
        textmod = module->m_Reference;

        for( int ii = 0; ii < 2; ii++ )
        {
            int      orient = textmod->m_Orient;
            wxString layer  = GenCADLayerName[(module->flag) ?
        SILKSCREEN_N_BACK : SILKSCREEN_N_FRONT];

            fprintf( aFile, "TEXT %g %g %g %g %s %s \"%s\"",
                     textmod->m_Pos0.x / SCALE_FACTOR,
                     -textmod->m_Pos0.y / SCALE_FACTOR,
                     textmod->m_Size.x / SCALE_FACTOR,
                     orient / 10.0,
                     mirror,
                     TO_UTF8( layer ),
                     TO_UTF8( textmod->m_Text ) );

        // Please note, the width is approx
            fprintf( aFile, " 0 0 %g %g\n",
                     (textmod->m_Size.x * textmod->m_Text.Len())
             / SCALE_FACTOR,
                     textmod->m_Size.y / SCALE_FACTOR );

            textmod = module->m_Value; // Dirty trick for the second iteration
        }

        // The SHEET is a 'generic description' for referencing the component
        fprintf( aFile, "SHEET \"RefDes: %s, Value: %s\"\n",
                 TO_UTF8( module->m_Reference->m_Text ),
                 TO_UTF8( module->m_Value->m_Text ) );
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

    for( unsigned ii = 0; ii < aPcb->m_NetInfo->GetCount(); ii++ )
    {
        net = aPcb->m_NetInfo->GetNetItem( ii );

        if( net->GetNetname() == wxEmptyString ) // dummy netlist (no connection)
        {
            wxString msg; msg << wxT( "NoConnection" ) << NbNoConn++;
            net->SetNetname( msg );
        }

        if( net->GetNet() <= 0 )  // dummy netlist (no connection)
            continue;

        msg = wxT( "SIGNAL " ) + net->GetNetname();

        fputs( TO_UTF8( msg ), aFile );
        fputs( "\n", aFile );

        for( module = aPcb->m_Modules; module != NULL; module = module->Next() )
        {
            for( pad = module->m_Pads; pad != NULL; pad = pad->Next() )
            {
                wxString padname;

                if( pad->GetNet() != net->GetNet() )
                    continue;

                pad->ReturnStringPadName( padname );
                msg.Printf( wxT( "NODE %s %s" ),
                            GetChars( module->m_Reference->m_Text ),
                            GetChars( padname ) );

                fputs( TO_UTF8( msg ), aFile );
                fputs( "\n", aFile );
            }
        }
    }

    fputs( "$ENDSIGNALS\n\n", aFile );
}


/* Creates the header section; some of the data come from the frame
 * (actually the screen), not from the pcb */
static bool CreateHeaderInfoData( FILE* aFile, PCB_EDIT_FRAME* aFrame )
{
    wxString    msg;
    PCB_SCREEN* screen = (PCB_SCREEN*) ( aFrame->GetScreen() );

    fputs( "$HEADER\n", aFile );
    fputs( "GENCAD 1.4\n", aFile );

    // Please note: GenCAD syntax requires quoted strings if they can contain spaces
    msg.Printf( wxT( "USER \"%s %s\"\n" ),
        GetChars( wxGetApp().GetAppName() ),
        GetChars( GetBuildVersion() ) );
    fputs( TO_UTF8( msg ), aFile );
    msg = wxT( "DRAWING \"" ) + screen->GetFileName() + wxT( "\"\n" );
    fputs( TO_UTF8( msg ), aFile );
    msg = wxT( "REVISION \"" ) + screen->m_Revision + wxT( " " ) +
    screen->m_Date + wxT( "\"\n" );
    fputs( TO_UTF8( msg ), aFile );
    fputs( "UNITS INCH\n", aFile);
    msg.Printf( wxT( "ORIGIN %g %g\n" ),
        MapXTo( aFrame->m_Auxiliary_Axis_Position.x ),
        MapYTo( aFrame->m_Auxiliary_Axis_Position.y ) );
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

    if( ( diff = ref->GetNet() - cmp->GetNet() ) )
        return diff;

    if( ( diff = ref->m_Width - cmp->m_Width ) )
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
    TRACK* track, ** tracklist;
    int    vianum = 1;
    int    old_netcode, old_width, old_layer;
    int    nbitems, ii;
    unsigned master_layermask = aPcb->GetDesignSettings().GetEnabledLayers();

    // Count items
    nbitems = 0;

    for( track = aPcb->m_Track; track != NULL; track = track->Next() )
        nbitems++;

    for( track = aPcb->m_Zone; track != NULL; track = track->Next() )
    {
        if( track->Type() == PCB_ZONE_T )
            nbitems++;
    }

    tracklist = (TRACK**) operator new( (nbitems + 1) * sizeof( TRACK* ) );

    nbitems = 0;

    for( track = aPcb->m_Track; track != NULL; track = track->Next() )
        tracklist[nbitems++] = track;

    for( track = aPcb->m_Zone; track != NULL; track = track->Next() )
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

        if( old_netcode != track->GetNet() )
        {
            old_netcode = track->GetNet();
            NETINFO_ITEM* net = aPcb->FindNet( track->GetNet() );
            wxString      netname;

            if( net && (net->GetNetname() != wxEmptyString) )
                netname = net->GetNetname();
            else
                netname = wxT( "_noname_" );

            fprintf( aFile, "ROUTE %s\n", TO_UTF8( netname ) );
        }

        if( old_width != track->m_Width )
        {
            old_width = track->m_Width;
            fprintf( aFile, "TRACK TRACK%d\n", track->m_Width );
        }

        if( (track->Type() == PCB_TRACE_T) || (track->Type() == PCB_ZONE_T) )
        {
            if( old_layer != track->GetLayer() )
            {
                old_layer = track->GetLayer();
                fprintf( aFile, "LAYER %s\n",
                         TO_UTF8( GenCADLayerName[track->GetLayer() & 0x1F] ) );
            }

            fprintf( aFile, "LINE %g %g %g %g\n",
                     MapXTo( track->m_Start.x ), MapYTo( track->m_Start.y ),
                     MapXTo( track->m_End.x ), MapYTo( track->m_End.y ) );
        }
        if( track->Type() == PCB_VIA_T )
        {
            fprintf( aFile, "VIA VIA%d.%d.%X %g %g ALL %g via%d\n",
                     track->m_Width,track->GetDrillValue(),
                     track->ReturnMaskLayer() & master_layermask,
                     MapXTo( track->m_Start.x ), MapYTo( track->m_Start.y ),
                     track->GetDrillValue()/SCALE_FACTOR, vianum++ );
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

    for( module = aPcb->m_Modules; module != NULL; module = module->Next() )
        {
        fprintf( aFile, "DEVICE \"%s\"\n", TO_UTF8( module->m_Reference->m_Text ) );
        fprintf( aFile, "PART \"%s\"\n", TO_UTF8( module->m_Value->m_Text ) );
    fprintf( aFile, "PACKAGE \"%s\"\n", TO_UTF8( module->m_LibRef ) );
    // The TYPE attribute is almost freeform
    const char *ty = "TH";
        if( module->m_Attributs & MOD_CMS )
            ty = "SMD";
        if( module->m_Attributs & MOD_VIRTUAL )
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
    for (EDA_ITEM* drawing = aPcb->m_Drawings;
        drawing != 0;
        drawing = drawing->Next() )
    {
    if ( drawing->Type() == PCB_LINE_T )
    {
        DRAWSEGMENT *drawseg = dynamic_cast<DRAWSEGMENT*>(drawing);
        if (drawseg->GetLayer() == EDGE_N)
        {
        // XXX GenCAD supports arc boundaries but I've seen nothing that reads them
        fprintf( aFile, "LINE %g %g %g %g\n",
            MapXTo(drawseg->m_Start.x), MapYTo(drawseg->m_Start.y),
            MapXTo(drawseg->m_End.x), MapYTo(drawseg->m_End.y));
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

    for( track = aPcb->m_Track; track != NULL; track = track->Next() )
    {
        if( last_width != track->m_Width ) // Find a thickness already used.
        {
            for( ii = 0; ii < trackinfo.size(); ii++ )
            {
                if( trackinfo[ii] == track->m_Width )
                    break;
            }

            if( ii == trackinfo.size() )    // not found
                trackinfo.push_back( track->m_Width );

            last_width = track->m_Width;
        }
    }

    for( track = aPcb->m_Zone; track != NULL; track = track->Next() )
    {
        if( last_width != track->m_Width ) // Find a thickness already used.
        {
            for( ii = 0; ii < trackinfo.size(); ii++ )
            {
                if( trackinfo[ii] == track->m_Width )
                    break;
            }

            if( ii == trackinfo.size() )    // not found
                trackinfo.push_back( track->m_Width );

            last_width = track->m_Width;
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
    EDGE_MODULE*    PtEdge;
    EDA_ITEM*       PtStruct;
    // Control Y axis change sign for flipped modules
    int             Yaxis_sign = -1;

    // Flip for bottom side components
    if( module->flag )
        Yaxis_sign = 1;

    /* creates header: */
    fprintf( aFile, "\nSHAPE %s\n", TO_UTF8( module->m_Reference->m_Text ) );

    if( module->m_Attributs & MOD_VIRTUAL )
    {
        fprintf( aFile, "INSERT SMD\n");
    }
    else
    {
        if( module->m_Attributs & MOD_CMS )
    {
            fprintf( aFile, "INSERT SMD\n");
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
    for( PtStruct = module->m_Drawings; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case PCB_MODULE_TEXT_T:
        // If we wanted to export text, this is not the correct section
            break;

        case PCB_MODULE_EDGE_T:
            PtEdge = (EDGE_MODULE*) PtStruct;
            if( PtEdge->GetLayer() == SILKSCREEN_N_FRONT
            || PtEdge->GetLayer() == SILKSCREEN_N_BACK )
            {
                switch( PtEdge->m_Shape )
            {
            case S_SEGMENT:
                    fprintf( aFile, "LINE %g %g %g %g\n",
                            (PtEdge->m_Start0.x) / SCALE_FACTOR,
                            (Yaxis_sign * PtEdge->m_Start0.y) / SCALE_FACTOR,
                            (PtEdge->m_End0.x) / SCALE_FACTOR,
                            (Yaxis_sign * PtEdge->m_End0.y ) / SCALE_FACTOR);
                break;

            case S_CIRCLE:
            {
                        int radius = (int) hypot(
                                (double) ( PtEdge->m_End0.x - PtEdge->m_Start0.x ),
                                (double) ( PtEdge->m_End0.y - PtEdge->m_Start0.y ) );
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
                        RotatePoint( &arcendx, &arcendy, -PtEdge->m_Angle );
                        arcendx += PtEdge->m_Start0.x;
                        arcendy += PtEdge->m_Start0.y;
            if (Yaxis_sign == -1) {
                // Flipping Y flips the arc direction too
                fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                    (arcendx) / SCALE_FACTOR,
                    (Yaxis_sign * arcendy) / SCALE_FACTOR,
                    (PtEdge->m_End0.x) / SCALE_FACTOR,
                    (Yaxis_sign * PtEdge->m_End0.y) / SCALE_FACTOR,
                    (PtEdge->m_Start0.x) / SCALE_FACTOR,
                    (Yaxis_sign * PtEdge->m_Start0.y) / SCALE_FACTOR );
            } else {
                fprintf( aFile, "ARC %g %g %g %g %g %g\n",
                    (PtEdge->m_End0.x) / SCALE_FACTOR,
                    (Yaxis_sign * PtEdge->m_End0.y) / SCALE_FACTOR,
                    (arcendx) / SCALE_FACTOR,
                    (Yaxis_sign * arcendy) / SCALE_FACTOR,
                    (PtEdge->m_Start0.x) / SCALE_FACTOR,
                    (Yaxis_sign * PtEdge->m_Start0.y) / SCALE_FACTOR );
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
