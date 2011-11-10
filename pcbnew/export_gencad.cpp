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


bool        CreateHeaderInfoData( FILE* file, PCB_EDIT_FRAME* frame );
static void CreateTracksInfoData( FILE* file, BOARD* pcb );
static void CreateBoardSection( FILE* file, BOARD* pcb );
static void CreateComponentsSection( FILE* file, BOARD* pcb );
static void CreateDevicesSection( FILE* file, BOARD* pcb );
static void CreateRoutesSection( FILE* file, BOARD* pcb );
static void CreateSignalsSection( FILE* file, BOARD* pcb );
static void CreateShapesSection( FILE* file, BOARD* pcb );
static void CreatePadsShapesSection( FILE* file, BOARD* pcb );
static void CreatePadsStacksSection( FILE* file, BOARD* pcb );
static void FootprintWriteShape( FILE* File, MODULE* module );

// layer name for Gencad export
static const wxString GenCAD_Layer_Name[32] =
{
    wxT( "BOTTOM" ),             wxT( "INNER1" ),            wxT( "INNER2" ),
    wxT( "INNER3" ),             wxT( "INNER4" ),            wxT( "INNER5" ),
    wxT( "INNER6" ),             wxT( "INNER7" ),            wxT( "INNER8" ),
    wxT( "INNER9" ),             wxT( "INNER10" ),           wxT( "INNER11" ),
    wxT( "INNER12" ),            wxT( "INNER13" ),           wxT( "INNER14" ),
    wxT( "TOP" ),                wxT( "adhecu" ),            wxT( "adhecmp" ),
    wxT( "SOLDERPASTE_BOTTOM" ), wxT( "SOLDERPASTE_TOP" ),
    wxT( "SILKSCREEN_BOTTOM" ),  wxT( "SILKSCREEN_TOP" ),
    wxT( "SOLDERMASK_BOTTOM" ),  wxT( "SOLDERMASK_TOP" ),    wxT( "drawings" ),
    wxT( "comments" ),           wxT( "eco1" ),              wxT( "eco2" ),
    wxT( "edges" ),              wxT( "--" ),                wxT( "--" ),
    wxT( "--" )
};

int    offsetX, offsetY;
D_PAD* PadList;


/* 2 helper functions to calculate coordinates of modules in gencad values (
 * GenCAD Y axis from bottom to top)
 */
static int mapXto( int x )
{
    return x - offsetX;
}


static int mapYto( int y )
{
    return offsetY - y;
}


void PCB_EDIT_FRAME::ExportToGenCAD( wxCommandEvent& event )
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

    /* Update some board data, to ensure a reliable gencad export: */
    GetBoard()->ComputeBoundingBox();

    offsetX = m_Auxiliary_Axis_Position.x;
    offsetY = m_Auxiliary_Axis_Position.y;
    Compile_Ratsnest( NULL, true );

    /* Temporary modification of footprints that are flipped (i.e. on bottom
     * layer) to convert them to non flipped footprints.
     *  This is necessary to easily export shapes to GenCAD,
     *  that are given as normal orientation (non flipped, rotation = 0))
     * these changes will be undone later
     */
    MODULE* module;

    for( module = GetBoard()->m_Modules; module != NULL; module = module->Next() )
    {
        module->flag = 0;

        if( module->GetLayer() == LAYER_N_BACK )
        {
            module->Flip( module->m_Pos );
            module->flag = 1;
        }
    }

    // Create file header:
    CreateHeaderInfoData( file, this );
    CreateBoardSection( file, GetBoard() );

    /* Create TRACKS list
     *  This is the section $TRACK) (track width sizes) */
    CreateTracksInfoData( file, GetBoard() );

    /* Create the shapes list
     *  (shapes of pads and footprints */
    CreatePadsShapesSection( file, GetBoard() );   /* Must be called
                                                    * before
                                                    * CreatePadsStacksSection
                                                    * and
                                                    * CreateShapesSection()
                                                    */
    CreatePadsStacksSection( file, GetBoard() );
    CreateShapesSection( file, GetBoard() );

    CreateDevicesSection( file, GetBoard() );
    CreateComponentsSection( file, GetBoard() );

    /* Create the list of Nets: */
    CreateSignalsSection( file, GetBoard() );

    // Creates the Routes section (i.e. the list of board tracks)
    CreateRoutesSection( file, GetBoard() );

    fclose( file );

    /* Undo the footprints modifications (flipped footprints) */
    for( module = GetBoard()->m_Modules; module != NULL; module = module->Next() )
    {
        if( module->flag )
        {
            module->Flip( module->m_Pos );
            module->flag = 0;
        }
    }
}


