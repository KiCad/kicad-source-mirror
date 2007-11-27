/****************************************/
/**** Routine de trace GERBER RS274X ****/
/****************************************/

#include "fctsys.h"

#include "common.h"
#include "plot_common.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "trigo.h"
#include "plotgerb.h"

#include "protos.h"


/* Variables locales : */
static int s_Last_D_code;
static float Gerb_scale_plot;       // Coeff de conversion d'unites des traces
static int scale_spot_mini;         // Ouverture mini (pour remplissages)
static D_CODE * s_DCodeList;        // Pointeur sur la zone de stockage des D_CODES
wxString GerberFullFileName;
static double scale_x, scale_y;     // echelles de convertion en X et Y (compte tenu
                                    // des unites relatives du PCB et des traceurs
static bool ShowDcodeError = TRUE;

/* Routines Locales */

static void Init_Trace_GERBER(WinEDA_BasePcbFrame * frame, FILE * gerbfile);
static void Init_ApertureList();
static void Fin_Trace_GERBER(WinEDA_BasePcbFrame * frame, FILE * gerbfile);
static void Plot_1_CIRCLE_pad_GERBER(wxPoint pos, int diametre);
static void trace_1_pastille_OVALE_GERBER(wxPoint pos, wxSize size, int orient);
static void PlotRectangularPad_GERBER(wxPoint pos, wxSize size, int orient);

static D_CODE * get_D_code(int dx, int dy, int type, int drill);
static void trace_1_pad_TRAPEZE_GERBER(wxPoint pos, wxSize size, wxSize delta,
							int orient, int modetrace);


/********************************************************************************/
void WinEDA_BasePcbFrame::Genere_GERBER(const wxString & FullFileName, int Layer,
		bool PlotOriginIsAuxAxis)
/********************************************************************************/

/* Genere les divers fichiers de trace:
 * Pour chaque couche  1 fichier xxxc.PHO au format RS274X
 */
{
int tracevia = 1;

	EraseMsgBox();
	GerberFullFileName = FullFileName;

	g_PlotOrient = 0;
	if( Plot_Set_MIROIR )
		g_PlotOrient |= PLOT_MIROIR;

	/* Calcul des echelles de conversion */
	Gerb_scale_plot = 1.0;  /* pour unites gerber en 0,1 Mils, format 3.4 */
	scale_spot_mini = (int)(10 * spot_mini * Gerb_scale_plot);
	scale_x = Scale_X * Gerb_scale_plot;
	scale_y = Scale_Y * Gerb_scale_plot;
	g_PlotOffset.x = 0;
	g_PlotOffset.y = 0;
	if( PlotOriginIsAuxAxis )
		g_PlotOffset = m_Auxiliary_Axis_Position;
	
	dest = wxFopen( FullFileName, wxT("wt") );
	if( dest == NULL )
	{
		wxString msg = _("unable to create file ") + FullFileName;
		DisplayError(this, msg);
		return;
	}

	setlocale(LC_NUMERIC, "C");

	InitPlotParametresGERBER(g_PlotOffset, scale_x, scale_y);

	/*	Clear the memory used for handle the D_CODE (aperture) list	 */
	Init_ApertureList();
	
	Affiche_1_Parametre(this, 0, _("File"), FullFileName, CYAN);

	Init_Trace_GERBER(this, dest);

	nb_plot_erreur = 0;

	int layer_mask = g_TabOneLayerMask[Layer];
	// Specify that the contents of the "Edges Pcb" layer are also to be
	// plotted, unless the option of excluding that layer has been selected.
	if( ! Exclude_Edges_Pcb )
		layer_mask |= EDGE_LAYER;

	switch( Layer )
	{
	case FIRST_COPPER_LAYER:
	case LAYER_N_2:
	case LAYER_N_3:
	case LAYER_N_4:
	case LAYER_N_5:
	case LAYER_N_6:
	case LAYER_N_7:
	case LAYER_N_8:
	case LAYER_N_9:
	case LAYER_N_10:
	case LAYER_N_11:
	case LAYER_N_12:
	case LAYER_N_13:
	case LAYER_N_14:
	case LAYER_N_15:
	case LAST_COPPER_LAYER:
		Plot_Layer_GERBER(dest, layer_mask, 0, 1);
		break;

	case SOLDERMASK_N_CU:
	case SOLDERMASK_N_CMP:  /* Trace du vernis epargne */
		if( g_DrawViaOnMaskLayer )
               tracevia = 1;
		else
               tracevia = 0;
		Plot_Layer_GERBER(dest, layer_mask, g_DesignSettings.m_MaskMargin, tracevia);
		break;

	case SOLDERPASTE_N_CU:
	case SOLDERPASTE_N_CMP:  /* Trace du masque de pate de soudure */
		Plot_Layer_GERBER(dest, layer_mask, 0, 0);
		break;

	default:
		Plot_Serigraphie(PLOT_FORMAT_GERBER, dest, layer_mask);
		break;
	}

	Fin_Trace_GERBER( this, dest );
	setlocale( LC_NUMERIC, "" );
}


