	/***************************************************/
	/* PCBNEW - Gestion des Recherches (fonction Find) */
	/***************************************************/

	/*	 Fichier find.cpp 	*/

/*
 Affichage et modifications des parametres de travail de PcbNew
 Parametres = dimensions des via, pistes, isolements, options...
*/


#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "id.h"

#include "protos.h"

#include "find.h"

/* Fonctions locales */

/* variables locales */
static wxString s_OldStringFound;
static int s_ItemCount, s_MarkerCount;


/*********************************************************************/
void WinEDA_PcbFrame::InstallFindFrame(const wxPoint & pos, wxDC * DC)
/*********************************************************************/
{
	WinEDA_PcbFindFrame * frame = new WinEDA_PcbFindFrame(this, DC, pos);
	frame->ShowModal(); frame->Destroy();
}



/*******************************************************/
void WinEDA_PcbFindFrame::FindItem(wxCommandEvent& event)
/********************************************************/
{
PCB_SCREEN * screen = m_Parent->GetScreen();
wxPoint locate_pos;
wxString msg;
bool succes = FALSE;
bool FindMarker = FALSE;
MODULE * Module;
int StartCount;
	
	switch ( event.GetId() )
	{
		case ID_FIND_ITEM:
			s_ItemCount = 0;
			break;
		
		case ID_FIND_MARKER: s_MarkerCount = 0;
		case ID_FIND_NEXT_MARKER:
			FindMarker = TRUE;
			break;
	}

	s_OldStringFound = m_NewText->GetValue();

	m_Parent->DrawPanel->GetViewStart(&screen->m_StartVisu.x, &screen->m_StartVisu.y);
	StartCount = 0;
	
	if( FindMarker )
	{
	MARQUEUR * Marker = (MARQUEUR *) m_Parent->m_Pcb->m_Drawings; 
		for( ; Marker != NULL; Marker = (MARQUEUR *)Marker->Pnext)
		{
			if( Marker->m_StructType != TYPEMARQUEUR ) continue;
			StartCount++;
			if ( StartCount > s_MarkerCount )
			{
				succes = TRUE;
				locate_pos = Marker->m_Pos;
				s_MarkerCount++;
				break;
			}
		}
	}
	
	else for ( Module = m_Parent->m_Pcb->m_Modules; Module != NULL; Module = (MODULE*)Module->Pnext)
	{
		if( WildCompareString( s_OldStringFound, Module->m_Reference->m_Text.GetData(), FALSE ) )
		{
			StartCount++;
			if ( StartCount > s_ItemCount )
			{
				succes = TRUE;
				locate_pos = Module->m_Pos;
				s_ItemCount++;
				break;
			}
		}
		if( WildCompareString( s_OldStringFound, Module->m_Value->m_Text.GetData(), FALSE ) )
		{
			StartCount++;
			if ( StartCount > s_ItemCount )
			{
				succes = TRUE;
				locate_pos = Module->m_Pos;
				s_ItemCount++;
				break;
			}
		}
	}
	
	if ( succes )
	{	/* Il y a peut-etre necessite de recadrer le dessin: */		
		if( ! m_Parent->DrawPanel->IsPointOnDisplay(locate_pos) )
		{
			screen->m_Curseur = locate_pos;
			m_Parent->Recadre_Trace(TRUE);
		}
		else
		{	// Positionnement du curseur sur l'item
			m_Parent->DrawPanel->CursorOff(m_DC);
			screen->m_Curseur = locate_pos;
			GRMouseWarp(m_Parent->DrawPanel, screen->m_Curseur );
			m_Parent->DrawPanel->MouseToCursorSchema();
			m_Parent->DrawPanel->CursorOn(m_DC);
		}

		if( FindMarker ) msg = _("Marker found");
		else msg.Printf( _("<%s> Found"), s_OldStringFound.GetData() );
		m_Parent->Affiche_Message(msg);
		EndModal(1);
	}

	else
	{
		m_Parent->Affiche_Message(wxEmptyString);
		if( FindMarker ) msg = _("Marker not found");
		else msg.Printf( _("<%s> Not Found"), s_OldStringFound.GetData());
		DisplayError(this,msg, 10);
        EndModal(0);
	}
}




/*!
 * WinEDA_PcbFindFrame type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WinEDA_PcbFindFrame, wxDialog )

/*!
 * WinEDA_PcbFindFrame event table definition
 */

