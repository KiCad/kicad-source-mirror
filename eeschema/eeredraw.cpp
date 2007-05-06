/*****************************************************************************
*	Program to draw EE diagrams.											 *
* This module redraw/draw all structs.										 *
*****************************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


char marq_bitmap[]=
	{
	12, 12, 0, 0,	/* Dimensions x et y, offsets x et y du bitmap de marqueurs*/
	YELLOW,			/* Couleur */
	1,1,1,1,1,1,1,1,0,0,0,0,	/* bitmap: >= 1 : color, 0 = notrace */
	1,1,1,0,1,0,1,1,0,0,0,0,
	1,1,1,1,0,0,0,1,0,0,0,0,
	1,0,1,1,1,0,0,0,0,0,0,0,
	1,1,0,1,1,1,0,0,0,0,0,0,
	1,1,0,0,1,1,1,0,0,0,0,0,
	1,1,1,0,0,1,1,1,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,
	0,0,0,0,0,0,0,1,1,1,0,0,
	0,0,0,0,0,0,0,0,1,1,1,0,
	0,0,0,0,0,0,0,0,0,1,1,1,
	0,0,0,0,0,0,0,0,0,0,1,0
	};

char marqERC_bitmap[]=
	{
	8, 8, 0, 0,	/* Dimensions x et y , offsets x et y du bitmap de marqueurs*/
	-1,			/* Color: -1 = couleur non précisée */
	1,1,1,1,1,0,0,0,
	1,1,1,0,1,0,0,0,
	1,1,1,1,0,0,0,0,
	1,0,1,1,1,0,0,0,
	1,1,0,1,1,1,0,0,
	0,0,0,0,1,1,1,0,
	0,0,0,0,0,1,1,1,
	0,0,0,0,0,0,1,0,
	};


static EDA_BaseStruct *HighLightStruct = NULL;

/************************************************************/
void  DrawDanglingSymbol(WinEDA_DrawPanel * panel,wxDC * DC,
			const wxPoint & pos, int Color)
/************************************************************/
{
#define DANGLING_SYMBOL_SIZE 12
	if ( ! g_IsPrinting )	// Draw but do not print the Dangling Symbol */
	{
		GRRect(&panel->m_ClipBox, DC,
			pos.x - DANGLING_SYMBOL_SIZE, pos.y - DANGLING_SYMBOL_SIZE,
			pos.x + DANGLING_SYMBOL_SIZE, pos.y + DANGLING_SYMBOL_SIZE,
			Color);
	}
}


/*************************************************/
void SetHighLightStruct(EDA_BaseStruct *HighLight)
/*************************************************/
{
	HighLightStruct = HighLight;
}

/**********************************************************************/
void WinEDA_SchematicFrame::RedrawActiveWindow(wxDC * DC, bool EraseBg)
/**********************************************************************/
/*
 Redraws only the active window which is assumed to be whole visible.
*/
{
wxString title;
	if( GetScreen() == NULL ) return;

	ActiveScreen = GetScreen();
	/* Forcage de la reinit de la brosse et plume courante */
	GRResetPenAndBrush(DC);
	DC->SetBackground(*wxBLACK_BRUSH);
	DC->SetBackgroundMode(wxTRANSPARENT);

	DrawPanel->CursorOff(DC); // effacement curseur

	if(DrawPanel->ManageCurseur)
	{
		DrawPanel->ManageCurseur(DrawPanel, DC, FALSE);
	}

	if ( EraseBg ) DrawPanel->EraseScreen(DC);

	DrawPanel->DrawBackGround(DC);

	RedrawStructList(DrawPanel, DC, GetScreen()->EEDrawList, GR_DEFAULT_DRAWMODE);

	TraceWorkSheet(DC, GetScreen());

	DrawPanel->CursorOn(DC); // reaffichage curseur
	if(DrawPanel->ManageCurseur)
	{
		DrawPanel->ManageCurseur(DrawPanel, DC, FALSE);
	}

	Affiche_Status_Box();
	GetScreen()->ClrRefreshReq();
	if( GetScreen()->m_FileName == g_DefaultSchematicFileName )
	{
		wxString msg = g_Main_Title + wxT(" ") + GetBuildVersion();
		title.Printf( wxT("%s [%s]"), msg.GetData(),GetScreen()->m_FileName.GetData());
		SetTitle(title);
	}
	else
	{
		title.Printf( wxT("[%s]"), GetScreen()->m_FileName.GetData());
		SetTitle(title);
	}

}

