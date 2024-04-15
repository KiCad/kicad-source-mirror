///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class STD_BITMAP_BUTTON;
class WX_GRID;

#include "dialog_shim.h"
#include <wx/dataview.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/srchctrl.h>
#include <wx/checkbox.h>
#include <wx/grid.h>
#include <wx/radiobut.h>
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
		wxDataViewListCtrl* m_fieldsCtrl;
		STD_BITMAP_BUTTON* m_addFieldButton;
		STD_BITMAP_BUTTON* m_renameFieldButton;
		STD_BITMAP_BUTTON* m_removeFieldButton;
		wxStaticLine* m_staticline1;
		wxStaticText* m_bomPresetsLabel;
		wxChoice* m_cbBomPresets;
		wxPanel* m_rightPanel;
		wxSearchCtrl* m_filter;
		wxStaticLine* m_staticline31;
		wxCheckBox* m_checkExcludeDNP;
		wxCheckBox* m_checkShowExcluded;
		wxStaticLine* m_staticline32;
		wxCheckBox* m_groupSymbolsBox;
		wxStaticLine* m_staticline3;
		STD_BITMAP_BUTTON* m_bRefresh;
		WX_GRID* m_grid;
		wxStaticLine* m_staticline7;
		wxStaticText* m_scopeLabel;
		wxRadioButton* m_radioProject;
		wxRadioButton* m_radioCurrentSheet;
		wxRadioButton* m_radioRecursive;
		wxStaticText* m_crossProbeLabel;
		wxRadioButton* m_radioHighlight;
		wxRadioButton* m_radioSelect;
		wxRadioButton* m_radioOff;
		wxPanel* m_panelExport;
		wxStaticText* m_labelFieldDelimiter;
		wxTextCtrl* m_textFieldDelimiter;
		wxStaticText* m_labelStringDelimiter;
		wxTextCtrl* m_textStringDelimiter;
		wxStaticText* m_labelRefDelimiter;
		wxTextCtrl* m_textRefDelimiter;
		wxStaticText* m_labelRefRangeDelimiter;
		wxTextCtrl* m_textRefRangeDelimiter;
		wxCheckBox* m_checkKeepTabs;
		wxCheckBox* m_checkKeepLineBreaks;
		wxStaticLine* m_staticline2;
		wxStaticText* m_labelBomExportPresets;
		wxChoice* m_cbBomFmtPresets;
		wxStaticText* m_labelOutputDirectory;
		wxTextCtrl* m_outputFileName;
		STD_BITMAP_BUTTON* m_browseButton;
		wxStaticText* m_labelPreview;
		STD_BITMAP_BUTTON* m_bRefreshPreview;
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
		virtual void OnRenameField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterMouseMoved( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnFilterText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExcludeDNPToggled( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowExcludedToggled( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGroupSymbolsToggled( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegroupSymbols( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTableValueChanged( wxGridEvent& event ) { event.Skip(); }
		virtual void OnTableCellClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnTableItemContextMenu( wxGridEvent& event ) { event.Skip(); }
		virtual void OnTableColSize( wxGridSizeEvent& event ) { event.Skip(); }
		virtual void OnScopeChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPreviewRefresh( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOutputFileBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExport( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveAndContinue( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOk( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SYMBOL_FIELDS_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Symbol Fields Table"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER );

		~DIALOG_SYMBOL_FIELDS_TABLE_BASE();

};