BEGIN_EVENT_TABLE( WinEDA_PcbFindFrame, wxDialog )

////@begin WinEDA_PcbFindFrame event table entries
    EVT_BUTTON( ID_FIND_ITEM, WinEDA_PcbFindFrame::OnFindItemClick )

    EVT_BUTTON( ID_FIND_NEXT_ITEM, WinEDA_PcbFindFrame::OnFindNextItemClick )

    EVT_BUTTON( ID_FIND_MARKER, WinEDA_PcbFindFrame::OnFindMarkerClick )

    EVT_BUTTON( ID_FIND_NEXT_MARKER, WinEDA_PcbFindFrame::OnFindNextMarkerClick )

////@end WinEDA_PcbFindFrame event table entries

END_EVENT_TABLE()

/*!
 * WinEDA_PcbFindFrame constructors
 */

WinEDA_PcbFindFrame::WinEDA_PcbFindFrame( )
{
}

WinEDA_PcbFindFrame::WinEDA_PcbFindFrame( WinEDA_BasePcbFrame *parent, wxDC * DC,
			 const wxPoint& pos, 
			wxWindowID id, const wxString& caption, const wxSize& size, long style )
{
	m_Parent = parent;
	m_DC = DC;

   Create(parent, id, caption, pos, size, style);

	m_NewText->SetFocus();
}

/*!
 * WinEDA_PcbFindFrame creator
 */

bool WinEDA_PcbFindFrame::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WinEDA_PcbFindFrame member initialisation
    m_NewText = NULL;
////@end WinEDA_PcbFindFrame member initialisation

////@begin WinEDA_PcbFindFrame creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WinEDA_PcbFindFrame creation
    return true;
}

/*!
 * Control creation for WinEDA_PcbFindFrame
 */

void WinEDA_PcbFindFrame::CreateControls()
{    
	SetFont(*g_DialogFont);

////@begin WinEDA_PcbFindFrame content construction
    // Generated by DialogBlocks, 04/03/2006 14:04:20 (unregistered)

    WinEDA_PcbFindFrame* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticText* itemStaticText3 = new wxStaticText( itemDialog1, wxID_STATIC, _("Item to find:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText3, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_NewText = new wxTextCtrl( itemDialog1, ID_TEXTCTRL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(m_NewText, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer5, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer5->Add(itemBoxSizer6, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    wxButton* itemButton7 = new wxButton( itemDialog1, ID_FIND_ITEM, _("Find Item"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton7->SetDefault();
    itemButton7->SetForegroundColour(wxColour(102, 0, 0));
    itemBoxSizer6->Add(itemButton7, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    wxButton* itemButton8 = new wxButton( itemDialog1, ID_FIND_NEXT_ITEM, _("Find Next Item"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton8->SetForegroundColour(wxColour(111, 0, 0));
    itemBoxSizer6->Add(itemButton8, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer5->Add(itemBoxSizer9, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    wxButton* itemButton10 = new wxButton( itemDialog1, ID_FIND_MARKER, _("Find Marker"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton10->SetForegroundColour(wxColour(0, 0, 255));
    itemBoxSizer9->Add(itemButton10, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    wxButton* itemButton11 = new wxButton( itemDialog1, ID_FIND_NEXT_MARKER, _("Find Next Marker"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton11->SetForegroundColour(wxColour(0, 0, 255));
    itemBoxSizer9->Add(itemButton11, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end WinEDA_PcbFindFrame content construction
}

/*!
 * Should we show tooltips?
 */

bool WinEDA_PcbFindFrame::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WinEDA_PcbFindFrame::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WinEDA_PcbFindFrame bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WinEDA_PcbFindFrame bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WinEDA_PcbFindFrame::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WinEDA_PcbFindFrame icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WinEDA_PcbFindFrame icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_ITEM
 */

void WinEDA_PcbFindFrame::OnFindItemClick( wxCommandEvent& event )
{
	FindItem(event);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_NEXT_ITEM
 */

void WinEDA_PcbFindFrame::OnFindNextItemClick( wxCommandEvent& event )
{
	FindItem(event);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_MARKER
 */

void WinEDA_PcbFindFrame::OnFindMarkerClick( wxCommandEvent& event )
{
	FindItem(event);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_NEXT_MARKER
 */

void WinEDA_PcbFindFrame::OnFindNextMarkerClick( wxCommandEvent& event )
{
	FindItem(event);
}


