	/****************************************/
	/* Modules to handle component drawing. */
	/****************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "component_class.h"
#include "general.h"
#include "trigo.h"
#include "protos.h"

#define UNVISIBLE_COLOR DARKGRAY

//#define DRAW_ARC_WITH_ANGLE		// Used to draw arcs


/* Fonctions locales */

/* Descr component <DUMMY> used when a component is not found in library,
 to draw a dummy shape*/
/*
This component is a 400 mils square with the text ??
DEF DUMMY U 0 40 Y Y 1 0 N
F0 "U" 0 -350 60 H V
F1 "DUMMY" 0 350 60 H V
DRAW
T 0 0 0 150 0 0 0 ??
S -200 200 200 -200 0 1 0
ENDDRAW
ENDDEF
*/

static int s_ItemSelectColor = BROWN;

static EDA_LibComponentStruct * DummyCmp;
static int * Buf_Poly_Drawings, Buf_Poly_Size;	// Used fo polyline drawings
static void DrawLibPartAux(WinEDA_DrawPanel * panel, wxDC * DC,
				EDA_SchComponentStruct *Component,
				EDA_LibComponentStruct *Entry,
				const wxPoint & Pos,
				int TransMat[2][2],
				int Multi, int convert,
				int DrawMode, int Color = -1, bool DrawPinText = TRUE);

/******************************/
static void CreateDummyCmp(void)
/******************************/
{
	DummyCmp = new EDA_LibComponentStruct( NULL);

	LibDrawSquare * Square = new LibDrawSquare();
	Square->m_Pos = wxPoint(- 200,200);
	Square->m_End = wxPoint(200, - 200);
	Square->m_Width = 4;

	LibDrawText * Text = new LibDrawText();
	Text->m_Size.x = Text->m_Size.y = 150;
	Text->m_Text = wxT("??");

	DummyCmp->m_Drawings = Square;
	Square->Pnext = Text;
}


/*************************************************************/
void DrawLibEntry(WinEDA_DrawPanel * panel,wxDC * DC,
							EDA_LibComponentStruct *LibEntry,
							int posX, int posY,
							int Multi, int convert,
							int DrawMode, int Color)
/**************************************************************/
/* Routine de dessin d'un composant d'une librairie
	LibEntry = pointeur sur la description en librairie
	posX, posY = position du composant
	DrawMode = GrOR ..
	Color = 0 : dessin en vraies couleurs, sinon couleur = Color

	Une croix symbolise le point d'accrochage (ref position) du composant

	Le composant est toujours trace avec orientation 0
*/
{
int color;
int TransMat[2][2];
wxString Prefix;
LibDrawField * Field;
wxPoint text_pos;

	/* Orientation normale */
	TransMat[0][0] = 1; TransMat[1][1] = -1;
	TransMat[1][0] = TransMat[0][1] = 0;

	DrawLibPartAux(panel, DC, NULL, LibEntry, wxPoint(posX, posY),
			TransMat, Multi,
			convert, DrawMode, Color);

	/* Trace des 2 champs ref et value (Attention aux coord: la matrice
	de transformation change de signe les coord Y */

	GRSetDrawMode(DC, DrawMode);

	if( LibEntry->m_Prefix.m_Attributs & TEXT_NO_VISIBLE )
		{
		if( Color >= 0 ) color = Color;
		else color = UNVISIBLE_COLOR;
		}

	else {
		if( Color >= 0) color = Color;
		else color = ReturnLayerColor(LAYER_REFERENCEPART);
		}

	if (LibEntry->m_UnitCount > 1)
		Prefix.Printf( wxT("%s?%c"),LibEntry->m_Prefix.m_Text.GetData(),Multi + 'A' - 1);
	else Prefix = LibEntry->m_Prefix.m_Text + wxT("?");

	text_pos.x = LibEntry->m_Prefix.m_Pos.x + posX;
	text_pos.y = posY - LibEntry->m_Prefix.m_Pos.y;
int LineWidth = MAX(LibEntry->m_Prefix.m_Width, g_DrawMinimunLineWidth);
	if ( (LibEntry->m_Prefix.m_Flags & IS_MOVED) == 0 )
		DrawGraphicText(panel, DC, text_pos,
				color,LibEntry->m_Prefix.m_Text.GetData(),
				LibEntry->m_Prefix.m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
				LibEntry->m_Prefix.m_Size,
				LibEntry->m_Prefix.m_HJustify, LibEntry->m_Prefix.m_VJustify, LineWidth);

	if( LibEntry->m_Name.m_Attributs & TEXT_NO_VISIBLE )
		{
		if( Color >= 0) color = Color;
		else color = UNVISIBLE_COLOR;
		}

	else {
		if( Color >= 0 ) color = Color;
		else color = ReturnLayerColor(LAYER_VALUEPART);
		}

	text_pos.x = LibEntry->m_Name.m_Pos.x + posX;
	text_pos.y = posY - LibEntry->m_Name.m_Pos.y;
	LineWidth = MAX(LibEntry->m_Name.m_Width, g_DrawMinimunLineWidth);
	if ( (LibEntry->m_Name.m_Flags & IS_MOVED) == 0 )
		DrawGraphicText(panel, DC, text_pos,
					color, LibEntry->m_Name.m_Text.GetData(),
					LibEntry->m_Name.m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
					LibEntry->m_Name.m_Size,
					LibEntry->m_Name.m_HJustify, LibEntry->m_Name.m_VJustify, LineWidth);

	for( Field = LibEntry->Fields; Field != NULL; Field = (LibDrawField *)Field->Pnext )
	{
		if( Field->m_Text.IsEmpty() ) return;
		if ( (Field->m_Flags & IS_MOVED) != 0 ) continue;
		if( Field->m_Attributs & TEXT_NO_VISIBLE )
		{
			if( Color >= 0) color = Color;
			else color = UNVISIBLE_COLOR;
		}
		else
		{
			if( Color >= 0) color = Color;
			else color = ReturnLayerColor(LAYER_FIELDS);
		}

		text_pos.x = Field->m_Pos.x + posX;
		text_pos.y = posY - Field->m_Pos.y;
		LineWidth = MAX(Field->m_Width, g_DrawMinimunLineWidth);
		DrawGraphicText(panel, DC, text_pos,
					color, Field->m_Text,
					Field->m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
					Field->m_Size,
					Field->m_HJustify, Field->m_VJustify, LineWidth);
	}

	// Tracé de l'ancre
	int len = 3 * panel->GetZoom();
	GRLine(&panel->m_ClipBox, DC, posX, posY - len, posX, posY + len, 0, color);
	GRLine(&panel->m_ClipBox, DC, posX - len, posY, posX + len, posY, 0, color);

}

