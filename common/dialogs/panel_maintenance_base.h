///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_MAINTENANCE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_MAINTENANCE_BASE : public RESETTABLE_PANEL
{
	private:
		wxButton* m_clearDontShowAgain;
		wxButton* m_clearDialogState;
		wxButton* m_resetAll;

	protected:
		wxStaticText* m_staticTextClear3DCache;
		wxSpinCtrl* m_Clear3DCacheFilesOlder;
		wxStaticText* m_staticTextDays;
		wxButton* m_clearFileHistory;

		// Virtual event handlers, override them in your derived class
		virtual void onClearFileHistory( wxCommandEvent& event ) { event.Skip(); }
		virtual void onClearDontShowAgain( wxCommandEvent& event ) { event.Skip(); }
		virtual void onClearDialogState( wxCommandEvent& event ) { event.Skip(); }
		virtual void onResetAll( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_MAINTENANCE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_MAINTENANCE_BASE();

};

