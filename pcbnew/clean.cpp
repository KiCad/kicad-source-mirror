		/******************************************************/
		/*			editeur de PCB PCBNEW					  */
		/* Fonctions de Nettoyage et reorganisation de Pistes */
		/******************************************************/

	/* Fichier CLEAN.CPP */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "protos.h"

/* Constantes de controle de l'affichage des messages */
#define AFFICHE 1
#define POS_AFF_PASSE 40
#define POS_AFF_VAR 50
#define POS_AFF_MAX 60
#define POS_AFF_NUMSEGM 70

/* Routines locales : */
static int clean_segments(WinEDA_PcbFrame * frame, wxDC * DC);
static void suppression_piste_non_connectee(WinEDA_PcbFrame * frame, wxDC * DC);
static TRACK * AlignSegment(BOARD * Pcb, TRACK * pt_ref, TRACK * pt_segm, int extremite);
static void Clean_Pcb_Items(WinEDA_PcbFrame * frame, wxDC * DC );

/* Variables locales : */
static bool a_color;	/* couleur du message */
static bool s_CleanVias = true;
static bool s_MergeSegments = true;
static bool s_DeleteUnconnectedSegm = true;
static bool s_ConnectToPads = false;

#include "cleaningoptions_dialog.cpp"

#define CONN2PAD_ENBL

#ifdef CONN2PAD_ENBL
static void ConnectDanglingEndToPad(WinEDA_PcbFrame * frame, wxDC * DC);
static void Gen_Raccord_Track(WinEDA_PcbFrame * frame, wxDC * DC);
#endif

/*****************************************/
void WinEDA_PcbFrame::Clean_Pcb(wxDC * DC)
/*****************************************/
/* Regroupement des segments de meme piste.
	Suppression des points inutiles
		- via sur pad
		- points de couche et coord identiques
		- points alignes (supp du pt milieu)
*/
{
	s_ConnectToPads = false;
	WinEDA_CleaningOptionsFrame * frame = new WinEDA_CleaningOptionsFrame( this, DC );
	frame->ShowModal(); frame->Destroy();
	DrawPanel->Refresh(true);

}

/************************************************************/
void Clean_Pcb_Items(WinEDA_PcbFrame * frame, wxDC * DC )
/************************************************************/
{
	frame->MsgPanel->EraseMsgBox();
	frame->m_Pcb->GetNumSegmTrack();	/* Met a jour le compte */
	/* construction de la liste des coordonnées des pastilles */
	frame->m_Pcb->m_Status_Pcb = 0;
	frame->build_liste_pads();
	frame->recalcule_pad_net_code();

	if ( s_CleanVias ) /* delete redundant vias */
	{
		TRACK *track, *next_track;
		for( track = frame->m_Pcb->m_Track; track != NULL; track = track->Next())
		{
			if( track->m_Shape != VIA_NORMALE ) continue;
			/* Search and delete others vias at same location */
			TRACK * alt_track = track->Next();
			for( ; alt_track != NULL; alt_track = next_track )
			{
				next_track = alt_track->Next();
				if( alt_track->m_Shape != VIA_NORMALE ) continue;
				if ( alt_track->m_Start != track->m_Start ) continue;
				/* delete via */
				alt_track->UnLink();
				delete alt_track;
			}
		}
		/* Delete Via on pads at same location */
		for( track = frame->m_Pcb->m_Track; track != NULL; track = next_track)
		{
			next_track = track->Next();
			if( track->m_Shape != VIA_NORMALE ) continue;
			D_PAD * pad = Fast_Locate_Pad_Connecte(frame->m_Pcb, track->m_Start, ALL_CU_LAYERS);
			if (pad && (pad->m_Masque_Layer&EXTERNAL_LAYERS) == EXTERNAL_LAYERS)	// redundant Via 
			{
				/* delete via */
				track->UnLink();
				delete track;
			}
		}
	}

#ifdef CONN2PAD_ENBL
	if( s_ConnectToPads ) /* Creation de points de connexion */
	{
		/* Raccordement des extremites de piste au centre des pastilles : */
		ConnectDanglingEndToPad(frame, DC);

		/* Creation de points de raccordements aux intersections de pistes */
//		Gen_Raccord_Track(frame, DC);
	}
#endif

	/* suppression des segments de longueur nulle et des points intermediaires
	 alignes */
	if ( s_MergeSegments ) clean_segments(frame, DC);

	/* suppression des pistes non connectees ( c.a.d dont 1 extremite est en l'air) */
	if ( s_DeleteUnconnectedSegm ) suppression_piste_non_connectee(frame, DC);

	frame->Compile_Ratsnest(DC, AFFICHE);

	frame->m_CurrentScreen->SetModify();
}



