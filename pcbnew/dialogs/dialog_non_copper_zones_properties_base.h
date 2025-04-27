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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/dataview.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/statbox.h>
#include <wx/spinctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextLayerSelection;
		wxDataViewListCtrl* m_layers;
		wxCheckBox* m_cbLocked;
		wxStaticText* m_staticTextStyle;
		wxChoice* m_OutlineDisplayCtrl;
		wxStaticText* m_stBorderHatchPitchText;
		wxTextCtrl* m_outlineHatchPitchCtrl;
		wxStaticText* m_outlineHatchUnits;
		wxStaticText* m_MinWidthLabel;
		wxTextCtrl* m_MinWidthCtrl;
		wxStaticText* m_MinWidthUnits;
		wxStaticText* m_staticTextSmoothing;
		wxChoice* m_cornerSmoothingChoice;
		wxStaticText* m_cornerRadiusLabel;
		wxTextCtrl* m_cornerRadiusCtrl;
		wxStaticText* m_cornerRadiusUnits;
		wxStaticText* m_staticTextGridFillType;
		wxChoice* m_GridStyleCtrl;
		wxStaticText* m_hatchOrientLabel;
		wxTextCtrl* m_hatchOrientCtrl;
		wxStaticText* m_hatchOrientUnits;
		wxStaticText* m_hatchWidthLabel;
		wxTextCtrl* m_hatchWidthCtrl;
		wxStaticText* m_hatchWidthUnits;
		wxStaticText* m_hatchGapLabel;
		wxTextCtrl* m_hatchGapCtrl;
		wxStaticText* m_hatchGapUnits;
		wxStaticText* m_smoothLevelLabel;
		wxSpinCtrl* m_spinCtrlSmoothLevel;
		wxStaticText* m_smoothValueLabel;
		wxSpinCtrlDouble* m_spinCtrlSmoothValue;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnLayerSelection( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnStyleSelection( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Non Copper Zone Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxBORDER_SUNKEN );

		~DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE();

};

