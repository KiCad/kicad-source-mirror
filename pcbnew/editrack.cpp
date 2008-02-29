/*******************************/
/* Edition des pistes			*/
/* Routines de trace de pistes */
/*******************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "protos.h"
#include "drc_stuff.h"
#include "trigo.h"



/* Routines Locales */
static void     Exit_Editrack( WinEDA_DrawPanel* panel, wxDC* DC );
void            ShowNewTrackWhenMovingCursor( WinEDA_DrawPanel* panel,
                                              wxDC* DC, bool erase );
static void     ComputeBreakPoint( TRACK* track, int n, wxPoint end );
static TRACK*   DeleteNullTrackSegments( BOARD* pcb, TRACK* track, int* segmcount );
static void     EnsureEndTrackOnPad( D_PAD* Pad );

/* variables locales */
static int OldNetCodeSurbrillance;
static int OldEtatSurbrillance;


/************************************************************/
static void Exit_Editrack( WinEDA_DrawPanel* Panel, wxDC* DC )
/************************************************************/

/* routine d'annulation de la Commande Begin_Route si une piste est en cours
 *  de tracage, ou de sortie de l'application EDITRACK.
 */
{
    WinEDA_PcbFrame* frame = (WinEDA_PcbFrame*) Panel->m_Parent;
    TRACK*           track = (TRACK*) frame->GetCurItem();

    if( track && ( track->Type()==TYPEVIA || track->Type()==TYPETRACK ) )
    {
        /* Erase the current drawing */
        ShowNewTrackWhenMovingCursor( Panel, DC, FALSE );
        if( g_HightLigt_Status )
            frame->Hight_Light( DC );

        g_HightLigth_NetCode = OldNetCodeSurbrillance;
        if( OldEtatSurbrillance )
            frame->Hight_Light( DC );

        frame->MsgPanel->EraseMsgBox();

        // Delete current (new) track
        TRACK* previoustrack;
        for(  ;   track;   track = previoustrack )
        {
            previoustrack = (TRACK*) track->Pback;
            delete track;
        }
    }

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    frame->SetCurItem( NULL );
}


/*************************************************************/
TRACK* WinEDA_PcbFrame::Begin_Route( TRACK* track, wxDC* DC )
/*************************************************************/