/*****************************************************************************/
static void suppression_piste_non_connectee(WinEDA_PcbFrame * frame, wxDC * DC)
/*****************************************************************************/
/*
	Supprime les segments de piste ayant 1 ou 2 extremites non connectees
	Cas des vias:
	si une extremite de segment est connectee uniquement a une via, la via
	et le segment seront supprimes
*/
{
TRACK  * PtSegm, * pt_other, * pt_via;
TRACK * PtStartNetCode;
EDA_BaseStruct * NextS;
D_PAD * ptr_pad;
int nbpoints_supprimes = 0;
int masklayer, oldnetcode;
int type_end,flag_erase;
int ii, percent, oldpercent;
wxString msg;

	frame->Affiche_Message( _("Delete unconnected tracks:") );
	frame->DrawPanel->m_AbortRequest = FALSE;

	/* Correction des defauts des vias et recalcul du nombre de segm */
	frame->m_Pcb->m_NbSegmTrack = 0; ii = 0;
	for(PtSegm = frame->m_Pcb->m_Track; PtSegm != NULL; PtSegm = (TRACK*)NextS)
	{
		frame->m_Pcb->m_NbSegmTrack++;
		NextS = PtSegm->Pnext;
		if(PtSegm->m_StructType == TYPEVIA)
		{
			if( (PtSegm->m_Start.x != PtSegm->m_End.x ) || (PtSegm->m_Start.y != PtSegm->m_End.y ) )
			{
				PtSegm->m_End.x = PtSegm->m_Start.x;
				PtSegm->m_End.y = PtSegm->m_Start.y;
				ii++;
				msg.Printf( wxT("%d "), ii);
				Affiche_1_Parametre(frame, POS_AFF_PASSE,_("ViaDef"),msg,LIGHTRED);
			}
			continue;
		}
	}

	/* Suppression des pistes en l'air */
	PtSegm = frame->m_Pcb->m_Track; ii = 0;
	percent = 0; oldpercent = -1;
	oldnetcode = 0; PtStartNetCode = frame->m_Pcb->m_Track;
	for( ; PtSegm != NULL; PtSegm = (TRACK*) NextS, ii++)
		{
		NextS = PtSegm->Pnext;
		/* Affiche activite */
		percent = (100 * ii) / frame->m_Pcb->m_NbSegmTrack;
		if( percent != oldpercent)
			{
			oldpercent = percent;
			frame->DisplayActivity(percent, wxT("No Conn: ") );

			msg.Printf( wxT("%d "), frame->m_Pcb->m_NbSegmTrack );
			Affiche_1_Parametre(frame,POS_AFF_MAX, wxT("Max"), msg,GREEN);
	 		msg.Printf( wxT("%d "),ii );
			Affiche_1_Parametre(frame,POS_AFF_NUMSEGM, wxT("Segm"), msg,CYAN);
			}

		if( frame->DrawPanel->m_AbortRequest ) break;

		if( PtSegm->m_NetCode != oldnetcode)
			{
			PtStartNetCode = PtSegm; oldnetcode = PtSegm->m_NetCode;
			}

		flag_erase = 0;	type_end = 0;
		/* y a t-il une pastille sur chaque extremite */

		masklayer = PtSegm->ReturnMaskLayer();

		ptr_pad = Fast_Locate_Pad_Connecte(frame->m_Pcb, PtSegm->m_Start, masklayer);

		if( ptr_pad != NULL)
			{
			PtSegm->start = ptr_pad;
			type_end |= START_SUR_PAD;
			}

		ptr_pad = Fast_Locate_Pad_Connecte(frame->m_Pcb, PtSegm->m_End, masklayer);

		if( ptr_pad != NULL)
			{
			PtSegm->end = ptr_pad;
			type_end |= END_SUR_PAD;
			}

		/* Test si une extremite est connectee sur une piste */
		if ( (type_end & START_SUR_PAD ) == 0 )
			{
			pt_other = Locate_Piste_Connectee(PtSegm,frame->m_Pcb->m_Track,
							NULL, START);

			if(pt_other == NULL) flag_erase |= 1;

			else	/* Segment ou via connectee a cette extremite */
				{
				PtSegm->start = pt_other;
				if(pt_other->m_StructType == TYPEVIA) /* recherche d'un autre segment */
					{
					PtSegm->SetState(BUSY,ON);
					pt_via = pt_other;
					pt_other = Locate_Piste_Connectee(pt_via,frame->m_Pcb->m_Track,
													NULL,START);
					if(pt_other == NULL) flag_erase |= 2;
					PtSegm->SetState(BUSY,OFF);
					}
				}
			}

		 if ( (type_end & END_SUR_PAD ) == 0 )
			{
			pt_other = Locate_Piste_Connectee(PtSegm,frame->m_Pcb->m_Track,
											NULL, END);
			if(pt_other == NULL) flag_erase |= 0x10;
			else	 /* Segment ou via connectee a cette extremite */
				{
				PtSegm->end = pt_other;
				if(pt_other->m_StructType == TYPEVIA) /* recherche d'un autre segment */
					{
					PtSegm->SetState(BUSY,ON);
					pt_via = pt_other;

					pt_other = Locate_Piste_Connectee(pt_via,frame->m_Pcb->m_Track,
												NULL, END);
					if(pt_other == NULL) flag_erase |= 0x20;
					PtSegm->SetState(BUSY,OFF);
					}
				}
			}

		if(flag_erase)
			{
			oldpercent = -1;	/* force affichage activite */
			nbpoints_supprimes++; ii--;
			msg.Printf( wxT("%d "),nbpoints_supprimes);
			Affiche_1_Parametre(frame,POS_AFF_VAR, wxT("NoConn."), msg,LIGHTRED);

			/* rectification du pointeur PtSegm pour repartir en debut
					du block des segments de meme net_code */
			if( PtSegm == PtStartNetCode )
				{
				NextS = PtSegm->Pnext;
				PtStartNetCode = (TRACK*)NextS;
				}
			else NextS = PtStartNetCode;

			/* Suppression du segment */
			PtSegm->Draw(frame->DrawPanel, DC, GR_XOR);
			DeleteStructure(PtSegm);
			if ( NextS == NULL ) break;
			}
		}
}