/***********************************************************************/
void WinEDA_BasePcbFrame::Plot_Layer_GERBER(FILE * File,int masque_layer,
						int garde, int tracevia)
/***********************************************************************/

/* Trace en format GERBER. d'une couche cuivre ou masque
 */
{
wxPoint pos;
wxSize size;
MODULE * Module;
D_PAD * PtPad;
TRACK * track ;
EDA_BaseStruct * PtStruct;
wxString msg;

//	(Following command has been superceded by new command on lines 93 and 94.)
//	masque_layer |= EDGE_LAYER;	/* Les elements de la couche EDGE sont tj traces */

	/* trace des elements type Drawings Pcb : */
	PtStruct = m_Pcb->m_Drawings;
	for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
	{
		switch( PtStruct->Type() )
		{
		case TYPEDRAWSEGMENT:
			PlotDrawSegment( (DRAWSEGMENT*) PtStruct, PLOT_FORMAT_GERBER,
											masque_layer );
			break;

		case TYPETEXTE:
			PlotTextePcb( (TEXTE_PCB*) PtStruct, PLOT_FORMAT_GERBER,
											masque_layer );
			break;

		case TYPECOTATION:
			PlotCotation( (COTATION*) PtStruct, PLOT_FORMAT_GERBER,
											masque_layer );
			break;

		case TYPEMIRE:
			PlotMirePcb( (MIREPCB*) PtStruct, PLOT_FORMAT_GERBER,
											masque_layer );
			break;

		case TYPEMARKER:
			break;

		default:
			DisplayError( this, wxT("Type Draw non gere") );
			break;
		}
	}

	/* Trace des Elements des modules autres que pads */
	nb_items = 0;
	Affiche_1_Parametre(this, 38, wxT("DrawMod"), wxEmptyString, GREEN);
	Module = m_Pcb->m_Modules;
	for( ; Module != NULL; Module = (MODULE *)Module->Pnext )
	{
		PtStruct = Module->m_Drawings;
		for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
		{
			switch( PtStruct->Type() )
			{
			case TYPEEDGEMODULE:
				if( masque_layer & g_TabOneLayerMask[((EDGE_MODULE*)PtStruct)->GetLayer()] )
					Plot_1_EdgeModule(PLOT_FORMAT_GERBER, (EDGE_MODULE*) PtStruct);
				break;

			default:
				break;
			}
		}
	}

	/* Trace des Elements des modules : Pastilles */
	nb_items = 0;
	Affiche_1_Parametre(this, 48, wxT("Pads"), wxEmptyString, GREEN);
	Module = m_Pcb->m_Modules;
	for( ; Module != NULL; Module = (MODULE *)Module->Pnext )
	{
		PtPad = (D_PAD*) Module->m_Pads;
		for( ; PtPad != NULL; PtPad = (D_PAD*)PtPad->Pnext )
		{
			wxPoint shape_pos;
			if( (PtPad->m_Masque_Layer & masque_layer) == 0 )
				continue;
			shape_pos = PtPad->ReturnShapePos();
			pos = shape_pos;

			size.x = PtPad->m_Size.x + 2 * garde;
			size.y = PtPad->m_Size.y + 2 * garde;
			
			/* Don't draw a null size item : */
			if( size.x <= 0 || size.y <= 0 )
				continue;

			nb_items++;

			switch( PtPad->m_PadShape )
			{
			case CIRCLE:
				Plot_1_CIRCLE_pad_GERBER(pos, size.x);
				break;

			case OVALE:
				// Check whether the pad really has a circular shape instead
				if( size.x == size.y )
					Plot_1_CIRCLE_pad_GERBER(pos, size.x);
				else
					trace_1_pastille_OVALE_GERBER(pos, size, PtPad->m_Orient);
				break;

			case TRAPEZE:
				{
					wxSize delta = PtPad->m_DeltaSize;
					trace_1_pad_TRAPEZE_GERBER(pos, size,
								delta, PtPad->m_Orient, FILLED);
				}
				break;

			case RECT:
			default:
				PlotRectangularPad_GERBER(pos, size, PtPad->m_Orient);
				break;
			}
			msg.Printf( wxT("%d"), nb_items );
			Affiche_1_Parametre(this, 48, wxEmptyString, msg, GREEN);
		}
	}
	/* trace des VIAS : */
	if( tracevia )
	{
		nb_items = 0;
		Affiche_1_Parametre(this, 56, wxT("Vias"), wxEmptyString, RED);
		for( track = m_Pcb->m_Track; track != NULL; track = (TRACK*) track->Pnext )
		{
			if( track->Type() != TYPEVIA )
				continue;
			SEGVIA * Via = (SEGVIA *) track;
			// vias not plotted if not on selected layer, but if layer
			// == SOLDERMASK_LAYER_CU or SOLDERMASK_LAYER_CMP, vias are drawn,
			// if they are on a external copper layer
			int via_mask_layer = Via->ReturnMaskLayer();
			if( via_mask_layer & CUIVRE_LAYER )
				via_mask_layer |= SOLDERMASK_LAYER_CU;
			if( via_mask_layer & CMP_LAYER )
				via_mask_layer |= SOLDERMASK_LAYER_CMP;
			if( ( via_mask_layer & masque_layer) == 0 )
				continue;

			pos = Via->m_Start;
			size.x = size.y = Via->m_Width + 2 * garde;
			/* Don't draw a null size item : */
			if( size.x <= 0 )
                continue;
			Plot_1_CIRCLE_pad_GERBER(pos, size.x);
			nb_items++;
			msg.Printf( wxT("%d"), nb_items );
			Affiche_1_Parametre(this, 56, wxEmptyString, msg, RED);
		}
	}
	/* trace des pistes : */
	nb_items = 0;
	Affiche_1_Parametre(this, 64, wxT("Tracks"), wxEmptyString, YELLOW);

	for( track = m_Pcb->m_Track; track != NULL; track = (TRACK*) track->Pnext )
	{
		wxPoint end;

		if( track->Type() == TYPEVIA )
			continue;
		if( (g_TabOneLayerMask[track->GetLayer()] & masque_layer) == 0 )
			continue;

		size.x = size.y = track->m_Width;
		pos = track->m_Start;
		end = track->m_End;

		PlotGERBERLine(pos, end, size.x);

		nb_items++;
		msg.Printf( wxT("%d"), nb_items );
		Affiche_1_Parametre(this, 64, wxEmptyString, msg, YELLOW);
	}

	/* trace des zones: */
	nb_items = 0;
	if( m_Pcb->m_Zone )
		Affiche_1_Parametre(this, 72, wxT("Zones  "), wxEmptyString, YELLOW);

	for( track = m_Pcb->m_Zone; track != NULL; track = (TRACK*) track->Pnext )
	{
		wxPoint end;

		if( (g_TabOneLayerMask[track->GetLayer()] & masque_layer) == 0 )
			continue;

		size.x = size.y = track->m_Width;
		pos = track->m_Start;
		end = track->m_End;

		PlotGERBERLine(pos, end, size.x);

		nb_items++;
		msg.Printf( wxT("%d"), nb_items );
		Affiche_1_Parametre(this, 72, wxEmptyString, msg, YELLOW);
	}
}