static int Pad_list_Sort_by_Shapes( const void* refptr, const void* objptr )
{
    const D_PAD* padref = *(D_PAD**) refptr;
    const D_PAD* padcmp = *(D_PAD**) objptr;

    return D_PAD::Compare( padref, padcmp );
}


/* Creates the pads shapes list ( 1 shape per pad )
 *  Uses .GetSubRatsnest member of class D_PAD, to handle the shape id (value 1
 * ..n) for pads shapes PAD1 to PADn
 *
 *  The PADS section is used to describe the shape of all the pads used on the
 *  printed circuit board. The PADS section must be included, even if only a
 *  default pad is described and used for all pads.
 * The keywords used in the PADS section are:
 *  $PADS
 *  PAD <pad_name> <pad_type> <drill_size>
 *  LINE <line_ref>
 *  ARC <arc_ref>
 *  CIRCLE <circle_ref>
 *  RECTANGLE <rectangle_ref>
 *  ATTRIBUTE <attrib_ref>
 *  $ENDPADS
 *  $PADS and $ENDPADS mark the PADS section of the GenCAD file. Each pad
 *  description must start with a PAD keyword.
 *  The layer in which a pad lies is defined in the SHAPE section of the GenCAD
 *  specification.
 *  The pad is always placed on a shape at the pad origin, or in a pad stack at
 *  the pad stack origin.
 */
void CreatePadsShapesSection( FILE* file, BOARD* pcb )
{
    std::vector<D_PAD*> pads;

    const char*         pad_type;

    fputs( "$PADS\n", file );

    if( pcb->GetPadsCount() > 0 )
    {
        pads.insert( pads.end(),
                     pcb->m_NetInfo->m_PadsFullList.begin(),
                     pcb->m_NetInfo->m_PadsFullList.end() );
        qsort( &pads[0], pcb->GetPadsCount(), sizeof( D_PAD* ), Pad_list_Sort_by_Shapes );
    }

    D_PAD* old_pad = NULL;
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

        fprintf( file, "PAD PAD%d", pad->GetSubRatsnest() );

        int dx = pad->m_Size.x / 2;
        int dy = pad->m_Size.y / 2;

        switch( pad->m_PadShape )
        {
        default:
        case PAD_CIRCLE:
            pad_type = "ROUND";
            fprintf( file, " %s %d\n", pad_type, pad->m_Drill.x );
            fprintf( file, "CIRCLE %d %d %d\n",
                     pad->m_Offset.x, -pad->m_Offset.y, dx );
            break;

        case PAD_RECT:
            pad_type = "RECTANGULAR";
            fprintf( file, " %s %d\n", pad_type, pad->m_Drill.x );
            fprintf( file, "RECTANGLE %d %d %d %d\n",
                     pad->m_Offset.x - dx, -pad->m_Offset.y - dy,
                     pad->m_Size.x, pad->m_Size.y );
            break;

        case PAD_OVAL:     /* Create outline by 2 lines and 2 arcs */
        {
            pad_type = "FINGER";
            fprintf( file, " %s %d\n", pad_type, pad->m_Drill.x );
            int dr = dx - dy;
            if( dr >= 0 )       // Horizontal oval
            {
                int radius = dy;
                fprintf( file, "LINE %d %d %d %d\n",
                         -dr + pad->m_Offset.x, -pad->m_Offset.y - radius,
                         dr + pad->m_Offset.x, -pad->m_Offset.y - radius );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         dr + pad->m_Offset.x, -pad->m_Offset.y - radius,
                         dr + pad->m_Offset.x, -pad->m_Offset.y + radius,
                         dr + pad->m_Offset.x, -pad->m_Offset.y );

                fprintf( file, "LINE %d %d %d %d\n",
                         dr + pad->m_Offset.x, -pad->m_Offset.y + radius,
                         -dr + pad->m_Offset.x, -pad->m_Offset.y + radius );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         -dr + pad->m_Offset.x, -pad->m_Offset.y + radius,
                         -dr + pad->m_Offset.x, -pad->m_Offset.y - radius,
                         -dr + pad->m_Offset.x, -pad->m_Offset.y );
            }
            else        // Vertical oval
            {
                dr = -dr;
                int radius = dx;
                fprintf( file, "LINE %d %d %d %d\n",
                         -radius + pad->m_Offset.x, -pad->m_Offset.y - dr,
                         -radius + pad->m_Offset.x, -pad->m_Offset.y + dr );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         -radius + pad->m_Offset.x, -pad->m_Offset.y + dr,
                         radius + pad->m_Offset.x, -pad->m_Offset.y + dr,
                         pad->m_Offset.x, -pad->m_Offset.y + dr );

                fprintf( file, "LINE %d %d %d %d\n",
                         radius + pad->m_Offset.x, -pad->m_Offset.y + dr,
                         radius + pad->m_Offset.x, -pad->m_Offset.y - dr );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         radius + pad->m_Offset.x, -pad->m_Offset.y - dr,
                         -radius + pad->m_Offset.x, -pad->m_Offset.y - dr,
                         pad->m_Offset.x, -pad->m_Offset.y - dr );
            }
            break;
        }

        case PAD_TRAPEZOID:
            pad_type = "POLYGON";
            break;
        }
    }

    fputs( "$ENDPADS\n\n", file );
}