/************************************************************/
static  int clean_segments(WinEDA_PcbFrame * frame, wxDC * DC)
/************************************************************/
/* Supprime segments nulls, points inutiles .. */
{
TRACK * PtSegm, * pt_aux;
EDA_BaseStruct * NextS;
int ii, nbpoints_supprimes = 0;
int flag, no_inc, percent, oldpercent;
wxString msg;

	frame->DrawPanel->m_AbortRequest = FALSE;

	/**********************************************/
	/* suppression des segments de longueur nulle */
	/**********************************************/

	a_color = GREEN;
	nbpoints_supprimes = 0; PtSegm = frame->m_Pcb->m_Track;
	percent = 0; oldpercent = -1;
	frame->MsgPanel->EraseMsgBox();
	frame->Affiche_Message( _("Clean Null Segments") );

	Affiche_1_Parametre(frame,POS_AFF_VAR, wxT("NullSeg"), wxT("0"),a_color);
	for(PtSegm = frame->m_Pcb->m_Track; PtSegm != NULL; PtSegm = (TRACK*)NextS )
		{
		NextS = PtSegm->Pnext;
		if( ! PtSegm->IsNull() ) continue;

		/* Lenght segment = 0; delete it */
		PtSegm->Draw(frame->DrawPanel, DC, GR_XOR);
		DeleteStructure(PtSegm);
		nbpoints_supprimes++;

		msg.Printf( wxT("  %d"),nbpoints_supprimes);
		Affiche_1_Parametre(frame,POS_AFF_VAR,wxEmptyString, msg,a_color);
		}

	/**************************************/
	/* suppression des segments confondus */
	/**************************************/

	Affiche_1_Parametre(frame,POS_AFF_VAR, wxT("Ident"), wxT("0"),a_color);
	percent = 0; oldpercent = -1;
	PtSegm = frame->m_Pcb->m_Track;
	for( ii = 0; PtSegm != NULL; PtSegm = (TRACK*) PtSegm->Pnext , ii++)
		{
		/* affichage activite */
		percent = (100 * ii) / frame->m_Pcb->m_NbSegmTrack;
		if( percent != oldpercent)
			{
			frame->DisplayActivity(percent, wxT("Id segm: ") );
			oldpercent = percent;

			msg.Printf( wxT("%d"), frame->m_Pcb->m_NbSegmTrack);
			Affiche_1_Parametre(frame,POS_AFF_MAX, wxT("Max"), msg,GREEN);
			msg.Printf( wxT("%d"), ii );
			Affiche_1_Parametre(frame,POS_AFF_NUMSEGM, wxT("Segm"), msg,CYAN);

			if( frame->DrawPanel->m_AbortRequest ) return(-1);
			}

		for( pt_aux = (TRACK*)PtSegm->Pnext; pt_aux != NULL; pt_aux = (TRACK*)NextS )
			{
			int erase = 0;
			NextS = pt_aux->Pnext;

			if( PtSegm->m_StructType != pt_aux->m_StructType ) continue;
			if( PtSegm->m_Layer != pt_aux->m_Layer ) continue;
			if( PtSegm->m_NetCode != pt_aux->m_NetCode ) break;

			if( (PtSegm->m_Start.x == pt_aux->m_Start.x ) && (PtSegm->m_Start.y == pt_aux->m_Start.y) )
				{
				if( (PtSegm->m_End.x == pt_aux->m_End.x ) && (PtSegm->m_End.y==pt_aux->m_End.y) )
					erase = 1;
				}

			if( (PtSegm->m_Start.x==pt_aux->m_End.x ) && (PtSegm->m_Start.y == pt_aux->m_End.y) )
				{
				if( (PtSegm->m_End.x==pt_aux->m_Start.x ) && (PtSegm->m_End.y==pt_aux->m_Start.y) )
					erase = 1;
				}

			/* suppression du point en trop */
			if( erase )
				{
				ii--;
				pt_aux->Draw(frame->DrawPanel, DC, GR_OR);
				DeleteStructure(pt_aux);
				nbpoints_supprimes++;

				msg.Printf( wxT("  %d"),nbpoints_supprimes);
				Affiche_1_Parametre(frame,50, wxEmptyString, msg,a_color);
				}
			}
		}

	/**************************************************************/
	/* suppression des points intermediaires ( segments alignes ) */
	/**************************************************************/

	PtSegm = frame->m_Pcb->m_Track; nbpoints_supprimes = 0;
	percent = 0; oldpercent = -1;
	frame->Affiche_Message( _("Merging Segments:") );

	Affiche_1_Parametre(frame,POS_AFF_VAR, _("Merge"), _("0"),a_color);

	ii = 0;
	for( PtSegm = frame->m_Pcb->m_Track; PtSegm!= NULL; PtSegm = (TRACK*) NextS)
		{
		TRACK * pt_segm_s, * pt_segm_e, *pt_segm_delete;

		NextS = PtSegm->Pnext;
		/* affichage activite */
		ii++;
		percent = (100 * ii)/ frame->m_Pcb->m_NbSegmTrack;
		if( percent != oldpercent)
			{
			frame->DisplayActivity(percent, _("Merge: ") );
			oldpercent = percent;
			msg.Printf( wxT("%d"), frame->m_Pcb->m_NbSegmTrack);
			Affiche_1_Parametre(frame, POS_AFF_MAX, wxT("Max"), msg,GREEN);
			msg.Printf( wxT("%d"), ii );
			Affiche_1_Parametre(frame, POS_AFF_NUMSEGM, wxT("Segm"), msg,CYAN);

			if( frame->DrawPanel->m_AbortRequest ) return(-1);
			}

		if(PtSegm->m_StructType != TYPETRACK) continue;

		flag = no_inc = 0;

		/* Recherche d'un point possible raccorde sur DEBUT de segment: */
		for( pt_segm_s = (TRACK*)PtSegm->Pnext; ; )
			{
			pt_segm_s = Locate_Piste_Connectee(PtSegm, pt_segm_s,
								NULL, START);
			if(pt_segm_s)
				{
				/* les 2 segments doivent avoir meme largeur */
				if(PtSegm->m_Width != pt_segm_s->m_Width ) break;
				/* Ce ne peut etre une via */
				if(pt_segm_s->m_StructType != TYPETRACK) break;

				/* On ne peut avoir que 1 seul segment connecte */
				pt_segm_s->SetState(BUSY,ON);
				pt_aux = Locate_Piste_Connectee(PtSegm, frame->m_Pcb->m_Track,
										NULL,START);
				pt_segm_s->SetState(BUSY,OFF);

				if(pt_aux == NULL) flag = 1;	/* OK */
				break;
				}
			break;
			}

		if(flag)	/* debut de segment raccorde a un autre segment */
			{
			pt_segm_delete = AlignSegment(frame->m_Pcb, PtSegm,pt_segm_s,START);
			if ( pt_segm_delete )
				{
				nbpoints_supprimes++; no_inc = 1;
				DeleteStructure(pt_segm_delete);
				}
			}

		/* Recherche d'un point possible raccorde sur FIN de segment: */
		for( pt_segm_e = (TRACK*)PtSegm->Pnext; ; )
			{
			pt_segm_e = Locate_Piste_Connectee(PtSegm, pt_segm_e,
												NULL, END);
			if(pt_segm_e)
				{
				/* les 2 segments doivent avoir meme largeur */
				if(PtSegm->m_Width != pt_segm_e->m_Width ) break;
				/* Ce ne peut etre une via */
				if(pt_segm_e->m_StructType != TYPETRACK) break;

				/* On ne peut avoir que 1 seul segment connecte */
				pt_segm_e->SetState(BUSY,ON);
				pt_aux = Locate_Piste_Connectee(PtSegm, frame->m_Pcb->m_Track,
												NULL,END) ;
				pt_segm_e->SetState(BUSY,OFF);
				if (pt_aux == NULL) flag |= 2;/* Ok */
				break;
				}
			else break;
			}

		if(flag & 2)	/* FIN de segment raccorde a un autre segment */
			{
			pt_segm_delete = AlignSegment(frame->m_Pcb, PtSegm,pt_segm_e,END);
			if ( pt_segm_delete )
				{
				nbpoints_supprimes++; no_inc = 1;
				DeleteStructure(pt_segm_delete);
				}
			}

		if(no_inc) /* Le segment en cours a ete modifie, il faut le reexaminer */
			{
			msg.Printf( wxT("%d "),nbpoints_supprimes);
			Affiche_1_Parametre(frame,POS_AFF_VAR,wxEmptyString, msg,a_color);
			NextS = PtSegm->Pnext;
			}
		}
	return (0) ;
}

