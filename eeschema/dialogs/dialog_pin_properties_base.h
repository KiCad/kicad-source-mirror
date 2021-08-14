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
class PinShapeComboBox;
class PinTypeComboBox;
class WX_GRID;
class wxBitmapComboBox;

#include "dialog_shim.h"
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/gbsizer.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/collpane.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PIN_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PIN_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_bSizerInfo;
		wxStaticBitmap* m_bitmapInfo;
		wxStaticText* m_textInfoUpper;
		wxStaticText* m_textInfoLower;
		wxStaticText* m_pinNameLabel;
		wxTextCtrl* m_textPinName;
		wxStaticText* m_pinNumberLabel;
		wxTextCtrl* m_textPinNumber;
		wxStaticText* m_posXLabel;
		wxTextCtrl* m_posXCtrl;
		wxStaticText* m_posYLabel;
		wxTextCtrl* m_posYCtrl;
		wxStaticText* m_pinLengthLabel;
		wxTextCtrl* m_pinLengthCtrl;
		wxStaticText* m_nameSizeLabel;
		wxTextCtrl* m_nameSizeCtrl;
		wxStaticText* m_numberSizeLabel;
		wxTextCtrl* m_numberSizeCtrl;
		wxStaticText* m_staticTextEType;
		PinTypeComboBox* m_choiceElectricalType;
		wxStaticText* m_staticTextGstyle;
		PinShapeComboBox* m_choiceStyle;
		wxStaticText* m_staticTextOrient;
		wxBitmapComboBox* m_choiceOrientation;
		wxStaticText* m_posXUnits;
		wxStaticText* m_posYUnits;
		wxStaticText* m_pinLengthUnits;
		wxStaticText* m_nameSizeUnits;
		wxStaticText* m_numberSizeUnits;
		wxCheckBox* m_checkApplyToAllParts;
		wxCheckBox* m_checkApplyToAllConversions;
		wxCheckBox* m_checkShow;
		wxStaticText* m_staticText16;
		wxPanel* m_panelShowPin;
		wxCollapsiblePane* m_alternatesTurndown;
		WX_GRID* m_alternatesGrid;
		wxBitmapButton* m_addAlternate;
		wxBitmapButton* m_deleteAlternate;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateUIInfo( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnPropertiesChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPaintShowPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnAddAlternate( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteAlternate( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_PIN_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pin Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_PIN_PROPERTIES_BASE();

};

