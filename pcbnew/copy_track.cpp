		/*******************************************/
		/* 	Track editing: routines to copy tracks */
		/*******************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "drag.h"

#include "protos.h"


/* local functions */


/* variables locales */


#if 0

/***************************************************************/
void WinEDA_PcbFrame::Place_Dupl_Track(Track * Track, wxDC * DC)
/***************************************************************/
/*
	Routine de placement d'une piste (succession de segments)
*/
{
D_PAD * pt_pad;
TRACK * pt_track, *Track, * pt_classe, *NextS;
int masquelayer;
EDA_BaseStruct * LockPoint;
int ii, old_net_code, new_net_code, DRC_error = 0;
wxDC * DC = Cmd->DC;

	ActiveDrawPanel->ManageCurseur = NULL;

	if( NewTrack == NULL ) return ;

	old_net_code = NewTrack->net_code;

	/* Placement du flag BUSY de la piste originelle, qui ne doit
	pas etre vue dans les recherches de raccordement suivantes */
	ii = NbPtNewTrack; pt_track = NewTrack;
	for ( ; ii > 0; ii --, pt_track = (TRACK*) pt_track->Pnext)
		{
		pt_track->SetState(BUSY, ON);
		}

	/* Detection du nouveau net_code */
	ii = NbPtNewTrack; pt_track = NewTrack;
	for ( ; ii > 0; ii --, pt_track = (TRACK*) pt_track->Pnext)
		{
		pt_track->net_code = 0;
		}

	new_net_code = 0;
	ii = 0; pt_track = NewTrack;
	for( ; ii < NbPtNewTrack ; ii++, pt_track = (TRACK*)pt_track->Pnext)
		{
		/* Localisation de la pastille ou segment en debut de segment: */
		masquelayer = tab_layer[pt_track->Layer];
		LockPoint = LocateLockPoint(pt_track->m_Start.x,pt_track->m_Start.y,masquelayer);
		if( LockPoint )
			{
			if ( LockPoint->Type() == TYPEPAD )
				{
				pt_pad = (D_PAD*) LockPoint;
				new_net_code = pt_pad->net_code;
				if ( new_net_code > 0 ) break;
				}
			else		/* debut de piste sur un segment de piste */
				{
				Track = (TRACK *) LockPoint;
				new_net_code = Track->net_code;
				if ( new_net_code > 0 ) break;
				}
			}
		LockPoint = LocateLockPoint(pt_track->m_End.x,pt_track->m_End.y,masquelayer);
		if( LockPoint )
			{
			if ( LockPoint->Type() == TYPEPAD )
				{
				pt_pad = (D_PAD*) LockPoint;
				new_net_code = pt_pad->net_code;
				if ( new_net_code > 0 ) break;
				}
			else		/* debut de piste sur un segment de piste */
				{
				Track = (TRACK *) LockPoint;
				new_net_code = Track->net_code;
				if ( new_net_code > 0 ) break;
				}
			}
		}

	/* Mise a jour du nouveau net code de la piste */
	ii = 0; pt_track = NewTrack;
	for( ; ii < NbPtNewTrack; ii++, pt_track = (TRACK*)pt_track->Pnext)
		{
		pt_track->net_code = new_net_code;
		}

	/* Controle DRC de la nouvelle piste */
	ii = 0; pt_track = NewTrack;
	for( ; ii < NbPtNewTrack; ii++, pt_track = pt_track->Next() )
		{
		if( Drc_On == RUN )
		  if( drc(DC, pt_track, pt_pcb->Track, 1) == BAD_DRC )
			{
			if( confirmation(" Erreur DRC, Place piste:") == YES ) continue;
			else { DRC_error = 1; break; }
			}
		}

	if( DRC_error == 0)
		{
		if(FlagState == MOVE_ROUTE)
			{
			/* copie nouvelle piste */
			pt_track = NewTrack;
			NewTrack = pt_track->Copy(NbPtNewTrack);
			/* effacement ancienne ( chainage et liens mauvais */
			ii = NbPtNewTrack;
			for ( ; ii > 0; ii --, pt_track = NextS)
				{
				NextS = (TRACK*) pt_track->Pnext;
				pt_track->DeleteStructure();
				}
			test_1_net_connexion(DC, old_net_code );
			}

		pt_classe = NewTrack->GetBestInsertPoint();
		NewTrack->Insert(pt_classe);

		Trace_Une_Piste(DC, NewTrack,NbPtNewTrack,GR_OR) ;

		/* Mise a jour des connexions sur pads et sur pistes */
		ii = 0; pt_track = NewTrack;
		for( ; ii < NbPtNewTrack; ii++, pt_track = NextS)
			{
			NextS = (TRACK*)pt_track->Pnext;
			pt_track->SetState(BEGIN_ONPAD|END_ONPAD, OFF);
			masquelayer = tab_layer[pt_track->Layer];

			/* Localisation de la pastille ou segment sur debut segment: */
			LockPoint = LocateLockPoint(pt_track->m_Start.x,pt_track->m_Start.y,masquelayer);
			if( LockPoint )
				{
				pt_track->start = LockPoint;
				if ( LockPoint->Type() == TYPEPAD )
					{	/* fin de piste sur un pad */
					pt_pad = (D_PAD*) LockPoint;
					pt_track->SetState(BEGIN_ONPAD, ON);
					}
				else		/* debut de piste sur un segment de piste */
					{
					Track = (TRACK *) LockPoint;
					CreateLockPoint(&pt_track->m_Start.x,&pt_track->m_Start.y,Track,pt_track);
					}
				}

			/* Localisation de la pastille ou segment sur fin de segment: */
			LockPoint = LocateLockPoint(pt_track->m_End.x,pt_track->m_End.y,masquelayer);
			if( LockPoint )
				{
				pt_track->end = LockPoint;
				if ( LockPoint->Type() == TYPEPAD )
					{	/* fin de piste sur un pad */
					pt_pad = (D_PAD*) LockPoint;
					pt_track->SetState(END_ONPAD, ON);
					}
				else		/* debut de piste sur un segment de piste */
					{
					Track = (TRACK *) LockPoint;
					CreateLockPoint(&pt_track->m_Start.x,&pt_track->m_Start.y,Track,pt_track);
					}
				}
			}

		/* Clear the BUSY flag */
		ii = NbPtNewTrack; pt_track = NewTrack;
		for ( ; ii > 0; ii --, pt_track = (TRACK*) pt_track->Pnext)
			{
			pt_track->SetState(BUSY, OFF);
			}

		test_1_net_connexion(DC, new_net_code );
		ActiveScreen->SetModify();
		}

	else	/* DRC error: Annulation commande */
		{
		DisplayOpt.DisplayPcbTrackFill = FALSE ;
		Trace_Une_Piste(DC, NewTrack,NbPtNewTrack,GR_XOR);
		DisplayOpt.DisplayPcbTrackFill = Track_fill_copy ;

		if(FlagState == MOVE_ROUTE)
			{	/* Remise en position de la piste deplacee */
			Track = NewTrack;
			PosInitX -= Track->m_Start.x; PosInitY -= Track->m_Start.y;
			for( ii = 0; ii < NbPtNewTrack; ii++, Track = (TRACK*) Track->Pnext)
				{
				if( Track == NULL ) break;
				Track->m_Start.x += PosInitX; Track->m_Start.y += PosInitY;
				Track->m_End.x += PosInitX; Track->m_End.y += PosInitY;
				Track->SetState(BUSY,OFF);
				}
			Trace_Une_Piste(DC, NewTrack,NbPtNewTrack,GR_OR);
			}

		if (FlagState == COPY_ROUTE )
			{	/* Suppression copie */
			for( ii = 0; ii < NbPtNewTrack; NewTrack = NextS)
				{
				if(NewTrack == NULL) break;
				NextS = (TRACK*) NewTrack->Pnext;
				delete NewTrack;
				}
			}
		}
	NewTrack = NULL;
	Affiche_Infos_Status_Pcb(Cmd);
	if(Etat_Surbrillance) Hight_Light(DC);
}