/*******************************************************************************/
void WinEDA_DrawPanel::PrintPage(wxDC * DC, bool Print_Sheet_Ref, int PrintMask)
/*******************************************************************************/
{
BASE_SCREEN * screen, * oldscreen = m_Parent->GetScreen();

	wxBeginBusyCursor();
		

	screen = m_Parent->m_CurrentScreen = ActiveScreen;
	RedrawStructList(this,DC, screen->EEDrawList, GR_COPY);

	if ( Print_Sheet_Ref )
		m_Parent->TraceWorkSheet(DC, screen);

	m_Parent->m_CurrentScreen = oldscreen;
	wxEndBusyCursor();
}


/*****************************************************************************
* Routine to redraw list of structs.										 *
* If the list is of DrawPickStruct types then the picked item are drawn.	 *
*****************************************************************************/
void RedrawStructList(WinEDA_DrawPanel * panel, wxDC * DC,
				EDA_BaseStruct *Structs, int DrawMode, int Color)
{
	while (Structs)
		{
		if (Structs->m_StructType == DRAW_PICK_ITEM_STRUCT_TYPE)
			{
			RedrawOneStruct(panel, DC,
						((DrawPickedStruct *) Structs)->m_PickedStruct,
									 DrawMode, Color);
			}
		else
			{
			if ( ! (Structs->m_Flags & IS_MOVED) )
				RedrawOneStruct(panel, DC, Structs, DrawMode, Color);
			}

		Structs = Structs->Pnext;
		}
}

/*****************************************************************************
* Routine to redraw list of structs.										 *
*****************************************************************************/
void RedrawOneStruct(WinEDA_DrawPanel * panel, wxDC * DC,
				EDA_BaseStruct *Struct, int DrawMode, int Color)
{

	if ( Struct == NULL ) return;
	if (HighLightStruct == Struct) Color = HIGHLIGHT_COLOR;

	Struct->Draw(panel, DC, wxPoint(0,0), DrawMode, Color);
}

/********************************************************************************************/
void DrawSheetLabelStruct::Draw(WinEDA_DrawPanel * panel,wxDC * DC, const wxPoint & offset,
					int DrawMode, int Color)
/********************************************************************************************/
/* Routine de dessin des Labels type hierarchie */
{
int side, txtcolor;
int posx , tposx, posy, size2;
wxSize size;
int NbSegm, coord[12];

	if( Color >= 0 ) txtcolor = Color;
	else txtcolor = ReturnLayerColor(m_Layer);
	GRSetDrawMode(DC, DrawMode);

	posx = m_Pos.x + offset.x; posy = m_Pos.y + offset.y; size = m_Size;
	if( !m_Text.IsEmpty() )
	{
		if( m_Edge )
		{
			tposx = posx - size.x;
			side = GR_TEXT_HJUSTIFY_RIGHT;
		}
		else
		{
			tposx = posx + size.x + (size.x /8) ;
			side = GR_TEXT_HJUSTIFY_LEFT;
		}
		DrawGraphicText(panel, DC, wxPoint(tposx, posy), txtcolor,
					m_Text, TEXT_ORIENT_HORIZ,size ,
					side, GR_TEXT_VJUSTIFY_CENTER);
	}
	/* dessin du symbole de connexion */

	if(m_Edge)
		{
		size.x = -size.x;
		size.y = -size.y;
		}
		
	coord[0] = posx; coord[1] = posy; size2 = size.x /2;
	NbSegm = 0;
	switch(m_Shape)
		{
		case 0:		/* input |> */
			coord[2] = posx ; coord[3] = posy - size2;
			coord[4] = posx + size2; coord[5] = posy - size2;
			coord[6] = posx + size.x; coord[7] = posy;
			coord[8] = posx + size2; coord[9] = posy + size2;
			coord[10] = posx ; coord[11] = posy + size2;
			coord[12] = coord[0] ; coord[13] = coord[1];
			NbSegm = 7;
			break;

		case 1:		/* output <| */
			coord[2] = posx + size2; coord[3] = posy - size2;
			coord[4] = posx + size.x; coord[5] = posy - size2;
			coord[6] = posx + size.x; coord[7] = posy + size2;
			coord[8] = posx + size2; coord[9] = posy + size2;
			coord[10] = coord[0] ; coord[11] = coord[1];
			NbSegm = 6;
			break;

		case 2:		/* bidi <> */
		case 3:		/* TriSt <> */
			coord[2] = posx + size2; coord[3] = posy - size2;
			coord[4] = posx + size.x; coord[5] = posy;
			coord[6] = posx + size2; coord[7] = posy +size2;
			coord[8] = coord[0];  coord[9] = coord[1];
			NbSegm = 5;
			break;

		default:	 /* unsp []*/
			coord[2] = posx ; coord[3] = posy - size2;
			coord[4] = posx + size.x; coord[5] = posy - size2;
			coord[6] = posx + size.x; coord[7] = posy + size2;
			coord[8] = posx ; coord[9] = posy + size2;
			coord[10] = coord[0] ; coord[11] = coord[1];
			NbSegm = 6;
			break;
		}
//	GRPoly(&panel->m_ClipBox, DC, NbSegm, coord, 1, txtcolor, txtcolor);	/* Poly rempli */
	GRPoly(&panel->m_ClipBox, DC, NbSegm, coord, 0, txtcolor, txtcolor);	/* Poly Non rempli */
}



