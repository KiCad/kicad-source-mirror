	/****************************************************/
	/* class_module.cpp : fonctions de la classe MODULE */
	/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "wxstruct.h"
#include "common.h"
#include "trigo.h"

#ifdef PCBNEW
#include "pcbnew.h"
#include "autorout.h"
#include "drag.h"
#endif

#ifdef CVPCB
#include "cvpcb.h"

#endif

#include "protos.h"

#define MAX_WIDTH 10000		// Epaisseur (en 1/10000 ") max raisonnable des traits, textes...

	/******************************************/
	/* class EDGE_MODULE ( contour de module ) */
	/******************************************/

EDGE_MODULE::EDGE_MODULE(MODULE * parent): EDA_BaseLineStruct( parent, TYPEEDGEMODULE)
{
	m_Shape = S_SEGMENT;
	m_Angle = 0;
	m_Width = 120;
	m_PolyCount = 0;		// For polygons : number of points (> 2)
	m_PolyList = NULL;		// For polygons: coord list (1 point = 2 coord)
}

EDGE_MODULE::~EDGE_MODULE()
{
	if ( m_PolyList ) free (m_PolyList);
	m_PolyList = NULL;
	m_PolyCount = 0;
}

/********************************************/
void EDGE_MODULE:: Copy(EDGE_MODULE * source)		// copy structure
/********************************************/
{
	if (source == NULL) return;

	m_Start = source->m_Start;
	m_End = source->m_End;
	m_Shape = source->m_Shape;
	m_Start0 = source->m_Start0; // coord relatives a l'ancre du point de depart(Orient 0)
	m_End0 = source->m_End0;	// coord relatives a l'ancre du point de fin (Orient 0)
	m_Angle = source->m_Angle;		// pour les arcs de cercle: longueur de l'arc en 0,1 degres
	m_Layer = source->m_Layer;
	m_Width = source->m_Width;
	if ( m_PolyList ) free (m_PolyList);
	m_PolyCount = 0;
	m_PolyList = NULL;
	if ( source->m_PolyCount && source->m_PolyList )
	{
	int size;
		m_PolyCount = source->m_PolyCount;		// For polygons : number of points
		size = m_PolyCount * 2 * sizeof(int);	// For polygons: 1 point = 2 coord
		m_PolyList = (int*) MyMalloc( size );
		memcpy (m_PolyList, source->m_PolyList, size);
	}

}


void EDGE_MODULE::UnLink( void )
{
	/* Modification du chainage arriere */
	if( Pback )
		{
		if( Pback->m_StructType != TYPEMODULE)
			{
			Pback->Pnext = Pnext;
			}

		else /* Le chainage arriere pointe sur la structure "Pere" */
			{
			((MODULE*) Pback)->m_Drawings = Pnext;
			}
		}

	/* Modification du chainage avant */
	if( Pnext) Pnext->Pback = Pback;

	Pnext = Pback = NULL;
}


/***********************************/
void EDGE_MODULE::SetDrawCoord(void)
/***********************************/
{
MODULE * Module = (MODULE*) m_Parent;
	m_Start = m_Start0;
	m_End = m_End0;

	if ( Module )
		{
		RotatePoint( &m_Start.x, &m_Start.y, Module->m_Orient);
		RotatePoint( &m_End.x, &m_End.y, Module->m_Orient);
		m_Start.x += Module->m_Pos.x;
		m_Start.y += Module->m_Pos.y;
		m_End.x += Module->m_Pos.x;
		m_End.y += Module->m_Pos.y;
		}
}


/********************************************************************************/
void EDGE_MODULE::Draw(WinEDA_DrawPanel * panel, wxDC * DC,
						const wxPoint & offset, int draw_mode)
/********************************************************************************/