/*******************************************************************************/
void WinEDA_PcbFrame::Start_CopyOrMove_Route(TRACK * track, wxDC * DC, bool Drag)
/*******************************************************************************/
/* Routine permettant la recopie d'une piste (suite de segments) deja tracee
*/
{
int ii;
TRACK *pt_segm, *pt_track;
int masquelayer = tab_layer[ActiveScreen->Active_Layer];

	if( NewTrack ) return;

	FlagState = (int)Cmd->Menu->param_inf;

	/* Recherche de la piste sur la couche active (non zone) */
	for(pt_segm = pt_pcb->Track; pt_segm != NULL; pt_segm = (TRACK*)pt_segm->Pnext)
		{
		pt_segm = Locate_Pistes(pt_segm,masquelayer, CURSEUR_OFF_GRILLE);
		if( pt_segm == NULL ) break ;
		break ;
		}

	if( pt_segm != NULL )
		{
		if (FlagState == COPY_ROUTE )
			pt_track = Marque_Une_Piste(DC, pt_segm, &NbPtNewTrack, 0);
		else pt_track = Marque_Une_Piste(DC, pt_segm, &NbPtNewTrack, GR_XOR);

		if(NbPtNewTrack) /* Il y a NbPtNewTrack segments de piste a traiter */
			{
			/* effacement du flag BUSY de la piste originelle */
			ii = NbPtNewTrack; pt_segm = pt_track;
			for ( ; ii > 0; ii --, pt_segm = (TRACK*) pt_segm->Pnext)
				{
				pt_segm->SetState(BUSY, OFF);
				}

			if (FlagState == COPY_ROUTE )
				NewTrack = pt_track->Copy(NbPtNewTrack);
			else NewTrack = pt_track;

			Affiche_Infos_Piste(Cmd, pt_track) ;

			startX = ActiveScreen->Curseur_X;
			startY = ActiveScreen->Curseur_Y;
			Place_Dupl_Route_Item.State = WAIT;
			ActiveDrawPanel->ManageCurseur = Show_Move_Piste;
			DisplayOpt.DisplayPcbTrackFill = FALSE ;
			Trace_Une_Piste(DC, NewTrack,NbPtNewTrack,GR_XOR) ;
			DisplayOpt.DisplayPcbTrackFill = Track_fill_copy ;
			PosInitX = NewTrack->m_Start.x; PosInitY = NewTrack->m_Start.y;
			}
		}
}


#endif