/**************************************************************************************/
void DrawSheetStruct::Draw(WinEDA_DrawPanel * panel,wxDC * DC, const wxPoint & offset,
		int DrawMode, int Color)
/**************************************************************************************/
/* Draw the hierarchical sheet shape */
{
DrawSheetLabelStruct * SheetLabelStruct;
int txtcolor;
wxString Text;
int color;
wxPoint pos = m_Pos + offset;
	
	if( Color >= 0 ) color = Color;
	else color = ReturnLayerColor(m_Layer);
	GRSetDrawMode(DC, DrawMode);

	GRRect(&panel->m_ClipBox, DC, pos.x, pos.y,
				 pos.x + m_Size.x, pos.y + m_Size.y, color);

	/* Trace des textes : SheetName */
	if( Color > 0 ) txtcolor = Color;
	else txtcolor = ReturnLayerColor(LAYER_SHEETNAME);

	Text = wxT("Sheet: ") + m_SheetName;
	DrawGraphicText(panel, DC,
				wxPoint(pos.x, pos.y - 8), txtcolor,
				Text, TEXT_ORIENT_HORIZ, wxSize(m_SheetNameSize,m_SheetNameSize),
				GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM);

	/* Trace des textes : FileName */
	if( Color >= 0 ) txtcolor = Color;
	else txtcolor = ReturnLayerColor(LAYER_SHEETFILENAME);
	Text = wxT("File: ") + m_FileName;
	DrawGraphicText(panel, DC,
				wxPoint(pos.x, pos.y + m_Size.y + 4),
				txtcolor,
				Text, TEXT_ORIENT_HORIZ, wxSize(m_FileNameSize,m_FileNameSize),
				GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP);


	/* Trace des textes : SheetLabel */
	SheetLabelStruct = m_Label;
	while( SheetLabelStruct != NULL )
	{
		SheetLabelStruct->Draw(panel, DC, offset,DrawMode, Color);
		SheetLabelStruct = (DrawSheetLabelStruct*)(SheetLabelStruct->Pnext);
	}
}


/*********************************************************************/
void EDA_DrawLineStruct::Draw(WinEDA_DrawPanel * panel,wxDC * DC, const wxPoint & offset,
					int DrawMode, int Color)
/*********************************************************************/
/* Routine de dessin des segments type wire, Bus .. */
{
int color;
int zoom = panel->GetZoom();

	if( Color >= 0 ) color = Color;
	else color = ReturnLayerColor(m_Layer);
	GRSetDrawMode(DC, DrawMode);

	if( m_Layer == LAYER_NOTES)
		GRDashedLine(&panel->m_ClipBox, DC, m_Start.x + offset.x, m_Start.y + offset.y,
					m_End.x + offset.x, m_End.y + offset.y, color);

	else if( (m_Layer == LAYER_BUS) && (zoom <= 16) )
		GRBusLine(&panel->m_ClipBox, DC, m_Start.x + offset.x, m_Start.y + offset.y,
				m_End.x + offset.x, m_End.y + offset.y, color);
	else
		GRLine(&panel->m_ClipBox, DC, m_Start.x + offset.x, m_Start.y + offset.y,
				m_End.x + offset.x, m_End.y + offset.y, color);
	if ( m_StartIsDangling )
		DrawDanglingSymbol(panel, DC, m_Start + offset, color);

	if ( m_EndIsDangling )
		DrawDanglingSymbol(panel, DC, m_End + offset, color);
}