/*
 *  Routine d'initialisation d'un trace de piste et/ou de mise en place d'un
 *  nouveau point piste
 *
 *  Si pas de piste en cours de trace:
 *      - Recherche de netname de la nouvelle piste ( pad de depart out netname
 *       de la piste si depart sur une ancienne piste
 *      - Met en surbrillance tout le net
 *      - Initilise les divers pointeurs de trace
 *  Si piste en cours:
 *      - controle DRC
 *      - si DRC OK : addition d'un nouveau point piste
 */
{
    D_PAD*          pt_pad = NULL;
    TRACK*          TrackOnStartPoint = NULL, * Track;
    int             masquelayer = g_TabOneLayerMask[((PCB_SCREEN*)GetScreen())->m_Active_Layer];
    EDA_BaseStruct* LockPoint;
    wxPoint         pos = GetScreen()->m_Curseur;
    static int      InitialTrackWidthValue; /* first track segment width.
                                            * used when we are in the auto tack width mode */

    DrawPanel->ManageCurseur = ShowNewTrackWhenMovingCursor;
    DrawPanel->ForceCloseManageCurseur = Exit_Editrack;

    if( track == NULL )  /* Starting a new track  */
    {
        /* undrw old hightlight */
        OldNetCodeSurbrillance = g_HightLigth_NetCode;
        OldEtatSurbrillance    = g_HightLigt_Status;

        if( g_HightLigt_Status )
            Hight_Light( DC );

        InitialTrackWidthValue = -1;        // Set to "no value"
        g_FirstTrackSegment    = g_CurrentTrackSegment = new TRACK( m_Pcb );
        g_CurrentTrackSegment->m_Flags = IS_NEW;
        g_TrackSegmentCount  = 1;
        g_HightLigth_NetCode = 0;

        /* Search for a pad at starting point of the new track: */
        LockPoint = LocateLockPoint( m_Pcb, pos, masquelayer );

        if( LockPoint ) // An item (pad or track) is found
        {
            if( LockPoint->Type() == TYPEPAD )
            {
                pt_pad = (D_PAD*) LockPoint;

                /* A pad is found: put the starting point on pad centre */
                pos = pt_pad->m_Pos;
                g_HightLigth_NetCode = pt_pad->GetNet();
            }
            else /* A track segment is found */
            {
                TrackOnStartPoint    = (TRACK*) LockPoint;
                g_HightLigth_NetCode = TrackOnStartPoint->GetNet();
                CreateLockPoint( &pos.x, &pos.y, TrackOnStartPoint, NULL );
            }
        }

        build_ratsnest_pad( LockPoint, wxPoint( 0, 0 ), TRUE );
        Hight_Light( DC );

        g_CurrentTrackSegment->m_Flags = IS_NEW;
        g_CurrentTrackSegment->SetLayer( ((PCB_SCREEN*)GetScreen())->m_Active_Layer );
        g_CurrentTrackSegment->m_Width = g_DesignSettings.m_CurrentTrackWidth;
        if( g_DesignSettings.m_UseConnectedTrackWidth )
        {
            if( TrackOnStartPoint && TrackOnStartPoint->Type() == TYPETRACK )
            {
                InitialTrackWidthValue = TrackOnStartPoint->m_Width;
                g_CurrentTrackSegment->m_Width = InitialTrackWidthValue;
            }
        }
        g_CurrentTrackSegment->m_Start = pos;
        g_CurrentTrackSegment->m_End   = g_CurrentTrackSegment->m_Start;
        g_CurrentTrackSegment->SetNet( g_HightLigth_NetCode );
        if( pt_pad )
        {
            g_CurrentTrackSegment->start = pt_pad;
            g_CurrentTrackSegment->SetState( BEGIN_ONPAD, ON );
        }
        else
            g_CurrentTrackSegment->start = TrackOnStartPoint;

        if( g_TwoSegmentTrackBuild )
        {
            // Create 2 segments
            g_CurrentTrackSegment = g_CurrentTrackSegment->Copy();
            g_TrackSegmentCount++;
            g_CurrentTrackSegment->Pback = g_FirstTrackSegment;
            g_FirstTrackSegment->Pnext   = g_CurrentTrackSegment;
            g_CurrentTrackSegment->start = g_FirstTrackSegment;
            g_FirstTrackSegment->end = g_CurrentTrackSegment;
            g_FirstTrackSegment->SetState( BEGIN_ONPAD | END_ONPAD, OFF );
        }
        g_CurrentTrackSegment->Display_Infos( this );
        SetCurItem( g_CurrentTrackSegment );
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

        if( Drc_On )
        {
            if( BAD_DRC == m_drc->Drc( g_CurrentTrackSegment, m_Pcb->m_Track ) )
            {
                return g_CurrentTrackSegment;
            }
        }
    }
    else    /* Track in progress : segment coordinates are updated by ShowNewTrackWhenMovingCursor*/
    {
        /* Tst for a D.R.C. error: */
        if( Drc_On )
        {
            if( BAD_DRC == m_drc->Drc( g_CurrentTrackSegment, m_Pcb->m_Track ) )
                return NULL;

            if( g_TwoSegmentTrackBuild     // We must handle 2 segments
               && g_CurrentTrackSegment->Back() )
            {
                if( BAD_DRC == m_drc->Drc( g_CurrentTrackSegment->Back(), m_Pcb->m_Track ) )
                    return NULL;
            }
        }

        /* Current track is Ok: current segment is kept, and a new one is created
         *  unless the current segment is null, or 2 last are null if a 2 segments track build
         */
        bool CanCreateNewSegment = TRUE;
        if( !g_TwoSegmentTrackBuild && g_CurrentTrackSegment->IsNull() )
            CanCreateNewSegment = FALSE;
        if( g_TwoSegmentTrackBuild && g_CurrentTrackSegment->IsNull()
           && g_CurrentTrackSegment->Back() && g_CurrentTrackSegment->Back()->IsNull() )
            CanCreateNewSegment = FALSE;
        if( CanCreateNewSegment )
        {
            /* Erase old track on screen */
            ShowNewTrackWhenMovingCursor( DrawPanel, DC, FALSE );

            if( g_Raccord_45_Auto )
            {
                if( Add_45_degrees_Segment( DC, g_CurrentTrackSegment ) != 0 )
                    g_TrackSegmentCount++;
            }
            Track = g_CurrentTrackSegment->Copy();
            Track->Insert( m_Pcb, g_CurrentTrackSegment );

            Track->SetState( BEGIN_ONPAD | END_ONPAD, OFF );
            g_CurrentTrackSegment->end = Locate_Pad_Connecte( m_Pcb, g_CurrentTrackSegment, END );
            if( g_CurrentTrackSegment->end )
            {
                g_CurrentTrackSegment->SetState( END_ONPAD, ON );
                Track->SetState( BEGIN_ONPAD, ON );
            }
            Track->start = g_CurrentTrackSegment->end;

            g_CurrentTrackSegment = Track;
            g_CurrentTrackSegment->m_Flags = IS_NEW;
            g_TrackSegmentCount++;
            g_CurrentTrackSegment->m_Start = g_CurrentTrackSegment->m_End;
            g_CurrentTrackSegment->SetLayer( ((PCB_SCREEN*)GetScreen())->m_Active_Layer );
            if( !g_DesignSettings.m_UseConnectedTrackWidth )
            {
                g_CurrentTrackSegment->m_Width = g_DesignSettings.m_CurrentTrackWidth;
            }
            /* Show the new position */
            ShowNewTrackWhenMovingCursor( DrawPanel, DC, FALSE );
        }
        g_CurrentTrackSegment->Display_Infos( this );
    }

    SetCurItem( g_CurrentTrackSegment );
    return g_CurrentTrackSegment;
}


