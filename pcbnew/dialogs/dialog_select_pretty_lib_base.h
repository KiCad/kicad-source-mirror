///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_SELECT_PRETTY_LIB_BASE_H__
#define __DIALOG_SELECT_PRETTY_LIB_BASE_H__

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
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SELECT_PRETTY_LIB_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SELECT_PRETTY_LIB_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText;
		wxDirPickerCtrl* m_dirCtrl;
		wxBoxSizer* m_SizerNewLibName;
		wxStaticText* m_staticTextDirname;
		wxTextCtrl* m_libName;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSelectFolder( wxFileDirPickerEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_SELECT_PRETTY_LIB_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Select Footprint Library Folder"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_SELECT_PRETTY_LIB_BASE();
	
};

#endif //__DIALOG_SELECT_PRETTY_LIB_BASE_H__