/****************************************************************************/
static TRACK * AlignSegment(BOARD * Pcb, TRACK * pt_ref, TRACK * pt_segm, int extremite)
/****************************************************************************/
/* Routine utilisee par clean_segments.
 Verifie l'alignement de pt_segm / pt_ref. et verifie que le point commun
 a faire disparaitre n'est pas sur un pad.
 l'extremite testee est debut (extremite == START) ou fin (extremite == FIN)
 si il y a alignement, modifie les coord d'extremite de pt_ref et retourne
 pt_segm.
 sinon retourne NULL
*/
{
int refdx,refdy, segmdx, segmdy;	/* projections des segments */
int flag = 0;

	refdx = pt_ref->m_End.x - pt_ref->m_Start.x; refdy = pt_ref->m_End.y - pt_ref->m_Start.y;
	segmdx = pt_segm->m_End.x - pt_segm->m_Start.x; segmdy = pt_segm->m_End.y - pt_segm->m_Start.y;

	/* Tst alignement vertical possible: */
	if(refdx == 0)
		{
		if (segmdx != 0) return(NULL);
		else flag = 1;
		}
	/* Tst alignement horizontal possible: */
	if(refdy == 0)
		{
		if (segmdy != 0) return(NULL);
		else flag = 2;
		}

	/* tst si il y a alignement d'angle qcq
	il faut que refdy/refdx == (+/-)segmdy/segmdx, c.a.d meme direction */
	if (flag == 0)
		{
		if( (refdy * segmdx != refdx * segmdy) &&
			(refdy * segmdx != -refdx * segmdy) ) return(NULL);
		flag = 4;
		}

	/* Ici il y a alignement: il faut determiner les positions relatives
	pour supprimer le point commun et le remplacer */
	if (extremite == START)
		{
		/* Ce ne doit pas etre sur un pad */
		if(Fast_Locate_Pad_Connecte(Pcb, pt_ref->m_Start,
									g_TabOneLayerMask[pt_ref->m_Layer]))
			return(NULL);

		if( (pt_ref->m_Start.x == pt_segm->m_Start.x) && (pt_ref->m_Start.y == pt_segm->m_Start.y) )
			{
			pt_ref->m_Start.x = pt_segm->m_End.x; pt_ref->m_Start.y = pt_segm->m_End.y;
			return(pt_segm);
			}
		else		/* connexion par la fin de pt_segm */
			{
			pt_ref->m_Start.x = pt_segm->m_Start.x; pt_ref->m_Start.y = pt_segm->m_Start.y;
			return(pt_segm);
			}
		}
	else	/* extremite == END */
		{
		/* Ce ne doit pas etre sur un pad */
		if(Fast_Locate_Pad_Connecte(Pcb, pt_ref->m_End, g_TabOneLayerMask[pt_ref->m_Layer]))
			return(NULL);

		if( pt_ref->m_End == pt_segm->m_Start )
			{
			pt_ref->m_End = pt_segm->m_End;
			return(pt_segm);
			}
		else		/* connexion par la fin de pt_segm */
			{
			pt_ref->m_End = pt_segm->m_Start;
			return(pt_segm);
			}
		}
	return(NULL);
}


