		/*********************************************************/
		/* Routines de trace: fonction communes aux diff formats */
		/*********************************************************/

/* Fichier PLOT_RTN.CPP*/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "plot_common.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "trigo.h"

#include "grfonte.h"

#include "protos.h"


/* Fonctions locales */
static void Plot_Edges_Modules(BOARD * pcb, int format_plot, int masque_layer);
static void PlotTextModule(TEXTE_MODULE * pt_texte);



/**********************************************************/
void WinEDA_BasePcbFrame::Plot_Serigraphie(int format_plot,
			FILE * File, int masque_layer)
/***********************************************************/

/* Genere le trace des couches type serigraphie, en format HPGL ou GERBER*/
{
wxPoint pos, shape_pos;
wxSize size;
bool trace_val, trace_ref;
MODULE* Module;
D_PAD * pt_pad;
TEXTE_MODULE * pt_texte;
EDA_BaseStruct * PtStruct;
wxString msg;
	
	/* Trace du contour du PCB et des Elements du  type Drawings Pcb */
	PtStruct = m_Pcb->m_Drawings;
	for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
		{
		switch( PtStruct->m_StructType )
			{
			case TYPEDRAWSEGMENT:
				PlotDrawSegment( (DRAWSEGMENT*) PtStruct, format_plot,
												EDGE_LAYER | masque_layer );
				break;

			case TYPETEXTE:
				PlotTextePcb((TEXTE_PCB*) PtStruct,format_plot,
												EDGE_LAYER | masque_layer);
				break;

			case TYPECOTATION:
				PlotCotation((COTATION*) PtStruct, format_plot,
												EDGE_LAYER | masque_layer);
				break;

			case TYPEMIRE:
				PlotMirePcb((MIREPCB*) PtStruct, format_plot,
												EDGE_LAYER | masque_layer);
				break;

			case TYPEMARQUEUR:
				break;

			default:
				DisplayError(this, wxT("Plot_Serigraphie() error: unexpected m_StructType"));
				break;
			}
		}

	/* trace des contours des MODULES : */
	Plot_Edges_Modules(m_Pcb, format_plot, masque_layer);

	/* Trace des MODULES : PADS */
	if(PlotPadsOnSilkLayer || Plot_Pads_All_Layers)
		{
		nb_items = 0 ;
		Affiche_1_Parametre(this, 56, wxT("Pads"),wxEmptyString,GREEN) ;
		Module = m_Pcb->m_Modules;
		for( ; Module != NULL ; Module = (MODULE *) Module->Pnext )
			{
			pt_pad = (D_PAD *) Module->m_Pads;
			for( ; pt_pad != NULL; pt_pad = (D_PAD *) pt_pad->Pnext)
				{
				/* Tst si layer OK */
				if( (pt_pad->m_Masque_Layer & masque_layer) == 0)
					{
					if( ! Plot_Pads_All_Layers ) continue;
					}
                shape_pos = pt_pad->ReturnShapePos();
				pos = shape_pos; size = pt_pad->m_Size;

				switch( pt_pad->m_PadShape & 0x7F )
					{
					case CIRCLE :
						switch ( format_plot )
							{
							case PLOT_FORMAT_GERBER:
								PlotCircle(PLOT_FORMAT_GERBER,
									g_PlotLine_Width ,pos, size.x / 2);
								break;

							case PLOT_FORMAT_HPGL:
								trace_1_pastille_RONDE_HPGL(pos, size.x ,FILAIRE);
								break;

							case PLOT_FORMAT_POST:
								trace_1_pastille_RONDE_POST(pos, size.x ,FILAIRE);
								break;
							}
						break ;

					case OVALE :
						switch ( format_plot )
							{
							case PLOT_FORMAT_GERBER:
								trace_1_contour_GERBER(pos, size, wxSize(0,0),
													g_PlotLine_Width,
													pt_pad->m_Orient);
								break;

							case PLOT_FORMAT_HPGL:
								trace_1_pastille_OVALE_HPGL(pos,size,
											pt_pad->m_Orient,FILAIRE) ;
								break;

							case PLOT_FORMAT_POST:
								trace_1_pastille_OVALE_POST(pos,size,
											pt_pad->m_Orient,FILAIRE) ;
								break;
							}
						break ;

					case TRAPEZE :
						{
						wxSize delta;
						delta = pt_pad->m_DeltaSize;

						switch ( format_plot )
							{
							case PLOT_FORMAT_GERBER:
								trace_1_contour_GERBER(pos, size, delta,
													g_PlotLine_Width,
													(int)pt_pad->m_Orient);
								break;

							case PLOT_FORMAT_HPGL:
								  trace_1_pad_TRAPEZE_HPGL(pos,size,
										delta, (int)pt_pad->m_Orient,
										FILAIRE) ;
								break;

							case PLOT_FORMAT_POST:
								  trace_1_pad_TRAPEZE_POST(pos,size,
										delta, (int)pt_pad->m_Orient,
										FILAIRE) ;
								break;
							}
						break ;
						}

					case RECT :
					default:
						switch ( format_plot )
							{
							case PLOT_FORMAT_GERBER:
								trace_1_contour_GERBER(pos, size, wxSize(0,0),
													g_PlotLine_Width,
													(int)pt_pad->m_Orient);
								break;

							case PLOT_FORMAT_HPGL:
								  PlotRectangularPad_HPGL(pos,size, pt_pad->m_Orient,FILAIRE) ;
								break;

							case PLOT_FORMAT_POST:
								  trace_1_pad_rectangulaire_POST(pos,size,
									(int)pt_pad->m_Orient,FILAIRE) ;
								break;
							}
						break ;
					}
				nb_items++; msg.Printf( wxT("%d"),nb_items) ;
				Affiche_1_Parametre(this, 56,wxEmptyString,msg,GREEN) ;
				}
			}
		} /* Fin Sequence de trace des Pads */

	/* Trace Textes MODULES */
	nb_items = 0; Affiche_1_Parametre(this, 64, wxT("TxtMod"),wxEmptyString,LIGHTBLUE) ;

	Module = m_Pcb->m_Modules;
	for( ; Module != NULL ; Module = (MODULE *) Module->Pnext )
	{
		/* Analyse des autorisations de trace pour les textes VALEUR et REF */
		trace_val = Sel_Texte_Valeur;
		trace_ref = Sel_Texte_Reference; // les 2 autorisations de tracer sont donnees

		if( (g_TabOneLayerMask[Module->m_Reference->m_Layer] & masque_layer) == 0)
			trace_ref = FALSE;
		if(Module->m_Reference->m_NoShow && !Sel_Texte_Invisible)
			trace_ref = FALSE;

		if( (g_TabOneLayerMask[Module->m_Value->m_Layer] & masque_layer) == 0)
			trace_val = FALSE;
		if(Module->m_Value->m_NoShow && !Sel_Texte_Invisible)
			trace_val = FALSE;

		/* Trace effectif des textes */
		if( trace_ref )
		{
			PlotTextModule(Module->m_Reference);
			nb_items++ ;
			msg.Printf( wxT("%d"),nb_items) ;
			Affiche_1_Parametre(this, 64,wxEmptyString,msg,LIGHTBLUE) ;
		}

		if( trace_val )
		{
			PlotTextModule(Module->m_Value);
			nb_items++ ;
			msg.Printf( wxT("%d"),nb_items) ;
			Affiche_1_Parametre(this, 64,wxEmptyString,msg,LIGHTBLUE) ;
		}

		pt_texte = (TEXTE_MODULE *) Module->m_Drawings;
		for( ;pt_texte != NULL ; pt_texte = (TEXTE_MODULE*)pt_texte->Pnext)
		{
			if(pt_texte->m_StructType != TYPETEXTEMODULE ) continue;

			if( !Sel_Texte_Divers ) continue;
			if( (pt_texte->m_NoShow) && !Sel_Texte_Invisible )
				continue;
			if( (g_TabOneLayerMask[pt_texte->m_Layer] & masque_layer) == 0)
				continue;
			PlotTextModule(pt_texte);
			nb_items++ ;
			msg.Printf( wxT("%d"),nb_items) ;
			Affiche_1_Parametre(this, 64,wxEmptyString,msg,LIGHTBLUE) ;
		}
	}
}

