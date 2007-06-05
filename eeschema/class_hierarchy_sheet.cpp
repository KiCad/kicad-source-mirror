/////////////////////////////////////////////////////////////////////////////
// Name:        sheet.cpp
// Purpose:     
// Author:      jean-pierre Charras
// Modified by: 
// Created:     08/02/2006 18:37:02
// RCS-ID:      
// Copyright:   License GNU
// Licence:     
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/***********************************************************/
DrawSheetStruct::DrawSheetStruct(const wxPoint & pos) :
			SCH_SCREEN( SCHEMATIC_FRAME )
/***********************************************************/
{
	m_Label = NULL;
	m_NbLabel = 0;
	m_Layer = LAYER_SHEET;
	m_Pos = pos;
	m_SheetNameSize = m_FileNameSize = 60;
	/* change the struct type: SCREEN_STRUCT_TYPE to DRAW_SHEET_STRUCT_TYPE */
	m_StructType = DRAW_SHEET_STRUCT_TYPE;
}

/**************************************/
DrawSheetStruct::~DrawSheetStruct(void)
/**************************************/
{
DrawSheetLabelStruct * label = m_Label, * next_label;

	while (label)
	{
		next_label = (DrawSheetLabelStruct *)label->Pnext;
		delete label;
		label = next_label;
	}
}


/***********************************************/
DrawSheetStruct * DrawSheetStruct::GenCopy(void)
/***********************************************/
/* creates a copy of a sheet
	The linked data itself (EEDrawList) is not duplicated
*/
{
DrawSheetStruct * newitem = new DrawSheetStruct(m_Pos);
DrawSheetLabelStruct * Slabel = NULL, * label = m_Label;

	newitem->m_Size = m_Size;
	newitem->m_Parent = m_Parent;
	newitem->m_TimeStamp = GetTimeStamp();

	newitem->m_FileName = m_FileName;
	newitem->m_FileNameSize = m_FileNameSize;
	newitem->m_SheetName = m_SheetName;
	newitem->m_SheetNameSize = m_SheetNameSize;

	if( label )
	{
		Slabel = newitem->m_Label = label->GenCopy();
		Slabel->m_Parent = newitem;
		label = (DrawSheetLabelStruct*)label->Pnext;
	}

	while( label )
	{
		Slabel->Pnext = label->GenCopy();
		Slabel = (DrawSheetLabelStruct*)Slabel->Pnext;
		Slabel->m_Parent = newitem;
		label = (DrawSheetLabelStruct*)label->Pnext;
	}

	/* copy screen data */
	newitem->m_DrawOrg = m_DrawOrg;
	newitem->m_Curseur = m_Curseur;
	newitem->m_MousePosition = m_MousePosition;
	newitem->m_MousePositionInPixels = m_MousePositionInPixels;
	newitem->m_O_Curseur = m_O_Curseur;
	newitem->m_ScrollbarPos = m_ScrollbarPos;
	newitem->m_ScrollbarNumber = m_ScrollbarNumber;
	newitem->m_StartVisu = m_StartVisu;
	newitem->m_FirstRedraw = m_FirstRedraw;

	newitem->EEDrawList = EEDrawList;	/* Object list (main data) for schematic */
	newitem->m_UndoList = m_UndoList;	/* Object list for the undo command (old data) */
	newitem->m_RedoList = m_RedoList;	/* Object list for the redo command (old data) */

	newitem->m_CurrentSheet = m_CurrentSheet;
	newitem->m_SheetNumber = m_SheetNumber;
	newitem->m_NumberOfSheet = m_NumberOfSheet;
	newitem->m_FileName = m_FileName;
	newitem->m_Title = m_Title;
	newitem->m_Date = m_Date;	
	newitem->m_Revision = m_Revision;
	newitem->m_Company = m_Company;
	newitem->m_Commentaire1 = m_Commentaire1;
	newitem->m_Commentaire2 = m_Commentaire2;
	newitem->m_Commentaire3 = m_Commentaire3;
	newitem->m_Commentaire4 = m_Commentaire4;

	return newitem;
}