/* Affichage d'un segment contour de module :
	Entree : ox, oy = offset de trace
	draw_mode = mode de trace ( GR_OR, GR_XOR, GR_AND)
		Les contours sont de differents type:
		- Segment
		- Cercles
		- Arcs
*/
{
int ux0, uy0, dx, dy,rayon, StAngle, EndAngle;
int color , type_trace;
int zoom;
int typeaff;
PCB_SCREEN * screen;
WinEDA_BasePcbFrame * frame;
MODULE * Module = NULL;

	if ( m_Parent && (m_Parent->m_StructType == TYPEMODULE) )
		Module = (MODULE*) m_Parent;

	color = g_DesignSettings.m_LayerColor[m_Layer];
	if ( (color & ITEM_NOT_SHOW) != 0 ) return;

	if ( panel ) screen = (PCB_SCREEN *) panel->m_Parent->m_CurrentScreen;
	else screen = (PCB_SCREEN *) ActiveScreen;

	frame = ( WinEDA_BasePcbFrame * ) panel->m_Parent;

	zoom = screen->GetZoom();

	type_trace = m_Shape;
	ux0 = m_Start.x - offset.x; uy0 = m_Start.y - offset.y;
	dx = m_End.x - offset.x ;
	dy = m_End.y - offset.y ;

	GRSetDrawMode(DC, draw_mode);
	typeaff = frame->m_DisplayModEdge;
	if( m_Layer <= CMP_N )
		typeaff = frame->m_DisplayPcbTrackFill;
	if( (m_Width /zoom) < L_MIN_DESSIN ) typeaff = FILAIRE;

	switch (type_trace )
	{
		case S_SEGMENT:
			if( typeaff == FILAIRE)
				GRLine(&panel->m_ClipBox, DC,  ux0, uy0, dx, dy, color);
			else Affiche_1_Segment(panel, DC, ux0,uy0,dx,dy,m_Width,
									typeaff,color);
			break ;

		case S_CIRCLE:
			rayon = (int)hypot((double)(dx-ux0),(double)(dy-uy0) );
			if( typeaff == FILAIRE)
			{
				GRCircle(&panel->m_ClipBox, DC, ux0, uy0, rayon, color) ;
			}
			else
			{
				if(typeaff == FILLED )
				{
					GRCircle(&panel->m_ClipBox, DC, ux0, uy0, rayon, m_Width, color);
				}
				else
				{
					GRCircle(&panel->m_ClipBox, DC, ux0, uy0, rayon + (m_Width/2), color) ;
					GRCircle(&panel->m_ClipBox, DC, ux0, uy0, rayon - (m_Width/2), color) ;
				}
			}
			break;

		case S_ARC:
			rayon = (int)hypot((double)(dx-ux0),(double)(dy-uy0) );
			StAngle = (int)ArcTangente( dy-uy0, dx-ux0 );
			EndAngle = StAngle + m_Angle;
			if ( StAngle > EndAngle) EXCHG (StAngle, EndAngle);
			if( typeaff == FILAIRE)
			{
				GRArc(&panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle, rayon, color) ;
			}
			else if(typeaff == FILLED )
				{
					GRArc(&panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle, rayon,
								m_Width, color);
				}
			else
			{
				GRArc(&panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
						rayon + (m_Width/2), color) ;
				GRArc(&panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
						rayon - (m_Width/2), color) ;
			}
			break;

		case S_POLYGON:
		{
			// We must compute true coordinates from m_PolyList
			// which are relative to module position, orientation 0
			int ii, * source, * ptr, * ptr_base;
			ptr = ptr_base = (int*) MyMalloc( 2 * m_PolyCount * sizeof(int) );
			source = m_PolyList;
			for (ii = 0; ii < m_PolyCount; ii++ )
			{
				int x, y;
				x = *source; source++; y = *source; source++;
				if ( Module )
				{
					RotatePoint (&x, &y, Module->m_Orient);
					x += Module->m_Pos.x;
					y += Module->m_Pos.y;
				}
				x +=  m_Start0.x - offset.x;
				y +=  m_Start0.y - offset.y;
				*ptr = x; ptr++; *ptr = y; ptr++;
			}
			GRPoly(&panel->m_ClipBox, DC, m_PolyCount, ptr_base,
				TRUE, color, color);
			free ( ptr_base);
			break;
		}
	}
}


