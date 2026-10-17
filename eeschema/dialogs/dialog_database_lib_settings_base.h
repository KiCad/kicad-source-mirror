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
class WX_INFOBAR;

#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/radiobut.h>
#include <wx/combobox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/scrolwin.h>
#include <wx/listbox.h>
#include <wx/bmpbuttn.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DATABASE_LIB_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DATABASE_LIB_SETTINGS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_INFOBAR* m_infoBar;
		wxSplitterWindow* m_splitter;
		wxPanel* m_editPanel;
		wxNotebook* m_notebook;
		wxScrolledWindow* m_connectionPage;
		wxTextCtrl* m_txtName;
		wxTextCtrl* m_txtDescription;
		wxRadioButton* m_rbDSN;
		wxComboBox* m_cbDSN;
		wxStaticText* m_stDsnDriver;
		wxButton* m_btnManageDsn;
		wxTextCtrl* m_txtUser;
		wxTextCtrl* m_txtPassword;
		wxCheckBox* m_cbPasswordInFile;
		wxRadioButton* m_rbConnectionString;
		wxTextCtrl* m_txtConnectionString;
		wxChoice* m_choiceDriver;
		wxStaticText* m_stCwdHelp;
		wxSpinCtrl* m_spinTimeout;
		wxButton* m_btnTest;
		wxStaticText* m_stConnectionStatus;
		wxSpinCtrl* m_spinCacheSize;
		wxSpinCtrl* m_spinCacheAge;
		wxCheckBox* m_cbGloballyUniqueKeys;
		wxScrolledWindow* m_librariesPage;
		wxStaticText* m_stNoLibraries;
		wxButton* m_btnAddFirstLibrary;
		wxListBox* m_libList;
		STD_BITMAP_BUTTON* m_btnAddLibrary;
		STD_BITMAP_BUTTON* m_btnDuplicateLibrary;
		STD_BITMAP_BUTTON* m_btnMoveLibraryUp;
		STD_BITMAP_BUTTON* m_btnMoveLibraryDown;
		STD_BITMAP_BUTTON* m_btnRemoveLibrary;
		wxTextCtrl* m_txtLibName;
		wxStaticText* m_stSymbolExample;
		wxComboBox* m_cbTable;
		wxComboBox* m_cbKeyCol;
		wxComboBox* m_cbSymbolsCol;
		wxComboBox* m_cbFootprintsCol;
		wxComboBox* m_cbPinsCol;
		wxStaticText* m_stRequiredNote;
		wxComboBox* m_cbDescriptionCol;
		wxComboBox* m_cbKeywordsCol;
		wxComboBox* m_cbFpFiltersCol;
		wxComboBox* m_cbExclSimCol;
		wxComboBox* m_cbExclBomCol;
		wxComboBox* m_cbExclBoardCol;
		WX_GRID* m_fieldsGrid;
		STD_BITMAP_BUTTON* m_btnAddField;
		STD_BITMAP_BUTTON* m_btnMoveFieldUp;
		STD_BITMAP_BUTTON* m_btnMoveFieldDown;
		STD_BITMAP_BUTTON* m_btnRemoveField;
		wxButton* m_btnAddUnmapped;
		wxPanel* m_dataPage;
		WX_GRID* m_dataGrid;
		wxStaticText* m_stDataStatus;
		wxPanel* m_previewPanel;
		wxBoxSizer* m_previewSizer;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnSettingChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSourceTypeChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSourceChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnManageDataSources( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnInsertDriver( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSourceSpinChanged( wxSpinEvent& event ) { event.Skip(); }
		virtual void OnTestConnection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSettingSpinChanged( wxSpinEvent& event ) { event.Skip(); }
		virtual void OnAddLibrary( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLibrarySelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDuplicateLibrary( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveLibraryUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveLibraryDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveLibrary( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLibraryChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTableChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFieldCellChanged( wxGridEvent& event ) { event.Skip(); }
		virtual void OnAddField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveFieldUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveFieldDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddUnmappedColumns( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDataLabelRightClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnDataCellSelected( wxGridEvent& event ) { event.Skip(); }


	public:

		DIALOG_DATABASE_LIB_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Database Library Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1200,760 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_DATABASE_LIB_SETTINGS_BASE();

		void m_splitterOnIdle( wxIdleEvent& )
		{
			m_splitter->SetSashPosition( 620 );
			m_splitter->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::m_splitterOnIdle ), NULL, this );
		}

};

