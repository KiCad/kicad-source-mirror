/***************************************************/
/* export_gencad.cpp - export en formay GenCAD 1.4 */
/***************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "trigo.h"

bool        CreateHeaderInfoData( FILE* file, WinEDA_PcbFrame* frame );
static void CreateTracksInfoData( FILE* file, BOARD* pcb );
static void CreateBoardSection( FILE* file, BOARD* pcb );
static void CreateComponentsSection( FILE* file, BOARD* pcb );
static void CreateDevicesSection( FILE* file, BOARD* pcb );
static void CreateRoutesSection( FILE* file, BOARD* pcb );
static void CreateSignalsSection( FILE* file, BOARD* pcb );
static void CreateShapesSection( FILE* file, BOARD* pcb );
static void CreatePadsShapesSection( FILE* file, BOARD* pcb );
static void FootprintWriteShape( FILE* File, MODULE* module );

// layer name for Gencad export
static const wxString GenCAD_Layer_Name[32] =
{
    wxT( "BOTTOM" ),            wxT( "INNER1" ),         wxT( "INNER2" ),
    wxT(
        "INNER3" ),
    wxT( "INNER4" ),            wxT( "INNER5" ),         wxT( "INNER6" ),            wxT(
        "INNER7" ),
    wxT( "INNER8" ),            wxT( "INNER9" ),         wxT( "INNER10" ),           wxT(
        "INNER11" ),
    wxT( "INNER12" ),           wxT( "INNER13" ),        wxT( "INNER14" ),           wxT( "TOP" ),
    wxT( "adhecu" ),            wxT( "adhecmp" ),        wxT( "SOLDERPASTE_BOTTOM" ),wxT(
        "SOLDERPASTE_TOP" ),
    wxT( "SILKSCREEN_BOTTOM" ), wxT( "SILKSCREEN_TOP" ), wxT( "SOLDERMASK_BOTTOM" ), wxT(
        "SOLDERMASK_TOP" ),
    wxT( "drawings" ),          wxT( "comments" ),       wxT( "eco1" ),              wxT( "eco2" ),
    wxT( "edges" ),             wxT( "--" ),             wxT( "--" ),                wxT( "--" )
};

int    offsetX, offsetY;
D_PAD* PadList;

/* 2 helper functions to calculate coordinates of modules in gencad values ( GenCAD Y axis from bottom to top)
 */
static int mapXto( int x )
{
    return x - offsetX;
}


static int mapYto( int y )
{
    return offsetY - y;
}


/***********************************************************/
void WinEDA_PcbFrame::ExportToGenCAD( wxCommandEvent& event )
/***********************************************************/