/**************************************************/
static void PlotTextModule(TEXTE_MODULE * pt_texte)
/**************************************************/
{
wxSize size;
wxPoint pos;
int orient, epaisseur, no_miroir;

	/* calcul des parametres du texte :*/
	size = pt_texte->m_Size;
	pos = pt_texte->m_Pos;

	orient = pt_texte->GetDrawRotation();

	no_miroir = pt_texte->m_Miroir  & 1;
	epaisseur = pt_texte->m_Width;
	if ( Plot_Mode == FILAIRE ) epaisseur = g_PlotLine_Width;

	if ( no_miroir == 0 ) size.y = -size.y;		// Text is mirrored

	Plot_1_texte(format_plot, pt_texte->m_Text,
					orient, epaisseur,
					pos.x, pos.y, size.x, size.y);
}



/*******************************************************************************/
void PlotCotation( COTATION * Cotation, int format_plot,int masque_layer)
/*******************************************************************************/
{
DRAWSEGMENT *DrawTmp;

	if( (g_TabOneLayerMask[Cotation->m_Layer] & masque_layer) == 0) return;

	DrawTmp = new DRAWSEGMENT(NULL);

	masque_layer |= EDGE_LAYER;
	DrawTmp->m_Width = Cotation->m_Width;
	DrawTmp->m_Layer = Cotation->m_Layer;

	PlotTextePcb( Cotation->m_Text,format_plot, masque_layer);

	DrawTmp->m_Start.x = Cotation->Barre_ox; DrawTmp->m_Start.y =Cotation->Barre_oy;
	DrawTmp->m_End.x = Cotation->Barre_fx; DrawTmp->m_End.y =Cotation->Barre_fy;
	PlotDrawSegment( DrawTmp, format_plot, masque_layer);

	DrawTmp->m_Start.x = Cotation->TraitG_ox; DrawTmp->m_Start.y =Cotation->TraitG_oy;
	DrawTmp->m_End.x = Cotation->TraitG_fx; DrawTmp->m_End.y =Cotation->TraitG_fy;
	PlotDrawSegment( DrawTmp, format_plot, masque_layer);

	DrawTmp->m_Start.x = Cotation->TraitD_ox; DrawTmp->m_Start.y =Cotation->TraitD_oy;
	DrawTmp->m_End.x = Cotation->TraitD_fx; DrawTmp->m_End.y =Cotation->TraitD_fy;
	PlotDrawSegment( DrawTmp, format_plot, masque_layer);

	DrawTmp->m_Start.x = Cotation->FlecheD1_ox; DrawTmp->m_Start.y =Cotation->FlecheD1_oy;
	DrawTmp->m_End.x = Cotation->FlecheD1_fx; DrawTmp->m_End.y =Cotation->FlecheD1_fy;
	PlotDrawSegment( DrawTmp, format_plot, masque_layer);

	DrawTmp->m_Start.x = Cotation->FlecheD2_ox; DrawTmp->m_Start.y = Cotation->FlecheD2_oy;
	DrawTmp->m_End.x = Cotation->FlecheD2_fx; DrawTmp->m_End.y = Cotation->FlecheD2_fy;
	PlotDrawSegment( DrawTmp, format_plot, masque_layer);

	DrawTmp->m_Start.x = Cotation->FlecheG1_ox; DrawTmp->m_Start.y =Cotation->FlecheG1_oy;
	DrawTmp->m_End.x = Cotation->FlecheG1_fx; DrawTmp->m_End.y =Cotation->FlecheG1_fy;
	PlotDrawSegment( DrawTmp, format_plot, masque_layer);

	DrawTmp->m_Start.x = Cotation->FlecheG2_ox; DrawTmp->m_Start.y =Cotation->FlecheG2_oy;
	DrawTmp->m_End.x = Cotation->FlecheG2_fx; DrawTmp->m_End.y =Cotation->FlecheG2_fy;
	PlotDrawSegment( DrawTmp, format_plot, masque_layer);

	delete DrawTmp;
}


