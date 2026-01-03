///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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
#include <wx/srchctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/dataview.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IMPORT_SYMBOL_SELECT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IMPORT_SYMBOL_SELECT_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxSearchCtrl* m_searchCtrl;
		wxDataViewListCtrl* m_symbolList;
		wxButton* m_selectAllButton;
		wxButton* m_selectNoneButton;
		wxStaticText* m_unitLabel;
		wxChoice* m_unitChoice;
		wxPanel* m_previewPanel;
		wxBoxSizer* m_previewSizer;
		wxStaticText* m_statusLine;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnFilterTextChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSymbolSelected( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnSelectAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectNone( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUnitChanged( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_IMPORT_SYMBOL_SELECT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Import Symbols from %s"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 900,650 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_IMPORT_SYMBOL_SELECT_BASE();

};

