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
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/bmpcbox.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DIMENSION_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DIMENSION_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_mainSizer;
		wxStaticBoxSizer* m_sizerLeader;
		wxStaticText* m_lblLeaderValue;
		wxTextCtrl* m_txtLeaderValue;
		wxStaticText* m_lblTextFrame;
		wxChoice* m_cbTextFrame;
		wxStaticText* m_lblLeaderLayer;
		PCB_LAYER_BOX_SELECTOR* m_cbLeaderLayer;
		wxStaticBoxSizer* m_sizerCenter;
		wxStaticText* m_lblCenterLayer;
		PCB_LAYER_BOX_SELECTOR* m_cbCenterLayer;
		wxStaticBoxSizer* m_sizerFormat;
		wxStaticText* m_lblValue;
		wxTextCtrl* m_txtValue;
		wxCheckBox* m_cbOverrideValue;
		wxStaticText* m_lblUnits;
		wxChoice* m_cbUnits;
		wxStaticText* m_lblPrefix;
		wxTextCtrl* m_txtPrefix;
		wxStaticText* m_txtUnitsFormat;
		wxChoice* m_cbUnitsFormat;
		wxStaticText* m_lblSuffix;
		wxTextCtrl* m_txtSuffix;
		wxStaticText* m_lblPrecision;
		wxChoice* m_cbPrecision;
		wxCheckBox* m_cbSuppressZeroes;
		wxStaticText* m_lblLayer;
		PCB_LAYER_BOX_SELECTOR* m_cbLayer;
		wxStaticText* m_lblPreview;
		wxStaticText* m_staticTextPreview;
		wxStaticBoxSizer* m_sizerText;
		wxStaticText* m_lblTextWidth;
		wxTextCtrl* m_txtTextWidth;
		wxStaticText* m_lblTextWidthUnits;
		wxStaticText* m_lblTextPosX;
		wxTextCtrl* m_txtTextPosX;
		wxStaticText* m_lblTextPosXUnits;
		wxStaticText* m_lblTextHeight;
		wxTextCtrl* m_txtTextHeight;
		wxStaticText* m_lblTextHeightUnits;
		wxStaticText* m_lblTextPosY;
		wxTextCtrl* m_txtTextPosY;
		wxStaticText* m_lblTextPosYUnits;
		wxStaticText* m_lblTextThickness;
		wxTextCtrl* m_txtTextThickness;
		wxStaticText* m_lblTextThicknessUnits;
		wxStaticText* m_lblTextOrientation;
		wxComboBox* m_cbTextOrientation;
		wxCheckBox* m_cbItalic;
		wxCheckBox* m_cbKeepAligned;
		wxCheckBox* m_cbMirrored;
		wxStaticText* m_lblJustification;
		wxChoice* m_cbJustification;
		wxStaticText* m_lblTextPositionMode;
		wxChoice* m_cbTextPositionMode;
		wxStaticText* m_lblLineThickness;
		wxTextCtrl* m_txtLineThickness;
		wxStaticText* m_lblLineThicknessUnits;
		wxStaticText* m_lblArrowLength;
		wxTextCtrl* m_txtArrowLength;
		wxStaticText* m_lblArrowLengthUnits;
		wxStaticText* m_lblExtensionOffset;
		wxTextCtrl* m_txtExtensionOffset;
		wxStaticText* m_lblExtensionOffsetUnits;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnDimensionUnitsChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_DIMENSION_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Dimension Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU );

		~DIALOG_DIMENSION_PROPERTIES_BASE();

};