/*******************************************************************/
void DrawMarkerStruct::Draw(WinEDA_DrawPanel * panel,wxDC * DC, const wxPoint & offset,
		int DrawMode, int Color)
/*******************************************************************/
/* Routine de dessin des marqueurs .. */
{
#define WAR 1	// utilisé aussi dans erc.cpp

	if( m_Type == MARQ_ERC )
		{
		int color = Color;
		if ( Color <= 0 )
			{
			color = (m_MarkFlags == WAR ) ?
				g_LayerDescr.LayerColor[LAYER_ERC_WARN] :
				g_LayerDescr.LayerColor[LAYER_ERC_ERR];
			}
		Draw_Marqueur(panel, DC, m_Pos+offset, marqERC_bitmap, DrawMode, color);
		}

	else Draw_Marqueur(panel, DC, m_Pos+offset, marq_bitmap, DrawMode, Color);
}

/*************************************************************************/
void DrawNoConnectStruct::Draw(WinEDA_DrawPanel * panel, wxDC * DC, const wxPoint & offset,
		int DrawMode, int Color)
/*************************************************************************/
/* Routine de dessin des symboles de "No Connexion" .. */
{
#define DELTA (DRAWNOCONNECT_SIZE/2)
int pX, pY, color;

	pX = m_Pos.x + offset.x; pY = m_Pos.y + offset.y;

	if( Color >= 0 ) color = Color;
	else color = ReturnLayerColor(LAYER_NOCONNECT);
	GRSetDrawMode(DC, DrawMode);

	GRLine(&panel->m_ClipBox, DC, pX - DELTA, pY - DELTA, pX + DELTA, pY + DELTA, color);
	GRLine(&panel->m_ClipBox, DC, pX + DELTA, pY - DELTA, pX - DELTA, pY + DELTA, color);

}

/**************************************************************/
void DrawBusEntryStruct::Draw(WinEDA_DrawPanel * panel, wxDC * DC, const wxPoint & offset,
		int DrawMode, int Color)
/***************************************************************/

/* Routine de dessin des Raccords a 45 degre type wire, Bus .. */

{
int color;
int zoom = panel->GetZoom();

	if( Color >= 0 ) color = Color;
	else color = ReturnLayerColor(m_Layer);
	GRSetDrawMode(DC, DrawMode);

	if( (m_Layer == LAYER_BUS) && (zoom <= 16) )
		GRBusLine(&panel->m_ClipBox, DC, m_Pos.x + offset.x, m_Pos.y + offset.y,
				  m_End().x + offset.x, m_End().y + offset.y, color);
	else
		GRLine(&panel->m_ClipBox, DC, m_Pos.x + offset.x, m_Pos.y + offset.y,
				  m_End().x + offset.x, m_End().y + offset.y, color);

}

/*****************************************************************************
* Routine to redraw polyline struct.										 *
*****************************************************************************/
void DrawPolylineStruct::Draw(WinEDA_DrawPanel * panel, wxDC * DC, const wxPoint & offset,
		int DrawMode, int Color)
{
int i, color ;
int zoom = panel->GetZoom();

	if( Color >= 0 ) color = Color;
	else color = ReturnLayerColor(m_Layer);
	GRSetDrawMode(DC, DrawMode);

	GRMoveTo(m_Points[0], m_Points[1]);
	if( m_Layer == LAYER_NOTES)
		{
		for (i = 1; i < m_NumOfPoints; i++)
			GRDashedLineTo(&panel->m_ClipBox, DC, m_Points[i * 2] + offset.x,
							m_Points[i * 2 + 1] + offset.y, color);
		}
	else if( (m_Layer == LAYER_BUS) && (zoom <= 16) )
		{
		for (i = 1; i < m_NumOfPoints; i++)
			GRBusLineTo(&panel->m_ClipBox, DC, m_Points[i * 2] + offset.x,
						m_Points[i * 2 + 1] + offset.y, color);
		}
	else
		{
		for (i = 1; i < m_NumOfPoints; i++)
			GRLineTo(&panel->m_ClipBox, DC, m_Points[i * 2] + offset.x, m_Points[i * 2 + 1] + offset.y, color);
		}
}