/***************************************************************************/
int Netliste_Controle_piste(WinEDA_PcbFrame * frame, wxDC * DC, int affiche)
/***************************************************************************/
/*
	verification des connexions.
	Supprime les segments mal connectes, c.a.d. interconnectant des segments
	de net_code differents
*/
{
TRACK  * PtSegm, * pt_aux;
EDA_BaseStruct * NextS;
int net_code_s, net_code_e;
int nbpoints_modifies = 0;
int flag = 0;
int ii, percent, oldpercent;
wxString msg;

	a_color = RED;
	percent = 0; oldpercent = -1;

	frame->Affiche_Message( _("DRC Control:") );

	frame->DrawPanel->m_AbortRequest = FALSE;

	if(affiche) Affiche_1_Parametre(frame,POS_AFF_VAR, _("NetCtr"), wxT("0 "),a_color);

	ii = 0;
	for(PtSegm = frame->m_Pcb->m_Track; PtSegm != NULL; PtSegm = (TRACK*)PtSegm->Pnext)
		{
		/* affichage activite */
		ii++;
		percent = (100 * ii) / frame->m_Pcb->m_NbSegmTrack;
		if( percent != oldpercent)
			{
			frame->DisplayActivity(percent, wxT("Drc: ") );
			oldpercent = percent;
			msg.Printf( wxT("%d"), frame->m_Pcb->m_NbSegmTrack);
			Affiche_1_Parametre(frame, POS_AFF_MAX, wxT("Max"),msg,GREEN);
			msg.Printf( wxT("%d"), frame->m_Pcb->m_NbSegmTrack);
			Affiche_1_Parametre(frame, POS_AFF_NUMSEGM, wxT("Segm"),msg,CYAN);

			if( frame->DrawPanel->m_AbortRequest ) return(flag);
			}

		PtSegm->SetState(FLAG0,OFF);

		/* Calcul du net_code des elements raccordes sur le point de debut */
		net_code_s = -1;
		if( (PtSegm->start != NULL) &&
				( ((EDA_BaseStruct*)(PtSegm->start))->m_StructType == TYPEPAD) )
			net_code_s = ((D_PAD*)(PtSegm->start))->m_NetCode;

		else
			{
			pt_aux = Locate_Piste_Connectee(PtSegm,frame->m_Pcb->m_Track,
										NULL, START);
			if(pt_aux) net_code_s = pt_aux->m_NetCode;
			}
		if(net_code_s < 0 ) continue ; /* Extremite en l'air */

		/* Calcul du net_code des elements raccordes sur le point de fin */
		net_code_e = -1;
		if( (PtSegm->end != NULL) &&
				( ((EDA_BaseStruct*)(PtSegm->end))->m_StructType == TYPEPAD) )
			net_code_e = ((D_PAD*)(PtSegm->end))->m_NetCode;

		else
			{
			pt_aux = Locate_Piste_Connectee(PtSegm,frame->m_Pcb->m_Track,
										NULL,END);
			if(pt_aux) net_code_e = pt_aux->m_NetCode;
			}

		if(net_code_e < 0 ) continue ; /* Extremite en l'air */

 		/* Marquage des segments a supprimer */
		if(net_code_s != net_code_e)
			{
			PtSegm->SetState(FLAG0,ON);
			}
		}

	/* Suppression effective des segments */

	for(PtSegm = frame->m_Pcb->m_Track; PtSegm != NULL; PtSegm = (TRACK*) NextS)
	{
		NextS = PtSegm->Pnext;
		if( PtSegm->GetState(FLAG0) ) /* Connexion erronee : a supprimer */
			{
			PtSegm->SetState(FLAG0,OFF);
			flag = 1; oldpercent = -1;
			frame->m_Pcb->m_Status_Pcb = 0;
			frame->Supprime_Une_Piste( DC, PtSegm);
			NextS = frame->m_Pcb->m_Track;	/* NextS a peut etre ete efface */
			if(affiche)
			{
				nbpoints_modifies++;
				msg.Printf( wxT("%d "),nbpoints_modifies);
				Affiche_1_Parametre(frame,POS_AFF_VAR,wxEmptyString,msg,a_color);
			}
		}
	}
	return(flag);
}