/**************************************************************************/
int WinEDA_PcbFrame::Add_45_degrees_Segment( wxDC* DC, TRACK* pt_segm )
/***************************************************************************/

/* rectifie un virage a 90 et le modifie par 2 coudes a 45
 *  n'opere que sur des segments horizontaux ou verticaux.
 *
 *  entree : pointeur sur le segment qui vient d'etre trace
 *          On suppose que le segment precedent est celui qui a ete
 *          precedement trace
 *  retourne:
 *      1 si ok
 *      0 si impossible
 */
{
    TRACK* Previous;
    TRACK* NewTrack;
    int    pas_45;
    int    dx0, dy0, dx1, dy1;

    if( g_TrackSegmentCount < 2 )
        return 0;                               /* il faut au moins 2 segments */

    Previous = (TRACK*) pt_segm->Pback;         // pointe le segment precedent

    // Test s'il y a 2 segments consecutifs a raccorder
    if( (pt_segm->Type() != TYPETRACK )
       || (Previous->Type() != TYPETRACK) )
    {
        return 0;
    }

    pas_45 = GetScreen()->GetGrid().x / 2;
    if( pas_45 < pt_segm->m_Width )
        pas_45 = GetScreen()->GetGrid().x;

    while( pas_45 < pt_segm->m_Width )
        pas_45 *= 2;

    // OK : tst si les segments sont a 90 degre et vertic ou horiz
    dx0 = Previous->m_End.x - Previous->m_Start.x;
    dy0 = Previous->m_End.y - Previous->m_Start.y;
    dx1 = pt_segm->m_End.x - pt_segm->m_Start.x;
    dy1 = pt_segm->m_End.y - pt_segm->m_Start.y;

    // les segments doivent etre de longueur suffisante:
    if( MAX( abs( dx0 ), abs( dy0 ) ) < (pas_45 * 2) )
        return 0;

    if( MAX( abs( dx1 ), abs( dy1 ) ) < (pas_45 * 2) )
        return 0;

    /* creation du nouveau segment, raccordant des 2 segm: */
    NewTrack = pt_segm->Copy();

    NewTrack->m_Start.x = Previous->m_End.x;
    NewTrack->m_Start.y = Previous->m_End.y;
    NewTrack->m_End.x   = pt_segm->m_Start.x;
    NewTrack->m_End.y   = pt_segm->m_Start.y;

    if( dx0 == 0 )          // Segment precedent Vertical
    {
        if( dy1 != 0 )      // les 2 segments ne sont pas a 90 ;
        {
            delete NewTrack;
            return 0;
        }

        /* Calcul des coordonnees du point de raccord :
         *  le nouveau segment raccorde le 1er segment Vertical
         *  au 2eme segment Horizontal */

        if( dy0 > 0 )
            NewTrack->m_Start.y -= pas_45;
        else
            NewTrack->m_Start.y += pas_45;

        if( dx1 > 0 )
            NewTrack->m_End.x += pas_45;
        else
            NewTrack->m_End.x -= pas_45;

        if( Drc_On && BAD_DRC==m_drc->Drc( pt_segm, m_Pcb->m_Track ) )
        {
            delete NewTrack;
            return 0;
        }

        Previous->m_End  = NewTrack->m_Start;
        pt_segm->m_Start = NewTrack->m_End;
        NewTrack->Insert( m_Pcb, Previous );
        return 1;
    }

    if( dy0 == 0 )      // Segment precedent Horizontal : dy0 = 0
    {
        if( dx1 != 0 )  // les 2 segments ne sont pas a 90 ;
        {
            delete NewTrack;
            return 0;
        }

        // Segments a 90

        /* Modif des coordonnees du point de raccord :
         *  un nouveau segment a ete cree, raccordant le 1er segment Horizontal
         *  au 2eme segment Vertical */

        if( dx0 > 0 )
            NewTrack->m_Start.x -= pas_45;
        else
            NewTrack->m_Start.x += pas_45;

        if( dy1 > 0 )
            NewTrack->m_End.y += pas_45;
        else
            NewTrack->m_End.y -= pas_45;

        if( Drc_On && BAD_DRC==m_drc->Drc( NewTrack, m_Pcb->m_Track ) )
        {
            delete NewTrack;
            return 0;
        }

        Previous->m_End  = NewTrack->m_Start;
        pt_segm->m_Start = NewTrack->m_End;
        NewTrack->Insert( m_Pcb, Previous );
        return 1;
    }

    return 0;
}