/*
 *  Creates an Export file (format GenCAD 1.4) from the current borad.
 */
{
    wxFileName fn = GetScreen()->m_FileName;
    wxString   msg, ext, wildcard;
    FILE*      file;

    ext = wxT( "gcd" );
    wildcard = _( "GenCAD board files (.gcd)|*.gcd" );
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
    GetBoard()->ComputeBoundaryBox();

    offsetX = m_Auxiliary_Axis_Position.x;
    offsetY = m_Auxiliary_Axis_Position.y;
    wxClientDC dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );
    Compile_Ratsnest( &dc, TRUE );

    /* Temporary modification of footprints that are flipped (i.e. on bottom layer)
     * to convert them to non flipped footprints.
     *  This is necessary to easily export shapes to GenCAD,
     *  that are given as normal orientation (non flipped, rotation = 0))
     * these changes will be undone later
     */
    MODULE* module;
    for( module = GetBoard()->m_Modules; module != NULL; module = module->Next() )
    {
        module->flag = 0;
        if( module->GetLayer() == COPPER_LAYER_N )
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
    CreatePadsShapesSection( file, GetBoard() );   // doit etre appele avant CreateShapesSection()
    CreateShapesSection( file, GetBoard() );

    /* Create the list of Nets: */
    CreateSignalsSection( file, GetBoard() );

    CreateDevicesSection( file, GetBoard() );
    CreateComponentsSection( file, GetBoard() );
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


/**************************************************************************/
static int Pad_list_Sort_by_Shapes( const void* refptr, const void* objptr )
/**************************************************************************/
{
    const D_PAD* padref = *(D_PAD**) refptr;
    const D_PAD* padcmp = *(D_PAD**) objptr;

    return D_PAD::Compare( padref, padcmp );
}


/*****************************************************/
void CreatePadsShapesSection( FILE* file, BOARD* pcb )
/*****************************************************/

/* Creates the pads shapes list ( 1 shape per pad )
 *  Uses .GetSubRatsnest member of class D_PAD, to handle the shape id (value 1 ..n)
 *  for pads shapes PAD1 to PADn
 */
{
    std::vector<D_PAD*> pads;

    const char*         pad_type;

    fputs( "$PADS\n", file );

    if( pcb->GetPadsCount() > 0 )
    {
        pads.insert( pads.end(),
                    pcb->m_NetInfo->m_PadsFullList.begin(), pcb->m_NetInfo->m_PadsFullList.end() );
        qsort( &pads[0], pcb->GetPadsCount(), sizeof( D_PAD* ), Pad_list_Sort_by_Shapes );
    }

    D_PAD* old_pad = NULL;
    int    pad_name_number = 0;
    for( unsigned i = 0;  i<pads.size();  ++i )
    {
        D_PAD* pad = pads[i];

        pad->SetSubRatsnest( pad_name_number );

        if( old_pad && 0==D_PAD::Compare( old_pad, pad ) )
            continue; // already created

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
                    pad->m_Offset.x - dx, -(pad->m_Offset.y - dy),
                    pad->m_Offset.x + dx, -(pad->m_Offset.y + dy) );
            break;

        case PAD_OVAL:     /* Create outline by 2 lines and 2 arcs */
        {
            pad_type = "FINGER";
            fprintf( file, " %s %d\n", pad_type, pad->m_Drill.x );
            int dr = dx - dy;
            if( dr >= 0 )       // Horizontal oval
            {
                int rayon = dy;
                fprintf( file, "LINE %d %d %d %d\n",
                         -dr + pad->m_Offset.x, -pad->m_Offset.y - rayon,
                         dr + pad->m_Offset.x, -pad->m_Offset.y - rayon );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         dr + pad->m_Offset.x, -pad->m_Offset.y - rayon,
                         dr + pad->m_Offset.x, -pad->m_Offset.y + rayon,
                         dr + pad->m_Offset.x, -pad->m_Offset.y );

                fprintf( file, "LINE %d %d %d %d\n",
                         dr + pad->m_Offset.x, -pad->m_Offset.y + rayon,
                         -dr + pad->m_Offset.x, -pad->m_Offset.y + rayon );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         -dr + pad->m_Offset.x, -pad->m_Offset.y + rayon,
                         -dr + pad->m_Offset.x, -pad->m_Offset.y - rayon,
                         -dr + pad->m_Offset.x, -pad->m_Offset.y );
            }
            else        // Vertical oval
            {
                dr = -dr;
                int rayon = dx;
                fprintf( file, "LINE %d %d %d %d\n",
                         -rayon + pad->m_Offset.x, -pad->m_Offset.y - dr,
                         -rayon + pad->m_Offset.x, -pad->m_Offset.y + dr );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         -rayon + pad->m_Offset.x, -pad->m_Offset.y + dr,
                         rayon + pad->m_Offset.x, -pad->m_Offset.y + dr,
                         pad->m_Offset.x, -pad->m_Offset.y + dr );

                fprintf( file, "LINE %d %d %d %d\n",
                         rayon + pad->m_Offset.x, -pad->m_Offset.y + dr,
                         rayon + pad->m_Offset.x, -pad->m_Offset.y - dr );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         rayon + pad->m_Offset.x, -pad->m_Offset.y - dr,
                         -rayon + pad->m_Offset.x, -pad->m_Offset.y - dr,
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


/**************************************************/
void CreateShapesSection( FILE* file, BOARD* pcb )
/**************************************************/

/* Creates the footprint shape list.
 * We must use one shape for identical footprint (i.e. come from the same footprint in lib)
 *  But because pads shapes and positions can be easily modified on board,
 *  a shape is created by footprint found.
 * (todo : compare footprints shapes and creates only one shape for all footprints found having the same shape)
 *  The shape is always given in orientation 0, position 0 not flipped
 *
 *  Syntaxe:
 *  $SHAPES
 *  SHAPE <shape_name>
 *  shape_descr (line, arc ..)
 *  PIN <pin_name> <pad_name> <x_y_ref> <layer> <rot> <mirror>
 *
 *  SHAPE <shape_name>
 *  ..
 *  $ENDSHAPES
 */
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
            if( (pad->m_Masque_Layer & ALL_CU_LAYERS) == CUIVRE_LAYER )
            {
                if( module->GetLayer() == CMP_N )
                    layer = "BOTTOM";
                else
                    layer = "TOP";
            }
            else if( (pad->m_Masque_Layer & ALL_CU_LAYERS) == CMP_LAYER )
            {
                if( module->GetLayer() == CMP_N )
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
                     CONV_TO_UTF8( pinname ), pad->GetSubRatsnest(),
                     pad->m_Pos0.x, -pad->m_Pos0.y,
                     layer, orient / 10, mirror );
            if( orient % 10 )
                fprintf( file, " .%d", orient % 10 );
            fprintf( file, "\n" );
        }
    }

    fputs( "$ENDSHAPES\n\n", file );
}


