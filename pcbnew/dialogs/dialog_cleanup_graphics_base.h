///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/dataview.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CLEANUP_GRAPHICS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CLEANUP_GRAPHICS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxCheckBox* m_createRectanglesOpt;
		wxCheckBox* m_deleteRedundantOpt;
		wxCheckBox* m_mergePadsOpt;
		wxStaticText* m_nettieHint;
		wxCheckBox* m_fixBoardOutlines;
		wxBoxSizer* m_toleranceSizer;
		wxStaticText* m_toleranceLabel;
		wxTextCtrl* m_toleranceCtrl;
		wxStaticText* m_toleranceUnits;
		wxStaticText* staticChangesLabel;
		wxDataViewCtrl* m_changesDataView;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnCheckBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectItem( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnLeftDClickItem( wxMouseEvent& event ) { event.Skip(); }


	public:

		DIALOG_CLEANUP_GRAPHICS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Cleanup Graphics"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_CLEANUP_GRAPHICS_BASE();

};

