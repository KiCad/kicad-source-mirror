///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE_H__
#define __DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE_H__

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
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/radiobox.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		WX_GRID* m_grid;
		wxBitmapButton* m_bpAdd;
		wxBitmapButton* m_bpMoveUp;
		wxBitmapButton* m_bpMoveDown;
		wxBitmapButton* m_bpDelete;
		wxButton* m_updateFieldValues;
		wxStaticText* m_staticText3;
		wxTextCtrl* m_libraryNameTextCtrl;
		wxBitmapButton* m_buttonBrowseLibrary;
		wxStaticText* m_unitLabel;
		wxChoice* m_unitChoice;
		wxCheckBox* m_cbAlternateSymbol;
		wxRadioBox* m_rbOrientation;
		wxRadioBox* m_rbMirror;
		wxStaticLine* m_staticline1;
		wxTextCtrl* m_textCtrlTimeStamp;
		wxButton* m_spiceFieldsButton;
		wxStdDialogButtonSizer* m_stdDialogButtonSizer;
		wxButton* m_stdDialogButtonSizerOK;
		wxButton* m_stdDialogButtonSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnSizeGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteField( wxCommandEvent& event ) { event.Skip(); }
		virtual void UpdateFieldsFromLibrary( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowseLibrary( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditSpiceModel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Symbol Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU ); 
		~DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE();
	
};

#endif //__DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE_H__