/*****************************************************************/
void PlotMirePcb( MIREPCB* Mire, int format_plot,int masque_layer)
/*****************************************************************/
{
DRAWSEGMENT *DrawTmp;
int dx1,dx2, dy1, dy2, rayon;

	if( (g_TabOneLayerMask[Mire->m_Layer] & masque_layer) == 0) return;

	DrawTmp = new DRAWSEGMENT(NULL);

	masque_layer |= EDGE_LAYER;
	DrawTmp->m_Width = Mire->m_Width;
	DrawTmp->m_Layer = Mire->m_Layer;

	DrawTmp->m_Start.x = Mire->m_Pos.x; DrawTmp->m_Start.y = Mire->m_Pos.y;
	DrawTmp->m_End.x = DrawTmp->m_Start.x + (Mire->m_Size / 4);
	DrawTmp->m_End.y = DrawTmp->m_Start.y;
	DrawTmp->m_Shape = S_CIRCLE;
	PlotDrawSegment( DrawTmp, format_plot, masque_layer);

	DrawTmp->m_Shape = S_SEGMENT;
	/* Trace des 2 traits */
	rayon = Mire->m_Size/2;
	dx1 = rayon, dy1 = 0; dx2 = 0, dy2 = rayon;

	if( Mire->m_Shape) /* Forme X */
		{
		dx1 = dy1 = (rayon * 7)/5;  dx2 = dx1; dy2 = -dy1;
		}

	DrawTmp->m_Start.x = Mire->m_Pos.x - dx1; DrawTmp->m_Start.y = Mire->m_Pos.y - dy1;
	DrawTmp->m_End.x = Mire->m_Pos.x + dx1; DrawTmp->m_End.y = Mire->m_Pos.y + dy1;
	PlotDrawSegment( DrawTmp, format_plot, masque_layer);

	DrawTmp->m_Start.x = Mire->m_Pos.x - dx2; DrawTmp->m_Start.y = Mire->m_Pos.y - dy2;
	DrawTmp->m_End.x = Mire->m_Pos.x + dx2; DrawTmp->m_End.y = Mire->m_Pos.y + dy2;
	PlotDrawSegment( DrawTmp, format_plot, masque_layer);

	delete DrawTmp;
}