/*****************************************************************************
* Routine to draw the given part at given position, transformed/mirror as	 *
* specified, and in the given drawing mode. Only this one is visible...		 *
*****************************************************************************/
void EDA_SchComponentStruct::Draw(WinEDA_DrawPanel * panel,wxDC * DC,
			const wxPoint & offset, int DrawMode, int Color)
{
EDA_LibComponentStruct *Entry;
int ii;
bool dummy = FALSE;

	if( (Entry = FindLibPart(m_ChipName.GetData(),wxEmptyString,FIND_ROOT)) == NULL)
	{	/* composant non trouvé, on affiche un composant "dummy" */
		dummy = TRUE;
		if( DummyCmp == NULL ) CreateDummyCmp();
		Entry = DummyCmp;
	}

	DrawLibPartAux(panel, DC, this, Entry, m_Pos + offset,
					m_Transform,
					dummy ? 0 : m_Multi,
					dummy ? 0 : m_Convert,
					DrawMode);

	/* Trace des champs, avec placement et orientation selon orient. du
		composant
	*/

	if( ((m_Field[REFERENCE].m_Attributs & TEXT_NO_VISIBLE) == 0)
		&& ! (m_Field[REFERENCE].m_Flags & IS_MOVED) )
		{
		if ( Entry->m_UnitCount > 1 )
			DrawTextField(panel, DC, &m_Field[REFERENCE],1,DrawMode);
		else
			DrawTextField(panel, DC, &m_Field[REFERENCE],0,DrawMode);
		}

	for( ii = VALUE; ii < NUMBER_OF_FIELDS; ii++ )
		{
		if (m_Field[ii].m_Flags & IS_MOVED) continue;
		DrawTextField(panel, DC, &m_Field[ii],0,DrawMode);
		}
}

/***********************************************************/
void DrawTextField(WinEDA_DrawPanel * panel,wxDC * DC,
		PartTextStruct * Field, int IsMulti, int DrawMode)
/***********************************************************/
/* Routine de trace des textes type Field du composant.
	entree:
		IsMulti: flag Non Null si il y a plusieurs parts par boitier.
				n'est utile que pour le champ reference pour ajouter a celui ci
				l'identification de la part ( A, B ... )
		DrawMode: mode de trace
*/
{
int orient, color;
wxPoint pos;		/* Position des textes */
EDA_SchComponentStruct *DrawLibItem = (EDA_SchComponentStruct *) Field->m_Parent;
int hjustify, vjustify;
int LineWidth = MAX(Field->m_Width, g_DrawMinimunLineWidth);

	if( Field->m_Attributs & TEXT_NO_VISIBLE ) return;
	if( Field->IsVoid() ) return;
		
	GRSetDrawMode(DC, DrawMode);

	/* Calcul de la position des textes, selon orientation du composant */
	orient = Field->m_Orient;
	hjustify = Field->m_HJustify; vjustify = Field->m_VJustify;
	pos.x = Field->m_Pos.x - DrawLibItem->m_Pos.x;
	pos.y = Field->m_Pos.y - DrawLibItem->m_Pos.y;

	pos = DrawLibItem->GetScreenCoord(pos);
	pos.x += DrawLibItem->m_Pos.x;
	pos.y += DrawLibItem->m_Pos.y;

	/* Y a t-il rotation (pour l'orientation, la justification)*/
	if(DrawLibItem->m_Transform[0][1])	// Rotation du composant de 90deg
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

	if( Field->m_FieldId == REFERENCE )
		color = ReturnLayerColor(LAYER_REFERENCEPART);
	else if( Field->m_FieldId == VALUE )
		color = ReturnLayerColor(LAYER_VALUEPART);
	else color = ReturnLayerColor(LAYER_FIELDS);
	if( !IsMulti || (Field->m_FieldId != REFERENCE) )
	{
		DrawGraphicText(panel, DC, pos, color, Field->m_Text.GetData(),
						orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
						Field->m_Size,
						hjustify, vjustify, LineWidth);
	}

	else /* Le champ est la reference, et il y a plusieurs parts par boitier */
		{/* On ajoute alors A ou B ... a la reference */
		wxString fulltext = Field->m_Text;
		fulltext.Append('A' - 1 + DrawLibItem->m_Multi);
		DrawGraphicText(panel, DC, pos, color, fulltext.GetData(),
						orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
						Field->m_Size,
						hjustify, vjustify, LineWidth);
		}
}


