/******************************************************************/
/* fonctions membres des classes TRACK et derivees (voir struct.h */
/******************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#ifdef CVPCB
#include "cvpcb.h"
#endif



	/**************************************/
	/* Classes pour Pistes, Vias et Zones */
	/**************************************/

/* Constructeur des classes type pistes, vias et zones */

TRACK::TRACK(EDA_BaseStruct * StructFather, DrawStructureType idtype):
		SEGDRAW_Struct( StructFather, idtype)
{
	m_Shape = S_SEGMENT;
	start = end = NULL;
	m_NetCode = 0;
	m_Sous_Netcode = 0;
}

SEGZONE::SEGZONE(EDA_BaseStruct * StructFather):
		TRACK( StructFather, TYPEZONE)
{
}

SEGVIA::SEGVIA(EDA_BaseStruct * StructFather):
		TRACK( StructFather, TYPEVIA)
{
}

/******************************************/
bool SEGVIA::IsViaOnLayer(int layer_number )
/******************************************/
/* Retoune TRUE si Via sur layer layer_number
*/
{
int via_type = Shape();

	if( via_type == VIA_NORMALE )
	{
		if ( layer_number <= LAYER_CMP_N ) return TRUE;
		else return FALSE;
	}

	// VIA_BORGNE ou  VIA_ENTERREE:

int bottom_layer, top_layer;
	ReturnLayerPair(& top_layer, & bottom_layer);
	if ( (bottom_layer <= layer_number) && (top_layer >= layer_number) )
		return TRUE;
	else return FALSE;
}


/*********************************************************/
void SEGVIA::SetLayerPair(int top_layer, int bottom_layer)
/*********************************************************/
/* Met a jour .m_Layer pour une via:
	m_Layer code les 2 couches limitant la via
*/
{
int via_type = m_Shape & 255;

	if( via_type == VIA_NORMALE )
	{
		top_layer = LAYER_CMP_N; bottom_layer = LAYER_CUIVRE_N;
	}

	if ( bottom_layer > top_layer ) EXCHG (bottom_layer, top_layer);
	m_Layer = (top_layer & 15) + ( (bottom_layer & 15) << 4 );
}

/***************************************************************/
void SEGVIA::ReturnLayerPair(int * top_layer, int * bottom_layer)
/***************************************************************/
/* Retourne les 2 couches limitant la via
	les pointeurs top_layer et bottom_layer peuvent etre NULLs
*/
{
int b_layer = (m_Layer >> 4) & 15;
int t_layer = m_Layer & 15;

	if ( b_layer > t_layer ) EXCHG (b_layer, t_layer);
	if ( top_layer ) * top_layer = t_layer;
	if ( bottom_layer ) * bottom_layer = b_layer;
}


/************************/
TRACK * TRACK::Next(void)
/************************/
{
	return (TRACK *) Pnext;
}



/* supprime du chainage la structure Struct
  les structures arrieres et avant sont chainees directement
 */
void TRACK::UnLink( void )
{
	/* Modification du chainage arriere */
	if( Pback )
		{
		if( Pback->m_StructType != TYPEPCB)
			{
			Pback->Pnext = Pnext;
			}

		else /* Le chainage arriere pointe sur la structure "Pere" */
			{
			if ( GetState(DELETED) )	// A REVOIR car Pback = NULL si place en undelete
				{
				if( g_UnDeleteStack ) g_UnDeleteStack[g_UnDeleteStackPtr-1] = Pnext;
				}
			else
				{
				if (m_StructType == TYPEZONE)
					{
					((BOARD*)Pback)->m_Zone = (TRACK*)Pnext;
					}
				else
					{
					((BOARD*)Pback)->m_Track = (TRACK*)Pnext;
					}
				}
			}
		}

	/* Modification du chainage avant */
	if( Pnext) Pnext->Pback = Pback;

	Pnext = Pback = NULL;
}

/************************************************************/
void TRACK::Insert(BOARD * Pcb, EDA_BaseStruct * InsertPoint)
/************************************************************/
/* Ajoute un element ou une liste  a une liste de base
	Si Insertpoint == NULL: insertion en debut de
		liste Pcb->Track ou Pcb->Zone
	Insertion a la suite de InsertPoint
	Si InsertPoint == NULL, insertion en tete de liste
*/
{
TRACK* track, *NextS;
	/* Insertion du debut de la chaine a greffer */
	if (InsertPoint == NULL)
		{
		Pback = Pcb;
		if (m_StructType == TYPEZONE)
			{
			NextS = Pcb->m_Zone; Pcb->m_Zone = this;
			}
		else
			{
			NextS = Pcb->m_Track; Pcb->m_Track = this;
			}
		}

	else
		{
		NextS = (TRACK*)InsertPoint->Pnext;
		Pback = InsertPoint;
		InsertPoint->Pnext = this;
		}

	/* Chainage de la fin de la liste a greffer */
	track = this;
	while ( track->Pnext ) track = (TRACK*) track->Pnext;
	/* Track pointe la fin de la chaine a greffer */
	track->Pnext = NextS;
	if ( NextS ) NextS->Pback = track;
}


