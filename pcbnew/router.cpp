/****************************************/
/* EDITEUR de PCB: Menus d'AUTOROUTAGE: */
/****************************************/

// #define ROUTER

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "autorout.h"
#include "zones.h"
#include "cell.h"
#include "trigo.h"

#include "protos.h"

#define PSCALE 1

/* routines internes */
#ifdef ROUTER
static void Out_Pads( BOARD* Pcb, FILE* outfile );
static int  GenEdges( BOARD* Pcb, FILE* outfile );
#endif
static void GenExistantTracks( BOARD* Pcb, FILE* outfile, int current_net_code, int type );
static void ReturnNbViasAndTracks( BOARD* Pcb, int netcode, int* nb_vias, int* nb_tracks );

/* variables locales */
static int min_layer, max_layer;


/******************************************/
void WinEDA_PcbFrame::GlobalRoute( wxDC* DC )
/******************************************/
{
#ifdef ROUTER
    wxFileName fn;
    FILE*      outfile;
    wxString   ExecFileName, msg;
    int        ii;
    int        net_number;

#ifdef __UNIX__
    ExecFileName = FindKicadFile( wxT( "anneal" ) );
#else
    ExecFileName = FindKicadFile( wxT( "anneal.exe" ) );
#endif
    /* test de la presence du fichier et execution si present */
    if( !wxFileExists( ExecFileName ) )
    {
        msg.Printf( wxT( "File <%s> not found" ), ExecFileName.GetData() );
        DisplayError( this, msg, 20 );
        return;
    }

    /* Calcule du nom du fichier intermediaire de communication */
    fn = GetScreen()->m_FileName;
    fn.SetExt( wxT( "ipt" ) );

    if( ( outfile = wxFopen( fn.GetFullPath(), wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Unable to create temporary file " ) + fn.GetFullPath();
        DisplayError( this, msg, 20 );
        return;
    }

    msg = _( "Create temporary file " ) + fn.GetFullPath();
    SetStatusText( msg );

    /* calcul ratsnest */
    GetBoard()->m_Status_Pcb = 0;
    Compile_Ratsnest( DC, TRUE );

    GetBoard()->ComputeBoundaryBox();
    g_GridRoutingSize = GetScreen()->GetGrid().x;

    // Sortie de la dimension hors tout du pcb (dimensions + marge + g_GridRoutingSize)
#define B_MARGE 1000       // en 1/10000 inch
    fprintf( outfile, "j %d %d %d %d",
             ( -B_MARGE - g_GridRoutingSize + GetBoard()->m_BoundaryBox.GetX() ) / PSCALE,
             ( -B_MARGE - g_GridRoutingSize + GetBoard()->m_BoundaryBox.GetY() ) / PSCALE,
             ( B_MARGE + g_GridRoutingSize + GetBoard()->m_BoundaryBox.GetRight() ) / PSCALE,
             ( B_MARGE + g_GridRoutingSize + GetBoard()->m_BoundaryBox.GetBottom() ) / PSCALE );

    /* calcul du nombre de couches cuivre */
    min_layer = 1;  /* -> couche soudure = min layer */
    max_layer = GetBoard()->m_BoardSettings->m_CopperLayerCount;

    fprintf( outfile, " %d %d", min_layer, max_layer );

    net_number = GetBoard()->m_Equipots.GetCount();

    fprintf( outfile, " %d", net_number );

    fprintf( outfile, " %d", g_GridRoutingSize / PSCALE );

    fprintf( outfile, " %d %d %d", /* isolation Pad, track, via */
             g_DesignSettings.m_TrackClearance / PSCALE,
             g_DesignSettings.m_TrackClearance / PSCALE,
             g_DesignSettings.m_TrackClearance / PSCALE );


    fprintf( outfile, " 0" );                                                       /* via type */

    fprintf( outfile, " %d", g_DesignSettings.m_CurrentViaSize / PSCALE );          /* via diam */

    fprintf( outfile, " n" );                                                       /* via enterree non permise */

    fprintf( outfile, " 0" );                                                       /* unused */

    fprintf( outfile, " %d", g_DesignSettings.m_CurrentTrackWidth / PSCALE );       /* default track width */

    fprintf( outfile, " 0" );                                                       /* unused */

    fprintf( outfile, " 0 0 0\n" );                                                 /* unused */

    fprintf( outfile, "k 0 0 0 0 0 0 0 0 0 0\n" );                                  /* spare record */

    fprintf( outfile, "m 0 0 0 0 0 0 0 0 0 0\n" );                                  /* cost record */

    for( ii = min_layer; ii <= max_layer; ii++ )
    {
        int dir;
        dir = 3;                                        /* horizontal */
        if( ii & 1 )
            dir = 1;                                    /* vertical */
        fprintf( outfile, "l %d %d\n", ii, dir );       /* layer direction record */
    }

    Out_Pads( GetBoard(), outfile );
    GenEdges( GetBoard(), outfile );

    fclose( outfile );

    ExecFileName += wxT( " " ) + fn.GetFullPath();

    Affiche_Message( ExecFileName );

    ProcessExecute( ExecFileName );

#else
    wxMessageBox( wxT( "TODO, currently not available" ) );
#endif
}


/************************************************/
void Out_Pads( BOARD* Pcb, FILE* outfile )
/************************************************/
{
    D_PAD*     pt_pad;

    int        netcode, mod_num, nb_pads, plink;
    LISTE_PAD* pt_liste_pad, * pt_start_liste,
    * pt_end_liste, * pt_liste_pad_limite;
    int        pin_min_layer, pin_max_layer;
    int        no_conn = Pcb->GetPadsCount() + 1;/* valeur incrementee pour indiquer
                                 *  que le pad n'est pas deja connecte a une piste*/

    pt_liste_pad = pt_start_liste = &Pcb->m_NetInfo->m_PadsFullList[0];
    pt_liste_pad_limite = pt_start_liste + Pcb->GetPadsCount();

    if( pt_liste_pad == NULL )
        return;

    netcode = (*pt_liste_pad)->GetNet();
    nb_pads = 1;
    plink = 0;

    for( ; pt_liste_pad < pt_liste_pad_limite; )
    {
        /* Recherche de la fin de la liste des pads du net courant */
        for( pt_end_liste = pt_liste_pad + 1; ; pt_end_liste++ )
        {
            if( pt_end_liste >= pt_liste_pad_limite )
                break;
            if( (*pt_end_liste)->GetNet() != netcode )
                break;
            nb_pads++;
        }

        /* fin de liste trouvee : */
        if( netcode > 0 )
        {
            int nb_vias, nb_tracks;
            ReturnNbViasAndTracks( Pcb, netcode, &nb_vias, &nb_tracks );
            if( nb_pads < 2 )
            {
                wxString Line;
                NETINFO_ITEM* net = Pcb->FindNet( netcode );
                Line.Printf( wxT( "Warning: %d pad, net %s" ),
                            nb_pads, net->GetNetname().GetData() );
                DisplayError( NULL, Line, 20 );
            }
            fprintf( outfile, "r %d %d %d %d %d %d 1 1\n",
                     netcode, nb_pads, nb_vias + nb_pads, nb_tracks, 0,
                     g_DesignSettings.m_CurrentTrackWidth / PSCALE );
        }

        for( ; pt_liste_pad < pt_end_liste; pt_liste_pad++ )
        {
            pt_pad  = *pt_liste_pad;
            netcode = pt_pad->GetNet();
            plink   = pt_pad->GetSubNet();
            /* plink = numero unique si pad non deja connecte a une piste */
            if( plink <= 0 )
                plink = no_conn++;

            if( netcode <= 0 ) /* pad non connecte */
                fprintf( outfile, "u 0" );
            else
                fprintf( outfile, "p %d", netcode );
            /* position */
            fprintf( outfile, " %d %d",
                     pt_pad->m_Pos.x / PSCALE, pt_pad->m_Pos.y / PSCALE );

            /* layers */
            pin_min_layer = 0; pin_max_layer = max_layer;

            if( (pt_pad->m_Masque_Layer & ALL_CU_LAYERS) == CUIVRE_LAYER )
                pin_min_layer = pin_max_layer = min_layer;

            else if( (pt_pad->m_Masque_Layer & ALL_CU_LAYERS) == CMP_LAYER )
                pin_min_layer = pin_max_layer = max_layer;

            fprintf( outfile, " %d %d", pin_min_layer, pin_min_layer );

            /* link */
            fprintf( outfile, " %d", plink );

            /* type of device (1 = IC, 2 = edge conn, 3 = discret, 4 = other */
            switch( pt_pad->m_Attribut )
            {
            case PAD_STANDARD:
            case PAD_SMD:
                fprintf( outfile, " %d", 1 );
                break;

            case PAD_CONN:
                fprintf( outfile, " %d", 2 );
                break;

            case PAD_HOLE_NOT_PLATED:
                fprintf( outfile, " %d", 4 );
                break;
            }

            /* pin number */
            fprintf( outfile, " %ld", (long) (pt_pad->m_Padname) );

            /* layer size number (tj = 1) */
            fprintf( outfile, " %d", 1 );

            /* device number */
            mod_num = 1;    /* A CHANGER */
            fprintf( outfile, " %d", mod_num );

            /* orientations pins 1 et 2 */
            fprintf( outfile, " x" );

            /* spare */
            fprintf( outfile, " 0 0\n" );

            /* output layer size record */
            fprintf( outfile, "q" );

            switch( pt_pad->m_PadShape )  /* out type, dims */
            {
            case PAD_CIRCLE:
                fprintf( outfile, " c 0 %d 0",
                         pt_pad->m_Size.x / PSCALE );
                break;

            case PAD_OVAL:
            case PAD_RECT:
            case PAD_TRAPEZOID:
                int lmax = pt_pad->m_Size.x;
                int lmin  = pt_pad->m_Size.y;
                int angle = pt_pad->m_Orient / 10;
                while( angle < 0 )
                    angle += 180;

                while( angle > 180 )
                    angle -= 180;

                while( angle > 135 )
                {
                    angle -= 90;
                    EXCHG( lmax, lmin );
                }

                fprintf( outfile, " r %d %d %d",
                         angle,
                         lmax / PSCALE, lmin / PSCALE );
                break;
            }

            /* layers */
            fprintf( outfile, " %d %d\n", pin_min_layer, pin_max_layer );
        }

        /* fin generation liste pads pour 1 net */

        if( netcode )
        {
            GenExistantTracks( Pcb, outfile, netcode, TYPE_VIA );
            GenExistantTracks( Pcb, outfile, netcode, TYPE_TRACK );
        }

        nb_pads      = 1;
        pt_liste_pad = pt_start_liste = pt_end_liste;
        if( pt_start_liste < pt_liste_pad_limite )
            netcode = (*pt_start_liste)->GetNet();
    }
}


/**************************************************************************/
void ReturnNbViasAndTracks( BOARD* Pcb, int netcode, int* nb_vias,
                                   int* nb_tracks )
/**************************************************************************/

/* calcule le nombre de vias et segments de pistes pour le net netcode
 */
{
    TRACK* track;

    *nb_vias   = 0;
    *nb_tracks = 0;

    track = Pcb->m_Track;
    if( track == NULL )
        return;

    for( ; track != NULL; track = track->Next() )
    {
        if( track->GetNet() > netcode )
            return;
        if( track->GetNet() != netcode )
            continue;
        if( track->Type() == TYPE_VIA )
            (*nb_vias)++;
        if( track->Type() == TYPE_TRACK )
            (*nb_tracks)++;
    }
}


/*************************************************************/
void GenExistantTracks( BOARD* Pcb, FILE* outfile,
                               int current_net_code, int type )
/*************************************************************/
/* generation des pistes existantes */
{
    int    netcode, plink;
    int    via_min_layer, via_max_layer;
    TRACK* track;


    track = Pcb->m_Track;
    if( track == NULL )
        return;


    for( ; track != NULL; track = track->Next() )
    {
        netcode = track->GetNet();
        if( netcode > current_net_code )
            return;
        if( netcode != current_net_code )
            continue;
        plink = track->GetSubNet();
        via_min_layer = track->GetLayer();

        if( track->Type() != type )
            continue;

        if( track->Type() == TYPE_VIA )
        {
            via_min_layer &= 15;
            via_max_layer  = (track->GetLayer() >> 4) & 15;

            if( via_min_layer == via_max_layer )
            {
                track->SetLayer( 0xF );
                via_min_layer  = 0; via_max_layer = 15;
            }

            if( via_max_layer < via_min_layer )
                EXCHG( via_max_layer, via_min_layer );

            if( via_max_layer == CMP_N )
                via_max_layer = max_layer;
            else
                via_max_layer++;

            if( via_min_layer == COPPER_LAYER_N )
                via_min_layer = min_layer;
            else
                via_min_layer++;

            fprintf( outfile, "v 0 " );
            fprintf( outfile, " %d %d", track->m_Start.x / PSCALE, track->m_Start.y / PSCALE );
            fprintf( outfile, " %d %d", via_min_layer, via_max_layer );
            fprintf( outfile, " %d", plink );
            fprintf( outfile, " %d\n", 1 );
            /* layer size record */
            fprintf( outfile, "q c 0 %d 0 0 0\n", track->m_Width / PSCALE );
        }

        if( track->Type() == TYPE_TRACK )
        {
            fprintf( outfile, "t 0 %d", track->m_Width / PSCALE );
            fprintf( outfile, " %d %d", track->m_Start.x / PSCALE, track->m_Start.y / PSCALE );
            fprintf( outfile, " %d %d", track->m_End.x / PSCALE, track->m_End.y / PSCALE );

            if( via_min_layer == CMP_N )
                via_min_layer = max_layer;
            else
                via_min_layer++;
            fprintf( outfile, " %d", via_min_layer );

            fprintf( outfile, " %d\n", 1 );  /*Number of corners */

            /* output corner */
            fprintf( outfile, "c" );
            fprintf( outfile, " %d %d\n", track->m_End.x / PSCALE, track->m_End.y / PSCALE );
        }
    }
}


/***********************************************/
int GenEdges( BOARD* Pcb, FILE* outfile )
/***********************************************/

/* Generation des contours (edges).
 *  Il sont générés comme des segments de piste placés sur chaque couche routable.
 */
{
#define NB_CORNERS 4

    EDA_BaseStruct* PtStruct;
    DRAWSEGMENT*    PtDrawSegment;
    int             ii;
    int             dx, dy, width, angle;
    int             ox, oy, fx, fy;
    wxPoint         lim[4];
    int             NbItems = 0;

    /* impression des contours  */
    for( PtStruct = Pcb->m_Drawings; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        if( PtStruct->Type() != TYPE_DRAWSEGMENT )
            continue;

        PtDrawSegment = (DRAWSEGMENT*) PtStruct;
        if( PtDrawSegment->GetLayer() != EDGE_N )
            continue;

        fx = PtDrawSegment->m_End.x; ox = PtDrawSegment->m_Start.x;
        fy = PtDrawSegment->m_End.y; oy = PtDrawSegment->m_Start.y;
        dx = fx - ox; dy = fy - oy;
        if( (dx == 0) && (dy == 0) )
            continue;

        /* elimination des segments non horizontaux, verticaux ou 45 degres,
         *  non gérés par l'autorouteur */
        if( (dx != 0) && (dy != 0) && ( abs( dx ) != abs( dy ) ) )
            continue;

        NbItems++;
        if( outfile == NULL )
            continue;                       /* car simple decompte des items */


        /* generation de la zone interdite */
        if( dx < 0 )
        {
            EXCHG( ox, fx ); EXCHG( oy, fy ); dx = -dx; dy = -dy;
        }
        if( (dx == 0) && (dy < 0 ) )
        {
            EXCHG( oy, fy ); dy = -dy;
        }

        width  = PtDrawSegment->m_Width;
        width += g_GridRoutingSize;

        angle = -ArcTangente( dy, dx );
        if( angle % 450 )/* not H, V or X */
        {
            wxBell();
            continue;
        }

        /* 1er point */
        dx = -width; dy = -width;
        RotatePoint( &dx, &dy, angle );
        lim[0].x = ox + dx;
        lim[0].y = oy + dy;

        /* 2eme point */
        RotatePoint( &dx, &dy, -900 );
        lim[1].x = fx + dx;
        lim[1].y = fy + dy;

        /* 3eme point */
        RotatePoint( &dx, &dy, -900 );
        lim[2].x = fx + dx;
        lim[2].y = fy + dy;

        /* 4eme point */
        RotatePoint( &dx, &dy, -900 );
        lim[3].x = ox + dx;
        lim[3].y = oy + dy;

        if( angle % 900 )
        {
        }
        /* mise a l'echelle */
        for( ii = 0; ii < 4; ii++ )
        {
            lim[ii].x = (int) ( ( (double) lim[ii].x + 0.5 ) / PSCALE );
            lim[ii].y = (int) ( ( (double) lim[ii].y + 0.5 ) / PSCALE );
        }

        /* sortie du 1er point */
        fprintf( outfile, "n %d %d %ld %ld %d\n",
                 0, /* layer number, 0 = all layers */
                 0, /* -1 (no via), 0 (no via no track), 1 (no track) */
                 (long) lim[0].x, (long) lim[0].y,
                 NB_CORNERS );

        /* sortie des autres points */
        for( ii = 1; ii < 4; ii++ )
        {
            fprintf( outfile, "c %ld %ld\n",
                     (long) lim[ii].x, (long) lim[ii].y );
        }
    }

    return NbItems;
}


/****************************************************/
void WinEDA_PcbFrame::ReadAutoroutedTracks( wxDC* DC )
/****************************************************/
{
    wxFileName fn;
    char       Line[1024];
    wxString   msg;
    int        LineNum = 0, NbTrack = 0, NetCode = 0;
    FILE*      File;
    TRACK*     newTrack;
    SEGVIA*    newVia;
    int        track_count, track_layer, image, track_width;
    int        via_layer1, via_layer2, via_size;
    wxPoint    track_start, track_end;
    int        max_layer = GetBoard()->m_BoardSettings->m_CopperLayerCount;

    /* Calcule du nom du fichier intermediaire de communication */
    fn = GetScreen()->m_FileName;
    fn.SetExt( wxT( "trc" ) );

    if( ( File = wxFopen( fn.GetFullPath(), wxT( "rt" ) ) ) == NULL )
    {
        msg = _( "Unable to find data file " ) + fn.GetFullPath();
        DisplayError( this, msg, 20 );
        return;
    }
    else
    {
        msg = _( "Reading autorouter data file " ) + fn.GetFullPath();
        Affiche_Message( msg );
    }

    SetLocaleTo_C_standard( );

    track_width = g_DesignSettings.m_CurrentTrackWidth;
    via_size    = g_DesignSettings.m_CurrentViaSize;
    while( GetLine( File, Line, &LineNum ) != NULL )
    {
        char ident = Line[0];

        switch( ident )
        {
        case 'j':    // Header, not used
            break;

        case 'R':    // Net record
            sscanf( Line + 2, "%d", &NetCode );
            break;

        case 'V':    // via record: fmt = V symbol pos_x pos_y layer1 layer2
            sscanf( Line + 2, "%d %d %d %d %d", &image,
                    &track_start.x, &track_start.y, &via_layer1, &via_layer2 );
            via_layer1--; via_layer2--;
            if( via_layer1 == max_layer - 1 )
                via_layer1 = CMP_N;
            if( via_layer2 == max_layer - 1 )
                via_layer2 = CMP_N;
            newVia = new SEGVIA( GetBoard() );

            newVia->m_Start = newVia->m_End = track_start;
            newVia->m_Width = via_size;
            newVia->SetLayer( via_layer1 + (via_layer2 << 4) );
            if( newVia->GetLayer() == 0x0F || newVia->GetLayer() == 0xF0 )
                newVia->m_Shape = VIA_THROUGH;
            else
                newVia->m_Shape = VIA_BLIND_BURIED;

            GetBoard()->m_Track.PushFront( newVia );
            NbTrack++;
            break;

        case 'T':    // Track list start: fmt = T image layer t_count
            sscanf( Line + 2, "%d %d %d", &image, &track_layer, &track_count );
            track_layer--;
            if( (track_layer != COPPER_LAYER_N) && (track_layer == max_layer - 1) )
                track_layer = CMP_N;

            // Read corners: fmt = C x_pos y_pos
            for( int ii = 0; ii < track_count; ii++ )
            {
                if( GetLine( File, Line, &LineNum ) != NULL )
                {
                    if( Line[0] != 'C' )
                        break;
                    if( ii == 0 )
                        sscanf( Line + 2, "%d %d", &track_start.x, &track_start.y );
                    else
                    {
                        sscanf( Line + 2, "%d %d", &track_end.x, &track_end.y );
                        newTrack = new TRACK( GetBoard() );

                        newTrack->m_Width = track_width;
                        newTrack->SetLayer( track_layer );
                        newTrack->m_Start = track_start;
                        newTrack->m_End   = track_end;
                        track_start = track_end;

                        GetBoard()->m_Track.PushFront( newTrack );
                        NbTrack++;
                    }
                }
                else
                    break;
            }

            break;

        default:
            break;
        }
    }

    fclose( File );

    SetLocaleTo_Default( );

    if( NbTrack == 0 )
        DisplayError( this, wxT( "Warning: No tracks" ), 10 );
    else
    {
        GetBoard()->m_Status_Pcb = 0;
        GetScreen()->SetModify();
    }

    Compile_Ratsnest( DC, TRUE );
    if( NbTrack )
        GetScreen()->SetRefreshReq();
}