/********************************************************************************/
EDA_LibComponentStruct *FindLibPart(const wxChar *Name, const wxString & LibName, int Alias)
/********************************************************************************/
/*
 Routine to find a part in one of the libraries given its name.
	Name = Name of part.
	LibName = Name of Lib; if "": seach in all libs
	Alias = Flag: si flag != 0, retourne un pointeur sur une part ou un alias
				  si flag = 0, retourne un pointeur sur une part meme si le nom
				  correspond a un alias
	Alias = FIND_ROOT, ou Alias = FIND_ALIAS
*/
{
EDA_LibComponentStruct *Entry;
static EDA_LibComponentStruct DummyEntry(wxEmptyString);  /* Used only to call PQFind. */
LibraryStruct *Lib = g_LibraryList;

	DummyEntry.m_Drawings = NULL;   /* Used only to call PQFind. */
	DummyEntry.m_Name.m_Text = Name;

	PQCompFunc((PQCompFuncType) LibraryEntryCompare);

	Entry = NULL; FindLibName.Empty();
	while (Lib)
	{
		if( ! LibName.IsEmpty() )
		{
			if( Lib->m_Name != LibName )
			{
				Lib = Lib->m_Pnext; continue ;
			}
		}
		if( Lib == NULL ) break;
		Entry = (EDA_LibComponentStruct*)PQFind(Lib->m_Entries, &DummyEntry);
		if( Entry != NULL)
		{
			FindLibName = Lib->m_Name;
			break;
		}
		Lib = Lib->m_Pnext;
	}

	/* Si le nom est un alias, recherche du vrai composant */
	if( Entry )
		{
		if( (Entry->Type != ROOT ) && (Alias == FIND_ROOT) )
			Entry = FindLibPart( ((EDA_LibCmpAliasStruct*)Entry)->m_RootName.GetData() ,
									Lib->m_Name, FIND_ROOT);
		}

	return (Entry);
}