/**********************************************************/
void DrawSheetStruct::SwapData(DrawSheetStruct * copyitem)
/**********************************************************/
/* Used if undo / redo command:
	swap data between this and copyitem
*/
{
	EXCHG(m_Pos, copyitem->m_Pos);
	EXCHG(m_Size, copyitem->m_Size);
	EXCHG(m_SheetName, copyitem->m_SheetName);
	EXCHG(m_SheetNameSize, copyitem->m_SheetNameSize);
	EXCHG(m_FileNameSize, copyitem->m_FileNameSize);
	EXCHG(m_Label, copyitem->m_Label);
	EXCHG(m_NbLabel, copyitem->m_NbLabel);
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
int LineWidth = g_DrawMinimunLineWidth;
	
	if( Color >= 0 ) color = Color;
	else color = ReturnLayerColor(m_Layer);
	GRSetDrawMode(DC, DrawMode);

	GRRect(&panel->m_ClipBox, DC, pos.x, pos.y,
				 pos.x + m_Size.x, pos.y + m_Size.y, LineWidth, color);

	/* Trace des textes : SheetName */
	if( Color > 0 ) txtcolor = Color;
	else txtcolor = ReturnLayerColor(LAYER_SHEETNAME);

	Text = wxT("Sheet: ") + m_SheetName;
	DrawGraphicText(panel, DC,
				wxPoint(pos.x, pos.y - 8), txtcolor,
				Text, TEXT_ORIENT_HORIZ, wxSize(m_SheetNameSize,m_SheetNameSize),
				GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM, LineWidth);

	/* Trace des textes : FileName */
	if( Color >= 0 ) txtcolor = Color;
	else txtcolor = ReturnLayerColor(LAYER_SHEETFILENAME);
	Text = wxT("File: ") + m_FileName;
	DrawGraphicText(panel, DC,
				wxPoint(pos.x, pos.y + m_Size.y + 4),
				txtcolor,
				Text, TEXT_ORIENT_HORIZ, wxSize(m_FileNameSize,m_FileNameSize),
				GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP, LineWidth);


	/* Trace des textes : SheetLabel */
	SheetLabelStruct = m_Label;
	while( SheetLabelStruct != NULL )
	{
		SheetLabelStruct->Draw(panel, DC, offset,DrawMode, Color);
		SheetLabelStruct = (DrawSheetLabelStruct*)(SheetLabelStruct->Pnext);
	}
}


/************************/
	/* DrawSheetLabelStruct */
	/************************/

/*******************************************************************/
DrawSheetLabelStruct::DrawSheetLabelStruct(DrawSheetStruct * parent,
			const wxPoint & pos, const wxString & text) :
			EDA_BaseStruct(DRAW_SHEETLABEL_STRUCT_TYPE),
			EDA_TextStruct(text)
/*******************************************************************/
{
	m_Layer = LAYER_SHEETLABEL;
	m_Parent = parent;
	m_Pos = pos;
	m_Edge = 0;
	m_Shape = NET_INPUT;
	m_IsDangling = TRUE;
}

/***********************************************************/
DrawSheetLabelStruct * DrawSheetLabelStruct::GenCopy(void)
/***********************************************************/
{
DrawSheetLabelStruct * newitem =
		new DrawSheetLabelStruct( (DrawSheetStruct *)m_Parent, m_Pos, m_Text);

	newitem->m_Edge = m_Edge;
	newitem->m_Shape = m_Shape;

	return newitem;
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
int LineWidth = g_DrawMinimunLineWidth;

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
					side, GR_TEXT_VJUSTIFY_CENTER, LineWidth);
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
int FillShape = FALSE;
	GRPoly(&panel->m_ClipBox, DC, NbSegm, coord, FillShape, LineWidth, txtcolor, txtcolor);	/* Poly Non rempli */
}


