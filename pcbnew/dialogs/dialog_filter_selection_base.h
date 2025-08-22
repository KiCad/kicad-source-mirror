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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FILTER_SELECTION_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FILTER_SELECTION_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxCheckBox* m_All_Items;
		wxCheckBox* m_Hidden_Spacer;
		wxCheckBox* m_Include_Modules;
		wxCheckBox* m_Include_PcbTexts;
		wxCheckBox* m_IncludeLockedModules;
		wxCheckBox* m_Include_Draw_Items;
		wxCheckBox* m_Include_Tracks;
		wxCheckBox* m_Include_Edges_Items;
		wxCheckBox* m_Include_Vias;
		wxCheckBox* m_Include_Zones;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void allItemsClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void checkBoxClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ExecuteCommand( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_FILTER_SELECTION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Filter Selected Items"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_FILTER_SELECTION_BASE();

};