/******************************************************/
void CreateComponentsSection( FILE* file, BOARD* pcb )
/******************************************************/

/* Creates the section $COMPONENTS (Footprints placement)
 *  When a footprint is on bottom side of the board::
 *  shapes are given with option "FLIP" and "MIRRORX".
 *  - But shapes remain given like component not mirrored and not flipped
 *  - orientaion is given like if where not mirrored and not flipped.
 */
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
            flip   = "FLIP";            // Normal shape description ( gencad viewer must show it flipped and mirrored)
            NEGATE_AND_NORMALIZE_ANGLE_POS( orient );
        }
        else
        {
            mirror = "0";
            flip   = "0";
        }

        fprintf( file, "COMPONENT %s\n", CONV_TO_UTF8( module->m_Reference->m_Text ) );
        fprintf( file, "DEVICE %s\n", CONV_TO_UTF8( module->m_Reference->m_Text ) );
        fprintf( file, "PLACE %d %d\n", mapXto( module->m_Pos.x ), mapYto( module->m_Pos.y ) );
        fprintf( file, "LAYER %s\n", (module->flag) ? "BOTTOM" : "TOP" );

        fprintf( file, "ROTATION %d", orient / 10 );
        if( orient % 10 )
            fprintf( file, ".%d", orient % 10 );
        fputs( "\n", file );

        fprintf( file, "SHAPE %s %s %s\n",
                 CONV_TO_UTF8( module->m_Reference->m_Text ), mirror, flip );

        /* creates texts (ref and value) */
        PtTexte = module->m_Reference;
        for( ii = 0; ii < 2; ii++ )
        {
            int      orient = PtTexte->m_Orient;
            wxString layer  = GenCAD_Layer_Name[SILKSCREEN_N_CMP];
            fprintf( file, "TEXT %d %d %d %d.%d %s %s \"%s\"",
                     PtTexte->m_Pos0.x, -PtTexte->m_Pos0.y,
                     PtTexte->m_Size.x,
                     orient / 10, orient % 10,
                     mirror,
                     CONV_TO_UTF8( layer ),
                     CONV_TO_UTF8( PtTexte->m_Text )
                     );

            fprintf( file, " 0 0 %d %d\n",
                     (int) ( PtTexte->m_Size.x * PtTexte->m_Text.Len() ),
                     (int) PtTexte->m_Size.y );

            PtTexte = module->m_Value;
        }

        //put a comment:
        fprintf( file, "SHEET Part %s %s\n", CONV_TO_UTF8( module->m_Reference->m_Text ),
                CONV_TO_UTF8( module->m_Value->m_Text ) );
    }

    fputs( "$ENDCOMPONENTS\n\n", file );
}