/**********************************************************************/
void Plot_Edges_Modules(BOARD * pcb, int format_plot, int masque_layer)
/**********************************************************************/
/* Trace les contours des modules */
{
int nb_items;				/* Pour affichage activite: nbr modules traites */
MODULE * Module;
EDGE_MODULE * PtEdge;
wxString msg;
	
	nb_items = 0 ;
	Module = pcb->m_Modules;
	for( ; Module != NULL ; Module = (MODULE *) Module->Pnext )
	{
		PtEdge = (EDGE_MODULE*) Module->m_Drawings;
		for ( ;PtEdge != NULL; PtEdge = (EDGE_MODULE*)PtEdge->Pnext)
		{
			if(PtEdge->m_StructType != TYPEEDGEMODULE) continue;
			if( (g_TabOneLayerMask[PtEdge->m_Layer] & masque_layer) == 0) continue;
			Plot_1_EdgeModule(format_plot, PtEdge);
		}
		/* Affichage du nombre de modules traites */
		nb_items++ ;
		msg.Printf( wxT("%d"),nb_items) ;
	}
}

/**************************************************************/
void Plot_1_EdgeModule(int format_plot, EDGE_MODULE * PtEdge)
/**************************************************************/
/* Trace les contours des modules */
{
int type_trace ;			/* forme a tracer (segment, cercle) */
int epaisseur;				/* epaisseur des segments */
int rayon;					/* rayon des cercles a tracer */
int StAngle, EndAngle;
wxPoint pos, end;	/* Coord des segments a tracer */

	if(PtEdge->m_StructType != TYPEEDGEMODULE) return;
	type_trace = PtEdge->m_Shape;
	epaisseur = PtEdge->m_Width;
	if ( Plot_Mode == FILAIRE ) epaisseur = g_PlotLine_Width;

	pos = PtEdge->m_Start;
	end = PtEdge->m_End;

	switch(type_trace )
	{
		case S_SEGMENT:	/* segment simple */
			switch ( format_plot )
			{
				case PLOT_FORMAT_GERBER:
					PlotGERBERLine(pos,end, epaisseur) ;
					break;

				case PLOT_FORMAT_HPGL:
					trace_1_segment_HPGL(pos.x,pos.y,end.x,end.y, epaisseur) ;
					break;

				case PLOT_FORMAT_POST:
					PlotFilledSegmentPS(pos, end, epaisseur) ;
					break;
			}
			break;  /* Fin trace segment simple */

		case S_CIRCLE:
			rayon = (int)hypot((double)(end.x - pos.x),(double)(end.y - pos.y) );
			PlotCircle(format_plot,epaisseur, pos, rayon) ;
			break;

		case S_ARC:
			rayon = (int)hypot((double)(end.x - pos.x),(double)(end.y - pos.y) );
			StAngle = ArcTangente(end.y - pos.y, end.x - pos.x);
			EndAngle = StAngle + PtEdge->m_Angle;
			if ( StAngle > EndAngle) EXCHG (StAngle, EndAngle);
			PlotArc(format_plot, pos, StAngle, EndAngle, rayon, epaisseur);
			break;

		case S_POLYGON:
		{
			// We must compute true coordinates from m_PolyList
			// which are relative to module position, orientation 0
			int ii, * source, * ptr, * ptr_base;
			MODULE * Module = NULL;
			if ( PtEdge->m_Parent && (PtEdge->m_Parent->m_StructType == TYPEMODULE) )
				Module = (MODULE*) PtEdge->m_Parent;
			ptr = ptr_base = (int*) MyMalloc( 2 * PtEdge->m_PolyCount * sizeof(int) );
			source = PtEdge->m_PolyList;
			for (ii = 0; ii < PtEdge->m_PolyCount; ii++ )
			{
				int x, y;
				x = *source; source++; y = *source; source++;
				if ( Module )
				{
					RotatePoint (&x, &y, Module->m_Orient);
					x += Module->m_Pos.x;
					y += Module->m_Pos.y;
				}
				x +=  PtEdge->m_Start0.x;
				y +=  PtEdge->m_Start0.y;
				*ptr = x; ptr++; *ptr = y; ptr++;
			}
			PlotPolygon(format_plot, TRUE, PtEdge->m_PolyCount, ptr_base);
			free ( ptr_base);
			break;
		}

	}
}



