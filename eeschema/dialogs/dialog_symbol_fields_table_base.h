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
class STD_BITMAP_BUTTON;
class WX_GRID;

#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
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
#include <wx/listbox.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SYMBOL_FIELDS_TABLE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SYMBOL_FIELDS_TABLE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxSplitterWindow* m_splitterMainWindow;
		wxPanel* m_leftPanel;
		wxBoxSizer* bLeftSizer;
		wxSplitterWindow* m_splitter_left;
		wxPanel* m_viewControlsPanel;
		WX_GRID* m_viewControlsGrid;
		STD_BITMAP_BUTTON* m_addFieldButton;
		STD_BITMAP_BUTTON* m_renameFieldButton;
		STD_BITMAP_BUTTON* m_removeFieldButton;
		wxStaticLine* m_staticline11;
		wxStaticText* m_bomPresetsLabel;
		wxChoice* m_cbBomPresets;
		wxPanel* m_variantsPanel;
		wxBoxSizer* bMargins2;
		wxStaticText* m_staticText9;
		wxListBox* m_variantListBox;
		STD_BITMAP_BUTTON* m_addVariantButton;
		STD_BITMAP_BUTTON* m_renameVariantButton;
		STD_BITMAP_BUTTON* m_copyVariantButton;
		STD_BITMAP_BUTTON* m_deleteVariantButton;
		wxPanel* m_rightPanel;
		wxNotebook* m_nbPages;
		wxPanel* m_panelEdit;
		wxSearchCtrl* m_filter;
		wxStaticLine* m_staticline31;
		wxChoice* m_scope;
		wxStaticLine* m_staticline311;
		wxCheckBox* m_groupSymbolsBox;
		wxStaticLine* m_staticline3;
		STD_BITMAP_BUTTON* m_bRefresh;
		STD_BITMAP_BUTTON* m_bMenu;
		WX_GRID* m_grid;
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
		STD_BITMAP_BUTTON* m_sidebarButton;
		wxButton* m_buttonExport;
		wxButton* m_buttonApply;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnViewControlsCellChanged( wxGridEvent& event ) { event.Skip(); }
		virtual void OnSizeViewControlsGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRenameField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveField( wxCommandEvent& event ) { event.Skip(); }
		virtual void onVariantSelectionChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onAddVariant( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRenameVariant( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCopyVariant( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDeleteVariant( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPageChanged( wxNotebookEvent& event ) { event.Skip(); }
		virtual void OnFilterMouseMoved( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnFilterText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnScope( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGroupSymbolsToggled( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegroupSymbols( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenu( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTableValueChanged( wxGridEvent& event ) { event.Skip(); }
		virtual void OnTableCellClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnTableColSize( wxGridSizeEvent& event ) { event.Skip(); }
		virtual void OnPreviewRefresh( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOutputFileBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSidebarToggle( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExport( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveAndContinue( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOk( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SYMBOL_FIELDS_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Symbol Fields Table"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER );

		~DIALOG_SYMBOL_FIELDS_TABLE_BASE();

};