/*****************************************************************************
* Routine to redraw connection struct.										 *
*****************************************************************************/
void DrawJunctionStruct::Draw(WinEDA_DrawPanel * panel,wxDC * DC, const wxPoint & offset,
						int DrawMode, int Color)
{
int color;
int	Width = DRAWJUNCTION_SIZE;

	if( Color >= 0 ) color = Color;
	else color = ReturnLayerColor(m_Layer);
	GRSetDrawMode(DC, DrawMode);

	GRFilledRect(&panel->m_ClipBox, DC, m_Pos.x - Width + offset.x, m_Pos.y - Width + offset.y,
			m_Pos.x + Width + offset.x, m_Pos.y + Width + offset.y, color, color);
}

/**********************************************************/
void DrawStructsInGhost(WinEDA_DrawPanel * panel,wxDC * DC,
		EDA_BaseStruct * DrawStruct, int dx, int dy )
/**********************************************************/
/* Routine de redessin en mode fantome (Dessin simplifie en g_XorMode et
 g_GhostColor
de structures.
	Utilisee dans les deplacements de blocs
*/
{
int Width, ii;
int DrawMode = g_XorMode;

	GRSetDrawMode(DC, DrawMode);

	switch (DrawStruct->m_StructType)
		{
		case DRAW_POLYLINE_STRUCT_TYPE:
			{
			DrawPolylineStruct * Struct;
			Struct = (DrawPolylineStruct * ) DrawStruct;
			GRMoveTo(Struct->m_Points[0] + dx, Struct->m_Points[1] + dy);
			for (ii = 1; ii < Struct->m_NumOfPoints; ii++)
			GRLineTo(&panel->m_ClipBox, DC, Struct->m_Points[ii * 2] + dx,
					 Struct->m_Points[ii * 2 + 1] +dy, g_GhostColor);
			break;
			}

		case DRAW_SEGMENT_STRUCT_TYPE:
			{
			EDA_DrawLineStruct * Struct;
			Struct = (EDA_DrawLineStruct * ) DrawStruct;
			if( (Struct->m_Flags & STARTPOINT) == 0 )
				{
				GRMoveTo(Struct->m_Start.x + dx, Struct->m_Start.y + dy);
				}
			else
				{
				GRMoveTo(Struct->m_Start.x, Struct->m_Start.y);
				}
			if( (Struct->m_Flags & ENDPOINT) == 0 )
				{
				GRLineTo(&panel->m_ClipBox, DC, Struct->m_End.x + dx, Struct->m_End.y + dy, g_GhostColor);
				}
			else
				{
				GRLineTo(&panel->m_ClipBox, DC, Struct->m_End.x, Struct->m_End.y, g_GhostColor);
				}
			break;
			}

		case DRAW_BUSENTRY_STRUCT_TYPE:
			{
			DrawBusEntryStruct * Struct = (DrawBusEntryStruct *) DrawStruct;
			int xx = Struct->m_Pos.x + dx, yy = Struct->m_Pos.y + dy;
			GRMoveTo(xx, yy);
			GRLineTo(&panel->m_ClipBox, DC, Struct->m_Size.x + xx, Struct->m_Size.y + yy, g_GhostColor);
			break;
			}

		case DRAW_JUNCTION_STRUCT_TYPE:
			{
			DrawJunctionStruct * Struct;
			Struct = (DrawJunctionStruct * ) DrawStruct;
			Width = DRAWJUNCTION_SIZE;
			GRFilledRect(&panel->m_ClipBox, DC, Struct->m_Pos.x - Width + dx, Struct->m_Pos.y - Width + dy,
				Struct->m_Pos.x + Width + dx, Struct->m_Pos.y + Width + dy,
				g_GhostColor, g_GhostColor);
			break;
			}

		case DRAW_TEXT_STRUCT_TYPE:
			{
			DrawTextStruct * Struct;
			Struct = (DrawTextStruct * ) DrawStruct;
			Struct->m_Pos.x += dx; Struct->m_Pos.y += dy;
			Struct->Draw(panel, DC, wxPoint(0,0),DrawMode,g_GhostColor);
			Struct->m_Pos.x -= dx; Struct->m_Pos.y -= dy;
			break;
			}

		case DRAW_LABEL_STRUCT_TYPE:
			{
			DrawLabelStruct * Struct;
			Struct = (DrawLabelStruct * ) DrawStruct;
			Struct->m_Pos.x += dx; Struct->m_Pos.y += dy;
			Struct->Draw(panel, DC, wxPoint(0,0),DrawMode,g_GhostColor);
			Struct->m_Pos.x -= dx; Struct->m_Pos.y -= dy;
			break;
			}

		case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
			{
			DrawGlobalLabelStruct * Struct;
			Struct = (DrawGlobalLabelStruct * ) DrawStruct;
			Struct->m_Pos.x += dx; Struct->m_Pos.y += dy;
			Struct->Draw(panel, DC, wxPoint(0,0),DrawMode,g_GhostColor);
			Struct->m_Pos.x -= dx; Struct->m_Pos.y -= dy;
			break;
			}

		case DRAW_NOCONNECT_STRUCT_TYPE:
			{
			DrawNoConnectStruct * Struct;
			Struct = (DrawNoConnectStruct * ) DrawStruct;
			Struct->m_Pos.x += dx; Struct->m_Pos.y += dy;
			Struct->Draw(panel, DC, wxPoint(0,0),DrawMode,g_GhostColor);
			Struct->m_Pos.x -= dx; Struct->m_Pos.y -= dy;
			break;
			}

		case DRAW_LIB_ITEM_STRUCT_TYPE:
			{
			EDA_LibComponentStruct *LibEntry;
			EDA_SchComponentStruct *Struct;
			Struct = (EDA_SchComponentStruct * ) DrawStruct;
			LibEntry = FindLibPart(Struct->m_ChipName.GetData(), wxEmptyString, FIND_ROOT);
			if( LibEntry == NULL ) break;
			DrawingLibInGhost(panel, DC, LibEntry, Struct, Struct->m_Pos.x + dx,
								Struct->m_Pos.y + dy,
								Struct->m_Multi, Struct->m_Convert,
								g_GhostColor, FALSE);
			break;
			}

		case DRAW_SHEET_STRUCT_TYPE:
			{
			DrawSheetStruct *Struct = (DrawSheetStruct * ) DrawStruct;
			GRRect(&panel->m_ClipBox, DC, Struct->m_Pos.x + dx, Struct->m_Pos.y + dy,
						Struct->m_Pos.x + Struct->m_Size.x + dx,
						Struct->m_Pos.y + Struct->m_Size.y + dy, g_GhostColor);
			break;
			}

		case DRAW_SHEETLABEL_STRUCT_TYPE:
		case DRAW_MARKER_STRUCT_TYPE:
			break;

		default:
			break;
	}
}


