/*********************************************/
/* Edition des pistes: Routines d'effacement */
/* Effacement de segment, piste, net et zone */
/*********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

/* Routines Locales */

/* Variables locales */


/***************************************************************/
TRACK* WinEDA_PcbFrame::Delete_Segment( wxDC* DC, TRACK* Track )
/***************************************************************/

/* Supprime 1 segment de piste.
 *  2 Cas possibles:
 *  Si On est en trace de nouvelle piste: Effacement du segment en
 *      cours de trace
 *  Sinon : Effacment du segment sous le curseur.
 */
{
    int current_net_code;

    if( Track == NULL )
        return NULL;

    if( Track->GetState( DELETED ) )
    {
        D( printf( "WinEDA_PcbFrame::Delete_Segment(): bug deleted already deleted TRACK\n" ); )
        return NULL;
    }

    if( Track->m_Flags & IS_NEW )  // Trace en cours, on peut effacer le dernier segment
    {
        if( g_TrackSegmentCount > 0 )
        {
            int previous_layer = ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer;

            // effacement de la piste en cours
            ShowNewTrackWhenMovingCursor( DrawPanel, DC, FALSE );

            // modification du trace
            Track = g_CurrentTrackSegment;
            g_CurrentTrackSegment = (TRACK*) g_CurrentTrackSegment->Pback;
            delete Track;
            g_TrackSegmentCount--;

            if( g_TwoSegmentTrackBuild )
            {
                // g_CurrentTrackSegment->Pback must not be a via, or we want delete also the via
                if( (g_TrackSegmentCount >= 2)
                   && (g_CurrentTrackSegment->Type() != TYPEVIA)
                   && (g_CurrentTrackSegment->Pback->Type() == TYPEVIA) )
                {
                    Track = g_CurrentTrackSegment;
                    g_CurrentTrackSegment = (TRACK*) g_CurrentTrackSegment->Pback;
                    delete Track;
                    g_TrackSegmentCount--;
                }
            }

            while( g_TrackSegmentCount && g_CurrentTrackSegment
                  && (g_CurrentTrackSegment->Type() == TYPEVIA) )
            {
                Track = g_CurrentTrackSegment;
                g_CurrentTrackSegment = (TRACK*) g_CurrentTrackSegment->Pback;
                delete Track;
                g_TrackSegmentCount--;
                if( g_CurrentTrackSegment && (g_CurrentTrackSegment->Type() != TYPEVIA) )
                    previous_layer = g_CurrentTrackSegment->GetLayer();
            }

            if( g_CurrentTrackSegment )
                g_CurrentTrackSegment->Pnext = NULL;

            // Rectification couche active qui a pu changer si une via
            // a ete effacee
            ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer = previous_layer;

            Affiche_Status_Box();
            if( g_TwoSegmentTrackBuild )   // We must have 2 segments or more, or 0
            {
                if( ( g_TrackSegmentCount == 1 )
                   && (g_CurrentTrackSegment->Type() != TYPEVIA) )
                {
                    delete g_CurrentTrackSegment;
                    g_TrackSegmentCount = 0;
                }
            }
            if( g_TrackSegmentCount == 0 )
            {
                DrawPanel->ManageCurseur = NULL;
                DrawPanel->ForceCloseManageCurseur = NULL;
                if( g_HightLigt_Status )
                    Hight_Light( DC );
                g_CurrentTrackSegment = NULL;
                g_FirstTrackSegment   = NULL;
                SetCurItem( NULL );
                return NULL;
            }
            else
            {
                if( DrawPanel->ManageCurseur )
                    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
                return g_CurrentTrackSegment;
            }
        }
        return NULL;
    } // Fin traitement si trace en cours


    current_net_code = Track->GetNet();

    // redraw the area where the track was
    DrawPanel->PostDirtyRect( Track->GetBoundingBox() );

    SaveItemEfface( Track, 1 );
    GetScreen()->SetModify();

    test_1_net_connexion( DC, current_net_code );
    m_Pcb->Display_Infos( this );
    return NULL;
}


/**********************************************************/
void WinEDA_PcbFrame::Delete_Track( wxDC* DC, TRACK* Track )
/**********************************************************/
{
    if( Track != NULL )
    {
        int current_net_code = Track->GetNet();
        Remove_One_Track( DC, Track );
        GetScreen()->SetModify();
        test_1_net_connexion( DC, current_net_code );
    }
}


/********************************************************/
void WinEDA_PcbFrame::Delete_net( wxDC* DC, TRACK* Track )
/********************************************************/
{
    if( Track == NULL )
        return;

    if( IsOK( this, _( "Delete NET ?" ) ) )
    {
        int    net_code_delete = Track->GetNet();

        /* Recherche du debut de la zone des pistes du net_code courant */
        TRACK* trackList = m_Pcb->m_Track->GetStartNetCode( net_code_delete );

        /* Decompte du nombre de segments de la sous-chaine */

        int    ii;
        TRACK* segm = trackList;
        for( ii = 0;  segm; segm = segm->Next(), ++ii )
        {
            if( segm->GetNet() != net_code_delete )
                break;

            // This works ok, but sometimes leaves stuff on screen.  I think
            // the erase rectangle is not large enough always.
            // DrawPanel->PostDirtyRect( segm->GetBoundingBox() );
        }

        // Do this instead of PostDirtyRect() for now
        DrawPanel->Refresh( TRUE );

        SaveItemEfface( trackList, ii );
        GetScreen()->SetModify();
        test_1_net_connexion( DC, net_code_delete );
        m_Pcb->Display_Infos( this );
    }
}


/********************************************************************/
void WinEDA_PcbFrame::Remove_One_Track( wxDC* DC, TRACK* pt_segm )
/********************************************************************/

/* Routine de suppression de 1 piste:
 *  le segment pointe est supprime puis les segments adjacents
 *  jusqu'a un pad ou un point de jonction de plus de 2 segments
 */
{
    TRACK*  trackList;
    int     nb_segm;

    if( pt_segm == NULL )
        return;

    trackList = Marque_Une_Piste( this, DC, pt_segm, &nb_segm, 0 );

    if( nb_segm ) /* Il y a nb_segm segments de piste a effacer */
    {
        TRACK*  t;
        int     ii;
        for( t = trackList, ii=0;  ii<nb_segm;  ii++, t = t->Next() )
        {
            t->SetState( BUSY, OFF );

            DrawPanel->PostDirtyRect( t->GetBoundingBox() );
        }

        SaveItemEfface( trackList, nb_segm );
    }
}