/*The PADSTACKS section is optional, and is used to describe how a group of
 * pads are
 *  arranged. The keywords used in the PADSTACKS section are:
 *  $PADSTACKS
 *  PADSTACK <pad_name> <drill_size>
 *  PAD <pad_name> <layer> <rot> <mirror>
 *  ATTRIBUTE <attrib_ref>
 *  $ENDPADSTACKS
 *  $PADSTACKS and $ENDPADSTACKS mark the PADSTACKS section of the GenCAD file.
 */
void CreatePadsStacksSection( FILE* file, BOARD* pcb )
{
    fputs( "$PADSTACKS\n", file );
    fputs( "$ENDPADSTACKS\n\n", file );
}


/* Creates the footprint shape list.
 * We must use one shape for identical footprint (i.e. come from the same
 * footprint in lib)
 *  But because pads shapes and positions can be easily modified on board,
 *  a shape is created by footprint found.
 * (todo : compare footprints shapes and creates only one shape for all
 * footprints found having the same shape)
 *  The shape is always given in orientation 0, position 0 not flipped
 *
 *  Syntax:
 *  $SHAPES
 *  SHAPE <shape_name>
 *  INSERT <string>         here <string> = "TH"
 *  shape_descr (line, arc ..)
 *  PIN <pin_name> <pad_name> <x_y_ref> <layer> <rot> <mirror>
 *
 *  SHAPE <shape_name>
 *  ..
 *  $ENDSHAPES
 */
void CreateShapesSection( FILE* file, BOARD* pcb )
{
    MODULE*     module;
    D_PAD*      pad;
    const char* layer;
    int         orient;
    wxString    pinname;
    const char* mirror = "0";

    fputs( "$SHAPES\n", file );

    for( module = pcb->m_Modules; module != NULL; module = module->Next() )
    {
        FootprintWriteShape( file, module );

        for( pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            layer = "ALL";

            if( ( pad->m_layerMask & ALL_CU_LAYERS ) == LAYER_BACK )
            {
                if( module->GetLayer() == LAYER_N_FRONT )
                    layer = "BOTTOM";
                else
                    layer = "TOP";
            }
            else if( ( pad->m_layerMask & ALL_CU_LAYERS ) == LAYER_FRONT )
            {
                if( module->GetLayer() == LAYER_N_FRONT )
                    layer = "TOP";
                else
                    layer = "BOTTOM";
            }

            pad->ReturnStringPadName( pinname );

            if( pinname.IsEmpty() )
                pinname = wxT( "noname" );

            orient = pad->m_Orient - module->m_Orient;
            NORMALIZE_ANGLE_POS( orient );
            fprintf( file, "PIN %s PAD%d %d %d %s %d %s",
                     TO_UTF8( pinname ), pad->GetSubRatsnest(),
                     pad->m_Pos0.x, -pad->m_Pos0.y,
                     layer, orient / 10, mirror );

            if( orient % 10 )
                fprintf( file, " .%d", orient % 10 );

            fprintf( file, "\n" );
        }
    }

    fputs( "$ENDSHAPES\n\n", file );
}


