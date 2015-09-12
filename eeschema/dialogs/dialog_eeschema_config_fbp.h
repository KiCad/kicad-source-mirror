///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EESCHEMA_CONFIG_FBP_H__
#define __DIALOG_EESCHEMA_CONFIG_FBP_H__

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
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EESCHEMA_CONFIG_FBP
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EESCHEMA_CONFIG_FBP : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			ID_ADD_LIB = 1000,
			ID_REMOVE_LIB,
			ID_LIB_PATH_SEL,
			wxID_INSERT_PATH,
			wxID_REMOVE_PATH
		};
		
		wxStaticText* m_staticTextLibsList;
		wxListBox* m_ListLibr;
		wxButton* m_buttonAddLib;
		wxButton* m_buttonIns;
		wxButton* m_buttonRemoveLib;
		wxButton* m_buttonUp;
		wxButton* m_buttonDown;
		wxStaticText* m_staticTextPaths;
		wxListBox* m_listUserPaths;
		wxButton* m_buttonAddPath;
		wxButton* m_buttonInsPath;
		wxButton* m_buttonRemovePath;
		wxStaticText* m_staticTextPathlist;
		wxListBox* m_DefaultLibraryPathslistBox;
		wxCheckBox* m_cbRescue;
		wxStaticLine* m_staticline3;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnFilesListClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddOrInsertLibClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveLibClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonUpClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonDownClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddOrInsertPath( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveUserPath( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		DIALOG_EESCHEMA_CONFIG_FBP( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_EESCHEMA_CONFIG_FBP();
	
};

#endif //__DIALOG_EESCHEMA_CONFIG_FBP_H__
