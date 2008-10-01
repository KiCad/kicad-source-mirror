	/****************************/
	/*  EESchema - viewlibs.cpp */
	/****************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

#include "id.h"

#define NEXT_PART 1
#define NEW_PART 0
#define PREVIOUS_PART -1


/* Routines locales */

/* Variables locales */

/*************************************************************************/
void WinEDA_ViewlibFrame::Process_Special_Functions(wxCommandEvent& event)
/*************************************************************************/
{
EDA_LibComponentStruct * LibEntry;
int ii, id = event.GetId();
wxPoint pos;

	wxGetMousePosition(&pos.x, &pos.y);
	pos.y += 20;

	switch ( id )
	{
		case ID_LIBVIEW_SELECT_LIB:
			SelectCurrentLibrary();
			break;

		case ID_LIBVIEW_SELECT_PART:
			SelectAndViewLibraryPart(NEW_PART);
			break;

		case ID_LIBVIEW_NEXT:
			SelectAndViewLibraryPart(NEXT_PART);
			break;

		case ID_LIBVIEW_PREVIOUS:
			SelectAndViewLibraryPart(PREVIOUS_PART);
			break;

		case ID_LIBVIEW_VIEWDOC:
			LibEntry = FindLibPart(g_CurrentViewComponentName.GetData(),
         					g_CurrentViewLibraryName.GetData(), FIND_ALIAS);
			if( LibEntry && ( !LibEntry->m_DocFile.IsEmpty()) )
				GetAssociatedDocument(this, g_RealLibDirBuffer, LibEntry->m_DocFile );
			break;

		case ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT:
			m_HToolBar->ToggleTool(ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, TRUE);
			m_HToolBar->ToggleTool(ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, FALSE);
			g_ViewConvert = 1;
			ReDrawPanel();
			break;

		case ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT:
			m_HToolBar->ToggleTool(ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, FALSE);
			m_HToolBar->ToggleTool(ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, TRUE);
			g_ViewConvert = 2;
			ReDrawPanel();
			break;

		case ID_LIBVIEW_SELECT_PART_NUMBER:
			ii = SelpartBox->GetChoice();
			if ( ii < 0 ) return;
			g_ViewUnit = ii + 1;
			ReDrawPanel();
			break;

		default:
		{
			wxString msg;
			msg << wxT("WinEDA_ViewlibFrame::Process_Special_Functions error: id = ") << id;
			DisplayError( this, msg);
			break;
		}
	}

}


/*************************************************************************/
void WinEDA_ViewlibFrame::OnLeftClick(wxDC * DC, const wxPoint& MousePos)
/*************************************************************************/
{
}

/********************************************************************************/
bool WinEDA_ViewlibFrame::OnRightClick(const wxPoint& MousePos, wxMenu * PopMenu)
/********************************************************************************/
{
	return true;
}


/**********************************************/
void WinEDA_ViewlibFrame::DisplayLibInfos()
/**********************************************/
/* Affiche en Ligne d'info la librairie en cours de visualisation */
{
wxString msg;
LibraryStruct * Lib;

	Lib = FindLibrary(g_CurrentViewLibraryName);
	msg = _("Library browser");

        msg << wxT(" [");

	if ( Lib ) msg << g_CurrentViewLibraryName;
	else msg += _("none selected");

        msg << wxT("]");
	SetTitle(msg);
}


	/*****************************************/
	/* Routine to Select Current library	  */
	/*****************************************/
void WinEDA_ViewlibFrame::SelectCurrentLibrary()
{
LibraryStruct * Lib;

	Lib = SelectLibraryFromList(this);
	if ( Lib )
	{
		g_CurrentViewComponentName.Empty();
		g_CurrentViewLibraryName = Lib->m_Name;
		DisplayLibInfos();
		if ( m_LibList )
		{
			ReCreateListCmp();
			ReDrawPanel();
			DisplayLibInfos();
			ReCreateHToolbar();
			int id = m_LibList->FindString(g_CurrentViewLibraryName.GetData());
			if ( id >= 0 ) m_LibList->SetSelection(id);
		}
	}
}

/**************************************************************/
void WinEDA_ViewlibFrame::SelectAndViewLibraryPart(int option)
/**************************************************************/
/* Routine to select and view library Part  (NEW, NEXT or PREVIOUS) */
{
LibraryStruct * Lib;

	if(g_CurrentViewLibraryName.IsEmpty() ) SelectCurrentLibrary();
	if(g_CurrentViewLibraryName.IsEmpty() ) return;

	Lib = FindLibrary(g_CurrentViewLibraryName);
	if ( Lib == NULL ) return;

	if ( (g_CurrentViewComponentName.IsEmpty()) || ( option == NEW_PART ) )
		{
		ViewOneLibraryContent(Lib, NEW_PART);
		return;
		}

	EDA_LibComponentStruct * LibEntry = FindLibPart(g_CurrentViewComponentName.GetData(),
								 g_CurrentViewLibraryName, FIND_ALIAS);

	if ( LibEntry == NULL ) return;

	if (option == NEXT_PART )
		ViewOneLibraryContent(Lib, NEXT_PART);

	if (option == PREVIOUS_PART )
		ViewOneLibraryContent(Lib, PREVIOUS_PART);

}