/**********************************************************************/
void trace_1_pastille_OVALE_GERBER(wxPoint pos, wxSize size, int orient)
/**********************************************************************/

/* Trace 1 pastille OVALE en position pos_X,Y:
 *     dimensions dx, dy,
 *     orientation orient
 * Pour une orientation verticale ou horizontale, la forme est flashee
 * Pour une orientation quelconque la forme est tracee comme un segment
 */
{
D_CODE * dcode_ptr;
int x0, y0, x1, y1, delta;

	if( orient == 900 || orient == 2700 ) /* orient tournee de 90 deg */
		EXCHG(size.x, size.y);

	/* Trace de la forme flashee */
    if( orient == 0 || orient == 900 || orient == 1800 || orient == 2700 )
	{
		UserToDeviceCoordinate(pos);
		UserToDeviceSize(size);

		dcode_ptr = get_D_code(size.x, size.y, GERB_OVALE, 0);
		if( dcode_ptr->m_NumDcode != s_Last_D_code )
		{
			sprintf(cbuf, "G54D%d*\n", dcode_ptr->m_NumDcode);
			fputs(cbuf, dest);
			s_Last_D_code = dcode_ptr->m_NumDcode;
		}
		sprintf(cbuf, "X%5.5dY%5.5dD03*\n", pos.x, pos.y);
		fputs(cbuf, dest);
	}

	else /* Forme tracee comme un segment */
	{
		if( size.x > size.y )
		{
			EXCHG(size.x, size.y);
			if( orient < 2700 )
				orient += 900;
			else
				orient -= 2700;
		}
		/* la pastille est ramenee a une pastille ovale avec dy > dx */
		delta = size.y - size.x;
		x0 = 0;
		y0 = -delta / 2;
		x1 = 0;
		y1 = delta / 2;
		RotatePoint(&x0, &y0, orient);
		RotatePoint(&x1, &y1, orient);
		PlotGERBERLine( wxPoint(pos.x + x0, pos.y + y0),
						wxPoint(pos.x + x1, pos.y + y1), size.x );
	}
}