/* Creates the section $COMPONENTS (Footprints placement)
 *  When a footprint is on bottom side of the board::
 *  shapes are given with option "FLIP" and "MIRRORX".
 *  - But shapes remain given like component not mirrored and not flipped
 *  - orientation is given like if where not mirrored and not flipped.
 */
void CreateComponentsSection( FILE* file, BOARD* pcb )
{
    MODULE*       module = pcb->m_Modules;
    TEXTE_MODULE* PtTexte;
    const char*   mirror;
    const char*   flip;
    int           ii;

    fputs( "$COMPONENTS\n", file );

    for( ; module != NULL; module = module->Next() )
    {
        int orient = module->m_Orient;

        if( module->flag )
        {
            mirror = "MIRRORX";         // Mirrored relative to X axis
            flip   = "FLIP";            // Normal shape description ( gencad
                                        // viewer must show it flipped and
                                        // mirrored)
            NEGATE_AND_NORMALIZE_ANGLE_POS( orient );
        }
        else
        {
            mirror = "0";
            flip   = "0";
        }

        fprintf( file, "COMPONENT %s\n", TO_UTF8( module->m_Reference->m_Text ) );
        fprintf( file, "DEVICE %s\n", TO_UTF8( module->m_Reference->m_Text ) );
        fprintf( file, "PLACE %d %d\n", mapXto( module->m_Pos.x ), mapYto( module->m_Pos.y ) );
        fprintf( file, "LAYER %s\n", (module->flag) ? "BOTTOM" : "TOP" );
        fprintf( file, "ROTATION %d", orient / 10 );

        if( orient % 10 )
            fprintf( file, ".%d", orient % 10 );

        fputs( "\n", file );

        fprintf( file, "SHAPE %s %s %s\n", TO_UTF8( module->m_Reference->m_Text ), mirror, flip );

        /* creates texts (ref and value) */
        PtTexte = module->m_Reference;

        for( ii = 0; ii < 2; ii++ )
        {
            int      orient = PtTexte->m_Orient;
            wxString layer  = GenCAD_Layer_Name[SILKSCREEN_N_FRONT];
            fprintf( file, "TEXT %d %d %d %d.%d %s %s \"%s\"",
                     PtTexte->m_Pos0.x, -PtTexte->m_Pos0.y,
                     PtTexte->m_Size.x,
                     orient / 10, orient % 10,
                     mirror,
                     TO_UTF8( layer ),
                     TO_UTF8( PtTexte->m_Text )
                     );

            fprintf( file, " 0 0 %d %d\n",
                     (int) ( PtTexte->m_Size.x * PtTexte->m_Text.Len() ),
                     (int) PtTexte->m_Size.y );

            PtTexte = module->m_Value;
        }

        //put a comment:
        fprintf( file, "SHEET Part %s %s\n",
                 TO_UTF8( module->m_Reference->m_Text ),
                 TO_UTF8( module->m_Value->m_Text ) );
    }

    fputs( "$ENDCOMPONENTS\n\n", file );
}


/* Creates the list of Nets:
 *  $SIGNALS
 *      SIGNAL <net name>
 *      NODE <component name> <pin name>
 *      ...
 *      NODE <component name> <pin name>
 *  $ENDSIGNALS
 */
