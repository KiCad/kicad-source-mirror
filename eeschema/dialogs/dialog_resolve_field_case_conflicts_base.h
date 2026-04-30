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
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/grid.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_headerLabel;
		wxGrid* m_conflictsGrid;
		wxCheckBox* m_bulkApplyCheckbox;
		wxStaticText* m_separatorLabel;
		wxTextCtrl* m_separatorCtrl;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_stdButtonsSizer;
		wxButton* m_stdButtonsSizerOK;
		wxButton* m_stdButtonsSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void onActionCellChanged( wxGridEvent& event ) { event.Skip(); }
		virtual void onBulkApplyToggled( wxCommandEvent& event ) { event.Skip(); }
		virtual void onApplyAndContinue( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Resolve Field Name Conflicts"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

		~DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE();

};