/****************************************************************************/
void PlotTextePcb(TEXTE_PCB * pt_texte, int format_plot,int masque_layer)
/****************************************************************************/
/* Trace 1 Texte type PCB , c.a.d autre que les textes sur modules */
{
int no_miroir, orient, epaisseur;
wxPoint pos;
wxSize size;

	if( pt_texte->m_Text.IsEmpty() ) return;
	if( (g_TabOneLayerMask[pt_texte->m_Layer] & masque_layer) == 0 ) return ;

	/* calcul des parametres du texte :*/
	size = pt_texte->m_Size;
	pos = pt_texte->m_Pos;
	orient = pt_texte->m_Orient;
	no_miroir = pt_texte->m_Miroir & 1;
	epaisseur = pt_texte->m_Width;

	if ( no_miroir == FALSE) size.x = - size.x;

	Plot_1_texte(format_plot, pt_texte->m_Text,orient,
							epaisseur, pos.x, pos.y, size.x, size.y);
}

/**********************************************************************/
void Plot_1_texte(int format_plot,const wxString & Text, int angle,
					int epaisseur, int cX,int cY,int size_h,int size_v,
					bool centreX, bool centreY)
/***********************************************************************/
/*
 Trace de 1 texte:
	ptr = pointeur sur le texte
	angle = angle d'orientation, dependant aussi du mode de trace (miroir..)
	cX, cY = position du centre du texte
	size_h , size_v = dimensions ( algebriques );
*/
{
int kk = 0, k1 , k2, end ;
int espacement;
char f_cod , plume ;
const SH_CODE * ptcar;
int ox,oy,fx,fy;			/* Coord de debut et fin des segments a tracer */
int sx, sy;					/* coord du debut du caractere courant */
int nbcodes = Text.Len();
	
	espacement = ((10 * size_h) / 9) + ( (size_h >= 0 ) ? epaisseur : - epaisseur);

	/* calcul de la position du debut du texte */
	if ( centreX ) sx = cX - ((espacement * nbcodes) / 2 ) + (espacement /9);
	else sx = cX;
	if ( centreY ) sy = cY + (size_v / 2);
	else sy = cY;

	/* trace du texte */
	for( ; kk < nbcodes; kk++)
	{
		int code = Text.GetChar(kk) & 0xFF;
		ptcar = graphic_fonte_shape[code] ; /* ptcar pointe la description
											du caractere a dessiner */

		plume = 'U'; ox = sx; oy = sy;
		RotatePoint(&ox, &oy, cX, cY, angle);
		fx = ox; fy = oy;

		for(end = 0; end == 0; ptcar++)
		{
			f_cod = * ptcar;	/* get code n de la forme selectionnee */
			switch(f_cod)
			{
				case 'X' : end = 1;/* fin du caractere */
					break;

				case 'U' :
				case 'D' : plume = f_cod ; break ;

				default  :
					k1 = f_cod; // Coord X
					k1 = (k1 * size_v) / 9;
					ptcar++ ;
					k2 = *ptcar;	// Coord Y
					k2 = (k2 * size_h) / 8 ;

					fx = k2 + sx ; fy = -k1 + sy ;
					RotatePoint(&fx, &fy, cX, cY, angle);

					/* Trace du segment */
					if ( plume == 'D' )
						{
						switch ( format_plot )
				 			{
							case PLOT_FORMAT_GERBER:
				 				PlotGERBERLine(wxPoint(ox,oy),wxPoint(fx,fy), epaisseur) ;
								break;

							case PLOT_FORMAT_HPGL:
								trace_1_segment_HPGL(ox,oy,fx,fy, epaisseur) ;
								break;

							case PLOT_FORMAT_POST:
								PlotFilledSegmentPS(wxPoint(ox,oy), wxPoint(fx,fy), epaisseur) ;
								break;

						}
					}
					ox = fx; oy = fy;
			} /* fin switch decodade matrice de forme */

		} /* end boucle for = end trace de 1 caractere */

		sx += espacement;
	} /* end trace du texte */
}


