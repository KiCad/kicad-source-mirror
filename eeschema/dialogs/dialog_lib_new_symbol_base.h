///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun  3 2020)
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
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_NEW_SYMBOL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_NEW_SYMBOL_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:

		// Private event handlers
		void _wxFB_OnParentSymbolSelect( wxCommandEvent& event ){ OnParentSymbolSelect( event ); }
		void _wxFB_onPowerCheckBox( wxCommandEvent& event ){ onPowerCheckBox( event ); }


	protected:
		wxStaticText* m_staticTextName;
		wxTextCtrl* m_textName;
		wxStaticText* m_staticText5;
		wxComboBox* m_comboInheritanceSelect;
		wxStaticText* m_staticTextDes;
		wxTextCtrl* m_textReference;
		wxStaticText* m_staticTextUnits;
		wxSpinCtrl* m_spinPartCount;
		wxCheckBox* m_checkLockItems;
		wxCheckBox* m_checkHasConversion;
		wxCheckBox* m_checkIsPowerSymbol;
		wxCheckBox* m_excludeFromBomCheckBox;
		wxCheckBox* m_excludeFromBoardCheckBox;
		wxStaticText* m_staticPinTextPositionLabel;
		wxTextCtrl* m_textPinTextPosition;
		wxStaticText* m_staticPinTextPositionUnits;
		wxCheckBox* m_checkShowPinNumber;
		wxCheckBox* m_checkShowPinName;
		wxCheckBox* m_checkShowPinNameInside;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnParentSymbolSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPowerCheckBox( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_LIB_NEW_SYMBOL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("New Symbol"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_LIB_NEW_SYMBOL_BASE();

};

