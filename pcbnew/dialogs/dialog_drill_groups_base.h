///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/wx_grid.h"
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DRILL_GROUPS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DRILL_GROUPS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_profileLabel;
		wxStaticText* m_profileName;
		wxCheckBox* m_freezeAssignments;
		WX_GRID* m_groupGrid;
		wxStaticText* m_markLabel;
		wxChoice* m_markCtrl;
		wxStaticText* m_shapeLabel;
		wxSpinCtrl* m_shapeCtrl;
		wxStaticText* m_letterLabel;
		wxTextCtrl* m_letterCtrl;
		wxStaticText* m_descriptionLabel;
		wxTextCtrl* m_descriptionCtrl;
		wxButton* m_flashButton;
		wxButton* m_closeButton;

		// Virtual event handlers, override them in your derived class
		virtual void onFreezeChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onFlash( wxCommandEvent& event ) { event.Skip(); }
		virtual void onClose( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_DRILL_GROUPS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Drill Groups"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 760,620 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_DRILL_GROUPS_BASE();

};