/*****************************************************************************
* Routine to draw the given part at given position, transformed/mirror as
* specified, and in the given drawing mode.
* if Color < 0: Draw in normal color
* else draw  in color = Color
*****************************************************************************/
/* DrawMode  = GrXOR, GrOR ..*/
void DrawLibPartAux(WinEDA_DrawPanel * panel,wxDC * DC,
					EDA_SchComponentStruct *Component,
					EDA_LibComponentStruct *Entry,
					const wxPoint & Pos,
					int TransMat[2][2],
					int Multi, int convert, int DrawMode,
					int Color, bool DrawPinText)
{
int i, x1, y1, x2, y2, t1, t2, orient;
LibEDA_BaseStruct *DEntry = NULL;
int CharColor;
int fill_option;
int SetHightColor;
int LineWidth;
//#define GETCOLOR(l) Color < 0 ? (ReturnLayerColor(l)| SetHightColor) : Color;
#define GETCOLOR(l) Color < 0 ? SetHightColor ? s_ItemSelectColor : (ReturnLayerColor(l)| SetHightColor) : Color;
	
	if (Entry->m_Drawings == NULL) return;
	GRSetDrawMode(DC, DrawMode);

	for( DEntry = Entry->m_Drawings; DEntry != NULL;DEntry = DEntry->Next())
	{
		/* Elimination des elements non relatifs a l'unite */
		if( Multi && DEntry->m_Unit && (DEntry->m_Unit != Multi) ) continue;
		if( convert && DEntry->m_Convert && (DEntry->m_Convert != convert) )
					continue;

		if ( DEntry->m_Flags & IS_MOVED ) continue; // Element en deplacement non trace
		SetHightColor = (DEntry->m_Selected & IS_SELECTED) ? HIGHT_LIGHT_FLAG : 0;
		LineWidth = MAX(DEntry->m_Width, g_DrawMinimunLineWidth);
		switch (DEntry->m_StructType)
		{
			case COMPONENT_ARC_DRAW_TYPE:
				{
				int xc,yc, x2, y2;
				LibDrawArc * Arc = (LibDrawArc *) DEntry;
				CharColor = GETCOLOR(LAYER_DEVICE);
				xc = Pos.x + TransMat[0][0] * Arc->m_Pos.x +
							  TransMat[0][1] * Arc->m_Pos.y;
				yc = Pos.y + TransMat[1][0] * Arc->m_Pos.x +
							  TransMat[1][1] * Arc->m_Pos.y;
				x2 = Pos.x + TransMat[0][0] * Arc->m_ArcStart.x +
							  TransMat[0][1] * Arc->m_ArcStart.y;;
				y2 = Pos.y + TransMat[1][0] * Arc->m_ArcStart.x +
							  TransMat[1][1] * Arc->m_ArcStart.y;
				x1 = Pos.x + TransMat[0][0] * Arc->m_ArcEnd.x +
							  TransMat[0][1] * Arc->m_ArcEnd.y;;
				y1 = Pos.y + TransMat[1][0] * Arc->m_ArcEnd.x +
							  TransMat[1][1] * Arc->m_ArcEnd.y;
				t1 = Arc->t1; t2 = Arc->t2;
				bool swap = MapAngles(&t1, &t2, TransMat);
				if ( swap ) { EXCHG(x1,x2); EXCHG(y1, y2) }
				fill_option = Arc->m_Fill & (~g_PrintFillMask);
 				if ( Color < 0 )	// Normal Color Layer
					{
					if ( (fill_option == FILLED_WITH_BG_BODYCOLOR) && ! g_IsPrinting )
						GRFilledArc(&panel->m_ClipBox, DC, xc, yc, t1, t2,
								Arc->m_Rayon, Arc->m_Width, CharColor,
								ReturnLayerColor(LAYER_DEVICE_BACKGROUND));
					else if ( fill_option == FILLED_SHAPE)
						GRFilledArc(&panel->m_ClipBox, DC, xc, yc, t1, t2,
								Arc->m_Rayon, CharColor, CharColor);
#ifdef DRAW_ARC_WITH_ANGLE
					else GRArc(&panel->m_ClipBox, DC, xc, yc, t1, t2,
								Arc->m_Rayon, LineWidth, CharColor);
#else
					else GRArc1(&panel->m_ClipBox, DC, x1, y1, x2, y2,
								xc, yc, LineWidth, CharColor);
#endif
					}
#ifdef DRAW_ARC_WITH_ANGLE
				else GRArc(&panel->m_ClipBox, DC, xc, yc, t1, t2,
								Arc->m_Rayon, Circle->m_Width, CharColor);
#else
				else GRArc1(&panel->m_ClipBox, DC, x1, y1, x2, y2,
								xc, yc, Arc->m_Width, CharColor);
#endif
				}
				break;

			case COMPONENT_CIRCLE_DRAW_TYPE:
				{
				LibDrawCircle * Circle = (LibDrawCircle *) DEntry;
				CharColor = GETCOLOR(LAYER_DEVICE);
				x1 = Pos.x + TransMat[0][0] * Circle->m_Pos.x +
								 TransMat[0][1] * Circle->m_Pos.y;
				y1 = Pos.y + TransMat[1][0] * Circle->m_Pos.x +
								 TransMat[1][1] * Circle->m_Pos.y;
				fill_option = Circle->m_Fill & (~g_PrintFillMask);
				if ( Color < 0 )
					{
					if ( (fill_option == FILLED_WITH_BG_BODYCOLOR) && ! g_IsPrinting )
						GRFilledCircle(&panel->m_ClipBox, DC, x1, y1,
								Circle->m_Rayon, LineWidth, CharColor,
								ReturnLayerColor(LAYER_DEVICE_BACKGROUND));
					else if ( fill_option == FILLED_SHAPE)
						GRFilledCircle(&panel->m_ClipBox, DC, x1, y1,
								Circle->m_Rayon, 0, CharColor, CharColor);
					else GRCircle(&panel->m_ClipBox, DC, x1, y1,
						 Circle->m_Rayon, LineWidth, CharColor);
					}
				else GRCircle(&panel->m_ClipBox, DC, x1, y1,
						 Circle->m_Rayon, LineWidth, CharColor);
				}
				break;

			case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
				{
				LibDrawText * Text = (LibDrawText *) DEntry;
				CharColor = GETCOLOR(LAYER_DEVICE);

				/* The text orientation may need to be flipped if the
				transformation matrix cuases xy axes to be flipped. */
				t1 = (TransMat[0][0] != 0) ^ (Text->m_Horiz != 0);
				x1 = Pos.x + TransMat[0][0] * Text->m_Pos.x
						  + TransMat[0][1] * Text->m_Pos.y;
				y1 = Pos.y + TransMat[1][0] * Text->m_Pos.x
						  + TransMat[1][1] * Text->m_Pos.y;
				DrawGraphicText(panel, DC, wxPoint(x1, y1), CharColor, Text->m_Text,
						t1 ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT,
						Text->m_Size,
						GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, LineWidth);
				}
				break;

			case COMPONENT_RECT_DRAW_TYPE:
				{
				LibDrawSquare * Square = (LibDrawSquare *) DEntry;
				CharColor = GETCOLOR(LAYER_DEVICE);

				x1 = Pos.x + TransMat[0][0] * Square->m_Pos.x
						  + TransMat[0][1] * Square->m_Pos.y;
				y1 = Pos.y + TransMat[1][0] * Square->m_Pos.x
						  + TransMat[1][1] * Square->m_Pos.y;
				x2 = Pos.x + TransMat[0][0] * Square->m_End.x
						  + TransMat[0][1] * Square->m_End.y;
				y2 = Pos.y + TransMat[1][0] * Square->m_End.x
						 + TransMat[1][1] * Square->m_End.y;
				fill_option = Square->m_Fill & (~g_PrintFillMask);
				if ( Color < 0 )
					{
					if ( (fill_option == FILLED_WITH_BG_BODYCOLOR) && ! g_IsPrinting )
						GRFilledRect(&panel->m_ClipBox, DC, x1, y1, x2, y2,
							CharColor, LineWidth, 
							ReturnLayerColor(LAYER_DEVICE_BACKGROUND));
					else if ( fill_option == FILLED_SHAPE)
						GRFilledRect(&panel->m_ClipBox, DC, x1, y1, x2, y2,
							CharColor, CharColor);
					else GRRect(&panel->m_ClipBox, DC, x1, y1, x2, y2,
							LineWidth, CharColor);
					}
				else GRRect(&panel->m_ClipBox, DC, x1, y1, x2, y2,
							LineWidth, CharColor);
				}
				break;

			case COMPONENT_PIN_DRAW_TYPE: /* Trace des Pins */
				{
				LibDrawPin * Pin = (LibDrawPin *) DEntry;
				if(Pin->m_Attributs & PINNOTDRAW)
					{
					if( (ActiveScreen->m_Type == SCHEMATIC_FRAME) &&
						!g_ShowAllPins )
						break;
					}
				/* Calcul de l'orientation reelle de la Pin */
				orient = Pin->ReturnPinDrawOrient(TransMat);

				/* Calcul de la position du point de reference */
				x2 = Pos.x + (TransMat[0][0] * Pin->m_Pos.x)
							+ (TransMat[0][1] * Pin->m_Pos.y);
				y2 = Pos.y + (TransMat[1][0] * Pin->m_Pos.x)
							+ (TransMat[1][1] * Pin->m_Pos.y);

				/* Dessin de la pin et du symbole special associe */
				CharColor = GETCOLOR(LAYER_PIN);
				Pin->DrawPinSymbol(panel, DC, wxPoint(x2, y2) , orient, DrawMode, CharColor );

				if ( DrawPinText )
				{
					wxPoint pinpos(x2,y2);
					CharColor = SetHightColor ? s_ItemSelectColor : Color;
					Pin->DrawPinTexts(panel, DC, pinpos, orient,
									Entry->m_TextInside,
									Entry->m_DrawPinNum,Entry->m_DrawPinName,
									CharColor, DrawMode);
				}
				}
				break;

			case COMPONENT_POLYLINE_DRAW_TYPE:
				{
				LibDrawPolyline * polyline = (LibDrawPolyline *) DEntry;
				CharColor = GETCOLOR(LAYER_DEVICE);
				if ( Buf_Poly_Drawings == NULL )
				{
					Buf_Poly_Size = polyline->n;
					Buf_Poly_Drawings = (int *) MyMalloc(sizeof(int) * 2 * Buf_Poly_Size);
				}
				else if ( Buf_Poly_Size < polyline->n )
				{
					Buf_Poly_Size = polyline->n;
					Buf_Poly_Drawings = (int *) realloc(Buf_Poly_Drawings,
							sizeof(int) * 2 * Buf_Poly_Size);
				}
				for (i = 0; i < polyline->n; i++)
					{
					Buf_Poly_Drawings[i * 2] = Pos.x +
							 TransMat[0][0] * polyline->PolyList[i * 2] +
							 TransMat[0][1] * polyline->PolyList[i * 2 + 1];
					Buf_Poly_Drawings[i * 2 + 1] = Pos.y +
							 TransMat[1][0] * polyline->PolyList[i * 2] +
							 TransMat[1][1] * polyline->PolyList[i * 2 + 1];
					}
				fill_option = polyline->m_Fill & (~g_PrintFillMask);
				if ( Color < 0 )
				{
					if ( (fill_option == FILLED_WITH_BG_BODYCOLOR) && ! g_IsPrinting )
						GRPoly(&panel->m_ClipBox, DC, polyline->n,
							Buf_Poly_Drawings, 1, LineWidth, CharColor,
							ReturnLayerColor(LAYER_DEVICE_BACKGROUND));
					else if ( fill_option == FILLED_SHAPE )
						GRPoly(&panel->m_ClipBox, DC, polyline->n,
							Buf_Poly_Drawings, 1, LineWidth, CharColor, CharColor);
					else GRPoly(&panel->m_ClipBox, DC, polyline->n,
							Buf_Poly_Drawings, 0, LineWidth, CharColor, CharColor);
				}
				else GRPoly(&panel->m_ClipBox, DC, polyline->n,
					Buf_Poly_Drawings, 0, LineWidth, CharColor, CharColor);
				}
				break;

			default:
				wxBell();
				break;
		}	/* Fin Switch */
	}	/* Fin Boucle de dessin */
	if ( g_DebugLevel > 4 )	/* Draw the component boundary box */
	{
		EDA_Rect BoundaryBox;
		if ( Component ) BoundaryBox = Component->GetBoundaryBox();
		else BoundaryBox = Entry->GetBoundaryBox(Multi, convert);
		x1 = BoundaryBox.GetX();
		y1 = BoundaryBox.GetY();
		x2 = BoundaryBox.GetRight();
		y2 = BoundaryBox.GetBottom();
		GRRect(&panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN);
		BoundaryBox = Component->m_Field[REFERENCE].GetBoundaryBox();
		x1 = BoundaryBox.GetX();
		y1 = BoundaryBox.GetY();
		x2 = BoundaryBox.GetRight();
		y2 = BoundaryBox.GetBottom();
		GRRect(&panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN);
		BoundaryBox = Component->m_Field[VALUE].GetBoundaryBox();
		x1 = BoundaryBox.GetX();
		y1 = BoundaryBox.GetY();
		x2 = BoundaryBox.GetRight();
		y2 = BoundaryBox.GetBottom();
		GRRect(&panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN);
	}
}


