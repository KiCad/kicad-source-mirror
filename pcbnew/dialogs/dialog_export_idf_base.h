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
class TEXT_CTRL_EVAL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/filepicker.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXPORT_IDF3_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXPORT_IDF3_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_txtBrdFile;
		wxFilePickerCtrl* m_filePickerIDF;
		wxCheckBox* m_cbSetBoardReferencePoint;
		wxStaticText* m_xLabel;
		TEXT_CTRL_EVAL* m_IDF_Xref;
		wxStaticText* m_xUnits;
		wxStaticText* m_yLabel;
		TEXT_CTRL_EVAL* m_IDF_Yref;
		wxStaticText* m_yUnits;
		wxStaticText* m_outputUnitsLabel;
		wxChoice* m_outputUnitsChoice;
		wxCheckBox* m_cbRemoveDNP;
		wxCheckBox* m_cbRemoveUnspecified;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

	public:

		DIALOG_EXPORT_IDF3_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Export IDFv3"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_EXPORT_IDF3_BASE();

};