/*****************************************/
int EDGE_MODULE::WriteDescr( FILE * File )
/*****************************************/
/* Write one EDGE_MODULE description
	File must be opened.
*/
{
int  NbLigne = 0, ii, *ptr;
	
	switch(m_Shape )
	{
		case S_SEGMENT:
			fprintf(File,"DS %d %d %d %d %d %d\n",
						m_Start0.x, m_Start0.y,
						m_End0.x, m_End0.y,
						m_Width, m_Layer);
			NbLigne++;
			break;

		case S_CIRCLE:
			fprintf(File,"DC %d %d %d %d %d %d\n",
						m_Start0.x, m_Start0.y,
						m_End0.x, m_End0.y,
						m_Width, m_Layer);
			NbLigne++;
			break;

		case S_ARC:
			fprintf(File,"DA %d %d %d %d %d %d %d\n",
						m_Start0.x, m_Start0.y,
						m_End0.x, m_End0.y,
						m_Angle,
						m_Width, m_Layer);
			NbLigne++;
			break;

		case S_POLYGON:
			fprintf(File,"DP %d %d %d %d %d %d %d\n",
						m_Start0.x, m_Start0.y,
						m_End0.x, m_End0.y,
						m_PolyCount,
						m_Width, m_Layer);
			NbLigne++;
			for ( ii = 0, ptr = m_PolyList; ii < m_PolyCount; ii++ )
			{
			fprintf(File,"Dl %d %d\n",
						*ptr, *(ptr+1));
			NbLigne++; ptr += 2;
			}
			break;

		default:
			DisplayError(NULL, wxT("Type Edge Module inconnu") );
			break;
	}

	return(NbLigne);
}


/****************************************************************/
int EDGE_MODULE::ReadDescr(char * Line, FILE * File,
		int * LineNum)
/***************************************************************/
/* Read a description line like:
DS 2600 0 2600 -600 120 21
this description line is in Line
EDGE_MODULE type can be:
	- Circle,
	- Segment (line)
	- Arc
	- Polygon
	
*/
{
int ii, *ptr;
int error = 0;
char Buf[1024];
	
	
	switch ( Line[1] )
	{
		case 'S':
			m_Shape = S_SEGMENT;
			break;
		case 'C':
			m_Shape = S_CIRCLE;
			break;
		case 'A':
			m_Shape = S_ARC;
			break;
		case 'P':
			m_Shape = S_POLYGON;
			break;
		default:
			wxString msg;
			msg.Printf( wxT("Unknown EDGE_MODULE type <%s>") , Line);
			DisplayError(NULL, msg);
			error = 1;
			break;
	}

	switch ( m_Shape )
	{
		case  S_ARC:
			sscanf(Line+3,"%d %d %d %d %d %d %d",
				&m_Start0.x, &m_Start0.y,
				&m_End0.x, &m_End0.y,
				&m_Angle, &m_Width, &m_Layer);
			break;

		case S_SEGMENT:
		case S_CIRCLE:
			sscanf(Line+3,"%d %d %d %d %d %d",
				&m_Start0.x, &m_Start0.y,
				&m_End0.x, &m_End0.y,
				&m_Width, &m_Layer);
			break;

		case S_POLYGON:
			sscanf(Line+3,"%d %d %d %d %d %d %d",
				&m_Start0.x, &m_Start0.y,
				&m_End0.x, &m_End0.y,
				&m_PolyCount, &m_Width, &m_Layer);
			(*LineNum)++;
			m_PolyList = (int*) MyZMalloc( 2 * m_PolyCount * sizeof(int) );
			for ( ii = 0, ptr = m_PolyList; ii < m_PolyCount; ii++ )
			{
				if( GetLine(File, Buf, LineNum , sizeof(Buf) -1) != NULL )
				{
					if( strncmp(Buf, "Dl", 2) != 0 ) { error = 1; break;}
					sscanf(Buf+3,"%d %d\n", ptr, ptr+1);
					(*LineNum)++; ptr += 2;
				}
				else {
					error = 1; break;
				}
			}
			break;

		default:
			sscanf(Line+3,"%d %d %d %d %d %d",
				&m_Start0.x, &m_Start0.y,
				&m_End0.x, &m_End0.y,
				&m_Width, &m_Layer);
			break;
	}

	// Controle d'epaisseur raisonnable:
	if( m_Width <= 1 ) m_Width = 1;
	if( m_Width > MAX_WIDTH ) m_Width = MAX_WIDTH;
		
	return error;

}

