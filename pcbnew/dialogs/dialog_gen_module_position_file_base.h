///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GEN_MODULE_POSITION_FILE_BASE_H__
#define __DIALOG_GEN_MODULE_POSITION_FILE_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/statbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GEN_MODULE_POSITION_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GEN_MODULE_POSITION_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxBoxSizer* m_MainSizer;
		wxStaticText* m_staticTextDir;
		wxTextCtrl* m_outputDirectoryName;
		wxButton* m_browseButton;
		wxRadioBox* m_radioBoxUnits;
		wxRadioBox* m_radioBoxFilesCount;
		wxRadioBox* m_radioBoxForceSmd;
		wxTextCtrl* m_messagesBox;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKButton( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GEN_MODULE_POSITION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Position Files:"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 510,351 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GEN_MODULE_POSITION_BASE();
	
};

#endif //__DIALOG_GEN_MODULE_POSITION_FILE_BASE_H__
