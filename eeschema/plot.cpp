		/************************************************/
		/* Routine de trace communes aux divers formats */
		/************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "plot_common.h"
#include "worksheet.h"

#include "protos.h"

/* Variables locales : */
static void PlotSheetLabelStruct(DrawSheetLabelStruct *Struct);
static void PlotTextField( EDA_SchComponentStruct *DrawLibItem,
					int FieldNumber, int IsMulti, int DrawMode);
static void PlotPinSymbol(int posX, int posY, int len, int orient, int Shape);
/***/

/* cte pour remplissage de polygones */
#define FILL 1
#define NOFILL 0

#define PLOT_SHEETREF_MARGIN 0		// margin for sheet refs

	/*******************************/
	/* Routines de base de trace : */
	/*******************************/
/* routine de lever ou baisser de plume.
	si plume = 'U' les traces suivants se feront plume levee
	si plume = 'D' les traces suivants se feront plume levee
*/
void Plume( int plume )
{
	if ( g_PlotFormat  == PLOT_FORMAT_HPGL ) Plume_HPGL(plume);
}

/* routine de deplacement de plume de plume.
*/
void Move_Plume( wxPoint pos, int plume )
{
	switch ( g_PlotFormat )
		{
		case PLOT_FORMAT_HPGL:
			Move_Plume_HPGL(pos, plume);
			break;
		case PLOT_FORMAT_POST:
		case PLOT_FORMAT_POST_A4:
			LineTo_PS(pos, plume);
			break;
		}
}

void SetCurrentLineWidth( int width)
{
	switch ( g_PlotFormat )
	{
		case PLOT_FORMAT_HPGL:
			break;

		case PLOT_FORMAT_POST:
		case PLOT_FORMAT_POST_A4:
			SetCurrentLineWidthPS(width);
			break;
	}
}


/*******************************************************************************/
void PlotArc(wxPoint centre, int StAngle, int EndAngle, int rayon, int width)
/*******************************************************************************/
/* trace d'un arc de cercle:
	x, y = coord du centre
	StAngle, EndAngle = angle de debut et fin
	rayon = rayon de l'arc
*/
{
	switch ( g_PlotFormat )
		{
		case PLOT_FORMAT_HPGL:
			PlotArcHPGL(centre, StAngle, EndAngle, rayon, width);
			break;
		case PLOT_FORMAT_POST:
			PlotArcPS(centre, StAngle, EndAngle, rayon, width);
			break;
		}
}

/*******************************************************/
void PlotCercle( wxPoint pos,int diametre, int width )
/*******************************************************/
{
	switch ( g_PlotFormat )
	{
		case PLOT_FORMAT_HPGL:
			PlotCircle_HPGL( pos, diametre, width);
			break;

		case PLOT_FORMAT_POST:
			PlotCircle_PS(pos, diametre, width);
			break;
	}
}

/******************************************************************/
void PlotPoly( int nb, int * coord, int fill, int width)
/******************************************************************/
/* Trace un polygone ferme
	coord = tableau des coord des sommets
	nb = nombre de coord ( 1 coord = 2 elements: X et Y du tableau )
	fill : si != 0 polygone rempli
*/
{
	if( nb <= 1 ) return;

	switch ( g_PlotFormat )
		{
		case PLOT_FORMAT_HPGL:
			PlotPolyHPGL( nb, coord,  fill, width);
			break;

		case PLOT_FORMAT_POST:
			PlotPolyPS( nb, coord,  fill, width);
			break;
		}
}



/**********************************************************/
void PlotNoConnectStruct(DrawNoConnectStruct * Struct)
/**********************************************************/
/* Routine de dessin des symboles de "No Connexion" ..
*/
{
#define DELTA (DRAWNOCONNECT_SIZE/2)
int pX, pY;

	pX = Struct->m_Pos.x; pY = Struct->m_Pos.y;

	SetCurrentLineWidth(-1);
	Move_Plume(wxPoint(pX - DELTA, pY - DELTA), 'U');
	Move_Plume(wxPoint(pX + DELTA, pY + DELTA), 'D');
	Move_Plume(wxPoint(pX + DELTA, pY - DELTA), 'U');
	Move_Plume(wxPoint(pX - DELTA, pY + DELTA), 'D');
	Plume('U');
}

