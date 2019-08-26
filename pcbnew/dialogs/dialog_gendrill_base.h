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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/checkbox.h>
#include <wx/radiobox.h>
#include <wx/statbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GENDRILL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GENDRILL_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* staticTextOutputDir;
		wxTextCtrl* m_outputDirectoryName;
		wxBitmapButton* m_browseButton;
		wxRadioButton* m_rbExcellon;
		wxCheckBox* m_Check_Mirror;
		wxCheckBox* m_Check_Minimal;
		wxCheckBox* m_Check_Merge_PTH_NPTH;
		wxRadioBox* m_radioBoxOvalHoleMode;
		wxRadioButton* m_rbGerberX2;
		wxRadioBox* m_Choice_Drill_Map;
		wxRadioBox* m_Choice_Drill_Offset;
		wxRadioBox* m_Choice_Unit;
		wxRadioBox* m_Choice_Zeros_Format;
		wxStaticText* m_staticTextTitle;
		wxStaticText* m_staticTextPrecision;
		wxStaticText* staticTextPlatedPads;
		wxStaticText* m_PlatedPadsCountInfoMsg;
		wxStaticText* staticTextNonPlatedPads;
		wxStaticText* m_NotPlatedPadsCountInfoMsg;
		wxStaticText* staticTextThroughVias;
		wxStaticText* m_ThroughViasInfoMsg;
		wxStaticText* staticTextMicroVias;
		wxStaticText* m_MicroViasInfoMsg;
		wxStaticText* staticTextBuriedVias;
		wxStaticText* m_BuriedViasInfoMsg;
		wxTextCtrl* m_messagesBox;
		wxBoxSizer* m_buttonsSizer;
		wxButton* m_buttonReport;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerApply;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onCloseDlg( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onFileFormatSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelDrillUnitsSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelZerosFmtSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenReportFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenMapFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void onQuitDlg( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenDrillFile( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_GENDRILL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Generate Drill Files"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_GENDRILL_BASE();

};

