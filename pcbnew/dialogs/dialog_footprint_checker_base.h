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
#include <wx/dataview.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <widgets/number_badge.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FOOTPRINT_CHECKER_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FOOTPRINT_CHECKER_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxDataViewCtrl* m_markersDataView;
		wxStaticText* m_showLabel;
		wxCheckBox* m_showAll;
		wxCheckBox* m_showErrors;
		NUMBER_BADGE* m_errorsBadge;
		wxCheckBox* m_showWarnings;
		NUMBER_BADGE* m_warningsBadge;
		wxCheckBox* m_showExclusions;
		NUMBER_BADGE* m_exclusionsBadge;
		wxButton* m_DeleteCurrentMarkerButton;
		wxButton* m_DeleteAllMarkersButton;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnSelectItem( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnLeftDClickItem( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSeverity( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteOneClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteAllClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRunChecksClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_FOOTPRINT_CHECKER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint Checker"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_FOOTPRINT_CHECKER_BASE();

};