/******************************************************************/
void Plot_1_CIRCLE_pad_GERBER(wxPoint pos,int diametre)
/******************************************************************/

/* Plot a circular pad or via at the user position pos
 */
{
D_CODE * dcode_ptr;
wxSize size(diametre, diametre);

	UserToDeviceCoordinate(pos);
	UserToDeviceSize(size);

	dcode_ptr = get_D_code(size.x, size.x, GERB_CIRCLE, 0);
	if( dcode_ptr->m_NumDcode != s_Last_D_code )
	{
		sprintf(cbuf, "G54D%d*\n", dcode_ptr->m_NumDcode);
		fputs(cbuf, dest);
		s_Last_D_code = dcode_ptr->m_NumDcode;
	}

	sprintf(cbuf, "X%5.5dY%5.5dD03*\n", pos.x, pos.y);
	fputs(cbuf, dest);
}


/**************************************************************************/
void PlotRectangularPad_GERBER(wxPoint pos, wxSize size, int orient)
/**************************************************************************/

/* Trace 1 pad rectangulaire d'orientation quelconque
 * donne par son centre, ses dimensions, et son orientation
 * Pour une orientation verticale ou horizontale, la forme est flashee
 * Pour une orientation quelconque la forme est tracee par 4 segments
 * de largeur 1/2 largeur pad
 */
{
D_CODE * dcode_ptr;

	/* Trace de la forme flashee */
	switch( orient )
	{
	case 900:
	case 2700: /* la rotation de 90 ou 270 degres revient a permutter des dimensions */
		EXCHG(size.x, size.y);
		// Pass through

	case 0:
	case 1800:
		UserToDeviceCoordinate(pos);
		UserToDeviceSize(size);

		dcode_ptr = get_D_code(size.x, size.y, GERB_RECT, 0);
		if( dcode_ptr->m_NumDcode != s_Last_D_code )
		{
			sprintf(cbuf, "G54D%d*\n", dcode_ptr->m_NumDcode);
			fputs(cbuf, dest);
			s_Last_D_code = dcode_ptr->m_NumDcode;
		}
		sprintf(cbuf, "X%5.5dY%5.5dD03*\n", pos.x, pos.y);
		fputs(cbuf, dest);
		break;

	default: /* Forme tracee par remplissage */
		trace_1_pad_TRAPEZE_GERBER(pos, size, wxSize(0, 0), orient, FILLED);
		break;
	}
}