/***************************************************/
void CreateSignalsSection( FILE* file, BOARD* pcb )
/***************************************************/

/* Creates the list of Nets:
 *  $SIGNALS
 *      SIGNAL <equipot name>
 *      NODE <component name> <pin name>
 *      ...
 *      NODE <component name> <pin name>
 *  $ENDSIGNALS
 */
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
        if( net->GetNetname() == wxEmptyString )  // dummy equipot (non connexion)
        {
            wxString msg; msg << wxT( "NoConnection" ) << NbNoConn++;
            net->SetNetname( msg );;
        }

        if( net->GetNet() <= 0 )  // dummy equipot (non connexion)
            continue;

        msg = wxT( "\nSIGNAL " ) + net->GetNetname();

        fputs( CONV_TO_UTF8( msg ), file );
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

                fputs( CONV_TO_UTF8( msg ), file );
                fputs( "\n", file );
            }
        }
    }

    fputs( "$ENDSIGNALS\n\n", file );
}


/*************************************************************/
bool CreateHeaderInfoData( FILE* file, WinEDA_PcbFrame* frame )
/*************************************************************/

/* Creates the section $HEADER ... $ENDHEADER
 */
{
    wxString    msg;
    PCB_SCREEN* screen = (PCB_SCREEN*)( frame->GetScreen() );

    fputs( "$HEADER\n", file );
    fputs( "GENCAD 1.4\n", file );
    msg = wxT( "USER " ) + wxGetApp().GetAppName() + wxT( " " ) +
          GetBuildVersion();
    fputs( CONV_TO_UTF8( msg ), file ); fputs( "\n", file );
    msg = wxT( "DRAWING " ) + screen->m_FileName;
    fputs( CONV_TO_UTF8( msg ), file ); fputs( "\n", file );
    msg = wxT( "REVISION " ) + screen->m_Revision + wxT( " " ) + screen->m_Date;
    fputs( CONV_TO_UTF8( msg ), file ); fputs( "\n", file );
    msg.Printf( wxT( "UNITS USER %d" ), PCB_INTERNAL_UNIT );
    fputs( CONV_TO_UTF8( msg ), file ); fputs( "\n", file );
    msg.Printf( wxT( "ORIGIN %d %d" ), mapXto( frame->m_Auxiliary_Axis_Position.x ),
               mapYto( frame->m_Auxiliary_Axis_Position.y ) );
    fputs( CONV_TO_UTF8( msg ), file ); fputs( "\n", file );
    fputs( "INTERTRACK 0\n", file );
    fputs( "$ENDHEADER\n\n", file );

    return TRUE;
}


/**************************************************************************/
static int Track_list_Sort_by_Netcode( const void* refptr, const void* objptr )
/**************************************************************************/

/*
 *  Sort function used to sort tracks segments:
 *   items are sorted by netcode, then by width then by layer
 */
{
    const TRACK* ref, * cmp;
    int          diff;

    ref = *( (TRACK**) refptr );
    cmp = *( (TRACK**) objptr );
    if( ( diff = ref->GetNet() - cmp->GetNet() ) )
        return diff;
    if( (diff = ref->m_Width - cmp->m_Width) )
        return diff;
    if( ( diff = ref->GetLayer() - cmp->GetLayer() ) )
        return diff;

    return 0;
}


/*************************************************/
void CreateRoutesSection( FILE* file, BOARD* pcb )
/*************************************************/