/********************************************************************************/
void LibDrawPin::DrawPinSymbol(WinEDA_DrawPanel * panel, wxDC * DC,
				const wxPoint & pin_pos, int orient, int DrawMode, int Color)
/*******************************************************************************/

/* Draw the pin symbol (without texts)
	if Color != 0 draw with Color, eles with the normal pin color
*/
{
int MapX1, MapY1, x1, y1;
int color;
int width = MAX(m_Width, g_DrawMinimunLineWidth);
int posX = pin_pos.x, posY = pin_pos.y, len = m_PinLen;

 	
	if( Color >= 0) color = Color;
	else color = ReturnLayerColor(LAYER_PIN);
	GRSetDrawMode(DC, DrawMode);


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

	if( m_PinShape & INVERT)
	{
		GRCircle(&panel->m_ClipBox, DC, MapX1 * INVERT_PIN_RADIUS + x1,
				 MapY1 * INVERT_PIN_RADIUS + y1,
				 INVERT_PIN_RADIUS, width, color);

		GRMoveTo(MapX1 * INVERT_PIN_RADIUS * 2 + x1,
				 MapY1 * INVERT_PIN_RADIUS * 2 + y1);
		GRLineTo(&panel->m_ClipBox, DC, posX, posY, width, color);
	}

	else
	{
		GRMoveTo(x1, y1);
		GRLineTo(&panel->m_ClipBox, DC, posX, posY, width, color);
	}

	if(m_PinShape & CLOCK)
		{
		if(MapY1 == 0 ) /* MapX1 = +- 1 */
			{
			GRMoveTo(x1, y1 + CLOCK_PIN_DIM);
			GRLineTo(&panel->m_ClipBox, DC, x1 - MapX1 *  CLOCK_PIN_DIM, y1, width, color);
			GRLineTo(&panel->m_ClipBox, DC, x1, y1 - CLOCK_PIN_DIM, width, color);
			}
		else	/* MapX1 = 0 */
			{
			GRMoveTo(x1 + CLOCK_PIN_DIM, y1 );
			GRLineTo(&panel->m_ClipBox, DC, x1, y1 - MapY1 * CLOCK_PIN_DIM, width, color);
			GRLineTo(&panel->m_ClipBox, DC, x1 - CLOCK_PIN_DIM, y1, width, color);
			}
		}

	if(m_PinShape & LOWLEVEL_IN) /* IEEE symbol "Active Low Input" */
		{
		if(MapY1 == 0 ) /* MapX1 = +- 1 */
			{
			GRMoveTo(x1 + MapX1 * IEEE_SYMBOL_PIN_DIM*2, y1);
			GRLineTo(&panel->m_ClipBox, DC, x1 + MapX1 * IEEE_SYMBOL_PIN_DIM*2,
					y1 - IEEE_SYMBOL_PIN_DIM, width, color);
			GRLineTo(&panel->m_ClipBox, DC, x1, y1, width, color);
			}
		else	/* MapX1 = 0 */
			{
			GRMoveTo(x1, y1 + MapY1 * IEEE_SYMBOL_PIN_DIM*2);
			GRLineTo(&panel->m_ClipBox, DC, x1 - IEEE_SYMBOL_PIN_DIM,
						y1 + MapY1 * IEEE_SYMBOL_PIN_DIM*2, width, color);
			GRLineTo(&panel->m_ClipBox, DC, x1 , y1, width, color);
			}
		}


	if(m_PinShape & LOWLEVEL_OUT) /* IEEE symbol "Active Low Output" */
		{
		if(MapY1 == 0 ) /* MapX1 = +- 1 */
			{
			GRMoveTo(x1, y1 - IEEE_SYMBOL_PIN_DIM);
			GRLineTo(&panel->m_ClipBox, DC, x1 + MapX1 * IEEE_SYMBOL_PIN_DIM*2, y1, width, color);
			}
		else	/* MapX1 = 0 */
			{
			GRMoveTo(x1 - IEEE_SYMBOL_PIN_DIM, y1);
			GRLineTo(&panel->m_ClipBox, DC, x1 , y1 + MapY1 * IEEE_SYMBOL_PIN_DIM*2, width, color);
			}
		}

	/* Draw the pin end target (active end of the pin) */
	if ( ! g_IsPrinting )	// Draw but do not print the pin end target 1 pixel width */
		GRCircle(&panel->m_ClipBox, DC, posX,posY,TARGET_PIN_DIAM, 0, color);
}


