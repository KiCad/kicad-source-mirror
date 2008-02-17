/*************************************************************************/
/* viewlib_frame.cpp - fonctions des classes du type WinEDA_ViewlibFrame */
/*************************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"

#include "common.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "bitmaps.h"

#include "protos.h"

#include "id.h"

#include "library_browse.xpm"

	/*****************************/
	/* class WinEDA_ViewlibFrame */
	/*****************************/
BEGIN_EVENT_TABLE(WinEDA_ViewlibFrame, wxFrame)
	COMMON_EVENTS_DRAWFRAME

	EVT_CLOSE(WinEDA_ViewlibFrame::OnCloseWindow)
	EVT_SIZE(WinEDA_ViewlibFrame::OnSize)
	EVT_ACTIVATE(WinEDA_DrawFrame::OnActivate)

	EVT_TOOL_RANGE(ID_LIBVIEW_START_H_TOOL, ID_LIBVIEW_END_H_TOOL,
			WinEDA_ViewlibFrame::Process_Special_Functions)

	EVT_TOOL_RANGE(ID_ZOOM_IN_BUTT, ID_ZOOM_PAGE_BUTT,
				WinEDA_DrawFrame::Process_Zoom)

	EVT_TOOL(ID_LIBVIEW_CMP_EXPORT_TO_SCHEMATIC,
				WinEDA_ViewlibFrame::ExportToSchematicLibraryPart)

	EVT_KICAD_CHOICEBOX(ID_LIBVIEW_SELECT_PART_NUMBER,
				WinEDA_ViewlibFrame::Process_Special_Functions)

	EVT_LISTBOX(ID_LIBVIEW_LIB_LIST, WinEDA_ViewlibFrame::ClickOnLibList)
	EVT_LISTBOX(ID_LIBVIEW_CMP_LIST, WinEDA_ViewlibFrame::ClickOnCmpList)

END_EVENT_TABLE()


	/****************/
	/* Constructeur */
	/****************/
WinEDA_ViewlibFrame::WinEDA_ViewlibFrame(wxWindow * father, WinEDA_App *parent,
				LibraryStruct * Library, wxSemaphore * semaphore ):
			WinEDA_DrawFrame(father, VIEWER_FRAME, parent, _("Library browser"),
				wxDefaultPosition, wxDefaultSize)
{
	m_FrameName = wxT("ViewlibFrame");

	m_Draw_Axis = TRUE;				// TRUE pour avoir les axes dessines
	m_Draw_Grid = TRUE;				// TRUE pour avoir la axes dessinee

	// Give an icon
	SetIcon(wxIcon(library_browse_xpm));

	m_CmpList = NULL;
	m_LibList = NULL;
	m_Semaphore = semaphore;
	if ( m_Semaphore ) SetWindowStyle( GetWindowStyle() | wxSTAY_ON_TOP);
		
	m_CurrentScreen = new SCH_SCREEN(VIEWER_FRAME);
	GetScreen()->SetZoom(16);

	if ( Library == NULL )
	{
		m_LibListSize.x = 90;
		m_LibListSize.y = -1;
		m_LibList = new wxListBox(this, ID_LIBVIEW_LIB_LIST,wxPoint(0,0),
					m_LibListSize, 0, NULL, wxLB_HSCROLL );
		m_LibList->SetFont(*g_DialogFont);
		m_LibList->SetBackgroundColour(wxColour(150,255,255));
		m_LibList->SetForegroundColour(wxColour(0,0,0));
	}
	else g_CurrentViewLibraryName = Library->m_Name;
		
	m_CmpListSize.x = 150;
	m_CmpListSize.y = -1;
	m_CmpList = new wxListBox(this, ID_LIBVIEW_CMP_LIST,wxPoint(m_LibListSize.x,0),
				m_CmpListSize, 0, NULL, wxLB_HSCROLL );
	m_CmpList->SetFont(*g_DialogFont);
	m_CmpList->SetBackgroundColour(wxColour(255,255,200));
	m_CmpList->SetForegroundColour(wxColour(0,0,0));

	GetSettings();
	SetSize(m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y);
	ReCreateHToolbar();
	ReCreateVToolbar();
	if ( m_LibList) ReCreateListLib();
	DisplayLibInfos();
	Show(TRUE);
}

	/***************/
	/* Destructeur */
	/***************/

WinEDA_ViewlibFrame::~WinEDA_ViewlibFrame()
{
	SAFE_DELETE( m_CurrentScreen );
	m_Parent->m_ViewlibFrame = NULL;
}

/*****************************************************************/
void WinEDA_ViewlibFrame::OnCloseWindow(wxCloseEvent & Event)
/*****************************************************************/
{
	SaveSettings();
	if ( m_Semaphore ) m_Semaphore->Post();
	Destroy();
}


/*****************************************************/
void WinEDA_ViewlibFrame::OnSize(wxSizeEvent & SizeEv)
/*****************************************************/
{
wxSize size;
wxSize maintoolbar_size;
wxSize Vtoolbar_size;

	GetClientSize(&size.x, &size.y);
	m_FrameSize = size;
	size.y -= m_MsgFrameHeight;

	if( m_HToolBar )
	{
		maintoolbar_size = m_HToolBar->GetSize();
	}

	if( m_VToolBar )
	{
		Vtoolbar_size = m_VToolBar->GetSize();
		m_VToolBar->SetSize(size.x -  maintoolbar_size.y, 0,-1, size.y);
	}

	if ( MsgPanel )
	{
		MsgPanel->SetSize(0, size.y, size.x, m_MsgFrameHeight);
	}

	if ( DrawPanel )
	{
		DrawPanel->SetSize(m_LibListSize.x + m_CmpListSize.x, 0,
			size.x - Vtoolbar_size.x - m_LibListSize.x - m_CmpListSize.x,
			size.y);
	}

	if ( m_LibList )
	{
		m_LibListSize.y = size.y;
		m_LibList->SetSize(0, 0, m_LibListSize.x, m_LibListSize.y);
	}

	if ( m_CmpList )
	{
		m_CmpListSize.y = size.y;
		m_CmpList->SetSize(m_LibListSize.x, 0, m_CmpListSize.x, m_CmpListSize.y);
	}
}