/*****************************************************************/
void trace_1_contour_GERBER(wxPoint pos, wxSize size, wxSize delta,
										int penwidth, int orient)
/*****************************************************************/

/* Trace 1 contour rectangulaire ou trapezoidal d'orientation quelconque
 * donne par son centre,
 * ses dimensions ,
 * ses variations ,
 * l'epaisseur du trait,
 * et son orientation orient
 */
{
int ii;
wxPoint coord[4];

	size.x /= 2;
	size.y /= 2;
	delta.x /= 2;
	delta.y /= 2; /* demi dim  dx et dy */

	coord[0].x = pos.x - size.x - delta.y;
	coord[0].y = pos.y + size.y + delta.x;

	coord[1].x = pos.x - size.x + delta.y;
	coord[1].y = pos.y - size.y - delta.x;

	coord[2].x = pos.x + size.x - delta.y;
	coord[2].y = pos.y - size.y + delta.x;

	coord[3].x = pos.x + size.x + delta.y;
	coord[3].y = pos.y + size.y - delta.x;

	for( ii = 0; ii < 4; ii++ )
	{
		RotatePoint(&coord[ii].x, &coord[ii].y, pos.x, pos.y, orient);
	}

	PlotGERBERLine( coord[0], coord[1], penwidth );
	PlotGERBERLine( coord[1], coord[2], penwidth );
	PlotGERBERLine( coord[2], coord[3], penwidth );
	PlotGERBERLine( coord[3], coord[0], penwidth );
}


/*******************************************************************/
void trace_1_pad_TRAPEZE_GERBER(wxPoint pos, wxSize size,wxSize delta,
							int orient,int modetrace)
/*******************************************************************/

