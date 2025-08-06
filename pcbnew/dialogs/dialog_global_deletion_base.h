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
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GLOBAL_DELETION_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GLOBAL_DELETION_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxCheckBox* m_delZones;
		wxCheckBox* m_delTexts;
		wxCheckBox* m_delBoardEdges;
		wxCheckBox* m_delDrawings;
		wxCheckBox* m_delFootprints;
		wxCheckBox* m_delTracks;
		wxCheckBox* m_delTeardrops;
		wxCheckBox* m_delMarkers;
		wxCheckBox* m_delAll;
		wxStaticBoxSizer* sbFilter;
		wxCheckBox* m_drawingFilterLocked;
		wxCheckBox* m_drawingFilterUnlocked;
		wxCheckBox* m_footprintFilterLocked;
		wxCheckBox* m_footprintFilterUnlocked;
		wxCheckBox* m_trackFilterLocked;
		wxCheckBox* m_trackFilterUnlocked;
		wxCheckBox* m_viaFilterLocked;
		wxCheckBox* m_viaFilterUnlocked;
		wxRadioBox* m_rbLayersOption;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void onCheckDeleteBoardOutlines( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckDeleteDrawings( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckDeleteFootprints( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckDeleteTracks( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_GLOBAL_DELETION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Delete Items"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_GLOBAL_DELETION_BASE();

};