/***********************************/
void Affiche_erreur( int nb_err)
/***********************************/
/* Affiche le nombre d'erreurs commises ( segments traces avec plume trop grosse
 ou autres */
{
//	sprintf(msg,"%d",nb_err) ;
//	Affiche_1_Parametre(this, 30,"Err",msg,GREEN) ;
}


/******************************************************************************/
void PlotDrawSegment(DRAWSEGMENT * pt_segm, int Format, int masque_layer )
/******************************************************************************/
/* Trace un element du type DRAWSEGMENT draw appartenant
	 aux couches specifiees par masque_layer
*/
{
wxPoint start, end;
int epaisseur;
int rayon = 0, StAngle = 0, EndAngle = 0;

	if( (g_TabOneLayerMask[pt_segm->m_Layer] & masque_layer) == 0) return;

	epaisseur = pt_segm->m_Width;
	if ( Plot_Mode == FILAIRE) epaisseur = g_PlotLine_Width;

	start = pt_segm->m_Start; end = pt_segm->m_End;

	if(pt_segm->m_Shape == S_CIRCLE)
		{
		rayon = (int)hypot((double)(end.x - start.x),(double)(end.y - start.y) );
		}

	if(pt_segm->m_Shape == S_ARC)
		{
		rayon = (int)hypot((double)(end.x - start.x),(double)(end.y - start.y) );
		StAngle = ArcTangente(end.y - start.y, end.x - start.x);
		EndAngle = StAngle + pt_segm->m_Angle;
		if ( StAngle > EndAngle) EXCHG (StAngle, EndAngle);
		}

	switch ( Format )
		{
		case PLOT_FORMAT_GERBER:
			if(pt_segm->m_Shape == S_CIRCLE)
				PlotCircle(PLOT_FORMAT_GERBER,epaisseur, start , rayon) ;
			else if(pt_segm->m_Shape == S_ARC)
				PlotArc(PLOT_FORMAT_GERBER, start ,
					StAngle, EndAngle, rayon, epaisseur);
			else PlotGERBERLine(start, end, epaisseur) ;
			break;

		case PLOT_FORMAT_HPGL:
			if(pt_segm->m_Shape == S_CIRCLE)
				PlotCircle(PLOT_FORMAT_HPGL,epaisseur, start , rayon) ;
			else if(pt_segm->m_Shape == S_ARC)
				PlotArc(PLOT_FORMAT_HPGL, start , StAngle, EndAngle, rayon, epaisseur);
			else trace_1_segment_HPGL(start.x,start.y, end.x,end.y, epaisseur) ;
			break;

		case PLOT_FORMAT_POST:
			if(pt_segm->m_Shape == S_CIRCLE)
				PlotCircle(PLOT_FORMAT_POST,epaisseur, start, rayon) ;
			else if(pt_segm->m_Shape == S_ARC)
				PlotArc(PLOT_FORMAT_POST, start,
					StAngle, EndAngle, rayon, epaisseur);
			else PlotFilledSegmentPS(start, end, epaisseur) ;
			break;
		}
}


