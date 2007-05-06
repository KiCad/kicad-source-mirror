/**********************************************************/
/*	EECLASS.CPP											  */
/* fonctions relatives aux classes definies dans EESCHEMA */
/**********************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

/******************************************************************/
void SetStructFather(EDA_BaseStruct * Struct, BASE_SCREEN * Screen)
/******************************************************************/
{
	switch( Struct->m_StructType )
		{
		case DRAW_POLYLINE_STRUCT_TYPE:
		case DRAW_JUNCTION_STRUCT_TYPE:
		case DRAW_TEXT_STRUCT_TYPE:
		case DRAW_LABEL_STRUCT_TYPE:
		case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
		case DRAW_LIB_ITEM_STRUCT_TYPE:
		case DRAW_SEGMENT_STRUCT_TYPE:
		case DRAW_BUSENTRY_STRUCT_TYPE:
		case DRAW_SHEET_STRUCT_TYPE:
		case DRAW_MARKER_STRUCT_TYPE:
		case DRAW_NOCONNECT_STRUCT_TYPE:
			Struct->m_Parent = Screen;
			break;

		case DRAW_SHEETLABEL_STRUCT_TYPE:
		case DRAW_PICK_ITEM_STRUCT_TYPE:
			break;

		default:
			break;
		}
}

/***************************************************************/
void EDA_BaseStruct::Place(WinEDA_DrawFrame * frame, wxDC * DC)
/***************************************************************/
/* place the struct in EEDrawList.
	if it is a new item, it it also put in undo list
	for an "old" item, saving it in undo list must be done before editiing, and not here!
*/
{
	if( m_Flags & IS_NEW)
	{
		Pnext = frame->m_CurrentScreen->EEDrawList;
		frame->m_CurrentScreen->EEDrawList = this;
		g_ItemToRepeat = this;
		if ( frame->m_Ident == SCHEMATIC_FRAME )
			((WinEDA_SchematicFrame *)frame)->SaveCopyInUndoList(this, IS_NEW);
	}

	m_Flags = 0;
	frame->GetScreen()->SetModify();
	frame->GetScreen()->m_CurrentItem = NULL;
	frame->DrawPanel->ManageCurseur = NULL;
	frame->DrawPanel->ForceCloseManageCurseur = NULL;

	if ( DC )
	{
		frame->DrawPanel->CursorOff(DC);	// Erase schematic cursor
		RedrawOneStruct(frame->DrawPanel, DC, this, GR_DEFAULT_DRAWMODE);
		frame->DrawPanel->CursorOn(DC);	// Display schematic cursor
	}
}


/***********************************************************************/
/* Class SCH_SCREEN: classe de gestion d'un affichage pour schematique */
/***********************************************************************/
static int table_zoom[] = {1,2,4,8,16,32,64,128, 0}; /* Valeurs standards du zoom */

/* Constructeur de SCREEN */
SCH_SCREEN::SCH_SCREEN(int screentype): BASE_SCREEN(screentype)
{
	EEDrawList = NULL;	 /* Schematic items list */
	m_Zoom = 32;
	m_Grid = wxSize(50,50);			/* pas de la grille */
	SetZoomList(table_zoom);
	SetGridList(g_GridList);
	m_UndoRedoCountMax = 10;

}

/****************************/
SCH_SCREEN::~SCH_SCREEN(void)
/****************************/
{
	ClearUndoRedoList();
	FreeDrawList();
}


/***********************************/
void SCH_SCREEN::FreeDrawList(void)
/***********************************/
/* Routine to clear (free) EESchema drawing list of a screen.
*/
{
EDA_BaseStruct *DrawStruct;

	while (EEDrawList != NULL)
	{
		DrawStruct = EEDrawList;
		EEDrawList = EEDrawList->Pnext;
		delete DrawStruct;
	}
}

/**************************************************************/
void SCH_SCREEN::RemoveFromDrawList(EDA_BaseStruct *DrawStruct)
/**************************************************************/
/* If found in EEDrawList, remove DrawStruct from EEDrawList.
	DrawStruct is not deleted or modified
*/
{
	if (DrawStruct == EEDrawList)
		EEDrawList = EEDrawList->Pnext;
	else
	{
		EDA_BaseStruct * DrawList = EEDrawList;
		while (DrawList->Pnext)
		{
			if (DrawList->Pnext == DrawStruct)
			{
				DrawList->Pnext = DrawList->Pnext->Pnext;
				break;
			}
			DrawList = DrawList->Pnext;
		}
	}
}


/*********************************************************************/
/* Class EDA_ScreenList to handle the list of screens in a hierarchy */
/*********************************************************************/

