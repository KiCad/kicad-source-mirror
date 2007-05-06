		/************************************/
		/****		editeur de PCB		 ****/
		/* traitement du Chevelu (Rastnest) */
		/************************************/

	/* Fichier RATSNEST.CPP */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "protos.h"

/* variables locales */
CHEVELU * g_pt_chevelu ;
CHEVELU * local_liste_chevelu;	// adresse de base du buffer des chevelus locaux
int nb_local_chevelu;			// nbr de links du module en deplacement
int nb_pads_ref;				// nbr de nodes du module en deplacement
int nb_pads_externes;			// nbr de pads connectes au module en deplacement
bool DisplayRastnestInProgress;	// autorise affichage chevelu en cours de calcul
								// de celui-ci



/******************************************************************************/
void WinEDA_BasePcbFrame::Compile_Ratsnest( wxDC * DC, bool display_status_pcb )
/******************************************************************************/
/*
	Génère le chevelu complet de la carte.
	Doit etre appelé APRES le calcul de connectivité
	Doit etre appelé apres changement de structure de la carte (modif
 de pads, de nets, de modules).

	Si display_status_pcb : affichage des résultats en bas d'ecran
*/
{
wxString msg;
	
	DisplayRastnestInProgress = TRUE;

	/* construction de la liste des coordonnées des pastilles */
	m_Pcb->m_Status_Pcb = 0;		/* réinit total du calcul */
	build_liste_pads() ;
	MsgPanel->EraseMsgBox() ;  /* effacement du bas d'ecran */
	msg.Printf( wxT(" %d"),m_Pcb->m_NbPads) ;
	Affiche_1_Parametre(this, 1, wxT("pads"), msg,RED) ;

	msg.Printf( wxT(" %d"),m_Pcb->m_NbNets) ;
	Affiche_1_Parametre(this, 8, wxT("Nets"), msg,CYAN) ;

	reattribution_reference_piste(display_status_pcb);
	Build_Board_Ratsnest(DC );	/* calcul du chevelu general */
	test_connexions(DC);		/* determine les blocks de pads connectés par
										les pistes existantes */

	Tst_Ratsnest(DC, 0 );	/* calcul du chevelu actif */

	// Reaffichage des chevelus actifs
	if( g_Show_Ratsnest ) DrawGeneralRatsnest( DC, 0 );

	if ( display_status_pcb ) Affiche_Infos_Status_Pcb(this);
}


/*****************************************************************/
static int tri_par_net(LISTE_PAD * pt_ref, LISTE_PAD * pt_compare)
/****************************************************************/
/* routine utilisee par la foncion QSORT */
{
	return( (*pt_ref)->m_NetCode - (*pt_compare)->m_NetCode );
}

/********************************************************/
static int sort_by_length(CHEVELU * ref, CHEVELU * compare)
/********************************************************/
/* routine de tri par longueur des chevelus utilisee par la foncion QSORT */
{
	return( ref->dist - compare->dist );
}


/*****************************************************************************/
static int gen_rats_block_to_block(WinEDA_DrawPanel * DrawPanel, wxDC * DC,
		LISTE_PAD * pt_liste_pad, LISTE_PAD * pt_limite, int * nblinks)
/*****************************************************************************/
/*
 Routine utilisee par Build_Board_Ratsnest()
 Routine generant le chevelu entre 2 blocks ( supposes du meme net )
	la recherche est faite entre les pads du block 1 et les autres blocks
	le block n ( n > 1 ) est alors connecte au block 1 par leur 2 pads
	les plus proches.
		Entree :
			pt_chain_pad = adresse de debut de recherche
			pt_limite	  = adresse de fin de recherche (borne non comprise)
		Sortie:
			liste des chevelus ( structures)
			mise a jour de g_pt_chevelu a la 1ere case libre
		Retourne:
			nombre de blocks non connectes entre eux
 */
{
int dist_min, current_dist ;
int current_num_block = 1 ;
LISTE_PAD * pt_liste_pad_tmp,
		  * pt_liste_pad_aux,
		  * pt_liste_pad_block1 = NULL,
		  * pt_start_liste ;

	pt_liste_pad_tmp = NULL ; dist_min = 0x7FFFFFFF;
	pt_start_liste = pt_liste_pad ;

	if ( DC ) GRSetDrawMode(DC, GR_XOR);

	/* Recherche du pad le plus proche du block 1 */
	for ( ; pt_liste_pad < pt_limite ; pt_liste_pad++ )
	{
		D_PAD * ref_pad = *pt_liste_pad;
		if(ref_pad->m_logical_connexion != 1) continue ;

		for ( pt_liste_pad_aux = pt_start_liste ; ;pt_liste_pad_aux++)
		{
			D_PAD * curr_pad = *pt_liste_pad_aux;
			if(pt_liste_pad_aux >= pt_limite ) break ;
			if(curr_pad->m_logical_connexion == 1) continue ;

			/* Comparaison des distances des pastilles (calcul simplifie) */
			current_dist =
					 abs( curr_pad->m_Pos.x - ref_pad->m_Pos.x)
				   + abs( curr_pad->m_Pos.y - ref_pad->m_Pos.y) ;

			if(dist_min > current_dist)
			{
				current_num_block = curr_pad->m_logical_connexion ;
				dist_min = current_dist;
				pt_liste_pad_tmp = pt_liste_pad_aux;
				pt_liste_pad_block1 = pt_liste_pad;
			}
		}
	}

	if (current_num_block > 1) /* le block n a ete connecte au bloc 1 */
	{
		/* le block n est fondu avec le bloc 1 : */
		for( pt_liste_pad = pt_start_liste; pt_liste_pad < pt_limite; pt_liste_pad++ )
		{
			if( (*pt_liste_pad)->m_logical_connexion == current_num_block)
						(*pt_liste_pad)->m_logical_connexion = 1;
		}

		pt_liste_pad = pt_liste_pad_block1 ;

		(*nblinks)++;
		g_pt_chevelu->m_NetCode = (*pt_liste_pad)->m_NetCode;
		g_pt_chevelu->status = CH_ACTIF|CH_VISIBLE;
		g_pt_chevelu->dist = dist_min;
		g_pt_chevelu->pad_start = *pt_liste_pad;
		g_pt_chevelu->pad_end = *pt_liste_pad_tmp;

		if( DisplayRastnestInProgress && DC )
			GRLine(&DrawPanel->m_ClipBox, DC, g_pt_chevelu->pad_start->m_Pos.x,
						g_pt_chevelu->pad_start->m_Pos.y,
						g_pt_chevelu->pad_end->m_Pos.x,
						g_pt_chevelu->pad_end->m_Pos.y,
						g_DesignSettings.m_RatsnestColor) ;

		g_pt_chevelu++ ;
	}
	return(current_num_block) ;
}