/* Trace 1 pad trapezoidal donne par :
 *    son centre pos.x,pos.y
 *    ses dimensions size.x et size.y
 *    les variations delta.x et delta.y ( 1 des deux au moins doit etre nulle)
 *    son orientation orient en 0.1 degres
 *    le mode de trace (FILLED, SKETCH, FILAIRE)
 *
 * Le trace n'est fait que pour un trapeze, c.a.d que delta.x ou delta.y
 *    = 0.
 *
 *   les notation des sommets sont ( vis a vis de la table tracante )
 *
 * "       0 ------------- 3   "
 * "        .             .    "
 * "         .     O     .     "
 * "          .         .      "
 * "           1 ---- 2        "
 *
 *
 *   exemple de Disposition pour delta.y > 0, delta.x = 0
 * "           1 ---- 2        "
 * "          .         .      "
 * "         .     O     .     "
 * "        .             .    "
 * "       0 ------------- 3   "
 *
 *
 *   exemple de Disposition pour delta.y = 0, delta.x > 0
 * "       0                  "
 * "       . .                "
 * "       .     .            "
 * "       .           3      "
 * "       .           .      "
 * "       .     O     .      "
 * "       .           .      "
 * "       .           2      "
 * "       .     .            "
 * "       . .                "
 * "       1                  "
 */
{
int ii, jj;
int dx, dy;
wxPoint polygone[4];	/* coord sommets */
int coord[8];
int ddx, ddy;

	/* calcul des dimensions optimales du spot choisi = 1/4 plus petite dim */
	dx = size.x - abs(delta.y);
	dy = size.y - abs(delta.x);

	dx = size.x / 2;
	dy = size.y / 2;
	ddx = delta.x / 2;
	ddy = delta.y / 2;

	polygone[0].x = - dx - ddy;
	polygone[0].y = + dy + ddx;
	polygone[1].x = - dx + ddy;
	polygone[1].y = - dy - ddx;
	polygone[2].x = + dx - ddy;
	polygone[2].y = - dy + ddx;
	polygone[3].x = + dx + ddy;
	polygone[3].y = + dy - ddx;

	/* Dessin du polygone et Remplissage eventuel de l'interieur */

	for( ii = 0, jj = 0; ii < 4; ii++ )
	{
		RotatePoint(&polygone[ii].x, &polygone[ii].y, orient);
		coord[jj] = polygone[ii].x += pos.x;
		jj++;
		coord[jj] = polygone[ii].y += pos.y;
		jj++;
	}

	if( modetrace != FILLED )
	{
		PlotGERBERLine( polygone[0], polygone[1], scale_spot_mini );
		PlotGERBERLine( polygone[1], polygone[2], scale_spot_mini );
		PlotGERBERLine( polygone[2], polygone[3], scale_spot_mini );
		PlotGERBERLine( polygone[3], polygone[0], scale_spot_mini );
	}
	
	else
		PlotPolygon_GERBER(4, coord, TRUE);
}


/**********************************************************/
void PlotGERBERLine(wxPoint start, wxPoint end, int large)
/**********************************************************/

/* Trace 1 segment de piste :
 */
{
D_CODE * dcode_ptr;

	UserToDeviceCoordinate(start);
	UserToDeviceCoordinate(end);

	dcode_ptr = get_D_code(large, large, GERB_LINE, 0);
	if(dcode_ptr->m_NumDcode != s_Last_D_code )
	{
		sprintf(cbuf, "G54D%d*\n", dcode_ptr->m_NumDcode);
		fputs(cbuf, dest);
		s_Last_D_code = dcode_ptr->m_NumDcode;
	}
	sprintf(cbuf, "X%5.5dY%5.5dD02*\n", start.x, start.y);
	fputs(cbuf, dest);
	sprintf(cbuf, "X%5.5dY%5.5dD01*\n", end.x, end.y);
	fputs(cbuf, dest);
}


/********************************************************************/
void PlotCircle_GERBER( wxPoint centre, int rayon, int epaisseur )
/********************************************************************/

/* routine de trace de 1 cercle de centre centre par approximation de segments
 */
{
int ii;
int ox, oy, fx, fy;
int delta;  /* increment (en 0.1 degres) angulaire pour trace de cercles */

	delta = 120;	/* un cercle sera trace en 3600/delta segments */
	/* Correction pour petits cercles par rapport a l'epaisseur du trait */
	if( rayon < (epaisseur * 10) )
		delta = 225; /* 16 segm pour 360 deg */
	if( rayon < (epaisseur * 5) )
		delta = 300;  /* 12 segm pour 360 deg */
	if( rayon < (epaisseur * 2) )
		delta = 600;  /* 6 segm pour 360 deg */

	ox = centre.x + rayon;
	oy = centre.y;
	for( ii = delta; ii < 3600; ii += delta )
	{
		fx = centre.x + (int)(rayon * fcosinus[ii]);
		fy = centre.y + (int)(rayon * fsinus[ii]);
		PlotGERBERLine(wxPoint(ox, oy), wxPoint(fx, fy), epaisseur);
		ox = fx;
		oy = fy;
	}

	fx = centre.x + rayon;
	fy = centre.y;
	PlotGERBERLine(wxPoint(ox ,oy), wxPoint(fx, fy), epaisseur);
}