void CreateSignalsSection( FILE* file, BOARD* pcb )
{
    wxString      msg;
    NETINFO_ITEM* net;
    D_PAD*        pad;
    MODULE*       module;
    int           NbNoConn = 1;

    fputs( "$SIGNALS\n", file );

    for( unsigned ii = 0; ii < pcb->m_NetInfo->GetCount(); ii++ )
    {
        net = pcb->m_NetInfo->GetNetItem( ii );

        if( net->GetNetname() == wxEmptyString ) // dummy netlist (no connection)
        {
            wxString msg; msg << wxT( "NoConnection" ) << NbNoConn++;
            net->SetNetname( msg );
        }

        if( net->GetNet() <= 0 )  // dummy netlist (no connection)
            continue;

        msg = wxT( "SIGNAL " ) + net->GetNetname();

        fputs( TO_UTF8( msg ), file );
        fputs( "\n", file );

        for( module = pcb->m_Modules; module != NULL; module = module->Next() )
        {
            for( pad = module->m_Pads; pad != NULL; pad = pad->Next() )
            {
                wxString padname;

                if( pad->GetNet() != net->GetNet() )
                    continue;

                pad->ReturnStringPadName( padname );
                msg.Printf( wxT( "NODE %s %.4s" ),
                            GetChars( module->m_Reference->m_Text ),
                            GetChars( padname ) );

                fputs( TO_UTF8( msg ), file );
                fputs( "\n", file );
            }
        }
    }

    fputs( "$ENDSIGNALS\n\n", file );
}


/* Creates the section $HEADER ... $ENDHEADER
 */
bool CreateHeaderInfoData( FILE* file, PCB_EDIT_FRAME* frame )
{
    wxString    msg;
    PCB_SCREEN* screen = (PCB_SCREEN*) ( frame->GetScreen() );

    fputs( "$HEADER\n", file );
    fputs( "GENCAD 1.4\n", file );
    msg = wxT( "USER " ) + wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();
    fputs( TO_UTF8( msg ), file ); fputs( "\n", file );
    msg = wxT( "DRAWING " ) + screen->GetFileName();
    fputs( TO_UTF8( msg ), file ); fputs( "\n", file );
    msg = wxT( "REVISION " ) + screen->m_Revision + wxT( " " ) + screen->m_Date;
    fputs( TO_UTF8( msg ), file ); fputs( "\n", file );
    msg.Printf( wxT( "UNITS USER %d" ), PCB_INTERNAL_UNIT );
    fputs( TO_UTF8( msg ), file ); fputs( "\n", file );
    msg.Printf( wxT( "ORIGIN %d %d" ),
                mapXto( frame->m_Auxiliary_Axis_Position.x ),
                mapYto( frame->m_Auxiliary_Axis_Position.y ) );
    fputs( TO_UTF8( msg ), file ); fputs( "\n", file );
    fputs( "INTERTRACK 0\n", file );
    fputs( "$ENDHEADER\n\n", file );

    return true;
}


/*
 *  Sort function used to sort tracks segments:
 *   items are sorted by netcode, then by width then by layer
 */
static int Track_list_Sort_by_Netcode( const void* refptr, const void* objptr )
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
void CreateRoutesSection( FILE* file, BOARD* pcb )
{
    TRACK* track, ** tracklist;
    int    vianum = 1;
    int    old_netcode, old_width, old_layer;
    int    nbitems, ii;

    // Count items
    nbitems = 0;

    for( track = pcb->m_Track; track != NULL; track = track->Next() )
        nbitems++;

    for( track = pcb->m_Zone; track != NULL; track = track->Next() )
    {
        if( track->Type() == PCB_ZONE_T )
            nbitems++;
    }

    tracklist = (TRACK**) operator new( (nbitems + 1) * sizeof( TRACK* ) );

    nbitems = 0;

    for( track = pcb->m_Track; track != NULL; track = track->Next() )
        tracklist[nbitems++] = track;

    for( track = pcb->m_Zone; track != NULL; track = track->Next() )
    {
        if( track->Type() == PCB_ZONE_T )
            tracklist[nbitems++] = track;
    }

    tracklist[nbitems] = NULL;

    qsort( tracklist, nbitems, sizeof(TRACK*), Track_list_Sort_by_Netcode );

    fputs( "$ROUTES\n", file );

    old_netcode = -1; old_width = -1; old_layer = -1;

    for( ii = 0; ii < nbitems; ii++ )
    {
        track = tracklist[ii];

        if( old_netcode != track->GetNet() )
        {
            old_netcode = track->GetNet();
            NETINFO_ITEM* net = pcb->FindNet( track->GetNet() );
            wxString      netname;

            if( net && (net->GetNetname() != wxEmptyString) )
                netname = net->GetNetname();
            else
                netname = wxT( "_noname_" );

            fprintf( file, "ROUTE %s\n", TO_UTF8( netname ) );
        }

        if( old_width != track->m_Width )
        {
            old_width = track->m_Width;
            fprintf( file, "TRACK TRACK%d\n", track->m_Width );
        }

        if( (track->Type() == PCB_TRACE_T) || (track->Type() == PCB_ZONE_T) )
        {
            if( old_layer != track->GetLayer() )
            {
                old_layer = track->GetLayer();
                fprintf( file, "LAYER %s\n",
                         TO_UTF8( GenCAD_Layer_Name[track->GetLayer() & 0x1F] ) );
            }

            fprintf( file, "LINE %d %d %d %d\n",
                     mapXto( track->m_Start.x ), mapYto( track->m_Start.y ),
                     mapXto( track->m_End.x ), mapYto( track->m_End.y ) );
        }
        if( track->Type() == PCB_VIA_T )
        {
            fprintf( file, "VIA viapad%d %d %d ALL %d via%d\n",
                     track->m_Width,
                     mapXto( track->m_Start.x ), mapYto( track->m_Start.y ),
                     track->GetDrillValue(), vianum++ );
        }
    }

    fputs( "$ENDROUTES\n\n", file );

    delete tracklist;
}