/*************************************************/
/* Routine to view one selected library content. */
/*************************************************/
void WinEDA_ViewlibFrame::ViewOneLibraryContent(LibraryStruct *Lib, int Flag)
{
int NumOfParts = 0;
EDA_LibComponentStruct *LibEntry, * PreviousLibEntry;
wxString CmpName;
wxClientDC dc(DrawPanel);


	DrawPanel->PrepareGraphicContext(&dc);

	if (Lib) NumOfParts = Lib->m_NumOfParts;

	if (NumOfParts == 0)
		{
		DisplayError(this, wxT("No Library or Library is empty!"));
		return;
		}

	if ( Lib == NULL ) return;

	PQCompFunc((PQCompFuncType) LibraryEntryCompare);
	LibEntry = (EDA_LibComponentStruct *) PQFirst(&Lib->m_Entries, FALSE);

	if( Flag == NEW_PART )
		{
		DisplayComponentsNamesInLib(this, Lib, CmpName, g_CurrentViewComponentName);
		}

	if( Flag == NEXT_PART )
		{
		LibEntry = (EDA_LibComponentStruct *) PQFirst(&Lib->m_Entries, FALSE);
		while ( LibEntry )
			{
			if( LibEntry->m_Name.m_Text == g_CurrentViewComponentName )
				{
				LibEntry = (EDA_LibComponentStruct *)
					PQNext(Lib->m_Entries, LibEntry, NULL);
				break;
				}
			LibEntry = (EDA_LibComponentStruct *)
					PQNext(Lib->m_Entries, LibEntry, NULL);
			}
		if(LibEntry == NULL)
				LibEntry = (EDA_LibComponentStruct *) PQFirst(&Lib->m_Entries, FALSE);
		if( LibEntry ) CmpName = LibEntry->m_Name.m_Text;
		}

	if( Flag == PREVIOUS_PART )
		{
		PreviousLibEntry = NULL;
		LibEntry = (EDA_LibComponentStruct *) PQFirst(&Lib->m_Entries, FALSE);
		while ( LibEntry )
			{
			if( LibEntry->m_Name.m_Text == g_CurrentViewComponentName )
				{
				if( PreviousLibEntry ) break;
				/* else : pointage sur 1er element :
				balayage de la liste et Previous pointera dernier element */
				}

			PreviousLibEntry = LibEntry;
			LibEntry = (EDA_LibComponentStruct *)
					PQNext(Lib->m_Entries, LibEntry, NULL);
			}
		if( PreviousLibEntry )
			CmpName = PreviousLibEntry->m_Name.m_Text;
		}

	g_ViewUnit = 1;
	g_ViewConvert = 1;

	LibEntry = FindLibPart(CmpName.GetData(),Lib->m_Name, FIND_ALIAS);
	g_CurrentViewComponentName = CmpName;
	DisplayLibInfos();
	Zoom_Automatique(FALSE);
	RedrawActiveWindow(&dc, TRUE);

	if ( m_CmpList )
		{
		int id = m_CmpList->FindString(g_CurrentViewComponentName.GetData());
		if ( id >= 0 ) m_CmpList->SetSelection(id);
		}
	ReCreateHToolbar();
}



/*****************************************************************************/
/* Routine d'affichage du composant selectionne                              */
/*	Si Le composant est un alias, le composant ROOT est recherche et affiche */
/*****************************************************************************/
void WinEDA_ViewlibFrame::RedrawActiveWindow(wxDC * DC, bool EraseBg)
{
EDA_LibComponentStruct * LibEntry = NULL;
LibCmpEntry * ViewCmpEntry = NULL;
const wxChar * RootName, * CmpName;
wxString Msg;

	ActiveScreen = GetScreen();

	if ( * g_CurrentViewComponentName )
		{
		LibEntry = FindLibPart(g_CurrentViewComponentName.GetData(),
				 g_CurrentViewLibraryName, FIND_ALIAS);
		ViewCmpEntry = (LibCmpEntry * ) LibEntry;
		}

	/* Forcage de la reinit de la brosse et plume courante */
	GRResetPenAndBrush(DC);
	DC->SetBackground(*wxBLACK_BRUSH );
	DC->SetBackgroundMode(wxTRANSPARENT);

	if ( EraseBg ) DrawPanel->EraseScreen(DC);

	DrawPanel->DrawBackGround(DC);

	if( LibEntry)
		{
		CmpName = LibEntry->m_Name.m_Text.GetData();
		if( LibEntry->Type != ROOT)
			{
			RootName = ((EDA_LibCmpAliasStruct*)LibEntry)->m_RootName.GetData();
			Msg.Printf( _("Current Part: <%s> (is Alias of <%s>)"),
					CmpName, RootName);
			LibEntry = FindLibPart(RootName,g_CurrentViewLibraryName,FIND_ROOT);

			if( LibEntry == NULL)
			{
				Msg.Printf( _("Error: Root Part <%s> not found"), RootName);
				DisplayError(this, Msg);
			}
			else
			{
				/* Affichage du composant ROOT, avec nom de l'alias */
				wxString RealName;
				RealName = LibEntry->m_Name.m_Text;
				LibEntry->m_Name.m_Text = CmpName;
				if ( g_ViewUnit < 1 ) g_ViewUnit = 1;
 				if ( g_ViewConvert < 1 ) g_ViewConvert = 1;
				DrawLibEntry(DrawPanel, DC, LibEntry, wxPoint(0, 0),
						g_ViewUnit, g_ViewConvert, GR_DEFAULT_DRAWMODE);
				LibEntry->m_Name.m_Text = RealName;
			}
		}

		else
			{
			Msg.Printf( _("Current Part: <%s>"), ViewCmpEntry->m_Name.m_Text.GetData());
			DrawLibEntry(DrawPanel, DC, LibEntry, wxPoint(0, 0),
					g_ViewUnit, g_ViewConvert, GR_DEFAULT_DRAWMODE);
			}
		AfficheDoc(this, ViewCmpEntry->m_Doc, ViewCmpEntry->m_KeyWord);
		}

	SetStatusText(Msg,0);

	DrawPanel->Trace_Curseur(DC);
}