/*****************************************************************************/
static int gen_rats_pad_to_pad(WinEDA_DrawPanel * DrawPanel, wxDC * DC,
				LISTE_PAD * pt_liste_pad,
				LISTE_PAD * pt_limite,int  current_num_block, int * nblinks)
/*****************************************************************************/
/*
 Routine utilisee par Build_Board_Ratsnest()
 Routine generant le chevelu entre 2 pads ( supposes du meme net )
	la routine connecte 1 pad non deja connecte a un autre et crée donc un certqins nombre
de blocks de pads liées par un chevelu
Ces blocks sont donc constitués de 2 pads.

		Entree :
			pt_chain_pad = adresse de debut de recherche
			pt_limite	  = adresse de fin de recherche (borne non comprise)
			current_num_block = numero du dernier block de pads (constitué par les connexions
			de pistes existantes

		Sortie:
			liste des chevelus ( structures)
			mise a jour de g_pt_chevelu a la 1ere case libre

		Retourne:
			nombre de blocks crees (paquets de pads)
 */
{
int dist_min, current_dist ;
LISTE_PAD * pt_liste_pad_tmp;
LISTE_PAD * pt_liste_pad_aux ;
LISTE_PAD * pt_start_liste;
D_PAD * ref_pad, * pad;

	pt_start_liste = pt_liste_pad ;

	if ( DC ) GRSetDrawMode(DC, GR_XOR);

	for (  ; pt_liste_pad < pt_limite ; pt_liste_pad++ )
	{
		ref_pad = *pt_liste_pad;
		if(ref_pad->m_logical_connexion) continue; // Pad deja connecte

		pt_liste_pad_tmp = NULL ; dist_min = 0x7FFFFFFF;

		for ( pt_liste_pad_aux = pt_start_liste ; ;pt_liste_pad_aux++)
		{
			if (pt_liste_pad_aux >= pt_limite ) break ;
			if(pt_liste_pad_aux == pt_liste_pad) continue ;

			pad = *pt_liste_pad_aux;
			/* Comparaison des distances des pastilles (calcul simplifie) */
			current_dist =
					  abs( pad->m_Pos.x - ref_pad->m_Pos.x)
					+ abs( pad->m_Pos.y - ref_pad->m_Pos.y);

			if(dist_min > current_dist)
			{
				dist_min = current_dist;
				pt_liste_pad_tmp = pt_liste_pad_aux;
			}
		}

		if (pt_liste_pad_tmp != NULL)
		{
			pad = *pt_liste_pad_tmp;

			/* Mise a jour du numero de block ( ou de sous graphe ) */
			/* si aucun des 2 pads n'est deja connecte : creation d'un nouveau block */
			if ( (pad->m_logical_connexion == 0) && (ref_pad->m_logical_connexion == 0) )
			{
				current_num_block++ ;
				pad->m_logical_connexion = current_num_block ;
				ref_pad->m_logical_connexion = current_num_block ;
			}
			/* si 1 des 2 pads est deja connecte : mise a jour pour l'autre */
			else
			{
				ref_pad->m_logical_connexion = pad->m_logical_connexion;
			}
			(*nblinks)++;
			g_pt_chevelu->m_NetCode = ref_pad->m_NetCode;
			g_pt_chevelu->status = CH_ACTIF|CH_VISIBLE;
			g_pt_chevelu->dist = dist_min;
			g_pt_chevelu->pad_start = ref_pad;
			g_pt_chevelu->pad_end = pad;

			if(DisplayRastnestInProgress)
			{
				GRLine(&DrawPanel->m_ClipBox, DC, g_pt_chevelu->pad_start->m_Pos.x,
							g_pt_chevelu->pad_start->m_Pos.y,
							g_pt_chevelu->pad_end->m_Pos.x,
							g_pt_chevelu->pad_end->m_Pos.y,
							g_DesignSettings.m_RatsnestColor);
			}
			g_pt_chevelu++ ;
		}
	}

	return(current_num_block) ;
}


