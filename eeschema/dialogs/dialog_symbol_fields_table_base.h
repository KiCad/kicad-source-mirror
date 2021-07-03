///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_GRID;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/dataview.h>
#include <wx/panel.h>
#include <wx/grid.h>
#include <wx/splitter.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define OPT_GROUP_COMPONENTS 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SYMBOL_FIELDS_TABLE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SYMBOL_FIELDS_TABLE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxSplitterWindow* m_splitterMainWindow;
		wxPanel* m_leftPanel;
		wxCheckBox* m_groupSymbolsBox;
		wxBitmapButton* m_bRefresh;
		wxDataViewListCtrl* m_fieldsCtrl;
		wxButton* m_addFieldButton;
		wxPanel* m_rightPanel;
		WX_GRID* m_grid;
		wxButton* m_buttonApply;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnGroupSymbolsToggled( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegroupSymbols( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnColumnItemToggled( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnSizeFieldList( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTableValueChanged( wxGridEvent& event ) { event.Skip(); }
		virtual void OnTableCellClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnTableItemContextMenu( wxGridEvent& event ) { event.Skip(); }
		virtual void OnTableColSize( wxGridSizeEvent& event ) { event.Skip(); }
		virtual void OnSaveAndContinue( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SYMBOL_FIELDS_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Symbol Fields Table"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER );
		~DIALOG_SYMBOL_FIELDS_TABLE_BASE();

};

