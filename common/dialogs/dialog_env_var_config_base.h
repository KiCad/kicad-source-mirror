///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 22 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_ENV_VAR_CONFIG_BASE_H__
#define __DIALOG_ENV_VAR_CONFIG_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/listctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

///////////////////////////////////////////////////////////////////////////

#define ID_BUTTON_ADD_PATH 1000
#define ID_BUTTON_EDIT_PATH 1001
#define ID_BUTTON_DELETE_PATH 1002

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ENV_VAR_CONFIG_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ENV_VAR_CONFIG_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxListCtrl* m_pathList;
		wxButton* m_addPathButton;
		wxButton* m_editPathButton;
		wxButton* m_deletePathButton;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		wxButton* m_sdbSizerHelp;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnPathActivated( wxListEvent& event ) { event.Skip(); }
		virtual void OnPathSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnAddButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelpButton( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_ENV_VAR_CONFIG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Environment Variable Configuration"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_ENV_VAR_CONFIG_BASE();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ENV_VAR_SINGLE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ENV_VAR_SINGLE_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_envVarNameLabel;
		wxTextCtrl* m_envVarName;
		wxButton* m_buttonHelp;
		wxStaticText* m_envVarPathLabel;
		wxTextCtrl* m_envVarPath;
		wxButton* m_selectPathButton;
		wxStaticLine* m_staticline2;
		wxStdDialogButtonSizer* m_sdbSizer2;
		wxButton* m_sdbSizer2OK;
		wxButton* m_sdbSizer2Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onHelpClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectPath( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_ENV_VAR_SINGLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Edit Environment Variable"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_ENV_VAR_SINGLE_BASE();
	
};

#endif //__DIALOG_ENV_VAR_CONFIG_BASE_H__