/*************************************************/
void PlotLibPart( EDA_SchComponentStruct *DrawLibItem )
/*************************************************/
/* Genere le trace d'un composant */
{
int ii, x1, y1, x2, y2, t1, t2, *Poly, orient;
LibEDA_BaseStruct *DEntry;
EDA_LibComponentStruct *Entry;
int TransMat[2][2], PartX, PartY, Multi, convert;
int CharColor = -1;
wxPoint pos;

	Entry = FindLibPart(DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT);
	if( Entry == NULL) return;;
	memcpy(TransMat, DrawLibItem->m_Transform, sizeof(TransMat));
	PartX = DrawLibItem->m_Pos.x; PartY = DrawLibItem->m_Pos.y;
	Multi = DrawLibItem->m_Multi;
	convert = DrawLibItem->m_Convert;

	for( DEntry = Entry->m_Drawings; DEntry != NULL;DEntry = DEntry->Next())
	{
		/* Elimination des elements non relatifs a l'unite */
		if( Multi && DEntry->m_Unit && (DEntry->m_Unit != Multi) ) continue;
		if( convert && DEntry->m_Convert && (DEntry->m_Convert != convert) )
					continue;

		Plume('U');
		if ( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
			SetColorMapPS ( ReturnLayerColor(LAYER_DEVICE) );

		switch (DEntry->Type())
		{
			case COMPONENT_ARC_DRAW_TYPE:
			{
				LibDrawArc * Arc = (LibDrawArc *) DEntry;
				t1 = Arc->t1; t2 = Arc->t2;
				pos.x = PartX + TransMat[0][0] * Arc->m_Pos.x +
							  TransMat[0][1] * Arc->m_Pos.y;
				pos.y = PartY + TransMat[1][0] * Arc->m_Pos.x +
							  TransMat[1][1] * Arc->m_Pos.y;
				MapAngles(&t1, &t2, TransMat);
				PlotArc(pos, t1, t2, Arc->m_Rayon, Arc->m_Width);
			}
				break;

			case COMPONENT_CIRCLE_DRAW_TYPE:
			{
				LibDrawCircle * Circle = (LibDrawCircle *) DEntry;
				pos.x = PartX + TransMat[0][0] * Circle->m_Pos.x +
								 TransMat[0][1] * Circle->m_Pos.y;
				pos.y = PartY + TransMat[1][0] * Circle->m_Pos.x +
								 TransMat[1][1] * Circle->m_Pos.y;
				PlotCercle(pos, Circle->m_Rayon * 2, Circle->m_Width);
			}
				break;

			case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
			{
				LibDrawText * Text = (LibDrawText *) DEntry;
				/* The text orientation may need to be flipped if the
				transformation matrix causes xy axes to be flipped. */
				t1 = (TransMat[0][0] != 0) ^ (Text->m_Horiz != 0);
				pos.x = PartX + TransMat[0][0] * Text->m_Pos.x
						  + TransMat[0][1] * Text->m_Pos.y;
				pos.y = PartY + TransMat[1][0] * Text->m_Pos.x
						  + TransMat[1][1] * Text->m_Pos.y;
				SetCurrentLineWidth(-1);
				PlotGraphicText(g_PlotFormat, pos , CharColor,
							Text->m_Text,
							t1 ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT,
							Text->m_Size,
							GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER);
			}
				break;

			case COMPONENT_RECT_DRAW_TYPE:
			{
				LibDrawSquare * Square = (LibDrawSquare *) DEntry;
				x1 = PartX + TransMat[0][0] * Square->m_Pos.x
						  + TransMat[0][1] * Square->m_Pos.y;
				y1 = PartY + TransMat[1][0] * Square->m_Pos.x
						  + TransMat[1][1] * Square->m_Pos.y;
				x2 = PartX + TransMat[0][0] * Square->m_End.x
						  + TransMat[0][1] * Square->m_End.y;
				y2 = PartY + TransMat[1][0] * Square->m_End.x
						 + TransMat[1][1] * Square->m_End.y;

				SetCurrentLineWidth(Square->m_Width);
				Move_Plume(wxPoint(x1, y1), 'U');
				Move_Plume(wxPoint(x1, y2), 'D');
				Move_Plume(wxPoint(x2, y2), 'D');
				Move_Plume(wxPoint(x2, y1), 'D');
				Move_Plume(wxPoint(x1, y1), 'D');
			}
				break;

			case COMPONENT_PIN_DRAW_TYPE: /* Trace des Pins */
			{
				LibDrawPin * Pin = (LibDrawPin *) DEntry;
				if(Pin->m_Attributs & PINNOTDRAW)
				{
					if( ActiveScreen->m_Type == SCHEMATIC_FRAME )
						break;
				}

				/* Calcul de l'orientation reelle de la Pin */
				orient = Pin->ReturnPinDrawOrient(TransMat);
				/* compute Pin Pos */
				x2 = PartX + TransMat[0][0] * Pin->m_Pos.x
						  + TransMat[0][1] * Pin->m_Pos.y;
				y2 = PartY + TransMat[1][0] * Pin->m_Pos.x
						  + TransMat[1][1] * Pin->m_Pos.y;

				/* Dessin de la pin et du symbole special associe */
				SetCurrentLineWidth(-1);
				PlotPinSymbol(x2, y2, Pin->m_PinLen, orient, Pin->m_PinShape);
				wxPoint pinpos(x2, y2);
				Pin->PlotPinTexts(pinpos, orient,
									 Entry->m_TextInside,
									 Entry->m_DrawPinNum, Entry->m_DrawPinName);
			}
				break;

			case COMPONENT_POLYLINE_DRAW_TYPE:
			{
				LibDrawPolyline * polyline = (LibDrawPolyline *) DEntry;
				Poly = (int *) MyMalloc(sizeof(int) * 2 * polyline->n);
				for (ii = 0; ii < polyline->n; ii++)
				{
					Poly[ii * 2] = PartX +
							 TransMat[0][0] * polyline->PolyList[ii * 2] +
							 TransMat[0][1] * polyline->PolyList[ii * 2 + 1];
					Poly[ii * 2 + 1] = PartY +
							 TransMat[1][0] * polyline->PolyList[ii * 2] +
							 TransMat[1][1] * polyline->PolyList[ii * 2 + 1];
				}
				PlotPoly(ii, Poly, polyline->m_Fill, polyline->m_Width);
				MyFree(Poly);
			}
				break;
                
            default:;
                
		}	/* Fin Switch */
		Plume('U');
	}	/* Fin Boucle de dessin */

	/* Trace des champs, avec placement et orientation selon orient. du
		composant
		Si la reference commence par # elle n'est pas tracee
	*/

	if( (Entry->m_Prefix.m_Attributs & TEXT_NO_VISIBLE) == 0)
		{
		if ( Entry->m_UnitCount > 1 )
			PlotTextField(DrawLibItem,REFERENCE,1,0);
		else
			PlotTextField(DrawLibItem,REFERENCE,0,0);
		}

	if( (Entry->m_Name.m_Attributs & TEXT_NO_VISIBLE) == 0)
		PlotTextField(DrawLibItem,VALUE,0,0);

	for( ii = 2; ii < NUMBER_OF_FIELDS; ii++ )
		{
		PlotTextField(DrawLibItem,ii,0,0);
		}
}

/*************************************************************/
static void PlotTextField( EDA_SchComponentStruct *DrawLibItem,
					int FieldNumber, int IsMulti, int DrawMode)
/**************************************************************/

/* Routine de trace des textes type Field du composant.
	entree:
		DrawLibItem: pointeur sur le composant
		FieldNumber: Numero du champ
		IsMulti: flag Non Null si il y a plusieurs parts par boitier.
				n'est utile que pour le champ reference pour ajouter a celui ci
				l'identification de la part ( A, B ... )
		DrawMode: mode de trace
*/

{
int posX, posY;		/* Position des textes */
int px, py, x1, y1;
PartTextStruct * Field = & DrawLibItem->m_Field[FieldNumber];
int hjustify, vjustify;
int orient, color = -1;

	if ( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
		color = ReturnLayerColor(Field->m_Layer);

	DrawMode = 0;	/* Unused */
	if( Field->m_Attributs & TEXT_NO_VISIBLE ) return;
	if( Field->IsVoid() ) return;

	/* Calcul de la position des textes, selon orientation du composant */
	orient = Field->m_Orient;
	hjustify = Field->m_HJustify; vjustify = Field->m_VJustify;
	posX = DrawLibItem->m_Pos.x; posY = DrawLibItem->m_Pos.y;
	x1 = Field->m_Pos.x - posX;
	y1 = Field->m_Pos.y - posY;

	px = posX  + (DrawLibItem->m_Transform[0][0] * x1)
					+ (DrawLibItem->m_Transform[0][1] * y1);
	py = posY + (DrawLibItem->m_Transform[1][0] * x1)
					+ (DrawLibItem->m_Transform[1][1] * y1);

	/* Y a t-il rotation */
	if(DrawLibItem->m_Transform[0][1])
		{
		if ( orient == TEXT_ORIENT_HORIZ) orient = TEXT_ORIENT_VERT;
		else orient = TEXT_ORIENT_HORIZ;
		/* Y a t-il rotation, miroir (pour les justifications)*/
		EXCHG(hjustify, vjustify);
		if (DrawLibItem->m_Transform[1][0] < 0 ) vjustify = - vjustify;
		if (DrawLibItem->m_Transform[1][0] > 0 ) hjustify = - hjustify;
		}
	else
	{	/* Texte horizontal: Y a t-il miroir (pour les justifications)*/
		if (DrawLibItem->m_Transform[0][0] < 0 )
			hjustify = - hjustify;
		if (DrawLibItem->m_Transform[1][1] > 0 )
			vjustify = - vjustify;
	}

	SetCurrentLineWidth(-1);
	//not sure what to do here in terms of plotting components that may have multiple REFERENCE entries. 
	if( !IsMulti || (FieldNumber != REFERENCE) )
	{
		PlotGraphicText( g_PlotFormat, wxPoint(px, py), color, Field->m_Text,
						orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
						Field->m_Size,
						hjustify, vjustify);
	}

	else /* Le champ est la reference, et il y a plusieurs parts par boitier */
	{	/* On ajoute alors A ou B ... a la reference */
		wxString Text;
		Text = Field->m_Text;
		Text.Append('A' - 1 + DrawLibItem->m_Multi);
		PlotGraphicText( g_PlotFormat, wxPoint(px, py), color, Text,
						orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
						Field->m_Size, hjustify, vjustify);
	}
}


/**************************************************************************/
static void PlotPinSymbol(int posX, int posY, int len, int orient, int Shape)
/**************************************************************************/
/* Trace la pin du symbole en cours de trace
*/
{
int MapX1, MapY1, x1, y1;
int color;

	color = ReturnLayerColor(LAYER_PIN);

	if ( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
		SetColorMapPS ( color );

	SetCurrentLineWidth(-1);

	MapX1 = MapY1 = 0; x1 = posX; y1 = posY;
	switch ( orient )
		{
		case PIN_UP:
			y1 = posY - len; MapY1 = 1;
			break;
		case PIN_DOWN:
			y1 = posY + len; MapY1 = -1;
			break;
		case PIN_LEFT:
			x1 = posX - len, MapX1 = 1;
			break;
		case PIN_RIGHT:
			x1 = posX + len; MapX1 = -1;
			break;
		}

	if( Shape & INVERT)
		{
		PlotCercle( wxPoint(MapX1 * INVERT_PIN_RADIUS + x1,
					MapY1 * INVERT_PIN_RADIUS + y1),
					INVERT_PIN_RADIUS * 2);

		Move_Plume( wxPoint(MapX1 * INVERT_PIN_RADIUS * 2 + x1,
						MapY1 * INVERT_PIN_RADIUS * 2 + y1), 'U' );
		Move_Plume(wxPoint(posX, posY), 'D' );
		}
	else
		{
		Move_Plume(wxPoint(x1, y1), 'U' );
		Move_Plume(wxPoint(posX, posY), 'D' );
		}

	if(Shape & CLOCK)
		{
		if(MapY1 == 0 ) /* MapX1 = +- 1 */
			{
			Move_Plume(wxPoint(x1, y1 + CLOCK_PIN_DIM), 'U' );
			Move_Plume(wxPoint(x1 - MapX1 *  CLOCK_PIN_DIM, y1), 'D' );
			Move_Plume(wxPoint(x1, y1 - CLOCK_PIN_DIM), 'D' );
			}
		else	/* MapX1 = 0 */
			{
			Move_Plume( wxPoint(x1 + CLOCK_PIN_DIM, y1), 'U' );
			Move_Plume( wxPoint(x1, y1 - MapY1 *  CLOCK_PIN_DIM), 'D' );
			Move_Plume( wxPoint(x1 - CLOCK_PIN_DIM, y1), 'D' );
			}
		}

	if(Shape & LOWLEVEL_IN) /* IEEE symbol "Active Low Input" */
		{
		if(MapY1 == 0 ) /* MapX1 = +- 1 */
			{
			Move_Plume(wxPoint(x1 + MapX1 * IEEE_SYMBOL_PIN_DIM*2, y1), 'U');
			Move_Plume(wxPoint(x1 + MapX1 * IEEE_SYMBOL_PIN_DIM*2,
							y1 - IEEE_SYMBOL_PIN_DIM), 'D');
			Move_Plume( wxPoint(x1, y1), 'D' );
			}
		else	/* MapX1 = 0 */
			{
			Move_Plume( wxPoint(x1, y1 + MapY1 * IEEE_SYMBOL_PIN_DIM*2), 'U');
			Move_Plume(wxPoint(x1 - IEEE_SYMBOL_PIN_DIM,
						y1 + MapY1 * IEEE_SYMBOL_PIN_DIM*2), 'D');
			Move_Plume(wxPoint(x1, y1), 'D');
			}
		}


	if(Shape & LOWLEVEL_OUT) /* IEEE symbol "Active Low Output" */
		{
		if(MapY1 == 0 ) /* MapX1 = +- 1 */
			{
			Move_Plume(wxPoint(x1, y1 - IEEE_SYMBOL_PIN_DIM), 'U' );
			Move_Plume(wxPoint(x1 + MapX1 * IEEE_SYMBOL_PIN_DIM*2,  y1), 'D' );
			}
		else	/* MapX1 = 0 */
			{
			Move_Plume(wxPoint(x1 - IEEE_SYMBOL_PIN_DIM, y1), 'U');
			Move_Plume(wxPoint(x1 , y1 + MapY1 * IEEE_SYMBOL_PIN_DIM*2),'D' );
			}
		}
	Plume('U');
}


/*******************************************/
void PlotTextStruct(EDA_BaseStruct *Struct)
/*******************************************/
/*
	Routine de trace des Textes, Labels et Global-Labels.
	Les textes peuvent avoir 4 directions.
*/
{
int * Template;
int Poly[50];
int ii, pX, pY, Shape = 0, Orient = 0, offset;
wxSize Size;
wxString Text;
int color = -1;
int HalfSize;

	switch ( Struct->Type() )
		{
		case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
		case DRAW_HIER_LABEL_STRUCT_TYPE:
		case DRAW_LABEL_STRUCT_TYPE:
		case DRAW_TEXT_STRUCT_TYPE:
			Text = ((DrawTextStruct*)Struct)->m_Text;
			Size = ((DrawTextStruct*)Struct)->m_Size;
			Orient = ((DrawTextStruct*)Struct)->m_Orient;
			Shape = ((DrawTextStruct*)Struct)->m_Shape;
			pX = ((DrawTextStruct*)Struct)->m_Pos.x;
			pY = ((DrawTextStruct*)Struct)->m_Pos.y;
			offset = TXTMARGE;
			if ( Struct->Type() == DRAW_GLOBAL_LABEL_STRUCT_TYPE ||
			   	 Struct->Type() == DRAW_HIER_LABEL_STRUCT_TYPE )
				offset += Size.x;	// We must draw the Glabel graphic symbol
			if ( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
				color = ReturnLayerColor(((DrawTextStruct*)Struct)->m_Layer);
			break;

		default: return;
		}

	if(Size.x == 0 ) Size = wxSize(DEFAULT_SIZE_TEXT, DEFAULT_SIZE_TEXT);

	SetCurrentLineWidth(-1);

	switch(Orient)
		{
		case 0:		/* Orientation horiz normale */
			if( Struct->Type() == DRAW_GLOBAL_LABEL_STRUCT_TYPE ||
			  	Struct->Type() == DRAW_HIER_LABEL_STRUCT_TYPE )
				PlotGraphicText(g_PlotFormat, wxPoint(pX - offset, pY),
						color, Text, TEXT_ORIENT_HORIZ, Size,
						GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER);
			else
				PlotGraphicText(g_PlotFormat, wxPoint(pX, pY - offset),
						color, Text, TEXT_ORIENT_HORIZ, Size,
						GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM);
			break;

		case 1:		/* Orientation vert UP */
			if( Struct->Type() == DRAW_GLOBAL_LABEL_STRUCT_TYPE ||
				Struct->Type() == DRAW_HIER_LABEL_STRUCT_TYPE )
				PlotGraphicText(g_PlotFormat, wxPoint(pX, pY + offset),
						color, Text, TEXT_ORIENT_VERT, Size,
						GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP);
			else
				PlotGraphicText( g_PlotFormat, wxPoint(pX - offset, pY),
						color, Text, TEXT_ORIENT_VERT, Size,
						GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_BOTTOM);
			break;

		case 2:		/* Horiz Orientation - Right justified */
			if( Struct->Type() == DRAW_GLOBAL_LABEL_STRUCT_TYPE ||
				Struct->Type() == DRAW_HIER_LABEL_STRUCT_TYPE )
				PlotGraphicText(g_PlotFormat, wxPoint(pX + offset, pY),
						color, Text, TEXT_ORIENT_HORIZ, Size,
						GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER);
			else
				PlotGraphicText(g_PlotFormat, wxPoint(pX, pY + offset),
						color, Text, TEXT_ORIENT_HORIZ, Size,
						GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_BOTTOM);
			break;

		case 3:		/* Orientation vert BOTTOM */
			if( Struct->Type() == DRAW_GLOBAL_LABEL_STRUCT_TYPE ||
				Struct->Type() == DRAW_HIER_LABEL_STRUCT_TYPE )
				PlotGraphicText(g_PlotFormat, wxPoint(pX, pY - offset),
						color, Text, TEXT_ORIENT_VERT, Size,
						GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM);
			else
				PlotGraphicText(g_PlotFormat, wxPoint(pX + offset, pY),
						color, Text, TEXT_ORIENT_VERT, Size,
						GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_TOP);
			break;
		}

	/* Trace du symbole associe au label global */
	if( Struct->Type() == DRAW_GLOBAL_LABEL_STRUCT_TYPE ||
		Struct->Type() == DRAW_HIER_LABEL_STRUCT_TYPE )
	{
		int jj, imax;
		HalfSize = Size.x / 2;
		Template = TemplateShape[Shape][Orient];
		imax = *Template; Template++;

		for ( ii = 0, jj = 0; ii < imax ; ii++ )
		{
			Poly[jj] = ( HalfSize * (*Template) ) + pX;
			jj ++; Template++;
			Poly[jj] = ( HalfSize * (*Template) ) + pY;
			jj ++; Template++;
		}
		PlotPoly(imax,Poly,NOFILL );
	}
}

/***********************************************************/
static void PlotSheetLabelStruct(DrawSheetLabelStruct *Struct)
/***********************************************************/
/* Routine de dessin des Sheet Labels type hierarchie */
{
int side, txtcolor = -1;
int posx , tposx, posy, size, size2;
int coord[16];

	if ( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
		txtcolor = ReturnLayerColor(Struct->m_Layer);
	
	posx = Struct->m_Pos.x; posy = Struct->m_Pos.y; size = Struct->m_Size.x;
	if( Struct->m_Edge )
		{
		tposx = posx - size;
		side = GR_TEXT_HJUSTIFY_RIGHT;
		}
	else {
		tposx = posx + size + (size /8) ;
		side = GR_TEXT_HJUSTIFY_LEFT;
		}
	PlotGraphicText(g_PlotFormat, wxPoint(tposx, posy), txtcolor,
				Struct->m_Text, TEXT_ORIENT_HORIZ, wxSize(size,size),
				side, GR_TEXT_VJUSTIFY_CENTER);
	/* dessin du symbole de connexion */

	if(Struct->m_Edge) size = -size;
	coord[0] = posx; coord[1] = posy; size2 = size /2;
	switch(Struct->m_Shape)
	{
		case 0:		/* input |> */
			coord[2] = posx ; coord[3] = posy - size2;
			coord[4] = posx + size2; coord[5] = posy - size2;
			coord[6] = posx + size; coord[7] = posy;
			coord[8] = posx + size2; coord[9] = posy + size2;
			coord[10] = posx ; coord[11] = posy + size2;
			coord[12] = posx; coord[13] = posy;
			PlotPoly(7, coord, NOFILL);
			break;

		case 1:		/* output <| */
			coord[2] = posx + size2; coord[3] = posy - size2;
			coord[4] = posx + size; coord[5] = posy - size2;
			coord[6] = posx + size; coord[7] = posy + size2;
			coord[8] = posx + size2; coord[9] = posy + size2;
			coord[10] = posx; coord[11] = posy;
			PlotPoly(6, coord, NOFILL);
			break;

		case 2:		/* bidi <> */
		case 3:		/* TriSt <> */
			coord[2] = posx + size2; coord[3] = posy - size2;
			coord[4] = posx + size; coord[5] = posy;
			coord[6] = posx + size2; coord[7] = posy +size2;
			coord[8] = posx; coord[9] = posy;
			PlotPoly(5, coord, NOFILL);
			break;

		default:	 /* unsp []*/
			coord[2] = posx ; coord[3] = posy - size2;
			coord[4] = posx + size; coord[5] = posy - size2;
			coord[6] = posx + size; coord[7] = posy + size2;
			coord[8] = posx ; coord[9] = posy + size2;
			coord[10] = posx; coord[11] = posy;
			PlotPoly(6, coord, NOFILL);
			break;
	}
}

/*************************************************/
void PlotSheetStruct(DrawSheetStruct *Struct)
/*************************************************/
/* Routine de dessin du bloc type hierarchie */
{
DrawSheetLabelStruct * SheetLabelStruct;
int txtcolor = -1;
wxSize size;
wxString Text;
wxPoint pos;

	if ( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
		SetColorMapPS ( ReturnLayerColor(Struct->m_Layer) );

	SetCurrentLineWidth(-1);

	Move_Plume(Struct->m_Pos, 'U');
	pos = Struct->m_Pos; pos.x += Struct->m_Size.x;
	Move_Plume(pos,'D');
	pos.y += Struct->m_Size.y;
	Move_Plume(pos, 'D');
	pos = Struct->m_Pos; pos.y += Struct->m_Size.y;
	Move_Plume(pos,'D');
	Move_Plume(Struct->m_Pos, 'D');
	Plume('U');

	/* Trace des textes : SheetName */
	Text = Struct->m_SheetName;
	size = wxSize(Struct->m_SheetNameSize,Struct->m_SheetNameSize);
	pos = Struct->m_Pos; pos.y -= 4;
	if ( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
		SetColorMapPS ( ReturnLayerColor(LAYER_SHEETNAME) );
	PlotGraphicText(g_PlotFormat, pos, txtcolor,
			  Text, TEXT_ORIENT_HORIZ, size,
			  GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM);

	/* Trace des textes : FileName */
	Text = Struct->m_FileName;
	size = wxSize(Struct->m_FileNameSize,Struct->m_FileNameSize);
	if ( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
		SetColorMapPS ( ReturnLayerColor(LAYER_SHEETFILENAME) );
	PlotGraphicText(g_PlotFormat, wxPoint(Struct->m_Pos.x, Struct->m_Pos.y + Struct->m_Size.y + 4),
			txtcolor,
			Text, TEXT_ORIENT_HORIZ, size,
			GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP);

	/* Trace des textes : SheetLabel */
	SheetLabelStruct = Struct->m_Label;
	if ( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
		SetColorMapPS ( ReturnLayerColor(Struct->m_Layer) );
	while( SheetLabelStruct != NULL )
	{
		PlotSheetLabelStruct(SheetLabelStruct);
		SheetLabelStruct = (DrawSheetLabelStruct*)(SheetLabelStruct->Pnext);
	}
}

