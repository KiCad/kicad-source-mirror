	/*******************************************/
	/* class_board.cpp - BOARD class functions */
	/*******************************************/
#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"

#include "bitmaps.h"



	/*****************/
	/* Class BOARD: */
	/*****************/

/* Constructor */
BOARD::BOARD(EDA_BaseStruct * parent, WinEDA_BasePcbFrame * frame):
		EDA_BaseStruct(parent, TYPEPCB)
{
	m_PcbFrame = frame;
	m_Status_Pcb = 0;					// Mot d'etat: Bit 1 = Chevelu calcule
	m_NbNets = 0;					// Nombre de nets (equipotentielles)
	m_BoardSettings = &g_DesignSettings;
	m_NbPads = 0;					// nombre total de pads
	m_NbNodes = 0;					// nombre de pads connectes
	m_NbLinks = 0;					// nombre de chevelus (donc aussi nombre
									// minimal de pistes a tracer
	m_NbSegmTrack = 0;				// nombre d'elements de type segments de piste
	m_NbSegmZone = 0;				// nombre d'elements de type segments de zone
	m_NbNoconnect=0;				// nombre de chevelus actifs
	m_NbLoclinks = 0;				// nb ratsnest local

	m_Drawings = NULL;			// pointeur sur liste drawings
	m_Modules = NULL;				// pointeur sur liste zone modules
	m_Equipots = NULL;			// pointeur liste zone equipot
	m_Track = NULL;				// pointeur relatif zone piste
	m_Zone = NULL;				// pointeur tableau zone zones de cuivre
	m_Pads = NULL;				// pointeur liste d'acces aux pads
	m_Ratsnest = NULL;			// pointeur liste rats
	m_LocalRatsnest = NULL;		// pointeur liste rats local
	m_CurrentLimitZone = NULL;	// pointeur liste des EDEGE_ZONES
								// de determination des contours de zone
}

	/***************/
	/* Destructeur */
	/***************/
BOARD::~BOARD(void)
{
}


void BOARD::UnLink( void )
{
	/* Modification du chainage arriere */
	if( Pback )
		{
		if( Pback->m_StructType == TYPEPCB)
			{
			Pback->Pnext = Pnext;
			}

		else /* Le chainage arriere pointe sur la structure "Pere" */
			{
//			Pback-> = Pnext;
			}
		}

	/* Modification du chainage avant */
	if( Pnext) Pnext->Pback = Pback;

	Pnext = Pback = NULL;
}

/* Routines de calcul des nombres de segments pistes et zones */
int BOARD::GetNumSegmTrack(void)
{
TRACK * CurTrack = m_Track;
int ii = 0;

	for ( ;CurTrack != NULL; CurTrack = (TRACK*)CurTrack->Pnext ) ii++;
	m_NbSegmTrack = ii;
	return(ii);
}
int BOARD::GetNumSegmZone(void)
{
TRACK * CurTrack = m_Zone;
int ii = 0;

	for ( ;CurTrack != NULL; CurTrack = (TRACK*)CurTrack->Pnext ) ii++;
	m_NbSegmZone = ii;
	return(ii);
}


	// retourne le nombre de connexions manquantes
int BOARD::GetNumNoconnect(void)
{
	return(m_NbNoconnect);
}

	// retourne le nombre de chevelus
int BOARD::GetNumRatsnests(void)
{
	return(m_NbLinks);
}

	// retourne le nombre de pads a netcode > 0
int BOARD::GetNumNodes(void)
{
	return(m_NbNodes);
}


/***********************************/
bool BOARD::ComputeBoundaryBox(void)
/***********************************/