/***************************************************************/
void PlotPolygon_GERBER(int nb_segm, int * coord, bool fill)
/***************************************************************/
{
int ii;
wxPoint pos;

	fputs("G36*\n", dest);
	pos.x = *coord;
	coord++;
	pos.y = *coord;
	coord++;
	UserToDeviceCoordinate(pos);
	fprintf( dest, "X%5.5dY%5.5dD02*\n", pos.x, pos.y );
	for( ii = 1; ii < nb_segm; ii++ )
	{
		pos.x = *coord;
		coord++;
		pos.y = *coord;
		coord++;
		UserToDeviceCoordinate(pos);
		fprintf( dest, "X%5.5dY%5.5dD01*\n", pos.x, pos.y );
	}

	fputs("G37*\n", dest);
}


/*******************************************************/
D_CODE * get_D_code( int dx, int dy, int type, int drill )
/*******************************************************/

/* Fonction Recherchant et Creant eventuellement la description
 *   du D_CODE du type et dimensions demandees
 */
{
D_CODE * ptr_tool, * last_dcode_ptr;
int num_new_D_code = FIRST_DCODE_VALUE;


	ptr_tool = last_dcode_ptr = s_DCodeList;

	while( ptr_tool && ptr_tool->m_Type >= 0 )
	{
		if( ( ptr_tool->m_Size.x == dx ) &&
			( ptr_tool->m_Size.y == dy ) &&
			( ptr_tool->m_Type == type ) )
					return(ptr_tool);	/* D_code deja existant */
			last_dcode_ptr = ptr_tool;
			ptr_tool = ptr_tool->m_Pnext;
			num_new_D_code++;
	}

	/* At this point, the requested D_CODE does not exist: It will be created */
	if( ptr_tool == NULL )  /* We must create a new data */
	{
		ptr_tool = new D_CODE();
		ptr_tool->m_NumDcode = num_new_D_code;
		if( last_dcode_ptr )
		{
			ptr_tool->m_Pback = last_dcode_ptr;
			last_dcode_ptr->m_Pnext = ptr_tool;
		}
		else
			s_DCodeList = ptr_tool;
	}
	ptr_tool->m_Size.x = dx;
	ptr_tool->m_Size.y = dy;
	ptr_tool->m_Type = type;
	return(ptr_tool);
}


/******************************************************************/
void Init_Trace_GERBER(WinEDA_BasePcbFrame * frame, FILE * gerbfile)
/******************************************************************/
{
char Line[1024];

	s_Last_D_code = 0;

	DateAndTime(Line);
	wxString Title = g_Main_Title + wxT(" ") + GetBuildVersion();
	fprintf(gerbfile, "G04 (Genere par %s) le %s*\n", CONV_TO_UTF8(Title), Line);

	// Specify linear interpol (G01), unit = INCH (G70), abs format (G90):
	fputs("G01*\nG70*\nG90*\n", gerbfile);
	fputs("%MOIN*%\n", gerbfile);		// set unites = INCHES

	/* Set gerber format to 3.4 */
	strcpy(Line, "G04 Gerber Fmt 3.4, Leading zero omitted, Abs format*\n%FSLAX34Y34*%\n");

	fputs(Line, gerbfile);

	fputs("G04 APERTURE LIST*\n", gerbfile);
}