/***********************************************/
TRACK * TRACK::GetBestInsertPoint( BOARD * Pcb )
/***********************************************/
/* Recherche du meilleur point d'insertion pour le nouveau segment de piste
   Retourne
		un pointeur sur le segment de piste APRES lequel l'insertion
			 doit se faire ( dernier segment du net d'apartenance )
		NULL si pas de piste ( liste vide );
*/
{
TRACK * track, * NextTrack;

	if( m_StructType == TYPEZONE ) track = Pcb->m_Zone;
	else track = Pcb->m_Track;

	/* Traitement du debut de liste */
	if ( track == NULL ) return(NULL);	/* pas de piste ! */
	if ( m_NetCode < track->m_NetCode ) /* insertion en tete de liste */
		return(NULL);

	while( (NextTrack = (TRACK*)track->Pnext) != NULL )
		{
		if ( NextTrack->m_NetCode > this->m_NetCode ) break;
		track = NextTrack;
		}
	return ( track);
}

/* Recherche du debut du net
   ( les elements sont classes par net_code croissant )
	la recherche se fait a partir de this
	si net_code == -1 le netcode de this sera utilise
	Retourne un pointeur sur le debut du net, ou NULL si net non trouve
*/
TRACK * TRACK::GetStartNetCode(int NetCode )
{
TRACK * Track = this;
int ii = 0;

	if( NetCode == -1 ) NetCode = m_NetCode;

	while( Track != NULL)
		{
		if ( Track->m_NetCode > NetCode ) break;
		if ( Track->m_NetCode == NetCode )
			{
			ii++; break;
			}
 		Track = (TRACK*) Track->Pnext;
		}
	if ( ii ) return(Track);
	else return (NULL);
}

/* Recherche de la fin du net
	Retourne un pointeur sur la fin du net, ou NULL si net non trouve
*/
TRACK * TRACK::GetEndNetCode(int NetCode)
{
TRACK * NextS, * Track = this;
int ii = 0;

	if( Track == NULL ) return(NULL);

	if( NetCode == -1 ) NetCode = m_NetCode;

	while( Track != NULL)
		{
		NextS = (TRACK*)Track->Pnext;
		if(Track->m_NetCode == NetCode) ii++;
		if ( NextS == NULL ) break;
		if ( NextS->m_NetCode > NetCode) break;
		Track = NextS;
		}
	if ( ii ) return(Track);
	else return (NULL);
}


/**********************************/
TRACK * TRACK:: Copy( int NbSegm  )
/**********************************/
/* Copie d'un Element ou d'une chaine de n elements
	Retourne un pointeur sur le nouvel element ou le debut de la
	nouvelle chaine
*/
{
TRACK * NewTrack, * FirstTrack, *OldTrack, * Source = this;
int ii;

	FirstTrack = NewTrack = new TRACK(NULL);
	*NewTrack = * Source;

	/* correction du chainage */
	NewTrack->Pback = NewTrack->Pnext = NULL;

	/* reset des pointeurs auxiliaires */
	NewTrack->start = NewTrack->end = NULL;

	if( NbSegm <=1 ) return (FirstTrack);

	for( ii = 1; ii < NbSegm; ii++ )
		{
		Source = (TRACK*) Source->Pnext;
		if( Source == NULL ) break;
		OldTrack = NewTrack;
		NewTrack = new TRACK(m_Parent);
		if ( NewTrack == NULL ) break;
		NewTrack->m_StructType = Source->m_StructType;
		NewTrack->m_Shape = Source->m_Shape;
		NewTrack->m_NetCode = Source->m_NetCode;
		NewTrack->m_Flags = Source->m_Flags;
		NewTrack->m_TimeStamp = Source->m_TimeStamp;
		NewTrack->SetStatus(Source->ReturnStatus() );
		NewTrack->m_Layer = Source->m_Layer;
		NewTrack->m_Start = Source->m_Start;
		NewTrack->m_End = Source->m_End;
		NewTrack->m_Width = Source->m_Width;

		NewTrack->Insert(NULL, OldTrack);
		}

	 return (FirstTrack);
}

/********************************************/
bool TRACK::WriteTrackDescr(FILE * File)
/********************************************/
{
int type;

	type = 0;
	if( m_StructType == TYPEVIA ) type = 1;

	if( GetState(DELETED) ) return FALSE;

	fprintf( File,"Po %d %d %d %d %d %d\n",m_Shape,
			m_Start.x, m_Start.y, m_End.x, m_End.y, m_Width );

	fprintf( File,"De %d %d %d %lX %X\n",
			m_Layer, type ,m_NetCode,
			m_TimeStamp, ReturnStatus());
	return TRUE;
}