/* Determine le rectangle d'encadrement du pcb
	Ce rectangle englobe les contours pcb, pads , vias et piste
	Sortie:
	m_PcbBox

	retourne:
		0 si aucun element utile
		1 sinon
*/
{
int rayon, cx, cy, d, xmin, ymin, xmax, ymax;
bool Has_Items = FALSE;
EDA_BaseStruct * PtStruct;
DRAWSEGMENT* ptr;
TRACK * Track;

	xmin = ymin =  0x7FFFFFFFl ;
	xmax = ymax = -0x7FFFFFFFl ;

	/* Analyse des Contours PCB */
	PtStruct = m_Drawings;
	for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
	{
		if( PtStruct->m_StructType != TYPEDRAWSEGMENT ) continue;
		ptr = (DRAWSEGMENT*) PtStruct;
		d = (ptr->m_Width /2) + 1;
		if(ptr->m_Shape == S_CIRCLE)
		{
			cx = ptr->m_Start.x; cy = ptr->m_Start.y;
			rayon = (int)hypot((double)(ptr->m_End.x-cx),(double)(ptr->m_End.y-cy) );
			rayon += d;
			xmin = min(xmin,cx-rayon);
			ymin = min(ymin,cy-rayon);
			xmax = max(xmax,cx+rayon);
			ymax = max(ymax,cy+rayon);
			Has_Items = TRUE;
		}
		else
		{
			cx = min(ptr->m_Start.x, ptr->m_End.x );
			cy = min(ptr->m_Start.y, ptr->m_End.y);
			xmin = min(xmin,cx - d);
			ymin = min(ymin,cy - d);
			cx = max(ptr->m_Start.x, ptr->m_End.x );
			cy = max(ptr->m_Start.y, ptr->m_End.y);
			xmax = max(xmax,cx + d);
			ymax = max(ymax,cy + d);
			Has_Items = TRUE;
		}
	}

	/* Analyse des Modules  */
	MODULE * module = m_Modules;
	for( ; module != NULL; module = (MODULE *) module->Pnext )
	{
		Has_Items = TRUE;
		xmin = min(xmin,(module->m_Pos.x + module->m_BoundaryBox.GetX()));
		ymin = min(ymin,(module->m_Pos.y + module->m_BoundaryBox.GetY()));
		xmax = max(xmax,module->m_Pos.x + module->m_BoundaryBox.GetRight());
		ymax = max(ymax,module->m_Pos.y + module->m_BoundaryBox.GetBottom());

		D_PAD * pt_pad = module->m_Pads;
		for ( ;pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
		{
			d = pt_pad->m_Rayon;
			xmin = min(xmin,pt_pad->m_Pos.x - d);
			ymin = min(ymin,pt_pad->m_Pos.y - d);
			xmax = max(xmax,pt_pad->m_Pos.x + d);
			ymax = max(ymax,pt_pad->m_Pos.y + d);
		}
	}

	/* Analyse des segments de piste et zone*/
	for( Track = m_Track; Track != NULL; Track = (TRACK*) Track->Pnext)
	{
		d = (Track->m_Width /2) + 1;
		cx = min(Track->m_Start.x, Track->m_End.x );
		cy = min(Track->m_Start.y, Track->m_End.y);
		xmin = min(xmin,cx - d);
		ymin = min(ymin,cy - d);
		cx = max(Track->m_Start.x, Track->m_End.x );
		cy = max(Track->m_Start.y, Track->m_End.y);
		xmax = max(xmax,cx + d);
		ymax = max(ymax,cy + d);
		Has_Items = TRUE;
	}

	for( Track = m_Zone; Track != NULL; Track = (TRACK*) Track->Pnext)
	{
		d = (Track->m_Width /2) + 1;
		cx = min(Track->m_Start.x, Track->m_End.x );
		cy = min(Track->m_Start.y, Track->m_End.y);
		xmin = min(xmin,cx - d);
		ymin = min(ymin,cy - d);
		cx = max(Track->m_Start.x, Track->m_End.x );
		cy = max(Track->m_Start.y, Track->m_End.y);
		xmax = max(xmax,cx + d);
		ymax = max(ymax,cy + d);
		Has_Items = TRUE;
	}

	if ( ! Has_Items && m_PcbFrame )
	{
		if ( m_PcbFrame->m_Draw_Sheet_Ref )
		{
			xmin = ymin = 0;
			xmax = m_PcbFrame->m_CurrentScreen->ReturnPageSize().x;
			ymax = m_PcbFrame->m_CurrentScreen->ReturnPageSize().y;
		}
		else
		{
			xmin = - m_PcbFrame->m_CurrentScreen->ReturnPageSize().x/2;
			ymin = - m_PcbFrame->m_CurrentScreen->ReturnPageSize().y/2;
			xmax = m_PcbFrame->m_CurrentScreen->ReturnPageSize().x/2;
			ymax = m_PcbFrame->m_CurrentScreen->ReturnPageSize().y/2;
		}
	}

	m_BoundaryBox.SetX(xmin);
	m_BoundaryBox.SetY(ymin);
	m_BoundaryBox.SetWidth(xmax - xmin);
	m_BoundaryBox.SetHeight(ymax - ymin);

	return(Has_Items);
}