/**************************************************************/
void WinEDA_PcbFrame::End_Route( TRACK* track, wxDC* DC )
/*************************************************************/

/*
 *  Routine de fin de trace d'une piste (succession de segments)
 */
{
    int             masquelayer = g_TabOneLayerMask[((PCB_SCREEN*)GetScreen())->m_Active_Layer];
    wxPoint         pos;
    EDA_BaseStruct* LockPoint;
    TRACK*          adr_buf;

    if( track == NULL )
        return;

    if( Drc_On && BAD_DRC==m_drc->Drc( g_CurrentTrackSegment, m_Pcb->m_Track ) )
        return;

    /* Sauvegarde des coord du point terminal de la piste */
    pos = g_CurrentTrackSegment->m_End;

    if( Begin_Route( track, DC ) == NULL )
        return;

    ShowNewTrackWhenMovingCursor( DrawPanel, DC, TRUE );    /* mise a jour trace reel */
    ShowNewTrackWhenMovingCursor( DrawPanel, DC, FALSE );   /* efface trace piste*/
    trace_ratsnest_pad( DC );                               /* efface trace chevelu*/


    // cleanup
    if( g_CurrentTrackSegment->Pnext != NULL )
    {
        delete g_CurrentTrackSegment->Pnext;
        g_CurrentTrackSegment->Pnext = NULL;
    }


    /* La piste est ici non chainee a la liste des segments de piste.
     *  Il faut la replacer dans la zone de net,
     *  le plus pres possible du segment d'attache ( ou de fin ), car
     *  ceci contribue a la reduction du temps de calcul */

    /* Accrochage de la fin de la piste */
    LockPoint = LocateLockPoint( m_Pcb, pos, masquelayer );

    if( LockPoint ) /* La fin de la piste est sur un PAD */
    {
        if( LockPoint->Type() ==  TYPEPAD )
        {
            EnsureEndTrackOnPad( (D_PAD*) LockPoint );
        }
        else    /* la fin de la piste est sur une autre piste: il faudra
                 *  peut-etre creer un point d'ancrage */
        {
            adr_buf = (TRACK*) LockPoint;
            g_HightLigth_NetCode = adr_buf->GetNet();

            /* creation eventuelle d'un point d'accrochage */
            LockPoint = CreateLockPoint( &g_CurrentTrackSegment->m_End.x,
                                         &g_CurrentTrackSegment->m_End.y,
                                         adr_buf,
                                         g_CurrentTrackSegment );
        }
    }


    // Delete Null segments:
    g_FirstTrackSegment = DeleteNullTrackSegments( m_Pcb,
                                                   g_FirstTrackSegment,
                                                   &g_TrackSegmentCount );
    /* Test if no segment left. Can happend on a double click on the start point */
    if( g_FirstTrackSegment != NULL )
    {
        // Put new track in board
        m_Pcb->Add( g_FirstTrackSegment );

        trace_ratsnest_pad( DC );
        Trace_Une_Piste( DrawPanel, DC, g_FirstTrackSegment, g_TrackSegmentCount, GR_OR );

        // Reset flags:
        TRACK* ptr = g_FirstTrackSegment; int ii;
        for( ii = 0; (ptr != NULL) && (ii < g_TrackSegmentCount); ii++ )
        {
            ptr->m_Flags = 0;
            ptr = ptr->Next();
        }

        /* Delete the old track, if exists */
        if( g_AutoDeleteOldTrack )
        {
            EraseOldTrack( this, m_Pcb, DC, g_FirstTrackSegment, g_TrackSegmentCount );
        }

        /* compute the new rastnest : */
        test_1_net_connexion( DC, g_FirstTrackSegment->GetNet() );

        GetScreen()->SetModify();
        m_Pcb->Display_Infos( this );
    }
    /* Finish the work, clear used variables */
    g_FirstTrackSegment = NULL;

    if( g_HightLigt_Status )
        Hight_Light( DC );

    g_HightLigth_NetCode = OldNetCodeSurbrillance;
    if( OldEtatSurbrillance )
        Hight_Light( DC );

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    SetCurItem( NULL );
}


