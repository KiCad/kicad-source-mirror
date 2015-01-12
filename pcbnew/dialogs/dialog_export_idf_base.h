///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EXPORT_IDF_BASE_H__
#define __DIALOG_EXPORT_IDF_BASE_H__

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
#include <wx/filepicker.h>
#include <wx/radiobox.h>
#include <wx/statline.h>
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
		wxRadioBox* m_rbUnitSelection;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
	
	public:
		
		DIALOG_EXPORT_IDF3_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Export IDFv3"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_EXPORT_IDF3_BASE();
	
};

#endif //__DIALOG_EXPORT_IDF_BASE_H__