/*****************************************************************************
* Routine to rotate the given angular direction by the given Transformation. *
* Input (and output) angles must be as follows:								 *
*	Unit is 0.1 degre														 *
* Angle1 in [0..3600], Angle2 > Angle1 in [0..7200]. Arc is assumed to be less *
* than 180.0 degrees.															 *
* Algorithm:																 *
* Map the angles to a point on the unit circle which is mapped using the	 *
* transform (only mirror and rotate so it remains on the unit circle) to	 *
* a new point which is used to detect new angle.							 *
*****************************************************************************/
bool MapAngles(int *Angle1, int *Angle2, int TransMat[2][2])
{
int Angle, Delta;
double x, y, t;
bool swap = FALSE;

	Delta = *Angle2 - *Angle1;
	if ( Delta >= 1800 )
	{
		*Angle1 -=1;
		*Angle2 +=1;
	}
	
	x = cos(*Angle1 * M_PI / 1800.0);
	y = sin(*Angle1 * M_PI / 1800.0);
	t = x * TransMat[0][0] + y * TransMat[0][1];
	y = x * TransMat[1][0] + y * TransMat[1][1];
	x = t;
	*Angle1 = (int) (atan2(y, x) * 1800.0 / M_PI + 0.5);

	x = cos(*Angle2 * M_PI / 1800.0);
	y = sin(*Angle2 * M_PI / 1800.0);
	t = x * TransMat[0][0] + y * TransMat[0][1];
	y = x * TransMat[1][0] + y * TransMat[1][1];
	x = t;
	*Angle2 = (int) (atan2(y, x) * 1800.0 / M_PI + 0.5);

	NORMALIZE_ANGLE(*Angle1);
	NORMALIZE_ANGLE(*Angle2);
	if (*Angle2 < *Angle1) *Angle2 += 3600;

	if (*Angle2 - *Angle1 > 1800)
		{			 /* Need to swap the two angles. */
		Angle = (*Angle1);
		*Angle1 = (*Angle2);
		*Angle2 = Angle;

		NORMALIZE_ANGLE(*Angle1);
		NORMALIZE_ANGLE(*Angle2);
		if (*Angle2 < *Angle1) *Angle2 += 3600;
		swap = TRUE;
		}

	if ( Delta >= 1800 )
	{
		*Angle1 +=1;
		*Angle2 -=1;
	}
	
	return swap;
}


