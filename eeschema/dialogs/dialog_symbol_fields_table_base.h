///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-282-g1fa54006)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class BITMAP_BUTTON;
class WX_GRID;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/dataview.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/srchctrl.h>
#include <wx/checkbox.h>
#include <wx/grid.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/notebook.h>
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
		wxNotebook* m_nbPages;
		wxPanel* m_panelEdit;
		wxSplitterWindow* m_splitterMainWindow;
		wxPanel* m_leftPanel;
		wxStaticText* m_bomPresetsLabel;
		wxChoice* m_cbBomPresets;
		wxDataViewListCtrl* m_fieldsCtrl;
		wxBitmapButton* m_addFieldButton;
		wxBitmapButton* m_removeFieldButton;
		wxBitmapButton* m_renameFieldButton;
		wxPanel* m_rightPanel;
		wxSearchCtrl* m_filter;
		BITMAP_BUTTON* m_separator1;
		wxCheckBox* m_checkExcludeDNP;
		BITMAP_BUTTON* m_separator2;
		wxCheckBox* m_groupSymbolsBox;
		BITMAP_BUTTON* m_separator3;
		wxBitmapButton* m_bRefresh;
		WX_GRID* m_grid;
		wxPanel* m_panelExport;
		wxStaticText* m_labelBomExportPresets;
		wxChoice* m_cbBomFmtPresets;
		wxStaticText* m_labelFieldDelimiter;
		wxTextCtrl* m_textFieldDelimiter;
		wxStaticText* m_labelStringDelimiter;
		wxTextCtrl* m_textStringDelimiter;
		wxStaticText* m_labelRefDelimiter;
		wxTextCtrl* m_textRefDelimiter;
		wxStaticText* m_labelRefRangeDelimiter;
		wxTextCtrl* m_textRefRangeDelimiter;
		wxStaticText* m_labelKeepTabs;
		wxCheckBox* m_checkKeepTabs;
		wxStaticText* m_labelKeepLineBreaks;
		wxCheckBox* m_checkKeepLineBreaks;
		wxStaticText* m_labelOutputDirectory;
		wxTextCtrl* m_outputFileName;
		wxBitmapButton* m_browseButton;
		wxStaticText* m_labelPreview;
		wxBitmapButton* m_bRefreshPreview;
		wxTextCtrl* m_textOutput;
		wxButton* m_buttonExport;
		wxButton* m_buttonApply;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnPageChanged( wxNotebookEvent& event ) { event.Skip(); }
		virtual void OnColumnItemToggled( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnFieldsCtrlSelectionChanged( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnSizeFieldList( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRenameField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterMouseMoved( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnFilterText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExcludeDNPToggled( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGroupSymbolsToggled( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegroupSymbols( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTableValueChanged( wxGridEvent& event ) { event.Skip(); }
		virtual void OnTableCellClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnTableItemContextMenu( wxGridEvent& event ) { event.Skip(); }
		virtual void OnTableColSize( wxGridSizeEvent& event ) { event.Skip(); }
		virtual void OnPreviewRefresh( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOutputFileBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExport( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveAndContinue( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SYMBOL_FIELDS_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Symbol Fields Table"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER );

		~DIALOG_SYMBOL_FIELDS_TABLE_BASE();

};