/*****************************************************************************/
void PlotCircle( int format_plot, int epaisseur, wxPoint centre, int rayon)
/*****************************************************************************/
/* routine de trace de 1 cercle de centre cx, cy */
{
	switch ( format_plot )
		{
		case PLOT_FORMAT_GERBER:
			PlotCircle_GERBER( centre, rayon, epaisseur) ;
			break;

		case PLOT_FORMAT_HPGL:
			trace_1_pastille_RONDE_HPGL( centre, rayon * 2, FILAIRE) ;
			break;

		case PLOT_FORMAT_POST:
			PlotCircle_PS ( centre, rayon * 2, epaisseur);
			break;
		}
}


/**********************************************************************/
void PlotPolygon(int format_plot, bool Filled, int nbpoints, int * coord)
/**********************************************************************/
/* plot a polygon */
{
	switch ( format_plot )
	{
		case PLOT_FORMAT_GERBER:
			PlotPolygon_GERBER(nbpoints, coord, Filled) ;
			break;

		case PLOT_FORMAT_HPGL:
			PlotPolyHPGL( nbpoints, coord, Filled);
			break;

		case PLOT_FORMAT_POST:
			PlotPolyPS( nbpoints, coord, Filled);
			break;
	}
}

/************************************************************************/
void PlotArc(int format_plot,wxPoint centre,int start_angle,int end_angle,
					int rayon,int epaisseur)
/************************************************************************/
/* Polt 1 arc
	start, end = angles in 1/10 degree for start and stop point
 */
{
int ii;
int ox, oy, fx, fy;
int delta;  /* increment (en 0.1 degres) angulaire pour trace de cercles */

	if ( Plot_Mode == FILAIRE) epaisseur = g_PlotLine_Width;

	if( format_plot == PLOT_FORMAT_POST )
		{
		PlotArcPS(centre,start_angle,end_angle, rayon, epaisseur);
		return;
		}

	// change due to Y axis is negative orientation on screen
	start_angle = -start_angle;
	end_angle = - end_angle;
	EXCHG (start_angle, end_angle);

	/* Correction pour petits cercles par rapport a l'epaisseur du trait */
	if( rayon < (epaisseur * 10) ) delta = 225; /* 16 segm pour 360 deg */
	if( rayon < (epaisseur * 5) ) delta = 300;  /* 12 segm pour 360 deg */

	if ( start_angle > end_angle ) end_angle += 3600;
	ox = rayon; oy = 0;
	RotatePoint(&ox, &oy, start_angle );

	delta = 120;	/* un cercle sera trace en 3600/delta = 30 segments / cercle*/
	for (ii = start_angle+delta ; ii < end_angle ; ii += delta )
		{
		fx = rayon; fy = 0;
		RotatePoint(&fx, &fy, ii);
		switch ( format_plot )
			{
			case PLOT_FORMAT_GERBER:
				PlotGERBERLine(wxPoint(centre.x+ox, centre.y+oy),
								wxPoint(centre.x+fx,centre.y+fy), epaisseur) ;
				break;

			case PLOT_FORMAT_HPGL:
				trace_1_segment_HPGL(centre.x+ox,centre.y+oy,centre.x+fx,centre.y+fy, epaisseur) ;
				break;

			case PLOT_FORMAT_POST:
				break;
			}
		ox = fx; oy = fy ;
		}

	fx = rayon; fy = 0;
	RotatePoint(&fx, &fy, end_angle);
	switch ( format_plot )
		{
		case PLOT_FORMAT_GERBER:
			PlotGERBERLine(wxPoint(centre.x+ox, centre.y+oy),
							wxPoint(centre.x+fx,centre.y+fy), epaisseur) ;
			break;

		case PLOT_FORMAT_HPGL:
			trace_1_segment_HPGL(centre.x+ox,centre.y+oy,centre.x+fx,centre.y+fy, epaisseur) ;
			break;

		case PLOT_FORMAT_POST:
			break;
		}
}