/*****************************************************************************
* Routine to display an outline version of given library entry.				 *
* This routine is applied by the PlaceLibItem routine above.			 	 *
*****************************************************************************/
void DrawingLibInGhost(WinEDA_DrawPanel * panel, wxDC * DC,
					EDA_LibComponentStruct *LibEntry,
					EDA_SchComponentStruct *DrawLibItem, int PartX, int PartY,
					int multi, int convert, int Color, bool DrawPinText)
{
int DrawMode = g_XorMode;

	DrawLibPartAux(panel, DC, DrawLibItem, LibEntry, wxPoint(PartX, PartY),
					DrawLibItem->m_Transform,
					multi, convert, DrawMode, Color, DrawPinText);

}

/************************************************************/
/* Routine to draw One LibraryDrawStruct at given position, */
/* matrice de transformation  1 0 0 -1 (normale)			*/
/* DrawMode  = GrXOR, GrOR ..								*/
/************************************************************/
/* Utilise en LibEdit et Lib Browse */
void DrawLibraryDrawStruct(WinEDA_DrawPanel * panel, wxDC * DC,
					EDA_LibComponentStruct *LibEntry,
					int PartX, int PartY,
					LibEDA_BaseStruct *DrawItem, int Multi,
					int DrawMode, int Color)
{
int i, x1, y1, x2, y2, t1, t2, orient;
int CharColor;
int TransMat[2][2];
int fill_option;

#undef GETCOLOR
#define GETCOLOR(l) Color < 0 ? ReturnLayerColor(l) : Color;

	Multi = 0; 	/* unused */
	/* Trace de la structure */
	CharColor = GETCOLOR(LAYER_DEVICE);
	GRSetDrawMode(DC, DrawMode);

	TransMat[0][0] = 1;
	TransMat[0][1] = TransMat[1][0] = 0;
	TransMat[1][1] = -1;

	int LineWidth = MAX(DrawItem->m_Width, g_DrawMinimunLineWidth);

	switch (DrawItem->m_StructType)
		{
		case COMPONENT_ARC_DRAW_TYPE:
			{
			int xc,yc, x2,y2;
			LibDrawArc * Arc = (LibDrawArc *) DrawItem;
			t1 = Arc->t1; t2 = Arc->t2;
			bool swap = MapAngles(&t1, &t2, TransMat);
			xc = PartX + Arc->m_Pos.x;
			yc = PartY - Arc->m_Pos.y;
			x2 = PartX + Arc->m_ArcStart.x;
			y2 = PartY - Arc->m_ArcStart.y;
			x1 = PartX + Arc->m_ArcEnd.x;
			y1 = PartY - Arc->m_ArcEnd.y;
			
			if ( swap ) { EXCHG(x1,x2); EXCHG(y1, y2)}
			fill_option = Arc->m_Fill & (~g_PrintFillMask);
			if ( (Arc->m_Fill == FILLED_WITH_BG_BODYCOLOR) && ! g_IsPrinting )
				GRFilledArc(&panel->m_ClipBox, DC, xc, yc, t1, t2,
								Arc->m_Rayon, CharColor,
								ReturnLayerColor(LAYER_DEVICE_BACKGROUND));
			else if ( Arc->m_Fill == FILLED_SHAPE)
				GRFilledArc(&panel->m_ClipBox, DC, xc, yc, t1, t2,
								Arc->m_Rayon, LineWidth, CharColor, CharColor);
#ifdef DRAW_ARC_WITH_ANGLE
			else GRArc(&panel->m_ClipBox, DC, xc, yc, t1, t2,
								Arc->m_Rayon, CharColor);
#else
			else GRArc1(&panel->m_ClipBox, DC, x1, y1, x2, y2,
								xc, yc, LineWidth, CharColor);
#endif
			}
			break;

		case COMPONENT_CIRCLE_DRAW_TYPE:
			{
			LibDrawCircle * Circle = (LibDrawCircle *) DrawItem;
			x1 = PartX + Circle->m_Pos.x;
			y1 = PartY - Circle->m_Pos.y;
			fill_option = Circle->m_Fill & (~g_PrintFillMask);
			if ( (fill_option == FILLED_WITH_BG_BODYCOLOR) && ! g_IsPrinting )
				GRFilledCircle(&panel->m_ClipBox, DC, x1, y1,
								Circle->m_Rayon, LineWidth, CharColor,
								ReturnLayerColor(LAYER_DEVICE_BACKGROUND));
			else if ( fill_option == FILLED_SHAPE)
				GRFilledCircle(&panel->m_ClipBox, DC, x1, y1,
								Circle->m_Rayon, 0, CharColor, CharColor);
			else GRCircle(&panel->m_ClipBox, DC, x1, y1,
						 Circle->m_Rayon, LineWidth, CharColor);
			}
			break;

		case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
			{
			LibDrawText * Text = (LibDrawText *) DrawItem;
			x1 = PartX + Text->m_Pos.x;
			y1 = PartY - Text->m_Pos.y;
			DrawGraphicText(panel, DC, wxPoint(x1, y1), CharColor, Text->m_Text,
					Text->m_Horiz,
					Text->m_Size,
					GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, LineWidth);
			}
			break;

		case COMPONENT_RECT_DRAW_TYPE:
			{
			LibDrawSquare * Square = (LibDrawSquare *) DrawItem;
			x1 = PartX + Square->m_Pos.x;
			y1 = PartY - Square->m_Pos.y;
			x2 = PartX + Square->m_End.x;
			y2 = PartY - Square->m_End.y;
			fill_option = Square->m_Fill & (~g_PrintFillMask);
			if ( (fill_option == FILLED_WITH_BG_BODYCOLOR) && ! g_IsPrinting )
				GRFilledRect(&panel->m_ClipBox, DC, x1, y1, x2, y2,
							CharColor, LineWidth,
							ReturnLayerColor(LAYER_DEVICE_BACKGROUND));
			else if ( fill_option == FILLED_SHAPE)
				GRFilledRect(&panel->m_ClipBox, DC, x1, y1, x2, y2,
							CharColor, CharColor);
			else GRRect(&panel->m_ClipBox, DC, x1, y1, x2, y2, LineWidth,
							CharColor);
			}
			break;

		case COMPONENT_PIN_DRAW_TYPE: /* Trace des Pins */
			{
			LibDrawPin * Pin = (LibDrawPin *) DrawItem;
			x2 = PartX + Pin->m_Pos.x;
			y2 = PartY - Pin->m_Pos.y;
			/* Compute the real pin orientation, i.e. pin orient + component orient */
			orient = Pin->ReturnPinDrawOrient(TransMat);

			/* Dessin de la pin et du symbole special associe */
			if( Pin->m_Attributs & PINNOTDRAW) CharColor = DARKGRAY;
			else CharColor = -1;

			Pin->DrawPinSymbol(panel, DC, wxPoint(x2, y2), orient, DrawMode);
			wxPoint pinpos(x2,y2);
			Pin->DrawPinTexts(panel, DC, pinpos, orient,
							LibEntry->m_TextInside,
							LibEntry->m_DrawPinNum,LibEntry->m_DrawPinName,
							CharColor, DrawMode);
			}
			break;

		case COMPONENT_POLYLINE_DRAW_TYPE:
			{
			LibDrawPolyline * polyline = (LibDrawPolyline *) DrawItem;
			if ( Buf_Poly_Drawings == NULL )
			{
				Buf_Poly_Size = polyline->n;
				Buf_Poly_Drawings = (int *) MyMalloc(sizeof(int) * 2 * Buf_Poly_Size);
			}
			else if ( Buf_Poly_Size < polyline->n )
			{
				Buf_Poly_Size = polyline->n;
				Buf_Poly_Drawings = (int *) realloc(Buf_Poly_Drawings,
								sizeof(int) * 2 * Buf_Poly_Size);
			}
			for (i = 0; i < polyline->n; i++)
			{
				Buf_Poly_Drawings[i * 2] = PartX + polyline->PolyList[i * 2];
				Buf_Poly_Drawings[i * 2 + 1] = PartY - polyline->PolyList[i * 2 + 1];
			}
			fill_option = polyline->m_Fill & (~g_PrintFillMask);
			if ( (fill_option == FILLED_WITH_BG_BODYCOLOR) && ! g_IsPrinting )
				GRPoly(&panel->m_ClipBox, DC, polyline->n,
						Buf_Poly_Drawings, 1, LineWidth, CharColor,
						ReturnLayerColor(LAYER_DEVICE_BACKGROUND));
			else if ( fill_option == FILLED_SHAPE )
				GRPoly(&panel->m_ClipBox, DC, polyline->n,
							Buf_Poly_Drawings, 1, LineWidth, CharColor, CharColor);
			else GRPoly(&panel->m_ClipBox, DC, polyline->n,
						Buf_Poly_Drawings, 0, LineWidth, CharColor, CharColor);
			break;
			}
		}
}