/* Creates the section $DEVICES
 * This is a list of footprints properties
 *  ( Shapes are in section $SHAPE )
 */
void CreateDevicesSection( FILE* file, BOARD* pcb )
{
    MODULE* module;
    D_PAD*  pad;

    fputs( "$DEVICES\n", file );

    for( module = pcb->m_Modules; module != NULL; module = module->Next() )
    {
        fprintf( file, "DEVICE %s\n", TO_UTF8( module->m_Reference->m_Text ) );
        fprintf( file, "PART %s\n", TO_UTF8( module->m_LibRef ) );
        fprintf( file, "TYPE %s\n", "UNKNOWN" );

        for( pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            fprintf( file, "PINDESCR %.4s", pad->m_Padname );

            if( pad->GetNetname() == wxEmptyString )
                fputs( " NoConn\n", file );
            else
                fprintf( file, " %.4s\n", pad->m_Padname );
        }

        fprintf( file, "ATTRIBUTE %s\n", TO_UTF8( module->m_Value->m_Text ) );
    }

    fputs( "$ENDDEVICES\n\n", file );
}


/* Creates the section $BOARD.
 *  We output here only the board boundary box
 */
void CreateBoardSection( FILE* file, BOARD* pcb )
{
    fputs( "$BOARD\n", file );
    fprintf( file, "LINE %d %d %d %d\n",
             mapXto( pcb->m_BoundaryBox.m_Pos.x ),
             mapYto( pcb->m_BoundaryBox.m_Pos.y ),
             mapXto( pcb->m_BoundaryBox.GetRight() ),
             mapYto( pcb->m_BoundaryBox.m_Pos.y ) );
    fprintf( file, "LINE %d %d %d %d\n",
             mapXto( pcb->m_BoundaryBox.GetRight() ),
             mapYto( pcb->m_BoundaryBox.m_Pos.y ),
             mapXto( pcb->m_BoundaryBox.GetRight() ),
             mapYto( pcb->m_BoundaryBox.GetBottom() ) );
    fprintf( file, "LINE %d %d %d %d\n",
             mapXto( pcb->m_BoundaryBox.GetRight() ),
             mapYto( pcb->m_BoundaryBox.GetBottom() ),
             mapXto( pcb->m_BoundaryBox.m_Pos.x ),
             mapYto( pcb->m_BoundaryBox.GetBottom() ) );
    fprintf( file, "LINE %d %d %d %d\n",
             mapXto( pcb->m_BoundaryBox.m_Pos.x ),
             mapYto( pcb->m_BoundaryBox.GetBottom() ),
             mapXto( pcb->m_BoundaryBox.m_Pos.x ),
             mapYto( pcb->m_BoundaryBox.m_Pos.y ) );

    fputs( "$ENDBOARD\n\n", file );
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
void CreateTracksInfoData( FILE* file, BOARD* pcb )
{
    TRACK* track;
    int    last_width = -1;

    /* Find thickness used for traces. */

    std::vector <int> trackinfo;

    unsigned          ii;

    for( track = pcb->m_Track; track != NULL; track = track->Next() )
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

    for( track = pcb->m_Zone; track != NULL; track = track->Next() )
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
    fputs( "$TRACKS\n", file );

    for( ii = 0; ii < trackinfo.size(); ii++ )
    {
        fprintf( file, "TRACK TRACK%d %d\n", trackinfo[ii], trackinfo[ii] );
    }

    fputs( "$ENDTRACKS\n\n", file );
}


/* Creates the shape of a footprint (section SHAPE)
 *  The shape is always given "normal" (Orient 0, not mirrored)
 *  Syntax:
 *  SHAPE <shape_name>
 *  INSERT <string>         here <string> = "TH"
 *  shape_descr (line, arc ..):
 *  LINE startX startY endX endY
 *  ARC startX startY endX endY centreX centreY
 *  PAD_CIRCLE centreX scentreY radius
 */
void FootprintWriteShape( FILE* file, MODULE* module )
{
    EDGE_MODULE* edge;
    EDA_ITEM*    item;
    int          y_axis_sign = -1; // Control Y axis change sign (as normal
                                   // module / mirror axis and conventions)

    /* creates header: */
    fprintf( file, "SHAPE %s\n", TO_UTF8( module->m_Reference->m_Text ) );
    fprintf( file, "INSERT %s\n", (module->m_Attributs & MOD_CMS) ? "SMD" : "TH" );

    /* creates Attributes */
    if( module->m_Attributs != MOD_DEFAULT )
    {
        fprintf( file, "ATTRIBUTE" );

        if( module->m_Attributs & MOD_CMS )
            fprintf( file, " PAD_SMD" );

        if( module->m_Attributs & MOD_VIRTUAL )
            fprintf( file, " VIRTUAL" );

        fprintf( file, "\n" );
    }

    /* creates Drawing */
    item = module->m_Drawings;

    for( ; item != NULL; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
            break;

        case PCB_MODULE_EDGE_T:
            edge = (EDGE_MODULE*) item;

            switch( edge->m_Shape )
            {
            case S_SEGMENT:
                fprintf( file, "LINE %d %d %d %d\n",
                         edge->m_Start0.x, y_axis_sign * edge->m_Start0.y,
                         edge->m_End0.x, y_axis_sign * edge->m_End0.y );
                break;

            case S_CIRCLE:
            {
                int radius = (int) hypot( (double) ( edge->m_End0.x - edge->m_Start0.x ),
                                          (double) ( edge->m_End0.y - edge->m_Start0.y ) );
                fprintf( file, "CIRCLE %d %d %d\n",
                         edge->m_Start0.x, y_axis_sign * edge->m_Start0.y, radius );
                break;
            }

            case S_ARC:         /* print ARC x,y start x,y end x,y center */
            {   // Arcs are defined counter clockwise (positive trigonometric)
                // from the start point to the end point (0 to 360 degrees)
                wxPoint arcStart, arcEnd;

                // edge->m_Start0 is the arc center relative to the shape position
                // edge->m_End0 is the arc start point relative to the shape position
                arcStart = edge->m_End0;

                // calculate arcEnd arc end point relative to the shape position, in Pcbnew
                // coordinates
                arcEnd = arcStart;
                RotatePoint( &arcEnd, edge->m_Start0, -edge->m_Angle );

                // due to difference between Pcbnew and gencad, swap arc start and arc end
                EXCHG(arcEnd, arcStart);

                // print arc shape:
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         arcStart.x, y_axis_sign * arcStart.y,   // Start point
                         arcEnd.x, y_axis_sign * arcEnd.y,               // End point
                         edge->m_Start0.x, y_axis_sign * edge->m_Start0.y );
                break;
            }

            default:
                DisplayError( NULL, wxT( "Type Edge Module invalid." ) );
                break;
            }  /* end switch  PtEdge->m_Shape */

            break;

        default:
            break;
        }  /* End switch Items type */
    }
}