#ifdef CONN2PAD_ENBL

/***************************************************************/
static void Gen_Raccord_Track(WinEDA_PcbFrame * frame, wxDC * DC)
/***************************************************************/

/* Teste les extremites de segments :
	si une extremite est sur un segment de piste, mais pas sur une extremite,
	le segment est coupe en 2, le point de coupure etant l'extremite du segment
	Ceci est fait pour que les tests de connexion qui ne testent que les extremites
	de segments voient la connexion
*/
{
TRACK * PtSegm, *pt_aux, * NewTrack;
EDA_BaseStruct * NextS;
int nn = 0;
int masquelayer;
int ii, percent, oldpercent;
wxString msg;

	frame->Affiche_Message( wxT("Gen Raccords sur Pistes:") );
	if( frame->m_Pcb->GetNumSegmTrack() == 0 ) return;

	frame->DrawPanel->m_AbortRequest = FALSE;

	oldpercent = -1; ii = 0;
	for( PtSegm = frame->m_Pcb->m_Track; PtSegm != NULL; PtSegm = (TRACK*) NextS)
	{
		NextS = PtSegm->Pnext;
		/* affichage activite */
		ii++;
		percent = (100 * ii)/ frame->m_Pcb->m_NbSegmTrack;
		if( percent != oldpercent)
		{
			frame->DisplayActivity(percent, wxT("Tracks: ") );
			oldpercent = percent;
			msg.Printf( wxT("%d"),frame->m_Pcb->m_NbSegmTrack );
			Affiche_1_Parametre(frame,POS_AFF_MAX, wxT("Max"),msg,GREEN);
			msg.Printf( wxT("%d"), ii);
			Affiche_1_Parametre(frame,POS_AFF_NUMSEGM, wxT("Segm"),msg,CYAN);
		}

		if( frame->DrawPanel->m_AbortRequest ) return;

		masquelayer = PtSegm->ReturnMaskLayer();

		/* examen du point de depart du segment de reference */
		for(pt_aux = frame->m_Pcb->m_Track; ; pt_aux = (TRACK*) pt_aux->Pnext)
		{
			if (pt_aux == NULL) break;
			pt_aux = Locate_Pistes(pt_aux, PtSegm->m_Start, masquelayer);
			if (pt_aux == NULL) break;
			if(pt_aux == PtSegm) continue;
			if(pt_aux->m_StructType == TYPEVIA) continue;
			if( PtSegm->m_Start == pt_aux->m_Start ) continue;
			if( PtSegm->m_Start == pt_aux->m_End ) continue;

			// Test si autre extremite du segment ne serait pas deja connectee
			if( PtSegm->m_End == pt_aux->m_Start ) continue;
			if( PtSegm->m_End == pt_aux->m_End ) continue;

			pt_aux->Draw(frame->DrawPanel, DC, GR_XOR);
			NewTrack = pt_aux->Copy();
			NewTrack->Insert(frame->m_Pcb, pt_aux);
			nn++;
			frame->m_Pcb->m_NbSegmTrack++;
			msg.Printf( wxT("%d"),nn);
			Affiche_1_Parametre(frame,POS_AFF_VAR, wxT("New <"),msg,YELLOW);
			pt_aux->m_End = PtSegm->m_Start;
			NewTrack->m_Start = PtSegm->m_Start;
			Trace_Une_Piste(frame->DrawPanel, DC, pt_aux, 2, GR_OR);
			pt_aux = NewTrack;
		}

		/* examen du point de fin du segment de reference */
		for(pt_aux = frame->m_Pcb->m_Track; pt_aux != NULL; pt_aux = (TRACK*) pt_aux->Pnext)
		{
			pt_aux = Locate_Pistes(pt_aux, PtSegm->m_End, masquelayer);
			if(pt_aux == NULL) break;
			if(pt_aux == PtSegm) continue;
			if(pt_aux->m_StructType == TYPEVIA) continue;

			if( (PtSegm->m_End.x == pt_aux->m_Start.x) && (PtSegm->m_End.y == pt_aux->m_Start.y)) continue;
			if( (PtSegm->m_End.x == pt_aux->m_End.x) && (PtSegm->m_End.y == pt_aux->m_End.y)) continue;

			if( (PtSegm->m_Start.x == pt_aux->m_Start.x) && (PtSegm->m_Start.y == pt_aux->m_Start.y)) continue;
			if( (PtSegm->m_Start.x == pt_aux->m_End.x) && (PtSegm->m_Start.y == pt_aux->m_End.y)) continue;

			pt_aux->Draw(frame->DrawPanel, DC, GR_XOR);

			NewTrack = pt_aux->Copy();
			NewTrack->Insert(frame->m_Pcb, pt_aux);

			frame->m_Pcb->m_NbSegmTrack++;
			nn++; msg.Printf( wxT("%d"),nn);
			Affiche_1_Parametre(frame,POS_AFF_VAR, wxT("New >"),msg,YELLOW);
			pt_aux->m_End.x = PtSegm->m_End.x; pt_aux->m_End.y = PtSegm->m_End.y;
			NewTrack->m_Start.x = PtSegm->m_End.x; NewTrack->m_Start.y = PtSegm->m_End.y;
			Trace_Une_Piste(frame->DrawPanel, DC, pt_aux,2,GR_OR);
			pt_aux = NewTrack;
		}
	}
}