/***********************************/
static void Init_ApertureList()
/***********************************/

/* Init the memory to handle the aperture list:
 *   the member .m_Type is used by get_D_code() to handle the end of list:
 *   .m_Type < 0 is the first free aperture descr
 */
{
D_CODE * ptr_tool;

	ptr_tool = s_DCodeList;
	while( ptr_tool )
	{
		s_DCodeList->m_Type = -1;
		ptr_tool = ptr_tool->m_Pnext;
	}
	ShowDcodeError = TRUE;
}


/*****************************************************************/
void Fin_Trace_GERBER(WinEDA_BasePcbFrame * frame, FILE * gerbfile)
/*****************************************************************/
{
char line[1024];
wxString TmpFileName, msg;
FILE * outfile;

	fputs("M02*\n", gerbfile);
	fclose(gerbfile);

	// Reouverture gerbfile pour ajout des Apertures
	gerbfile = wxFopen( GerberFullFileName, wxT("rt") );
	if( gerbfile == NULL )
	{
		msg.Printf( _("unable to reopen file <%s>"), GerberFullFileName.GetData() );
		DisplayError(frame, msg);
		return;
	}

	// Ouverture tmpfile
	TmpFileName = GerberFullFileName + wxT(".$$$");
	outfile = wxFopen( TmpFileName, wxT("wt") );
	if( outfile == NULL )
	{
		fclose(gerbfile);
		DisplayError(frame, wxT("Fin_Trace_GERBER(): Can't Open tmp file"));
		return;
	}

	// Placement des Apertures en RS274X
	rewind(gerbfile);
	while( fgets(line, 1024, gerbfile) )
	{
		fputs(line, outfile);
		if( strcmp(strtok(line, "\n\r"), "G04 APERTURE LIST*") == 0 )
		{
			frame->Gen_D_CODE_File(outfile);
			fputs("G04 APERTURE END LIST*\n", outfile);
		}
	}

	fclose(outfile);
	fclose(gerbfile);
	wxRemoveFile(GerberFullFileName);
	wxRenameFile(TmpFileName, GerberFullFileName);
}


/******************************************************/
int WinEDA_BasePcbFrame::Gen_D_CODE_File(FILE * penfile)
/******************************************************/

/* Genere la liste courante des D_CODES
 * Retourne le nombre de D_Codes utilises
 * Genere une sequence RS274X
 */
{
D_CODE * ptr_tool;
int nb_dcodes = 0 ;

	/* Init : */
	ptr_tool = s_DCodeList;

	while( ptr_tool && ( ptr_tool->m_Type >= 0 ) )
	{
		float fscale = 0.0001; // For 3.4 format
		char * text;
		sprintf(cbuf, "%%ADD%d", ptr_tool->m_NumDcode);
		text = cbuf + strlen(cbuf);
		switch( ptr_tool->m_Type )
		{
		case 1:	// Circle (flash )
			sprintf(text, "C,%f*%%\n", ptr_tool->m_Size.x * fscale);
			break;

		case 2:	// RECT
			sprintf(text, "R,%fX%f*%%\n", ptr_tool->m_Size.x * fscale,
							ptr_tool->m_Size.y * fscale);
			break;

		case 3:	// Circle ( lines )
			sprintf(text, "C,%f*%%\n", ptr_tool->m_Size.x * fscale);
			break;

		case 4:	// OVALE
			sprintf(text, "O,%fX%f*%%\n", ptr_tool->m_Size.x * fscale,
							ptr_tool->m_Size.y * fscale);
			break;

		default:
			DisplayError( this, wxT("Gen_D_CODE_File(): Dcode Type err") );
			break;
		}
		// compensation localisation printf (float x.y généré x,y)
		to_point( text + 2 );

		fputs(cbuf, penfile);
		ptr_tool = ptr_tool->m_Pnext;
		nb_dcodes++;
	}

	return( nb_dcodes );
}