/**********************************************************************/
void TRACK::Draw(WinEDA_DrawPanel * panel, wxDC * DC, int draw_mode)
/*********************************************************************/
 /* routine de trace de 1 segment de piste.
Parametres :
	draw_mode = mode ( GR_XOR, GR_OR..)
*/
{
int l_piste;
int color;
int zoom;
int rayon;
int curr_layer = ((PCB_SCREEN*)panel->GetScreen())->m_Active_Layer;

	if(m_StructType == TYPEZONE && (! DisplayOpt.DisplayZones) )
		return;

	GRSetDrawMode(DC, draw_mode);

	if ( m_StructType == TYPEVIA ) /* VIA rencontree */
		color = g_DesignSettings.m_ViaColor[m_Shape];
	else color = g_DesignSettings.m_LayerColor[m_Layer];

	if( (color & (ITEM_NOT_SHOW | HIGHT_LIGHT_FLAG)) == ITEM_NOT_SHOW) return ;

	if ( DisplayOpt.ContrastModeDisplay )
	{
		if ( m_StructType == TYPEVIA )
		{
			if ( ! ((SEGVIA*)this)->IsViaOnLayer(curr_layer) )
			{
				color &= ~MASKCOLOR;
				color |= DARKDARKGRAY;
			}
		}
		else if ( m_Layer != curr_layer)
		{
			color &= ~MASKCOLOR;
			color |= DARKDARKGRAY;
		}
	}

	if( draw_mode & GR_SURBRILL)
		{
		if( draw_mode & GR_AND)	color &= ~HIGHT_LIGHT_FLAG;
		else color |= HIGHT_LIGHT_FLAG;
		}
	if ( color & HIGHT_LIGHT_FLAG)
		color = ColorRefs[color & MASKCOLOR].m_LightColor;

	zoom = panel->GetZoom();

	l_piste = m_Width >> 1;

	if ( m_StructType == TYPEVIA ) /* VIA rencontree */
		{
		rayon = l_piste; if( rayon < zoom ) rayon = zoom;
		GRCircle(&panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon, color) ;
		if ( rayon > (4*zoom) )
			{
			GRCircle(&panel->m_ClipBox, DC, m_Start.x, m_Start.y,
						rayon-(2*zoom) , color);

			if(DisplayOpt.DisplayTrackIsol)
				 GRCircle(&panel->m_ClipBox, DC, m_Start.x, m_Start.y,
						rayon + g_DesignSettings.m_TrackClearence, color);
			}
		return;
		}

	if(m_Shape == S_CIRCLE)
		{
		rayon = (int)hypot((double)(m_End.x - m_Start.x),
							(double)(m_End.y - m_Start.y) );
		if ( (l_piste/zoom) < L_MIN_DESSIN)
			{
			GRCircle(&panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon , color) ;
			}

		else
			{
			if(l_piste <= zoom) /* trace simplifie si l_piste/zoom <= 1 */
				{
				GRCircle(&panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon, color);
				}
			else if( ( ! DisplayOpt.DisplayPcbTrackFill) || GetState(FORCE_SKETCH))
				{
				GRCircle(&panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon-l_piste, color);
				GRCircle(&panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon+l_piste, color);
				}
			else
				{
				GRCircle(&panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon,
						m_Width, color);
				}
			}
		return;
		}

	if ( (l_piste/zoom) < L_MIN_DESSIN)
		{
		GRLine(&panel->m_ClipBox, DC, m_Start.x, m_Start.y,
					m_End.x, m_End.y, color);
		return;
		}

	if( (! DisplayOpt.DisplayPcbTrackFill) || GetState(FORCE_SKETCH) )
		{
		GRCSegm(&panel->m_ClipBox, DC, m_Start.x, m_Start.y,
						m_End.x, m_End.y, m_Width, color) ;
		}
	else
		{
		GRFillCSegm(&panel->m_ClipBox, DC, m_Start.x, m_Start.y,
				m_End.x, m_End.y, m_Width, color) ;
		}

	/* Trace de l'isolation (pour segments type CUIVRE et TRACK uniquement */
	if( (DisplayOpt.DisplayTrackIsol) && (m_Layer <= CMP_N )
		&& ( m_StructType == TYPETRACK) )
		{
		GRCSegm(&panel->m_ClipBox, DC, m_Start.x, m_Start.y,
				m_End.x, m_End.y,
				m_Width + (g_DesignSettings.m_TrackClearence*2), color) ;
		}
}