/************************************************************/
void Draw_Marqueur(WinEDA_DrawPanel * panel, wxDC * DC,
			wxPoint pos, char* pt_bitmap, int DrawMode, int Color)
/************************************************************/
/*
 Place un repere sur l'ecran au point de coordonnees PCB pos_X, pos_Y
	Le marqueur est defini par un tableau de 2 + (lig*col) elements:
	 1er element: dim nbre ligne
	 2er element: dim nbre col
	 suite: lig * col elements a 0 ou 1 : si 1 mise a color du pixel

 copie la description du marqueur en current_marqueur (global)
*/
{
int px, py, color;
char ii, ii_max, jj, jj_max;
	
	if ( pt_bitmap == NULL ) pt_bitmap = marq_bitmap;

	px = GRMapX(pos.x); py = GRMapY(pos.y);

	/* Lecture des dimensions */
	ii_max = *(pt_bitmap++); jj_max = *(pt_bitmap++);

	/* lecture des offsets */
	px += *(pt_bitmap++); py += *(pt_bitmap++);

	color = *(pt_bitmap++);
	if ( (Color > 0) ) color = Color;
	if (color < 0) color = 0;
	GRSetDrawMode(DC, DrawMode);

	/* Trace du bitmap */
	for( ii = 0; ii < ii_max; ii++)
		{
		for( jj = 0; jj < jj_max; jj++, pt_bitmap++)
			{
			if(*pt_bitmap)  GRSPutPixel(&panel->m_ClipBox, DC, px+ii , py+jj , color);
			}
		}
}