/* Creates the tracks, vias
 * TODO: add zones
 *  section:
 *  $ROUTE
 *  ...
 *  $ENROUTE
 *  Track segments must be sorted by nets
 */
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
        if( track->Type() == TYPE_ZONE )
            nbitems++;
    }

    tracklist = (TRACK**) MyMalloc( (nbitems + 1) * sizeof(TRACK*) );

    nbitems = 0;
    for( track = pcb->m_Track; track != NULL; track = track->Next() )
        tracklist[nbitems++] = track;

    for( track = pcb->m_Zone; track != NULL; track = track->Next() )
    {
        if( track->Type() == TYPE_ZONE )
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
            fprintf( file, "\nROUTE %s\n", CONV_TO_UTF8( netname ) );
        }

        if( old_width != track->m_Width )
        {
            old_width = track->m_Width;
            fprintf( file, "TRACK TRACK%d\n", track->m_Width );
        }

        if( (track->Type() == TYPE_TRACK) || (track->Type() == TYPE_ZONE) )
        {
            if( old_layer != track->GetLayer() )
            {
                old_layer = track->GetLayer();
                fprintf( file, "LAYER %s\n",
                        CONV_TO_UTF8( GenCAD_Layer_Name[track->GetLayer() & 0x1F] ) );
            }

            fprintf( file, "LINE %d %d %d %d\n",
                    mapXto( track->m_Start.x ), mapYto( track->m_Start.y ),
                    mapXto( track->m_End.x ), mapYto( track->m_End.y ) );
        }
        if( track->Type() == TYPE_VIA )
        {
            fprintf( file, "VIA viapad%d %d %d ALL %d via%d\n",
                     track->m_Width,
                     mapXto( track->m_Start.x ), mapYto( track->m_Start.y ),
                     track->GetDrillValue(), vianum++ );
        }
    }

    fputs( "$ENDROUTES\n\n", file );

    free( tracklist );
}


/***************************************************/
void CreateDevicesSection( FILE* file, BOARD* pcb )
/***************************************************/

/* Creatthes the section $DEVICES
 * This is a list of footprints properties
 *  ( Shapes are in section $SHAPE )
 */
{
    MODULE* module;
    D_PAD*  pad;

    fputs( "$DEVICES\n", file );

    for( module = pcb->m_Modules; module != NULL; module = module->Next() )
    {
        fprintf( file, "DEVICE %s\n", CONV_TO_UTF8( module->m_Reference->m_Text ) );
        fprintf( file, "PART %s\n", CONV_TO_UTF8( module->m_LibRef ) );
        fprintf( file, "TYPE %s\n", "UNKNOWN" );
        for( pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            fprintf( file, "PINDESCR %.4s", pad->m_Padname );
            if( pad->GetNetname() == wxEmptyString )
                fputs( " NoConn\n", file );
            else
                fprintf( file, " %.4s\n", pad->m_Padname );
        }

        fprintf( file, "ATTRIBUTE %s\n", CONV_TO_UTF8( module->m_Value->m_Text ) );
    }

    fputs( "$ENDDEVICES\n\n", file );
}


/*************************************************/
void CreateBoardSection( FILE* file, BOARD* pcb )
/*************************************************/

/* Creatthe  section $BOARD.
 *  We output here only the board boudary box
 */
{
    fputs( "$BOARD\n", file );
    fprintf( file, "LINE %d %d %d %d\n",
            mapXto( pcb->m_BoundaryBox.m_Pos.x ), mapYto( pcb->m_BoundaryBox.m_Pos.y ),
            mapXto( pcb->m_BoundaryBox.GetRight() ), mapYto( pcb->m_BoundaryBox.m_Pos.y ) );
    fprintf( file, "LINE %d %d %d %d\n",
            mapXto( pcb->m_BoundaryBox.GetRight() ), mapYto( pcb->m_BoundaryBox.m_Pos.y ),
            mapXto( pcb->m_BoundaryBox.GetRight() ), mapYto( pcb->m_BoundaryBox.GetBottom() ) );
    fprintf( file, "LINE %d %d %d %d\n",
            mapXto( pcb->m_BoundaryBox.GetRight() ), mapYto( pcb->m_BoundaryBox.GetBottom() ),
            mapXto( pcb->m_BoundaryBox.m_Pos.x ), mapYto( pcb->m_BoundaryBox.GetBottom() ) );
    fprintf( file, "LINE %d %d %d %d\n",
            mapXto( pcb->m_BoundaryBox.m_Pos.x ), mapYto( pcb->m_BoundaryBox.GetBottom() ),
            mapXto( pcb->m_BoundaryBox.m_Pos.x ), mapYto( pcb->m_BoundaryBox.m_Pos.y ) );

    fputs( "$ENDBOARD\n\n", file );
}