EDA_ScreenList::EDA_ScreenList(EDA_BaseStruct * DrawStruct)
/* create the list of screens (i.e hierarchycal sheets) found in DrawStruct
if DrawStruct == NULL: start from root sheet and the root screen is included in list
*/
{
	m_Count = 0;
	m_List = NULL;
	m_Index = 0;
	
	/* Count the number of screens */
	BuildScreenList(NULL, DrawStruct, & m_Count);
	if( m_Count > NB_MAX_SHEET )
	{
		wxString msg;
		msg.Printf(wxT("ReturnScreenList: Error: screen count > %d"), NB_MAX_SHEET);
		DisplayError(NULL, msg);
	}


	m_List = (SCH_SCREEN **) MyZMalloc( sizeof(SCH_SCREEN *) * (m_Count + 2) );
	/* Fill the list */
	BuildScreenList(m_List, DrawStruct, & m_Count);
}

/*****************************************/
EDA_ScreenList::~EDA_ScreenList()
/*****************************************/
{
	if ( m_List ) free( m_List );
	m_List = NULL;
}

/*****************************************/
SCH_SCREEN * EDA_ScreenList::GetFirst(void)
/*****************************************/
{
	m_Index = 0;
	if ( m_List ) return * m_List;
	else return NULL;
}

/*****************************************/
SCH_SCREEN * EDA_ScreenList::GetNext(void)
/*****************************************/
{
	if (m_Index < m_Count) m_Index ++;
		
	if ( (m_Index < m_Count) && m_List )
	{
		return m_List[m_Index];
	}
	else return NULL;
}

/************************************************/
SCH_SCREEN * EDA_ScreenList::GetScreen(int index)
/************************************************/
/* return the m_List[index] item
*/
{
SCH_SCREEN * screen = NULL;
	
	if( (index >= 0) && index < m_Count )
		screen = m_List[index];

	return screen;
}

/**************************************************/
void EDA_ScreenList::UpdateSheetNumberAndDate(void)
/**************************************************/
/* Update the sheet number, the sheet count and the date for all sheets in list
*/
{
int SheetNumber = 1;
SCH_SCREEN * screen;
wxString sheet_date = GenDate();
	
	for ( screen = GetFirst(); screen != NULL; screen = GetNext() )
	{
		screen->m_SheetNumber = SheetNumber++;		/* Update the sheet number */
		screen->m_NumberOfSheet = m_Count;	/* Update the number of sheets */
		screen->m_Date = sheet_date;		/* Update the sheet date */
	}
}

/************************************************************************/
SCH_SCREEN ** EDA_ScreenList::BuildScreenList(SCH_SCREEN ** ScreenList, 
		EDA_BaseStruct * DrawStruct, int * Count)
/************************************************************************/
/* Count the Hierachical sheet number (ScreenList == NULL )
	or fill the screen pointer buffer (ScreenList != NULL )
	If DrawStruct = NULL, search starts from Root screen, and puts root screen in list
	Recursive function !
*/
{
bool HasSubhierarchy = FALSE;
EDA_BaseStruct * CurrStruct;
	
	if( * Count > NB_MAX_SHEET )
	{
		return ScreenList;
	}

	/* Read the current list and put Screen pointers in list */
	if ( DrawStruct == NULL )
	{
		DrawStruct = ScreenSch->EEDrawList;
		if ( ScreenList )
		{
			*ScreenList = ScreenSch;
			ScreenList++;
		}
		else (*Count)++;
	}
	
	CurrStruct = DrawStruct;
	while( CurrStruct )
	{
		if(CurrStruct->m_StructType == DRAW_SHEET_STRUCT_TYPE )
		{
			HasSubhierarchy = TRUE;
			if ( ScreenList )
			{
				*ScreenList = (SCH_SCREEN*) CurrStruct;
				ScreenList++;
			}
			else (*Count)++;
		}
		CurrStruct = CurrStruct->Pnext;
	}
	
	if ( ! HasSubhierarchy ) return ScreenList;
		
	/* Read the Sub Hierarchies  */
	CurrStruct = DrawStruct;
	while( CurrStruct )
	{
		if(CurrStruct->m_StructType == DRAW_SHEET_STRUCT_TYPE )
		{
			SCH_SCREEN* Screen = (SCH_SCREEN*) CurrStruct;
			/* Go to Subhierachy if needed
			(warning: BuildScreenList must not called with a NULL parameter
			for DrawStruct, because BuildScreenList restart from the root screen
			when DrawStruct == NULL */
			if ( Screen->EEDrawList )
				ScreenList = BuildScreenList(ScreenList, Screen->EEDrawList,Count);
		}
		CurrStruct = CurrStruct->Pnext;
	}
	return ScreenList;
}

