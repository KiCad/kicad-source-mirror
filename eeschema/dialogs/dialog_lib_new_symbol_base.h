///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
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
#include <widgets/symbol_filter_combobox.h>
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
		void _wxFB_onPowerCheckBox( wxCommandEvent& event ){ onPowerCheckBox( event ); }
		void _wxFB_onCheckTransferUserFields( wxCommandEvent& event ){ onCheckTransferUserFields( event ); }


	protected:
		WX_INFOBAR* m_infoBar;
		wxStaticText* m_staticTextName;
		wxTextCtrl* m_textName;
		wxStaticText* m_staticText5;
		SYMBOL_FILTER_COMBOBOX* m_comboInheritanceSelect;
		wxStaticText* m_staticTextDes;
		wxTextCtrl* m_textReference;
		wxStaticText* m_staticTextUnits;
		wxSpinCtrl* m_spinPartCount;
		wxCheckBox* m_checkUnitsInterchangeable;
		wxCheckBox* m_checkHasAlternateBodyStyle;
		wxCheckBox* m_checkIsPowerSymbol;
		wxCheckBox* m_excludeFromBomCheckBox;
		wxCheckBox* m_excludeFromBoardCheckBox;
		wxCheckBox* m_checkKeepDatasheet;
		wxCheckBox* m_checkKeepFootprint;
		wxCheckBox* m_checkTransferUserFields;
		wxCheckBox* m_checkKeepContentUserFields;
		wxStaticText* m_staticPinTextPositionLabel;
		wxTextCtrl* m_textPinTextPosition;
		wxStaticText* m_staticPinTextPositionUnits;
		wxCheckBox* m_checkShowPinNumber;
		wxCheckBox* m_checkShowPinName;
		wxCheckBox* m_checkShowPinNameInside;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onPowerCheckBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckTransferUserFields( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_LIB_NEW_SYMBOL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("New Symbol"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_LIB_NEW_SYMBOL_BASE();

};