/*******************************************/
int WinEDA_ViewlibFrame::BestZoom()
/*******************************************/
{
int bestzoom, ii, jj;
wxSize size, itemsize;
EDA_LibComponentStruct * CurrentLibEntry = NULL;

	CurrentLibEntry = FindLibPart(g_CurrentViewComponentName.GetData(),
							g_CurrentViewLibraryName.GetData(), FIND_ROOT);

	if( CurrentLibEntry == NULL )
		{
		bestzoom = 16;
		GetScreen()->m_Curseur.x = 0;
		GetScreen()->m_Curseur.y = 0;
		return(bestzoom);
		}

	EDA_Rect BoundaryBox = CurrentLibEntry->GetBoundaryBox(g_ViewUnit, g_ViewConvert);
	itemsize = BoundaryBox.GetSize();

	size =  DrawPanel->GetClientSize();
	size.x -= 60;	// Pour marges haut et bas
	ii = itemsize.x / size.x;
	jj = itemsize.y / size.y;
	ii = MAX(ii, jj);

	/* determination du zoom existant le plus proche */
	for ( bestzoom = 1 ;  bestzoom < 512 ;  bestzoom <<= 1 )
		{
		if(bestzoom > ii ) break;
		}

		GetScreen()->m_Curseur = BoundaryBox.Centre();
		GetScreen()->m_Curseur.y = -( GetScreen()->m_Curseur.y );

	return(bestzoom);
}

/***************************************************/
void WinEDA_ViewlibFrame::ReCreateListLib()
/***************************************************/
{
const wxChar ** ListNames, ** names;
int ii;
bool found = FALSE;

	if ( m_LibList == NULL ) return;
	
	ListNames = GetLibNames();

	m_LibList->Clear();
	for ( names = ListNames, ii = 0; *names != NULL; names++, ii++ )
	{
		m_LibList->Append(*names);
		if ( g_CurrentViewLibraryName.Cmp(*names) == 0)
		{
			m_LibList->SetSelection(ii, TRUE);
			found = TRUE;
		}
	}

	free (ListNames);

	/* Librairie courante peut etre non retrouv�e en liste
		(peut etre effac�e lors d'une modification de configuration) */
	if ( ! found )
	{
		g_CurrentViewLibraryName.Empty();
		g_CurrentViewComponentName.Empty();
	}

	ReCreateListCmp();
	ReCreateHToolbar();
	DisplayLibInfos();
	ReDrawPanel();
}

/***********************************************/
void WinEDA_ViewlibFrame::ReCreateListCmp()
/***********************************************/
{
int ii;
EDA_LibComponentStruct *LibEntry = NULL;
LibraryStruct *Library = FindLibrary(g_CurrentViewLibraryName.GetData());

	m_CmpList->Clear();
	ii = 0;
	g_CurrentViewComponentName.Empty();
	g_ViewConvert = 1;						/* Vue normal / convert */
	g_ViewUnit = 1;						/* unit� a afficher (A, B ..) */
	if ( Library )
		LibEntry = (EDA_LibComponentStruct *) PQFirst(&Library->m_Entries, FALSE);
	while( LibEntry )
	{
		m_CmpList->Append(LibEntry->m_Name.m_Text);
		LibEntry = (EDA_LibComponentStruct *) PQNext(Library->m_Entries, LibEntry, NULL);
	}
}


/********************************************************************/
void WinEDA_ViewlibFrame::ClickOnLibList(wxCommandEvent & event)
/********************************************************************/
{
int ii = m_LibList->GetSelection();
	if ( ii < 0 ) return;

wxString name = m_LibList->GetString(ii);
	if ( g_CurrentViewLibraryName == name ) return;
	g_CurrentViewLibraryName = name;
	ReCreateListCmp();
	ReDrawPanel();
	DisplayLibInfos();
	ReCreateHToolbar();
}


/****************************************************************/
void WinEDA_ViewlibFrame::ClickOnCmpList(wxCommandEvent & event)
/****************************************************************/
{
int ii = m_CmpList->GetSelection();
	if ( ii < 0 ) return;

wxString name = m_CmpList->GetString(ii);
	g_CurrentViewComponentName = name;
	DisplayLibInfos();
	g_ViewUnit = 1;
	g_ViewConvert = 1;
	Zoom_Automatique(FALSE);
	ReCreateHToolbar();
	ReDrawPanel();
}


/****************************************************************************/
void WinEDA_ViewlibFrame::ExportToSchematicLibraryPart(wxCommandEvent& event)
/****************************************************************************/
/* Export to schematic the current viewed component, and close the library browser
*/
{
int ii = m_CmpList->GetSelection();
	if ( ii >= 0 ) g_CurrentViewComponentName = m_CmpList->GetString(ii);
	else g_CurrentViewComponentName.Empty();
	Close(TRUE);
}