/***********************************************************/
void WinEDA_BasePcbFrame::Build_Board_Ratsnest( wxDC * DC )
/***********************************************************/
/* Routine de calcul du chevelu complet du circuit (algorithme de LEE )
	les connexions physiques (pistes) ne sont pas ici prises en compte
	Il s'agit donc du chevelu de base qui ne depend que de la disposition des pads.

	- etablit la liste complete des pads si nécessaire
			les pads utiles (cad appartenant a un net ) sont appeles
			nodes (noeuds)
	et la trie par blocs de pads homogenes ( i.e. appartenant au meme net )

	- puis calcule le ratsnest selon l'algorithme de LEE, consistant a
		a - etablir le ratsnest entre 1 pad non "connecte" et son plus proche
			voisin. Ceci cree des "blocks" ou sous graphes non connectes entre
			eux
		b - "connecter" (par un chevelu) entre eux ces blocks en prenant le 1er block et
			en le connectant a son plus proche voisin par les 2 pads les plus
			proches (Iteration jusqu'a ce qu'il n'y ait plus qu'un seul block).

	 les chevelus calculés montrent les connexions "logiques"

	Entree = adr du buffer de classement (usuellement buf_work)
	met a jour :
		nb_nodes = nombre de pads connectes a un net ( pads "utiles")
		nb_links = nombre min de liens :
		     il y a n-1 liens par equipotentielle comportant n pads.

*/
{
LISTE_PAD * pt_liste_pad, * pt_start_liste, * pt_end_liste, * pt_liste_pad_limite;
D_PAD * pad;
int ii, num_block, nbpads;
CHEVELU * pt_deb_liste_ch;
int current_net_code, noconn;
EQUIPOT * equipot;

	m_Pcb->m_NbNoconnect = 0;
	m_Pcb->m_NbLinks = 0 ;

	if( m_Pcb->m_NbPads == 0 ) return;

	/* Etablissement de la liste des pads et leur net_codes si necessaire */
	if( (m_Pcb->m_Status_Pcb & NET_CODES_OK) == 0 ) recalcule_pad_net_code();

	pt_liste_pad = m_Pcb->m_Pads;
	for( ii = m_Pcb->m_NbPads ; ii > 0 ; pt_liste_pad++, ii-- )
	{
		pad = *pt_liste_pad;
		pad->m_logical_connexion = 0;
	}

	/* classement des pointeurs sur pads par nets */
	qsort(m_Pcb->m_Pads,m_Pcb->m_NbPads,sizeof(LISTE_PAD),
			(int(*)(const void *, const void *))tri_par_net) ;

	/* Allocation memoire du buffer des chevelus: il y a nb_nodes - 1 chevelu
	au maximum ( 1 node = 1 pad connecte ).
	on alloue donc un buffer pour nb_nodes chevelus... (+ une petite marge)
	le nombre reel de chevelus est nb_links
	*/
	if( m_Pcb->m_Ratsnest ) MyFree(m_Pcb->m_Ratsnest);
	m_Pcb->m_Ratsnest = NULL;

	if( m_Pcb->m_NbNodes == 0 ) return;	/* pas de connexions utiles */

	m_Pcb->m_Ratsnest = (CHEVELU*)MyZMalloc( (m_Pcb->m_NbNodes + 10 ) * sizeof(CHEVELU) );
	if(m_Pcb->m_Ratsnest == NULL) return;


	/* calcul du chevelu */
	DisplayRastnestInProgress = TRUE;
	g_pt_chevelu = m_Pcb->m_Ratsnest ;
	pt_liste_pad = pt_start_liste = m_Pcb->m_Pads;
	pt_liste_pad_limite = pt_start_liste + m_Pcb->m_NbPads;
	current_net_code = 1;	// 1er net_code a analyser (net_code = 0 -> no connect)
	equipot = m_Pcb->m_Equipots;
	noconn = 0;

	for ( ; pt_liste_pad < pt_liste_pad_limite ; )
		{
		pt_deb_liste_ch = g_pt_chevelu;
		pad = *pt_liste_pad;
		/* saut des pads non connectes */
		if(pad->m_NetCode == 0 )
			{
			pt_liste_pad++; pt_start_liste = pt_liste_pad;
			continue;
			}
		/* Recherche de la fin de la liste des pads du net courant */
		num_block = pad->m_logical_connexion;
		nbpads = 0;
		for( pt_end_liste = pt_liste_pad + 1 ; ; pt_end_liste++)
			{
			if (pt_end_liste >= pt_liste_pad_limite ) break ;
			pad = *pt_end_liste;
			if (pad->m_NetCode != current_net_code ) break ;
			nbpads++;
			if( num_block < pad->m_logical_connexion )
			  num_block = pad->m_logical_connexion;
			}
		m_Pcb->m_NbLinks += nbpads;

		/* fin de liste trouvee: calcul du chevelu du net "net_code" */
		equipot = GetEquipot(m_Pcb, current_net_code);
		if(equipot == NULL)
			DisplayError(this, wxT("Gen ratsnest err: NULL equipot") );
		else
			{
			equipot->m_NbNodes = nbpads;
			equipot->m_NbLink = nbpads+1;
			equipot->m_PadzoneStart = pt_start_liste;
			equipot->m_PadzoneEnd = pt_end_liste;
			equipot->m_RatsnestStart = g_pt_chevelu;
			}

		/* a - connexion des pads entre eux */
		ii = gen_rats_pad_to_pad(DrawPanel, DC, pt_start_liste,
					pt_end_liste, num_block, &noconn);

		/* b - connexion des blocks formes precedemment (Iteration) */
		while (ii > 1 )
			{
			ii = gen_rats_block_to_block(DrawPanel, DC, pt_liste_pad,
								pt_end_liste, &noconn);
			}

		if( equipot )
			{
			equipot->m_RatsnestEnd = g_pt_chevelu;
			/* classement des chevelus par longueur croissante */
			qsort(equipot->m_RatsnestStart,
				  equipot->m_RatsnestEnd - equipot->m_RatsnestStart,
				  sizeof(CHEVELU),
				  (int(*)(const void *, const void *))sort_by_length) ;
			}
		pt_liste_pad = pt_start_liste = pt_end_liste ;
		pt_deb_liste_ch = g_pt_chevelu;
		if(pt_start_liste < pt_liste_pad_limite)
				current_net_code = (*pt_start_liste)->m_NetCode;
		}


	m_Pcb->m_NbNoconnect = noconn;
	m_Pcb->m_Status_Pcb |= LISTE_CHEVELU_OK ;
	adr_lowmem = buf_work;

	// Effacement du chevelu calcule
	CHEVELU * Chevelu = (CHEVELU*)m_Pcb->m_Ratsnest;
	GRSetDrawMode(DC, GR_XOR);
	for( ii = m_Pcb->GetNumRatsnests(); ii > 0; ii--, Chevelu++ )
	{
		if( ! g_Show_Ratsnest ) Chevelu->status &= ~CH_VISIBLE;
        if ( DC ) GRLine(&DrawPanel->m_ClipBox, DC,
					Chevelu->pad_start->m_Pos.x, Chevelu->pad_start->m_Pos.y,
					Chevelu->pad_end->m_Pos.x, Chevelu->pad_end->m_Pos.y,
					g_DesignSettings.m_RatsnestColor);
	}

}


/**********************************************************************/
void WinEDA_BasePcbFrame::ReCompile_Ratsnest_After_Changes(wxDC *  DC )
/**********************************************************************/
/* recompile rastnest afet am module move, delete, ..
*/
{
	if ( g_Show_Ratsnest && DC ) Compile_Ratsnest( DC, TRUE );
}


