///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jan 17 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class TEXT_CTRL_EVAL;
class WX_HTML_REPORT_PANEL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/filepicker.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXPORT_STEP_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXPORT_STEP_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_txtBrdFile;
		wxFilePickerCtrl* m_filePickerSTEP;
		wxStaticLine* m_staticline2;
		wxStaticText* m_staticText6;
		wxRadioButton* m_rbDrillAndPlotOrigin;
		wxRadioButton* m_rbGridOrigin;
		wxRadioButton* m_rbUserDefinedOrigin;
		wxRadioButton* m_rbBoardCenterOrigin;
		wxStaticText* m_staticText2;
		wxStaticText* m_staticText5;
		wxChoice* m_STEP_OrgUnitChoice;
		wxStaticText* m_staticText3;
		TEXT_CTRL_EVAL* m_STEP_Xorg;
		wxStaticText* m_staticText4;
		TEXT_CTRL_EVAL* m_STEP_Yorg;
		wxStaticText* m_staticText7;
		wxCheckBox* m_cbRemoveVirtual;
		wxStaticText* m_staticText8;
		wxChoice* m_tolerance;
		WX_HTML_REPORT_PANEL* m_messagesPanel;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onUpdateUnits( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateXPos( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateYPos( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onExportButton( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_EXPORT_STEP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Export STEP"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 650,361 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_EXPORT_STEP_BASE();

};