TRACK* LocateIntrusion( TRACK* start, int net, int width )
{
    int     layer = ( (PCB_SCREEN*) ActiveScreen )->m_Active_Layer;

    wxPoint ref = ActiveScreen->RefPos( true );

    TRACK*  found = NULL;

    for( TRACK* track = start;  track;  track = track->Next() )
    {
        if( track->Type() == TYPETRACK )    // skip vias
        {
            if( track->GetState( BUSY | DELETED ) )
                continue;

            if( layer != track->GetLayer() )
                continue;

            if( track->GetNet() == net )
                continue;

            /* TRACK::HitTest */
            int dist = width / 2 + track->m_Width / 2 + g_DesignSettings.m_TrackClearence;

            wxPoint pos  = ref - track->m_Start;
            wxPoint vec  = track->m_End - track->m_Start;

            if( !DistanceTest( dist, vec.x, vec.y, pos.x, pos.y ) )
                continue;

            found = track;

            /* prefer intrusions from the side, not the end */
            double tmp = (double) pos.x * vec.x + (double) pos.y * vec.y;
            if( tmp >= 0 && tmp <= (double) vec.x * vec.x + (double) vec.y * vec.y )
                break;
        }
    }

    return found;
}


/**
 * Function PushTrack
 * detects if the mouse is pointing into a conflicting track.
 * In this case, it tries to push the new track out of the conflicting track's
 * clearance zone. This gives us a cheap mechanism for drawing tracks that
 * tightly follow others, independent of grid settings.
 *
 * KNOWN BUGS:
 * - we do the same sort of search and calculation up to three times:
 *   1) we search for magnetic hits (in controle.cpp)
 *   2) we check if there's a DRC violation in the making (also controle.cpp)
 *   3) we try to fix the DRC violation (here)
 * - if we have a magnetic hit and a DRC violation at the same time, we choose
 *   the magnetic hit instead of solving the violation
 * - should locate conflicting tracks also when we're crossing over them
 */