/*********************************************************************/
void WinEDA_BasePcbFrame::DrawGeneralRatsnest(wxDC * DC, int net_code)
/*********************************************************************/
/*
	Affiche le chevelu general du circuit
	Affiche les chevelus dont le bit CH_VISIBLE du status du chevelu est a 1
   Si net_code > 0, affichage des seuls chevelus de net_code correspondant
*/
{
int ii;
CHEVELU * Chevelu;

	if( (m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 ) return;
	if( (m_Pcb->m_Status_Pcb & DO_NOT_SHOW_GENERAL_RASTNEST) ) return;
	if ( DC == NULL ) return;

	Chevelu = m_Pcb->m_Ratsnest;
	if( Chevelu == NULL ) return;

	GRSetDrawMode(DC, GR_XOR);
	for( ii = m_Pcb->GetNumRatsnests() ;ii > 0; Chevelu++, ii--)
		{
		if( (Chevelu->status & (CH_VISIBLE|CH_ACTIF)) != (CH_VISIBLE|CH_ACTIF) )
			continue;
		if( (net_code <= 0) || (net_code == Chevelu->m_NetCode) )
			{
			GRLine(&DrawPanel->m_ClipBox, DC,
						Chevelu->pad_start->m_Pos.x, Chevelu->pad_start->m_Pos.y,
						Chevelu->pad_end->m_Pos.x, Chevelu->pad_end->m_Pos.y,
						g_DesignSettings.m_RatsnestColor);
			}
		}
}



/*****************************************************************************/
static int tst_rats_block_to_block(WinEDA_DrawPanel * DrawPanel, wxDC * DC,
		LISTE_PAD * pt_liste_pad_start, LISTE_PAD * pt_liste_pad_end,
		CHEVELU* start_rat_list, CHEVELU* end_rat_list)
/*****************************************************************************/
/*
 Routine utilisee par Tst_Ratsnest()
	Routine tres proche de  gen_rats_block_to_block(..)
 Routine testant le chevelu entre 2 blocks ( supposes du meme net )
	la recherche est faite entre les pads du block 1 et les autres blocks
	le block n ( n > 1 ) est alors connecte au block 1 par le chevelu le plus court
	A la différence de gen_rats_block_to_block(..),
	l'analyse n'est pas faite pads a pads mais a travers la liste générale des chevelus.
	La routine active alors le chevelu le plus court reliant le block 1 au block n
	(etablissement d'une connexion "logique")

		Entree :
			pt_chain_pad = adresse de debut de zone pad utile
			pt_limite	  = adresse de fin de zone (borne non comprise)
		Sortie:
			Membre .state du chevelu sélectionné
		Retourne:
			nombre de blocks non connectes entre eux
 */
{
int current_num_block, min_block ;
LISTE_PAD * pt_liste_pad;
CHEVELU * chevelu, * min_chevelu;

	/* Recherche du chevelu le plus court d'un block a un autre block */
	min_chevelu = NULL;
	for ( chevelu = start_rat_list; chevelu < end_rat_list ; chevelu++ )
		{
		if( chevelu->pad_start->m_logical_connexion == chevelu->pad_end->m_logical_connexion )
			continue ;
		if( min_chevelu == NULL ) min_chevelu = chevelu;
		else if(min_chevelu->dist > chevelu->dist) min_chevelu = chevelu;
		}

	if( min_chevelu == NULL ) return 1;

	min_chevelu->status |= CH_ACTIF;
	current_num_block = min_chevelu->pad_start->m_logical_connexion;
	min_block = min_chevelu->pad_end->m_logical_connexion;
	if (min_block > current_num_block ) EXCHG(min_block, current_num_block);

	/* les 2 blocks vont etre fondus */
	for( pt_liste_pad = pt_liste_pad_start; pt_liste_pad < pt_liste_pad_end; pt_liste_pad++ )
	{
		if( (*pt_liste_pad)->m_logical_connexion == current_num_block)
		{
			(*pt_liste_pad)->m_logical_connexion = min_block;
		}
	}

	return(current_num_block) ;
}

/*********************************************************************/
static int tst_rats_pad_to_pad(WinEDA_DrawPanel * DrawPanel, wxDC * DC,
				int  current_num_block,
				CHEVELU* start_rat_list, CHEVELU* end_rat_list)
/**********************************************************************/
/*
 Routine utilisee par Tst_Ratsnest_general()
 Routine Activant le chevelu entre 2 pads ( supposes du meme net )
	la routine connecte 1 pad non deja connecte a un autre et active donc
	un certain nombre de blocks de pads liées par un chevelu
Ces blocks sont donc constitués de 2 pads.

		Entree :
			pt_chain_pad = adresse de debut de zone pad
			pt_limite	  = adresse de fin de recherche (borne non comprise)
			current_num_block = numero du dernier block de pads (constitué par les connexions
			de pistes existantes

		Sortie:
			liste des chevelus ( structures)
			mise a jour du membre .state du chevelu activé

		Retourne:
			nombre de blocks crees (paquets de pads)
*/
{
D_PAD * pad_start, * pad_end;
CHEVELU * chevelu;

	for ( chevelu = start_rat_list; chevelu < end_rat_list; chevelu++ )
	{
		pad_start = chevelu->pad_start; pad_end = chevelu->pad_end;
		/* Mise a jour du numero de block ( ou de sous graphe ) */

		/* si aucun des 2 pads n'est deja connecte : creation d'un nouveau block */
		if ( (pad_start->m_logical_connexion == 0) && (pad_end->m_logical_connexion == 0) )
		{
			current_num_block++ ;
			pad_start->m_logical_connexion = current_num_block ;
			pad_end->m_logical_connexion = current_num_block;
			chevelu->status |= CH_ACTIF;
		}
			/* si 1 des 2 pads est deja connecte : mise a jour pour l'autre */
		else if ( pad_start->m_logical_connexion == 0)
		{
			pad_start->m_logical_connexion = pad_end->m_logical_connexion;
			chevelu->status |= CH_ACTIF;
		}
		else if ( pad_end->m_logical_connexion == 0)
		{
			pad_end->m_logical_connexion = pad_start->m_logical_connexion;
			chevelu->status |= CH_ACTIF;
		}
	}

	return(current_num_block) ;
}


/******************************************************************/
void WinEDA_BasePcbFrame::Tst_Ratsnest(wxDC * DC, int ref_netcode )
/*******************************************************************/

/* calcul du chevelu actif
	Le chevelu général doit etre calculé
	Determite les chevelus ACTIFS dans la liste générale des chevelus
*/
{
LISTE_PAD * pt_liste_pad;
CHEVELU * chevelu;
D_PAD * pad;
int ii, num_block;
int net_code;
EQUIPOT * equipot;

	if( m_Pcb->m_NbPads == 0 ) return;

	for ( net_code = 1; ; net_code++)
		{
		equipot = GetEquipot(m_Pcb, net_code);
		if(equipot == NULL ) break;
		if( ref_netcode && (net_code != ref_netcode) ) continue;

		num_block = 0;
		pt_liste_pad = equipot->m_PadzoneStart;
		for( ;pt_liste_pad < equipot->m_PadzoneEnd; pt_liste_pad++)
			{
			pad = *pt_liste_pad;
			pad->m_logical_connexion = pad->m_physical_connexion;
			num_block = max (num_block, pad->m_logical_connexion);
			}

		for ( chevelu = equipot->m_RatsnestStart; chevelu < equipot->m_RatsnestEnd; chevelu++ )
			{
			chevelu->status &= ~CH_ACTIF;
			}


		/* a - tst connexion des pads entre eux */
		ii = tst_rats_pad_to_pad(DrawPanel, DC, num_block,
				equipot->m_RatsnestStart, equipot->m_RatsnestEnd);

		/* b - connexion des blocks formes precedemment (Iteration) */
		while (ii > 1 )
			{
			ii = tst_rats_block_to_block(DrawPanel, DC,
						equipot->m_PadzoneStart, equipot->m_PadzoneEnd,
						equipot->m_RatsnestStart, equipot->m_RatsnestEnd);
			}
		}


	m_Pcb->m_NbNoconnect = 0;
	CHEVELU * Chevelu = m_Pcb->m_Ratsnest;
	for( ii = m_Pcb->GetNumRatsnests(); ii > 0; ii--, Chevelu++ )
	{
		if( Chevelu->status & CH_ACTIF )
			m_Pcb->m_NbNoconnect++;
	}
}

/**************************************************************************/
int WinEDA_BasePcbFrame::Test_1_Net_Ratsnest(wxDC * DC, int ref_netcode)
/**************************************************************************/
/* Calcule le chevelu du net net_code */
{

	DisplayRastnestInProgress = FALSE;
	DrawGeneralRatsnest(DC, ref_netcode);
	Tst_Ratsnest(DC, ref_netcode );
	DrawGeneralRatsnest(DC, ref_netcode);

	return m_Pcb->GetNumRatsnests();
}

/*****************************************************/
void WinEDA_BasePcbFrame::recalcule_pad_net_code(void)
/*****************************************************/
/*
	Calcule et met a jour les net_codes des PADS et des equipotentielles
	met a jour le buffer des equipotentielles
	A utiliser apres edition de nets sur un pad ou lecture d'une netliste
	positionne a 1 le bit NET_CODE_OK du status_pcb;
*/
{
LISTE_PAD * pad_ref, *pad_courant;
int ii, jj;
EQUIPOT * pt_equipot;
EDA_BaseStruct * PtStruct;
EQUIPOT ** BufPtEquipot;

	/* construction de la liste des adr des PADS */
	build_liste_pads();

	/* calcul des net_codes des pads */
	ii = m_Pcb->m_NbPads;
	m_Pcb->m_NbNodes = 0;
	m_Pcb->m_NbNets = 0;

	pad_courant = m_Pcb->m_Pads;
	for( ; ii > 0 ; pad_courant++, ii-- )
		{
		if( (*pad_courant)->m_Netname.IsEmpty()) // pad non connecte
			{
			(*pad_courant)->m_NetCode = 0 ; continue ;
			}
		m_Pcb->m_NbNodes++;
		/* si le netname a deja ete rencontre: mise a jour , sinon nouveau net_code */
		pad_ref = m_Pcb->m_Pads ;
		while (pad_ref < pad_courant )
			{
			if( (*pad_ref)->m_Netname == (*pad_courant)->m_Netname )
					break; // sont du meme met
			pad_ref++ ;
			}

		/* si pad_ref = pad_courant: nouveau net sinon, deja net deja traite */
		if ( pad_ref == pad_courant )
			{
			m_Pcb->m_NbNets++ ; (*pad_courant)->m_NetCode = m_Pcb->m_NbNets;
			}
		 else (*pad_courant)->m_NetCode = (*pad_ref)->m_NetCode ;
		}

	/* Construction ou correction de la liste des equipotentielles,
	et construction d'un tableau d'adressage des equipots*/

	BufPtEquipot = (EQUIPOT**)MyMalloc(sizeof(EQUIPOT*) * (m_Pcb->m_NbNets+1));
	pt_equipot = m_Pcb->m_Equipots;
	PtStruct = (EDA_BaseStruct *) m_Pcb;
	for( ii = 0 ; ii <= m_Pcb->m_NbNets ;ii++ )
		{
		if(pt_equipot == NULL )	/* Creation d'une nouvelle equipot */
			{
			pt_equipot = new EQUIPOT(m_Pcb);

			if( ii == 0 )
				{
				m_Pcb->m_Equipots = pt_equipot;
				pt_equipot->Pback = m_Pcb;
				}
			else
				{
				PtStruct->Pnext = pt_equipot;
				pt_equipot->Pback = PtStruct;
				}
			pt_equipot->Pnext = NULL;
			}

		pt_equipot->m_NetCode = ii; // Mise a jour du numero d'equipot
		pt_equipot->m_NbNodes  = 0;
		pt_equipot->m_Netname.Empty();

		BufPtEquipot[ii] = pt_equipot;
		PtStruct =  (EDA_BaseStruct *) pt_equipot;
		pt_equipot = (EQUIPOT *) pt_equipot->Pnext;
		}

	/* Effacement des equipots inutiles */

	while ( pt_equipot )
		{
		PtStruct = pt_equipot->Pnext;
		DeleteStructure(pt_equipot);
		pt_equipot = (EQUIPOT*) PtStruct;
		}

	pad_courant = m_Pcb->m_Pads;
	pt_equipot = m_Pcb->m_Equipots;

	/* Placement des noms de net en structure EQUIPOT */
	for( ii = m_Pcb->m_NbPads ; ii > 0 ; pad_courant++, ii-- )
		{
		jj = (*pad_courant)->m_NetCode;
		pt_equipot = BufPtEquipot[jj];
		pt_equipot->m_NbNodes ++;
		if(pt_equipot->m_Netname.IsEmpty())
			{
			pt_equipot->m_Netname = (*pad_courant)->m_Netname;
			}
		}
	MyFree(BufPtEquipot);
	m_Pcb->m_Status_Pcb |= NET_CODES_OK;
}



/***********************************************/
void WinEDA_BasePcbFrame::build_liste_pads(void)
/***********************************************/
/*
 construction de la liste ( sous forme d'une liste de stucture )
 des caract utiles des pads du PCB pour autoroutage,DRC .. )
	parametres:
		adresse du buffer de classement = buf_work
	retourne:
		1ere adresse disponible si OK
		NULL si trop de pastilles


Parametres de routage calcules et mis a jour
- parametre net_code:
	numero de code interne de chaque net du PCB.
	permet d'accelerer les calculs de chevelu et de connexions
- parametre .link est mis a jour
	pour chaque pastille, il indique le nombre d'autres pastilles du meme net
	appartenant au meme module.

Variables globales mise a jour:
	pointeur base_adr_liste_pad (adr de classement de la liste des pads)
	nb_pads = nombre total de pastilles du PCB
	nb_nets = nombre de nets differents
	status_pcb |= LISTE_PAD_OK (flag permettant d'eviter la reexecution inutile
							de cette routine)
*/
{
LISTE_PAD* pt_liste_pad;
MODULE * Module;
D_PAD * PtPad;

	if( m_Pcb->m_Status_Pcb & LISTE_PAD_OK ) return;

	/* construction de la liste des pointeurs sur les structures D_PAD */
	if( m_Pcb->m_Pads ) MyFree(m_Pcb->m_Pads);
	m_Pcb->m_Pads = NULL;

	/* Calcul du nombre de pads */
	Module = m_Pcb->m_Modules; m_Pcb->m_NbPads = 0;
	for( ; Module != NULL ; Module = (MODULE*) Module->Pnext)
		{
		PtPad = (D_PAD*) Module->m_Pads;
		for(; PtPad != NULL; PtPad = (D_PAD*) PtPad->Pnext ) m_Pcb->m_NbPads++;
		}

	if( m_Pcb->m_NbPads == 0 ) return;

	/* Allocation memoire du buffer */
	pt_liste_pad = m_Pcb->m_Pads
					 = (D_PAD**) MyZMalloc( (m_Pcb->m_NbPads+1) * sizeof(D_PAD*) );
	m_Pcb->m_NbNodes = 0;

	/* Initialisation du buffer et des variables de travail */
	Module = m_Pcb->m_Modules;
	for( ; Module != NULL ; Module = (MODULE*) Module->Pnext)
		{
		PtPad = (D_PAD*) Module->m_Pads;
		for(; PtPad != NULL; PtPad = (D_PAD*) PtPad->Pnext )
			{
			*pt_liste_pad = PtPad;
			PtPad->m_logical_connexion = 0 ;
			PtPad->m_Parent = Module;
			if(PtPad->m_NetCode) m_Pcb->m_NbNodes++;
			pt_liste_pad++;
			}
		}

   *pt_liste_pad = NULL;	// fin de liste

	adr_lowmem = buf_work;
	if( m_Pcb->m_Ratsnest ) MyFree(m_Pcb->m_Ratsnest);
	m_Pcb->m_Ratsnest = NULL;
	m_Pcb->m_Status_Pcb = LISTE_PAD_OK;
}


/*****************************************************************************/
char *  WinEDA_BasePcbFrame::build_ratsnest_module(wxDC * DC, MODULE * Module)
/*****************************************************************************/
/*
 construction de la liste en mode de calcul rapide pour affichage
 en temps reel lors des deplacements du chevelu d'un module.

	parametres d'appel:
		Module = pointeur sur module dont le ratsnest est a calculer

	retourne: adresse memoire disponible
	Le chevelu calcule comporte 2 parties
		- un chevelu interne relatif aux pads du module appartenant a un
			meme net. Il est calcule 1 seule fois
		- le chevelu externe reliant un pad interne a un pad externe au module
			Ce chevelu est recalcule a chaque deplacement
*/
{
LISTE_PAD	* pt_liste_pad,
			* pt_liste_ref,
			* pt_liste_generale;
D_PAD		* pad_ref,
			* pad_externe;
LISTE_PAD	* pt_liste_pad_limite,
			* pt_start_liste,
			* pt_end_liste;
int	ii, jj;
CHEVELU		* local_chevelu;
static CHEVELU  * pt_fin_int_chevelu;	// pointeur sur la fin de la liste
										// des chevelus internes au module
static int		nb_int_chevelu;			// nombre e chevelus internes
int current_net_code;
int increment, distance;	// variables de calcul de ratsnest
int pad_pos_X, pad_pos_Y;	// position reelle des pads du module en mouvement


	if((m_Pcb->m_Status_Pcb & LISTE_PAD_OK) == 0 ) build_liste_pads();

	/* construction de la liste des pads du module si necessaire */
	if((m_Pcb->m_Status_Pcb & CHEVELU_LOCAL_OK) != 0) goto calcul_chevelu_ext;

	/* calcul du chevelu "interne", c.a.d. liant les seuls pads du module */
	pt_liste_pad = (LISTE_PAD*) adr_lowmem; nb_pads_ref = 0;
	pad_ref = Module->m_Pads;
	for( ; pad_ref != NULL ; pad_ref = (D_PAD*)pad_ref->Pnext)
		{
		if(pad_ref->m_NetCode == 0) continue;
		*pt_liste_pad = pad_ref;
		pad_ref->m_logical_connexion = 0;
		pad_ref->m_physical_connexion = 0;
		pt_liste_pad++; nb_pads_ref++;
		}

	if(nb_pads_ref == 0) return( (char*)pt_liste_pad); /* pas de connexions! */

	qsort(adr_lowmem,nb_pads_ref,sizeof(D_PAD*),
					(int(*)(const void *, const void *))tri_par_net) ;

	/* construction de la liste des pads connectes aux pads de ce module */
	DisplayRastnestInProgress = FALSE;
	pt_liste_ref = (LISTE_PAD*) adr_lowmem;
	nb_pads_externes = 0; current_net_code = 0;
	for(ii = 0 ; ii < nb_pads_ref ; ii++)
		{
		pad_ref = pt_liste_ref[ii];
		if(pad_ref->m_NetCode == current_net_code) continue;
		current_net_code = pad_ref->m_NetCode;

		pt_liste_generale = m_Pcb->m_Pads ;
		for(jj = m_Pcb->m_NbPads ; jj > 0; jj--)
			{
			pad_externe = *pt_liste_generale ; pt_liste_generale++;
			if(pad_externe->m_NetCode != current_net_code) continue;
			if(pad_externe->m_Parent == Module) continue;
			pad_externe->m_logical_connexion = 0;
			pad_externe->m_physical_connexion = 0;
			*pt_liste_pad = pad_externe ; pt_liste_pad++;
			nb_pads_externes++;
			}
		}
	/* tri par net_codes croissants de la liste des pads externes */
	qsort(pt_liste_ref+nb_pads_ref,nb_pads_externes,sizeof(D_PAD*),
					(int(*)(const void *, const void *))tri_par_net) ;

	/* calcul du chevelu interne au module:
		Ce calcul est identique au calcul du chevelu general, mais il est
		restreint aux seuls pads du module courant */
	local_liste_chevelu = (CHEVELU*)(pt_liste_pad);	// buffer chevelu a la suite de la liste des pads
	nb_local_chevelu = 0;
	pt_liste_ref = (LISTE_PAD*) adr_lowmem;

	g_pt_chevelu = local_liste_chevelu ;
	pt_liste_pad = pt_start_liste = (LISTE_PAD *) adr_lowmem ;
	pt_liste_pad_limite = pt_liste_pad + nb_pads_ref;
	current_net_code = (*pt_liste_pad)->m_NetCode ;

	for ( ; pt_liste_pad < pt_liste_pad_limite ; )
		{
		/* Recherche de la fin de la liste des pads du net courant */

		for( pt_end_liste = pt_liste_pad + 1 ; ; pt_end_liste++)
			{
			if (pt_end_liste >= pt_liste_pad_limite ) break ;
			if ((*pt_end_liste)->m_NetCode != current_net_code ) break ;
			}

		/* fin de liste trouvee : */
		/* a - connexion des pads entre eux */
		ii = gen_rats_pad_to_pad(DrawPanel, DC, pt_start_liste,pt_end_liste,
								0, &nb_local_chevelu ) ;

		/* b - connexion des blocks formes precedemment (Iteration) */
		while (ii > 1 )
			{
			ii = gen_rats_block_to_block(DrawPanel, DC, pt_liste_pad,
						pt_end_liste, &nb_local_chevelu ) ;
			}

		pt_liste_pad = pt_start_liste = pt_end_liste ;
		if(pt_start_liste < pt_liste_pad_limite)
				current_net_code = (*pt_start_liste)->m_NetCode ;
		}

	pt_fin_int_chevelu = local_chevelu = g_pt_chevelu;
	nb_int_chevelu = nb_local_chevelu;

	/* Mise a 1 du flag LOCAL */
	g_pt_chevelu = local_liste_chevelu;
	while(g_pt_chevelu < pt_fin_int_chevelu)
		{
		g_pt_chevelu->status = LOCAL_CHEVELU; g_pt_chevelu++;
		}

	m_Pcb->m_Status_Pcb |= CHEVELU_LOCAL_OK;

	/////////////////////////////////////////
	// calcul du chevelu externe au module //
	/////////////////////////////////////////
calcul_chevelu_ext:
	/* Cette partie est executee a chaque deplacement du module: on calcule
	pour chaque pad du module courant la + courte distance a un pad externe.
	Pour chaque groupe de pad du module courant appartenant a un meme net,
	on ne garde qu'un seul chevelu: le plus court.
	*/
	local_chevelu = pt_fin_int_chevelu;
	nb_local_chevelu = nb_int_chevelu;
	pt_liste_ref = (LISTE_PAD*) adr_lowmem;
	pad_ref = *pt_liste_ref;
	current_net_code = pad_ref->m_NetCode;
	local_chevelu->dist = 0x7FFFFFFF;
	local_chevelu->status = 0;
	increment = 0;
	for(ii = 0 ; ii < nb_pads_ref ; ii++)
		{
		pad_ref = *(pt_liste_ref+ii);
		if(pad_ref->m_NetCode != current_net_code)
			{ /* un nouveau chevelu est cree (si necessaire) pour
			 chaque nouveau net */
			if(increment)
				{
				nb_local_chevelu++; local_chevelu++;
				}
			increment = 0;
			current_net_code = pad_ref->m_NetCode;
			local_chevelu->dist = 0x7FFFFFFF;
			}

		pad_pos_X = pad_ref->m_Pos.x - g_Offset_Module.x;
		pad_pos_Y = pad_ref->m_Pos.y - g_Offset_Module.y;
		pt_liste_generale = pt_liste_ref + nb_pads_ref ;

		for(jj = nb_pads_externes ; jj > 0; jj--)
			{
			pad_externe = *pt_liste_generale ; pt_liste_generale++;
			/* les netcodes doivent etre identiques */
			if(pad_externe->m_NetCode < pad_ref->m_NetCode) continue;
			if(pad_externe->m_NetCode > pad_ref->m_NetCode) break;
			distance = abs(pad_externe->m_Pos.x - pad_pos_X) +
						abs(pad_externe->m_Pos.y - pad_pos_Y);
			if(distance < local_chevelu->dist)
				{
				local_chevelu->pad_start = pad_ref;
				local_chevelu->pad_end = pad_externe;
				local_chevelu->m_NetCode = pad_ref->m_NetCode;
				local_chevelu->dist = distance;
				local_chevelu->status = 0;
				increment = 1;
				}
			}
		}
	if(increment) // fin de balayage : le ratsnest courant doit etre memorise
		{
		nb_local_chevelu++; local_chevelu++;
		}

	/* Retourne l'adr de la zone disponible */
	adr_max = max(adr_max, (char*)(local_chevelu+1));

	return( (char*) (local_chevelu+1) ) ; /* la struct pointee par
											 local_chevelu est utilisee
											 pour des calculs temporaires */
}


/***********************************************************/
void WinEDA_BasePcbFrame::trace_ratsnest_module(wxDC * DC)
/**********************************************************/
/*
 affiche le chevelu d'un module calcule en mode rapide.
	retourne: rien
*/
{
CHEVELU* local_chevelu;
int ii;

	if((m_Pcb->m_Status_Pcb & CHEVELU_LOCAL_OK) == 0) return ;

	local_chevelu = local_liste_chevelu;
	ii = nb_local_chevelu;

	GRSetDrawMode(DC, GR_XOR);

	while( ii-- > 0 )
		{
		if(local_chevelu->status & LOCAL_CHEVELU)
			{
			GRLine(&DrawPanel->m_ClipBox, DC,
				local_chevelu->pad_start->m_Pos.x - g_Offset_Module.x,
				local_chevelu->pad_start->m_Pos.y - g_Offset_Module.y,
				local_chevelu->pad_end->m_Pos.x - g_Offset_Module.x,
				local_chevelu->pad_end->m_Pos.y - g_Offset_Module.y,
				YELLOW);
			}
		else
			{
			GRLine(&DrawPanel->m_ClipBox, DC,
				local_chevelu->pad_start->m_Pos.x - g_Offset_Module.x,
				local_chevelu->pad_start->m_Pos.y - g_Offset_Module.y,
				local_chevelu->pad_end->m_Pos.x,
				local_chevelu->pad_end->m_Pos.y,
				g_DesignSettings.m_RatsnestColor);
			}
		local_chevelu++;
		}
}



/*********************************************************************************************/
/* int * WinEDA_BasePcbFrame::build_ratsnest_pad(D_PAD * pad_ref, const wxPoint & refpos) */
/*********************************************************************************************/

/*
 construction de la liste en mode de calcul rapide pour affichage
 en temps reel du chevelu d'un pad lors des tracés d'une piste démarrant
 sur ce pad.

	parametres d'appel:
		pad_ref ( si null : mise a 0 du nombre de chevelus )
		ox, oy = coord de l'extremite de la piste en trace
		init (flag)
			= 0 : mise a jour des chevelu
			<> 0:	creation de la liste
	retourne: adresse memoire disponible
*/

/* routine locale de tri par longueur de links utilisee par la fonction QSORT */
static int sort_by_localnetlength(int * ref, int * compare)
{
int * org = (int*)adr_lowmem;
int ox = *org++;
int oy = *org++;
int lengthref, lengthcmp;

	lengthref = abs( *ref - ox);
	ref++;
	lengthref += abs( *ref - oy);	// = longueur entre point origine et pad ref
	lengthcmp = abs( *compare - ox);
	compare++;
	lengthcmp += abs( *compare - oy);	// = longueur entre point origine et pad comparé

	return( lengthref - lengthcmp );
}

/****************************************************************************************/
int * WinEDA_BasePcbFrame::build_ratsnest_pad(EDA_BaseStruct * ref,
		const wxPoint & refpos, bool init)
/****************************************************************************************/
{
int ii;
int * pt_coord, * base_data;
int current_net_code = 0, conn_number = 0;
LISTE_PAD * padlist;
D_PAD * pad_ref = NULL;

	if( ((m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 ) ||
		((m_Pcb->m_Status_Pcb & LISTE_PAD_OK) == 0) )
	{
		nb_local_chevelu = 0;
		return(NULL);
	}


	base_data = pt_coord = (int *) adr_lowmem;
	local_liste_chevelu = (CHEVELU *) pt_coord;
	if (init)
	{
		nb_local_chevelu = 0;
		if(ref == NULL) return(NULL);

		switch ( ref->m_StructType )
		{
			case TYPEPAD:
				pad_ref = (D_PAD*) ref;
				current_net_code = pad_ref->m_NetCode;
				conn_number = pad_ref->m_physical_connexion;
				break;

			case TYPETRACK:
			case TYPEVIA:
			{
				TRACK *track_ref = (TRACK*) ref;
				current_net_code = track_ref->m_NetCode;
				conn_number = track_ref->m_Sous_Netcode;
				break;
			}
				
		}
		if ( current_net_code <= 0 ) return NULL;

		*pt_coord = refpos.x; pt_coord++;
		*pt_coord = refpos.y; pt_coord++;

		if( m_Pcb->m_Ratsnest == NULL ) return(NULL);

		padlist = m_Pcb->m_Pads;
		for( ii = 0 ;ii < m_Pcb->m_NbPads; padlist++, ii++)
		{
			D_PAD * pad = *padlist;
			if( pad->m_NetCode != current_net_code ) continue;
			if( pad == pad_ref ) continue;
			if( !pad->m_physical_connexion || (pad->m_physical_connexion != conn_number) )
			{
				*pt_coord = pad->m_Pos.x; pt_coord++;
				*pt_coord = pad->m_Pos.y; pt_coord++;
				nb_local_chevelu++;
			}
		}
	}	/* Fin Init */

	else if( nb_local_chevelu )
	{
		*pt_coord = refpos.x; *(pt_coord+1) = refpos.y;
	}

	qsort(base_data + 2,nb_local_chevelu,2*sizeof(int),
					(int(*)(const void *, const void *))sort_by_localnetlength) ;
	return pt_coord;
}

/*******************************************************/
void WinEDA_BasePcbFrame::trace_ratsnest_pad(wxDC * DC)
/*******************************************************/
/*
 affiche le "chevelu" d'un pad lors des trace de segments de piste
*/
{
int * pt_coord;
int ii;
int refX, refY;

	if((m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0) return;
	if( nb_local_chevelu == 0 ) return;
	if ( local_liste_chevelu == NULL ) return;

	pt_coord = (int*) local_liste_chevelu;
	refX = *pt_coord; pt_coord++;
	refY = *pt_coord; pt_coord++;

	GRSetDrawMode(DC, GR_XOR);
	for( ii = 0; ii < nb_local_chevelu; ii++)
		{
		if ( ii >= g_MaxLinksShowed ) break;
		GRLine(&DrawPanel->m_ClipBox, DC, refX, refY, *pt_coord, *(pt_coord+1), YELLOW);
		pt_coord += 2;
		}
}