/****************************************************/
void CreateTracksInfoData( FILE* file, BOARD* pcb )
/****************************************************/

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
{
    TRACK* track;
    int    last_width = -1;

    /* recherche des epaisseurs utilisees pour les traces: */

    std::vector <int> trackinfo;

    unsigned          ii;
    for( track = pcb->m_Track; track != NULL; track = track->Next() )
    {
        if( last_width != track->m_Width ) // recherche d'une epaisseur deja utilisee
        {
            for( ii = 0;  ii < trackinfo.size(); ii++ )
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
        if( last_width != track->m_Width ) // recherche d'une epaisseur deja utilisee
        {
            for( ii = 0;  ii < trackinfo.size(); ii++ )
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
    for( ii = 0;  ii < trackinfo.size(); ii++ )
    {
        fprintf( file, "TRACK TRACK%d %d\n", trackinfo[ii], trackinfo[ii] );
    }

    fputs( "$ENDTRACKS\n\n", file );
}


/***************************************************/
void FootprintWriteShape( FILE* file, MODULE* module )
/***************************************************/

/* Creates the shape of a footprint (section SHAPE)
 *  The shape is always given "normal" (Orient 0, not mirrored)
 *  Syntax:
 *  SHAPE <shape_name>
 *  shape_descr (line, arc ..):
 *  LINE startX startY endX endY
 *  ARC startX startY endX endY centreX centreY
 *  PAD_CIRCLE centreX scentreY radius
 */
{
    EDGE_MODULE*    PtEdge;
    EDA_BaseStruct* PtStruct;
    int             Yaxis_sign = -1; // Controle changement signe axe Y (selon module normal/miroir et conventions d'axe)


    /* creates header: */
    fprintf( file, "\nSHAPE %s\n", CONV_TO_UTF8( module->m_Reference->m_Text ) );

    /* creates Attributs */
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
    PtStruct = module->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_TEXTE_MODULE:
            break;

        case TYPE_EDGE_MODULE:
            PtEdge = (EDGE_MODULE*) PtStruct;

            switch( PtEdge->m_Shape )
            {
            case S_SEGMENT:
                fprintf( file, "LINE %d %d %d %d\n",
                         PtEdge->m_Start0.x, Yaxis_sign * PtEdge->m_Start0.y,
                         PtEdge->m_End0.x, Yaxis_sign * PtEdge->m_End0.y );
                break;

            case S_CIRCLE:
            {
                int rayon = (int) hypot(
                    (double) ( PtEdge->m_End0.x - PtEdge->m_Start0.x ),
                    (double) ( PtEdge->m_End0.y - PtEdge->m_Start0.y ) );
                fprintf( file, "CIRCLE %d %d %d\n",
                         PtEdge->m_Start0.x, Yaxis_sign * PtEdge->m_Start0.y,
                         rayon );
                break;
            }

            case S_ARC:         /* print ARC x,y start x,y end x,y centre */
            {
                int arcendx, arcendy;
                arcendx = PtEdge->m_Start0.x;
                arcendy = PtEdge->m_Start0.y;
                RotatePoint( &arcendx, &arcendy, PtEdge->m_Angle );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         PtEdge->m_End0.x, Yaxis_sign * PtEdge->m_End0.y,
                         arcendx, Yaxis_sign * arcendy,
                         PtEdge->m_Start0.x, Yaxis_sign * PtEdge->m_Start0.y );
                break;
            }

            default:
                DisplayError( NULL, wxT( "Type Edge Module inconnu" ) );
                break;
            }  /* end switch  PtEdge->m_Shape */

            break;

        default:
            break;
        }  /* End switch Items type */

    }
}
