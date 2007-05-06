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
