///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_export_3Dfiles_base__
#define __dialog_export_3Dfiles_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/filepicker.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXPORT_3DFILE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXPORT_3DFILE_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxFilePickerCtrl* m_filePicker;
		wxStaticText* m_staticText3;
		wxTextCtrl* m_SubdirNameCtrl;
		wxRadioBox* m_rbSelectUnits;
		wxRadioBox* m_rb3DFilesOption;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_EXPORT_3DFILE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 370,252 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_EXPORT_3DFILE_BASE();
	
};

#endif //__dialog_export_3Dfiles_base__