static void PushTrack( WinEDA_DrawPanel* panel )
{
    BOARD*  pcb    = ( (WinEDA_BasePcbFrame*) (panel->m_Parent) )->m_Pcb;
    wxPoint cursor = ActiveScreen->m_Curseur;
    wxPoint cv, vec, n;
    TRACK*  track = g_CurrentTrackSegment;
    TRACK*  other;
    double  det;
    int     dist;
    double  f;

    other = LocateIntrusion( pcb->m_Track, track->GetNet(), track->m_Width );

    /* are we currently pointing into a conflicting trace ? */
    if( !other )
        return;

    if( other->GetNet() == track->GetNet() )
        return;

    cv  = cursor - other->m_Start;
    vec = other->m_End - other->m_Start;

    det = (double) cv.x * vec.y - (double) cv.y * vec.x;

    /* cursor is right at the center of the old track */
    if( !det )
        return;

    dist = (track->m_Width + 1) / 2 + (other->m_Width + 1) / 2 +
           g_DesignSettings.m_TrackClearence + 2;

    /*
     * DRC wants >, so +1.
     * We may have a quantization error of 1/sqrt(2), so +1 again.
     */

    /* Vector "n" is perpendicular to "other", pointing towards the cursor. */
    if( det > 0 )
    {
        n.x = vec.y;
        n.y = -vec.x;
    }
    else
    {
        n.x = -vec.y;
        n.y = vec.x;
    }
    f   = dist / hypot( double(n.x), double(n.y) );
    n.x = (int) round( f * n.x );
    n.y = (int) round( f * n.y );

    Project( &track->m_End, cursor, other );
    track->m_End += n;
}