/***************************************************************/
void ConnectDanglingEndToPad(WinEDA_PcbFrame * frame, wxDC * DC)
/**************************************************************/
/*
 rajoute eventuellement un segment a une extremite de piste connectee
 a un pad si cette extremite n'est pas exactement connectee au centre
 du pad.
	Ceci permet un controle plus rapide des connexions
*/
{
TRACK  * PtSegm;
EDA_BaseStruct * NextS;
D_PAD * ptr_pad;
int nb_new_piste = 0 , ii;
int percent, oldpercent;
wxString msg;

	a_color = GREEN;
	percent = 0; oldpercent = -1;

	frame->DrawPanel->m_AbortRequest = FALSE;

	Affiche_1_Parametre(frame, POS_AFF_VAR, _("Centre"), _("0 "),a_color);

	ii = 0;
	for(PtSegm = frame->m_Pcb->m_Track; PtSegm != NULL; PtSegm = (TRACK*)NextS)
		{
		NextS = PtSegm->Pnext;
		/* affichage activite */
		ii++;
		percent = (100 * ii)/ frame->m_Pcb->m_NbSegmTrack;
		if( percent != oldpercent)
			{
			frame->DisplayActivity(percent, _("Pads: "));
			oldpercent = percent;
			msg.Printf( wxT("%d"), frame->m_Pcb->m_NbSegmTrack );
			Affiche_1_Parametre(frame, POS_AFF_MAX, _("Max"),msg,GREEN);
			msg.Printf( wxT("%d"), ii);
			Affiche_1_Parametre(frame, POS_AFF_NUMSEGM, _("Segm"),msg,CYAN);

			if( frame->DrawPanel->m_AbortRequest ) return;
			}

		ptr_pad = Locate_Pad_Connecte(frame->m_Pcb, PtSegm,START);
		if( ptr_pad != NULL )
			{	/* Test si point piste non deja connecte a un autre segment */
			if( PtSegm->m_Start != ptr_pad->m_Pos )
				{
				if(Locate_Piste_Connectee(PtSegm,frame->m_Pcb->m_Track,
											NULL, START) == NULL)
					{
					TRACK * NewTrack = PtSegm->Copy();
					NewTrack->Insert(frame->m_Pcb, PtSegm);
					NewTrack->m_End = ptr_pad->m_Pos;
					NewTrack->start = PtSegm;
					NewTrack->end = ptr_pad;
					nb_new_piste++;
					NewTrack->Draw(frame->DrawPanel, DC, GR_OR);

					Affiche_1_Parametre(frame,POS_AFF_VAR,wxEmptyString,msg,a_color);
					}
				}
			}

		ptr_pad = Locate_Pad_Connecte(frame->m_Pcb, PtSegm,END);
		if( ptr_pad != NULL )
			{	/* Test si point piste "en l'air" */
			if( (PtSegm->m_End.x != ptr_pad->m_Pos.x) ||
				(PtSegm->m_End.y != ptr_pad->m_Pos.y) )
				{
				if(Locate_Piste_Connectee(PtSegm,frame->m_Pcb->m_Track,
											NULL,END) == NULL)
					{
					TRACK * NewTrack = PtSegm->Copy();
					NewTrack->Insert(frame->m_Pcb, PtSegm);
					NewTrack->m_Start.x = ptr_pad->m_Pos.x;
					NewTrack->m_Start.y = ptr_pad->m_Pos.y;
					NewTrack->start = ptr_pad;
					NewTrack->end = PtSegm;
					nb_new_piste++;

					msg.Printf( wxT("e %d"),nb_new_piste);
					Affiche_1_Parametre(frame, POS_AFF_VAR,wxEmptyString,msg,a_color);
					}
				}
			}
		}
}

#endif
