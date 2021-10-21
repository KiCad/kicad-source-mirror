///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
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
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SYMBOL_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SYMBOL_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_notebook1;
		wxPanel* generalPage;
		WX_GRID* m_fieldsGrid;
		wxBitmapButton* m_bpAdd;
		wxBitmapButton* m_bpMoveUp;
		wxBitmapButton* m_bpMoveDown;
		wxBitmapButton* m_bpDelete;
		wxStaticText* m_unitLabel;
		wxChoice* m_unitChoice;
		wxCheckBox* m_cbAlternateSymbol;
		wxStaticText* m_orientationLabel;
		wxChoice* m_orientationCtrl;
		wxStaticText* m_mirrorLabel;
		wxChoice* m_mirrorCtrl;
		wxCheckBox* m_ShowPinNumButt;
		wxCheckBox* m_ShowPinNameButt;
		wxCheckBox* m_cbExcludeFromBom;
		wxCheckBox* m_cbExcludeFromBoard;
		wxButton* m_updateSymbolBtn;
		wxButton* m_changeSymbolBtn;
		wxButton* m_editSchematicSymbolBtn;
		wxButton* m_editLibrarySymbolBtn;
		wxPanel* m_pinTablePage;
		WX_GRID* m_pinGrid;
		wxStaticText* m_libraryIDLabel;
		wxTextCtrl* m_tcLibraryID;
		wxButton* m_spiceFieldsButton;
		wxStdDialogButtonSizer* m_stdDialogButtonSizer;
		wxButton* m_stdDialogButtonSizerOK;
		wxButton* m_stdDialogButtonSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnGridEditorShown( wxGridEvent& event ) { event.Skip(); }
		virtual void OnSizeGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateSymbol( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExchangeSymbol( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditSymbol( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditLibrarySymbol( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPinTableCellEdited( wxGridEvent& event ) { event.Skip(); }
		virtual void OnEditSpiceModel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SYMBOL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Symbol Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU );

		~DIALOG_SYMBOL_PROPERTIES_BASE();

};