/****************************************************************************/
void ShowNewTrackWhenMovingCursor( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/****************************************************************************/

/* redessin du contour de la piste  lors des deplacements de la souris
 *  Cette routine est utilisee comme .ManageCurseur()
 *  si ShowIsolDuringCreateTrack_Item.State == RUN la marge d'isolation
 *  est aussi affichee
 */
{
    int         IsolTmp, Track_fill_copy;
    PCB_SCREEN* screen = (PCB_SCREEN*) panel->GetScreen();

    Track_fill_copy = DisplayOpt.DisplayPcbTrackFill;
    DisplayOpt.DisplayPcbTrackFill = SKETCH;
    IsolTmp = DisplayOpt.DisplayTrackIsol;

    if( g_ShowIsolDuringCreateTrack )
        DisplayOpt.DisplayTrackIsol = TRUE;

    /* efface ancienne position si elle a ete deja dessinee */
    if( erase )
    {
        Trace_Une_Piste( panel, DC, g_FirstTrackSegment, g_TrackSegmentCount, GR_XOR );
        ( (WinEDA_BasePcbFrame*) (panel->m_Parent) )->trace_ratsnest_pad( DC );
    }

    /* dessin de la nouvelle piste : mise a jour du point d'arrivee */
    g_CurrentTrackSegment->SetLayer( screen->m_Active_Layer );
    if( ! g_DesignSettings.m_UseConnectedTrackWidth )
        g_CurrentTrackSegment->m_Width = g_DesignSettings.m_CurrentTrackWidth;
    if( g_TwoSegmentTrackBuild )
    {
        TRACK* previous_track = (TRACK*) g_CurrentTrackSegment->Pback;
        if( previous_track && (previous_track->Type() == TYPETRACK) )
        {
            previous_track->SetLayer( screen->m_Active_Layer );
            if( ! g_DesignSettings.m_UseConnectedTrackWidth )
                previous_track->m_Width = g_DesignSettings.m_CurrentTrackWidth;
        }
    }

    if( Track_45_Only )
    {
        if( g_TwoSegmentTrackBuild ) {
        g_CurrentTrackSegment->m_End = ActiveScreen->m_Curseur;
        if (Drc_On)
        PushTrack(panel);
            ComputeBreakPoint( g_CurrentTrackSegment, g_TrackSegmentCount,
           g_CurrentTrackSegment->m_End);
    }
        else
        {
            /* Calcul de l'extremite de la piste pour orientations permises:
             *                              horiz,vertical ou 45 degre */
            Calcule_Coord_Extremite_45( g_CurrentTrackSegment->m_Start.x,
                                        g_CurrentTrackSegment->m_Start.y,
                                        &g_CurrentTrackSegment->m_End.x,
                                        &g_CurrentTrackSegment->m_End.y );
        }
    }
    else    /* ici l'angle d'inclinaison est quelconque */
    {
        g_CurrentTrackSegment->m_End = screen->m_Curseur;
    }

    Trace_Une_Piste( panel, DC, g_FirstTrackSegment, g_TrackSegmentCount, GR_XOR );

    DisplayOpt.DisplayTrackIsol    = IsolTmp;
    DisplayOpt.DisplayPcbTrackFill = Track_fill_copy;

    ( (WinEDA_BasePcbFrame*) (panel->m_Parent) )->
    build_ratsnest_pad( NULL, g_CurrentTrackSegment->m_End, FALSE );

    ( (WinEDA_BasePcbFrame*) (panel->m_Parent) )->trace_ratsnest_pad( DC );
}


/*****************************************************************/
void Calcule_Coord_Extremite_45( int ox, int oy, int* fx, int* fy )
/*****************************************************************/

/* determine les parametres .fx et .fy du segment pointe par pt_segm
 *  pour avoir un segment oriente a 0, 90 ou 45 degres, selon position
 *  du oint d'origine et de la souris
 */
{
    int deltax, deltay, angle;

    deltax = ActiveScreen->m_Curseur.x - ox;
    deltay = ActiveScreen->m_Curseur.y - oy;
    /* calcul de l'angle preferentiel : 0, 45 , 90 degre */
    deltax = abs( deltax ); deltay = abs( deltay ); angle = 45;
    if( deltax >= deltay )
    {
        if( deltax == 0 )
            angle = 0;
        else if( ( (deltay << 6 ) / deltax ) < 26 )
            angle = 0;
    }
    else
    {
        angle = 45;
        if( deltay == 0 )
            angle = 90;
        else if( ( (deltax << 6 ) / deltay ) < 26 )
            angle = 90;
    }

    switch( angle )
    {
    case 0:
        *fx = ActiveScreen->m_Curseur.x;
        *fy = oy;
        break;

    case 45:
        deltax = MIN( deltax, deltay ); deltay = deltax;
        /* recalcul des signes de deltax et deltay */
        if( (ActiveScreen->m_Curseur.x - ox) < 0 )
            deltax = -deltax;
        if( (ActiveScreen->m_Curseur.y - oy) < 0 )
            deltay = -deltay;
        *fx = ox + deltax;
        *fy = oy + deltay;
        break;

    case 90:
        *fx = ox;
        *fy = ActiveScreen->m_Curseur.y;
        break;
    }
}


/********************************************************/
void ComputeBreakPoint( TRACK* track, int SegmentCount, wxPoint end )
/********************************************************/

/**
 * Compute new track angle based on previous track.
 */
{
    int iDx    = 0;
    int iDy    = 0;
    int iAngle = 0;

    if( SegmentCount <= 0 )
        return;
    if( track == NULL )
        return;

    TRACK* NewTrack = track;
    track = (TRACK*) track->Pback;
    SegmentCount--;
    if( track )
    {
        iDx = end.x - track->m_Start.x;
        iDy = end.y - track->m_Start.y;

        iDx = abs( iDx );
        iDy = abs( iDy );
    }

    TRACK* LastTrack = track ? (TRACK*) track->Pback : NULL;
    if( LastTrack )
    {
        if( (LastTrack->m_End.x == LastTrack->m_Start.x)
           || (LastTrack->m_End.y == LastTrack->m_Start.y) )
        {
            iAngle = 45;
        }
    }

    if( iAngle == 0 )
    {
        if( iDx >= iDy )
            iAngle = 0;
        else
            iAngle = 90;
    }

    if( track == NULL )
        iAngle = -1;

    switch( iAngle )
    {
    case - 1:
        break;

    case 0:
        if( (end.x - track->m_Start.x) < 0 )
            track->m_End.x = end.x + iDy;
        else
            track->m_End.x = end.x - iDy;
        track->m_End.y = track->m_Start.y;
        break;

    case 45:
        iDx = MIN( iDx, iDy );
        iDy = iDx;
        /* recalcul des signes de deltax et deltay */
        if( (end.x - track->m_Start.x) < 0 )
            iDx = -iDx;
        if( (end.y - track->m_Start.y) < 0 )
            iDy = -iDy;
        track->m_End.x = track->m_Start.x + iDx;
        track->m_End.y = track->m_Start.y + iDy;
        break;

    case 90:
        if( (end.y - track->m_Start.y) < 0 )
            track->m_End.y = end.y + iDx;
        else
            track->m_End.y = end.y - iDx;
        track->m_End.x = track->m_Start.x;
        break;
    }

    if( track )
    {
        if( track->IsNull() )
            track->m_End = end;
        NewTrack->m_Start = track->m_End;
    }
    NewTrack->m_End = end;
}


/****************************************************************************/
TRACK* DeleteNullTrackSegments( BOARD* pcb, TRACK* track, int* segmcount )
/****************************************************************************/

/* Delete track segments which have len = 0; after creating a new track
 *  return a pointer on the first segment (start of track list)
 */
{
    TRACK*      firsttrack = track;
    TRACK*      oldtrack;
    int         nn = 0;
    BOARD_ITEM* LockPoint;

    if( track == 0 )
        return NULL;
    LockPoint = track->start;
    while( track != NULL )
    {
        oldtrack = track;
        track    = track->Next();
        if( !oldtrack->IsNull() )
        {
            nn++;
            continue;
        }

        // NULL segment, delete it
        if( firsttrack == oldtrack )
            firsttrack = track;
        oldtrack->UnLink();
        delete oldtrack;
    }

    if( segmcount )
        *segmcount = nn;

    if( nn == 0 )
        return NULL; // all the new track segments have been deleted


    // we must set the pointers on connected items and the connection status
    oldtrack = track = firsttrack;
    firsttrack->start = NULL;
    while( track != NULL )
    {
        oldtrack      = track;
        track         = track->Next();
        oldtrack->end = track;
        if( track )
            track->start = oldtrack;
        oldtrack->SetStatus( 0 );
    }

    firsttrack->start = LockPoint;
    if( LockPoint && (LockPoint->Type() == TYPEPAD ) )
        firsttrack->SetState( BEGIN_ONPAD, ON );

    track = firsttrack;
    while( track != NULL )
    {
        TRACK* next_track = track->Next();
        LockPoint = Locate_Pad_Connecte( pcb, track, END );
        if( LockPoint )
        {
            track->end = LockPoint;
            track->SetState( END_ONPAD, ON );
            if( next_track )
            {
                next_track->start = LockPoint;
                next_track->SetState( BEGIN_ONPAD, ON );
            }
        }
        track = next_track;
    }

    return firsttrack;
}


/************************************/
void EnsureEndTrackOnPad( D_PAD* Pad )
/************************************/

/* Ensure the end point of g_CurrentTrackSegment is on the pas "Pad"
 *  if no, create a new track segment if necessary
 *  and move current (or new) end segment on pad
 */
{
    if( g_CurrentTrackSegment->m_End == Pad->m_Pos ) // Ok !
    {
        g_CurrentTrackSegment->end = Pad;
        g_CurrentTrackSegment->SetState( END_ONPAD, ON );
        return;
    }

    TRACK* lasttrack = g_CurrentTrackSegment;
    if( !g_CurrentTrackSegment->IsNull() )
    {
        /* Must create a new segment, from track end to pad center */
        g_CurrentTrackSegment = lasttrack->Copy();
        g_TrackSegmentCount++;
        lasttrack->Pnext = g_CurrentTrackSegment;
        g_CurrentTrackSegment->Pback = lasttrack;
        lasttrack->end = g_CurrentTrackSegment;
    }

    g_CurrentTrackSegment->m_End = Pad->m_Pos;
    g_CurrentTrackSegment->SetState( END_ONPAD, OFF );

    g_CurrentTrackSegment->end = Pad;
    g_CurrentTrackSegment->SetState( END_ONPAD, ON );
}
